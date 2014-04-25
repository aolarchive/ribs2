/*
    This file is part of RIBS2.0 (Robust Infrastructure for Backend Systems).
    RIBS is an infrastructure for building great SaaS applications (but not
    limited to).

    Copyright (C) 2013 Adap.tv, Inc.

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
/*
 * extern functions for internal use
 */
int memalloc_new_block(struct memalloc *ma);
#include <stdio.h>
#include <string.h>
#include "mempool.h"

/*
 * inline functions
 */
_RIBS_INLINE_ int memalloc_resize_if_less(struct memalloc *ma, size_t size) {
    while (unlikely(ma->avail < size))
        if (0 > memalloc_new_block(ma))
            return -1;
    return 0;
}

_RIBS_INLINE_ void memalloc_seek(struct memalloc *ma, size_t size) {
    ma->mem += size;
    ma->avail -= size;
}

_RIBS_INLINE_ void *memalloc_alloc(struct memalloc *ma, size_t size) {
    if (0 > memalloc_resize_if_less(ma, size))
        return NULL;
    void *mem = ma->mem;
    memalloc_seek(ma, size);
    return mem;
}

_RIBS_INLINE_ void memalloc_reset(struct memalloc *ma) {
    if (0 < ma->capacity) {
        struct memalloc_block *cur_block = ma->blocks_head;
        size_t size = MEMALLOC_INITIAL_BLOCK_SIZE;
        for (;;size <<= 1) {
            void *mem = cur_block;
            cur_block = cur_block->next;
            mempool_free_chunk(mem, size);
            if (NULL == cur_block) break;
        }
        ma->avail = ma->capacity = 0;
    }
}

_RIBS_INLINE_ int memalloc_is_mine(struct memalloc *ma, const void *ptr) {
    if (0 < ma->capacity) {
        struct memalloc_block *cur_block = ma->blocks_head;
        size_t size = MEMALLOC_INITIAL_BLOCK_SIZE;
        for (;;size <<= 1) {
            void *mem = cur_block;
            if (ptr >= mem && ptr < mem + size)
                return 1;
            cur_block = cur_block->next;
            if (NULL == cur_block) break;
        }
    }
    return 0;
}

_RIBS_INLINE_ char *memalloc_vsprintf(struct memalloc *ma, const char *format, va_list ap) {
    if (0 == ma->avail && 0 > memalloc_new_block(ma))
        return NULL;
    int n;
    char *str;
    for (;;) {
        va_list apc;
        va_copy(apc, ap);
        str = ma->mem;
        n = vsnprintf(str, ma->avail, format, apc);
        va_end(apc);
        if (unlikely(0 > n))
            return NULL;
        if (likely((unsigned int)n < ma->avail))
            break;
        /* not enough space, alloc new block */
        if (unlikely(0 > memalloc_new_block(ma)))
            return NULL;
    }
    memalloc_seek(ma, n + 1);
    return str;
}

_RIBS_INLINE_ char *memalloc_sprintf(struct memalloc *ma, const char *format, ...) {
    va_list ap;
    va_start(ap, format);
    char *str = memalloc_vsprintf(ma, format, ap);
    va_end(ap);
    return str;
}

_RIBS_INLINE_ void *memalloc_memcpy(struct memalloc *ma, const void *s, size_t n) {
    if (0 > memalloc_resize_if_less(ma, n))
        return NULL;
    void *mem = ma->mem;
    memcpy(mem, s, n);
    memalloc_seek(ma, n);
    return mem;
}

_RIBS_INLINE_ char *memalloc_strcpy(struct memalloc *ma, const char *s) {
    if (0 == ma->avail && 0 > memalloc_new_block(ma))
        return NULL;
    char *mem;
    for (;;) {
        mem = ma->mem;
        char *dst = mem;
        const char *src = s;
        char *last = dst + ma->avail;
        /* copy until end of string or end of buf */
        for (; dst < last && *src; *dst++ = *src++);
        if (0 == *src && dst < last /* need one more for \0 */) {
            *dst = 0;
            memalloc_seek(ma, src - s + 1);
            break;
        }
        if (0 > memalloc_new_block(ma))
            return NULL;
    }
    return mem;
}

_RIBS_INLINE_ char *memalloc_strftime(struct memalloc *ma, const char *format, const struct tm *tm) {
    size_t n;
    char *mem;
    for (;;) {
        mem = ma->mem;
        n = strftime(mem, ma->avail, format, tm);
        if (n > 0)
            break;
        /* not enough space, alloc new block */
        if (0 > memalloc_new_block(ma))
            return NULL;
    }
    memalloc_seek(ma, n + 1);
    return mem;
}
