#ifndef _HTTP_SERVER__H_
#define _HTTP_SERVER__H_

#include "ctx_pool.h"
#include "vmbuf.h"
#include "epoll_worker.h"

struct http_server_context {
    struct vmbuf request;
    struct vmbuf response;
    char data[];
};

struct http_server {
    struct ctx_pool ctx_pool;
    void (*user_func)();
    /* misc ctx */
    struct ribs_context accept_ctx;
    void *accept_stack;
    struct ribs_context idle_ctx;
    char *idle_stack;
};

int http_server_init(struct http_server *server, uint16_t port, void (*func)(void), size_t context_size);
void http_server_accept_connections(void);
void http_server_fiber_main(void);

/*
 * inline
 */
static inline struct http_server_context *http_server_get_context(void) {
    return (struct http_server_context *)(current_ctx->reserved);
}

#endif // _HTTP_SERVER__H_
