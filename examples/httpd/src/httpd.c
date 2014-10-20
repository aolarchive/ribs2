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
#include "ribs.h"
#include <getopt.h>
#include <unistd.h>

/* do not allow files outside current working directory */
char *current_dir_name = NULL;
size_t current_dir_name_size = 0;

/*
 * file server
 */
void simple_file_server(void) {
    /* get the current http server context */
    struct http_server_context *ctx = http_server_get_context();

    /* uri decode the uri in the context */
    http_server_decode_uri(ctx->uri);

    /* remove the leading slash, we will be serving files from the
       current directory */
    const char *file = ctx->uri;
    if (*file == '/') ++file;

    if (*file) {
        char *rp = ribs_malloc(PATH_MAX);
        if (NULL == realpath(file, rp) ||
            0 != strncmp(current_dir_name, rp, current_dir_name_size)) {
            http_server_response_sprintf(HTTP_STATUS_404,
                                         HTTP_CONTENT_TYPE_TEXT_PLAIN, "not found: %s", file);
            return;
        }
    }

    int res = http_server_sendfile(file);
    if (0 > res) {
        /* not found */
        if (HTTP_SERVER_NOT_FOUND == res)
            http_server_response_sprintf(HTTP_STATUS_404,
                                         HTTP_CONTENT_TYPE_TEXT_PLAIN, "not found: %s", file);
        else
            http_server_response_sprintf(HTTP_STATUS_500,
                                         HTTP_CONTENT_TYPE_TEXT_PLAIN, "500 Internal Server Error: %s", file);
    }
    else if (0 < res) {
        /* directory */
        if (0 > http_server_generate_dir_list(file)) {
            http_server_response_sprintf(HTTP_STATUS_404,
                                         HTTP_CONTENT_TYPE_TEXT_PLAIN, "dir not found: %s", ctx->uri);
            return;
        }
        http_server_response(HTTP_STATUS_200,
                             HTTP_CONTENT_TYPE_TEXT_HTML);
    }
}

int main(int argc, char *argv[]) {
    static struct option long_options[] = {
        {"port", 1, 0, 'p'},
        {"daemonize", 0, 0, 'd'},
        {"forks", 1, 0, 'f'},
        {"ssl_port", 1, 0 ,'s'},
        {"key_file", 1, 0, 'k'},
        {"chain_file", 1, 0, 'c'},
        {"cipher_list", 1, 0, 'l'},
        {0, 0, 0, 0}
    };

    int port = 8080;
    int daemon_mode = 0;
    int forks = 0;
#ifdef RIBS2_SSL
    int sport = 8443;
    char *key_file = NULL;
    char *chain_file = NULL;
    char *cipher_list = NULL;
#endif
    for (;;) {
        int option_index = 0;
        int c = getopt_long(argc, argv, "p:f:s:c:k:l:d", long_options, &option_index);
        if (c == -1)
            break;
        switch (c) {
        case 'p':
            port = atoi(optarg);
            break;
        case 'd':
            daemon_mode = 1;
            break;
        case 'f':
            forks = atoi(optarg);
            break;
#ifdef RIBS2_SSL
        case 'k':
            key_file = optarg;
            break;
        case 'c':
            chain_file = optarg;
            break;
        case 's':
            sport = atoi(optarg);
            break;
        case 'l':
            cipher_list = optarg;
            break;
#endif
        }
    }

    char *cd = get_current_dir_name();
    current_dir_name = malloc(PATH_MAX);
    if (NULL == realpath(cd, current_dir_name)) {
        LOGGER_PERROR("realpath");
        exit(EXIT_FAILURE);
    }
    free(cd);
    current_dir_name_size = strlen(current_dir_name);

    /*
     * server config
     */
    struct http_server server = HTTP_SERVER_INITIALIZER;
    /* port number */
    server.port = port,
    /* call simple_file_server upon receiving http request */
    server.user_func = simple_file_server,
    /* set idle connection timeout to 60 seconds */
    server.timeout_handler.timeout = 60000,
    /* set fiber's stack size to automatic (0) */
    server.stack_size = 0,
    /* start the server with 100 stacks */
    /* more stacks will be created if necessary */
    server.num_stacks =  100,
    /* we expect most of our requests to be less than 8K */
    server.init_request_size = 8192,
    /* we expect most of our response headers to be less than
       8K */
    server.init_header_size = 8192,
    /* we expect most of our response payloads to be less than
       8K */
    server.init_payload_size = 8192,
    /* no limit on the request size, this should be set to
       something reasonable if you want to protect your server
       against denial of service attack */
    server.max_req_size = 0,
    /* no additional space is needed in the context to store app
       specified data (fiber local storage) */
    server.context_size = 0,
    server.bind_addr = htonl(INADDR_ANY);
#ifdef RIBS2_SSL
    server.use_ssl = 0;

    /*
     * ssl server config
     */
    struct http_server server_ssl = HTTP_SERVER_INITIALIZER;
    /* port number */
    server_ssl.port = sport,
    /* call simple_file_server upon receiving http request */
    server_ssl.user_func = simple_file_server,
    /* set idle connection timeout to 60 seconds */
    server_ssl.timeout_handler.timeout = 60000,
    /* set fiber's stack size to automatic (0) */
    server_ssl.stack_size = 0,
    /* start the server with 100 stacks */
    /* more stacks will be created if necessary */
    server_ssl.num_stacks =  100,
    /* we expect most of our requests to be less than 8K */
    server_ssl.init_request_size = 8192,
    /* we expect most of our response headers to be less than
       8K */
    server_ssl.init_header_size = 8192,
    /* we expect most of our response payloads to be less than
       8K */
    server_ssl.init_payload_size = 8192,
    /* no limit on the request size, this should be set to
       something reasonable if you want to protect your server
       against denial of service attack */
    server_ssl.max_req_size = 0,
    /* no additional space is needed in the context to store app
       specified data (fiber local storage) */
    server_ssl.context_size = 0,
    /* accept connections from any address */
    server_ssl.bind_addr = htonl(INADDR_ANY),
    server_ssl.use_ssl = 1,
    server_ssl.privatekey_file = key_file,
    server_ssl.certificate_chain_file = chain_file;

    if (cipher_list)
        server_ssl.cipher_list = cipher_list;

    if (key_file && 0 > http_server_init2(&server_ssl))
        exit(EXIT_FAILURE);
#endif

    /* initialize server, but don't accept connections yet */
    if (0 > http_server_init2(&server))
        exit(EXIT_FAILURE);

    if (0 > ribs_server_init(daemon_mode, "httpd.pid", "httpd.log", forks))
        exit(EXIT_FAILURE);

    /* initialize the event loop */
    if (0 > epoll_worker_init())
        exit(EXIT_FAILURE);

    /* start accepting connections, must be called after initializing
       epoll worker */
    if (0 > http_server_init_acceptor(&server))
        exit(EXIT_FAILURE);
#ifdef RIBS2_SSL
    if (key_file && 0 > http_server_init_acceptor(&server_ssl))
        exit(EXIT_FAILURE);
#endif
    ribs_server_start();
    return 0;
}
