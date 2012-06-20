#include "ribify.h"
#include "epoll_worker.h"
#include "logger.h"
#include <fcntl.h>
#include <errno.h>

int ribs_connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen, int timeout) {
    (void)timeout;
    int flags = fcntl(sockfd, F_GETFL);
    if (0 > fcntl(sockfd, F_SETFL, flags | O_NONBLOCK))
        return LOGGER_PERROR("mysql_client: fcntl"), -1;

    int res = connect(sockfd, addr, addrlen);
    if (res < 0 && errno != EAGAIN && errno != EINPROGRESS) {
        LOGGER_PERROR("mysql_client: connect");
        return res;
    }
    struct epoll_event ev = { .events = EPOLLIN | EPOLLOUT | EPOLLET, .data.fd = sockfd };
    if (0 > epoll_ctl(ribs_epoll_fd, EPOLL_CTL_ADD, sockfd, &ev))
        return LOGGER_PERROR("mysql_client: epoll_ctl"), -1;

    return 0;
}

ssize_t ribs_read(int fd, void *buf, size_t count) {
    int res;
    epoll_worker_fd_map[fd].ctx = current_ctx;
    while ((res = read(fd, buf, count)) < 0) {
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

ssize_t ribs_write(int fd, const void *buf, size_t count) {
    int res;
    epoll_worker_fd_map[fd].ctx = current_ctx;
    while ((res = write(fd, buf, count)) < 0) {
        if (errno != EAGAIN) {
            LOGGER_PERROR("write\n");
            break;
        }
        LOGGER_INFO("waiting for write...");
        yield();
        LOGGER_INFO("waiting for write...done");
    }
    epoll_worker_fd_map[fd].ctx = &main_ctx;
    return res;
}
