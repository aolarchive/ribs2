#include "http_server.h"
#include <sys/types.h>
#include <dirent.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/uio.h>
#include <sys/sendfile.h>
#include <sys/timerfd.h>
#include "mime_types.h"
#include "logger.h"

#define ACCEPTOR_STACK_SIZE 8192
#define MIN_HTTP_REQ_SIZE 5 // method(3) + space(1) + URI(1) + optional VER...
#define DEFAULT_MAX_REQ_SIZE 1024*1024*1024

/* methods */
SSTRL(HEAD, "HEAD " );
SSTRL(GET,  "GET "  );
SSTRL(POST, "POST " );
SSTRL(PUT,  "PUT "  );
/* misc */
SSTRL(HTTP_SERVER_VER, "HTTP/1.1");
SSTRL(HTTP_SERVER_NAME, "ribs2.0");
SSTRL(CRLFCRLF, "\r\n\r\n");
SSTRL(CRLF, "\r\n");
SSTRL(CONNECTION, "\r\nConnection: ");
SSTRL(CONNECTION_CLOSE, "close");
SSTRL(CONNECTION_KEEPALIVE, "Keep-Alive");
SSTRL(CONTENT_LENGTH, "\r\nContent-Length: ");
/* 1xx */
SSTRL(HTTP_STATUS_100, "100 Continue");
SSTRL(EXPECT_100, "\r\nExpect: 100");
/* 2xx */
SSTR(HTTP_STATUS_200, "200 OK");
/* 4xx */
SSTR(HTTP_STATUS_404, "404 Not Found");
SSTRL(HTTP_STATUS_411, "411 Length Required");
SSTRL(HTTP_STATUS_413, "413 Request Entity Too Large");
/* 5xx */
SSTR(HTTP_STATUS_500, "500 Internal Server Error");
SSTRL(HTTP_STATUS_501, "501 Not Implemented");
SSTRL(HTTP_STATUS_503, "503 Service Unavailable");
/* content types */
SSTR(HTTP_CONTENT_TYPE_TEXT_PLAIN, "text/plain");
SSTR(HTTP_CONTENT_TYPE_TEXT_HTML, "text/html");

static void http_server_process_request(char *uri, size_t uri_len, char *headers);

static void http_server_fiber_main_wrapper(void) {
    http_server_fiber_main();
    struct http_server *server = (struct http_server *)current_ctx->data.ptr;
    ctx_pool_put(&server->ctx_pool, current_ctx);
}

static void http_server_idle_handler(void) {
    struct http_server *server = (struct http_server *)current_ctx->data.ptr;
    for (;;) {
        if (last_epollev.events == EPOLLOUT)
            yield();
        else {
            struct ribs_context *new_ctx = ctx_pool_get(&server->ctx_pool);
            ribs_makecontext(new_ctx, &main_ctx, http_server_fiber_main_wrapper);
            int fd = last_epollev.data.fd;
            new_ctx->fd = fd;
            new_ctx->data.ptr = server;
            struct epoll_worker_fd_data *fd_data = epoll_worker_fd_map + fd;
            fd_data->ctx = new_ctx;
            TIMEOUT_HANDLER_REMOVE_FD_DATA(fd_data);
            ribs_swapcurcontext(new_ctx);
        }
    }
}

static void http_server_accept_connections(void);

