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
#include "timeout_handler.h"
#include <sys/socket.h>

static void expiration_handler(void) {
    uint64_t num_exp;
    struct timeout_handler **ref = (struct timeout_handler **)current_ctx->reserved;
    struct timeout_handler *timeout_handler = *ref;
    struct timeval when = {timeout_handler->timeout/1000,(timeout_handler->timeout%1000)*1000};
    int fd = timeout_handler->fd;
    for (;;yield()) {
        if (sizeof(num_exp) != read(fd, &num_exp, sizeof(num_exp)) || list_empty(&timeout_handler->timeout_chain))
            continue;
        struct timeval now, ts;
        gettimeofday(&now, NULL);
        timersub(&now, &when, &ts);
        struct list *fd_data_list;
        int arm_timer = 1;
        LIST_FOR_EACH(&timeout_handler->timeout_chain, fd_data_list) {
            struct epoll_worker_fd_data *fd_data = LIST_ENTRY(fd_data_list, struct epoll_worker_fd_data, timeout_chain);
            if (timercmp(&fd_data->timestamp, &ts, >)) {
                timersub(&fd_data->timestamp, &ts, &now);
                struct itimerspec whence = {{0,0},{now.tv_sec,now.tv_usec*1000}};
                if (0 > timerfd_settime(timeout_handler->fd, 0, &whence, NULL))
                    LOGGER_PERROR("timerfd_settime");
                arm_timer = 0;
                break;
            }
            if (0 > shutdown(fd_data - epoll_worker_fd_map, SHUT_RDWR))
                LOGGER_PERROR("shutdown");
        }
        if (arm_timer) {
            struct itimerspec whence = {{0,0},{when.tv_sec,when.tv_usec*1000}};
            if (0 > timerfd_settime(timeout_handler->fd, 0, &whence, NULL))
                LOGGER_PERROR("timerfd_settime");
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
    timeout_handler->timeout_handler_ctx = small_ctx_for_fd(tfd, sizeof(struct timeout_handler *), expiration_handler);
    timeout_handler->fd = tfd;
    struct timeout_handler **ref = (struct timeout_handler **)timeout_handler->timeout_handler_ctx->reserved;
    *ref = timeout_handler;
    /*
     * timeout chain
     */
    list_init(&timeout_handler->timeout_chain);
    return 0;
}
