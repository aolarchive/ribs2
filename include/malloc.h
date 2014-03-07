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
#ifndef _MALLOC__H_
#define _MALLOC__H_

#include "ribs_defs.h"
#include "context.h"
#include "memalloc.h"

_RIBS_INLINE_ void *ribs_malloc(size_t size);
_RIBS_INLINE_ void *ribs_malloc2(struct ribs_context *ctx, size_t size);
_RIBS_INLINE_ void *ribs_calloc(size_t nmemb, size_t size);
_RIBS_INLINE_ void *ribs_calloc2(struct ribs_context *ctx, size_t nmemb, size_t size);
_RIBS_INLINE_ void ribs_reset_malloc(void);
_RIBS_INLINE_ void ribs_reset_malloc2(struct ribs_context *ctx);
_RIBS_INLINE_ char *ribs_malloc_vsprintf(const char *format, va_list ap) __attribute__ ((format (gnu_printf, 1, 0)));
_RIBS_INLINE_ char *ribs_malloc_vsprintf2(struct ribs_context *ctx, const char *format, va_list ap) __attribute__ ((format (gnu_printf, 2, 0)));
_RIBS_INLINE_ char *ribs_malloc_sprintf(const char *format, ...) __attribute__ ((format (gnu_printf, 1, 2)));
_RIBS_INLINE_ char *ribs_malloc_sprintf2(struct ribs_context *ctx, const char *format, ...) __attribute__ ((format (gnu_printf, 2, 3)));
_RIBS_INLINE_ void *ribs_memdup(const void *s, size_t n);
_RIBS_INLINE_ void *ribs_memdup2(struct ribs_context *ctx, const void *s, size_t n);
_RIBS_INLINE_ char *ribs_strdup(const char *s);
_RIBS_INLINE_ char *ribs_strdup2(struct ribs_context *ctx, const char *s);
_RIBS_INLINE_ char *ribs_malloc_strftime(const char *format, const struct tm *tm) __attribute__ ((format (strftime, 1, 0)));
_RIBS_INLINE_ char *ribs_malloc_strftime2(struct ribs_context *ctx, const char *format, const struct tm *tm) __attribute__ ((format (strftime, 2, 0)));
_RIBS_INLINE_ size_t ribs_malloc_usage(void);
_RIBS_INLINE_ size_t ribs_malloc_usage2(struct ribs_context *ctx);

#include "../src/_malloc.c"

#endif // _MALLOC__H_
