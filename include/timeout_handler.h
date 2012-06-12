#ifndef _TIMEOUT_HANDLER__H_
#define _TIMEOUT_HANDLER__H_

#include "ribs_defs.h"
#include "context.h"
#include "list.h"
#include "epoll_worker.h"
#include <sys/time.h>
#include <sys/timerfd.h>
#include <stdio.h>

struct timeout_handler {
    struct ribs_context timeout_handler_ctx;
    char *timeout_handler_stack;
    struct list timeout_chain;
    time_t timeout;
};

int timeout_handler_init(struct timeout_handler *timeout_handler);
_RIBS_INLINE_ void timeout_handler_add_fd_data(struct timeout_handler *timeout_handler, struct epoll_worker_fd_data *fd_data);

#define TIMEOUT_HANDLER_REMOVE_FD_DATA(fd_data) \
    list_remove(&fd_data->timeout_chain);

#include "../src/_timeout_handler.c"

#endif // _TIMEOUT_HANDLER__H_
