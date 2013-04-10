#ifndef _MEMPOOL__H_
#define _MEMPOOL__H_

#include "ribs_defs.h"

void *mempool_alloc_chunk(size_t s);
int mempool_free_chunk(void *mem, size_t s);
void mempool_dump_stats(void);

#endif // _MEMPOOL__H_
