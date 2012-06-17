#include "context.h"
#include <stdint.h>

struct ribs_context main_ctx;
struct ribs_context *current_ctx = &main_ctx;

int ribs_makecontext(struct ribs_context *ctx, struct ribs_context *rctx, void *sp, void (*func)(void)) {
    /* align stack to 16 bytes, assuming function always does push rbp to align
       __ribs_context_exit doesn't need to be aligned since it doesn't rely on stack alignment
       (needed when using SSE instructions)
    */
    sp = (unsigned long int *) ((((uintptr_t) sp) & -16L) - 8);

    *(uintptr_t *)(sp) = (uintptr_t)&__ribs_context_exit;

    sp -= 8;
    *(uintptr_t *)(sp) = (uintptr_t)func;

    ctx->rsp = (uintptr_t) sp;
    ctx->rbx = (uintptr_t) rctx;

    return 0;
}

struct ribs_context *ribs_context_create(size_t stack_size, void (*func)(void)) {
    void *stack;
    stack = malloc(stack_size + sizeof(struct ribs_context));
    stack += stack_size;
    struct ribs_context *ctx = (struct ribs_context *)stack;
    ribs_makecontext(ctx, current_ctx, ctx, func);
    return ctx;
}
