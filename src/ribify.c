#include "ribify.h"
#include "epoll_worker.h"
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>

int ribs_connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen, int timeout) {
    (void)timeout;
    int flags = fcntl(sockfd, F_GETFL);
    if (0 > fcntl(sockfd, F_SETFL, flags | O_NONBLOCK))
        return perror("mysql_client: fcntl"), -1;

    int res = connect(sockfd, addr, addrlen);
    if (res < 0 && errno != EAGAIN && errno != EINPROGRESS) {
        perror("mysql_client: connect");
        return res;
    }
    struct epoll_event ev = { .events = EPOLLIN | EPOLLOUT | EPOLLET, .data.fd = sockfd };
    if (0 > epoll_ctl(ribs_epoll_fd, EPOLL_CTL_ADD, sockfd, &ev))
        return perror("mysql_client: epoll_ctl"), -1;

    return 0;
}

ssize_t ribs_read(int fd, void *buf, size_t count) {
    int res;
    epoll_worker_fd_map[fd].ctx = current_ctx;
    while ((res = read(fd, buf, count)) < 0) {
        if (errno != EAGAIN) {
            perror("read");
            break;
        }
        dprintf(2, "waiting for read...\n");
        yield();
        dprintf(2, "waiting for read...done\n");
    }
    epoll_worker_fd_map[fd].ctx = &main_ctx;
    return res;
}

ssize_t ribs_write(int fd, const void *buf, size_t count) {
    int res;
    epoll_worker_fd_map[fd].ctx = current_ctx;
    while ((res = write(fd, buf, count)) < 0) {
        if (errno != EAGAIN) {
            perror("write\n");
            break;
        }
        dprintf(2, "waiting for write...\n");
        yield();
        dprintf(2, "waiting for write...done\n");
    }
    epoll_worker_fd_map[fd].ctx = &main_ctx;
    return res;
}
