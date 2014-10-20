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
#ifndef _RINGFILE__H_
#define _RINGFILE__H_

#include "ribs_defs.h"

struct ringfile {
    void *rbuf;
    void *mem;
};

struct ringfile_header {
    size_t capacity;
    size_t read_loc;
    size_t write_loc;
    size_t reserved_size;
    char reserved[];
};

#define RINGFILE_INITIALIZER { NULL, NULL }
#define RINGFILE_INIT(x) (x) = (struct ringfile)RINGFILE_INITIALIZER

int ringfile_init(struct ringfile *rb, const char *filename, size_t size, size_t reserved);
int ringfile_init_with_resize(struct ringfile *rb, const char *filename, size_t size, size_t reserved, void (*evict_one_rec)(struct ringfile *));
int ringfile_free(struct ringfile *rb);
int ringfile_sync(struct ringfile *rb);
_RIBS_INLINE_ void *ringfile_data(struct ringfile *rb);
_RIBS_INLINE_ void *ringfile_get_reserved(struct ringfile *rb);
_RIBS_INLINE_ void *ringfile_wloc(struct ringfile *rb);
_RIBS_INLINE_ void *ringfile_rloc(struct ringfile *rb);
_RIBS_INLINE_ size_t ringfile_rlocpos(struct ringfile *rb);
_RIBS_INLINE_ size_t ringfile_wlocpos(struct ringfile *rb);
_RIBS_INLINE_ void ringfile_wseek(struct ringfile *rb, size_t by);
_RIBS_INLINE_ void ringfile_rseek(struct ringfile *rb, size_t by);
_RIBS_INLINE_ size_t ringfile_size(struct ringfile *rb);
_RIBS_INLINE_ size_t ringfile_avail(struct ringfile *rb);
_RIBS_INLINE_ size_t ringfile_capacity(struct ringfile *rb);
_RIBS_INLINE_ int ringfile_empty(struct ringfile *rb);
_RIBS_INLINE_ void *ringfile_pop(struct ringfile *rb, size_t n);
_RIBS_INLINE_ void *ringfile_push(struct ringfile *rb, size_t n);
_RIBS_INLINE_ void *ringfile_rolling_push(struct ringfile *rb, size_t n);

#include "../src/_ringfile.c"

#endif // _RINGFILE__H_
