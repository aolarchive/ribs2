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
    epoll_data_t userdata;
    int persistent;
    int http_status_code;
    char *content;
    uint32_t content_length;

    /* TODO: add initial buffer sizes */

    /* keep 1 byte aligned structs last */
#pragma pack(push, 1)
    struct http_client_key {
        struct in_addr addr;
        uint16_t port;
    } key;
#pragma pack(pop)
};

struct http_client_pool {
    struct ctx_pool ctx_pool;
};

int http_client_pool_init(struct http_client_pool *http_client_pool, size_t initial, size_t grow);
void http_client_close(struct http_client_pool *client_pool, struct http_client_context *cctx);
struct http_client_context *http_client_pool_create_client(struct http_client_pool *http_client_pool, struct in_addr addr, uint16_t port);
_RIBS_INLINE_ struct http_client_context *http_client_get_context(void);
_RIBS_INLINE_ struct http_client_context *http_client_get_last_context();
_RIBS_INLINE_ int http_client_send_request(struct http_client_context *cctx);

#include "../src/_http_client_pool.c"

#endif // _HTTP_CLIENT_POOL__H_
