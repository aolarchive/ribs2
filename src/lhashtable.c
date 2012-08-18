#include "lhashtable.h"
#include "hash_funcs.h"
#include "ilog2.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <stdio.h>

const char LHT_SIGNATURE[] = "RIBS_LH";
#define LHT_VERSION 1

/*
 * memory management
 */
#define LHT_BLOCK_SIZE_BITS 20
#define LHT_BLOCK_SIZE (1<<LHT_BLOCK_SIZE_BITS)
#define LHT_ALLOC_ALIGN_BITS 3
#define LHT_ALLOC_ALIGN (1<<LHT_ALLOC_ALIGN_BITS)
/*
 * sub table
 */
#define SUB_TABLE_MIN_BITS 3
#define SUB_TABLE_INITIAL_SIZE (1<<SUB_TABLE_MIN_BITS)

#define HEADER() ((struct lhashtable_header *)lht->mem)
#define SUB_TABLE() ((struct lhashtable_table *)(lht->mem + sub_table_ofs))

#define _ALIGN_P2(x,b) (x)+=(b)-1; (x)&=~((b)-1)

#define N_ALIGN() _ALIGN_P2(n, LHT_ALLOC_ALIGN)

static inline int _lhashtable_resize_to(struct lhashtable *lht, uint64_t new_capacity) {
    _ALIGN_P2(new_capacity, LHT_BLOCK_SIZE);
    if (0 > ftruncate(lht->fd, new_capacity))
        return LOGGER_PERROR("ftruncate, new size = %zu", new_capacity), -1;
    void *mem = mremap(lht->mem, HEADER()->capacity, new_capacity, MREMAP_MAYMOVE);
    if (MAP_FAILED == mem)
        return LOGGER_PERROR("mremap, new size = %zu", new_capacity), -1;
    lht->mem = mem;
    HEADER()->capacity = new_capacity;
    return 0;
}

static inline int _lhashtable_resize_check(struct lhashtable *lht, size_t n) {
    uint64_t ofs = HEADER()->write_loc;
    if (HEADER()->capacity - ofs < n)
        return _lhashtable_resize_to(lht, ofs + n);
    return 0;
}

static inline uint64_t _lhashtable_alloc_ofs(struct lhashtable *lht, size_t n) {
    N_ALIGN();
    uint64_t ofs = HEADER()->write_loc;
    if (0 > _lhashtable_resize_check(lht, n))
        return -1;
    HEADER()->write_loc += n;
    return ofs;
}

int lhashtable_init(struct lhashtable *lht, const char *filename) {
    lhashtable_close(lht);
    struct stat st;
    if (0 == stat(filename, &st)) {
        if (st.st_size < (ssize_t)sizeof(struct lhashtable_header))
            return LOGGER_ERROR("corrupted file (size): %s", filename), -1;
        lht->fd = open(filename, O_RDWR, 0644);
        if (0 > lht->fd)
            return LOGGER_PERROR(filename), -1;
        lht->mem = mmap(NULL, st.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, lht->fd, 0);
        if (MAP_FAILED == lht->mem)
            return LOGGER_PERROR(filename), close(lht->fd), lht->fd = -1;
        if (0 != memcmp(LHT_SIGNATURE, lht->mem, sizeof(LHT_SIGNATURE))) {
            LOGGER_ERROR("corrupted file (signature): %s", filename), -1;
            return close(lht->fd), lht->fd = -1, munmap(lht->mem, st.st_size), -1;
        }
        return 0;
    }
    unlink(filename);
    lht->fd = open(filename, O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (0 > lht->fd)
        return LOGGER_PERROR(filename), -1;
    /* initial size */
    size_t size = sizeof(struct lhashtable_header);
    _ALIGN_P2(size, LHT_BLOCK_SIZE);
    if (0 > ftruncate(lht->fd, size))
        return LOGGER_PERROR(filename), -1;
    lht->mem = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, lht->fd, 0);
    if (MAP_FAILED == lht->mem)
        return LOGGER_PERROR(filename), close(lht->fd), lht->fd = -1;
    struct lhashtable_header *header = (struct lhashtable_header *)lht->mem;
    header->write_loc = 0;
    header->capacity = size;
    /* align, alloc space for the header */
    _lhashtable_alloc_ofs(lht, sizeof(struct lhashtable_header));
    strncpy(HEADER()->signature, LHT_SIGNATURE, sizeof(HEADER()->signature));
    HEADER()->version = LHT_VERSION;
    HEADER()->flags = 0;
    memset(HEADER()->tables_offsets, 0, sizeof(HEADER()->tables_offsets));
    HEADER()->num_data_blocks = 4096; /* TODO: make it configurable */
    int i;
    for (i = 0; i < LHASHTABLE_NUM_SUB_TABLES; ++i) {
        uint64_t sub_table_ofs = _lhashtable_alloc_ofs(lht, sizeof(struct lhashtable_table) + sizeof(uint64_t) * (HEADER()->num_data_blocks + 1));
        HEADER()->tables_offsets[i] = sub_table_ofs;
        memset(SUB_TABLE()->buckets_offsets, 0, sizeof(SUB_TABLE()->buckets_offsets));
        memset(SUB_TABLE()->freelist, 0, sizeof(SUB_TABLE()->freelist));
        memset(SUB_TABLE()->data_start_ofs, 0, sizeof(uint64_t) * (HEADER()->num_data_blocks + 1)); /* 0 is reserved */
        SUB_TABLE()->mask = SUB_TABLE_INITIAL_SIZE - 1;
        SUB_TABLE()->size = 0;
        SUB_TABLE()->next_alloc = LHT_BLOCK_SIZE; /* 0 is reserved ==> set to full */
        SUB_TABLE()->current_block = 0;
        SUB_TABLE()->buckets_offsets[0] = _lhashtable_alloc_ofs(lht, sizeof(struct lhashtable_bucket) * SUB_TABLE_INITIAL_SIZE);
    }
    return 0;
}

