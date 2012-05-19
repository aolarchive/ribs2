#ifndef _EPOLL_WORKER__H_
#define _EPOLL_WORKER__H_

#include "context.h"

extern int ribs_epoll_fd;
extern struct ribs_context ribs_epoll_ctx;

int epoll_worker_init(void);
void epoll_worker_loop(void);
void yield(void);


#endif // _EPOLL_WORKER__H_
