#ifndef _HTTP_SERVER__H_
#define _HTTP_SERVER__H_

#include "ctx_pool.h"
#include "vmbuf.h"
#include "epoll_worker.h"
#include "sstr.h"

struct http_server_context {
    struct vmbuf request;
    struct vmbuf header;
    struct vmbuf payload;
    int persistent;
    char user_data[];
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

SSTREXTRN(HTTP_STATUS_200);
SSTREXTRN(HTTP_STATUS_404);
SSTREXTRN(HTTP_CONTENT_TYPE_TEXT_PLAIN);

int http_server_init(struct http_server *server, uint16_t port, void (*func)(void), size_t context_size);
void http_server_accept_connections(void);
void http_server_header_start(const char *status, const char *content_type);
void http_server_header_close();
void http_server_response(const char *status, const char *content_type);
void http_server_response_sprintf(const char *status, const char *content_type, const char *format, ...);
void http_server_header_content_length();
void http_server_fiber_main(void);
int http_server_sendfile(const char *filename);

/*
 * inline
 */
static inline struct http_server_context *http_server_get_context(void) {
    return (struct http_server_context *)(current_ctx->reserved);
}

#endif // _HTTP_SERVER__H_
