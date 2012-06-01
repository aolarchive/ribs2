#ifndef _CONTEXT__H_
#define _CONTEXT__H_

#include "ribs_defs.h"
#include <sys/epoll.h>

struct ribs_context {
    long rbx; /* 0   */
    long rbp; /* 8   */
    long r12; /* 16  */
    long r13; /* 24  */
    long r14; /* 32  */
    long r15; /* 40  */
    long rip; /* 48  */
    long rsp; /* 56  */
    int fd;
    epoll_data_t data;
    struct ribs_context *next_free;
    char reserved[];
};

extern struct ribs_context main_ctx;
extern struct ribs_context *current_ctx;

extern int ribs_swapcurcontext(struct ribs_context *rctx);
extern int ribs_swapcontext(struct ribs_context *rctx, struct ribs_context *ctx);
extern int ribs_makecontext(struct ribs_context *ctx, struct ribs_context *rctx, void *sp, void (*func)(void), void (*user_cleanup_func)(void));
extern void __ribs_context_exit(void);

#endif // _CONTEXT__H_
