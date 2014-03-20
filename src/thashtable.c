/*
    This file is part of RIBS2.0 (Robust Infrastructure for Backend Systems).
    RIBS is an infrastructure for building great SaaS applications (but not
    limited to).

    Copyright (C) 2014 Adap.tv, Inc.

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
#include "thashtable.h"
#include "malloc.h"
#include "ilog2.h"
#include "hash_funcs.h"

#define THASHTABLE_INITIAL_SIZE_BITS 5
#define THASHTABLE_INITIAL_SIZE (1<<THASHTABLE_INITIAL_SIZE_BITS)
#define _THASHTABLE_REC_SIZE offsetof(struct thashtable_internal_rec, data)

struct thashtable *thashtable_create(void) {
    struct thashtable *tht = ribs_malloc(sizeof(struct thashtable));
    tht->mask = THASHTABLE_INITIAL_SIZE - 1;
    tht->size = 0;
    tht->buckets[0] = ribs_calloc(THASHTABLE_INITIAL_SIZE, sizeof(struct tht_entry));
    return tht;
}

static inline struct tht_entry *_thashtable_bucket(struct thashtable *tht, uint32_t bucket) {
    struct tht_entry *buckets;
    if (bucket < THASHTABLE_INITIAL_SIZE) {
        buckets = tht->buckets[0];
    } else {
        uint32_t il2 = ilog2(bucket);
        bucket -= 1 << il2; /* offset within the vector */
        buckets = tht->buckets[il2 - THASHTABLE_INITIAL_SIZE_BITS + 1];
    }
    return buckets + bucket;
}

