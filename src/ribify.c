#include "epoll_worker.h"
#include "logger.h"
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdarg.h>

int ribs_connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    LOGGER_INFO("connect");
    int flags=fcntl(sockfd, F_GETFL);
    if (0 > fcntl(sockfd, F_SETFL, flags | O_NONBLOCK))
        return LOGGER_PERROR("mysql_client: fcntl"), -1;

    int res = connect(sockfd, addr, addrlen);
    if (res < 0 && errno != EINPROGRESS) {
        return res;
    }

    struct epoll_event ev = { .events = EPOLLIN | EPOLLOUT | EPOLLET, .data.fd = sockfd };
    if (0 > epoll_ctl(ribs_epoll_fd, EPOLL_CTL_ADD, sockfd, &ev))
        return LOGGER_PERROR("mysql_client: epoll_ctl"), -1;
    return 0;
}

int ribs_fcntl(int fd, int cmd, ...) {
    LOGGER_INFO("fcntl");
    va_list ap;
    long arg;

    va_start (ap, cmd);
    arg = va_arg (ap, long);
    va_end (ap);

    if (F_SETFL == cmd)
        arg |= O_NONBLOCK;

    return fcntl(fd, cmd, arg);
}

ssize_t ribs_read(int fd, void *buf, size_t count) {
    LOGGER_INFO("read");
    int res;

    epoll_worker_fd_map[fd].ctx = current_ctx;
    while ((res = read(fd, buf, count)) < 0) {
        if (errno != EAGAIN)
            break;
        LOGGER_INFO("yield");
        yield();
    }
    epoll_worker_fd_map[fd].ctx = &main_ctx;
    return res;
}

ssize_t ribs_write(int fd, const void *buf, size_t count) {
    LOGGER_INFO("write");
    int res;

    epoll_worker_fd_map[fd].ctx = current_ctx;
    while ((res = write(fd, buf, count)) < 0) {
        if (errno != EAGAIN)
            break;
        LOGGER_INFO("yield");
        yield();
    }
    epoll_worker_fd_map[fd].ctx = &main_ctx;
    return res;
}

