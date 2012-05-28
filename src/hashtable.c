/*
    This file is part of RIBS (Robust Infrastructure for Backend Systems).
    RIBS is an infrastructure for building great SaaS applications (but not
    limited to).

    Copyright (C) 2011 Adap.tv, Inc.

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
#include "hash_funcs.h"
#include "ilog2.h"

struct ht_entry {
    uint32_t hashcode;
    uint32_t rec;
};

int hashtable_init(struct hashtable *ht, uint32_t nel) {
    nel = next_p2(nel);
    vmbuf_make(&ht->data);
    if (0 > vmbuf_init(&ht->data, sizeof(struct ht_entry) * nel))
        return -1;
    vmbuf_alloczero(&ht->data, sizeof(struct ht_entry) * nel);
    ht->size = 0;
    ht->mask = nel - 1;
    return 0;
}

uint32_t hashtable_insert(struct hashtable *ht, const void *key, size_t key_len, const void *val, size_t val_len) {
    uint32_t hc = hashcode(key, key_len);
    uint32_t mask = ht->mask;
    uint32_t bucket = hc & mask;
    struct ht_entry *entries = (struct ht_entry *)vmbuf_data(&ht->data);
    for (;;) {
        struct ht_entry *e = entries + bucket;
        if (0 == e->rec) {
            e->hashcode = hc;
            uint32_t ofs = vmbuf_wlocpos(&ht->data);
            e->rec = ofs;
            uint32_t *rec = (uint32_t *)vmbuf_data_ofs(&ht->data, vmbuf_alloc(&ht->data, 2 * sizeof(uint32_t)));
            rec[0] = key_len;
            rec[1] = val_len;
            vmbuf_memcpy(&ht->data, key, key_len);
            vmbuf_memcpy(&ht->data, val, val_len);
            ++ht->size;
            return ofs;
        } else {
            ++bucket;
            if (bucket > mask)
                bucket = 0;
        }
    }
    return 0;
}

uint32_t hashtable_lookup(struct hashtable *ht, const void *key, size_t key_len) {
    uint32_t hc = hashcode(key, key_len);
    uint32_t mask = ht->mask;
    uint32_t bucket = hc & mask;
    struct ht_entry *entries = (struct ht_entry *)vmbuf_data(&ht->data);
    for (;;) {
        struct ht_entry *e = entries + bucket;
        uint32_t ofs = e->rec;
        if (ofs == 0)
            return 0;
        if (hc == e->hashcode) {
            char *rec = vmbuf_data_ofs(&ht->data, ofs);
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


