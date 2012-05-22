#include "context.h"
#include <stdint.h>

struct ribs_context main_ctx;

int ribs_makecontext(struct ribs_context *ctx, struct ribs_context *rctx, void *sp, void (*func)(void), void (*user_cleanup_func)(void)) {
   sp = (unsigned long int *) ((((uintptr_t) sp) & -16L) - 8);

   ctx->rbx = (uintptr_t) rctx;
   ctx->rip = (uintptr_t) func;

   *(uintptr_t *)(sp) = (uintptr_t)&__ribs_context_exit;
   if (user_cleanup_func) {
      sp -= 8;
      *(uintptr_t *)(sp) = (uintptr_t)user_cleanup_func;
   }
   ctx->rsp = (uintptr_t) sp;

   return 0;
}
