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
#include "epoll_worker.h"

static struct ribs_context acceptor_ctx;
extern struct ribs_context main_ctx;
extern struct ribs_context *current_ctx;
static char stk[4096];

void accept_connections(void) {
   for (;;) {
      struct sockaddr_in new_addr;
      socklen_t new_addr_size = sizeof(struct sockaddr_in);
      int fd = accept4(current_ctx->fd, (struct sockaddr *)&new_addr, &new_addr_size, SOCK_CLOEXEC | SOCK_NONBLOCK);
      if (fd > -1)
         close(fd);

      yield();
   }
}

int acceptor_init(uint16_t port) {
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

   printf("listening on port: %d, backlog: %d", port, LISTEN_BACKLOG);

   ribs_makecontext(&acceptor_ctx, &main_ctx, stk, sizeof(stk), accept_connections);
   acceptor_ctx.fd = lfd;
   struct epoll_event ev;
   ev.events = EPOLLIN;
   ev.data.ptr = &acceptor_ctx;
   if (0 > epoll_ctl(ribs_epoll_fd, EPOLL_CTL_ADD, lfd, &ev))
      return perror("epoll_ctl"), -1;

   return 0;
}
