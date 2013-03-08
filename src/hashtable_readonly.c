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

#include "hashtable_readonly.h"
#include "string.h"

struct ht_entry {
    uint32_t hashcode;
    uint32_t rec;
};

int hashtablefile_readonly_init(struct hashtablefile_readonly *ht, const char *file_name) {

    if (0 > file_mapper_init(&ht->fm, file_name))
        return -1;
    void *data = file_mapper_data(&ht->fm);

    ht->header = (struct hashtablefile_header *) data;
    ht->data = data;

    return 0;
}

int hashtablefile_readonly_close(struct hashtablefile_readonly *ht) {

    if (0 > file_mapper_free(&ht->fm))
        return -1;

    return 0;
}

static inline struct ht_entry *_hashtablefile_readonly_bucket(struct hashtablefile_readonly *ht, uint32_t bucket) {
    uint32_t *slots = (uint32_t *) (ht->data + sizeof(struct hashtablefile_header));
    uint32_t buckets_ofs;
    if (bucket < HASHTABLE_INITIAL_SIZE) {
        buckets_ofs = slots[0];
    } else {
        uint32_t il2 = ilog2(bucket);
        bucket -= 1 << il2; /* offset within the vector */
        buckets_ofs = slots[il2 - HASHTABLE_INITIAL_SIZE_BITS + 1];
    }
    return ((struct ht_entry *) _data_ofs(ht->data, buckets_ofs)) + bucket;
}

uint32_t hashtablefile_readonly_lookup(struct hashtablefile_readonly *ht, const void *key, size_t key_len) {
    uint32_t hc = hashcode(key, key_len);
    uint32_t mask = ht->header->mask;
    uint32_t bucket = hc & mask;
    for (;;) {
        struct ht_entry *e = _hashtablefile_readonly_bucket(ht, bucket);
        uint32_t ofs = e->rec;
        if (ofs == 0)
            return 0;
        if (hc == e->hashcode) {
            char *rec = _data_ofs(ht->data, ofs);
            char *k = rec + (sizeof(uint32_t) * 2);
            if (*(uint32_t *)rec == key_len && 0 == memcmp(key, k, key_len))
                return ofs;
        }
        ++bucket;
        if (bucket > mask)
            bucket = 0;
    }
    return 0;
}