int http_server_init(struct http_server *server, size_t context_size) {
    if (0 > mime_types_init())
        return LOGGER_ERROR("failed to initialize mime types"), -1;
    /*
     * idle connection handler
     */
    server->idle_ctx = ribs_context_create(SMALL_STACK_SIZE, http_server_idle_handler);
    server->idle_ctx->data.ptr = server;
    /*
     * context pool
     */
    if (0 == server->stack_size || 0 == server->num_stacks) {
        struct rlimit rlim;
        if (0 > getrlimit(RLIMIT_STACK, &rlim))
            return LOGGER_PERROR("getrlimit(RLIMIT_STACK)"), -1;

        long total_mem = sysconf(_SC_PHYS_PAGES);
        if (total_mem < 0)
            return LOGGER_PERROR("sysconf"), -1;
        total_mem *= getpagesize();
        size_t num_ctx_in_one_map = total_mem / rlim.rlim_cur;
        /* half of total mem to start with so we don't need to enable overcommit */
        num_ctx_in_one_map >>= 1;
        LOGGER_INFO("http server pool: initial=%zu, grow=%zu", num_ctx_in_one_map, num_ctx_in_one_map);
        ctx_pool_init(&server->ctx_pool, num_ctx_in_one_map, num_ctx_in_one_map, rlim.rlim_cur, sizeof(struct http_server_context) + context_size);
    } else {
        ctx_pool_init(&server->ctx_pool, server->num_stacks, server->num_stacks, server->stack_size, sizeof(struct http_server_context) + context_size);
    }


    /*
     * listen socket
     */
    const int LISTEN_BACKLOG = 32768;
    int lfd = socket(PF_INET, SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_TCP);
    if (0 > lfd)
        return -1;

    int rc;
    const int option = 1;
    rc = setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
    if (0 > rc)
        return LOGGER_PERROR("setsockopt, SO_REUSEADDR"), rc;

    rc = setsockopt(lfd, IPPROTO_TCP, TCP_NODELAY, &option, sizeof(option));
    if (0 > rc)
        return LOGGER_PERROR("setsockopt, TCP_NODELAY"), rc;

    struct linger ls;
    ls.l_onoff = 0;
    ls.l_linger = 0;
    rc = setsockopt(lfd, SOL_SOCKET, SO_LINGER, (void *)&ls, sizeof(ls));
    if (0 > rc)
        return LOGGER_PERROR("setsockopt, SO_LINGER"), rc;

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(server->port);
    addr.sin_addr.s_addr = INADDR_ANY;
    if (0 > bind(lfd, (struct sockaddr *)&addr, sizeof(addr)))
        return LOGGER_PERROR("bind"), -1;

    if (0 > listen(lfd, LISTEN_BACKLOG))
        return LOGGER_PERROR("listen"), -1;

    server->accept_ctx = ribs_context_create(ACCEPTOR_STACK_SIZE, http_server_accept_connections);
    server->accept_ctx->fd = lfd;
    server->accept_ctx->data.ptr = server;
    LOGGER_INFO("listening on port: %d, backlog: %d", server->port, LISTEN_BACKLOG);

    if (server->max_req_size == 0)
        server->max_req_size = DEFAULT_MAX_REQ_SIZE;
    return 0;
}

int http_server_init_acceptor(struct http_server *server) {
    int lfd = server->accept_ctx->fd;
    epoll_worker_fd_map[lfd].ctx = server->accept_ctx;
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = lfd;
    if (0 > epoll_ctl(ribs_epoll_fd, EPOLL_CTL_ADD, lfd, &ev))
        return LOGGER_PERROR("epoll_ctl"), -1;

    return timeout_handler_init(&server->timeout_handler);
}

static void http_server_accept_connections(void) {
    struct http_server *server = (struct http_server *)current_ctx->data.ptr;
    struct epoll_event ev;
    for (;; yield()) {
        struct sockaddr_in new_addr;
        socklen_t new_addr_size = sizeof(struct sockaddr_in);
        int fd = accept4(current_ctx->fd, (struct sockaddr *)&new_addr, &new_addr_size, SOCK_CLOEXEC | SOCK_NONBLOCK);
        if (fd < 0)
            continue;

        struct epoll_worker_fd_data *fd_data = epoll_worker_fd_map + fd;
        fd_data->ctx = server->idle_ctx;
        timeout_handler_add_fd_data(&server->timeout_handler, fd_data);

        ev.events = EPOLLIN | EPOLLOUT | EPOLLET;
        ev.data.fd = fd;
        if (0 > epoll_ctl(ribs_epoll_fd, EPOLL_CTL_ADD, fd, &ev))
            LOGGER_PERROR("epoll_ctl");
    }
}

static int check_persistent(char *p) {
    char *conn = strstr(p, CONNECTION);
    char *h1_1 = strstr(p, " HTTP/1.1");
    // HTTP/1.1
    if ((NULL != h1_1 &&
         (NULL == conn ||
          0 != SSTRNCMPI(CONNECTION_CLOSE, conn + SSTRLEN(CONNECTION)))) ||
        // HTTP/1.0
        (NULL == h1_1 &&
         NULL != conn &&
         0 == SSTRNCMPI(CONNECTION_KEEPALIVE, conn + SSTRLEN(CONNECTION))))
        return 1;
    else
        return 0;
}


void http_server_header_start(const char *status, const char *content_type) {
    struct http_server_context *ctx = http_server_get_context();
    vmbuf_sprintf(&ctx->header, "%s %s\r\nServer: %s\r\nContent-Type: %s%s%s", HTTP_SERVER_VER, status, HTTP_SERVER_NAME, content_type, CONNECTION, ctx->persistent ? CONNECTION_KEEPALIVE : CONNECTION_CLOSE);
}

