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

#include "hashtable.h"

struct ht_entry {
    uint32_t hashcode;
    uint32_t rec;
};

int hashtable_init(struct hashtable *ht, uint32_t initial_size) {
    size_t size = sizeof(uint32_t) * 32; // 32 slots, offs ==> buckets
    size += HASHTABLE_INITIAL_SIZE * sizeof(struct ht_entry); // allocate buckets in the first slot
    if (0 > vmbuf_init(&ht->data, size))
        return -1;
    vmbuf_alloczero(&ht->data, size);

    uint32_t *slots = (uint32_t *) vmbuf_data(&ht->data);
    slots[0] = sizeof(uint32_t) * 32;
    ht->size = 0;
    ht->mask = HASHTABLE_INITIAL_SIZE - 1;
    if (initial_size > HASHTABLE_INITIAL_SIZE) {
        initial_size = next_p2(initial_size) << 1;
        ht->mask = initial_size - 1;
        uint32_t num_slots = ilog2(initial_size) - HASHTABLE_INITIAL_SIZE_BITS + 1;
        uint32_t capacity = HASHTABLE_INITIAL_SIZE;
        uint32_t s;
        for (s = 1; s < num_slots; ++s) {
            uint32_t ofs = vmbuf_alloczero(&ht->data, capacity * sizeof(struct ht_entry));
            uint32_t *slots = (uint32_t *)vmbuf_data(&ht->data);
            slots[s] = ofs;
            capacity <<= 1;
        }
    }
    return 0;
}

int hashtablefile_init_create(struct hashtablefile *ht, const char *file_name, uint32_t initial_size) {
    size_t size = sizeof(uint32_t) * 32; // 32 slots, offs ==> buckets
    size += HASHTABLE_INITIAL_SIZE * sizeof(struct ht_entry); // allocate buckets in the first slot
    if (0 > vmfile_init(&ht->data, file_name, size))
        return -1;
    vmfile_alloczero(&ht->data, size);

    uint32_t *slots = (uint32_t *) vmfile_data(&ht->data);
    slots[0] = sizeof(uint32_t) * 32;
    ht->size = 0;
    ht->mask = HASHTABLE_INITIAL_SIZE - 1;
    if (initial_size > HASHTABLE_INITIAL_SIZE) {
        initial_size = next_p2(initial_size) << 1;
        ht->mask = initial_size - 1;
        uint32_t num_slots = ilog2(initial_size) - HASHTABLE_INITIAL_SIZE_BITS + 1;
        uint32_t capacity = HASHTABLE_INITIAL_SIZE;
        uint32_t s;
        for (s = 1; s < num_slots; ++s) {
            uint32_t ofs = vmfile_alloczero(&ht->data, capacity * sizeof(struct ht_entry));
            uint32_t *slots = (uint32_t *) vmfile_data(&ht->data);
            slots[s] = ofs;
            capacity <<= 1;
        }
    }
    return 0;
}

#ifdef T
#undef T
#endif

#ifdef TS
#undef TS
#endif

#define T
#define TS vmbuf
#include "_hashtable.c"
#undef TS
#undef T

#define T file
#define TS vmfile
#include "_hashtable.c"
#undef TS
#undef T
