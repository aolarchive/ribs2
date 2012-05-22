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

#define _GNU_SOURCE
#include <sys/mman.h>
#include <stdarg.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#define CAT(X,Y) X##_##Y
#define TEMPLATE(X,Y) CAT(X,Y)

#define PAGEMASK 4095
#define PAGESIZE 4096

struct vmbuf
{
    char *buf;
    size_t capacity;
    size_t read_loc;
    size_t write_loc;
};

inline off_t vmbuf_align(off_t off);
inline int vmbuf_init(struct vmbuf *vmb, size_t initial_size);
inline void vmbuf_make(struct vmbuf *vmb);
inline void vmbuf_reset(struct vmbuf *vmb);
inline int vmbuf_free(struct vmbuf *vmb);
inline int vmbuf_free_most(struct vmbuf *vmb);

inline int vmbuf_resize_by(struct vmbuf *vmb, size_t by);
inline int vmbuf_resize_to(struct vmbuf *vmb, size_t new_capacity);
inline int vmbuf_resize_if_full(struct vmbuf *vmb);
inline int vmbuf_resize_if_less(struct vmbuf *vmb, size_t desired_size);
inline int vmbuf_resize_no_check(struct vmbuf *vmb, size_t n);

inline size_t vmbuf_alloc(struct vmbuf *vmb, size_t n);
inline size_t vmbuf_alloczero(struct vmbuf *vmb, size_t n);
inline size_t vmbuf_alloc_aligned(struct vmbuf *vmb, size_t n);

inline size_t vmbuf_num_elements(struct vmbuf *vmb, size_t size_of_element);

inline char *vmbuf_data(struct vmbuf *vmb);
inline char *vmbuf_data_ofs(struct vmbuf *vmb, size_t loc);

inline size_t vmbuf_wavail(struct vmbuf *vmb);
inline size_t vmbuf_ravail(struct vmbuf *vmb);

inline char *vmbuf_wloc(struct vmbuf *vmb);
inline char *vmbuf_rloc(struct vmbuf *vmb);

inline size_t vmbuf_rlocpos(struct vmbuf *vmb);
inline size_t vmbuf_wlocpos(struct vmbuf *vmb);

inline void vmbuf_rlocset(struct vmbuf *vmb, size_t ofs);
inline void vmbuf_wlocset(struct vmbuf *vmb, size_t ofs);

inline void vmbuf_rrewind(struct vmbuf *vmb, size_t by);
inline void vmbuf_wrewind(struct vmbuf *vmb, size_t by);

inline size_t vmbuf_capacity(struct vmbuf *vmb);

inline void vmbuf_unsafe_wseek(struct vmbuf *vmb, size_t by);
inline int vmbuf_wseek(struct vmbuf *vmb, size_t by);
inline void vmbuf_rseek(struct vmbuf *vmb, size_t by);

inline void vmbuf_rreset(struct vmbuf *vmb);
inline void vmbuf_wreset(struct vmbuf *vmb);

inline int vmbuf_sprintf(struct vmbuf *vmb, const char *format, ...);
inline int vmbuf_vsprintf(struct vmbuf *vmb, const char *format, va_list ap);
inline int vmbuf_strcpy(struct vmbuf *vmb, const char *src);

inline void vmbuf_remove_last_if(struct vmbuf *vmb, char c);

inline int vmbuf_read(struct vmbuf *vmb, int fd);
inline int vmbuf_write(struct vmbuf *vmb, int fd);

inline int vmbuf_memcpy(struct vmbuf *vmb, const void *src, size_t n);
inline void vmbuf_memset(struct vmbuf *vmb, int c, size_t n);

inline int vmbuf_strftime(struct vmbuf *vmb, const char *format, const struct tm *tm);

#endif // _VM_BUF__H_