void http_server_header_close() {
    struct http_server_context *ctx = http_server_get_context();
    vmbuf_strcpy(&ctx->header, CRLFCRLF);
}

void http_server_response(const char *status, const char *content_type) {
    struct http_server_context *ctx = http_server_get_context();
    vmbuf_reset(&ctx->header);
    http_server_header_start(status, content_type);
    http_server_header_content_length();
    http_server_header_close();
}

void http_server_response_sprintf(const char *status, const char *content_type, const char *format, ...) {
    struct http_server_context *ctx = http_server_get_context();
    vmbuf_reset(&ctx->header);
    vmbuf_reset(&ctx->payload);
    http_server_header_start(status, content_type);
    va_list ap;
    va_start(ap, format);
    vmbuf_vsprintf(&ctx->payload, format, ap);
    va_end(ap);
    http_server_header_content_length();
    http_server_header_close();
}

void http_server_header_content_length() {
    struct http_server_context *ctx = http_server_get_context();
    vmbuf_sprintf(&ctx->header, "%s%zu", CONTENT_LENGTH, vmbuf_wlocpos(&ctx->payload));
}

#define READ_FROM_SOCKET()                                              \
    res = vmbuf_read(&ctx->request, fd);                                \
    if (0 >= res) {                                                     \
        close(fd); /* remote side closed or other error occured */      \
        return;                                                         \
    }                                                                   \
    if (vmbuf_wlocpos(&ctx->request) > max_req_size) {                  \
        ctx->persistent = 0;                                            \
        http_server_response(HTTP_STATUS_413, HTTP_CONTENT_TYPE_TEXT_PLAIN); \
        http_server_write();                                            \
        close(fd);                                                      \
        return;                                                         \
    }


static inline void http_server_yield() {
    struct http_server *server = (struct http_server *)current_ctx->data.ptr;
    struct epoll_worker_fd_data *fd_data = epoll_worker_fd_map + current_ctx->fd;
    timeout_handler_add_fd_data(&server->timeout_handler, fd_data);
    yield();
    TIMEOUT_HANDLER_REMOVE_FD_DATA(fd_data);
}

inline void http_server_write() {
    struct http_server_context *ctx = http_server_get_context();
    struct iovec iovec[2] = {
        { vmbuf_data(&ctx->header), vmbuf_wlocpos(&ctx->header)},
        { vmbuf_data(&ctx->payload), vmbuf_wlocpos(&ctx->payload)}
    };
    int fd = current_ctx->fd;

    ssize_t num_write;
    for (;;http_server_yield()) {
        num_write = writev(fd, iovec, iovec[1].iov_len ? 2 : 1);
        if (0 > num_write) {
            if (EAGAIN == errno) {
                continue;
            } else {
                close(fd);
                return;
            }
        } else {
            if (num_write >= (ssize_t)iovec[0].iov_len) {
                num_write -= iovec[0].iov_len;
                iovec[0].iov_len = iovec[1].iov_len - num_write;
                if (iovec[0].iov_len == 0)
                    break;
                iovec[0].iov_base = iovec[1].iov_base + num_write;
                iovec[1].iov_len = 0;
            } else {
                iovec[0].iov_len -= num_write;
                iovec[0].iov_base += num_write;
            }
        }
    }
}

