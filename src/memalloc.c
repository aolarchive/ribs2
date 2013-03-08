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
#include "memalloc.h"
#include <sys/mman.h>
#include "logger.h"

int memalloc_init(struct memalloc *ma) {
    if (unlikely(NULL != ma->blocks_head)) {
        memalloc_reset(ma);
    } else {
        ma->current_block = ma->blocks_tail = (struct memalloc_block *)&ma->blocks_head;
        ma->capacity = ma->avail = 0;
    }
    return 0;
}

int memalloc_new_block(struct memalloc *ma) {
    size_t size;
    if (0 == ma->capacity) {
        size = MEMALLOC_INITIAL_BLOCK_SIZE;
    } else {
        size = ma->capacity << 1;
    }

    void *mem;
    if (ma->current_block == ma->blocks_tail) {
        mem = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (mem == MAP_FAILED)
            return LOGGER_PERROR("mmap"), -1;
        ma->current_block = mem;
        ma->blocks_tail->next = ma->current_block;
        ma->blocks_tail = ma->current_block;
    } else {
        ma->current_block = ma->current_block->next;
        mem = ma->current_block;
    }

    ma->capacity = size;
    ma->avail = size - sizeof(struct memalloc_block);
    ma->mem = mem + sizeof(struct memalloc_block);
    return 0;
}
