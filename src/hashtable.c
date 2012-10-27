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
#include "hash_funcs.h"
#include "ilog2.h"

struct ht_entry {
    uint32_t hashcode;
    uint32_t rec;
};

#define HASHTABLE_INITIAL_SIZE_BITS 5
#define HASHTABLE_INITIAL_SIZE (1<<HASHTABLE_INITIAL_SIZE_BITS)


int hashtable_init(struct hashtable *ht, uint32_t initial_size) {
    size_t size = sizeof(uint32_t) * 32; // 32 slots, offs ==> buckets
    size += HASHTABLE_INITIAL_SIZE * sizeof(struct ht_entry); // allocate buckets in the first slot
    if (0 > vmbuf_init(&ht->data, size))
        return -1;
    vmbuf_alloczero(&ht->data, size);
    uint32_t *slots = (uint32_t *)vmbuf_data(&ht->data);
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

static inline struct ht_entry *_hashtable_bucket(struct hashtable *ht, uint32_t bucket) {
    uint32_t *slots = (uint32_t *)vmbuf_data(&ht->data);
    uint32_t buckets_ofs;
    if (bucket < HASHTABLE_INITIAL_SIZE) {
        buckets_ofs = slots[0];
    } else {
        uint32_t il2 = ilog2(bucket);
        bucket -= 1 << il2; /* offset within the vector */
        buckets_ofs = slots[il2 - HASHTABLE_INITIAL_SIZE_BITS + 1];
    }
    return (struct ht_entry *)vmbuf_data_ofs(&ht->data, buckets_ofs) + bucket;
}

static inline void _hashtable_move_buckets_range(struct hashtable *ht, uint32_t new_mask, uint32_t begin, uint32_t end) {
    for (; begin < end; ++begin) {
        struct ht_entry *e = _hashtable_bucket(ht, begin);
        uint32_t new_bkt_idx = e->hashcode & new_mask;
        /* if already in the right place skip it */
        if (begin == new_bkt_idx)
            continue;
        uint32_t rec = e->rec;
        /* free the bucket so it can be reused */
        e->rec = 0;
        for (;;) {
            struct ht_entry *new_bucket = _hashtable_bucket(ht, new_bkt_idx);
            if (0 == new_bucket->rec) {
                new_bucket->hashcode = e->hashcode;
                new_bucket->rec = rec;
                break;
            }
            ++new_bkt_idx;
            if (unlikely(new_bkt_idx > new_mask))
                new_bkt_idx = 0;
        }
    }
}

static inline int _hashtable_grow(struct hashtable *ht) {
    uint32_t capacity = ht->mask + 1;
    uint32_t ofs = vmbuf_alloczero(&ht->data, capacity * sizeof(struct ht_entry));
    uint32_t il2 = ilog2(capacity);
    uint32_t *slots = (uint32_t *)vmbuf_data(&ht->data);
    slots[il2 - HASHTABLE_INITIAL_SIZE_BITS + 1] = ofs;
    uint32_t new_mask = (capacity << 1) - 1;
    uint32_t b = 0;
    for (; 0 != _hashtable_bucket(ht, b)->rec; ++b);
    _hashtable_move_buckets_range(ht, new_mask, b, capacity);
    _hashtable_move_buckets_range(ht, new_mask, 0, b);
    ht->mask = new_mask;
    return 0;
}

uint32_t hashtable_insert(struct hashtable *ht, const void *key, size_t key_len, const void *val, size_t val_len) {
    if (unlikely(ht->size > (ht->mask >> 1)) && 0 > _hashtable_grow(ht))
        return 0;
    uint32_t hc = hashcode(key, key_len);
    uint32_t bucket = hc & ht->mask;
    for (;;) {
        struct ht_entry *e = _hashtable_bucket(ht, bucket);
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
            if (bucket > ht->mask)
                bucket = 0;
        }
    }
    return 0;
}

