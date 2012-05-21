#ifndef _ACCEPTOR__H_
#define _ACCEPTOR__H_

#include <stdint.h>
#include "context.h"

struct acceptor
{
    struct ribs_context ctx;
    void (*ctx_func)(void);
    void *stack;
};

int acceptor_init(struct acceptor *acceptor, uint16_t port, void (*func)(void));

#endif // _ACCEPTOR__H_
