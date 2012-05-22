#ifndef _HTTP_SERVER__H_
#define _HTTP_SERVER__H_

#include "acceptor.h"

struct http_server {
    struct acceptor acceptor;
    void (*user_func)();
};

int http_server_init(struct http_server *server, uint16_t port, void (*func)(void));
void http_server_fiber_main(void);


#endif // _HTTP_SERVER__H_
