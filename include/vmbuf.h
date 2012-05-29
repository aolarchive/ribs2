/*
    This file is part of RIBS (Robust Infrastructure for Backend Systems).
    RIBS is an infrastructure for building great SaaS applications (but not
    limited to).

    Copyright (C) 2011 Adap.tv, Inc.

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
#ifndef _VM_BUF__H_
#define _VM_BUF__H_

#include "ribs_defs.h"
#include <sys/mman.h>
#include <stdarg.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#define CAT(X,Y) X##_##Y
#define TEMPLATE(X,Y) CAT(X,Y)

#define PAGEMASK 4095
#define PAGESIZE 4096

#ifndef VMBUF_INLINE
#define VMBUF_INLINE extern inline
#endif

struct vmbuf
{
    char *buf;
    size_t capacity;
    size_t read_loc;
    size_t write_loc;
};

#define VMBUF_INITIALIZER { NULL, 0, 0, 0 }

VMBUF_INLINE off_t vmbuf_align(off_t off);
VMBUF_INLINE int vmbuf_init(struct vmbuf *vmb, size_t initial_size);
VMBUF_INLINE void vmbuf_make(struct vmbuf *vmb);
VMBUF_INLINE void vmbuf_reset(struct vmbuf *vmb);
VMBUF_INLINE int vmbuf_free(struct vmbuf *vmb);
VMBUF_INLINE int vmbuf_free_most(struct vmbuf *vmb);

VMBUF_INLINE int vmbuf_resize_by(struct vmbuf *vmb, size_t by);
VMBUF_INLINE int vmbuf_resize_to(struct vmbuf *vmb, size_t new_capacity);
VMBUF_INLINE int vmbuf_resize_if_full(struct vmbuf *vmb);
VMBUF_INLINE int vmbuf_resize_if_less(struct vmbuf *vmb, size_t desired_size);
VMBUF_INLINE int vmbuf_resize_no_check(struct vmbuf *vmb, size_t n);

VMBUF_INLINE size_t vmbuf_alloc(struct vmbuf *vmb, size_t n);
VMBUF_INLINE size_t vmbuf_alloczero(struct vmbuf *vmb, size_t n);
VMBUF_INLINE size_t vmbuf_alloc_aligned(struct vmbuf *vmb, size_t n);

VMBUF_INLINE size_t vmbuf_num_elements(struct vmbuf *vmb, size_t size_of_element);

VMBUF_INLINE char *vmbuf_data(struct vmbuf *vmb);
VMBUF_INLINE char *vmbuf_data_ofs(struct vmbuf *vmb, size_t loc);

VMBUF_INLINE size_t vmbuf_wavail(struct vmbuf *vmb);
VMBUF_INLINE size_t vmbuf_ravail(struct vmbuf *vmb);

VMBUF_INLINE char *vmbuf_wloc(struct vmbuf *vmb);
VMBUF_INLINE char *vmbuf_rloc(struct vmbuf *vmb);

VMBUF_INLINE size_t vmbuf_rlocpos(struct vmbuf *vmb);
VMBUF_INLINE size_t vmbuf_wlocpos(struct vmbuf *vmb);

VMBUF_INLINE void vmbuf_rlocset(struct vmbuf *vmb, size_t ofs);
VMBUF_INLINE void vmbuf_wlocset(struct vmbuf *vmb, size_t ofs);

VMBUF_INLINE void vmbuf_rrewind(struct vmbuf *vmb, size_t by);
VMBUF_INLINE void vmbuf_wrewind(struct vmbuf *vmb, size_t by);

VMBUF_INLINE size_t vmbuf_capacity(struct vmbuf *vmb);

VMBUF_INLINE void vmbuf_unsafe_wseek(struct vmbuf *vmb, size_t by);
VMBUF_INLINE int vmbuf_wseek(struct vmbuf *vmb, size_t by);
VMBUF_INLINE void vmbuf_rseek(struct vmbuf *vmb, size_t by);

VMBUF_INLINE void vmbuf_rreset(struct vmbuf *vmb);
VMBUF_INLINE void vmbuf_wreset(struct vmbuf *vmb);

VMBUF_INLINE int vmbuf_sprintf(struct vmbuf *vmb, const char *format, ...);
VMBUF_INLINE int vmbuf_vsprintf(struct vmbuf *vmb, const char *format, va_list ap);
VMBUF_INLINE int vmbuf_strcpy(struct vmbuf *vmb, const char *src);

VMBUF_INLINE void vmbuf_remove_last_if(struct vmbuf *vmb, char c);

VMBUF_INLINE int vmbuf_read(struct vmbuf *vmb, int fd);
VMBUF_INLINE int vmbuf_write(struct vmbuf *vmb, int fd);

VMBUF_INLINE int vmbuf_memcpy(struct vmbuf *vmb, const void *src, size_t n);
VMBUF_INLINE void vmbuf_memset(struct vmbuf *vmb, int c, size_t n);

VMBUF_INLINE int vmbuf_strftime(struct vmbuf *vmb, const char *format, const struct tm *tm);

#include "../src/_vmbuf.c"

#endif // _VM_BUF__H_