int lhashtable_close(struct lhashtable *lht) {
    if (lht->fd < 0)
        return 0;
    if (0 > close(lht->fd))
        LOGGER_ERROR("failed to close file (fd=%d)", lht->fd);
    lht->fd = -1;
    if (0 > munmap(lht->mem, HEADER()->capacity))
        LOGGER_ERROR("failed to unmap file (addr=%p)", lht->mem);
    lht->mem = NULL;
    return 0;
}

static inline uint64_t _lhashtable_data_ofs_to_abs_ofs(struct lhashtable_table *tbl, union lhashtable_data_ofs *data_ofs) {
    return tbl->data_start_ofs[data_ofs->bits.block] + (data_ofs->bits.ofs << LHT_ALLOC_ALIGN_BITS);
}

static inline int _lhashtable_alloc_from_freelist(struct lhashtable *lht, uint64_t sub_table_ofs, size_t n, union lhashtable_data_ofs *data_ofs) {
    N_ALIGN();
    n >>= LHT_ALLOC_ALIGN_BITS;
    if (0 == SUB_TABLE()->freelist[n].u32)
        return -1;
    *data_ofs = SUB_TABLE()->freelist[n];
    uint64_t ofs = _lhashtable_data_ofs_to_abs_ofs(SUB_TABLE(), data_ofs);
    union lhashtable_data_ofs *data_ofs_ptr = lht->mem + ofs;
    SUB_TABLE()->freelist[n] = *data_ofs_ptr;
    return 0;
}

static inline int _lhashtable_sub_alloc(struct lhashtable *lht, uint64_t sub_table_ofs, size_t n, union lhashtable_data_ofs *data_ofs) {
    if (0 == _lhashtable_alloc_from_freelist(lht, sub_table_ofs, n, data_ofs))
        return 0;
    N_ALIGN();
    if (LHT_BLOCK_SIZE - SUB_TABLE()->next_alloc < n) {
        /* TODO: add leftover to the free list */
        uint64_t ofs = _lhashtable_alloc_ofs(lht, LHT_BLOCK_SIZE);
        ++SUB_TABLE()->current_block;
        SUB_TABLE()->data_start_ofs[SUB_TABLE()->current_block] = ofs;
        SUB_TABLE()->next_alloc = 0;
    }
    data_ofs->bits.ofs = SUB_TABLE()->next_alloc >> LHT_ALLOC_ALIGN_BITS;
    data_ofs->bits.block = SUB_TABLE()->current_block;
    SUB_TABLE()->next_alloc += n;
    return 0;
}

static inline uint64_t _lhashtable_get_bucket_ofs(struct lhashtable *lht, uint64_t sub_table_ofs, uint32_t bucket) {
    if (bucket < SUB_TABLE_INITIAL_SIZE)
        return SUB_TABLE()->buckets_offsets[0] + (sizeof(struct lhashtable_bucket) * bucket);
    uint32_t il2 = ilog2(bucket);
    /* clears the high bit to get offset within the vector */
    bucket -= 1 << il2;
    return SUB_TABLE()->buckets_offsets[il2 - SUB_TABLE_MIN_BITS + 1] + (sizeof(struct lhashtable_bucket) * bucket);
}

