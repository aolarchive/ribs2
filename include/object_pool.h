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
#ifndef _OBJECT_POOL__H_
#define _OBJECT_POOL__H_

#include "ribs_defs.h"
#include "vmbuf.h"
#include "logger.h"

#define OBJECT_POOL_INITIALIZER { 0, 0, 0, NULL, VMBUF_INITIALIZER }
#define OBJECT_POOL_INIT(x) (x) = (struct object_pool)OBJECT_POOL_INITIALIZER

struct object_pool {
    size_t object_size;
    size_t initial_size;
    size_t grow;
    void (*init_object)(void *mem);
    struct vmbuf pool;
};

_RIBS_INLINE_ int object_pool_grow(struct object_pool *op, size_t growby) {
    size_t i;
    LOGGER_INFO("object_pool<%zu>: allocating %zu elements", op->object_size, growby);
    void *mem = calloc(growby, op->object_size);
    vmbuf_alloc(&op->pool, growby * sizeof(uintptr_t));
    for (i = 0; i < growby; ++i, mem += op->object_size) {
        if (op->init_object)
            op->init_object(mem);
        *(uintptr_t *)vmbuf_rloc(&op->pool) = (uintptr_t)mem;
        vmbuf_rseek(&op->pool, sizeof(uintptr_t));
    }
    return 0;
}

_RIBS_INLINE_ void *object_pool_get(struct object_pool *op) {
    if (0 == vmbuf_rlocpos(&op->pool) && 0 > object_pool_grow(op, op->grow))
        return NULL;
    vmbuf_rrewind(&op->pool, sizeof(uintptr_t));
    return (void *)(*(uintptr_t *)vmbuf_rloc(&op->pool));
}

_RIBS_INLINE_ void object_pool_put(struct object_pool *op, void *mem) {
    *(uintptr_t *)vmbuf_rloc(&op->pool) = (uintptr_t)mem;
    vmbuf_rseek(&op->pool, sizeof(uintptr_t));
    return ;
}

_RIBS_INLINE_ int object_pool_init(struct object_pool *op) {
    if (0 == op->object_size) return -1;
    if (0 == op->grow) op->grow = 1;
    if (0 > vmbuf_init(&op->pool, (op->initial_size ? op->initial_size : op->grow) * sizeof(void *)))
        return -1;
    return (0 < op->initial_size) ? object_pool_grow(op, op->initial_size) : 0;
}

_RIBS_INLINE_ void object_pool_noop() { }

#if 0 /* documentation only */
_RIBS_INLINE_ int object_pool_free(struct object_pool *op) {
    return -1; /* not supported, we never free */
}
#endif

#endif // _OBJECT_POOL__H_
