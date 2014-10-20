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
#include "memalloc.h"
#include <sys/mman.h>
#include "logger.h"
#include "mempool.h"
#include "ilog2.h"

int memalloc_new_block(struct memalloc *ma) {
    size_t size;
    if (list_is_null(&ma->memblocks))
        list_init(&ma->memblocks);
    size = ma->capacity ? ma->capacity << 1 : MEMALLOC_INITIAL_BLOCK_SIZE;
    void *mem = mempool_alloc_chunk(size);
    if (NULL == mem)
        return LOGGER_ERROR("failed to allocate memory: %zu (most likely total memory allocations exceeded 4GB)", size), -1;
    struct memalloc_block *new_block = mem;
    new_block->size.size = size;
    new_block->size.type = MEMALLOC_MEM_TYPE_MALLOC;
    list_insert_tail(&ma->memblocks, &new_block->_memblocks);

    ma->capacity = size;
    ma->avail = size - sizeof(struct memalloc_block);
    ma->mem = mem + sizeof(struct memalloc_block);
    return 0;
}

size_t memalloc_usage(struct memalloc *ma) {
    if (list_is_null(&ma->memblocks))
        return 0;
    struct list *it;
    size_t usage = 0;
    LIST_FOR_EACH(&ma->memblocks, it) {
        struct memalloc_block *cur_block = LIST_ENTRY(it, struct memalloc_block, _memblocks);
        usage += cur_block->size.size;
    }
    usage -= ma->avail;
    return usage;
}

size_t memalloc_alloc_raw(struct memalloc *ma, size_t size, void **memblock) {
    if (list_is_null(&ma->memblocks))
        list_init(&ma->memblocks);
    size = next_p2_64(size + sizeof(struct memalloc_block));
    void *mem = mempool_alloc_chunk(size);
    if (NULL == mem)
        return LOGGER_ERROR("failed to allocate memory: %zu", size), 0;
    struct memalloc_block *new_block = mem;
    new_block->size.size = size;
    new_block->size.type = MEMALLOC_MEM_TYPE_BUF;
    list_insert_tail(&ma->memblocks, &new_block->_memblocks);
    *memblock = mem + sizeof(struct memalloc_block);
    return size - sizeof(struct memalloc_block);
}

void memalloc_free_raw(void *mem) {
    struct memalloc_block *block = mem - sizeof(struct memalloc_block);
    list_remove(&block->_memblocks);
    mempool_free_chunk(block, block->size.size);
}
