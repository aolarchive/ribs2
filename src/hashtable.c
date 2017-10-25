/*
    This file is part of RIBS2.0 (Robust Infrastructure for Backend Systems).
    RIBS is an infrastructure for building great SaaS applications (but not
    limited to).

    Copyright (C) 2012,2013,2014 Adap.tv, Inc.

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
#include <fcntl.h>

#define HASHTABLE_INITIAL_SIZE_BITS 5
#define HASHTABLE_INITIAL_SIZE (1<<HASHTABLE_INITIAL_SIZE_BITS)

struct ht_entry {
    uint32_t hashcode;
    uint32_t rec;
};

#define _HASHTABLE_SLOTS() _HASHTABLE_HEADER()->slots
#define _HASHTABLE_REC_SIZE sizeof(((union hashtable_internal_rec *)0)->rec)

static int _hashtable_init(struct hashtable *ht, uint32_t initial_size) {
    size_t size = sizeof(struct hashtable_header);
    size += HASHTABLE_INITIAL_SIZE * sizeof(struct ht_entry); // allocate buckets in the first slot
    memset(vmallocator_allocptr(&ht->data, size), 0, size);
    uint32_t *slots = _HASHTABLE_SLOTS();
    slots[0] = sizeof(struct hashtable_header);
    struct hashtable_header *header = _HASHTABLE_HEADER();
    header->size = 0;
    header->mask = HASHTABLE_INITIAL_SIZE - 1;
    if (initial_size > HASHTABLE_INITIAL_SIZE) {
        initial_size = next_p2(initial_size) << 1;
        header->mask = initial_size - 1;
        uint32_t num_slots = ilog2(initial_size) - HASHTABLE_INITIAL_SIZE_BITS + 1;
        uint32_t capacity = HASHTABLE_INITIAL_SIZE;
        uint32_t s;
        for (s = 1; s < num_slots; ++s) {
            uint32_t ofs = vmallocator_alloczero_aligned(&ht->data, capacity * sizeof(struct ht_entry));
            uint32_t *slots = _HASHTABLE_SLOTS();
            slots[s] = ofs;
            capacity <<= 1;
        }
    }
    return 0;
}

int hashtable_init(struct hashtable *ht, uint32_t initial_size) {
    if (0 > vmallocator_init(&ht->data))
        return -1;
    return _hashtable_init(ht, initial_size);
}

int hashtable_open(struct hashtable *ht, uint32_t initial_size, const char *filename, int flags) {
    if (0 > vmallocator_open(&ht->data, filename, flags))
        return -1;
    if (0 == _HASHTABLE_HEADER()->mask)
        return _hashtable_init(ht, initial_size);
    return 0;
}

int hashtable_create(struct hashtable *ht, uint32_t initial_size, const char *filename) {
    return hashtable_open(ht, initial_size, filename, O_CREAT | O_RDWR | O_TRUNC);
}

int hashtable_close(struct hashtable *ht) {
    return vmallocator_close(&ht->data);
}

static inline struct ht_entry *_hashtable_bucket(struct hashtable *ht, uint32_t bucket) {
    uint32_t *slots = _HASHTABLE_SLOTS();
    uint32_t buckets_ofs;
    if (bucket < HASHTABLE_INITIAL_SIZE) {
        buckets_ofs = slots[0];
    } else {
        uint32_t il2 = ilog2(bucket);
        bucket -= 1 << il2; /* offset within the vector */
        buckets_ofs = slots[il2 - HASHTABLE_INITIAL_SIZE_BITS + 1];
    }
    return (struct ht_entry *)vmallocator_ofs2mem(&ht->data, buckets_ofs) + bucket;
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
    uint32_t capacity = _HASHTABLE_HEADER()->mask + 1;
    uint32_t ofs = vmallocator_alloczero_aligned(&ht->data, capacity * sizeof(struct ht_entry));
    uint32_t il2 = ilog2(capacity);
    uint32_t *slots = _HASHTABLE_SLOTS();
    slots[il2 - HASHTABLE_INITIAL_SIZE_BITS + 1] = ofs;
    uint32_t new_mask = (capacity << 1) - 1;
    uint32_t b = 0;
    for (; 0 != _hashtable_bucket(ht, b)->rec; ++b);
    _hashtable_move_buckets_range(ht, new_mask, b, capacity);
    _hashtable_move_buckets_range(ht, new_mask, 0, b);
    _HASHTABLE_HEADER()->mask = new_mask;
    return 0;
}

