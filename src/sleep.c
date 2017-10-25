/*
    This file is part of RIBS2.0 (Robust Infrastructure for Backend Systems).
    RIBS is an infrastructure for building great SaaS applications (but not
    limited to).

    Copyright (C) 2013 Adap.tv, Inc.

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
#include "ribs.h"
#include <sys/timerfd.h>

int ribs_sleep_init(void) {
    int tfd = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC | TFD_NONBLOCK);
    if (0 > tfd)
        return LOGGER_PERROR("ribs_sleep_init: timerfd_create"), -1;

    if (0 > ribs_epoll_add(tfd, EPOLLIN, current_ctx))
        return close(tfd), -1;
    return tfd;
}

int ribs_nanosleep(int tfd, const struct timespec *req, struct timespec *rem) {
    struct itimerspec when = {{0,0},{req->tv_sec, req->tv_nsec}};
    if (0 > timerfd_settime(tfd, 0, &when, NULL))
        return LOGGER_PERROR("_ribified_nanosleep: timerfd_settime"), close(tfd), -1;
    yield();
    if (NULL != rem)
        rem->tv_sec = 0, rem->tv_nsec = 0;
    return 0;
}

unsigned int ribs_sleep(int tfd, unsigned int seconds) {
    struct timespec req = {seconds, 0};
    ribs_nanosleep(tfd, &req, NULL);
    return 0;
}

int ribs_usleep(int tfd, useconds_t usec) {
    struct timespec req = {usec/1000000L, (usec%1000000L)*1000L};
    return ribs_nanosleep(tfd, &req, NULL);
}
