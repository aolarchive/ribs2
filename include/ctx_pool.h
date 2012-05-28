#ifndef _CTX_POOL__H_
#define _CTX_POOL__H_

#include "ribs_defs.h"
#include "context.h"

#ifndef CTX_POOL_INLINE
#define CTX_POOL_INLINE extern inline
#endif

struct ctx_pool {
    size_t grow_by;
    size_t stack_size;
    size_t reserved_size;
    struct ribs_context *freelist;
};

int ctx_pool_init(struct ctx_pool *cp, size_t initial_size, size_t grow_by, size_t stack_size, size_t reserved_size);
int ctx_pool_createstacks(struct ctx_pool *cp, size_t num_stacks, size_t stack_size, size_t reserved_size);
CTX_POOL_INLINE struct ribs_context *ctx_pool_get(struct ctx_pool *cp);
CTX_POOL_INLINE void ctx_pool_put(struct ctx_pool *cp, struct ribs_context *ctx);

#include "../src/_ctx_pool.c"


#endif // _CTX_POOL__H_
