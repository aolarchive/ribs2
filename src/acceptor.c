#define _GNU_SOURCE
#include "acceptor.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/resource.h>
#include "epoll_worker.h"

extern struct ribs_context main_ctx;
extern struct ribs_context *current_ctx;

#define ACCEPTOR_STACK_SIZE 8192

void accept_connections(void) {
   for (;; yield()) {
      struct sockaddr_in new_addr;
      socklen_t new_addr_size = sizeof(struct sockaddr_in);
      int fd = accept4(current_ctx->fd, (struct sockaddr *)&new_addr, &new_addr_size, SOCK_CLOEXEC | SOCK_NONBLOCK);
      if (fd < 0)
         continue;
      struct acceptor *acceptor = (struct acceptor *)current_ctx->data.ptr;
      struct ribs_context *ctx = ctx_pool_get(&acceptor->ctx_pool);
      ribs_makecontext(ctx, &main_ctx, ctx, acceptor->ctx_start_func, ctx_pool_cleanup_func);
      ctx->fd = fd;
      ctx->data.ptr = &acceptor->ctx_pool;

      struct epoll_event ev;
      ev.events = EPOLLIN | EPOLLET;
      ev.data.ptr = ctx;
      if (0 > epoll_ctl(ribs_epoll_fd, EPOLL_CTL_ADD, fd, &ev))
         perror("epoll_ctl");
   }
}

int acceptor_init(struct acceptor *acceptor, uint16_t port, void (*func)(void)) {
   struct rlimit rlim;
   if (0 > getrlimit(RLIMIT_STACK, &rlim))
      return perror("getrlimit(RLIMIT_STACK)"), -1;
   ctx_pool_init(&acceptor->ctx_pool, 1024, 1024, rlim.rlim_cur);
   acceptor->ctx_start_func = func;

   const int LISTEN_BACKLOG = 32768;
   int lfd = socket(PF_INET, SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_TCP);
   if (0 > lfd)
      return -1;

   int rc;

   const int option = 1;
   rc = setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
   if (0 > rc)
      return perror("setsockopt, SO_REUSEADDR"), rc;

   rc = setsockopt(lfd, IPPROTO_TCP, TCP_NODELAY, &option, sizeof(option));
   if (0 > rc)
      return perror("setsockopt, TCP_NODELAY"), rc;

   struct linger ls;
   ls.l_onoff = 0;
   ls.l_linger = 0;
   rc = setsockopt(lfd, SOL_SOCKET, SO_LINGER, (void *)&ls, sizeof(ls));
   if (0 > rc)
      return perror("setsockopt, SO_LINGER"), rc;

   struct sockaddr_in addr;
   memset(&addr, 0, sizeof(struct sockaddr_in));
   addr.sin_family = AF_INET;
   addr.sin_port = htons(port);
   addr.sin_addr.s_addr = INADDR_ANY;
   if (0 > bind(lfd, (struct sockaddr *)&addr, sizeof(addr)))
      return perror("bind"), -1;

   if (0 > listen(lfd, LISTEN_BACKLOG))
      return perror("listen"), -1;

   printf("listening on port: %d, backlog: %d\n", port, LISTEN_BACKLOG);

   acceptor->stack = malloc(ACCEPTOR_STACK_SIZE);
   ribs_makecontext(&acceptor->ctx, &main_ctx, acceptor->stack + ACCEPTOR_STACK_SIZE, accept_connections, NULL);
   acceptor->ctx.fd = lfd;
   acceptor->ctx.data.ptr = acceptor;
   struct epoll_event ev;
   ev.events = EPOLLIN;
   ev.data.ptr = &acceptor->ctx;
   if (0 > epoll_ctl(ribs_epoll_fd, EPOLL_CTL_ADD, lfd, &ev))
      return perror("epoll_ctl"), -1;

   return 0;
}
