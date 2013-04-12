#ifndef _MALLOC__H_
#define _MALLOC__H_

#include "ribs_defs.h"
#include "context.h"
#include "memalloc.h"

_RIBS_INLINE_ void *ribs_malloc(size_t size);
_RIBS_INLINE_ void *ribs_calloc(size_t nmemb, size_t size);
_RIBS_INLINE_ void ribs_reset_malloc(void);
_RIBS_INLINE_ char *ribs_malloc_vsprintf(const char *format, va_list ap);
_RIBS_INLINE_ char *ribs_malloc_sprintf(const char *format, ...);
_RIBS_INLINE_ void *ribs_memdup(const void *s, size_t n);
_RIBS_INLINE_ char *ribs_strdup(const char *s);
_RIBS_INLINE_ char *ribs_malloc_strftime(const char *format, const struct tm *tm);

#include "../src/_malloc.c"

#endif // _MALLOC__H_
