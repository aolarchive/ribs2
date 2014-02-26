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
#ifndef _FILE_WRITER__H_
#define _FILE_WRITER__H_

#include "ribs_defs.h"
#include "logger.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <sys/mman.h>
#include <string.h>
#include "ilog2.h"

#define FILE_WRITER_INITIALIZER { -1, NULL, 0, 0, 0, 0, 1024*1024 }
#define FILE_WRITER_INIT(var) (var) = ((struct file_writer)FILE_WRITER_INITIALIZER)

struct file_writer {
    int fd;
    char *mem;
    size_t base_loc;
    size_t next_loc;
    size_t write_loc;
    size_t capacity;
    size_t buffer_size;
};

_RIBS_INLINE_ void file_writer_make(struct file_writer *fw);
_RIBS_INLINE_ int file_writer_attachfd(struct file_writer *fw, int fd, size_t initial_size);
_RIBS_INLINE_ int file_writer_init(struct file_writer *fw, const char *filename);
_RIBS_INLINE_ size_t file_writer_wavail(struct file_writer *fw);
_RIBS_INLINE_ char *file_writer_wloc(struct file_writer *fw);
_RIBS_INLINE_ size_t file_writer_wlocpos(struct file_writer *fw);
_RIBS_INLINE_ int file_writer_wseek(struct file_writer *fw, size_t n);
_RIBS_INLINE_ int file_writer_lseek(struct file_writer *fw, off_t offset, int whence);
_RIBS_INLINE_ int file_writer_write(struct file_writer *fw, const void *buf, size_t size);
_RIBS_INLINE_ int file_writer_align(struct file_writer *fw);
_RIBS_INLINE_ int file_writer_close(struct file_writer *fw);

#include "../src/_file_writer.c"

#endif // _FILE_WRITER__H_
