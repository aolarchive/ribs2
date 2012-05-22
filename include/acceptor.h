#ifndef _ACCEPTOR__H_
#define _ACCEPTOR__H_

#include <stdint.h>
#include "context.h"
#include "ctx_pool.h"

struct acceptor
{
    struct ribs_context ctx;
    void *stack;
    struct ctx_pool ctx_pool;
    void (*ctx_start_func)(void);
};

int acceptor_init(struct acceptor *acceptor, uint16_t port, void (*func)(void));

#endif // _ACCEPTOR__H_
