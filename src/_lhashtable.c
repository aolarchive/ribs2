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
#include <inttypes.h>

_RIBS_INLINE_ uint64_t lhashtable_put_str(struct lhashtable *lht, const char *key, const char *val) {
    return lhashtable_put(lht, key, strlen(key), val, strlen(val) + 1);
}

_RIBS_INLINE_ const char *lhashtable_get_str(struct lhashtable *lht, const char *key) {
    uint64_t rec_ofs = lhashtable_get(lht, key, strlen(key));
    if (0 == rec_ofs)
        return NULL;
    return lhashtable_get_val(lht, rec_ofs);
}

_RIBS_INLINE_ int lhashtable_del_str(struct lhashtable *lht, const char *key) {
    return lhashtable_del(lht, key, strlen(key));
}

_RIBS_INLINE_ void *lhashtable_get_key(struct lhashtable *lht, uint64_t rec_ofs) {
    return ((struct lhashtable_record *)(lht->mem + rec_ofs))->data;
}

_RIBS_INLINE_ size_t lhashtable_get_key_len(struct lhashtable *lht, uint64_t rec_ofs) {
    return ((struct lhashtable_record *)(lht->mem + rec_ofs))->key_len;
}

_RIBS_INLINE_ void *lhashtable_get_val(struct lhashtable *lht, uint64_t rec_ofs) {
    struct lhashtable_record *rec = lht->mem + rec_ofs;
    return rec->data + rec->key_len;
}

_RIBS_INLINE_ size_t lhashtable_get_val_len(struct lhashtable *lht, uint64_t rec_ofs) {
    return ((struct lhashtable_record *)(lht->mem + rec_ofs))->val_len;
}

_RIBS_INLINE_ uint64_t lhashtable_writeloc(struct lhashtable *lht) {
    return ((struct lhashtable_header *)lht->mem)->write_loc;
}

/*
 * inline functions for internal use
 */
static inline int _lhashtable_resize_to(struct lhashtable *lht, uint64_t new_capacity) {
    _LHT_ALIGN_P2(new_capacity, LHT_BLOCK_SIZE);
    if (0 > ftruncate(lht->fd, new_capacity))
        return LOGGER_PERROR("ftruncate, new size = %"PRIu64, new_capacity), -1;
    void *mem = mremap(lht->mem, LHT_GET_HEADER()->capacity, new_capacity, MREMAP_MAYMOVE);
    if (MAP_FAILED == mem)
        return LOGGER_PERROR("mremap, new size = %"PRIu64, new_capacity), -1;
    lht->mem = mem;
    LHT_GET_HEADER()->capacity = new_capacity;
    return 0;
}

static inline int _lhashtable_resize_check(struct lhashtable *lht, size_t n) {
    uint64_t ofs = LHT_GET_HEADER()->write_loc;
    if (LHT_GET_HEADER()->capacity - ofs < n)
        return _lhashtable_resize_to(lht, ofs + n);
    return 0;
}

static inline uint64_t _lhashtable_alloc_ofs(struct lhashtable *lht, size_t n) {
    LHT_N_ALIGN();
    uint64_t ofs = LHT_GET_HEADER()->write_loc;
    if (0 > _lhashtable_resize_check(lht, n))
        return -1;
    LHT_GET_HEADER()->write_loc += n;
    return ofs;
}

static inline uint64_t _lhashtable_data_ofs_to_abs_ofs(struct lhashtable_table *tbl, union lhashtable_data_ofs *data_ofs) {
    return tbl->data_start_ofs[data_ofs->bits.block] + (data_ofs->bits.ofs << LHT_ALLOC_ALIGN_BITS);
}

static inline int _lhashtable_alloc_from_freelist(struct lhashtable *lht, uint64_t sub_table_ofs, size_t n, union lhashtable_data_ofs *data_ofs) {
    LHT_N_ALIGN();
    n >>= LHT_ALLOC_ALIGN_BITS;
    if (0 == LHT_GET_SUB_TABLE()->freelist[n].u32)
        return -1;
    *data_ofs = LHT_GET_SUB_TABLE()->freelist[n];
    uint64_t ofs = _lhashtable_data_ofs_to_abs_ofs(LHT_GET_SUB_TABLE(), data_ofs);
    union lhashtable_data_ofs *data_ofs_ptr = lht->mem + ofs;
    LHT_GET_SUB_TABLE()->freelist[n] = *data_ofs_ptr;
    return 0;
}

