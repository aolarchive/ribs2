#include "lhashtable.h"
#include "hash_funcs.h"
#include "ilog2.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>

const char LHT_SIGNATURE[] = "RIBS_LH";
#define LHT_VERSION 1

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
        if (0 == (LHT_GET_HEADER()->flags & LHT_FLAG_FIN))
            LOGGER_ERROR("dirty table detected");
        LHT_GET_HEADER()->flags &= ~LHT_FLAG_FIN;
        return 0;
    }
    unlink(filename);
    lht->fd = open(filename, O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (0 > lht->fd)
        return LOGGER_PERROR(filename), -1;
    /* initial size */
    size_t size = sizeof(struct lhashtable_header);
    _LHT_ALIGN_P2(size, LHT_BLOCK_SIZE);
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
    strncpy(LHT_GET_HEADER()->signature, LHT_SIGNATURE, sizeof(LHT_GET_HEADER()->signature));
    LHT_GET_HEADER()->version = LHT_VERSION;
    LHT_GET_HEADER()->flags = 0;
    memset(LHT_GET_HEADER()->tables_offsets, 0, sizeof(LHT_GET_HEADER()->tables_offsets));
    LHT_GET_HEADER()->num_data_blocks = 4096; /* TODO: make it configurable */
    int i;
    for (i = 0; i < LHT_NUM_SUB_TABLES; ++i) {
        uint64_t sub_table_ofs = _lhashtable_alloc_ofs(lht, sizeof(struct lhashtable_table) + sizeof(uint64_t) * (LHT_GET_HEADER()->num_data_blocks + 1));
        LHT_GET_HEADER()->tables_offsets[i] = sub_table_ofs;
        memset(LHT_GET_SUB_TABLE()->buckets_offsets, 0, sizeof(LHT_GET_SUB_TABLE()->buckets_offsets));
        memset(LHT_GET_SUB_TABLE()->freelist, 0, sizeof(LHT_GET_SUB_TABLE()->freelist));
        memset(LHT_GET_SUB_TABLE()->data_start_ofs, 0, sizeof(uint64_t) * (LHT_GET_HEADER()->num_data_blocks + 1)); /* 0 is reserved */
        LHT_GET_SUB_TABLE()->mask = LHT_SUB_TABLE_INITIAL_SIZE - 1;
        LHT_GET_SUB_TABLE()->size = 0;
        LHT_GET_SUB_TABLE()->next_alloc = LHT_BLOCK_SIZE; /* 0 is reserved ==> set to full */
        LHT_GET_SUB_TABLE()->current_block = 0;
        LHT_GET_SUB_TABLE()->buckets_offsets[0] = _lhashtable_alloc_ofs(lht, sizeof(struct lhashtable_bucket) * LHT_SUB_TABLE_INITIAL_SIZE);
    }
    return 0;
}

int lhashtable_close(struct lhashtable *lht) {
    if (lht->fd < 0)
        return 0;
    LHT_GET_HEADER()->flags |= LHT_FLAG_FIN;
    if (0 > close(lht->fd))
        LOGGER_ERROR("failed to close file (fd=%d)", lht->fd);
    lht->fd = -1;
    if (0 > munmap(lht->mem, LHT_GET_HEADER()->capacity))
        LOGGER_ERROR("failed to unmap file (addr=%p)", lht->mem);
    lht->mem = NULL;
    return 0;
}

int lhashtable_insert(struct lhashtable *lht, const void *key, size_t key_len, const void *val, size_t val_len) {
    uint32_t h = hashcode(key, key_len);
    uint32_t sub_tbl_idx = LHT_SUB_TABLE_HASH(h);
    uint64_t sub_table_ofs = LHT_GET_HEADER()->tables_offsets[sub_tbl_idx];
    _lhashtable_resize_table_if_needed(lht, sub_table_ofs);
    uint32_t mask = LHT_GET_SUB_TABLE()->mask;
    uint32_t b = h & mask;
    for (;;) {
        uint64_t bkt_ofs = _lhashtable_get_bucket_ofs(lht, sub_table_ofs, b);
        if (0 == ((struct lhashtable_bucket *)(lht->mem + bkt_ofs))->data_ofs.u32) {
            size_t n = sizeof(struct lhashtable_record) + key_len + val_len;
            union lhashtable_data_ofs data_ofs;
            _lhashtable_sub_alloc(lht, sub_table_ofs, n, &data_ofs);
            struct lhashtable_bucket *bkt = lht->mem + bkt_ofs;
            bkt->hashcode = h;
            bkt->data_ofs = data_ofs;
            struct lhashtable_record *rec = lht->mem + _lhashtable_data_ofs_to_abs_ofs(LHT_GET_SUB_TABLE(), &data_ofs);
            rec->key_len = key_len;
            rec->val_len = val_len;
            memcpy(rec->data, key, key_len);
            memcpy(rec->data + key_len, val, val_len);;
            ++LHT_GET_SUB_TABLE()->size;
            return 0;
        }
        ++b;
        if (b > mask)
            b = 0;
    }
    return 0;
}

