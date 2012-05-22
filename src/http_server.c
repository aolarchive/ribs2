#include "http_server.h"
#include <stdio.h>
#include "vmbuf.h"

extern struct ribs_context *current_ctx;

int http_server_init(struct http_server *server, uint16_t port, void (*func)(void)) {
    server->user_func = func;
    if (acceptor_init(&server->acceptor, port, http_server_fiber_main) < 0)
        return -1;
    return 0;
}

void http_server_fiber_main(void) {
   struct vmbuf request;
   int fd = current_ctx->fd;
   vmbuf_read(&request, fd);
   printf("in http_server\n");
}
