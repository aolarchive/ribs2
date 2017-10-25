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
#ifndef _VMALLOCATOR__H_
#define _VMALLOCATOR__H_

#include "ribs_defs.h"
#include <string.h>

#define VM_ALLOC_MIN_SIZE 4096
#define VM_OFS_ALIGN_BYTES 8

#define VMALLOCATOR_INITIALIZER { NULL, 0, 0, NULL, -1 }

#define VM_ALLOC_ALIGN(x,s) (((x)+((size_t)(s))-1)&~(((size_t)(s))-1))

struct vmallocator {
    void *mem;
    size_t wlocpos;
    size_t capacity;
    int (*resize_func)(struct vmallocator *v, size_t new_size);
    int fd;
};

int vmallocator_open(struct vmallocator *v, const char *filename, int flags);
int vmallocator_close(struct vmallocator *v);
int vmallocator_init(struct vmallocator *v);
void vmallocator_free(struct vmallocator *v);

static inline void vmallocator_reset(struct vmallocator *v);
static inline size_t vmallocator_wlocpos(struct vmallocator *v);
static inline size_t vmallocator_avail(struct vmallocator *v);
static inline int vmallocator_check_resize(struct vmallocator *v, size_t s);
static inline size_t vmallocator_alloc(struct vmallocator *v, size_t s);
static inline size_t vmallocator_alloczero(struct vmallocator *v, size_t s);
static inline size_t vmallocator_alloczero_aligned(struct vmallocator *v, size_t s);
static inline size_t vmallocator_alloc_aligned(struct vmallocator *v, size_t s);
static inline void *vmallocator_allocptr(struct vmallocator *v, size_t s);
static inline void *vmallocator_allocptr_aligned(struct vmallocator *v, size_t s);
static inline void *vmallocator_ofs2mem(struct vmallocator *v, size_t ofs);

#include "../src/_vmallocator.c"

#endif // _VMALLOCATOR__H_