static inline int _lhashtable_sub_alloc(struct lhashtable *lht, uint64_t sub_table_ofs, size_t n, union lhashtable_data_ofs *data_ofs) {
    if (0 == _lhashtable_alloc_from_freelist(lht, sub_table_ofs, n, data_ofs))
        return 0;
    LHT_N_ALIGN();
    if (LHT_BLOCK_SIZE - LHT_GET_SUB_TABLE()->next_alloc < n) {
        /* TODO: add leftover to the free list */
        uint64_t ofs = _lhashtable_alloc_ofs(lht, LHT_BLOCK_SIZE);
        ++LHT_GET_SUB_TABLE()->current_block;
        LHT_GET_SUB_TABLE()->data_start_ofs[LHT_GET_SUB_TABLE()->current_block] = ofs;
        LHT_GET_SUB_TABLE()->next_alloc = 0;
    }
    data_ofs->bits.ofs = LHT_GET_SUB_TABLE()->next_alloc >> LHT_ALLOC_ALIGN_BITS;
    data_ofs->bits.block = LHT_GET_SUB_TABLE()->current_block;
    LHT_GET_SUB_TABLE()->next_alloc += n;
    return 0;
}

static inline uint64_t _lhashtable_get_bucket_ofs(struct lhashtable *lht, uint64_t sub_table_ofs, uint32_t bucket) {
    if (bucket < LHT_SUB_TABLE_INITIAL_SIZE)
        return LHT_GET_SUB_TABLE()->buckets_offsets[0] + (sizeof(struct lhashtable_bucket) * bucket);
    uint32_t il2 = ilog2(bucket);
    /* clears the high bit to get offset within the vector */
    bucket -= 1 << il2;
    return LHT_GET_SUB_TABLE()->buckets_offsets[il2 - LHT_SUB_TABLE_MIN_BITS + 1] + (sizeof(struct lhashtable_bucket) * bucket);
}

static inline struct lhashtable_bucket *_lhashtable_get_bucket_ptr(struct lhashtable *lht, uint64_t sub_table_ofs, uint32_t bucket) {
    return lht->mem + _lhashtable_get_bucket_ofs(lht, sub_table_ofs, bucket);
}

static inline void _lhashtable_move_buckets_range(struct lhashtable *lht, uint64_t sub_table_ofs, uint32_t new_mask, uint32_t begin, uint32_t end) {
    for (; begin < end; ++begin) {
        uint64_t bkt_ofs = _lhashtable_get_bucket_ofs(lht, sub_table_ofs, begin);
        struct lhashtable_bucket *bkt = lht->mem + bkt_ofs;
        if (0 == bkt->data_ofs.u32)
            continue;
        uint32_t new_bkt_idx = bkt->hashcode & new_mask;
        /* if already in the right place skip it */
        if (begin == new_bkt_idx)
            continue;
        union lhashtable_data_ofs data_ofs = bkt->data_ofs;
        /* free the bucket so it can be reused */
        bkt->data_ofs.u32 = 0;
        for (;;) {
            uint64_t new_bkt_ofs = _lhashtable_get_bucket_ofs(lht, sub_table_ofs, new_bkt_idx);
            struct lhashtable_bucket *new_bkt = lht->mem + new_bkt_ofs;
            if (0 == new_bkt->data_ofs.u32) {
                new_bkt->hashcode = bkt->hashcode;
                new_bkt->data_ofs = data_ofs;
                break;
            }
            ++new_bkt_idx;
            if (unlikely(new_bkt_idx > new_mask))
                new_bkt_idx = 0;
        }
    }
}

static inline int _lhashtable_resize_table_if_needed(struct lhashtable *lht, uint64_t sub_table_ofs) {
    uint32_t mask = LHT_GET_SUB_TABLE()->mask;
    uint32_t capacity = mask + 1;
    if (likely(LHT_GET_SUB_TABLE()->size < (capacity >> 1)))
        return 0;
    uint64_t new_vect_ofs = _lhashtable_alloc_ofs(lht, sizeof(struct lhashtable_bucket) * capacity);
    uint32_t il2 = ilog2(capacity);
    LHT_GET_SUB_TABLE()->buckets_offsets[il2 - LHT_SUB_TABLE_MIN_BITS + 1] = new_vect_ofs;

    uint32_t new_mask = (capacity << 1) - 1;
    uint32_t b = 0;
    /* skip the begining for now, this part may contain buckets from
       the end of the table (wraparound) */
    for (; 0 != _lhashtable_get_bucket_ptr(lht, sub_table_ofs, b)->data_ofs.u32; ++b);
    _lhashtable_move_buckets_range(lht, sub_table_ofs, new_mask, b, capacity);
    _lhashtable_move_buckets_range(lht, sub_table_ofs, new_mask, 0, b);

    LHT_GET_SUB_TABLE()->mask = new_mask;
    /* TODO: resize here */
    return 0;
}

static inline void _lhashtable_add_to_freelist(struct lhashtable *lht, uint64_t sub_table_ofs, uint64_t rec_ofs, union lhashtable_data_ofs data_ofs, size_t n) {
    LHT_N_ALIGN();
    n >>= LHT_ALLOC_ALIGN_BITS;
    union lhashtable_data_ofs *data_ofs_ptr = lht->mem + rec_ofs;
    *data_ofs_ptr = LHT_GET_SUB_TABLE()->freelist[n];
    LHT_GET_SUB_TABLE()->freelist[n] = data_ofs;
}
