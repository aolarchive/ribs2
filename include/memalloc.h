/*
    This file is part of RIBS2.0 (Robust Infrastructure for Backend Systems).
    RIBS is an infrastructure for building great SaaS applications (but not
    limited to).

    Copyright (C) 2013,2014 Adap.tv, Inc.

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
#include "list.h"
#include <stdarg.h>
#include <time.h>

#define MEMALLOC_MEM_TYPE_ANY    0
#define MEMALLOC_MEM_TYPE_MALLOC 0
#define MEMALLOC_MEM_TYPE_BUF    1

struct memalloc_block {
    struct {
        unsigned long size:63;
        unsigned long type:1;
    } size;
    struct list _memblocks;
};

struct memalloc {
    void *mem;
    size_t capacity; /* capacity of the current block */
    size_t avail;
    struct list memblocks;
};

#define MEMALLOC_INITIALIZER { NULL, 0, 0, LIST_NULL_INITIALIZER }
#define MEMALLOC_INITIAL_BLOCK_SIZE 131072

size_t memalloc_usage(struct memalloc *ma);
size_t memalloc_alloc_raw(struct memalloc *ma, size_t size, void **memblock);
void memalloc_free_raw(void *mem);


_RIBS_INLINE_ void *memalloc_alloc(struct memalloc *ma, size_t size);
_RIBS_INLINE_ void memalloc_reset(struct memalloc *ma);
_RIBS_INLINE_ void memalloc_reset2(struct memalloc *ma, int type);
_RIBS_INLINE_ int memalloc_is_mine(struct memalloc *ma, const void *ptr);
_RIBS_INLINE_ char *memalloc_vsprintf(struct memalloc *ma, const char *format, va_list ap) __attribute__ ((format (gnu_printf, 2, 0)));
_RIBS_INLINE_ char *memalloc_sprintf(struct memalloc *ma, const char *format, ...) __attribute__ ((format (gnu_printf, 2, 3)));
_RIBS_INLINE_ int memalloc_strcat_vsprintf(struct memalloc *ma, char **buf, const char *format, va_list ap) __attribute__ ((format (gnu_printf, 3, 0)));
_RIBS_INLINE_ int memalloc_strcat_sprintf(struct memalloc *ma, char **buf, const char *format, ...) __attribute__ ((format (gnu_printf, 3, 4)));
_RIBS_INLINE_ void memalloc_str_remove_last_if(struct memalloc *ma, char c);
_RIBS_INLINE_ void *memalloc_memcpy(struct memalloc *ma, const void *s, size_t n);
_RIBS_INLINE_ char *memalloc_strcpy(struct memalloc *ma, const char *s);
_RIBS_INLINE_ char *memalloc_strftime(struct memalloc *ma, const char *format, const struct tm *tm) __attribute__ ((format (strftime, 2, 0)));

#include "../src/_memalloc.c"

#endif // _MEMALLOC__H_
