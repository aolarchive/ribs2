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
#ifndef _TIMER_WORKER__H_
#define _TIMER_WORKER__H_

#include "ribs_defs.h"

void *timer_worker_init(size_t stack_size, size_t reserved_size, time_t usec_initial, void (*user_func)(void));
int timer_worker_schedule_next(time_t usec);
void *timer_worker_get_user_data(void);

#endif // _TIMER_WORKER__H_
