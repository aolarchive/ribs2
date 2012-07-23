/*
    This file is part of RIBS2.0 (Robust Infrastructure for Backend Systems).
    RIBS is an infrastructure for building great SaaS applications (but not
    limited to).

    Copyright (C) 2012 Adap.tv, Inc.

    RIBS is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, version 2.1 of the License.

    RIBS is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with RIBS.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "epoll_worker.h"
#include "logger.h"
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdarg.h>
#include <sys/uio.h>
#include <netdb.h>
#include <sys/eventfd.h>
#include <signal.h>
#include <sys/timerfd.h>

int ribs_connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    int flags=fcntl(sockfd, F_GETFL);
    if (0 > fcntl(sockfd, F_SETFL, flags | O_NONBLOCK))
        return LOGGER_PERROR("fcntl"), -1;

    int res = connect(sockfd, addr, addrlen);
    if (res < 0 && errno != EINPROGRESS) {
        return res;
    }

    struct epoll_event ev = { .events = EPOLLIN | EPOLLOUT | EPOLLET, .data.fd = sockfd };
    if (0 > epoll_ctl(ribs_epoll_fd, EPOLL_CTL_ADD, sockfd, &ev))
        return LOGGER_PERROR("epoll_ctl"), -1;
    return 0;
}

int ribs_fcntl(int fd, int cmd, ...) {
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
    int res;

    epoll_worker_set_fd_ctx(fd, current_ctx);
    while ((res = read(fd, buf, count)) < 0) {
        if (errno != EAGAIN)
            break;
        yield();
    }
    epoll_worker_set_fd_ctx(fd, &main_ctx);
    return res;
}

ssize_t ribs_write(int fd, const void *buf, size_t count) {
    int res;

    epoll_worker_set_fd_ctx(fd, current_ctx);
    while ((res = write(fd, buf, count)) < 0) {
        if (errno != EAGAIN)
            break;
        yield();
    }
    epoll_worker_set_fd_ctx(fd, &main_ctx);
    return res;
}

ssize_t ribs_recvfrom(int sockfd, void *buf, size_t len, int flags,
                      struct sockaddr *src_addr, socklen_t *addrlen) {
    int res;

    epoll_worker_set_fd_ctx(sockfd, current_ctx);
    while ((res = recvfrom(sockfd, buf, len, flags, src_addr, addrlen)) < 0) {
        if (errno != EAGAIN)
            break;
        yield();
    }
    epoll_worker_set_fd_ctx(sockfd, &main_ctx);
    return res;
}

ssize_t ribs_send(int sockfd, const void *buf, size_t len, int flags) {
    int res;

    epoll_worker_set_fd_ctx(sockfd, current_ctx);
    while ((res = send(sockfd, buf, len, flags)) < 0) {
        if (errno != EAGAIN)
            break;
        yield();
    }
    epoll_worker_set_fd_ctx(sockfd, &main_ctx);
    return res;
}

ssize_t ribs_recv(int sockfd, void *buf, size_t len, int flags) {
    int res;

    epoll_worker_set_fd_ctx(sockfd, current_ctx);
    while ((res = recv(sockfd, buf, len, flags)) < 0) {
        if (errno != EAGAIN)
            break;
        yield();
    }
    epoll_worker_set_fd_ctx(sockfd, &main_ctx);
    return res;
}

ssize_t ribs_readv(int fd, const struct iovec *iov, int iovcnt) {
    int res;

    epoll_worker_set_fd_ctx(fd, current_ctx);
    while ((res = readv(fd, iov, iovcnt)) < 0) {
        if (errno != EAGAIN)
            break;
        yield();
    }
    epoll_worker_set_fd_ctx(fd, &main_ctx);
    return res;
}

ssize_t ribs_writev(int fd, const struct iovec *iov, int iovcnt) {
    int res;

    epoll_worker_set_fd_ctx(fd, current_ctx);
    while ((res = writev(fd, iov, iovcnt)) < 0) {
        if (errno != EAGAIN)
            break;
        yield();
    }
    epoll_worker_set_fd_ctx(fd, &main_ctx);
    return res;
}


int ribs_pipe2(int pipefd[2], int flags) {

    if (0 > pipe2(pipefd, flags | O_NONBLOCK))
        return -1;

    struct epoll_event ev = { .events = EPOLLIN | EPOLLOUT | EPOLLET, .data.fd = pipefd[0] };
    if (0 > epoll_ctl(ribs_epoll_fd, EPOLL_CTL_ADD, pipefd[0], &ev))
        goto epoll_ctl_error;

    ev.data.fd = pipefd[1];
    if (0 > epoll_ctl(ribs_epoll_fd, EPOLL_CTL_ADD, pipefd[1], &ev))
        goto epoll_ctl_error;

    return 0;

epoll_ctl_error:
    {
        int my_error = errno;
        LOGGER_PERROR("epoll_ctl");
        close(pipefd[0]);
        close(pipefd[1]);
        errno = my_error;
    }
    return -1;
}

int ribs_pipe(int pipefd[2]) {
    return ribs_pipe2(pipefd, 0);
}

int ribs_nanosleep(const struct timespec *req, struct timespec *rem) {
    int tfd = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC | TFD_NONBLOCK);
    if (0 > tfd)
        return LOGGER_PERROR("ribs_nanosleep: timerfd_create"), -1;

    struct epoll_event ev = { .events = EPOLLIN, .data.fd = tfd };
    if (0 > epoll_ctl(ribs_epoll_fd, EPOLL_CTL_ADD, tfd, &ev))
        return LOGGER_PERROR("ribs_nanosleep: epoll_ctl"), close(tfd), -1;

    struct itimerspec when = {{0,0},{req->tv_sec, req->tv_nsec}};
    if (0 > timerfd_settime(tfd, 0, &when, NULL))
        return LOGGER_PERROR("ribs_nanosleep: timerfd_settime"), close(tfd), -1;

    epoll_worker_set_fd_ctx(tfd, current_ctx);
    yield();
    close(tfd);

    if (NULL != rem)
        rem->tv_sec = 0, rem->tv_nsec = 0;
    return 0;
}

unsigned int ribs_sleep(unsigned int seconds) {
    struct timespec req = {seconds, 0};
    ribs_nanosleep(&req, NULL);
    return 0;
}

int ribs_usleep(useconds_t usec) {
    struct timespec req = {usec/1000000L, (usec%1000000L)*1000L};
    return ribs_nanosleep(&req, NULL);
}

#ifdef UGLY_GETADDRINFO_WORKAROUND
int ribs_getaddrinfo(const char *node, const char *service,
                     const struct addrinfo *hints,
                     struct addrinfo **results) {

    struct gaicb cb = { .ar_name=node, .ar_service=service, .ar_request=hints, .ar_result=NULL };
    struct gaicb *cb_p[1] = { &cb };

    struct sigevent sevp;
    sevp.sigev_notify = SIGEV_SIGNAL;
    sevp.sigev_signo = SIGRTMIN; /* special support in epoll_worker.c */
    sevp.sigev_value.sival_ptr = current_ctx;
    sevp.sigev_notify_attributes = NULL;

    int res = getaddrinfo_a(GAI_NOWAIT, cb_p, 1, &sevp);
    if (!res) {
        yield();
        res = gai_error(cb_p[0]);
        *results = cb.ar_result;
    }
    return res;
}
#endif
