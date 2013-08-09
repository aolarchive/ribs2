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
#ifndef _VMBUF__H_
#define _VMBUF__H_

#include "ribs_defs.h"
#include "template.h"
#include <sys/mman.h>
#include <stdarg.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include "vm_misc.h"
#ifdef VMBUF_T
#undef VMBUF_T
#endif
#define VMBUF_T vmbuf

struct vmbuf {
    char *buf;
    size_t capacity;
    size_t read_loc;
    size_t write_loc;
};

#define VMBUF_INITIALIZER { NULL, 0, 0, 0 }
#define VMBUF_INIT(var) (var) = ((struct vmbuf)VMBUF_INITIALIZER)

/* vmbuf */
_RIBS_INLINE_ int vmbuf_init(struct vmbuf *vmb, size_t initial_size);
_RIBS_INLINE_ int vmbuf_resize_to(struct vmbuf *vmb, size_t new_capacity);
_RIBS_INLINE_ void vmbuf_make(struct vmbuf *vmb);
_RIBS_INLINE_ void vmbuf_reset(struct vmbuf *vmb);
_RIBS_INLINE_ int vmbuf_free(struct vmbuf *vmb);
_RIBS_INLINE_ int vmbuf_free_most(struct vmbuf *vmb);
_RIBS_INLINE_ int vmbuf_resize_by(struct vmbuf *vmb, size_t by);
_RIBS_INLINE_ int vmbuf_resize_if_full(struct vmbuf *vmb);
_RIBS_INLINE_ int vmbuf_resize_if_less(struct vmbuf *vmb, size_t desired_size);
_RIBS_INLINE_ int vmbuf_resize_no_check(struct vmbuf *vmb, size_t n);
_RIBS_INLINE_ size_t vmbuf_alloc(struct vmbuf *vmb, size_t n);
_RIBS_INLINE_ size_t vmbuf_alloczero(struct vmbuf *vmb, size_t n);
_RIBS_INLINE_ size_t vmbuf_alloc_aligned(struct vmbuf *vmb, size_t n);
_RIBS_INLINE_ size_t vmbuf_num_elements(struct vmbuf *vmb, size_t size_of_element);
_RIBS_INLINE_ char *vmbuf_data(struct vmbuf *vmb);
_RIBS_INLINE_ char *vmbuf_data_ofs(struct vmbuf *vmb, size_t loc);
_RIBS_INLINE_ size_t vmbuf_wavail(struct vmbuf *vmb);
_RIBS_INLINE_ size_t vmbuf_ravail(struct vmbuf *vmb);
_RIBS_INLINE_ char *vmbuf_wloc(struct vmbuf *vmb);
_RIBS_INLINE_ char *vmbuf_rloc(struct vmbuf *vmb);
_RIBS_INLINE_ size_t vmbuf_rlocpos(struct vmbuf *vmb);
_RIBS_INLINE_ size_t vmbuf_wlocpos(struct vmbuf *vmb);
_RIBS_INLINE_ void vmbuf_rlocset(struct vmbuf *vmb, size_t ofs);
_RIBS_INLINE_ void vmbuf_wlocset(struct vmbuf *vmb, size_t ofs);
_RIBS_INLINE_ void vmbuf_rrewind(struct vmbuf *vmb, size_t by);
_RIBS_INLINE_ void vmbuf_wrewind(struct vmbuf *vmb, size_t by);
_RIBS_INLINE_ size_t vmbuf_capacity(struct vmbuf *vmb);
_RIBS_INLINE_ void vmbuf_unsafe_wseek(struct vmbuf *vmb, size_t by);
_RIBS_INLINE_ int vmbuf_wseek(struct vmbuf *vmb, size_t by);
_RIBS_INLINE_ void vmbuf_rseek(struct vmbuf *vmb, size_t by);
_RIBS_INLINE_ void vmbuf_rreset(struct vmbuf *vmb);
_RIBS_INLINE_ void vmbuf_wreset(struct vmbuf *vmb);
_RIBS_INLINE_ int vmbuf_sprintf(struct vmbuf *vmb, const char *format, ...) __attribute__ ((format (gnu_printf, 2, 3)));
_RIBS_INLINE_ int vmbuf_vsprintf(struct vmbuf *vmb, const char *format, va_list ap) __attribute__ ((format (gnu_printf, 2, 0)));
_RIBS_INLINE_ int vmbuf_strcpy(struct vmbuf *vmb, const char *src);
_RIBS_INLINE_ void vmbuf_remove_last_if(struct vmbuf *vmb, char c);
_RIBS_INLINE_ int vmbuf_read(struct vmbuf *vmb, int fd);
_RIBS_INLINE_ int vmbuf_write(struct vmbuf *vmb, int fd);
_RIBS_INLINE_ int vmbuf_memcpy(struct vmbuf *vmb, const void *src, size_t n);
_RIBS_INLINE_ void vmbuf_memset(struct vmbuf *vmb, int c, size_t n);
_RIBS_INLINE_ int vmbuf_strftime(struct vmbuf *vmb, const char *format, const struct tm *tm) __attribute__ ((format (strftime, 2, 0)));
_RIBS_INLINE_ void *vmbuf_allocptr(struct vmbuf *vmb, size_t n);
_RIBS_INLINE_ int vmbuf_chrcpy(struct vmbuf *vmb, char c);
_RIBS_INLINE_ void vmbuf_swap(struct vmbuf *vmb1, struct vmbuf *vmb2);

#include "../src/_vmbuf.c"
#include "../src/_vmbuf_impl.c"

#endif // _VMBUF__H_
