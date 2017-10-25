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
#ifndef _FILE_MAPPER__H_
#define _FILE_MAPPER__H_

#include "ribs_defs.h"

#define FILE_MAPPER_INITIALIZER { NULL, 0 }
#define FILE_MAPPER_INIT(var) (var) = ((struct file_mapper)FILE_MAPPER_INITIALIZER)

struct file_mapper {
    void *mem;
    size_t size;
};

int file_mapper_init(struct file_mapper *fm, const char *filename);
int file_mapper_init2(struct file_mapper *fm, const char *filename, size_t size, int flags, int mmap_prot, int mmap_flags);
int file_mapper_init_rw(struct file_mapper *fm, const char *filename, size_t size);
int file_mapper_init_with_fd_r(struct file_mapper *fm, int fd);
int file_mapper_init_with_fd_rw(struct file_mapper *fm, int fd);
int file_mapper_free(struct file_mapper *fm);

_RIBS_INLINE_ void *file_mapper_data(struct file_mapper *fm) {
    return fm->mem;
}

_RIBS_INLINE_ size_t file_mapper_size(struct file_mapper *fm) {
    return fm->size;
}

#endif // _FILE_MAPPER__H_
