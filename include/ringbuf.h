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
#ifndef _RINGBUF__H_
#define _RINGBUF__H_

#include "ribs_defs.h"

struct ringbuf {
    void *mem;
    size_t capacity;
    size_t read_loc;
    size_t write_loc;
};

#define RINGBUF_INITIALIZER { NULL, 0, 0, 0 }
#define RINGBUF_INIT(x) (x) = (struct ringbuf)RINGBUF_INITIALIZER

int ringbuf_init(struct ringbuf *rb, size_t size);
int ringbuf_free(struct ringbuf *rb);
_RIBS_INLINE_ void *ringbuf_mem(struct ringbuf *rb);
_RIBS_INLINE_ void *ringbuf_wloc(struct ringbuf *rb);
_RIBS_INLINE_ void *ringbuf_rloc(struct ringbuf *rb);
_RIBS_INLINE_ void ringbuf_wseek(struct ringbuf *rb, size_t by);
_RIBS_INLINE_ void ringbuf_rseek(struct ringbuf *rb, size_t by);
_RIBS_INLINE_ size_t ringbuf_size(struct ringbuf *rb);
_RIBS_INLINE_ size_t ringbuf_avail(struct ringbuf *rb);
_RIBS_INLINE_ int ringbuf_empty(struct ringbuf *rb);
_RIBS_INLINE_ void *ringbuf_pop(struct ringbuf *rb, size_t n);
_RIBS_INLINE_ void *ringbuf_push(struct ringbuf *rb, size_t n);
_RIBS_INLINE_ void *ringbuf_rolling_push(struct ringbuf *rb, size_t n);

#include "../src/_ringbuf.c"

#endif // _RINGBUF__H_
