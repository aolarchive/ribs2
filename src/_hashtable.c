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

static inline struct ht_entry *TEMPLATE_HTBL_FUNC(_hashtable, T, bucket)(struct TEMPLATE_HTBL(hashtable, T) *ht, uint32_t bucket) {
    uint32_t *slots = HT_SLOT;
    uint32_t buckets_ofs;
    if (bucket < HASHTABLE_INITIAL_SIZE) {
        buckets_ofs = slots[0];
    } else {
        uint32_t il2 = ilog2(bucket);
        bucket -= 1 << il2; /* offset within the vector */
        buckets_ofs = slots[il2 - HASHTABLE_INITIAL_SIZE_BITS + 1];
    }
    return (struct ht_entry *)TEMPLATE(TS, data_ofs)(&ht->data, buckets_ofs) + bucket;
}

static inline void TEMPLATE_HTBL_FUNC(_hashtable, T, move_buckets_range)(struct TEMPLATE_HTBL(hashtable, T) *ht, uint32_t new_mask, uint32_t begin, uint32_t end) {
    for (; begin < end; ++begin) {
        struct ht_entry *e = TEMPLATE_HTBL_FUNC(_hashtable, T, bucket)(ht, begin);
        if (0 == e->rec)
            continue;
        uint32_t new_bkt_idx = e->hashcode & new_mask;
        /* if already in the right place skip it */
        if (begin == new_bkt_idx)
            continue;
        uint32_t rec = e->rec;
        /* free the bucket so it can be reused */
        e->rec = 0;
        for (;;) {
            struct ht_entry *new_bucket = TEMPLATE_HTBL_FUNC(_hashtable, T, bucket)(ht, new_bkt_idx);
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

static inline int TEMPLATE_HTBL_FUNC(_hashtable, T, grow)(struct TEMPLATE_HTBL(hashtable, T) *ht) {
    uint32_t capacity = HT_MASK + 1;
    uint32_t ofs = TEMPLATE(TS, alloczero)(&ht->data, capacity * sizeof(struct ht_entry));
    uint32_t il2 = ilog2(capacity);
    uint32_t *slots = HT_SLOT;
    slots[il2 - HASHTABLE_INITIAL_SIZE_BITS + 1] = ofs;
    uint32_t new_mask = (capacity << 1) - 1;
    uint32_t b = 0;
    for (; 0 != TEMPLATE_HTBL_FUNC(_hashtable, T, bucket)(ht, b)->rec; ++b);
    TEMPLATE_HTBL_FUNC(_hashtable, T, move_buckets_range)(ht, new_mask, b, capacity);
    TEMPLATE_HTBL_FUNC(_hashtable, T, move_buckets_range)(ht, new_mask, 0, b);
    HT_MASK = new_mask;
    return 0;
}

uint32_t TEMPLATE_HTBL_FUNC(hashtable, T, insert)(struct TEMPLATE_HTBL(hashtable, T) *ht, const void *key, size_t key_len, const void *val, size_t val_len) {
    if (unlikely(HT_SIZE > (HT_MASK >> 1)) && 0 > TEMPLATE_HTBL_FUNC(_hashtable, T, grow)(ht))
        return 0;
    uint32_t hc = hashcode(key, key_len);
    uint32_t bucket = hc & HT_MASK;
    for (;;) {
        struct ht_entry *e = TEMPLATE_HTBL_FUNC(_hashtable, T, bucket)(ht, bucket);
        if (0 == e->rec) {
            e->hashcode = hc;
            uint32_t ofs = TEMPLATE(TS, wlocpos)(&ht->data);
            e->rec = ofs;
            uint32_t *rec = (uint32_t *)TEMPLATE(TS, data_ofs)(&ht->data, TEMPLATE(TS, alloc)(&ht->data, 2 * sizeof(uint32_t)));
            rec[0] = key_len;
            rec[1] = val_len;
            TEMPLATE(TS, memcpy)(&ht->data, key, key_len);
            TEMPLATE(TS, memcpy)(&ht->data, val, val_len);
            ++HT_SIZE;
            return ofs;
        } else {
            ++bucket;
            if (bucket > HT_MASK)
                bucket = 0;
        }
    }
    return 0;
}

uint32_t TEMPLATE_HTBL_FUNC(hashtable, T, insert_new)(struct TEMPLATE_HTBL(hashtable, T) *ht, const void *key, size_t key_len, size_t val_len) {
    if (unlikely(HT_SIZE > (HT_MASK >> 1)) && 0 > TEMPLATE_HTBL_FUNC(_hashtable, T, grow)(ht))
        return 0;
    uint32_t hc = hashcode(key, key_len);
    uint32_t bucket = hc & HT_MASK;
    for (;;) {
        struct ht_entry *e = TEMPLATE_HTBL_FUNC(_hashtable, T, bucket)(ht, bucket);
        if (0 == e->rec) {
            e->hashcode = hc;
            uint32_t ofs = TEMPLATE(TS, wlocpos)(&ht->data);
            e->rec = ofs;
            uint32_t *rec = (uint32_t *)TEMPLATE(TS, data_ofs)(&ht->data, TEMPLATE(TS, alloc)(&ht->data, 2 * sizeof(uint32_t)));
            rec[0] = key_len;
            rec[1] = val_len;
            TEMPLATE(TS, memcpy)(&ht->data, key, key_len);
            TEMPLATE(TS, alloc)(&ht->data, val_len);
            ++HT_SIZE;
            return ofs;
        } else {
            ++bucket;
            if (bucket > HT_MASK)
                bucket = 0;
        }
    }
    return 0;
}

uint32_t TEMPLATE_HTBL_FUNC(hashtable, T, lookup_insert)(struct TEMPLATE_HTBL(hashtable, T) *ht, const void *key, size_t key_len, const void *val, size_t val_len) {
    uint32_t hc = hashcode(key, key_len);
    uint32_t bucket = hc & HT_MASK;
    for (;;) {
        struct ht_entry *e = TEMPLATE_HTBL_FUNC(_hashtable, T, bucket)(ht, bucket);
        if (0 == e->rec) {
            if (unlikely(HT_SIZE > (HT_MASK >> 1))) {
                int res = TEMPLATE_HTBL_FUNC(_hashtable, T, grow)(ht);
                if (0 > res)
                    return 0;
                bucket = hc & HT_MASK;
                continue;
            }
            e->hashcode = hc;
            uint32_t ofs = TEMPLATE(TS, wlocpos)(&ht->data);
            e->rec = ofs;
            uint32_t *rec = (uint32_t *)TEMPLATE(TS, data_ofs)(&ht->data, TEMPLATE(TS, alloc)(&ht->data, 2 * sizeof(uint32_t)));
            rec[0] = key_len;
            rec[1] = val_len;
            TEMPLATE(TS, memcpy)(&ht->data, key, key_len);
            TEMPLATE(TS, memcpy)(&ht->data, val, val_len);
            ++HT_SIZE;
            return ofs;
        } else if (hc == e->hashcode) {
            char *rec = TEMPLATE(TS, data_ofs)(&ht->data, e->rec);
            char *k = rec + (sizeof(uint32_t) * 2);
            if (*(uint32_t *)rec == key_len && 0 == memcmp(key, k, key_len))
                return e->rec;
        } else {
            ++bucket;
            if (bucket > HT_MASK)
                bucket = 0;
        }
    }
    return 0;
}

uint32_t TEMPLATE_HTBL_FUNC(hashtable, T, lookup)(struct TEMPLATE_HTBL(hashtable, T) *ht, const void *key, size_t key_len) {
    uint32_t hc = hashcode(key, key_len);
    uint32_t mask = HT_MASK;
    uint32_t bucket = hc & mask;
    for (;;) {
        struct ht_entry *e = TEMPLATE_HTBL_FUNC(_hashtable, T, bucket)(ht, bucket);
        uint32_t ofs = e->rec;
        if (ofs == 0)
            return 0;
        if (hc == e->hashcode) {
            char *rec = TEMPLATE(TS, data_ofs)(&ht->data, ofs);
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

static inline void TEMPLATE_HTBL_FUNC(_hashtable, T, fix_chain_down)(struct TEMPLATE_HTBL(hashtable, T) *ht, uint32_t mask, uint32_t bucket) {
    for(;;) {
        struct ht_entry *bkt = TEMPLATE_HTBL_FUNC(_hashtable, T, bucket)(ht, bucket);
        if (0 == bkt->rec) /* is end of chain? */
            break;
        uint32_t new_bucket = bkt->hashcode & mask; /* ideal bucket */
        for (;;) {
            if (bucket == new_bucket) /* no need to move */
                break;
            struct ht_entry *new_bkt = TEMPLATE_HTBL_FUNC(_hashtable, T, bucket)(ht, new_bucket);
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

uint32_t TEMPLATE_HTBL_FUNC(hashtable, T, remove)(struct TEMPLATE_HTBL(hashtable, T) *ht, const void *key, size_t key_len) {
    uint32_t hc = hashcode(key, key_len);
    uint32_t mask = HT_MASK;
    uint32_t bucket = hc & mask;
    for (;;) {
        struct ht_entry *e = TEMPLATE_HTBL_FUNC(_hashtable, T, bucket)(ht, bucket);
        uint32_t ofs = e->rec;
        if (ofs == 0)
            return 0;
        if (hc == e->hashcode) {
            char *rec = TEMPLATE(TS, data_ofs)(&ht->data, ofs);
            char *k = rec + (sizeof(uint32_t) * 2);
            if (*(uint32_t *)rec == key_len && 0 == memcmp(key, k, key_len)) {
                e->rec = 0; /* mark as deleted */
                ++bucket;
                if (bucket > mask)
                    bucket = 0;
                TEMPLATE_HTBL_FUNC(_hashtable, T, fix_chain_down)(ht, mask, bucket);
                return ofs;
            }
        }
        ++bucket;
        if (bucket > mask)
            bucket = 0;
    }
    return 0;
}

int TEMPLATE_HTBL_FUNC(hashtable, T, foreach)(struct TEMPLATE_HTBL(hashtable, T) *ht, int (*func)(uint32_t rec)) {
    uint32_t capacity = HT_MASK + 1;
    uint32_t num_slots = ilog2(capacity) - HASHTABLE_INITIAL_SIZE_BITS + 1;
    uint32_t *slots = HT_SLOT;
    uint32_t s;
    uint32_t vect_size = HASHTABLE_INITIAL_SIZE;
    for (s = 0; s < num_slots; ++s) {
        struct ht_entry *hte = (struct ht_entry *)TEMPLATE(TS, data_ofs)(&ht->data, slots[s]), *htend = hte + vect_size;
        for (; hte != htend; ++hte) {
            if (0 < hte->rec && 0 > func(hte->rec))
                return -1;
        }
        if (s) vect_size <<= 1;
    }
    return 0;
}
