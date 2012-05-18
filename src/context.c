#include "context.h"
#include <stdint.h>

int ribs_makecontext(struct ribs_context *ctx, struct ribs_context *rctx, void *stack, size_t stack_size, void (*func)(void)) {
   stack = (unsigned long int *) ((uintptr_t) stack + stack_size);
   stack = (unsigned long int *) ((((uintptr_t) stack) & -16L) - 8);

   ctx->rbx = (uintptr_t) rctx;
   ctx->rip = (uintptr_t) func;
   ctx->rsp = (uintptr_t) stack;

   *(uintptr_t *)stack = (uintptr_t)&__ribs_context_exit;

   return 0;
}
