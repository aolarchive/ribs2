/*
    This file is part of RIBS2.0 (Robust Infrastructure for Backend Systems).
    RIBS is an infrastructure for building great SaaS applications (but not
    limited to).

    Copyright (C)  Adap.tv, Inc.

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
#include "timer_worker.h"
#include "ribs.h"
#include <sys/timerfd.h>

struct timer_worker_context {
    int tfd;
    void (*user_func)(void);
    char user_data[];
};

static int _timer_worker_schedule_next(int tfd, time_t usec) {
    struct itimerspec timerspec = {{0,0},{usec/1000000,(usec%1000000)*1000}};
    if (0 > timerfd_settime(tfd, 0, &timerspec, NULL))
        return LOGGER_PERROR("timerfd: %d", tfd), -1;
    return 0;
}

static inline struct timer_worker_context *_timer_worker_get_context(void) {
    struct timer_worker_context *twc = (struct timer_worker_context *)current_ctx->reserved;
    return twc;
}

int timer_worker_schedule_next(time_t usec) {
    struct timer_worker_context *twc = _timer_worker_get_context();
    return _timer_worker_schedule_next(twc->tfd, usec);
}

static void _timer_worker_wrapper(void) {
    struct timer_worker_context *twc = _timer_worker_get_context();
    twc->user_func();
}

static void _timer_worker_trigger(void) {
    struct ribs_context **worker_ctx_ref = (struct ribs_context **)current_ctx->reserved;
    struct ribs_context *worker_ctx = *worker_ctx_ref;
    for (;;yield()) {
        uint64_t num_exp;
        if (sizeof(num_exp) != read(last_epollev.data.fd, &num_exp, sizeof(num_exp))) {
            if (EAGAIN == errno)
                continue;
            abort();
        }
        ribs_makecontext(worker_ctx, event_loop_ctx, _timer_worker_wrapper);
        ribs_swapcurcontext(worker_ctx);
    }
}

void *timer_worker_init(size_t stack_size, size_t reserved_size, time_t usec_initial, void (*user_func)(void)) {
    int tfd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    if (0 > tfd)
        return LOGGER_PERROR("timerfd"), NULL;
    if (0 > _timer_worker_schedule_next(tfd, usec_initial))
        return close(tfd), NULL;
    struct ribs_context *ctx = small_ctx_for_fd(tfd, sizeof(struct ribs_context *), _timer_worker_trigger);
    struct ribs_context *worker_ctx = ribs_context_create(stack_size, sizeof(struct timer_worker_context) + reserved_size, NULL);
    struct ribs_context **ctx_ref = (struct ribs_context **)ctx->reserved;
    *ctx_ref = worker_ctx;
    struct timer_worker_context *twc = (struct timer_worker_context *)worker_ctx->reserved;
    twc->tfd = tfd;
    twc->user_func = user_func;
    return twc->user_data;
}

void *timer_worker_get_user_data(void) {
    struct timer_worker_context *twc = _timer_worker_get_context();
    return twc->user_data;
}
