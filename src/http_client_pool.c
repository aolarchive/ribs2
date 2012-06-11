#include "http_client_pool.h"
#include <netinet/tcp.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <search.h>
#include "hashtable.h"
#include "sstr.h"
#include "list.h"

#define CLIENT_STACK_SIZE  65536

SSTRL(CRLFCRLF, "\r\n\r\n");
SSTRL(CRLF, "\r\n");
const char CR = '\r';
SSTRL(CONTENT_LENGTH, "\r\nContent-Length: ");
SSTRL(TRANSFER_ENCODING, "\r\nTransfer-Encoding: ");

SSTRL(CONNECTION, "\r\nConnection: ");
SSTRL(CONNECTION_CLOSE, "close");

#define SMALL_STACK_SIZE 4096

static struct list* client_chains = NULL;
static struct ribs_context idle_ctx;
static struct hashtable ht_persistent_clients = HASHTABLE_INITIALIZER;

void http_client_close(struct http_client_pool *client_pool, struct http_client_context *cctx) {
    if (cctx->persistent) {
        int fd = RIBS_RESERVED_TO_CONTEXT(cctx)->fd;
        epoll_worker_set_fd_ctx(fd, &idle_ctx);
        uint32_t ofs = hashtable_lookup(&ht_persistent_clients, &cctx->key, sizeof(struct http_client_key));
        if (0 == ofs) {
            /* TODO: bug: the list is on vmbuf, change to ofs */
            ofs = hashtable_insert_new(&ht_persistent_clients, &cctx->key, sizeof(struct http_client_key), sizeof(struct list));
            list_init((struct list *)hashtable_get_val(&ht_persistent_clients, ofs));
        }
        struct list *head = (struct list *)hashtable_get_val(&ht_persistent_clients, ofs);
        struct list *client = client_chains + fd;
        list_insert_head(head, client);
    }
    ctx_pool_put(&client_pool->ctx_pool, RIBS_RESERVED_TO_CONTEXT(cctx));
}

/* Idle client, ignore EPOLLOUT only, close on any other event */
static void http_client_idle_handler(void) {
    for (;;yield()) {
        if (last_epollev.events != EPOLLOUT) {
            int fd = last_epollev.data.fd;
            struct list *client = client_chains + fd;
            list_remove(client);
            close(fd);
            /* TODO: remove from hashtable when list is empty (first add remove method to hashtable) */
        }
    }
}

int http_client_pool_init(struct http_client_pool *http_client_pool, size_t initial, size_t grow) {
    if (0 > ctx_pool_init(&http_client_pool->ctx_pool, initial, grow, CLIENT_STACK_SIZE, sizeof(struct http_client_context)))
        return -1;

    /* Global to all clients */
    if (!client_chains) {

        struct rlimit rlim;
        if (0 > getrlimit(RLIMIT_NOFILE, &rlim))
            return perror("getrlimit(RLIMIT_NOFILE)"), -1;

        client_chains = calloc(rlim.rlim_cur, sizeof(struct list));
        if (!client_chains)
            return perror("calloc client_chais"), -1;

        void* idle_stack = malloc(SMALL_STACK_SIZE);
        ribs_makecontext(&idle_ctx, &main_ctx, idle_stack + SMALL_STACK_SIZE, http_client_idle_handler);

        hashtable_init(&ht_persistent_clients, rlim.rlim_cur);
    }
    return 0;
}

#define CLIENT_ERROR()                                   \
    {                                                    \
        perror("client error\n"); /* TODO: remove */     \
        close(fd);                                       \
        return;                                          \
    }

#define _READ_MORE_DATA(cond, extra)                        \
    while(cond) {                                           \
        if (res == 0)                                       \
            CLIENT_ERROR(); /* partial response */          \
        yield();                                            \
        if ((res = vmbuf_read(&ctx->response, fd)) < 0)     \
            CLIENT_ERROR();                                 \
        extra;                                              \
    }

#define READ_MORE_DATA_STR(cond)                                \
    _READ_MORE_DATA(cond, *vmbuf_wloc(&ctx->response) = 0)

#define READ_MORE_DATA(cond)                    \
    _READ_MORE_DATA(cond, )


