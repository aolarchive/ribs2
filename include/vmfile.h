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
#ifndef _VMFILE__H_
#define _VMFILE__H_

#include "ribs_defs.h"
#include "template.h"
#include <sys/mman.h>
#include <stdarg.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "vm_misc.h"
#ifdef VMBUF_T
#undef VMBUF_T
#endif
#define VMBUF_T vmfile

struct vmfile {
    char *buf;
    size_t capacity;
    size_t read_loc;
    size_t write_loc;
    int fd;
};

#define VMFILE_INITIALIZER { NULL, 0, 0, 0, -1 }
#define VMFILE_INIT(var) (var) = ((struct vmfile)VMFILE_INITIALIZER)

/* vmfile */
_RIBS_INLINE_ int vmfile_attachfd(struct vmfile *vmb, int fd, size_t initial_size);
_RIBS_INLINE_ int vmfile_init(struct vmfile *vmb, const char *filename, size_t initial_size);
_RIBS_INLINE_ int vmfile_resize_to(struct vmfile *vmb, size_t new_capacity);
_RIBS_INLINE_ int vmfile_close(struct vmfile *vmb);
_RIBS_INLINE_ void vmfile_make(struct vmfile *vmb);
_RIBS_INLINE_ void vmfile_reset(struct vmfile *vmb);
_RIBS_INLINE_ int vmfile_free(struct vmfile *vmb);
_RIBS_INLINE_ int vmfile_free_most(struct vmfile *vmb);
_RIBS_INLINE_ int vmfile_resize_by(struct vmfile *vmb, size_t by);
_RIBS_INLINE_ int vmfile_resize_if_full(struct vmfile *vmb);
_RIBS_INLINE_ int vmfile_resize_if_less(struct vmfile *vmb, size_t desired_size);
_RIBS_INLINE_ int vmfile_resize_no_check(struct vmfile *vmb, size_t n);
_RIBS_INLINE_ size_t vmfile_alloc(struct vmfile *vmb, size_t n);
_RIBS_INLINE_ size_t vmfile_alloczero(struct vmfile *vmb, size_t n);
_RIBS_INLINE_ size_t vmfile_alloc_aligned(struct vmfile *vmb, size_t n);
_RIBS_INLINE_ size_t vmfile_num_elements(struct vmfile *vmb, size_t size_of_element);
_RIBS_INLINE_ char *vmfile_data(struct vmfile *vmb);
_RIBS_INLINE_ char *vmfile_data_ofs(struct vmfile *vmb, size_t loc);
_RIBS_INLINE_ size_t vmfile_wavail(struct vmfile *vmb);
_RIBS_INLINE_ size_t vmfile_ravail(struct vmfile *vmb);
_RIBS_INLINE_ char *vmfile_wloc(struct vmfile *vmb);
_RIBS_INLINE_ char *vmfile_rloc(struct vmfile *vmb);
_RIBS_INLINE_ size_t vmfile_rlocpos(struct vmfile *vmb);
_RIBS_INLINE_ size_t vmfile_wlocpos(struct vmfile *vmb);
_RIBS_INLINE_ void vmfile_rlocset(struct vmfile *vmb, size_t ofs);
_RIBS_INLINE_ void vmfile_wlocset(struct vmfile *vmb, size_t ofs);
_RIBS_INLINE_ void vmfile_rrewind(struct vmfile *vmb, size_t by);
_RIBS_INLINE_ void vmfile_wrewind(struct vmfile *vmb, size_t by);
_RIBS_INLINE_ size_t vmfile_capacity(struct vmfile *vmb);
_RIBS_INLINE_ void vmfile_unsafe_wseek(struct vmfile *vmb, size_t by);
_RIBS_INLINE_ int vmfile_wseek(struct vmfile *vmb, size_t by);
_RIBS_INLINE_ void vmfile_rseek(struct vmfile *vmb, size_t by);
_RIBS_INLINE_ void vmfile_rreset(struct vmfile *vmb);
_RIBS_INLINE_ void vmfile_wreset(struct vmfile *vmb);
_RIBS_INLINE_ int vmfile_sprintf(struct vmfile *vmb, const char *format, ...) __attribute__ ((format (gnu_printf, 2, 3)));
_RIBS_INLINE_ int vmfile_vsprintf(struct vmfile *vmb, const char *format, va_list ap) __attribute__ ((format (gnu_printf, 2, 0)));
_RIBS_INLINE_ int vmfile_strcpy(struct vmfile *vmb, const char *src);
_RIBS_INLINE_ void vmfile_remove_last_if(struct vmfile *vmb, char c);
_RIBS_INLINE_ int vmfile_read(struct vmfile *vmb, int fd);
_RIBS_INLINE_ int vmfile_write(struct vmfile *vmb, int fd);
_RIBS_INLINE_ int vmfile_memcpy(struct vmfile *vmb, const void *src, size_t n);
_RIBS_INLINE_ void vmfile_memset(struct vmfile *vmb, int c, size_t n);
_RIBS_INLINE_ int vmfile_strftime(struct vmfile *vmb, const char *format, const struct tm *tm) __attribute__ ((format (strftime, 2, 0)));
_RIBS_INLINE_ void *vmfile_allocptr(struct vmfile *vmb, size_t n);
_RIBS_INLINE_ int vmfile_chrcpy(struct vmfile *vmb, char c);
_RIBS_INLINE_ void vmfile_swap(struct vmfile *vmb1, struct vmfile *vmb2);

#include "../src/_vmfile.c"
#include "../src/_vmbuf_impl.c"

#endif // _VMFILE__H_
