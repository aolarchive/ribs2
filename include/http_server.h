#ifndef _HTTP_SERVER__H_
#define _HTTP_SERVER__H_

#include "ribs_defs.h"
#include "ctx_pool.h"
#include "vmbuf.h"
#include "sstr.h"
#include "timeout_handler.h"
#include "hashtable.h"
#include "uri_decode.h"

struct http_server_context {
    struct vmbuf request;
    struct vmbuf header;
    struct vmbuf payload;
    char *uri;
    char *decoded_uri;
    char *headers;
    char *query;
    struct hashtable query_params;
    char *content;
    uint32_t content_len;
    int persistent;
    char user_data[];
};

struct http_server {
    uint16_t port;
    struct ctx_pool ctx_pool;
    void (*user_func)();
    /* misc ctx */
    struct ribs_context *accept_ctx;
    struct ribs_context *idle_ctx;
    struct timeout_handler timeout_handler;
    size_t stack_size; /* set to zero for auto */
    size_t num_stacks; /* set to zero for auto */
    size_t init_request_size;
    size_t init_header_size;
    size_t init_payload_size;
    size_t max_req_size;
};

#define HTTP_SERVER_INIT_DEFAULTS .stack_size = 0, .num_stacks = 0, .init_request_size = 8*1024, .init_header_size = 8*1024, .init_payload_size = 8*1024, .max_req_size = 0
#define HTTP_SERVER_INITIALIZER { HTTP_SERVER_INIT_DEFAULTS }


SSTREXTRN(HTTP_STATUS_200);
SSTREXTRN(HTTP_STATUS_404);
SSTREXTRN(HTTP_STATUS_500);
SSTREXTRN(HTTP_CONTENT_TYPE_TEXT_PLAIN);
SSTREXTRN(HTTP_CONTENT_TYPE_TEXT_HTML);

#define HTTP_SERVER_NOT_FOUND (-2)

int http_server_init(struct http_server *server, size_t context_size);
int http_server_init_acceptor(struct http_server *server);
void http_server_header_start(const char *status, const char *content_type);
void http_server_header_close();
void http_server_response(const char *status, const char *content_type);
void http_server_response_sprintf(const char *status, const char *content_type, const char *format, ...);
void http_server_header_content_length();
void http_server_fiber_main(void);
int http_server_sendfile(const char *filename);
int http_server_generate_dir_list(const char *filename);

/*
 * inline
 */
static inline struct http_server_context *http_server_get_context(void) {
    return (struct http_server_context *)(current_ctx->reserved);
}

static inline const char *http_server_get_query_param(struct http_server_context *ctx, const char *name, const char *default_val) {
    uint32_t ofs = hashtable_lookup(&ctx->query_params, name, strlen(name));
    return (ofs ? hashtable_get_val(&ctx->query_params, ofs) : default_val);
}

static inline void http_server_decode_uri() {
    struct http_server_context *ctx = http_server_get_context();
    http_uri_decode(ctx->uri, vmbuf_wloc(&ctx->request));
}

static inline void http_server_parse_query_params(size_t num_expected_params) {
    struct http_server_context *ctx = http_server_get_context();
    hashtable_init(&ctx->query_params, num_expected_params);
    http_uri_decode_query_params(ctx->query, &ctx->query_params);
}

#endif // _HTTP_SERVER__H_
