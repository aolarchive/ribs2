/*
    This file is part of RIBS2.0 (Robust Infrastructure for Backend Systems).
    RIBS is an infrastructure for building great SaaS applications (but not
    limited to).

    Copyright (C) 2012,2013,2014 Adap.tv, Inc.

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
#include <sys/mman.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#define VMBUF_INITIALIZER { NULL, NULL, 0, 0, 0 }
#define VMBUF_INIT(var) (var) = ((struct vmbuf)VMBUF_INITIALIZER)

struct vmbuf {
    void *mem;
    struct vmbuf_mem_funcs *funcs;
    int fd;
    size_t capacity;
    size_t rloc;
};

struct vmbuf_header {
    size_t wloc;
    size_t capacity;
};

int vmbuf_init(struct vmbuf *vmbuf, size_t initial_size);
int vmbuf_init_tmp(struct vmbuf *vmbuf, size_t initial_size);
int vmbuf_init_shared(struct vmbuf *vmbuf, size_t initial_size);
int vmbuf_init_shared_fixed(struct vmbuf *vmbuf, size_t max_size);
int vmbuf_free(struct vmbuf *vmbuf);
int vmbuf_add_capacity(struct vmbuf *vmbuf);
int vmbuf_resize_to(struct vmbuf *vmbuf, size_t size);
int vmbuf_sync(struct vmbuf *vmbuf);
static inline void vmbuf_reset(struct vmbuf *vmbuf);
static inline void vmbuf_wreset(struct vmbuf *vmbuf);
static inline void vmbuf_rreset(struct vmbuf *vmbuf);
static inline void *vmbuf_mem(struct vmbuf *vmbuf);
static inline void *vmbuf_mem2(struct vmbuf *vmbuf, size_t ofs);
static inline size_t vmbuf_wavail(struct vmbuf *vmbuf);
static inline size_t vmbuf_ravail(struct vmbuf *vmbuf);
static inline size_t vmbuf_capacity(struct vmbuf *vmbuf);
static inline void *vmbuf_wloc2(struct vmbuf *vmbuf);
static inline void *vmbuf_rloc2(struct vmbuf *vmbuf);
static inline int vmbuf_wseek(struct vmbuf *vmbuf, size_t by);
static inline void vmbuf_rseek(struct vmbuf *vmbuf, size_t by);
static inline int vmbuf_vsprintf(struct vmbuf *vmbuf, const char *format, va_list ap) __attribute__ ((format (gnu_printf, 2, 0)));
static inline int vmbuf_sprintf(struct vmbuf *vmbuf, const char *format, ...) __attribute__ ((format (gnu_printf, 2, 3)));
static inline int vmbuf_strftime(struct vmbuf *vmbuf, const char *format, const struct tm *tm) __attribute__ ((format (strftime, 2, 0)));
static inline int vmbuf_strcpy(struct vmbuf *vmbuf, const char *src);
static inline int vmbuf_replace_last_if(struct vmbuf *vmbuf, char s, char d);
static inline void vmbuf_remove_last_if(struct vmbuf *vmbuf, char c);
static inline int vmbuf_chrcpy(struct vmbuf *vmbuf, char c);
static inline int vmbuf_memcpy(struct vmbuf *vmbuf, const void *src, size_t n);
static inline size_t vmbuf_alloc(struct vmbuf *vmbuf, size_t n);
static inline size_t vmbuf_alloczero(struct vmbuf *vmbuf, size_t n);
static inline size_t vmbuf_alloc_aligned(struct vmbuf *vmbuf, size_t n);
static inline void *vmbuf_allocptr(struct vmbuf *vmbuf, size_t n);
static inline char *vmbuf_str(struct vmbuf *vmbuf);
static inline char *vmbuf_str2(struct vmbuf *vmbuf, size_t ofs);
static inline size_t vmbuf_wlocpos(struct vmbuf *vmbuf);
static inline size_t vmbuf_rlocpos(struct vmbuf *vmbuf);
static inline size_t vmbuf_size(struct vmbuf *vmbuf);
static inline size_t vmbuf_num_elements(struct vmbuf *vmbuf, size_t size_of_element);
static inline int vmbuf_read(struct vmbuf *vmbuf, int fd);
static inline int vmbuf_write(struct vmbuf *vmbuf, int fd);

static inline void vmbuf_wlocset(struct vmbuf *vmbuf, size_t ofs);
static inline void vmbuf_rlocset(struct vmbuf *vmbuf, size_t ofs);
static inline void vmbuf_wrewind(struct vmbuf *vmbuf, size_t by);
static inline void vmbuf_rrewind(struct vmbuf *vmbuf, size_t by);
static inline void vmbuf_swap(struct vmbuf *vmbuf1, struct vmbuf *vmbuf2);

static inline char *vmbuf_data(struct vmbuf *vmbuf);
static inline char *vmbuf_data_ofs(struct vmbuf *vmbuf, size_t ofs);
static inline char *vmbuf_wloc(struct vmbuf *vmbuf);
static inline char *vmbuf_rloc(struct vmbuf *vmbuf);

/* inline functions */
#include "../src/_vmbuf.c"

#endif // _VMBUF__H_
