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
static struct list* client_heads = NULL;
static struct list free_list = LIST_INITIALIZER(free_list);
uint32_t next_avail_head = 0;
static struct ribs_context *idle_ctx;
static struct hashtable ht_persistent_clients = HASHTABLE_INITIALIZER;

void http_client_free(struct http_client_pool *client_pool, struct http_client_context *cctx) {
    if (cctx->persistent) {
        int fd = RIBS_RESERVED_TO_CONTEXT(cctx)->fd;
        epoll_worker_set_fd_ctx(fd, idle_ctx);
        uint32_t ofs = hashtable_lookup(&ht_persistent_clients, &cctx->key, sizeof(struct http_client_key));
        struct list *head;
        if (0 == ofs) {
            if (list_empty(&free_list)) {
                close(fd);
                return;
            }
            head = list_pop_head(&free_list);
            uint32_t h = head - client_heads;
            hashtable_insert(&ht_persistent_clients, &cctx->key, sizeof(struct http_client_key), &h, sizeof(h));
            list_init(head);
        } else
            head = client_heads + *(uint32_t *)hashtable_get_val(&ht_persistent_clients, ofs);
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
            /* TODO: insert list head into free list */
        }
    }
}

int http_client_pool_init(struct http_client_pool *http_client_pool, size_t initial, size_t grow) {
    LOGGER_INFO("http client pool: initial=%zu, grow=%zu", initial, grow);
    if (0 > ctx_pool_init(&http_client_pool->ctx_pool, initial, grow, CLIENT_STACK_SIZE, sizeof(struct http_client_context)))
        return -1;

    /* Global to all clients */
    if (!client_chains) {
        struct rlimit rlim;
        if (0 > getrlimit(RLIMIT_NOFILE, &rlim))
            return LOGGER_PERROR("getrlimit(RLIMIT_NOFILE)"), -1;

        client_chains = calloc(rlim.rlim_cur, sizeof(struct list));
        if (!client_chains)
            return LOGGER_PERROR("calloc client_chains"), -1;

        /* storage for multiple client chains */
        client_heads = calloc(rlim.rlim_cur, sizeof(struct list));
        struct list *tmp = client_heads, *tmp_end = tmp + rlim.rlim_cur;
        if (!client_heads)
            return LOGGER_PERROR("calloc client_heads"), -1;
        for (;tmp != tmp_end; ++tmp)
            list_insert_tail(&free_list, tmp);

        idle_ctx = ribs_context_create(SMALL_STACK_SIZE, http_client_idle_handler);

        hashtable_init(&ht_persistent_clients, rlim.rlim_cur);
    }
    return timeout_handler_init(&http_client_pool->timeout_handler);
}

#define CLIENT_ERROR()                                   \
    {                                                    \
        ctx->http_status_code = 500;                     \
        ctx->persistent = 0;                             \
        close(fd);                                       \
        return;                                          \
    }

#define _READ_MORE_DATA(cond, extra)                        \
    while(cond) {                                           \
        if (res == 0)                                       \
            CLIENT_ERROR(); /* partial response */          \
        http_client_yield();                                \
        if ((res = vmbuf_read(&ctx->response, fd)) < 0) {   \
            LOGGER_PERROR("read");                                 \
            CLIENT_ERROR();                                 \
        }                                                   \
        extra;                                              \
    }

#define READ_MORE_DATA_STR(cond)                                \
    _READ_MORE_DATA(cond, *vmbuf_wloc(&ctx->response) = 0)

#define READ_MORE_DATA(cond)                    \
    _READ_MORE_DATA(cond, )

static inline void http_client_yield() {
    struct http_client_pool *client_pool = (struct http_client_pool *)current_ctx->data.ptr;
    struct epoll_worker_fd_data *fd_data = epoll_worker_fd_map + current_ctx->fd;
    timeout_handler_add_fd_data(&client_pool->timeout_handler, fd_data);
    yield();
    TIMEOUT_HANDLER_REMOVE_FD_DATA(fd_data);
}