static inline uint32_t _hashtable_alloc_from_freelist(struct hashtable *ht, uint32_t key_size, uint32_t val_size) {
    uint32_t size = key_size + val_size;
    uint32_t *prev_ofs = &_HASHTABLE_HEADER()->free_list, rec_ofs = *prev_ofs;
    while (0 < rec_ofs) {
        union hashtable_internal_rec *rec = _HASHTABLE_REC(rec_ofs);
        if (rec->free_rec.size == size) {
            *prev_ofs = rec->free_rec.next;
            return rec_ofs;
        }
        prev_ofs = &rec->free_rec.next;
        rec_ofs = rec->free_rec.next;
    }
    return 0;
}

static inline uint32_t _hashtable_alloc_rec(struct hashtable *ht, struct ht_entry *e, size_t key_len, size_t val_len) {
    uint32_t ofs = _hashtable_alloc_from_freelist(ht, key_len, val_len);
    if (0 == ofs) {
        ofs = vmallocator_wlocpos(&ht->data);
        e->rec = ofs;
        return vmallocator_alloc(&ht->data, _HASHTABLE_REC_SIZE + key_len + val_len);
    }
    return e->rec = ofs;
}

static inline int _hashtable_check_resize(struct hashtable *ht) {
    if (unlikely(_HASHTABLE_HEADER()->size > (_HASHTABLE_HEADER()->mask >> 1)) && 0 > _hashtable_grow(ht))
        return -1;
    return 0;
}

uint32_t hashtable_insert(struct hashtable *ht, const void *key, size_t key_len, const void *val, size_t val_len) {
    if (0 > _hashtable_check_resize(ht))
        return 0;
    uint32_t mask = _HASHTABLE_HEADER()->mask;
    uint32_t hc = hashcode(key, key_len);
    uint32_t bucket = hc & mask;
    for (;;) {
        struct ht_entry *e = _hashtable_bucket(ht, bucket);
        if (0 == e->rec) {
            e->hashcode = hc;
            uint32_t ofs = _hashtable_alloc_rec(ht, e, key_len, val_len);
            union hashtable_internal_rec *rec = _HASHTABLE_REC(ofs);
            rec->rec.key_size = key_len;
            rec->rec.val_size = val_len;
            memcpy(rec->rec.data, key, key_len);
            memcpy(rec->rec.data + key_len, val, val_len);
            ++_HASHTABLE_HEADER()->size;
            return ofs;
        } else {
            ++bucket;
            if (unlikely(bucket > mask))
                bucket = 0;
        }
    }
    return 0;
}

uint32_t hashtable_insert_alloc(struct hashtable *ht, const void *key, size_t key_len, size_t val_len) {
    if (0 > _hashtable_check_resize(ht))
        return 0;
    uint32_t mask = _HASHTABLE_HEADER()->mask;
    uint32_t hc = hashcode(key, key_len);
    uint32_t bucket = hc & mask;
    for (;;) {
        struct ht_entry *e = _hashtable_bucket(ht, bucket);
        if (0 == e->rec) {
            e->hashcode = hc;
            uint32_t ofs = _hashtable_alloc_rec(ht, e, key_len, val_len);
            union hashtable_internal_rec *rec = _HASHTABLE_REC(ofs);
            rec->rec.key_size = key_len;
            rec->rec.val_size = val_len;
            memcpy(rec->rec.data, key, key_len);
            ++_HASHTABLE_HEADER()->size;
            return ofs;
        } else {
            ++bucket;
            if (unlikely(bucket > mask))
                bucket = 0;
        }
    }
    return 0;
}