static inline void _thashtable_move_buckets_range(struct thashtable *tht, uint32_t new_mask, uint32_t begin, uint32_t end) {
    for (; begin < end; ++begin) {
        struct tht_entry *e = _thashtable_bucket(tht, begin);
        uint32_t new_bkt_idx = e->hashcode & new_mask;
        /* if already in the right place skip it */
        if (begin == new_bkt_idx)
            continue;
        void *rec = e->rec;
        /* free the bucket so it can be reused */
        e->rec = NULL;
        for (;;) {
            struct tht_entry *new_bucket = _thashtable_bucket(tht, new_bkt_idx);
            if (NULL == new_bucket->rec) {
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

static inline int _thashtable_grow(struct thashtable *tht) {
    uint32_t capacity = tht->mask + 1;
    uint32_t il2 = ilog2(capacity);
    tht->buckets[il2 - THASHTABLE_INITIAL_SIZE_BITS + 1] = ribs_calloc(capacity, sizeof(struct tht_entry));
    uint32_t new_mask = (capacity << 1) - 1;
    uint32_t b = 0;
    for (; 0 != _thashtable_bucket(tht, b)->rec; ++b);
    _thashtable_move_buckets_range(tht, new_mask, b, capacity);
    _thashtable_move_buckets_range(tht, new_mask, 0, b);
    tht->mask = new_mask;
    return 0;
}

static inline void *_thashtable_alloc_rec(struct thashtable *tht, struct tht_entry *e, size_t key_len, size_t val_len) {
    /* TODO: alloc from free list */
    UNUSED(tht);
    void *rec = ribs_malloc(_THASHTABLE_REC_SIZE + key_len + val_len);
    e->rec = rec;
    return rec;
}

static inline int _thashtable_check_resize(struct thashtable *tht) {
    if (unlikely(tht->size > (tht->mask >> 1)) && 0 > _thashtable_grow(tht))
        return -1;
    return 0;
}

static struct thashtable_internal_rec *_thashtable_new_rec(struct thashtable *tht, struct tht_entry *e, const void *key, size_t key_len, const void *val, size_t val_len) {
    struct thashtable_internal_rec *rec = _thashtable_alloc_rec(tht, e, key_len, val_len);
    rec->key_size = key_len;
    rec->val_size = val_len;
    memcpy(rec->data, key, key_len);
    memcpy(rec->data + key_len, val, val_len);
    return rec;
}

thashtable_rec_t *thashtable_insert(struct thashtable *tht, const void *key, size_t key_len, const void *val, size_t val_len, int *inserted) {
    if (0 > _thashtable_check_resize(tht))
        return 0;
    uint32_t mask = tht->mask;
    uint32_t hc = hashcode(key, key_len);
    uint32_t bucket = hc & mask;
    for (;;) {
        struct tht_entry *e = _thashtable_bucket(tht, bucket);
        if (NULL == e->rec) {
            e->hashcode = hc;
            ++tht->size;
            *inserted = 1;
            return _thashtable_new_rec(tht, e, key, key_len, val, val_len); /* insert new */
        } else if (hc == e->hashcode) {
            struct thashtable_internal_rec *rec = e->rec;
            if (rec->key_size == key_len && 0 == memcmp(key, rec->data, key_len)) {
                *inserted = 0;
                return rec; /* return existing record */
            }
        } else {
            ++bucket;
            if (bucket > mask)
                bucket = 0;
        }
    }
    return 0;
}

thashtable_rec_t *thashtable_put(struct thashtable *tht, const void *key, size_t key_len, const void *val, size_t val_len) {
    if (0 > _thashtable_check_resize(tht))
        return 0;
    uint32_t mask = tht->mask;
    uint32_t hc = hashcode(key, key_len);
    uint32_t bucket = hc & mask;
    for (;;) {
        struct tht_entry *e = _thashtable_bucket(tht, bucket);
        if (NULL == e->rec) {
            e->hashcode = hc;
            ++tht->size;
            return _thashtable_new_rec(tht, e, key, key_len, val, val_len); /* insert new */
        } else if (hc == e->hashcode) {
            struct thashtable_internal_rec *rec = e->rec;
            if (rec->key_size == key_len && 0 == memcmp(key, rec->data, key_len)) {
                return _thashtable_new_rec(tht, e, key, key_len, val, val_len); /* replace with new */
            }
        } else {
            ++bucket;
            if (bucket > mask)
                bucket = 0;
        }
    }
    return 0;
}

thashtable_rec_t *thashtable_insert_alloc(struct thashtable *tht, const void *key, size_t key_len, size_t val_len) {
    if (0 > _thashtable_check_resize(tht))
        return 0;
    uint32_t mask = tht->mask;
    uint32_t hc = hashcode(key, key_len);
    uint32_t bucket = hc & mask;
    for (;;) {
        struct tht_entry *e = _thashtable_bucket(tht, bucket);
        if (0 == e->rec) {
            e->hashcode = hc;
            struct thashtable_internal_rec *rec = _thashtable_alloc_rec(tht, e, key_len, val_len);
            rec->key_size = key_len;
            rec->val_size = val_len;
            memcpy(rec->data, key, key_len);
            ++tht->size;
            return rec;
        } else if (hc == e->hashcode) {
            struct thashtable_internal_rec *rec = e->rec;
            if (rec->key_size == key_len && 0 == memcmp(key, rec->data, key_len))
                return rec->val_size == val_len ? rec : NULL; /* already exists, val size must be the same */
        } else {
            ++bucket;
            if (bucket > mask)
                bucket = 0;
        }
    }
    return 0;
}

thashtable_rec_t *thashtable_lookup(struct thashtable *tht, const void *key, size_t key_len) {
    uint32_t hc = hashcode(key, key_len);
    uint32_t mask = tht->mask;
    uint32_t bucket = hc & mask;
    for (;;) {
        struct tht_entry *e = _thashtable_bucket(tht, bucket);
        struct thashtable_internal_rec *rec = e->rec;
        if (NULL == rec)
            return 0;
        if (hc == e->hashcode) {
            if (rec->key_size == key_len && 0 == memcmp(key, rec->data, key_len))
                return rec;
        }
        ++bucket;
        if (bucket > mask)
            bucket = 0;
    }
    return 0;
}

static inline void _thashtable_fix_chain_down(struct thashtable *tht, uint32_t mask, uint32_t bucket) {
    for(;;) {
        struct tht_entry *bkt = _thashtable_bucket(tht, bucket);
        if (0 == bkt->rec) /* is end of chain? */
            break;
        uint32_t new_bucket = bkt->hashcode & mask; /* ideal bucket */
        for (;;) {
            if (bucket == new_bucket) /* no need to move */
                break;
            struct tht_entry *new_bkt = _thashtable_bucket(tht, new_bucket);
            if (0 == new_bkt->rec) {
                /* found new spot, move the bucket */
                *new_bkt = *bkt;
                bkt->rec = NULL; /* make as deleted */
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

thashtable_rec_t *thashtable_remove(struct thashtable *tht, const void *key, size_t key_len) {
    uint32_t hc = hashcode(key, key_len);
    uint32_t mask = tht->mask;
    uint32_t bucket = hc & mask;
    for (;;) {
        struct tht_entry *e = _thashtable_bucket(tht, bucket);
        struct thashtable_internal_rec *rec = e->rec;
        if (NULL == rec)
            return 0;
        if (hc == e->hashcode) {
            if (rec->key_size == key_len && 0 == memcmp(key, rec->data, key_len)) {
                /* TODO: add to free list */
                e->rec = NULL; /* mark as deleted */
                --tht->size;
                ++bucket;
                if (bucket > mask)
                    bucket = 0;
                _thashtable_fix_chain_down(tht, mask, bucket);
                return rec;
            }
        }
        ++bucket;
        if (bucket > mask)
            bucket = 0;
    }
    return 0;
}

int thashtable_foreach(struct thashtable *tht, int (*func)(thashtable_rec_t *rec)) {
    uint32_t capacity = tht->mask + 1;
    uint32_t num_buckets = ilog2(capacity) - THASHTABLE_INITIAL_SIZE_BITS + 1;
    uint32_t b;
    uint32_t vect_size = THASHTABLE_INITIAL_SIZE;
    for (b = 0; b < num_buckets; ++b) {
        struct tht_entry *hte = tht->buckets[b], *htend = hte + vect_size;
        for (; hte != htend; ++hte) {
            if (hte->rec && 0 > func(hte->rec))
                return -1;
        }
        if (b) vect_size <<= 1;
    }
    return 0;
}