static inline struct lhashtable_bucket *_lhashtable_get_bucket_ptr(struct lhashtable *lht, uint64_t sub_table_ofs, uint32_t bucket) {
    return lht->mem + _lhashtable_get_bucket_ofs(lht, sub_table_ofs, bucket);
}

static void _lhashtable_move_buckets_range(struct lhashtable *lht, uint64_t sub_table_ofs, uint32_t new_mask, uint32_t begin, uint32_t end) {
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
    uint32_t mask = SUB_TABLE()->mask;
    uint32_t capacity = mask + 1;
    if (likely(SUB_TABLE()->size < (capacity >> 1)))
        return 0;
    uint64_t new_vect_ofs = _lhashtable_alloc_ofs(lht, sizeof(struct lhashtable_bucket) * capacity);
    uint32_t il2 = ilog2(capacity);
    SUB_TABLE()->buckets_offsets[il2 - SUB_TABLE_MIN_BITS + 1] = new_vect_ofs;

    uint32_t new_mask = (capacity << 1) - 1;
    uint32_t b = 0;
    /* skip the begining for now, this part may contain buckets from
       the end of the table (wraparound) */
    for (; 0 != _lhashtable_get_bucket_ptr(lht, sub_table_ofs, b)->data_ofs.u32; ++b);
    _lhashtable_move_buckets_range(lht, sub_table_ofs, new_mask, b, capacity);
    _lhashtable_move_buckets_range(lht, sub_table_ofs, new_mask, 0, b);

    SUB_TABLE()->mask = new_mask;
    /* TODO: resize here */
    return 0;
}

int lhashtable_insert(struct lhashtable *lht, const void *key, size_t key_len, const void *val, size_t val_len) {
    uint32_t h = hashcode(key, key_len);
    uint32_t sub_tbl_idx = (h ^ (h >> 8)) & (LHASHTABLE_NUM_SUB_TABLES - 1); /* TODO: maybe change later */
    uint64_t sub_table_ofs = HEADER()->tables_offsets[sub_tbl_idx];
    _lhashtable_resize_table_if_needed(lht, sub_table_ofs);
    uint32_t mask = SUB_TABLE()->mask;
    uint32_t b = h & mask;
    for (;;) {
        uint64_t bkt_ofs = _lhashtable_get_bucket_ofs(lht, sub_table_ofs, b);
        struct lhashtable_bucket *bkt = lht->mem + bkt_ofs;
        if (0 == bkt->data_ofs.u32) {
            size_t n = sizeof(struct lhashtable_record) + key_len + val_len;
            union lhashtable_data_ofs data_ofs;
            _lhashtable_sub_alloc(lht, sub_table_ofs, n, &data_ofs);
            /* re-calc pointer, mem can move after allocating */
            bkt = lht->mem + bkt_ofs;
            bkt->hashcode = h;
            bkt->data_ofs = data_ofs;
            struct lhashtable_record *rec = lht->mem + _lhashtable_data_ofs_to_abs_ofs(SUB_TABLE(), &data_ofs);
            rec->key_len = key_len;
            rec->val_len = val_len;
            memcpy(rec->data, key, key_len);
            memcpy(rec->data + key_len, val, val_len);;
            ++SUB_TABLE()->size;
            return 0;
        }
        ++b;
        if (b > mask)
            b = 0;
    }
    return 0;
}

int lhashtable_insert_str(struct lhashtable *lht, const char *key, const char *val) {
    return lhashtable_insert(lht, key, strlen(key), val, strlen(val) + 1);
}

