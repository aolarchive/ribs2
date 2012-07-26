#include "ribs.h"

static void my_server(void) {
    http_server_response_sprintf(HTTP_STATUS_200, HTTP_CONTENT_TYPE_TEXT_PLAIN, "hello world\n");
}

int main(void) {
    /* must use the initializer or set all the values in http_server */
    struct http_server server = HTTP_SERVER_INITIALIZER;
    /* port number */
    server.port = 8000;
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
