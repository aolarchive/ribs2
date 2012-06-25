#include "epoll_worker.h"
#include "logger.h"
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdarg.h>
#include <sys/uio.h>
#define _GNU_SOURCE
#include <netdb.h>
#include <sys/eventfd.h>

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

ssize_t ribs_recvfrom(int sockfd, void *buf, size_t len, int flags,
                      struct sockaddr *src_addr, socklen_t *addrlen) {
    LOGGER_INFO("recvfrom");
    int res;

    epoll_worker_fd_map[sockfd].ctx = current_ctx;
    while ((res = recvfrom(sockfd, buf, len, flags, src_addr, addrlen)) < 0) {
        if (errno != EAGAIN)
            break;
        LOGGER_INFO("yield");
        yield();
    }
    epoll_worker_fd_map[sockfd].ctx = &main_ctx;
    return res;
}

ssize_t ribs_send(int sockfd, const void *buf, size_t len, int flags) {
    LOGGER_INFO("send");
    int res;

    epoll_worker_fd_map[sockfd].ctx = current_ctx;
    while ((res = send(sockfd, buf, len, flags)) < 0) {
        if (errno != EAGAIN)
            break;
        LOGGER_INFO("yield");
        yield();
    }
    epoll_worker_fd_map[sockfd].ctx = &main_ctx;
    return res;
}

ssize_t ribs_recv(int sockfd, void *buf, size_t len, int flags) {
    LOGGER_INFO("recv");
    int res;

    epoll_worker_fd_map[sockfd].ctx = current_ctx;
    while ((res = recv(sockfd, buf, len, flags)) < 0) {
        if (errno != EAGAIN)
            break;
        LOGGER_INFO("yield");
        yield();
    }
    epoll_worker_fd_map[sockfd].ctx = &main_ctx;
    return res;
}

ssize_t ribs_readv(int fd, const struct iovec *iov, int iovcnt) {
    LOGGER_INFO("readv");
    int res;

    epoll_worker_fd_map[fd].ctx = current_ctx;
    while ((res = readv(fd, iov, iovcnt)) < 0) {
        if (errno != EAGAIN)
            break;
        LOGGER_INFO("yield");
        yield();
    }
    epoll_worker_fd_map[fd].ctx = &main_ctx;
    return res;
}

ssize_t ribs_writev(int fd, const struct iovec *iov, int iovcnt) {
    LOGGER_INFO("writev");
    int res;

    epoll_worker_fd_map[fd].ctx = current_ctx;
    while ((res = writev(fd, iov, iovcnt)) < 0) {
        if (errno != EAGAIN)
            break;
        LOGGER_INFO("yield");
        yield();
    }
    epoll_worker_fd_map[fd].ctx = &main_ctx;
    return res;
}

void sig_to_eventfd(union sigval fd) {
    long i = 1;
    if (sizeof(i) != write(fd.sival_int, &i, sizeof(i)))
        LOGGER_PERROR("write");
}

int ribs_getaddrinfo(const char *node, const char *service,
                     const struct addrinfo *hints,
                     struct addrinfo **results) {
    LOGGER_INFO("getaddrinfo");

    struct gaicb cb = { .ar_name=node, .ar_service=service, .ar_request=hints, .ar_result=NULL };
    struct gaicb *cb_p[1] = { &cb };

    int efd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);

    struct epoll_event ev = { .events = EPOLLIN | EPOLLET, .data.fd = efd };
    if (0 > epoll_ctl(ribs_epoll_fd, EPOLL_CTL_ADD, efd, &ev))
        return LOGGER_PERROR("mysql_client: epoll_ctl"), -1;

    struct sigevent sevp;
    sevp.sigev_notify = SIGEV_THREAD; /* Notification method */
    sevp.sigev_signo = 0; /* Notification signal */
    sevp.sigev_value.sival_int = efd; /* Data passed with notification */
    sevp.sigev_notify_function = sig_to_eventfd; /* Function used for thread notification (SIGEV_THREAD) */
    sevp.sigev_notify_attributes = NULL; /* Attributes for notification thread (SIGEV_THREAD) */

    int res = getaddrinfo_a(GAI_NOWAIT, &cb_p[0], 1, &sevp);
    LOGGER_INFO("getaddrinfo_a: %d", res);
    if (!res) {
        epoll_worker_fd_map[efd].ctx = current_ctx;
 
        LOGGER_INFO("yield");
        yield();
        res = gai_error(cb_p[0]);
        *results = cb.ar_result;
        LOGGER_INFO("gai_error: %d", res);
 
        char host[NI_MAXHOST];
        struct addrinfo *resi;
 
        LOGGER_INFO(" %s: ", cb_p[0]->ar_name);
 
        if (!res) {
             resi = cb_p[0]->ar_result;

             res = getnameinfo(resi->ai_addr, resi->ai_addrlen,
                               host, sizeof(host),
                               NULL, 0, NI_NUMERICHOST);
             if (res)
                 LOGGER_PERROR("getnameinfo() failed: %s\n", gai_strerror(res));
             else
                 LOGGER_INFO(host);
        } else {
             LOGGER_ERROR(gai_strerror(res));
        }
    }

    close(efd);
    return res;
}


