#include "ribify.h"
#include "epoll_worker.h"
#include "logger.h"
#include <fcntl.h>
#include <errno.h>

int __real_connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
ssize_t __real_read(int fd, void *buf, size_t count);
ssize_t __real_write(int fd, const void *buf, size_t count);

int ribs_connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen, int timeout) {
    (void)timeout;
    int flags = fcntl(sockfd, F_GETFL);
    if (0 > fcntl(sockfd, F_SETFL, flags | O_NONBLOCK))
        return LOGGER_PERROR("mysql_client: fcntl"), -1;

    int res = __real_connect(sockfd, addr, addrlen);
    if (res < 0 && errno != EAGAIN && errno != EINPROGRESS) {
        LOGGER_PERROR("mysql_client: connect");
        return res;
    }

    struct epoll_event ev = { .events = EPOLLIN | EPOLLOUT | EPOLLET, .data.fd = sockfd };
    if (0 > epoll_ctl(ribs_epoll_fd, EPOLL_CTL_ADD, sockfd, &ev))
        return LOGGER_PERROR("mysql_client: epoll_ctl"), -1;
    return 0;
}

int __wrap_connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    return ribs_connect(sockfd, addr, addrlen, 0);
}

ssize_t ribs_read(int fd, void *buf, size_t count) {
    int res;
    epoll_worker_fd_map[fd].ctx = current_ctx;
    while ((res = __real_read(fd, buf, count)) < 0) {
        if (errno != EAGAIN) {
            LOGGER_PERROR("read");
            break;
        }
        LOGGER_INFO("waiting for read...");
        yield();
        LOGGER_INFO("waiting for read...done");
    }
    epoll_worker_fd_map[fd].ctx = &main_ctx;
    return res;
}

ssize_t __wrap_read(int fd, void *buf, size_t count) {
    return ribs_read(fd, buf, count);
}

ssize_t ribs_write(int fd, const void *buf, size_t count) {
    int res;
    epoll_worker_fd_map[fd].ctx = current_ctx;
    while ((res = __real_write(fd, buf, count)) < 0) {
        if (errno != EAGAIN) {
            LOGGER_PERROR("write");
            break;
        }
        LOGGER_INFO("waiting for write...");
        yield();
        LOGGER_INFO("waiting for write...done");
    }
    epoll_worker_fd_map[fd].ctx = &main_ctx;
    return res;
}

ssize_t __wrap_write(int fd, const void *buf, size_t count) {
    return ribs_write(fd, buf, count);
}


