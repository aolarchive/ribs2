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

static void my_server(void) {
    http_server_response_sprintf(HTTP_STATUS_200, HTTP_CONTENT_TYPE_TEXT_PLAIN, "hello world\n");
}

int main(void) {
    /* must use the initializer or set all the values in http_server */
    struct http_server server = HTTP_SERVER_INITIALIZER;
    /* port number */
    server.port = 8080;
    /* call my_server upon receiving http request */
    server.user_func = my_server;
    /* init and check for errors */
    if (0 > http_server_init(&server)) {
        printf("http_server_init failed\n");
        exit(EXIT_FAILURE);
    }

    /* initialize the event loop */
    if (0 > epoll_worker_init()) {
        printf("epoll_worker_init failed\n");
        exit(EXIT_FAILURE);
    }

    /* tell http server to start accepting connections */
    if (0 > http_server_init_acceptor(&server)) {
        printf("http_server_init_acceptor failed\n");
        exit(EXIT_FAILURE);
    }

    /* lastly, we need to handle events */
    epoll_worker_loop();
    return 0;
}
