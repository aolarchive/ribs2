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
#ifndef _TIMEOUT_HANDLER__H_
#define _TIMEOUT_HANDLER__H_

#include "ribs_defs.h"
#include "context.h"
#include "list.h"
#include "epoll_worker.h"
#include <sys/time.h>
#include <sys/timerfd.h>
#include <stdio.h>
#include "logger.h"

struct timeout_handler {
    int fd;
    struct ribs_context *timeout_handler_ctx;
    struct list timeout_chain;
    time_t timeout;
};

int timeout_handler_init(struct timeout_handler *timeout_handler);
_RIBS_INLINE_ void timeout_handler_add_fd_data(struct timeout_handler *timeout_handler, struct epoll_worker_fd_data *fd_data);

#define TIMEOUT_HANDLER_REMOVE_FD_DATA(fd_data) \
    list_remove(&(fd_data)->timeout_chain)

#include "../src/_timeout_handler.c"

#endif // _TIMEOUT_HANDLER__H_