uint64_t lhashtable_lookup(struct lhashtable *lht, const void *key, size_t key_len) {
    uint32_t h = hashcode(key, key_len);
    uint32_t sub_tbl_idx = (h ^ (h >> 8)) & (LHASHTABLE_NUM_SUB_TABLES - 1); /* TODO: maybe change later */
    uint64_t sub_table_ofs = HEADER()->tables_offsets[sub_tbl_idx];
    uint32_t mask = SUB_TABLE()->mask;
    uint32_t b = h & mask;
    for (;;) {
        uint64_t bkt_ofs = _lhashtable_get_bucket_ofs(lht, sub_table_ofs, b);
        struct lhashtable_bucket *bkt = lht->mem + bkt_ofs;
        if (0 == bkt->data_ofs.u32)
            return 0;
        if (h == bkt->hashcode) {
            uint64_t rec_ofs = _lhashtable_data_ofs_to_abs_ofs(SUB_TABLE(), &bkt->data_ofs);
            struct lhashtable_record *rec = lht->mem + rec_ofs;
            if (key_len == rec->key_len && 0 == memcmp(rec->data, key, key_len))
                return rec_ofs;
        }
        ++b;
        if (b > mask)
            b = 0;
    }
    return 0;
}

static void _lhashtable_fix_chain_down(struct lhashtable *lht, uint64_t sub_table_ofs, uint32_t mask, uint32_t bucket) {
    for(;;) {
        uint64_t bkt_ofs = _lhashtable_get_bucket_ofs(lht, sub_table_ofs, bucket);
        struct lhashtable_bucket *bkt = lht->mem + bkt_ofs;
        if (0 == bkt->data_ofs.u32) /* is end of chain? */
            break;
        uint32_t new_bucket = bkt->hashcode & mask; /* ideal bucket */
        for (;;) {
            if (bucket == new_bucket) /* no need to move */
                break;
            struct lhashtable_bucket *new_bkt = lht->mem + _lhashtable_get_bucket_ofs(lht, sub_table_ofs, new_bucket);
            if (0 == new_bkt->data_ofs.u32) {
                /* found new spot, move the bucket */
                *new_bkt = *bkt;
                bkt->data_ofs.u32 = 0; /* make as deleted */
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

int lhashtable_remove(struct lhashtable *lht, const void *key, size_t key_len) {
    uint32_t h = hashcode(key, key_len);
    uint32_t sub_tbl_idx = (h ^ (h >> 8)) & (LHASHTABLE_NUM_SUB_TABLES - 1); /* TODO: maybe change later */
    uint64_t sub_table_ofs = HEADER()->tables_offsets[sub_tbl_idx];
    uint32_t mask = SUB_TABLE()->mask;
    uint32_t b = h & mask;
    for (;;) {
        uint64_t bkt_ofs = _lhashtable_get_bucket_ofs(lht, sub_table_ofs, b);
        struct lhashtable_bucket *bkt = lht->mem + bkt_ofs;
        if (0 == bkt->data_ofs.u32)
            return -1;
        if (h == bkt->hashcode) {
            uint64_t rec_ofs = _lhashtable_data_ofs_to_abs_ofs(SUB_TABLE(), &bkt->data_ofs);
            struct lhashtable_record *rec = lht->mem + rec_ofs;
            if (key_len == rec->key_len && 0 == memcmp(rec->data, key, key_len)) {
                /* add to freelist */
                size_t n = sizeof(struct lhashtable_record) + key_len + rec->val_len;
                N_ALIGN();
                n >>= LHT_ALLOC_ALIGN_BITS;
                union lhashtable_data_ofs *data_ofs_ptr = lht->mem + rec_ofs;
                *data_ofs_ptr = SUB_TABLE()->freelist[n];
                SUB_TABLE()->freelist[n] = bkt->data_ofs;
                bkt->data_ofs.u32 = 0; /* mark as deleted */
                --SUB_TABLE()->size;
                /* skip the deleted bucket and fix buckets below it */
                ++b;
                if (b > mask)
                    b = 0;
                _lhashtable_fix_chain_down(lht, sub_table_ofs, mask, b);
                return 0;
            }
        }
        ++b;
        if (b > mask)
            b = 0;
    }
    return -1;
}


void lhashtable_dump(struct lhashtable *lht) {
    int i;
    printf("signature: %s, version: %hu, flags: %hhu, write_loc: %llu, capacity: %llu\n",
           HEADER()->signature,
           HEADER()->version,
           HEADER()->flags,
           (unsigned long long)HEADER()->write_loc,
           (unsigned long long)HEADER()->capacity);
    for (i = 0; i < LHASHTABLE_NUM_SUB_TABLES; ++i) {
        struct lhashtable_table *tbl = lht->mem + HEADER()->tables_offsets[i];
        printf("sub table %d\n", i);
        printf("\tmask=%u, size=%u, next_alloc=%u, current_block=%hu\n", tbl->mask, tbl->size, tbl->next_alloc, tbl->current_block);
        /* TODO: print keys and values */
    }
}