void http_server_fiber_main(void) {
    struct http_server_context *ctx = http_server_get_context();
    struct http_server *server = (struct http_server *)current_ctx->data.ptr;
    int fd = current_ctx->fd;

    char *URI;
    char *headers;
    char *content;
    size_t content_length;
    int res;
    ctx->persistent = 0;

    vmbuf_init(&ctx->request, server->init_request_size);
    vmbuf_init(&ctx->header, server->init_header_size);
    vmbuf_init(&ctx->payload, server->init_payload_size);
    size_t max_req_size = server->max_req_size;

    for (;; http_server_yield()) {
        READ_FROM_SOCKET();
        if (vmbuf_wlocpos(&ctx->request) > MIN_HTTP_REQ_SIZE)
            break;
    }
    do {
        if (0 == SSTRNCMP(GET, vmbuf_data(&ctx->request)) || 0 == SSTRNCMP(HEAD, vmbuf_data(&ctx->request))) {
            /* GET or HEAD */
            while (0 != SSTRNCMP(CRLFCRLF,  vmbuf_wloc(&ctx->request) - SSTRLEN(CRLFCRLF))) {
                http_server_yield();
                READ_FROM_SOCKET();
            }
            /* make sure the string is \0 terminated */
            /* this will overwrite the first CR */
            *(vmbuf_wloc(&ctx->request) - SSTRLEN(CRLFCRLF)) = 0;
            char *p = vmbuf_data(&ctx->request);
            ctx->persistent = check_persistent(p);
            URI = strchrnul(p, ' '); /* can't be NULL GET and HEAD constants have space at the end */
            *URI = 0;
            ++URI; // skip the space
            p = strchrnul(URI, '\r'); /* HTTP/1.0 */
            headers = p;
            if (0 != *headers) /* are headers present? */
                headers += SSTRLEN(CRLF); /* skip the new line */
            *p = 0;
            p = strchrnul(URI, ' '); /* truncate the version part */
            *p = 0; /* \0 at the end of URI */

            ctx->content = NULL;
            ctx->content_len = 0;

            /* minimal parsing and call user function */
            http_server_process_request(URI, p - URI + 1, headers);
        } else if (0 == SSTRNCMP(POST, vmbuf_data(&ctx->request)) || 0 == SSTRNCMP(PUT, vmbuf_data(&ctx->request))) {
            /* POST or PUT */
            for (;;) {
                *vmbuf_wloc(&ctx->request) = 0;
                /* wait until we have the header */
                if (NULL != (content = strstr(vmbuf_data(&ctx->request), CRLFCRLF)))
                    break;
                http_server_yield();
                READ_FROM_SOCKET();
            }

            *content = 0; /* terminate at the first CR like in GET */
            content += SSTRLEN(CRLFCRLF);

            if (strstr(vmbuf_data(&ctx->request), EXPECT_100)) {
                vmbuf_sprintf(&ctx->header, "%s %s\r\n\r\n", HTTP_SERVER_VER, HTTP_STATUS_100);
                if (0 > vmbuf_write(&ctx->header, fd)) {
                    close(fd);
                    return;
                }
                vmbuf_reset(&ctx->header);
            }
            ctx->persistent = check_persistent(vmbuf_data(&ctx->request));

            /* parse the content length */
            char *p = strcasestr(vmbuf_data(&ctx->request), CONTENT_LENGTH);
            if (NULL == p) {
                http_server_response(HTTP_STATUS_411, HTTP_CONTENT_TYPE_TEXT_PLAIN);
                break;
            }

            p += SSTRLEN(CONTENT_LENGTH);
            content_length = atoi(p);
            for (;;) {
                if (content + content_length <= vmbuf_wloc(&ctx->request))
                    break;
                http_server_yield();
                READ_FROM_SOCKET();
            }
            p = vmbuf_data(&ctx->request);
            URI = strchrnul(p, ' '); /* can't be NULL PUT and POST constants have space at the end */
            *URI = 0;
            ++URI; /* skip the space */
            p = strchrnul(URI, '\r'); /* HTTP/1.0 */
            headers = p;
            if (0 != *headers) /* are headers present? */
                headers += SSTRLEN(CRLF); /* skip the new line */
            *p = 0;
            p = strchrnul(URI, ' '); /* truncate http version */
            *p = 0; /* \0 at the end of URI */
            *(content + content_length) = 0;

            ctx->content = content;
            ctx->content_len = content_length;

            /* minimal parsing and call user function */
            http_server_process_request(URI, p - URI + 1, headers);
        } else {
            http_server_response(HTTP_STATUS_501, HTTP_CONTENT_TYPE_TEXT_PLAIN);
            break;
        }
    } while(0);

    if (vmbuf_wlocpos(&ctx->header) > 0) {
        epoll_worker_resume_events();
        http_server_write();
    }

    if (ctx->persistent) {
        struct epoll_worker_fd_data *fd_data = epoll_worker_fd_map + fd;
        fd_data->ctx = server->idle_ctx;
        timeout_handler_add_fd_data(&server->timeout_handler, fd_data);
    } else
        close(fd);
}

static void http_server_process_request(char *uri, size_t uri_len, char *headers) {
    struct http_server_context *ctx = http_server_get_context();
    struct http_server *server = (struct http_server *)current_ctx->data.ptr;
    ctx->headers = headers;
    char *query = strchrnul(uri, '?');
    if (*query)
        *query++ = 0;
    ctx->query = query;
    static const char HTTP[] = "http://";
    if (0 == SSTRNCMP(HTTP, uri)) {
        uri += SSTRLEN(HTTP);
        uri = strchrnul(uri, '/');
    }
    vmbuf_resize_if_less(&ctx->request, uri_len); /* prepare space for decoded URI */
    ctx->uri = uri;
    ctx->decoded_uri = NULL;
    epoll_worker_ignore_events();
    server->user_func();
}


