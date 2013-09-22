/*
    This file is part of RIBS2.0 (Robust Infrastructure for Backend Systems).
    RIBS is an infrastructure for building great SaaS applications (but not
    limited to).

    Copyright (C) 2012,2013 Adap.tv, Inc.

    RIBS is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, version 2.1 of the License.

    RIBS is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with RIBS.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "context.h"
#include <stdint.h>
#include <stdio.h>
#include "logger.h"

extern void __ribs_context_jump(void);

void ribs_makecontext(struct ribs_context *ctx, struct ribs_context *pctx, void (*func)(void)) {

#if defined(__i386__) || defined(__x86_64__)
    /* align stack to 16 bytes, assuming function always does push rbp to align.
       func doesn't need to be aligned since it doesn't rely on stack alignment
       (x86_64: needed when using SSE instructions)
    */

    void *sp = (unsigned long int *) ((((uintptr_t) ctx) & -16L) -sizeof(uintptr_t));
    *(uintptr_t *)(sp) = (uintptr_t) __ribs_context_jump;
    sp -= sizeof(uintptr_t);
    *(uintptr_t *)(sp) = (uintptr_t) func;
#else
    /* arm: align stack to 8 bytes
    */

    void *sp = (unsigned long int *) (((uintptr_t) ctx) & -8L);
    ctx->linked_func_reg = (uintptr_t) __ribs_context_jump;
    ctx->first_func_reg = (uintptr_t) func;
#endif

    ctx->stack_pointer_reg = (uintptr_t) sp;
    ctx->parent_context_reg = (uintptr_t) pctx;
}

struct ribs_context *ribs_context_create(size_t stack_size, size_t reserved_size, void (*func)(void)) {
    void *stack;
    stack = calloc(1, stack_size + sizeof(struct ribs_context) + reserved_size);
    if (!stack)
        return LOGGER_PERROR("calloc stack"), NULL;
    stack += stack_size;
    struct ribs_context *ctx = (struct ribs_context *)stack;
    ribs_makecontext(ctx, current_ctx, func);
    return ctx;
}

void __ribs_context_cleanup(void) {
    if (0 == current_ctx->ribify_memalloc_refcount)
        memalloc_reset(&current_ctx->memalloc);
    else {
        LOGGER_ERROR("misbehaving ribified library didn't free() %u time(s)", current_ctx->ribify_memalloc_refcount);
        current_ctx->ribify_memalloc_refcount = 0;
        current_ctx->memalloc = (struct memalloc)MEMALLOC_INITIALIZER;
    }
}
