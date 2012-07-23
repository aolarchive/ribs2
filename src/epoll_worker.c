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
#include <sys/epoll.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/signalfd.h>
#include "logger.h"
#include <fcntl.h>

int ribs_epoll_fd = -1;
struct epoll_event last_epollev;
struct epoll_worker_fd_data *epoll_worker_fd_map;

static int queue_ctx_fd = -1;

LIST_CREATE(epoll_worker_timeout_chain);

static void sigrtmin_to_context(void) {
    struct signalfd_siginfo siginfo;
    while (1) {
       int res = read(last_epollev.data.fd, &siginfo, sizeof(struct signalfd_siginfo));
       if (sizeof(struct signalfd_siginfo) != res || NULL == (void *)siginfo.ssi_ptr) {
           LOGGER_PERROR("sigrtmin_to_ctx got NULL or < 128 bytes: %d", res);
           yield();
       } else
           ribs_swapcurcontext((void *)siginfo.ssi_ptr);
    }
}

static void pipe_to_context(void) {
    void *ctx;
    while (1) {
        if (sizeof(&ctx) != read(last_epollev.data.fd, &ctx, sizeof(&ctx))) {
            LOGGER_PERROR("read in pipe_to_context");
            yield();
        } else
            ribs_swapcurcontext(ctx);
    }
}

struct ribs_context* small_ctx_for_fd(int fd, void (*func)(void)) {
    void *ctx=ribs_context_create(SMALL_STACK_SIZE, func);
    if (NULL == ctx)
        return LOGGER_PERROR("ribs_context_create"), NULL;
    struct epoll_event ev = { .events = EPOLLIN, .data.fd = fd };
    if (0 > epoll_ctl(ribs_epoll_fd, EPOLL_CTL_ADD, fd, &ev))
        return LOGGER_PERROR("epoll_ctl"), NULL;
    epoll_worker_set_fd_ctx(fd, ctx);
    return ctx;
}

int epoll_worker_init(void) {

    struct rlimit rlim;
    if (0 > getrlimit(RLIMIT_NOFILE, &rlim))
        return LOGGER_PERROR("getrlimit(RLIMIT_NOFILE)"), -1;
    epoll_worker_fd_map = calloc(rlim.rlim_cur, sizeof(struct epoll_worker_fd_data));

    ribs_epoll_fd = epoll_create1(EPOLL_CLOEXEC);
    if (ribs_epoll_fd < 0)
        return LOGGER_PERROR("epoll_create1"), -1;

    /* block signals */
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGPIPE);
    sigaddset(&set, SIGRTMIN);
    if (-1 == sigprocmask(SIG_BLOCK, &set, NULL))
        return LOGGER_PERROR("sigprocmask"), -1;

    /* sigrtmin to context */
    sigdelset(&set, SIGPIPE);
    int sfd = signalfd(-1, &set, SFD_NONBLOCK);
    if (0 > sfd)
        return LOGGER_PERROR("signalfd"), -1;
    if (NULL == small_ctx_for_fd(sfd, sigrtmin_to_context))
        return -1;

    /* pipe to conetxt */
    int pipefd[2];
    if (0 > pipe2(pipefd, O_NONBLOCK))
        return LOGGER_PERROR("pipe"), -1;
    if (NULL == small_ctx_for_fd(pipefd[0], pipe_to_context))
        return -1;
    queue_ctx_fd = pipefd[1];

    return 0;
}

void epoll_worker_loop(void) {
    for (;;yield());
}

inline void yield(void) {
    while(0 >= epoll_wait(ribs_epoll_fd, &last_epollev, 1, -1));
    ribs_swapcurcontext(epoll_worker_get_last_context());
}

inline void courtesy_yield(void) {
    if (0 == epoll_wait(ribs_epoll_fd, &last_epollev, 1, 0))
        return;
    if (0 > write(queue_ctx_fd, &current_ctx, sizeof(void *)))
        LOGGER_PERROR("unable to queue context: write");
    ribs_swapcurcontext(epoll_worker_get_last_context());
}
