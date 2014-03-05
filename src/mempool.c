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
#include "mempool.h"
#include "ilog2.h"
#include <sys/mman.h>
#include <time.h>
#include "logger.h"

#define MIN_CHUNK_SIZE_BITS (12)
#define MIN_CHUNK_SIZE (1U << MIN_CHUNK_SIZE_BITS)
#define NUM_CHUNK_BUCKETS (32-MIN_CHUNK_SIZE_BITS)

struct memchunk {
    struct memchunk *next;
};

struct memchunks {
    size_t num_chunks;
    size_t num_free_chunks;
    struct memchunk *head;
};

static struct memchunks memchunks[NUM_CHUNK_BUCKETS] = { [0 ... NUM_CHUNK_BUCKETS-1] = {0,0,NULL} };

static inline int _mempool_alloc_validate_size(size_t s) {
    if (s < MIN_CHUNK_SIZE || s != next_p2(s))
        return LOGGER_ERROR("size '%zu' is not power of 2 or too small (<%u)", s, MIN_CHUNK_SIZE), -1;
    return 0;
}

void *mempool_alloc_chunk(size_t s) {
    if (0 > _mempool_alloc_validate_size(s))
        return NULL;
    uint32_t chunk_index = ilog2(s) - MIN_CHUNK_SIZE_BITS;
    void *mem = memchunks[chunk_index].head;
    if (NULL == mem) {
        ++memchunks[chunk_index].num_chunks;
        LOGGER_INFO("mempool: allocating %zu", s);
        mem = mmap(NULL, s, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (MAP_FAILED == mem)
            return LOGGER_PERROR("mmap"), NULL;
    } else {
        memchunks[chunk_index].head = memchunks[chunk_index].head->next;
        --memchunks[chunk_index].num_free_chunks;
    }
    return mem;
}

int mempool_free_chunk(void *mem, size_t s) {
    if (0 > _mempool_alloc_validate_size(s))
        return -1;
    uint32_t chunk_index = ilog2(s) - MIN_CHUNK_SIZE_BITS;
    struct memchunk *chunk = mem;
    chunk->next = memchunks[chunk_index].head;
    memchunks[chunk_index].head = chunk;
    ++memchunks[chunk_index].num_free_chunks;
    return 0;
}

void mempool_dump_stats(void) {
    int i;
    size_t s = MIN_CHUNK_SIZE;
    const char HEADER[] = "=== memory stats ===";
    LOGGER_INFO("%*s", (int)(50 + sizeof(HEADER))/2, HEADER);
    LOGGER_INFO("%15s   %15s   %15s", "block size", "num chunks", "free chunks");
    for (i = 0; i < NUM_CHUNK_BUCKETS; ++i, s <<= 1) {
        if (0 < memchunks[i].num_chunks)
            LOGGER_INFO("%15zu   %15zu   %15zu", s, memchunks[i].num_chunks, memchunks[i].num_free_chunks);
    }
}
