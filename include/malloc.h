#ifndef _MALLOC__H_
#define _MALLOC__H_

#include "ribs_defs.h"
#include "context.h"
#include "memalloc.h"

_RIBS_INLINE_ void *ribs_malloc(size_t size);
_RIBS_INLINE_ void *ribs_calloc(size_t nmemb, size_t size);
_RIBS_INLINE_ void ribs_reset_malloc(void);

#include "../src/_malloc.c"

#endif // _MALLOC__H_
