#ifndef _CTX_POOL__H_
#define _CTX_POOL__H_

#include "ribs_defs.h"
#include "context.h"

struct ctx_pool {
    size_t grow_by;
    size_t stack_size;
    size_t reserved_size;
    struct ribs_context *freelist;
};

int ctx_pool_init(struct ctx_pool *cp, size_t initial_size, size_t grow_by, size_t stack_size, size_t reserved_size);
int ctx_pool_createstacks(struct ctx_pool *cp, size_t num_stacks, size_t stack_size, size_t reserved_size);
_RIBS_INLINE_ struct ribs_context *ctx_pool_get(struct ctx_pool *cp);
_RIBS_INLINE_ void ctx_pool_put(struct ctx_pool *cp, struct ribs_context *ctx);

#include "../src/_ctx_pool.c"

#endif // _CTX_POOL__H_
