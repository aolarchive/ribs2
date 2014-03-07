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
#ifndef _MEMALLOC__H_
#define _MEMALLOC__H_

#include "ribs_defs.h"
#include <stdarg.h>
#include <time.h>

struct memalloc_block {
    struct memalloc_block *next;
};

struct memalloc {
    void *mem;
    size_t capacity; /* capacity of the current block */
    size_t avail;
    struct memalloc_block *blocks_head;
    struct memalloc_block *blocks_tail;
};

#define MEMALLOC_INITIALIZER { NULL, 0, 0, NULL, NULL }
#define MEMALLOC_INITIAL_BLOCK_SIZE 131072

size_t memalloc_usage(struct memalloc *ma);

_RIBS_INLINE_ void *memalloc_alloc(struct memalloc *ma, size_t size);
_RIBS_INLINE_ void memalloc_reset(struct memalloc *ma);
_RIBS_INLINE_ int memalloc_is_mine(struct memalloc *ma, const void *ptr);
_RIBS_INLINE_ char *memalloc_vsprintf(struct memalloc *ma, const char *format, va_list ap) __attribute__ ((format (gnu_printf, 2, 0)));
_RIBS_INLINE_ char *memalloc_sprintf(struct memalloc *ma, const char *format, ...) __attribute__ ((format (gnu_printf, 2, 3)));
_RIBS_INLINE_ void *memalloc_memcpy(struct memalloc *ma, const void *s, size_t n);
_RIBS_INLINE_ char *memalloc_strcpy(struct memalloc *ma, const char *s);
_RIBS_INLINE_ char *memalloc_strftime(struct memalloc *ma, const char *format, const struct tm *tm) __attribute__ ((format (strftime, 2, 0)));

#include "../src/_memalloc.c"

#endif // _MEMALLOC__H_