void http_client_fiber_main(void) {
    struct http_client_context *ctx = http_client_get_context();
    int fd = current_ctx->fd;
    int persistent = 0;
    epoll_worker_set_last_fd(fd); /* needed in the case where epoll_wait never occured */

    /*
     * write request
     */
    int res;
    for (; (res = vmbuf_write(&ctx->request, fd)) == 0; yield());
    if (0 > res)
        perror("write");
    /*
     * HTTP header
     */
    uint32_t eoh_ofs;
    char *data;
    char *eoh;
    res = vmbuf_read(&ctx->response, fd);
    *vmbuf_wloc(&ctx->response) = 0;
    READ_MORE_DATA_STR(NULL == (eoh = strstr(data = vmbuf_data(&ctx->response), CRLFCRLF)));
    eoh_ofs = eoh - data + SSTRLEN(CRLFCRLF);
    *eoh = 0;
    char *p = strstr(data, CONNECTION);
    if (p != NULL) {
        p += SSTRLEN(CONNECTION);
        persistent = (0 == SSTRNCMPI(CONNECTION_CLOSE, p) ? 0 : 1);
    }
    SSTRL(HTTP, "HTTP/");
    if (0 != SSTRNCMP(HTTP, data))
        CLIENT_ERROR();

    p = strchrnul(data, ' ');
    int code = (*p ? atoi(p + 1) : 0);
    if (0 == code)
        CLIENT_ERROR();
    do {
        if (code == 204 || code == 304) /* No Content,  Not Modified */
            break;

        char *content_len_str = strstr(data, CONTENT_LENGTH);
        if (NULL != content_len_str) {
            content_len_str += SSTRLEN(CONTENT_LENGTH);
            size_t content_end = eoh_ofs + atoi(content_len_str);
            READ_MORE_DATA(vmbuf_wlocpos(&ctx->response) < content_end);
            break;
        }
        char *transfer_encoding_str = strstr(data, TRANSFER_ENCODING);
        if (NULL != transfer_encoding_str &&
            0 == SSTRNCMP(transfer_encoding_str + SSTRLEN(TRANSFER_ENCODING), "chunked")) {
            size_t chunk_start = eoh_ofs;
            char *p;
            for (;;) {
                READ_MORE_DATA_STR((p = strchrnul((data = vmbuf_data(&ctx->response)) + chunk_start, '\r')) == 0);
                if (0 != SSTRNCMP(CRLF, p))
                    CLIENT_ERROR();
                uint32_t s = strtoul(data + chunk_start, NULL, 16);
                if (0 == s)
                    break;
                p += SSTRLEN(CRLF); /* points to beginning of chunk data */
                size_t move_size = vmbuf_wloc(&ctx->response) - p;
                memmove(data + chunk_start, p, move_size);
                vmbuf_wlocset(&ctx->response, chunk_start + move_size);
                *vmbuf_wloc(&ctx->response) = 0;
                size_t chunk_end = chunk_start + s + SSTRLEN(CRLF);
                READ_MORE_DATA(vmbuf_wlocpos(&ctx->response) < chunk_end);
                chunk_start = chunk_end;
            }
            break;
        }
        for (;;yield()) {
            if ((res = vmbuf_read(&ctx->response, fd)) < 0)
                CLIENT_ERROR();
            if (0 == res)
                break;
        }
    } while (0);
    ctx->content = vmbuf_data_ofs(&ctx->response, eoh_ofs);
    ctx->content_length = vmbuf_wlocpos(&ctx->response) - eoh_ofs;
    ctx->http_status_code = code;
    vmbuf_data_ofs(&ctx->response, eoh_ofs - SSTRLEN(CRLFCRLF))[0] = CR;
    *vmbuf_wloc(&ctx->response) = 0;
    ctx->persistent = persistent;
    if (!persistent)
        close(fd);
}

struct http_client_context *http_client_pool_create_client(struct http_client_pool *http_client_pool, struct in_addr addr, uint16_t port) {
    int cfd;
    struct http_client_key key = { .addr = addr, .port = port };
    uint32_t ofs = hashtable_lookup(&ht_persistent_clients, &key, sizeof(struct http_client_key));
    struct list *head;
    if (ofs > 0 && !list_empty(head = (struct list *)hashtable_get_val(&ht_persistent_clients, ofs))) {
        struct list *client = list_pop_head(head);
        cfd = client - client_chains;
    } else {
        cfd = socket(PF_INET, SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_TCP);
        if (0 > cfd)
            return perror("socket"), NULL;

        const int option = 1;
        if (0 > setsockopt(cfd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option)))
            return perror("setsockopt SO_REUSEADDR"), NULL;

        if (0 > setsockopt(cfd, IPPROTO_TCP, TCP_NODELAY, &option, sizeof(option)))
            return perror("setsockopt TCP_NODELAY"), NULL;

        struct sockaddr_in saddr = { .sin_family = AF_INET, .sin_port = htons(port), .sin_addr = addr };
        connect(cfd, (struct sockaddr *)&saddr, sizeof(saddr));
        struct epoll_event ev;
        ev.events = EPOLLIN | EPOLLOUT | EPOLLET;
        ev.data.fd = cfd;
        if (0 > epoll_ctl(ribs_epoll_fd, EPOLL_CTL_ADD, cfd, &ev))
            perror("epoll_ctl");
    }
    struct ribs_context *new_ctx = ctx_pool_get(&http_client_pool->ctx_pool);
    new_ctx->fd = cfd;
    new_ctx->data.ptr = http_client_pool;
    epoll_worker_fd_map[cfd].ctx = new_ctx;
    ribs_makecontext(new_ctx, current_ctx, new_ctx, http_client_fiber_main);
    struct http_client_context *cctx = (struct http_client_context *)new_ctx->reserved;
    cctx->key = (struct http_client_key){ .addr = addr, .port = port };
    vmbuf_init(&cctx->request, 4096);
    vmbuf_init(&cctx->response, 4096);
    return cctx;

}
