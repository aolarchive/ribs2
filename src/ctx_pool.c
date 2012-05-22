#define _GNU_SOURCE
#include "ctx_pool.h"
#include <sys/mman.h>
#include <stdio.h>

extern struct ribs_context *current_ctx;

/*
  when defined, it will increase dramatically the number of maps
  enable when debugging memory corruptions
*/
/* #define STACK_PROTECTION */

int ctx_pool_init(struct ctx_pool *cp, size_t initial_size, size_t grow_by, size_t stack_size, size_t reserved_size) {
    stack_size += 4095ULL;
    stack_size &= ~4095ULL;
    cp->grow_by = grow_by;
    cp->stack_size = stack_size;
    cp->reserved_size = reserved_size;
    cp->freelist = NULL;
    return ctx_pool_createstacks(cp, initial_size, stack_size, reserved_size);
}

int ctx_pool_createstacks(struct ctx_pool *cp, size_t num_stacks, size_t stack_size, size_t reserved_size) {
#ifdef STACK_PROTECTION
   stack_size += 4096; // one more page as stack guard page
#endif
    void *mem = mmap(NULL, num_stacks * stack_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (MAP_FAILED == mem)
        return perror("mmap, ctx_pool_init"), -1;
    size_t rc_ofs = stack_size - sizeof(struct ribs_context) - reserved_size;
    size_t i;
    for (i = 0; i < num_stacks; ++i, mem += stack_size) {
#ifdef STACK_PROTECTION
        if (MAP_FAILED == mmap(mem, 4096, PROT_NONE, MAP_FIXED | MAP_ANONYMOUS | MAP_PRIVATE, -1, 0))
            return perror("mmap, ctx_pool_init, PROT_NONE"), -1;
#endif
        struct ribs_context *rc = (struct ribs_context *)(mem + rc_ofs);
        rc->next_free = cp->freelist;
        cp->freelist = rc;
    }
    return 0;
}

inline struct ribs_context *ctx_pool_get(struct ctx_pool *cp) {
   if (NULL == cp->freelist && 0 != ctx_pool_createstacks(cp, cp->grow_by, cp->stack_size, cp->reserved_size))
      return NULL;
   struct ribs_context *ctx = cp->freelist;
   cp->freelist = ctx->next_free;
   return ctx;
}

inline void ctx_pool_put(struct ctx_pool *cp, struct ribs_context *ctx) {
   ctx->next_free = cp->freelist;
   cp->freelist = ctx;
}

void ctx_pool_cleanup_func(void) {
   struct ctx_pool *cp = (struct ctx_pool *)current_ctx->data.ptr;
   ctx_pool_put(cp, current_ctx);
}
