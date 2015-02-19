/*
    This file is part of RIBS2.0 (Robust Infrastructure for Backend Systems).
    RIBS is an infrastructure for building great SaaS applications (but not
    limited to).

    Copyright (C) 2012,2013,2014 Adap.tv, Inc.

    RIBS is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, version 2.1 of the License.

    RIBS is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with RIBS.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "http_client_pool.h"
#include <netinet/tcp.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <search.h>
#include <fnmatch.h>
#ifdef RIBS2_SSL
#include <openssl/x509v3.h>
#endif
#include "hashtable.h"
#include "sstr.h"
#include "list.h"

#define CLIENT_STACK_SIZE 65536

SSTRL(CRLFCRLF, "\r\n\r\n");
SSTRL(CRLF, "\r\n");
const char CR = '\r';
SSTRL(CONTENT_LENGTH, "\r\nContent-Length: ");
SSTRL(TRANSFER_ENCODING, "\r\nTransfer-Encoding: ");

SSTRL(CONNECTION, "\r\nConnection: ");
SSTRL(CONNECTION_CLOSE, "close");

static struct http_client_context* last_ctx = NULL;
static struct list* client_chains = NULL;
static struct list* client_heads = NULL;
static struct list free_list = LIST_INITIALIZER(free_list);
static struct ribs_context *idle_ctx;
static struct hashtable ht_persistent_clients = HASHTABLE_INITIALIZER;

void http_client_free(struct http_client_context *cctx) {
    if (cctx->persistent) {
        int fd = cctx->fd;
        epoll_worker_set_fd_ctx(fd, idle_ctx);
        uint32_t ofs = hashtable_lookup(&ht_persistent_clients, &cctx->key, sizeof(struct http_client_key));
        struct list *head;
        if (0 == ofs) {
            if (list_empty(&free_list)) {
                ribs_close(fd);
                return;
            }
            head = list_pop_head(&free_list);
            uint32_t h = head - client_heads;
            hashtable_insert(&ht_persistent_clients, &cctx->key, sizeof(struct http_client_key), &h, sizeof(h));
            list_init(head);
        } else
            head = client_heads + *(uint32_t *)hashtable_get_val(&ht_persistent_clients, ofs);
        struct list *client = client_chains + fd;
        list_insert_tail(head, client);
        struct epoll_worker_fd_data *fd_data = epoll_worker_fd_map + fd;
        timeout_handler_add_fd_data(&cctx->pool->timeout_handler_persistent, fd_data);
    }
    ctx_pool_put(&cctx->pool->ctx_pool, RIBS_RESERVED_TO_CONTEXT(cctx));
}

/* Idle client, ignore EPOLLOUT only, close on any other event */
static void http_client_idle_handler(void) {
    for (;;yield()) {
        if (last_epollev.events != EPOLLOUT) {
            int fd = last_epollev.data.fd;
            struct list *client = client_chains + fd;
            list_remove(client);
            ribs_close(fd);
            struct epoll_worker_fd_data *fd_data = epoll_worker_fd_map + fd;
            TIMEOUT_HANDLER_REMOVE_FD_DATA(fd_data);
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

        idle_ctx = ribs_context_create(SMALL_STACK_SIZE, 0, http_client_idle_handler);

        hashtable_init(&ht_persistent_clients, rlim.rlim_cur);
    }

    if (0 > timeout_handler_init(&http_client_pool->timeout_handler) ||
        0 > timeout_handler_init(&http_client_pool->timeout_handler_persistent))
        return -1;

#ifdef RIBS2_SSL
    http_client_pool->ssl_ctx = NULL;
#endif
    return 0;
}

#ifdef RIBS2_SSL
int http_client_pool_init_ssl(struct http_client_pool *http_client_pool, size_t initial, size_t grow, char *cacert) {
    if (-1 == http_client_pool_init(http_client_pool, initial, grow))
        return -1;

    /* init */
    SSL_library_init();
    SSL_load_error_strings();
    http_client_pool->ssl_ctx = SSL_CTX_new(SSLv23_client_method());

    if (0 != ribs_ssl_set_options(http_client_pool->ssl_ctx, NULL))
        return -1;

    if (cacert) {
        if (!SSL_CTX_load_verify_locations(http_client_pool->ssl_ctx, cacert, NULL))
            return LOGGER_ERROR("Unable to read Certificate Authority certificates file"), -1;
        http_client_pool->check_cert = 1;
    } else
        http_client_pool->check_cert = 0;

    SSL_CTX_set_verify(http_client_pool->ssl_ctx, cacert ? SSL_VERIFY_PEER : SSL_VERIFY_NONE, NULL);

    return 0;
}
#endif

#define CLIENT_ERROR()                   \
    {                                    \
        ctx->http_status_code = 500;     \
        ctx->persistent = 0;             \
        ribs_close(fd);                  \
        return;                          \
    }

#define __READ_MORE_DATA(cond, container, buf, th, fd, extra)           \
    extra;                                                              \
    while(cond) {                                                       \
        if (*res <= 0)                                                  \
            return -1; /* partial response */                           \
        if (errno == EAGAIN)                                            \
            http_client_yield_ignore_epollout(th, fd);                  \
        if ((*res = container ## _read(buf, fd)) < 0) {                 \
            LOGGER_PERROR("read %s:%hu",                                \
                          inet_ntoa(cctx->key.addr),                    \
                          cctx->key.port);                              \
            return -1;                                                  \
        }                                                               \
        extra;                                                          \
    }

#ifndef RIBS2_SSL
#define _READ_MORE_DATA(cond, container, buf, th, fd, extra)        \
    __READ_MORE_DATA(cond, container, buf, th, fd, extra)
#else
#define _READ_MORE_DATA(cond, container, buf, th, fd, extra)            \
    SSL *ssl = ribs_ssl_get(fd);                                        \
    if (!ssl) {                                                         \
               __READ_MORE_DATA(cond, container, buf, th, fd, extra)    \
               }                                                        \
    else {                                                              \
        extra;                                                          \
        while (cond) {                                                  \
            if (*res < 0) {                                             \
                int err = SSL_get_error(ssl, *res);                     \
                if (SSL_ERROR_WANT_READ == err ||                       \
                    SSL_ERROR_WANT_WRITE == err)                        \
                    http_client_yield(th, fd);                          \
                else {                                                  \
                    LOGGER_PERROR("SSL_read %s:%hu %s",                 \
                                  inet_ntoa(cctx->key.addr),            \
                                  cctx->key.port,                       \
                                  ERR_reason_error_string(ERR_get_error())); \
                    return -1;                                          \
                }                                                       \
            }                                                           \
            *res = SSL_read(ssl,                                        \
                            container ## _wloc(buf),                    \
                            container ## _wavail(buf));                 \
            if (0 == *res)                                              \
                return -1;                                              \
            if (*res > 0)                                               \
                if (0 > container ## _wseek(buf, *res))                 \
                    return -1;                                          \
            extra;                                                      \
        }                                                               \
    }
#endif

#define READ_MORE_DATA_STR(cond, container, buf, th, fd)            \
    _READ_MORE_DATA(cond, container, buf, th, fd,                   \
                    *(container ## _wloc(buf)) = 0)

#define READ_MORE_DATA(cond, container, buf, th, fd)                \
    _READ_MORE_DATA(cond, container, buf, th, fd, )

#ifndef RIBS2_SSL
#define READ_DATA_STR(cond, container, buf, th, fd)                 \
    *res = container ## _read(buf, fd);                             \
    READ_MORE_DATA_STR(cond, container, buf, th, fd)
#else
#define READ_DATA_STR(cond, container, buf, th, fd)                 \
    *res = 1, errno = 0;                                            \
    READ_MORE_DATA_STR(cond, container, buf, th, fd)
#endif

static inline void http_client_yield(struct timeout_handler* timeout_handler, int fd) {
    struct epoll_worker_fd_data *fd_data = epoll_worker_fd_map + fd;
    timeout_handler_add_fd_data(timeout_handler, fd_data);
    yield();
    TIMEOUT_HANDLER_REMOVE_FD_DATA(fd_data);
}

static inline void http_client_yield_ignore_epollout(struct timeout_handler* timeout_handler, int fd) {
    struct epoll_worker_fd_data *fd_data = epoll_worker_fd_map + fd;
    timeout_handler_add_fd_data(timeout_handler, fd_data);
    do
        yield();
    while (EPOLLOUT == last_epollev.events);
    TIMEOUT_HANDLER_REMOVE_FD_DATA(fd_data);
}

inline int http_client_write_request(struct http_client_context *cctx, struct timeout_handler* th)
{
    int res;
#ifdef RIBS2_SSL
    SSL *ssl = ribs_ssl_get(cctx->fd);
    if (!ssl) {
#endif
        for (;
             (res = vmbuf_write(&cctx->request, cctx->fd)) == 0;
             http_client_yield(th, cctx->fd));
        if (0 > res)
            return LOGGER_PERROR("write %s:%hu",inet_ntoa(cctx->key.addr), cctx->key.port), -1;
#ifdef RIBS2_SSL
    } else {
        size_t rav;
        while ((rav = vmbuf_ravail(&cctx->request)) > 0) {
            res = SSL_write(ssl, vmbuf_rloc(&cctx->request), rav);
            if (res > 0) {
                vmbuf_rseek(&cctx->request, res);
                continue;
            }
            int err = SSL_get_error(ssl, res);
            if (res < 0 && (SSL_ERROR_WANT_WRITE == err ||
                            SSL_ERROR_WANT_READ == err)) {
                http_client_yield(th, cctx->fd);
                continue;
            }
            LOGGER_PERROR("SSL_write %s:%hu %s",inet_ntoa(cctx->key.addr), cctx->key.port, ERR_reason_error_string(ERR_get_error()));
            return -1;
        }
    }
#endif
    return 0;
}

inline int http_client_read_headers(struct http_client_context *cctx, int *code, uint32_t *eoh_ofs, int *res, char **data, struct timeout_handler *th)
{
    char *eoh;
    READ_DATA_STR(NULL == (eoh = strstr(*data = vmbuf_data(&cctx->response), CRLFCRLF)),
                  vmbuf, &cctx->response, th, cctx->fd)
    *eoh_ofs = eoh - *data + SSTRLEN(CRLFCRLF);
    *eoh = 0;
    cctx->persistent = 1;
    char *p = strstr(*data, CONNECTION);
    if (p != NULL) {
        p += SSTRLEN(CONNECTION);
        if (0 == SSTRNCMPI(CONNECTION_CLOSE, p))
            cctx->persistent = 0;
    }
    SSTRL(HTTP, "HTTP/");
    if (0 != SSTRNCMP(HTTP, *data))
            return -1;

    p = strchrnul(*data, ' ');
    *code = (*p ? atoi(p + 1) : 0);
    if (0 == *code)
        return -1;
    return 0;
}

inline int http_client_read_body(struct http_client_context *cctx, int *code, struct vmfile *infile, uint32_t *eoh_ofs, int *res, char **data, struct timeout_handler *th)
{
    struct vmbuf *response = &cctx->response;
    do {
        if (*code == 204 || *code == 304) /* No Content,  Not Modified */
            break;

        if (infile) {
            /*Copy extra data into vmfile*/
            vmfile_memcpy(infile, vmbuf_data_ofs(response, *eoh_ofs), (vmbuf_wlocpos(response) - *eoh_ofs));
            *eoh_ofs = 0;// since we are saving body to a file
        }

        /*
         * content length
         */
        char *content_len_str = strstr(*data, CONTENT_LENGTH);
        if (NULL != content_len_str) {
            content_len_str += SSTRLEN(CONTENT_LENGTH);
            char *content_length = NULL;
            errno = 0;
            uint64_t content_len = strtol(content_len_str, &content_length, 0);
            if (0 != errno || content_len_str == content_length)
                return -1;
            size_t content_end = *eoh_ofs + content_len;
            if (infile) {
                READ_MORE_DATA(vmfile_wlocpos(infile) < content_end, vmfile, infile, th, cctx->fd);
            }
            else {
                READ_MORE_DATA(vmbuf_wlocpos(response) < content_end, vmbuf, response, th, cctx->fd);
            }
            break;
        }
        /*
         * chunked encoding
         */
        char *transfer_encoding_str = strstr(*data, TRANSFER_ENCODING);
        char *p=NULL;
        if (NULL != transfer_encoding_str &&
            0 == SSTRNCMP(transfer_encoding_str + SSTRLEN(TRANSFER_ENCODING), "chunked")) {
            size_t chunk_start = *eoh_ofs;
            size_t data_start = *eoh_ofs;
            for (;;) {
                if (infile) {
                    READ_MORE_DATA_STR(*(p = strchrnul((*data = vmfile_data(infile)) + chunk_start, '\r')) == 0, vmfile, infile, th, cctx->fd);
                }
                else {
                    READ_MORE_DATA_STR(*(p = strchrnul((*data = vmbuf_data(response)) + chunk_start, '\r')) == 0, vmbuf, response, th, cctx->fd);
                }
                if (0 != SSTRNCMP(CRLF, p))
                    return -1;
                uint32_t s = strtoul(*data + chunk_start, NULL, 16);
                if (0 == s) {
                    if (infile)
                        vmfile_wlocset(infile, data_start);
                    else
                        vmbuf_wlocset(response, data_start);
                    break;
                }
                chunk_start = p - *data + SSTRLEN(CRLF);
                size_t chunk_end = chunk_start + s + SSTRLEN(CRLF);
                if (infile) {
                    READ_MORE_DATA(vmfile_wlocpos(infile) < chunk_end, vmfile, infile, th, cctx->fd);
                    memmove(vmfile_data(infile) + data_start, vmfile_data(infile) + chunk_start, s);
                 } else {
                    READ_MORE_DATA(vmbuf_wlocpos(response) < chunk_end, vmbuf, response, th, cctx->fd);
                    memmove(vmbuf_data(response) + data_start, vmbuf_data(response) + chunk_start, s);
                 }
                data_start += s;
                chunk_start = chunk_end;
            }
            break;
        }
        /*
         * determine content length by server closing connection
         */
        int read_until_close() {
            READ_MORE_DATA(1, vmbuf, response, th, cctx->fd);
            return 0;
        }
        if (read_until_close() < 0 && *res != 0)
            return -1;
    } while (0);
    return 0;
}

void http_client_fiber_main(void) {
    struct http_client_context *ctx = (struct http_client_context *)current_ctx->reserved;
    int fd = ctx->fd;
    struct timeout_handler *th = &ctx->pool->timeout_handler;
    struct epoll_worker_fd_data *fd_data = epoll_worker_fd_map + fd;
    TIMEOUT_HANDLER_REMOVE_FD_DATA(fd_data);

    epoll_worker_set_last_fd(fd); /* needed in the case where epoll_wait never occured */

#ifdef RIBS2_SSL
    SSL *ssl = ribs_ssl_get(fd);
    if (ssl && !ctx->ssl_connected) {
        for (;;http_client_yield(th, fd)) {
            int ret = SSL_connect(ssl);
            if (1 == ret)
                break;
            int err = SSL_get_error(ssl, ret);
            if (-1 == ret) {
                if (SSL_ERROR_WANT_WRITE == err ||
                    SSL_ERROR_WANT_READ == err)
                    continue;
            }
            LOGGER_PERROR("SSL_connect %s:%hu %s",inet_ntoa(ctx->key.addr), ctx->key.port, ERR_reason_error_string(ERR_get_error()));
            CLIENT_ERROR();
        }
        ctx->ssl_connected = 1;

        /* TODO: hostname verification might break in case of SNI and persistent connection
           proposed solution: The key for persistent should include the hostname as well */

        if (ctx->pool->check_cert && ctx->hostname) {
            const X509 *server_cert = SSL_get_peer_certificate(ssl);
            if (!server_cert) {
                LOGGER_ERROR("SSL_get_peer_certificate(): no certificate");
                CLIENT_ERROR();
            }
            int ndots(const char *s) {
                int r = 0;
                for(; *s; ++s) if (*s == '.') ++r;
                return r;
            }
            int hdots = ndots(ctx->hostname);
            int matched = 0;
            // Extract the names within the SAN extension from the certificate
            STACK_OF(GENERAL_NAME) *san_names = X509_get_ext_d2i((X509 *) server_cert, NID_subject_alt_name, NULL, NULL);
            if (san_names != NULL) {
                int san_names_nb = sk_GENERAL_NAME_num(san_names);
                int i = 0;
                for (; i<san_names_nb; i++) {
                    const GENERAL_NAME *current_name = sk_GENERAL_NAME_value(san_names, i);
                    if (current_name->type == GEN_DNS) {
                        // Current name is a DNS name, let's check it
                        char *dns_name = (char *) ASN1_STRING_data(current_name->d.dNSName);
                        // RFC2818
                        if (ndots(dns_name) == hdots &&
                            0 == fnmatch(dns_name, ctx->hostname, FNM_NOESCAPE|FNM_PATHNAME|FNM_PERIOD|FNM_CASEFOLD)) {
                            matched = 1;
                            break;
                        }
                    }
		}
                sk_GENERAL_NAME_pop_free(san_names, GENERAL_NAME_free);
            }
            if (!matched) {
                // Find the position of the CN field in the Subject field of the certificate
                int common_name_loc = X509_NAME_get_index_by_NID(X509_get_subject_name((X509 *) server_cert), NID_commonName, -1);
                if (common_name_loc >= 0) {
                    X509_NAME_ENTRY *common_name_entry = X509_NAME_get_entry(X509_get_subject_name((X509 *) server_cert), common_name_loc);
                    if (common_name_entry != NULL) {
                        ASN1_STRING *common_name_asn1 = X509_NAME_ENTRY_get_data(common_name_entry);
                        if (common_name_asn1 != NULL) {
                            char *common_name_str = (char *) ASN1_STRING_data(common_name_asn1);
                            // RFC2818
                            if (ndots(common_name_str) == hdots &&
                                0 == fnmatch(common_name_str, ctx->hostname, FNM_NOESCAPE|FNM_PATHNAME|FNM_PERIOD|FNM_CASEFOLD))
                                matched = 1;
                        }
                    }
                }
            }
            if (!matched) {
                LOGGER_ERROR("Certificate do not match hostname: %s", ctx->hostname);
                CLIENT_ERROR();
            }
        }
    }
#endif

    //write_request
    if ( 0 > http_client_write_request(ctx, th))
        CLIENT_ERROR();

    //read headers
    uint32_t eoh_ofs;
    int code;
    char *data;
    int res;
    if ( 0 > http_client_read_headers(ctx, &code, &eoh_ofs, &res, &data, th))
        CLIENT_ERROR();

    if ( 0 > http_client_read_body(ctx, &code, NULL, &eoh_ofs, &res, &data, th))
        CLIENT_ERROR();

    ctx->content = vmbuf_data_ofs(&ctx->response, eoh_ofs);
    vmbuf_rlocset(&ctx->response, eoh_ofs);
    ctx->content_length = vmbuf_wlocpos(&ctx->response) - eoh_ofs;
    ctx->http_status_code = code;
    vmbuf_data_ofs(&ctx->response, eoh_ofs - SSTRLEN(CRLFCRLF))[0] = CR;
    *vmbuf_wloc(&ctx->response) = 0;
    if (!ctx->persistent)
        ribs_close(fd);
}

void http_client_fiber_main_wrapper(void) {
    http_client_fiber_main();
    last_ctx = (struct http_client_context *)current_ctx->reserved;
}

int http_client_pool_get_request(struct http_client_pool *http_client_pool, struct in_addr addr, uint16_t port, const char *hostname, const char *format, ...) {
    struct http_client_context *cctx = http_client_pool_create_client2(http_client_pool, addr, port, hostname, NULL);
    if (NULL == cctx)
        return -1;
    vmbuf_strcpy(&cctx->request, "GET ");
    va_list ap;
    va_start(ap, format);
    vmbuf_vsprintf(&cctx->request, format, ap);
    va_end(ap);
    vmbuf_sprintf(&cctx->request, " HTTP/1.1\r\nHost: %s\r\n\r\n", hostname);
    if (0 > http_client_send_request(cctx))
        return http_client_free(cctx), -1;
    return 0;
}

int http_client_pool_get_request2(struct http_client_pool *http_client_pool, struct in_addr addr, uint16_t port, const char *hostname, const char **headers, const char *format, ...) {
    struct http_client_context *cctx = http_client_pool_create_client2(http_client_pool, addr, port, hostname, NULL);
    if (NULL == cctx)
        return -1;
    vmbuf_strcpy(&cctx->request, "GET ");
    va_list ap;
    va_start(ap, format);
    vmbuf_vsprintf(&cctx->request, format, ap);
    va_end(ap);
    vmbuf_sprintf(&cctx->request, " HTTP/1.1\r\nHost: %s", hostname);
    if (headers) {
        for (; NULL != *headers; ++headers) {
            vmbuf_strcpy(&cctx->request, "\r\n");
            vmbuf_strcpy(&cctx->request, *headers);
        }
    }
    vmbuf_strcpy(&cctx->request, "\r\n\r\n");
    if (0 > http_client_send_request(cctx))
        return http_client_free(cctx), -1;
    return 0;
}

int http_client_pool_post_request(struct http_client_pool *http_client_pool,
        struct in_addr addr, uint16_t port, const char *hostname,
        const char *data, size_t size_of_data, const char *format, ...) {
    struct http_client_context *cctx = http_client_pool_create_client2(http_client_pool, addr, port, hostname, NULL);
    if (NULL == cctx)
        return -1;
    vmbuf_strcpy(&cctx->request, "POST ");
    va_list ap;
    va_start(ap, format);
    vmbuf_vsprintf(&cctx->request, format, ap);
    va_end(ap);
    vmbuf_sprintf(&cctx->request, " HTTP/1.1\r\nHost: %s\r\nContent-Type: application/octet-stream\r\nContent-Length: %zu\r\n\r\n", hostname, size_of_data);
    vmbuf_memcpy(&cctx->request, data, size_of_data);
    if (0 > http_client_send_request(cctx))
        return http_client_free(cctx), -1;
    return 0;
}

struct http_client_context *http_client_pool_post_request_init(struct http_client_pool *http_client_pool,
        struct in_addr addr, uint16_t port, const char *hostname, const char *format, ...) {
    struct http_client_context *cctx = http_client_pool_create_client2(http_client_pool, addr, port, hostname, NULL);
    if (NULL == cctx)
        return NULL;
    vmbuf_strcpy(&cctx->request, "POST ");
    va_list ap;
    va_start(ap, format);
    vmbuf_vsprintf(&cctx->request, format, ap);
    va_end(ap);
    vmbuf_sprintf(&cctx->request, " HTTP/1.1\r\nHost: %s", hostname);
    return cctx;
}

/* inline */
inline struct http_client_context *_http_client_pool_create_client(struct http_client_pool *http_client_pool, struct in_addr addr, uint16_t port, const char *hostname, struct ribs_context *rctx, void (*func)(void)) {
    int cfd;
    struct http_client_key key = { .addr = addr, .port = port };
    uint32_t ofs = hashtable_lookup(&ht_persistent_clients, &key, sizeof(struct http_client_key));
    struct list *head;
    struct ribs_context *new_ctx;
    struct epoll_worker_fd_data *fd_data;
    struct http_client_context *cctx;
    const struct timeval epoch = {0,0};

    /* find matching client fd, if not found, creat one */
    if (ofs > 0 && !list_empty(head = client_heads + *(uint32_t *)hashtable_get_val(&ht_persistent_clients, ofs))) {
        struct list *client = list_pop_tail(head);
        cfd = client - client_chains;
        fd_data = epoll_worker_fd_map + cfd;
        // do not use already shutdown()
        if (timercmp(&fd_data->timestamp, &epoch, !=)) {
            TIMEOUT_HANDLER_REMOVE_FD_DATA(fd_data);
            new_ctx = ctx_pool_get(&http_client_pool->ctx_pool);
            cctx = (struct http_client_context *)new_ctx->reserved;
#ifdef RIBS2_SSL
            /* persistent connection must be ssl_connected */
            cctx->ssl_connected = 1; /* this would be ignored in non-ssl */
#endif
            goto CONNECTED;
        } else {
            list_insert_tail(head, client);
        }
    }
    cfd = socket(PF_INET, SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_TCP);
    if (0 > cfd)
        return LOGGER_PERROR("socket"), NULL;
    const int option = 1;
    if (0 > setsockopt(cfd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option)))
        return LOGGER_PERROR("setsockopt SO_REUSEADDR"), ribs_close(cfd), NULL;
    if (0 > setsockopt(cfd, IPPROTO_TCP, TCP_NODELAY, &option, sizeof(option)))
        return LOGGER_PERROR("setsockopt TCP_NODELAY"), ribs_close(cfd), NULL;
    struct sockaddr_in saddr = { .sin_family = AF_INET, .sin_port = htons(port), .sin_addr = addr };
    if (0 > connect(cfd, (struct sockaddr *)&saddr, sizeof(saddr)) && EINPROGRESS != errno)
        return LOGGER_PERROR("connect %s:%hu",inet_ntoa(addr), port), ribs_close(cfd), NULL;
    if (0 > ribs_epoll_add(cfd, EPOLLIN | EPOLLOUT | EPOLLET, event_loop_ctx))
        return ribs_close(cfd), NULL;

#ifdef RIBS2_SSL
    SSL *ssl = NULL;
    if (http_client_pool->ssl_ctx) {
        ssl = ribs_ssl_alloc(cfd, http_client_pool->ssl_ctx);
        if (NULL == ssl)
            return LOGGER_ERROR("ssl alloc"), ribs_close(cfd), NULL;
    }
#endif
    new_ctx = ctx_pool_get(&http_client_pool->ctx_pool);
    cctx = (struct http_client_context *)new_ctx->reserved;
    fd_data = epoll_worker_fd_map + cfd;
#ifdef RIBS2_SSL
    if (ssl) {
        SSL_set_tlsext_host_name(ssl, hostname);
        cctx->hostname = hostname;
        SSL_set_connect_state(ssl);
        cctx->ssl_connected = 0;
    }
#else
    (void)hostname;
#endif
 CONNECTED:
    fd_data->ctx = new_ctx;
    if (func)
        ribs_makecontext(new_ctx, rctx ? rctx : current_ctx, func);
    else
        epoll_worker_set_fd_ctx(cfd, rctx ? rctx : current_ctx);
    cctx->fd = cfd;
    cctx->pool = http_client_pool;
    cctx->key.addr = addr;
    cctx->key.port = port;
    vmbuf_init(&cctx->request, 4096);
    vmbuf_init(&cctx->response, 4096);
    cctx->content = NULL;
    cctx->content_length = 0;
    cctx->persistent = 0;
    timeout_handler_add_fd_data(&http_client_pool->timeout_handler, fd_data);
    return cctx;
}

struct http_client_context *http_client_pool_create_client2(struct http_client_pool *http_client_pool, struct in_addr addr, uint16_t port, const char *hostname, struct ribs_context *rctx) {
    return _http_client_pool_create_client(http_client_pool, addr, port, hostname, rctx, http_client_fiber_main_wrapper);
}

struct http_client_context *http_client_pool_create_client(struct http_client_pool *http_client_pool, struct in_addr addr, uint16_t port, struct ribs_context *rctx) {
    return _http_client_pool_create_client(http_client_pool, addr, port, NULL, rctx, http_client_fiber_main_wrapper);
}

int http_client_pool_post_request_content_type(struct http_client_context *cctx, const char *content_type) {
    return vmbuf_sprintf(&cctx->request, "\r\nContent-Type: %s", content_type);
}

int http_client_pool_post_request_send(struct http_client_context *cctx, struct vmbuf *post_data) {
    size_t size = vmbuf_wlocpos(post_data);
    vmbuf_sprintf(&cctx->request, "\r\nContent-Length: %zu\r\n\r\n", size);
    // TODO: use writev instead of copying
    vmbuf_memcpy(&cctx->request, vmbuf_data(post_data), size);
    if (0 > http_client_send_request(cctx))
        return http_client_free(cctx), -1;
    return 0;
}

void http_client_close_free(struct http_client_context *cctx) {
    cctx->persistent = 0;
    ribs_close(cctx->fd);
    http_client_free(cctx);
}

int
http_client_get_file(struct http_client_pool *http_client_pool, struct vmfile *infile, struct in_addr addr,
                    uint16_t port, const char *hostname, int compression, int *file_compressed, const char *format, ...) {

    struct http_client_context *cctx = _http_client_pool_create_client(http_client_pool, addr, port, hostname, NULL, NULL);
    if (NULL == cctx)
        return -1;

    int cfd = cctx->fd;
    struct epoll_worker_fd_data *fd_data = epoll_worker_fd_map + cfd;
    TIMEOUT_HANDLER_REMOVE_FD_DATA(fd_data);

    vmbuf_strcpy(&cctx->request, "GET ");
    va_list ap;
    va_start(ap, format);
    vmbuf_vsprintf(&cctx->request, format, ap);
    va_end(ap);
    vmbuf_sprintf(&cctx->request, " HTTP/1.1\r\n");
    if (compression)
        vmbuf_sprintf(&cctx->request, "Accept-Encoding: gzip\r\n");
    vmbuf_sprintf(&cctx->request, "Host: %s\r\n\r\n", hostname);

#ifdef RIBS2_SSL
    SSL *ssl = ribs_ssl_get(cfd);
    if (ssl && !cctx->ssl_connected) {
        for (;;http_client_yield(&cctx->pool->timeout_handler, cfd)) {
            int ret = SSL_connect(ssl);
            if (1 == ret)
                break;
            int err = SSL_get_error(ssl, ret);
            if (-1 == ret) {
                if (SSL_ERROR_WANT_WRITE == err ||
                    SSL_ERROR_WANT_READ == err)
                    continue;
            }
            LOGGER_PERROR("SSL_connect %s:%hu %s",inet_ntoa(cctx->key.addr), cctx->key.port, ERR_reason_error_string(ERR_get_error()));
            return http_client_close_free(cctx), -1;
        }
        cctx->ssl_connected = 1;
    }
#endif

    // send request
    if (0 > http_client_write_request(cctx, &cctx->pool->timeout_handler))
        return http_client_close_free(cctx), -1;

    uint32_t eoh_ofs;
    int code;
    int res;
    char *data;

    // read headers
    if ( 0 > http_client_read_headers(cctx, &code, &eoh_ofs, &res, &data, &cctx->pool->timeout_handler))
        return http_client_close_free(cctx), -1;

    vmbuf_rlocset(&cctx->response, eoh_ofs);

    SSTRL(CONTENT_ENCODING_GZIP, "Content-Encoding: gzip");
    char * headers = http_client_response_headers(cctx);
    if (strstr(headers, CONTENT_ENCODING_GZIP))
        *file_compressed = 1;

    // read into file
    if ( 0 > http_client_read_body(cctx, &code, infile, &eoh_ofs, &res, &data, &cctx->pool->timeout_handler))
        return http_client_close_free(cctx), -1;

    if (!cctx->persistent)
        ribs_close(cfd);
    http_client_free(cctx);

    return code;
}

struct http_client_context *http_client_get_last_context(void) {
    return last_ctx;
}

void http_client_reuse_context(struct http_client_context *ctx) {
    ribs_makecontext(RIBS_RESERVED_TO_CONTEXT(ctx), current_ctx, http_client_fiber_main_wrapper);
}
