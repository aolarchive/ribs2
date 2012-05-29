#include "epoll_worker.h"
#include <sys/epoll.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <stdlib.h>
#include <signal.h>

int ribs_epoll_fd = -1;
struct ribs_context *current_ctx = NULL;
struct epoll_event current_epollev;
struct ribs_context **fd_to_ctx;

LIST_CREATE(timeout_chain_head);

int epoll_worker_init(void) {
    struct rlimit rlim;
    if (0 > getrlimit(RLIMIT_NOFILE, &rlim))
        return perror("getrlimit(RLIMIT_NOFILE)"), -1;
    fd_to_ctx = calloc(rlim.rlim_cur, sizeof(struct ribs_context *));
    ribs_epoll_fd = epoll_create1(EPOLL_CLOEXEC);
    if (ribs_epoll_fd < 0)
        return perror("epoll_create1"), -1;
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGPIPE);
    if (-1 == sigprocmask(SIG_BLOCK, &set, NULL))
        return perror("sigprocmask"), -1;
    return 0;
}

void epoll_worker_loop(void) {
   for (;;) {
      if (0 >= epoll_wait(ribs_epoll_fd, &current_epollev, 1, -1))
         continue;
      current_ctx = fd_to_ctx[current_epollev.data.fd];
      ribs_swapcontext(current_ctx, &main_ctx);
   }
}

void yield(void) {
   while(0 >= epoll_wait(ribs_epoll_fd, &current_epollev, 1, -1));
   struct ribs_context *old_ctx = current_ctx;
   current_ctx = fd_to_ctx[current_epollev.data.fd];
   ribs_swapcontext(current_ctx, old_ctx);
}
