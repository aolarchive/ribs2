#ifndef _HTTP_CLIENT_POOL__H_
#define _HTTP_CLIENT_POOL__H_

#include "ribs_defs.h"
#include "ctx_pool.h"
#include "vmbuf.h"
#include "epoll_worker.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

struct http_client_context {
    struct vmbuf request;
    struct vmbuf response;
    epoll_data_t data;
};

struct http_client_pool {
    struct ctx_pool ctx_pool;
};

int http_client_pool_init(struct http_client_pool *http_client_pool, size_t initial, size_t grow);

struct http_client_context *http_client_pool_create_client(struct http_client_pool *http_client_pool, struct in_addr addr, uint16_t port);

/*
 * inline
 */
static inline struct http_client_context *http_client_get_context(void) {
    return (struct http_client_context *)(current_ctx->reserved);
}

static inline struct http_client_context *http_client_get_last_context() {
    return (struct http_client_context *)epoll_worker_fd_map[last_epollev.data.fd].ctx->reserved;
}

static inline void http_client_close(struct http_client_pool *client_pool, struct http_client_context *cctx) {
    struct ribs_context *ctx = ribs_reserved_to_context(cctx);
    ctx_pool_put(&client_pool->ctx_pool, ctx);
}


#endif // _HTTP_CLIENT_POOL__H_
