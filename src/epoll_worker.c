#include "epoll_worker.h"
#include <sys/epoll.h>
#include <stdio.h>

int ribs_epoll_fd = -1;
struct ribs_context *current_ctx = NULL;
struct ribs_context main_ctx;


int epoll_worker_init(void) {
   ribs_epoll_fd = epoll_create1(EPOLL_CLOEXEC);
   if (ribs_epoll_fd < 0)
      return perror("epoll_create1"), -1;
   return 0;
}

void epoll_worker_loop(void) {
   struct epoll_event epollev;
   for (;;) {
      if (0 >= epoll_wait(ribs_epoll_fd, &epollev, 1, -1))
         continue;
      current_ctx = (struct ribs_context *)epollev.data.ptr;
      ribs_swapcontext(current_ctx, &main_ctx);
   }
}

void yield(void) {
   struct epoll_event epollev;
   while(0 >= epoll_wait(ribs_epoll_fd, &epollev, 1, -1));
   current_ctx = (struct ribs_context *)epollev.data.ptr;
   ribs_swapcontext(current_ctx, &main_ctx);
}
