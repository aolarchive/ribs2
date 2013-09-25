/*
    This file is part of RIBS2.0 (Robust Infrastructure for Backend Systems).
    RIBS is an infrastructure for building great SaaS applications (but not
    limited to).

    Copyright (C) 2012,2013 Adap.tv, Inc.

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
#include "ribify.h"
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
#include <sys/sendfile.h>

int _ribified_connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    int flags=fcntl(sockfd, F_GETFL);
    if (0 > fcntl(sockfd, F_SETFL, flags | O_NONBLOCK))
        return LOGGER_PERROR("fcntl"), -1;

    int res = connect(sockfd, addr, addrlen);
    if (res < 0 && errno != EINPROGRESS) {
        return res;
    }
    return ribs_epoll_add(sockfd, EPOLLIN | EPOLLOUT | EPOLLET | EPOLLRDHUP, event_loop_ctx);
}

int _ribified_fcntl(int fd, int cmd, ...) {
    va_list ap;
    long arg;

    va_start (ap, cmd);
    arg = va_arg (ap, long);
    va_end (ap);

    if (F_SETFL == cmd)
        arg |= O_NONBLOCK;

    return fcntl(fd, cmd, arg);
}

inline static int yield_if_eagain(int sockfd) {
    if (errno != EAGAIN)
        return 0;
    epoll_worker_set_fd_ctx(sockfd, current_ctx);
    yield();
    epoll_worker_set_fd_ctx(sockfd, event_loop_ctx);
    return 1;
}

ssize_t _ribified_read(int fd, void *buf, size_t count) {
    int res;
    while ((res = read(fd, buf, count)) < 0 &&
           yield_if_eagain(fd));
    return res;
}

ssize_t _ribified_write(int fd, const void *buf, size_t count) {
    int res;
    while ((res = write(fd, buf, count)) < 0 &&
           yield_if_eagain(fd));
    return res;
}

ssize_t _ribified_recv(int sockfd, void *buf, size_t len, int flags) {
    int res;
    while ((res = recv(sockfd, buf, len, flags)) < 0 &&
           yield_if_eagain(sockfd));
    return res;
}

ssize_t _ribified_recvfrom(int sockfd, void *buf, size_t len, int flags,
                      struct sockaddr *src_addr, socklen_t *addrlen) {
    int res;
    while ((res = recvfrom(sockfd, buf, len, flags, src_addr, addrlen)) < 0 &&
           yield_if_eagain(sockfd));
    return res;
}

ssize_t _ribified_recvmsg(int sockfd, struct msghdr *msg, int flags) {
    int res;
    while ((res = recvmsg(sockfd, msg, flags)) < 0 &&
           yield_if_eagain(sockfd));
    return res;
}

ssize_t _ribified_send(int sockfd, const void *buf, size_t len, int flags) {
    int res;
    while ((res = send(sockfd, buf, len, flags)) < 0 &&
           yield_if_eagain(sockfd));
    return res;
}

ssize_t _ribified_sendto(int sockfd, const void *buf, size_t len, int flags,
                         const struct sockaddr *dest_addr, socklen_t addrlen) {
    int res;
    while ((res = sendto(sockfd, buf, len, flags, dest_addr, addrlen)) < 0 &&
           yield_if_eagain(sockfd));
    return res;
}

ssize_t _ribified_sendmsg(int sockfd, const struct msghdr *msg, int flags) {
    int res;
    while ((res = sendmsg(sockfd, msg, flags)) < 0 &&
           yield_if_eagain(sockfd));
    return res;
}

ssize_t _ribified_readv(int fd, const struct iovec *iov, int iovcnt) {
    int res;
    while ((res = readv(fd, iov, iovcnt)) < 0 &&
           yield_if_eagain(fd));
    return res;
}

ssize_t _ribified_writev(int fd, const struct iovec *iov, int iovcnt) {
    int res;
    while ((res = writev(fd, iov, iovcnt)) < 0 &&
           yield_if_eagain(fd));
    return res;
}

ssize_t _ribified_sendfile(int out_fd, int in_fd, off_t *offset, size_t count) {
    ssize_t res;
    while ((res = sendfile(out_fd, in_fd, offset, count)) < 0 &&
           yield_if_eagain(out_fd));
    return res;
}

int _ribified_pipe2(int pipefd[2], int flags) {

    if (0 > pipe2(pipefd, flags | O_NONBLOCK))
        return -1;

    if (0 == ribs_epoll_add(pipefd[0], EPOLLIN | EPOLLET | EPOLLRDHUP, event_loop_ctx) &&
        0 == ribs_epoll_add(pipefd[1], EPOLLOUT | EPOLLET | EPOLLRDHUP, event_loop_ctx))
        return 0;

    int my_error = errno;
    close(pipefd[0]);
    close(pipefd[1]);
    errno = my_error;
    return -1;
}

int _ribified_pipe(int pipefd[2]) {
    return _ribified_pipe2(pipefd, 0);
}

int _ribified_nanosleep(const struct timespec *req, struct timespec *rem) {
    int tfd = ribs_sleep_init();
    if (0 > tfd) return -1;
    int res = ribs_nanosleep(tfd, req, rem);
    close(tfd);
    return res;
}

unsigned int _ribified_sleep(unsigned int seconds) {
    struct timespec req = {seconds, 0};
    _ribified_nanosleep(&req, NULL);
    return 0;
}

int _ribified_usleep(useconds_t usec) {
    struct timespec req = {usec/1000000L, (usec%1000000L)*1000L};
    return _ribified_nanosleep(&req, NULL);
}

void *_ribified_malloc(size_t size) {
    if (0 == size) return NULL;
    ++current_ctx->ribify_memalloc_refcount;
    void *mem = memalloc_alloc(&current_ctx->memalloc, size + sizeof(uint32_t));
    *(uint32_t *)mem = size;
    return mem + sizeof(uint32_t);
}

void _ribified_free(void *ptr) {
    if (NULL == ptr) return;
    --current_ctx->ribify_memalloc_refcount;
}

void *_ribified_calloc(size_t nmemb, size_t size) {
    size_t s = nmemb * size;
    void *mem = _ribified_malloc(s);
    memset(mem, 0, s);
    return mem;
}

void *_ribified_realloc(void *ptr, size_t size) {
    if (NULL == ptr) return _ribified_malloc(size);
    if (memalloc_is_mine(&current_ctx->memalloc, ptr))
        --current_ctx->ribify_memalloc_refcount; // consider mine as freed
    size_t old_size = *(uint32_t *)(ptr - sizeof(uint32_t));
    void *mem = _ribified_malloc(size);
    memcpy(mem, ptr, size > old_size ? old_size : size);
    return mem;
}

char *_ribified_strdup(const char *s) {
    size_t l = strlen(s) + 1;
    char *mem = _ribified_malloc(l);
    memcpy(mem, s, l);
    return mem;
}


void *ribify_malloc(size_t size) __attribute__ ((weak, alias("_ribified_malloc")));
void ribify_free(void *ptr) __attribute__ ((weak, alias("_ribified_free")));
void *ribify_calloc(size_t nmemb, size_t size) __attribute__ ((weak, alias("_ribified_calloc")));
void *ribify_realloc(void *ptr, size_t size) __attribute__ ((weak, alias("_ribified_realloc")));
char *ribify_strdup(const char *s) __attribute__ ((weak, alias("_ribified_strdup")));



#ifdef UGLY_GETADDRINFO_WORKAROUND
int _ribified_getaddrinfo(const char *node, const char *service,
                     const struct addrinfo *hints,
                     struct addrinfo **results) {

    struct gaicb cb = { .ar_name = node, .ar_service = service, .ar_request = hints, .ar_result = NULL };
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
