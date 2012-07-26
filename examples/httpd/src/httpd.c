/*
    This file is part of RIBS2.0 (Robust Infrastructure for Backend Systems).
    RIBS is an infrastructure for building great SaaS applications (but not
    limited to).

    Copyright (C) 2012 Adap.tv, Inc.

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
/*
 * file server
 */
void simple_file_server(void) {
    struct http_server_context *ctx = http_server_get_context();
    http_server_decode_uri(ctx->uri);

    const char *file = ctx->uri;
    if (*file == '/') ++file;

    int res = http_server_sendfile(file);
    if (0 > res) { // not found
        if (HTTP_SERVER_NOT_FOUND == res)
            http_server_response_sprintf(HTTP_STATUS_404, HTTP_CONTENT_TYPE_TEXT_PLAIN, "not found");
    }
    else if (0 < res) { // directory
        if (0 > http_server_generate_dir_list(ctx->uri)) {
            http_server_response_sprintf(HTTP_STATUS_404, HTTP_CONTENT_TYPE_TEXT_PLAIN, "not found");
            return;
        }
        http_server_response(HTTP_STATUS_200, HTTP_CONTENT_TYPE_TEXT_HTML);
    }
}

int main(int argc, char *argv[]) {
    static struct option long_options[] = {
        {"port", 1, 0, 'p'},
        {"daemonize", 0, 0, 'd'},
        {"help", 0, 0, 1},
        {0, 0, 0, 0}
    };

    int port = 8080;
    int daemon_mode = 0;
    while (1) {
        int option_index = 0;
        int c = getopt_long(argc, argv, "p:d", long_options, &option_index);
        if (c == -1)
            break;
        switch (c) {
        case 'p':
            port = atoi(optarg);
            break;
        case 'd':
            daemon_mode = 1;
            break;
        }
    }

    /* server config */
    struct http_server server = {
        .port = port,
        .user_func = simple_file_server,
        .timeout_handler.timeout = 60000,
        .stack_size = 64*1024,
        .num_stacks =  100,
        .init_request_size = 8192,
        .init_header_size = 8192,
        .init_payload_size = 8192,
        .max_req_size = 0,
        .context_size = 0
    };

    /* initialize server, but don't accept connections yet */
    if (0 > http_server_init(&server))
        exit(EXIT_FAILURE);

    if (daemon_mode)
        daemonize();

    /* fork should be called here in case of multiple processes
       listening on the same socket. for file server, one process is
       more than enough */
    /* fork(); */
    if (epoll_worker_init() < 0)
        exit(EXIT_FAILURE);

    /* start accepting connections, must be called after initializing
       epoll worker */
    if (0 > http_server_init_acceptor(&server))
        exit(EXIT_FAILURE);

    epoll_worker_loop();
    return 0;
}