int lhashtable_insert_or_update(struct lhashtable *lht, const void *key, size_t key_len, const void *val, size_t val_len) {
    uint32_t h = hashcode(key, key_len);
    uint32_t sub_tbl_idx = LHT_SUB_TABLE_HASH(h);
    uint64_t sub_table_ofs = LHT_GET_HEADER()->tables_offsets[sub_tbl_idx];
    _lhashtable_resize_table_if_needed(lht, sub_table_ofs);
    uint32_t mask = LHT_GET_SUB_TABLE()->mask;
    uint32_t b = h & mask;
    for (;;) {
        uint64_t bkt_ofs = _lhashtable_get_bucket_ofs(lht, sub_table_ofs, b);
        if (0 == ((struct lhashtable_bucket *)(lht->mem + bkt_ofs))->data_ofs.u32) {
            size_t n = sizeof(struct lhashtable_record) + key_len + val_len;
            union lhashtable_data_ofs data_ofs;
            _lhashtable_sub_alloc(lht, sub_table_ofs, n, &data_ofs);
            struct lhashtable_bucket *bkt = lht->mem + bkt_ofs;
            bkt->hashcode = h;
            bkt->data_ofs = data_ofs;
            struct lhashtable_record *rec = lht->mem + _lhashtable_data_ofs_to_abs_ofs(LHT_GET_SUB_TABLE(), &data_ofs);
            rec->key_len = key_len;
            rec->val_len = val_len;
            memcpy(rec->data, key, key_len);
            memcpy(rec->data + key_len, val, val_len);;
            ++LHT_GET_SUB_TABLE()->size;
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
    uint32_t sub_tbl_idx = LHT_SUB_TABLE_HASH(h);
    uint64_t sub_table_ofs = LHT_GET_HEADER()->tables_offsets[sub_tbl_idx];
    uint32_t mask = LHT_GET_SUB_TABLE()->mask;
    uint32_t b = h & mask;
    for (;;) {
        uint64_t bkt_ofs = _lhashtable_get_bucket_ofs(lht, sub_table_ofs, b);
        struct lhashtable_bucket *bkt = lht->mem + bkt_ofs;
        if (0 == bkt->data_ofs.u32)
            return 0;
        if (h == bkt->hashcode) {
            uint64_t rec_ofs = _lhashtable_data_ofs_to_abs_ofs(LHT_GET_SUB_TABLE(), &bkt->data_ofs);
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

const char *lhashtable_lookup_str(struct lhashtable *lht, const char *key) {
    uint64_t rec_ofs = lhashtable_lookup(lht, key, strlen(key));
    if (0 == rec_ofs)
        return NULL;
    return lhashtable_get_val(lht, rec_ofs);
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
    uint32_t sub_tbl_idx = LHT_SUB_TABLE_HASH(h);
    uint64_t sub_table_ofs = LHT_GET_HEADER()->tables_offsets[sub_tbl_idx];
    uint32_t mask = LHT_GET_SUB_TABLE()->mask;
    uint32_t b = h & mask;
    for (;;) {
        uint64_t bkt_ofs = _lhashtable_get_bucket_ofs(lht, sub_table_ofs, b);
        struct lhashtable_bucket *bkt = lht->mem + bkt_ofs;
        if (0 == bkt->data_ofs.u32)
            return -1;
        if (h == bkt->hashcode) {
            uint64_t rec_ofs = _lhashtable_data_ofs_to_abs_ofs(LHT_GET_SUB_TABLE(), &bkt->data_ofs);
            struct lhashtable_record *rec = lht->mem + rec_ofs;
            if (key_len == rec->key_len && 0 == memcmp(rec->data, key, key_len)) {
                /* add to freelist */
                size_t n = sizeof(struct lhashtable_record) + key_len + rec->val_len;
                LHT_N_ALIGN();
                n >>= LHT_ALLOC_ALIGN_BITS;
                union lhashtable_data_ofs *data_ofs_ptr = lht->mem + rec_ofs;
                *data_ofs_ptr = LHT_GET_SUB_TABLE()->freelist[n];
                LHT_GET_SUB_TABLE()->freelist[n] = bkt->data_ofs;
                bkt->data_ofs.u32 = 0; /* mark as deleted */
                --LHT_GET_SUB_TABLE()->size;
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

int lhashtable_remove_str(struct lhashtable *lht, const char *key) {
    return lhashtable_remove(lht, key, strlen(key));
}

void lhashtable_dump(struct lhashtable *lht) {
    int i;
    printf("signature: %s, version: %hu, flags: %hhu, write_loc: %llu, capacity: %llu\n",
           LHT_GET_HEADER()->signature,
           LHT_GET_HEADER()->version,
           LHT_GET_HEADER()->flags,
           (unsigned long long)LHT_GET_HEADER()->write_loc,
           (unsigned long long)LHT_GET_HEADER()->capacity);
    for (i = 0; i < LHT_NUM_SUB_TABLES; ++i) {
        struct lhashtable_table *tbl = lht->mem + LHT_GET_HEADER()->tables_offsets[i];
        printf("sub table %d\n", i);
        printf("\tmask=%u, size=%u, next_alloc=%u, current_block=%hu\n", tbl->mask, tbl->size, tbl->next_alloc, tbl->current_block);
        /* TODO: print keys and values */
    }
}