int http_server_sendfile(const char *filename) {
    if (0 == *filename)
        filename = ".";
    struct http_server_context *ctx = http_server_get_context();
    int fd = current_ctx->fd;
    int ffd = open(filename, O_RDONLY);
    if (ffd < 0)
        return -2;
    struct stat st;
    if (0 > fstat(ffd, &st)) {
        LOGGER_PERROR(filename);
        close(ffd);
        return -2;
    }
    if (S_ISDIR(st.st_mode)) {
        close(ffd);
        return 1;
    }
    epoll_worker_resume_events();
    vmbuf_reset(&ctx->header);
    http_server_header_start(HTTP_STATUS_200, mime_types_by_filename(filename));
    vmbuf_sprintf(&ctx->header, "%s%zu", CONTENT_LENGTH, st.st_size);
    http_server_header_close();
    int option = 1;
    if (0 > setsockopt(fd, IPPROTO_TCP, TCP_CORK, &option, sizeof(option)))
        LOGGER_PERROR("TCP_CORK set");
    http_server_write();
    off_t ofs = 0;
    ssize_t res;
    int ret;
    for (;;http_server_yield()) {
        res = sendfile(fd, ffd, &ofs, st.st_size - ofs);
        if (res < 0) {
            if (EAGAIN == errno)
                continue;
            LOGGER_PERROR(filename);
            ctx->persistent = 0;
            ret = -1;
            break;
        }
        if (ofs < st.st_size)
            continue;
        ret = 0;
        break;
    }
    vmbuf_reset(&ctx->header);
    close(ffd);
    if (ctx->persistent) {
        option = 0;
        if (0 > setsockopt(fd, IPPROTO_TCP, TCP_CORK, &option, sizeof(option)))
            LOGGER_PERROR("TCP_CORK release");
    }
    return ret;
}

int http_server_generate_dir_list(const char *URI) {
    struct http_server_context *ctx = http_server_get_context();
    struct http_server *server = (struct http_server *)current_ctx->data.ptr;
    struct vmbuf *payload = &ctx->payload;
    const char *dir = URI;
    if (*dir == '/') ++dir;
    if (0 == *dir)
        dir = ".";
    vmbuf_sprintf(payload, "<html><head><title>Index of %s</title></head>", dir);
    vmbuf_strcpy(payload, "<body>");
    vmbuf_sprintf(payload, "<h1>Index of %s</h1><hr>", dir);

    vmbuf_sprintf(payload, "<a href=\"..\">../</a><br><br>");
    vmbuf_sprintf(payload, "<table width=\"100%\" border=\"0\">");
    DIR *d = opendir(dir);
    int error = 0;
    if (d) {
        struct dirent de, *dep;
        while (0 == readdir_r(d, &de, &dep) && dep) {
            if (de.d_name[0] == '.')
                continue;
            struct stat st;
            if (0 > fstatat(dirfd(d), de.d_name, &st, 0)) {
                vmbuf_sprintf(payload, "<tr><td>ERROR: %s</td><td>N/A</td></tr>", de.d_name);
                continue;
            }
            const char *slash = (S_ISDIR(st.st_mode) ? "/" : "");
            struct tm t_res, *t;
            t = localtime_r(&st.st_mtime, &t_res);

            vmbuf_strcpy(payload, "<tr>");
            vmbuf_sprintf(payload, "<td><a href=\"%s%s%s\">%s%s</a></td>", URI, de.d_name, slash, de.d_name, slash);
            vmbuf_strcpy(payload, "<td>");
            if (t)
                vmbuf_strftime(payload, "%F %T", t);
            vmbuf_strcpy(payload, "</td>");
            vmbuf_sprintf(payload, "<td>%zu</td>", st.st_size);
            vmbuf_strcpy(payload, "</tr>");
        }
        closedir(d);
    }
    vmbuf_strcpy(payload, "<tr><td colspan=3><hr></td></tr></table>");
    vmbuf_sprintf(payload, "<address>RIBS 2.0 Port %hu</address></body>", server->port);
    vmbuf_strcpy(payload, "</html>");
    return error;
}


