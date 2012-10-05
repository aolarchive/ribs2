#ifndef _OBJECT_POOL__H_
#define _OBJECT_POOL__H_

#include "ribs_defs.h"
#include "vmbuf.h"

#define OBJECT_POOL_INITIALIZER { 0, 0, 0, NULL, VMBUF_INITIALIZER }

struct object_pool {
    size_t object_size;
    size_t initial_size;
    size_t grow;
    void (*init_object)(void *mem);
    struct vmbuf pool;
};

_RIBS_INLINE_ int object_pool_grow(struct object_pool *op, size_t growby) {
    size_t i;
    printf("allocating %zu elements\n", growby);
    void *mem = calloc(growby, op->object_size);
    for (i = 0; i < growby; ++i, mem += op->object_size) {
        if (op->init_object)
            op->init_object(mem);
        *(uintptr_t *)vmbuf_wloc(&op->pool) = (uintptr_t)mem;
        vmbuf_wseek(&op->pool, sizeof(uintptr_t));
    }
    return 0;
}

_RIBS_INLINE_ void *object_pool_get(struct object_pool *op) {
    if (0 == vmbuf_wlocpos(&op->pool) && 0 > object_pool_grow(op, op->grow))
        return NULL;
    vmbuf_wrewind(&op->pool, sizeof(uintptr_t));
    return (void *)(*(uintptr_t *)vmbuf_wloc(&op->pool));
}

_RIBS_INLINE_ void object_pool_put(struct object_pool *op, void *mem) {
    *(uintptr_t *)vmbuf_wloc(&op->pool) = (uintptr_t)mem;
    vmbuf_unsafe_wseek(&op->pool, sizeof(uintptr_t));
    return ;
}

_RIBS_INLINE_ int object_pool_init(struct object_pool *op) {
    if (0 == op->object_size) return -1;
    if (0 == op->grow) op->grow = 1;
    if (0 > vmbuf_init(&op->pool, (op->initial_size ? op->initial_size : op->grow) * sizeof(void *)))
        return -1;
    return (0 < op->initial_size) ? object_pool_grow(op, op->initial_size) : 0;
}

#if 0 /* documentation only */
_RIBS_INLINE_ int object_pool_free(struct object_pool *op) {
    return -1; /* not supported, we never free */
}
#endif

#endif // _OBJECT_POOL__H_
