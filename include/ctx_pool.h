/*
    This file is part of RIBS2.0 (Robust Infrastructure for Backend Systems).
    RIBS is an infrastructure for building great SaaS applications (but not
    limited to).

    Copyright (C) 2012 Adap.tv, Inc.

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
