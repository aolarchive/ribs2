/*
    This file is part of RIBS2.0 (Robust Infrastructure for Backend Systems).
    RIBS is an infrastructure for building great SaaS applications (but not
    limited to).

    Copyright (C) 2012 Adap.tv, Inc.

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

#include "_vmbuf_preamble.h"
#include "_vmbuf.h"

#define VMBUF_INITIALIZER { NULL, 0, 0, 0 }

_RIBS_INLINE_ int TEMPLATE(VMBUF_T,init)(struct VMBUF_T *vmb, size_t initial_size) {
    if (NULL == vmb->buf) {
        initial_size = vmbuf_align(initial_size);
        vmb->buf = (char *)mmap(NULL, initial_size, PROT_WRITE | PROT_READ, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
        if (MAP_FAILED == vmb->buf) {
            perror("mmap, vmbuf_init");
            vmb->buf = NULL;
            return -1;
        }
        vmb->capacity = initial_size;
    } else if (vmb->capacity < initial_size)
        TEMPLATE(VMBUF_T,resize_to)(vmb, initial_size);
    TEMPLATE(VMBUF_T,reset)(vmb);
    return 0;
}

_RIBS_INLINE_ int TEMPLATE(VMBUF_T,resize_to)(struct VMBUF_T *vmb, size_t new_capacity) {
    new_capacity = TEMPLATE(VMBUF_T,align)(new_capacity);
    char *newaddr = (char *)mremap(vmb->buf, vmb->capacity, new_capacity, MREMAP_MAYMOVE);
    if ((void *)-1 == newaddr)
        return perror("mremap, " STRINGIFY(VMBUF_T) "_resize_to"), -1;
    // success
    vmb->buf = newaddr;
    vmb->capacity = new_capacity;
    return 0;
}

#endif // _VMBUF__H_
