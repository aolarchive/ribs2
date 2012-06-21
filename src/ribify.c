#include "epoll_worker.h"
#include "logger.h"
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdarg.h>

int __wrap_connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    int flags=__real_fcntl(sockfd, F_GETFL);
    if (0 > __real_fcntl(sockfd, F_SETFL, flags | O_NONBLOCK))
        return LOGGER_PERROR("mysql_client: fcntl"), -1;

    int res = __real_connect(sockfd, addr, addrlen);
    if (res < 0 && errno != EINPROGRESS) {
        return res;
    }

    struct epoll_event ev = { .events = EPOLLIN | EPOLLOUT | EPOLLET, .data.fd = sockfd };
    if (0 > epoll_ctl(ribs_epoll_fd, EPOLL_CTL_ADD, sockfd, &ev))
        return LOGGER_PERROR("mysql_client: epoll_ctl"), -1;
    return 0;
}

int __wrap_fcntl(int fd, int cmd, ...) {
    va_list ap;
    long arg;

    va_start (ap, cmd);
    arg = va_arg (ap, long);
    va_end (ap);

    if (F_SETFL == cmd)
        arg |= O_NONBLOCK;

    return __real_fcntl(fd, cmd, arg);
}

ssize_t __wrap_read(int fd, void *buf, size_t count) {
    int res;

    epoll_worker_fd_map[fd].ctx = current_ctx;
    while ((res = __real_read(fd, buf, count)) < 0) {
        if (errno != EAGAIN)
            break;
        yield();
    }
    epoll_worker_fd_map[fd].ctx = &main_ctx;
    return res;
}

ssize_t __wrap_write(int fd, const void *buf, size_t count) {
    int res;

    epoll_worker_fd_map[fd].ctx = current_ctx;
    while ((res = __real_write(fd, buf, count)) < 0) {
        if (errno != EAGAIN)
            break;
        yield();
    }
    epoll_worker_fd_map[fd].ctx = &main_ctx;
    return res;
}

