#include "timeout_handler.h"
#include <sys/socket.h>

static void expiration_handler(void) {
    uint64_t num_exp;
    struct timeout_handler *timeout_handler = (struct timeout_handler *)current_ctx->data.ptr;
    struct timeval when = {timeout_handler->timeout/1000,(timeout_handler->timeout%1000)*1000};
    int fd = current_ctx->fd;
    for (;;yield()) {
        if (sizeof(num_exp) != read(fd, &num_exp, sizeof(num_exp)))
            continue;
        struct timeval now, ts;
        gettimeofday(&now, NULL);
        timersub(&now, &when, &ts);
        struct list *fd_data_list;
        LIST_FOR_EACH(&timeout_handler->timeout_chain, fd_data_list) {
            struct epoll_worker_fd_data *fd_data = LIST_ENTRY(fd_data_list, struct epoll_worker_fd_data, timeout_chain);
            if (timercmp(&fd_data->timestamp, &ts, >)) {
                timersub(&fd_data->timestamp, &ts, &now);
                struct itimerspec when = {{0,0},{now.tv_sec,now.tv_usec*1000}};
                if (0 > timerfd_settime(timeout_handler->timeout_handler_ctx->fd, 0, &when, NULL))
                    LOGGER_PERROR("timerfd_settime");
                break;
            }
            if (0 > shutdown(fd_data - epoll_worker_fd_map, SHUT_RDWR))
                LOGGER_PERROR("shutdown");
        }
    }
}

int timeout_handler_init(struct timeout_handler *timeout_handler) {

    /*
     * expiration handler
     */
    int tfd = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC | TFD_NONBLOCK);
    if (0 > tfd)
        return LOGGER_PERROR("timerfd_create"), -1;
    timeout_handler->timeout_handler_ctx = small_ctx_for_fd(tfd, expiration_handler);
    timeout_handler->timeout_handler_ctx->fd = tfd;
    timeout_handler->timeout_handler_ctx->data.ptr = timeout_handler;
    /*
     * timeout chain
     */
    list_init(&timeout_handler->timeout_chain);
    return 0;
}
