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
#include "memalloc.h"
#include <sys/mman.h>
#include "logger.h"
#include "mempool.h"

int memalloc_new_block(struct memalloc *ma) {
    size_t size;
    if (0 == ma->capacity) {
        ma->blocks_head = NULL;
        ma->blocks_tail = (struct memalloc_block *)&ma->blocks_head;
        size = MEMALLOC_INITIAL_BLOCK_SIZE;
    } else {
        size = ma->capacity << 1;
    }

    void *mem = mempool_alloc_chunk(size);
    if (NULL == mem)
        return LOGGER_ERROR("failed to allocate memory: %zu", size), -1;
    struct memalloc_block *new_block = mem;
    new_block->next = NULL;
    ma->blocks_tail->next = new_block;
    ma->blocks_tail = new_block;

    ma->capacity = size;
    ma->avail = size - sizeof(struct memalloc_block);
    ma->mem = mem + sizeof(struct memalloc_block);
    return 0;
}

size_t memalloc_usage(struct memalloc *ma) {
    size_t usage = 0;
    if (0 < ma->capacity) {
        struct memalloc_block *cur_block = ma->blocks_head;
        size_t size = MEMALLOC_INITIAL_BLOCK_SIZE;
        for (;;size <<= 1) {
            usage += size;
            cur_block = cur_block->next;
            if (NULL == cur_block) break;
        }
        usage -= ma->avail;
    }
    return usage;
}
