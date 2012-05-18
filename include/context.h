#ifndef _CONTEXT__H_
#define _CONTEXT__H_

#include <stddef.h>

struct ribs_context {
    long rbx; /* 0   */
    long rbp; /* 8   */
    long r12; /* 16  */
    long r13; /* 24  */
    long r14; /* 32  */
    long r15; /* 40  */
    long rip; /* 48  */
    long rsp; /* 56  */
};

extern int ribs_swapcontext(struct ribs_context *rctx, struct ribs_context *ctx);
extern int ribs_makecontext(struct ribs_context *ctx, struct ribs_context *rctx, void *stack, size_t stack_size, void (*func)(void));
extern void __ribs_context_exit(void);

#endif // _CONTEXT__H_