uint32_t hashtable_insert_new(struct hashtable *ht, const void *key, size_t key_len, size_t val_len) {
    if (unlikely(ht->size > (ht->mask >> 1)) && 0 > _hashtable_grow(ht))
        return 0;
    uint32_t hc = hashcode(key, key_len);
    uint32_t bucket = hc & ht->mask;
    for (;;) {
        struct ht_entry *e = _hashtable_bucket(ht, bucket);
        if (0 == e->rec) {
            e->hashcode = hc;
            uint32_t ofs = vmbuf_wlocpos(&ht->data);
            e->rec = ofs;
            uint32_t *rec = (uint32_t *)vmbuf_data_ofs(&ht->data, vmbuf_alloc(&ht->data, 2 * sizeof(uint32_t)));
            rec[0] = key_len;
            rec[1] = val_len;
            vmbuf_memcpy(&ht->data, key, key_len);
            vmbuf_alloc(&ht->data, val_len);
            ++ht->size;
            return ofs;
        } else {
            ++bucket;
            if (bucket > ht->mask)
                bucket = 0;
        }
    }
    return 0;
}

uint32_t hashtable_lookup(struct hashtable *ht, const void *key, size_t key_len) {
    uint32_t hc = hashcode(key, key_len);
    uint32_t mask = ht->mask;
    uint32_t bucket = hc & mask;
    for (;;) {
        struct ht_entry *e = _hashtable_bucket(ht, bucket);
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

static inline void _hashtable_fix_chain_down(struct hashtable *ht, uint32_t mask, uint32_t bucket) {
    for(;;) {
        struct ht_entry *bkt = _hashtable_bucket(ht, bucket);
        if (0 == bkt->rec) /* is end of chain? */
            break;
        uint32_t new_bucket = bkt->hashcode & mask; /* ideal bucket */
        for (;;) {
            if (bucket == new_bucket) /* no need to move */
                break;
            struct ht_entry *new_bkt = _hashtable_bucket(ht, new_bucket);
            if (0 == new_bkt->rec) {
                /* found new spot, move the bucket */
                *new_bkt = *bkt;
                bkt->rec = 0; /* make as deleted */
                break;
            }
            ++new_bucket;
            if (new_bucket > mask)
                new_bucket = 0;
        }
        ++bucket;
        if (bucket > mask)
            bucket = 0;
    }
}

uint32_t hashtable_remove(struct hashtable *ht, const void *key, size_t key_len) {
    uint32_t hc = hashcode(key, key_len);
    uint32_t mask = ht->mask;
    uint32_t bucket = hc & mask;
    for (;;) {
        struct ht_entry *e = _hashtable_bucket(ht, bucket);
        uint32_t ofs = e->rec;
        if (ofs == 0)
            return 0;
        if (hc == e->hashcode) {
            char *rec = vmbuf_data_ofs(&ht->data, ofs);
            char *k = rec + (sizeof(uint32_t) * 2);
            if (*(uint32_t *)rec == key_len && 0 == memcmp(key, k, key_len)) {
                e->rec = 0; /* mark as deleted */
                ++bucket;
                if (bucket > mask)
                    bucket = 0;
                _hashtable_fix_chain_down(ht, mask, bucket);
                return ofs;
            }
        }
        ++bucket;
        if (bucket > mask)
            bucket = 0;
    }
    return 0;
}

int hashtable_foreach(struct hashtable *ht, int (*func)(uint32_t rec)) {
    uint32_t capacity = ht->mask + 1;
    uint32_t num_slots = ilog2(capacity) - HASHTABLE_INITIAL_SIZE_BITS + 1;
    uint32_t *slots = (uint32_t *)vmbuf_data(&ht->data);
    uint32_t s;
    uint32_t vect_size = HASHTABLE_INITIAL_SIZE;
    for (s = 0; s < num_slots; ++s) {
        struct ht_entry *hte = (struct ht_entry *)vmbuf_data_ofs(&ht->data, slots[s]), *htend = hte + vect_size;
        for (; hte != htend; ++hte) {
            if (0 < hte->rec && 0 > func(hte->rec))
                return -1;
        }
        if (s) vect_size <<= 1;
    }
    return 0;
}