void http_client_fiber_main(void) {
    struct http_client_context *ctx = (struct http_client_context *)current_ctx->reserved;
    int fd = current_ctx->fd;
    struct epoll_worker_fd_data *fd_data = epoll_worker_fd_map + fd;
    TIMEOUT_HANDLER_REMOVE_FD_DATA(fd_data);

    int persistent = 0;
    epoll_worker_set_last_fd(fd); /* needed in the case where epoll_wait never occured */

    /*
     * write request
     */
    int res;
    for (; (res = vmbuf_write(&ctx->request, fd)) == 0; http_client_yield());
    if (0 > res) {
        LOGGER_PERROR("write");
        CLIENT_ERROR();
    }
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
        /*
         * content length
         */
        char *content_len_str = strstr(data, CONTENT_LENGTH);
        if (NULL != content_len_str) {
            content_len_str += SSTRLEN(CONTENT_LENGTH);
            size_t content_end = eoh_ofs + atoi(content_len_str);
            READ_MORE_DATA(vmbuf_wlocpos(&ctx->response) < content_end);
            break;
        }
        /*
         * chunked encoding
         */
        char *transfer_encoding_str = strstr(data, TRANSFER_ENCODING);
        if (NULL != transfer_encoding_str &&
            0 == SSTRNCMP(transfer_encoding_str + SSTRLEN(TRANSFER_ENCODING), "chunked")) {
            size_t chunk_start = eoh_ofs;
            size_t data_start = eoh_ofs;
            char *p;
            for (;;) {
                READ_MORE_DATA_STR((p = strchrnul((data = vmbuf_data(&ctx->response)) + chunk_start, '\r')) == 0);
                if (0 != SSTRNCMP(CRLF, p))
                    CLIENT_ERROR();
                uint32_t s = strtoul(data + chunk_start, NULL, 16);
                if (0 == s) {
                    vmbuf_wlocset(&ctx->response, data_start);
                    break;
                }
                chunk_start = p - data + SSTRLEN(CRLF);
                size_t chunk_end = chunk_start + s + SSTRLEN(CRLF);
                READ_MORE_DATA(vmbuf_wlocpos(&ctx->response) < chunk_end);
                memmove(vmbuf_data(&ctx->response) + data_start, vmbuf_data(&ctx->response) + chunk_start, s);
                data_start += s;
                chunk_start = chunk_end;
            }
            break;
        }
        /*
         * older versions of HTTP, terminated by disconnect
         */
        for (;;yield()) {
            if ((res = vmbuf_read(&ctx->response, fd)) < 0)
                CLIENT_ERROR();
            if (0 == res)
                break; /* remote side closed connection */
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

struct http_client_context *http_client_pool_create_client(struct http_client_pool *http_client_pool, struct in_addr addr, uint16_t port, struct ribs_context *rctx) {
    int cfd;
    struct http_client_key key = { .addr = addr, .port = port };
    uint32_t ofs = hashtable_lookup(&ht_persistent_clients, &key, sizeof(struct http_client_key));
    struct list *head;
    if (ofs > 0 && !list_empty(head = client_heads + *(uint32_t *)hashtable_get_val(&ht_persistent_clients, ofs))) {
        struct list *client = list_pop_head(head);
        cfd = client - client_chains;
    } else {
        cfd = socket(PF_INET, SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_TCP);
        if (0 > cfd)
            return LOGGER_PERROR("socket"), NULL;

        const int option = 1;
        if (0 > setsockopt(cfd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option)))
            return LOGGER_PERROR("setsockopt SO_REUSEADDR"), close(cfd), NULL;

        if (0 > setsockopt(cfd, IPPROTO_TCP, TCP_NODELAY, &option, sizeof(option)))
            return LOGGER_PERROR("setsockopt TCP_NODELAY"), close(cfd), NULL;

        struct sockaddr_in saddr = { .sin_family = AF_INET, .sin_port = htons(port), .sin_addr = addr };
        if (0 > connect(cfd, (struct sockaddr *)&saddr, sizeof(saddr)) && EINPROGRESS != errno)
            return LOGGER_PERROR("connect"), close(cfd), NULL;

        struct epoll_event ev;
        ev.events = EPOLLIN | EPOLLOUT | EPOLLET;
        ev.data.fd = cfd;
        if (0 > epoll_ctl(ribs_epoll_fd, EPOLL_CTL_ADD, cfd, &ev))
            return LOGGER_PERROR("epoll_ctl"), close(cfd), NULL;
    }
    struct ribs_context *new_ctx = ctx_pool_get(&http_client_pool->ctx_pool);
    new_ctx->fd = cfd;
    new_ctx->data.ptr = http_client_pool;
    struct epoll_worker_fd_data *fd_data = epoll_worker_fd_map + cfd;
    fd_data->ctx = new_ctx;
    ribs_makecontext(new_ctx, rctx ? rctx : current_ctx, http_client_fiber_main);
    struct http_client_context *cctx = (struct http_client_context *)new_ctx->reserved;
    cctx->key = (struct http_client_key){ .addr = addr, .port = port };
    vmbuf_init(&cctx->request, 4096);
    vmbuf_init(&cctx->response, 4096);
    timeout_handler_add_fd_data(&http_client_pool->timeout_handler, fd_data);
    return cctx;
}

int http_client_pool_get_request(struct http_client_pool *http_client_pool, struct in_addr addr, uint16_t port, const char *hostname, const char *format, ...) {
    struct http_client_context *cctx = http_client_pool_create_client(http_client_pool, addr, port, NULL);
    if (NULL == cctx)
        return -1;
    vmbuf_strcpy(&cctx->request, "GET ");
    va_list ap;
    va_start(ap, format);
    vmbuf_vsprintf(&cctx->request, format, ap);
    va_end(ap);
    vmbuf_sprintf(&cctx->request, " HTTP/1.1\r\nHost: %s\r\n\r\n", hostname);
    if (0 > http_client_send_request(cctx)) {
        http_client_free(http_client_pool, cctx);
        return -1;
    }
    return 0;
}