uint32_t hashtable_lookup_insert(struct hashtable *ht, const void *key, size_t key_len, const void *val, size_t val_len) {
    uint32_t mask = _HASHTABLE_HEADER()->mask;
    uint32_t hc = hashcode(key, key_len);
    uint32_t bucket = hc & mask;
    for (;;) {
        struct ht_entry *e = _hashtable_bucket(ht, bucket);
        if (0 == e->rec) {
            if (unlikely(_HASHTABLE_HEADER()->size > (mask >> 1))) {
                if (0 > _hashtable_grow(ht))
                    return 0;
                mask = _HASHTABLE_HEADER()->mask;
                bucket = hc & mask;
                continue;
            }
            e->hashcode = hc;
            uint32_t ofs = _hashtable_alloc_rec(ht, e, key_len, val_len);
            union hashtable_internal_rec *rec = _HASHTABLE_REC(ofs);
            rec->rec.key_size = key_len;
            rec->rec.val_size = val_len;
            memcpy(rec->rec.data, key, key_len);
            memcpy(rec->rec.data + key_len, val, val_len);
            ++_HASHTABLE_HEADER()->size;
            return ofs;
        } else if (hc == e->hashcode) {
            union hashtable_internal_rec *rec = _HASHTABLE_REC(e->rec);
            if (rec->rec.key_size == key_len && 0 == memcmp(key, rec->rec.data, key_len))
                return e->rec;
        }
        ++bucket;
        if (unlikely(bucket > mask))
            bucket = 0;
    }
    return 0;
}

uint32_t hashtable_lookup(struct hashtable *ht, const void *key, size_t key_len) {
    uint32_t hc = hashcode(key, key_len);
    uint32_t mask = _HASHTABLE_HEADER()->mask;
    uint32_t bucket = hc & mask;
    for (;;) {
        struct ht_entry *e = _hashtable_bucket(ht, bucket);
        uint32_t ofs = e->rec;
        if (ofs == 0)
            return 0;
        if (hc == e->hashcode) {
            union hashtable_internal_rec *rec = _HASHTABLE_REC(ofs);
            if (rec->rec.key_size == key_len && 0 == memcmp(key, rec->rec.data, key_len))
                return ofs;
        }
        ++bucket;
        if (unlikely(bucket > mask))
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
            if (unlikely(new_bucket > mask))
                new_bucket = 0;
        }
        ++bucket;
        if (unlikely(bucket > mask))
            bucket = 0;
    }
}

uint32_t hashtable_remove(struct hashtable *ht, const void *key, size_t key_len) {
    struct hashtable_header *header = _HASHTABLE_HEADER();
    uint32_t hc = hashcode(key, key_len);
    uint32_t mask = header->mask;
    uint32_t bucket = hc & mask;
    for (;;) {
        struct ht_entry *e = _hashtable_bucket(ht, bucket);
        uint32_t ofs = e->rec;
        if (ofs == 0)
            return 0;
        if (hc == e->hashcode) {
            union hashtable_internal_rec *rec = _HASHTABLE_REC(ofs);
            if (rec->rec.key_size == key_len && 0 == memcmp(key, rec->rec.data, key_len)) {
                rec->free_rec.size = rec->rec.key_size + rec->rec.val_size;
                rec->free_rec.next = header->free_list;
                header->free_list = ofs;
                e->rec = 0; /* mark as deleted */
                --_HASHTABLE_HEADER()->size;
                ++bucket;
                if (unlikely(bucket > mask))
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
    uint32_t capacity = _HASHTABLE_HEADER()->mask + 1;
    uint32_t num_slots = ilog2(capacity) - HASHTABLE_INITIAL_SIZE_BITS + 1;
    uint32_t *slots = _HASHTABLE_SLOTS();
    uint32_t s;
    uint32_t vect_size = HASHTABLE_INITIAL_SIZE;
    for (s = 0; s < num_slots; ++s) {
        struct ht_entry *hte = (struct ht_entry *)vmallocator_ofs2mem(&ht->data, slots[s]), *htend = hte + vect_size;
        for (; hte != htend; ++hte) {
            if (0 < hte->rec && 0 > func(hte->rec))
                return -1;
        }
        if (s) vect_size <<= 1;
    }
    return 0;
}
