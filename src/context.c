#include "context.h"
#include <stdint.h>
#include <stdio.h>
#include "logger.h"

struct ribs_context main_ctx;
struct ribs_context *current_ctx = &main_ctx;

void ribs_makecontext(struct ribs_context *ctx, struct ribs_context *rctx, void (*func)(void)) {
    /* align stack to 16 bytes, assuming function always does push rbp to align
       __ribs_context_exit doesn't need to be aligned since it doesn't rely on stack alignment
       (needed when using SSE instructions)
    */
    void *sp = (unsigned long int *) ((((uintptr_t) ctx) & -16L) - 8);

    *(uintptr_t *)(sp) = (uintptr_t)&__ribs_context_exit;

    sp -= 8;
    *(uintptr_t *)(sp) = (uintptr_t)func;

    ctx->rsp = (uintptr_t) sp;
    ctx->rbx = (uintptr_t) rctx;
}

struct ribs_context *ribs_context_create(size_t stack_size, void (*func)(void)) {
    void *stack;
    stack = malloc(stack_size + sizeof(struct ribs_context));
    if (!stack)
	return LOGGER_PERROR("malloc stack"), NULL;
    stack += stack_size;
    struct ribs_context *ctx = (struct ribs_context *)stack;
    ribs_makecontext(ctx, current_ctx, func);
    return ctx;
}
