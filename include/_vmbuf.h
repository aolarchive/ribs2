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
#ifndef VMBUF_T
#define VMBUF_T vmbuf
#endif

#ifndef VMBUF_T_EXTRA
#define VMBUF_T_EXTRA
#endif

struct VMBUF_T {
    char *buf;
    size_t capacity;
    size_t read_loc;
    size_t write_loc;
    VMBUF_T_EXTRA;
};

_RIBS_INLINE_ void TEMPLATE(VMBUF_T,make)(struct VMBUF_T *vmb);
_RIBS_INLINE_ void TEMPLATE(VMBUF_T,reset)(struct VMBUF_T *vmb);
_RIBS_INLINE_ int TEMPLATE(VMBUF_T,free)(struct VMBUF_T *vmb);
_RIBS_INLINE_ int TEMPLATE(VMBUF_T,free_most)(struct VMBUF_T *vmb);

_RIBS_INLINE_ int TEMPLATE(VMBUF_T,resize_by)(struct VMBUF_T *vmb, size_t by);
_RIBS_INLINE_ int TEMPLATE(VMBUF_T,resize_to)(struct VMBUF_T *vmb, size_t new_capacity);
_RIBS_INLINE_ int TEMPLATE(VMBUF_T,resize_if_full)(struct VMBUF_T *vmb);
_RIBS_INLINE_ int TEMPLATE(VMBUF_T,resize_if_less)(struct VMBUF_T *vmb, size_t desired_size);
_RIBS_INLINE_ int TEMPLATE(VMBUF_T,resize_no_check)(struct VMBUF_T *vmb, size_t n);

_RIBS_INLINE_ size_t TEMPLATE(VMBUF_T,alloc)(struct VMBUF_T *vmb, size_t n);
_RIBS_INLINE_ size_t TEMPLATE(VMBUF_T,alloczero)(struct VMBUF_T *vmb, size_t n);
_RIBS_INLINE_ size_t TEMPLATE(VMBUF_T,alloc_aligned)(struct VMBUF_T *vmb, size_t n);

_RIBS_INLINE_ size_t TEMPLATE(VMBUF_T,num_elements)(struct VMBUF_T *vmb, size_t size_of_element);

_RIBS_INLINE_ char *TEMPLATE(VMBUF_T,data)(struct VMBUF_T *vmb);
_RIBS_INLINE_ char *TEMPLATE(VMBUF_T,data_ofs)(struct VMBUF_T *vmb, size_t loc);

_RIBS_INLINE_ size_t TEMPLATE(VMBUF_T,wavail)(struct VMBUF_T *vmb);
_RIBS_INLINE_ size_t TEMPLATE(VMBUF_T,ravail)(struct VMBUF_T *vmb);

_RIBS_INLINE_ char *TEMPLATE(VMBUF_T,wloc)(struct VMBUF_T *vmb);
_RIBS_INLINE_ char *TEMPLATE(VMBUF_T,rloc)(struct VMBUF_T *vmb);

_RIBS_INLINE_ size_t TEMPLATE(VMBUF_T,rlocpos)(struct VMBUF_T *vmb);
_RIBS_INLINE_ size_t TEMPLATE(VMBUF_T,wlocpos)(struct VMBUF_T *vmb);

_RIBS_INLINE_ void TEMPLATE(VMBUF_T,rlocset)(struct VMBUF_T *vmb, size_t ofs);
_RIBS_INLINE_ void TEMPLATE(VMBUF_T,wlocset)(struct VMBUF_T *vmb, size_t ofs);

_RIBS_INLINE_ void TEMPLATE(VMBUF_T,rrewind)(struct VMBUF_T *vmb, size_t by);
_RIBS_INLINE_ void TEMPLATE(VMBUF_T,wrewind)(struct VMBUF_T *vmb, size_t by);

_RIBS_INLINE_ size_t TEMPLATE(VMBUF_T,capacity)(struct VMBUF_T *vmb);

_RIBS_INLINE_ void TEMPLATE(VMBUF_T,unsafe_wseek)(struct VMBUF_T *vmb, size_t by);
_RIBS_INLINE_ int TEMPLATE(VMBUF_T,wseek)(struct VMBUF_T *vmb, size_t by);
_RIBS_INLINE_ void TEMPLATE(VMBUF_T,rseek)(struct VMBUF_T *vmb, size_t by);

_RIBS_INLINE_ void TEMPLATE(VMBUF_T,rreset)(struct VMBUF_T *vmb);
_RIBS_INLINE_ void TEMPLATE(VMBUF_T,wreset)(struct VMBUF_T *vmb);

_RIBS_INLINE_ int TEMPLATE(VMBUF_T,sprintf)(struct VMBUF_T *vmb, const char *format, ...);
_RIBS_INLINE_ int TEMPLATE(VMBUF_T,vsprintf)(struct VMBUF_T *vmb, const char *format, va_list ap);
_RIBS_INLINE_ int TEMPLATE(VMBUF_T,strcpy)(struct VMBUF_T *vmb, const char *src);

_RIBS_INLINE_ void TEMPLATE(VMBUF_T,remove_last_if)(struct VMBUF_T *vmb, char c);

_RIBS_INLINE_ int TEMPLATE(VMBUF_T,read)(struct VMBUF_T *vmb, int fd);
_RIBS_INLINE_ int TEMPLATE(VMBUF_T,write)(struct VMBUF_T *vmb, int fd);

_RIBS_INLINE_ int TEMPLATE(VMBUF_T,memcpy)(struct VMBUF_T *vmb, const void *src, size_t n);
_RIBS_INLINE_ void TEMPLATE(VMBUF_T,memset)(struct VMBUF_T *vmb, int c, size_t n);

_RIBS_INLINE_ int TEMPLATE(VMBUF_T,strftime)(struct VMBUF_T *vmb, const char *format, const struct tm *tm);

#include "../src/_vmbuf.c"
