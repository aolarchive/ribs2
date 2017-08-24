/*
    This file is part of RIBS2.0 (Robust Infrastructure for Backend Systems).
    RIBS is an infrastructure for building great SaaS applications (but not
    limited to).

    Copyright (C) 2017 Adap.tv, Inc.

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
#include "lshashtable.h"
#include "hash_funcs.h"
#include <dirent.h>


#define RS_BLOCK_SIZE_BITS  3
#define RS_BLOCK_SIZE (1ULL << RS_BLOCK_SIZE_BITS)
#define NUM_BITS_TO_BYTES(x) (((uint64_t)(x)) >> 3ULL)
#define MIN_CAPACITY (4096ULL << RS_BLOCK_SIZE_BITS << 3)
#define LSHT_INITIAL_NUM_ENTRIES 4096

/* used as safety zone to avoid reading record that was just relocated
   and the other process is currently overwriting */
#define MIN_FREE_SIZE (10*1024*1024)

#define _FNAME_PREFIX "store_"
SSTRL(FNAME_PREFIX, _FNAME_PREFIX);

enum store_type {
    RINGFILE,
    RINGBUF,
};


#define NEXT_BUCKET(m,b) b += (unlikely(b == m)) ? -b : 1;


struct lsht_record_store_header {
};

struct lsht_record_store_rec {
    uint32_t num_blocks;
    char data[];
};

struct lsht_record_store {
    union {
        struct ringfile file;
        struct ringbuf  buf;
    } data;
    struct ringbuf bv_block_map;
    size_t num_blocks;
    size_t num_free_blocks;
    size_t min_free_size;
    struct vmbuf recbuf;
    enum store_type type;
};

#define LSHT_RECORD_STORE_INITIALIZER_FILE { .data.file = RINGFILE_INITIALIZER, RINGBUF_INITIALIZER, 0, 0, 0, VMBUF_INITIALIZER, RINGFILE }
#define LSHT_RECORD_STORE_INITIALIZER_BUF { .data.buf = RINGBUF_INITIALIZER, RINGBUF_INITIALIZER, 0, 0, 0, VMBUF_INITIALIZER, RINGBUF }
#define LSHT_RECORD_STORE_INIT_VAR_FILE(x) (*(x)) = ((struct lsht_record_store)LSHT_RECORD_STORE_INITIALIZER_FILE)
#define LSHT_RECORD_STORE_INIT_VAR_BUF(x) (*(x)) = ((struct lsht_record_store)LSHT_RECORD_STORE_INITIALIZER_BUF)


void rs_rseek(struct lsht_record_store *rs, size_t by);
void rs_wseek(struct lsht_record_store *rs, size_t by);
void *rs_wloc(struct lsht_record_store *rs);
void *rs_rloc(struct lsht_record_store *rs);
size_t rs_wlocpos(struct lsht_record_store *rs);
size_t rs_rlocpos(struct lsht_record_store *rs);
int rs_empty(struct lsht_record_store *rs);
size_t rs_capacity(struct lsht_record_store *rs);
size_t rs_avail(struct lsht_record_store *rs);
void *rs_data(struct lsht_record_store *rs);

int lsht_record_store_init(struct lsht_record_store *rs, const char *filename, size_t capacity);
void *lsht_record_store_alloc(struct lshashtable *lsht, struct lsht_record_store *rs, size_t data_size);
void lsht_record_store_del(struct lsht_record_store *rs, struct lshashtable_rec *rec);
int lsht_record_store_foreach(struct lsht_record_store *rs, int (*callback)(void *, void *), void *arg);
int lsht_record_store_foreach_all(struct lsht_record_store *rs, int (*callback)(struct lsht_record_store *, struct lsht_record_store_rec *));


static inline size_t lsht_record_store_num_free_blocks(struct lsht_record_store *rs) {
    return rs->num_free_blocks;
}

static inline size_t lsht_record_store_total_blocks(struct lsht_record_store *rs) {
    return rs->num_blocks;
}

static int lshashtable_create_store(struct lshashtable *lsht, struct lsht_record_store **rs_ret);
static inline struct lsht_record_store *lshashtable_get_rs_by_id(struct lshashtable *lsht, uint16_t rs_id);
static int _lshashtable_update_rec_ptr(struct lshashtable *lsht, struct lshashtable_rec *rec, uint16_t rs_id);
static inline size_t lsht_record_store_avail(struct lsht_record_store *rs);
static inline void lshashtable_make_room(struct lshashtable *lsht, struct lsht_record_store *rs, size_t size);
static inline int lshashtable_del_key_index(struct lshashtable *lsht, const void *key, size_t key_size, struct lshashtable_entry *lsht_entry);
static inline int lshashtable_del_key_no_log(struct lshashtable *lsht, const void *key, size_t key_size);
static inline int lshashtable_add_index(struct lshashtable *lsht, struct lshashtable_rec *rec, uint16_t rs_id);

int lsht_record_store_init(struct lsht_record_store *rs, const char *filename, size_t capacity) {
    capacity = (capacity + MIN_CAPACITY - 1) &~ (MIN_CAPACITY - 1);
    rs->num_blocks = rs->num_free_blocks = capacity >> RS_BLOCK_SIZE_BITS;
    rs->min_free_size = capacity / 2 < MIN_FREE_SIZE ? capacity >> 4 : MIN_FREE_SIZE;

    if (RINGBUF == rs->type) {
        if (0 > ringbuf_init(&rs->data.buf, capacity)){
            return LOGGER_ERROR_FUNC("failed to init ringbuf: %zu bytes", capacity), -1;
        }
    } else {
        if (0 > ringfile_init_safe_resize(&rs->data.file, filename, capacity, sizeof(struct lsht_record_store_header))){
            return LOGGER_ERROR_FUNC("record store data file init failed: %s", filename), -1;
        }
    }

    size_t bv_size_bytes = NUM_BITS_TO_BYTES(rs->num_blocks);

    if (0 > ringbuf_init(&rs->bv_block_map, bv_size_bytes))
        return LOGGER_ERROR_FUNC("failed to init ringbuf: %zu bytes", bv_size_bytes), -1;

    return 0;
}


static inline size_t _lsht_record_store_pos2blockpos(size_t pos) {
    return pos >> RS_BLOCK_SIZE_BITS;
}

static inline void _lsht_record_store_set_use(struct lsht_record_store *rs, size_t rec_ofs) {
    uint64_t *bv = ringbuf_mem(&rs->bv_block_map);
    uint64_t *p = bv + (rec_ofs >> 6);
    *p |= 1ULL << (rec_ofs & 0x3F);
}

static inline int _lsht_record_store_is_inuse(struct lsht_record_store *rs, size_t rec_ofs) {
    uint64_t *bv = ringbuf_mem(&rs->bv_block_map);
    uint64_t *p = bv + (rec_ofs >> 6);
    return (*p & 1ULL << (rec_ofs & 0x3F)) > 0;
}

static inline void _lsht_record_store_clear_use(struct lsht_record_store *rs, size_t rec_ofs) {
    uint64_t *bv = ringbuf_mem(&rs->bv_block_map);
    uint64_t *p = bv + (rec_ofs >> 6);
    *p &= ~(1ULL << (rec_ofs & 0x3F));
}

static inline size_t _rec_size(struct lsht_record_store_rec *r) {
    return r->num_blocks << RS_BLOCK_SIZE_BITS;
}

static inline struct lsht_record_store_rec *lsht_record_store_oldest_rec(struct lsht_record_store *rs) {
    return rs_rloc(rs);
}

static inline struct lsht_record_store_rec *lsht_record_store_pop_oldest_rec(struct lsht_record_store *rs) {
    struct lsht_record_store_rec *r = lsht_record_store_oldest_rec(rs);
    rs->num_free_blocks += r->num_blocks;
    rs_rseek(rs, _rec_size(r));
    return r;
}

/* clone rec WITHOUT invoking lshashtable_make_room, hence no moving old records to next level */
static inline struct lsht_record_store_rec *lsht_record_store_clone_no_move(struct lsht_record_store *rs, struct lsht_record_store_rec *rec) {
    size_t size = _rec_size(rec);
    if (lsht_record_store_avail(rs) < size)
        return NULL;
    struct lsht_record_store_rec *newrec = rs_wloc(rs);
    _lsht_record_store_set_use(rs, _lsht_record_store_pos2blockpos(rs_wlocpos(rs)));
    memcpy(newrec, rec, size);
    rs_wseek(rs, size);
    rs->num_free_blocks -= newrec->num_blocks;
    return newrec;
}

static inline struct lsht_record_store_rec *lsht_record_store_clone(struct lshashtable *lsht, struct lsht_record_store *rs, struct lsht_record_store_rec *rec) {
    size_t size = _rec_size(rec);
    lshashtable_make_room(lsht, rs, size + rs->min_free_size);
    if (lsht_record_store_avail(rs) < size)
        return NULL;
    struct lsht_record_store_rec *newrec = rs_wloc(rs);
    _lsht_record_store_set_use(rs, _lsht_record_store_pos2blockpos(rs_wlocpos(rs)));
    memcpy(newrec, rec, size);
    rs_wseek(rs, size);
    rs->num_free_blocks -= newrec->num_blocks;
    return newrec;
}

static inline uint16_t lshashtable_get_rs_id(struct lshashtable *lsht, struct lsht_record_store *rs) {
    struct lsht_record_store *rs_begin = (struct lsht_record_store *)vmbuf_data(&lsht->record_stores);
    return rs - rs_begin;
}

static inline void *_next_rec(void *r) {
    return r + (((struct lsht_record_store_rec *)r)->num_blocks << RS_BLOCK_SIZE_BITS);
}

static inline int lshashtable_is_expired(struct lshashtable *lsht, struct lshashtable_rec *rec) {
    return rec->val_size != UINT_MAX && lsht->is_expired(rec);
}

static inline void lshashtable_make_room(struct lshashtable *lsht, struct lsht_record_store *rs, size_t size) {
    uint64_t num_deleted_keys = 0;
    struct vmbuf *recs = &rs->recbuf;
    vmbuf_init(recs, 4096);
    uint16_t rs_id = lshashtable_get_rs_id(lsht, rs), next_rs_id = rs_id + 1;
    size_t num_free_blocks_threshold = (size >> RS_BLOCK_SIZE_BITS) + (lsht_record_store_total_blocks(rs) >> 2);
    while (lsht_record_store_avail(rs) < size) {
        /* reclaim space of deleted records */
        while (!rs_empty(rs)) {
            struct lsht_record_store_rec *r = lsht_record_store_oldest_rec(rs);
            if (_lsht_record_store_is_inuse(rs, _lsht_record_store_pos2blockpos(rs_rlocpos(rs)))) {
                struct lshashtable_rec *rec = (struct lshashtable_rec *)r->data;
                /* if the record is the special record for deleted key and in the last store, we can safely discard it */
                if (UINT_MAX != rec->val_size || next_rs_id != lsht->next_rs_id) {
                    if (lshashtable_is_expired(lsht, rec)) {
                        lshashtable_del_key_index(lsht, rec->data, rec->key_size, NULL);
                        lsht_record_store_del(rs, rec);
                    } else
                        break;
                }
            }
            rs_rseek(rs, _rec_size(r));
        }
        if (lsht_record_store_avail(rs) >= size)
            break;
        /* sanity check */
        if (rs_empty(rs)) {
            LOGGER_ERROR("requested size is larger than configured capacity: %zu > %zu, aborting...", size, rs_capacity(rs));
            abort();
        }
        struct lsht_record_store_rec *r = lsht_record_store_pop_oldest_rec(rs);
        if (lsht_record_store_num_free_blocks(rs) > num_free_blocks_threshold) { /* relocate */
            size_t rec_size = _rec_size(r);
            vmbuf_memcpy(recs, r, rec_size);
            size += rec_size;
        } else {/* evict or delete LRU */
            struct lshashtable_rec *rec = (struct lshashtable_rec *)r->data;
            if (lshashtable_is_expired(lsht, rec)) {
                if (UINT_MAX != rec->val_size){
                    lshashtable_del_key_index(lsht, rec->data, rec->key_size, NULL);
                }
                continue;
            }
            struct lsht_record_store *next_rs;
            if (RINGFILE == rs->type) { //evict
                if (unlikely(next_rs_id == lsht->next_rs_id)) {
                    if (0 > lshashtable_create_store(lsht, &next_rs)) {
                        LOGGER_ERROR("failed to create new store");
                        abort();
                    }
                } else {
                    next_rs = lshashtable_get_rs_by_id(lsht, next_rs_id);
                }
                _lshashtable_update_rec_ptr(lsht, (struct lshashtable_rec *)lsht_record_store_clone(lsht, next_rs, r)->data, next_rs_id);
            } else { // delete
                lshashtable_del_key_index(lsht, rec->data, rec->key_size, NULL);
                ++num_deleted_keys;
            }

        }
    }

    struct lsht_record_store_rec *r = (struct lsht_record_store_rec *)vmbuf_data(recs), *rend = (struct lsht_record_store_rec *)vmbuf_wloc(recs);
    for (; r != rend; r = _next_rec(r)) {
        /* we can safely call lsht_record_store_clone_no_move since we free'd enough space in our rs */
        struct lsht_record_store_rec *newrec = lsht_record_store_clone_no_move(rs, r);
        _lshashtable_update_rec_ptr(lsht, (struct lshashtable_rec *)newrec->data, rs_id);
    }
    if (num_deleted_keys)
        LOGGER_INFO("make room:%ld keys deleted", num_deleted_keys);
}

static inline size_t lsht_record_store_avail(struct lsht_record_store *rs) {
    return rs_avail(rs);
}

void *lsht_record_store_alloc(struct lshashtable *lsht, struct lsht_record_store *rs, size_t data_size) {
    size_t total_size = sizeof(struct lsht_record_store_rec) + data_size;
    total_size = (total_size + RS_BLOCK_SIZE - 1) & ~(RS_BLOCK_SIZE - 1);
    lshashtable_make_room(lsht, rs, total_size + rs->min_free_size);
    if (lsht_record_store_avail(rs) < total_size){
        LOGGER_ERROR("failed to allocate new record");
        abort();
    }
    struct lsht_record_store_rec *r = rs_wloc(rs);
    _lsht_record_store_set_use(rs, _lsht_record_store_pos2blockpos(rs_wlocpos(rs)));
    r->num_blocks = total_size >> RS_BLOCK_SIZE_BITS;
    rs_wseek(rs, total_size);
    rs->num_free_blocks -= r->num_blocks;
    return r->data;
}

void lsht_record_store_set_use(struct lsht_record_store *rs, struct lsht_record_store_rec *r) {
    rs->num_free_blocks -= r->num_blocks;
    _lsht_record_store_set_use(rs, _lsht_record_store_pos2blockpos((void *)r - rs_data(rs)));
}

void lsht_record_store_del(struct lsht_record_store *rs, struct lshashtable_rec *rec) {
    struct lsht_record_store_rec *r = (void *)rec - offsetof(struct lsht_record_store_rec, data);
    rs->num_free_blocks += r->num_blocks;
    _lsht_record_store_clear_use(rs, _lsht_record_store_pos2blockpos((void *)r - rs_data(rs)));
}

void lsht_record_store_dump(struct lsht_record_store *rs) {
    size_t num_blocks = rs_capacity(rs) >> RS_BLOCK_SIZE_BITS;
    size_t i;
    for (i = 0; i < num_blocks; ++i) {
        if (0 == (i & 63))
            printf("\n%08zx: ", i);
        printf("%d", _lsht_record_store_is_inuse(rs, i) ? 1 : 0);
    }
    printf("\n");
}

int lsht_record_store_foreach(struct lsht_record_store *rs, int (*callback)(void *, void *), void *arg) {
    void *it = rs_rloc(rs), *end = rs_wloc(rs);
    void *start = rs_data(rs);
    int res;
    while (it != end) {
        struct lsht_record_store_rec *r = it;
        if (_lsht_record_store_is_inuse(rs, _lsht_record_store_pos2blockpos(it - start)) && 0 > (res = callback(r->data, arg)))
            return res;
        it += r->num_blocks << RS_BLOCK_SIZE_BITS;
    }
    return 0;
}

int lsht_record_store_foreach_all(struct lsht_record_store *rs, int (*callback)(struct lsht_record_store *, struct lsht_record_store_rec *)) {
    void *it = rs_rloc(rs), *end = rs_wloc(rs);
    int res;
    while (it != end) {
        struct lsht_record_store_rec *r = it;
        if (0 > (res = callback(rs, r)))
            return res;
        it += r->num_blocks << RS_BLOCK_SIZE_BITS;
    }
    return 0;
}

static inline void _lshashtable_move_buckets_range(struct lshashtable *lsht, uint32_t new_mask, uint32_t begin, uint32_t end) {
    struct lshashtable_entry *entries = (struct lshashtable_entry *)vmbuf_data(&lsht->index);
    for (; begin < end; ++begin) {
        struct lshashtable_entry *e = entries + begin;
        uint32_t new_bkt_idx = e->hashcode & new_mask;
        /* if already in the right place skip it */
        if (begin == new_bkt_idx)
            continue;
        void *rec = e->rec;
        uint16_t rs_id = e->rs_id;
        /* free the bucket so it can be reused */
        e->rec = NULL;
        for (;;) {
            struct lshashtable_entry *new_bucket = entries + new_bkt_idx;
            if (NULL == new_bucket->rec) {
                new_bucket->hashcode = e->hashcode;
                new_bucket->rec = rec;
                new_bucket->rs_id = rs_id;
                break;
            }
            NEXT_BUCKET(new_mask, new_bkt_idx)
        }
    }
}

static inline int _lshashtable_check_resize(struct lshashtable *lsht) {
    if (likely(lsht->size < (lsht->mask >> 1)))
        return 0;

    if (UINT_MAX == lsht->mask){
         LOGGER_ERROR("table reached max bucket size/capacity (%u/%llu)", lsht->size, lsht->mask + 1ULL);
         abort();
    }

    uint32_t new_mask = (INT_MAX == lsht->mask) ? UINT_MAX : ((lsht->mask + 1) << 1) - 1;

    LOGGER_INFO("resizing... %u ==> %llu", lsht->mask + 1, new_mask + 1ULL);

    vmbuf_resize_if_less(&lsht->index, (new_mask + 1ULL)  * sizeof(struct lshashtable_entry));
    struct lshashtable_entry *entries = (struct lshashtable_entry *)vmbuf_data(&lsht->index), *e;
    for (e = entries; e->rec; ++e);
    uint32_t b = e - entries;
    _lshashtable_move_buckets_range(lsht, new_mask, b, lsht->mask + 1);
    _lshashtable_move_buckets_range(lsht, new_mask, 0, b);
    lsht->mask = new_mask;
    return 1;
}

static inline struct lsht_record_store *lshashtable_get_rs(struct lshashtable *lsht) {
    return (struct lsht_record_store *)vmbuf_data(&lsht->record_stores);
}

static inline struct lsht_record_store *lshashtable_get_rs_by_id(struct lshashtable *lsht, uint16_t rs_id) {
    return ((struct lsht_record_store *)vmbuf_data(&lsht->record_stores)) + rs_id;
}

static inline int _lshashtable_key_equals(struct lshashtable_rec *rec, const void *key, size_t key_size) {
    return (key_size == rec->key_size && 0 == memcmp(key, rec->data, key_size));
}

static int lshashtable_create_store(struct lshashtable *lsht, struct lsht_record_store **rs_ret) {
    char filename[4096];
    if (PATH_MAX <= snprintf(filename, PATH_MAX, "%s/%s%d.dat", lsht->basedir, FNAME_PREFIX, lsht->next_rs_id))
        return LOGGER_ERROR("filename too long: %s/%s%d.dat", lsht->basedir, FNAME_PREFIX, lsht->next_rs_id), -1;

    struct lsht_record_store *rs = vmbuf_allocptr(&lsht->record_stores, sizeof(struct lsht_record_store));

    if (!lsht->basedir)
        LSHT_RECORD_STORE_INIT_VAR_BUF(rs);
    else
        LSHT_RECORD_STORE_INIT_VAR_FILE(rs);


    if (0 > lsht_record_store_init(rs, filename, lsht->capacity))
        return -1;
    ++lsht->next_rs_id;
    if (rs_ret)
        *rs_ret = rs;
    return 0;
}

static int _lshashtable_update_rec_ptr(struct lshashtable *lsht, struct lshashtable_rec *rec, uint16_t rs_id) {
    if (UINT_MAX == rec->val_size)
        return 0;
    uint32_t mask = lsht->mask;
    struct lshashtable_entry *entries = (struct lshashtable_entry *)vmbuf_data(&lsht->index);
    uint32_t hc = hashcode2(rec->data, rec->key_size);
    uint32_t index = hc & mask;
    for (;;) {
        struct lshashtable_entry *e = entries + index;
        if (!e->rec)
            return LOGGER_ERROR("key not found: [%.*s] [%u:%u]", rec->key_size, rec->data, hc, index), -1;
        if (e->hashcode == hc) {
            struct lshashtable_rec *_rec = e->rec;
            if (_lshashtable_key_equals(_rec, rec->data, rec->key_size)) {
                e->rec = rec;
                e->rs_id = rs_id;
                return 0;
            }
        }
        NEXT_BUCKET(mask, index)
    }
}

static inline int lshashtable_add_index(struct lshashtable *lsht, struct lshashtable_rec *rec, uint16_t rs_id) {
    _lshashtable_check_resize(lsht);
    uint32_t mask = lsht->mask;
    struct lshashtable_entry *entries = (struct lshashtable_entry *)vmbuf_data(&lsht->index);
    uint32_t hc = hashcode2(rec->data, rec->key_size);
    uint32_t index = hc & mask;
    for (;;) {
        struct lshashtable_entry *e = entries + index;
        if (!e->rec) {
            e->rec = rec;
            e->hashcode = hc;
            e->rs_id = rs_id;
            ++lsht->size;
            return 0;
        }
        if (e->hashcode == hc) {
            struct lshashtable_rec *oldrec = e->rec;
            if (_lshashtable_key_equals(oldrec, rec->data, rec->key_size)) {
                lsht_record_store_del(lshashtable_get_rs_by_id(lsht, e->rs_id), oldrec);
                e->rec = rec;
                e->rs_id = rs_id;
                return 0;
            }
        }
        NEXT_BUCKET(mask, index)
    }
    return 0;
}

static int _lshashtable_load(struct lshashtable *lsht) {
    if (!lsht->basedir)
        return 1;

    size_t total = 0;
    uint16_t rs_id = 0;

    int rebuild_ht(struct lsht_record_store *rs, struct lsht_record_store_rec *r) {
        struct lshashtable_rec *rec = (struct lshashtable_rec *)r->data;
        if (rec->val_size == UINT_MAX) {
            if (lshashtable_del_key_no_log(lsht, rec->data, rec->key_size))
                lsht_record_store_set_use(rs, r);
        } else {
            if (lshashtable_is_expired(lsht, rec))
                return 0;
            lsht_record_store_set_use(rs, r);
            lshashtable_add_index(lsht, rec, rs_id);
        }
        return 0;
    }
    char filename[4096];
    for (;;++rs_id) {
        if (PATH_MAX <= snprintf(filename, PATH_MAX, "%s/%s%d.dat", lsht->basedir, FNAME_PREFIX, rs_id))
            return LOGGER_ERROR("filename too long: %s/%s%d.dat", lsht->basedir, FNAME_PREFIX, rs_id), -1;
        if (0 > access(filename, F_OK))
            break;
        LOGGER_INFO("loading record store %hu", rs_id);
        struct lsht_record_store *rs;
        if (0 > lshashtable_create_store(lsht, &rs))
            return -1;

    }
    uint16_t max_rs_id = rs_id;
    while (rs_id > 0) {
        --rs_id;
        struct lsht_record_store *rs = lshashtable_get_rs_by_id(lsht, rs_id);
        LOGGER_INFO("record store %hu: rebuilding index...", rs_id);
        lsht_record_store_foreach_all(rs, rebuild_ht);
        LOGGER_INFO("record store %hu: rebuilding index...done %zu", rs_id, total);
    }
    return max_rs_id == 0;
}


int lshashtable_init(struct lshashtable *lsht, const char *basedir, size_t capacity) {

    if (0 == capacity) {
        capacity = sysconf(_SC_PHYS_PAGES);
        capacity *= sysconf(_SC_PAGESIZE);
        capacity >>= 1;
    }
    if (basedir)
        lsht->basedir = strdup(basedir);
    else
        lsht->basedir = NULL;

    lsht->capacity = capacity;
    lsht->size = 0;
    lsht->next_rs_id = 0;
    vmbuf_init(&lsht->record_stores, 4096);
    vmbuf_init(&lsht->index, LSHT_INITIAL_NUM_ENTRIES * sizeof(struct lshashtable_entry));
    lsht->mask = LSHT_INITIAL_NUM_ENTRIES - 1;

    int res = _lshashtable_load(lsht);
    if (0 >= res)
        return res;

    if (0 > lshashtable_create_store(lsht, NULL))
        return -1;

    return 0;
}

int lshashtable_close(struct lshashtable *lsht) {
    if (lsht->basedir)
        free(lsht->basedir);
    vmbuf_free(&lsht->record_stores);
    return 0;
}

static inline struct lshashtable_rec *_lshashtable_new_rec(struct lshashtable *lsht, const void *key, size_t key_size, size_t val_size) {
    struct lsht_record_store *rs = lshashtable_get_rs(lsht);
    struct lshashtable_rec *rec = lsht_record_store_alloc(lsht, rs, sizeof(struct lshashtable_rec) + key_size + val_size);
    rec->key_size = key_size;
    rec->val_size = val_size;
    memcpy(rec->data, key, key_size);
    return rec;
}

static inline void *_update_entry(struct lshashtable_entry *e, uint32_t hc, size_t key_size, struct lshashtable_rec *rec) {
    e->rec = rec;
    e->hashcode = hc;
    e->rs_id = 0;
    return rec->data + key_size;
}

void *lshashtable_put_key(struct lshashtable *lsht, const void *key, size_t key_size, size_t val_size, struct lshashtable_val *oldval) {
    struct lshashtable_rec *newrec = _lshashtable_new_rec(lsht, key, key_size, val_size);
    uint32_t mask = lsht->mask;
    struct lshashtable_entry *entries = (struct lshashtable_entry *)vmbuf_data(&lsht->index);
    uint32_t hc = hashcode2(key, key_size);
    uint32_t index = hc & mask;
    for (;;) {
        struct lshashtable_entry *e = entries + index;
        if (!e->rec) {
            /* new record */
            if (_lshashtable_check_resize(lsht)) {
                entries = (struct lshashtable_entry *)vmbuf_data(&lsht->index);
                mask = lsht->mask;
                index = hc & mask;
                continue;
            }
            if (oldval) {
                oldval->val_size = 0;
                oldval->val = NULL;
            }
            ++lsht->size;
            return _update_entry(e, hc, key_size, newrec);
        }
        if (e->hashcode == hc) {
            struct lshashtable_rec *rec = e->rec;
            if (_lshashtable_key_equals(rec, key, key_size)) {
                lsht_record_store_del(lshashtable_get_rs_by_id(lsht, e->rs_id), rec);
                if (oldval) {
                    if (lshashtable_is_expired(lsht, rec)) {
                        oldval->val_size = 0;
                        oldval->val = NULL;
                    } else {
                        oldval->val_size = rec->val_size;
                        oldval->val = rec->data + rec->key_size;
                    }
                }
                return _update_entry(e, hc, key_size, newrec);
            }
        }
        NEXT_BUCKET(mask, index)
    }
}

static inline void _lshashtable_fix_chain_down(struct lshashtable *lsht, uint32_t mask, uint32_t index) {
    struct lshashtable_entry *entries = (struct lshashtable_entry *)vmbuf_data(&lsht->index);
    for(;;) {
        struct lshashtable_entry *e = entries + index;
        if (NULL == e->rec) /* is end of chain? */
            break;
        uint32_t new_index = e->hashcode & mask; /* ideal bucket */
        for (;;) {
            if (index == new_index) /* no need to move */
                break;
            struct lshashtable_entry *new_entry = entries + new_index;
            if (NULL == new_entry->rec) {
                /* found new spot, move the bucket */
                *new_entry = *e;
                e->rec = NULL; /* mark as deleted */
                break;
            }
            ++new_index;
            if (unlikely(new_index > mask))
                new_index = 0;
        }
        NEXT_BUCKET(mask, index)
    }
}

int lshashtable_find_key(struct lshashtable *lsht, const void *key, size_t key_size, struct lshashtable_val *val) {
    uint32_t mask = lsht->mask;
    struct lshashtable_entry *entries = (struct lshashtable_entry *)vmbuf_data(&lsht->index);
    uint32_t hc = hashcode2(key, key_size);
    uint32_t index = hc & mask;
    for (;;) {
        struct lshashtable_entry *e = entries + index;
        if (!e->rec)
            return 0;
        if (e->hashcode == hc) {
            struct lshashtable_rec *rec = e->rec;
            if (_lshashtable_key_equals(rec, key, key_size)) {
                if (lshashtable_is_expired(lsht, rec))
                    return 0;
                val->val = rec->data + key_size;
                val->val_size = rec->val_size;
                return 1;
            }
        }
        NEXT_BUCKET(mask, index)
    }
}

static inline int lshashtable_del_key_index(struct lshashtable *lsht, const void *key, size_t key_size, struct lshashtable_entry *lsht_entry) {
    uint32_t mask = lsht->mask;
    struct lshashtable_entry *entries = (struct lshashtable_entry *)vmbuf_data(&lsht->index);
    uint32_t hc = hashcode2(key, key_size);
    uint32_t index = hc & mask;
    for (;;) {
        struct lshashtable_entry *e = entries + index;
        if (!e->rec) {
            if (lsht_entry)
                memset(lsht_entry, 0, sizeof(lsht_entry[0]));
            return 0;
        }
        if (e->hashcode == hc) {
            struct lshashtable_rec *rec = e->rec;
            if (_lshashtable_key_equals(rec, key, key_size)) {
                /* mark as deleted */
                if (lsht_entry)
                    *lsht_entry = *e;
                //lsht_record_store_del(lshashtable_get_rs_by_id(lsht, e->rs_id), rec);
                e->rec = NULL;
                --lsht->size;
                NEXT_BUCKET(mask, index)
                _lshashtable_fix_chain_down(lsht, mask, index);
                return 1;
            }
        }
        NEXT_BUCKET(mask, index)
    }
}

static inline int _lshashtable_del_key(struct lshashtable *lsht, const void *key, size_t key_size, int log) {
    struct lshashtable_entry e;
    int was_deleted = lshashtable_del_key_index(lsht, key, key_size, &e);
    if (was_deleted) {
        lsht_record_store_del(lshashtable_get_rs_by_id(lsht, e.rs_id), e.rec);
        if (log) {
            struct lshashtable_rec *delrec = _lshashtable_new_rec(lsht, key, key_size, 0);
            delrec->val_size = UINT_MAX;
        }
    }
    return was_deleted;
}

static inline int lshashtable_del_key_no_log(struct lshashtable *lsht, const void *key, size_t key_size) {
    return _lshashtable_del_key(lsht, key, key_size, 0);
}

int lshashtable_del_key(struct lshashtable *lsht, const void *key, size_t key_size) {
    return _lshashtable_del_key(lsht, key, key_size, 1);
}

int lshashtable_stats(struct lshashtable *lsht, struct vmbuf *buf) {
    struct lsht_record_store *rsbegin = (struct lsht_record_store *)vmbuf_data(&lsht->record_stores);
    struct lsht_record_store *rsend = (struct lsht_record_store *)vmbuf_wloc(&lsht->record_stores);
    struct lsht_record_store *rs;
    for (rs = rsbegin; rs != rsend; ++rs) {
        vmbuf_sprintf(buf, "store %hu, %.2f%% free (%zu/%zu)\n", lshashtable_get_rs_id(lsht, rs), (double)lsht_record_store_num_free_blocks(rs)/(double)lsht_record_store_total_blocks(rs)*100.0, lsht_record_store_num_free_blocks(rs), lsht_record_store_total_blocks(rs));
    }
    vmbuf_chrcpy(buf, '\n');
    return 0;
}

int lshashtable_foreach(struct lshashtable *lsht, int (*store_callback)(struct lshashtable_store*), int (*rec_callback)(struct lshashtable_rec*)) {
    struct lsht_record_store *rsbegin = (struct lsht_record_store *)vmbuf_data(&lsht->record_stores);
    struct lsht_record_store *rsend = (struct lsht_record_store *)vmbuf_wloc(&lsht->record_stores);
    struct lsht_record_store *rs;
    int res;

    int foreach_rec(void *r, void *arg __attribute__((unused))) {
        if (lshashtable_is_expired(lsht, r))
            return 0;
        struct lshashtable_rec *rec = r;
        return rec_callback (rec);
    }

    for (rs = rsbegin; rs != rsend; ++rs) {
        if (store_callback){
            struct lshashtable_store store_info = {
                    lshashtable_get_rs_id(lsht, rs),
                    lsht_record_store_total_blocks(rs),
                    lsht_record_store_num_free_blocks(rs)
            };
            store_callback(&store_info);
        }
       if  (rec_callback && 0 > (res = lsht_record_store_foreach(rs, foreach_rec, NULL)))
            return res;
    }
    return 0;
}

void rs_rseek(struct lsht_record_store *rs, size_t by){
    if (RINGFILE == rs->type)
        ringfile_rseek(&rs->data.file, by);
    else
        ringbuf_rseek(&rs->data.buf, by);
}

void rs_wseek(struct lsht_record_store *rs, size_t by){
    if (RINGFILE == rs->type)
        ringfile_wseek(&rs->data.file, by);
    else
        ringbuf_wseek(&rs->data.buf, by);
}

void *rs_wloc(struct lsht_record_store *rs){
    if (RINGFILE == rs->type)
        return ringfile_wloc(&rs->data.file);
    else
        return ringbuf_wloc(&rs->data.buf);
}

void *rs_rloc(struct lsht_record_store *rs){
    if (RINGFILE == rs->type)
        return ringfile_rloc(&rs->data.file);
    else
        return ringbuf_rloc(&rs->data.buf);
}

size_t rs_wlocpos(struct lsht_record_store *rs){
    if (RINGFILE == rs->type)
        return ringfile_wlocpos(&rs->data.file);
    else
        return ringbuf_wlocpos(&rs->data.buf);
}

size_t rs_rlocpos(struct lsht_record_store *rs){
    if (RINGFILE == rs->type)
        return ringfile_rlocpos(&rs->data.file);
    else
        return ringbuf_rlocpos(&rs->data.buf);
}

int rs_empty(struct lsht_record_store *rs){
    if (RINGFILE == rs->type)
        return ringfile_empty(&rs->data.file);
    else
        return ringbuf_empty(&rs->data.buf);
}

size_t rs_capacity(struct lsht_record_store *rs){
    if (RINGFILE == rs->type)
        return ringfile_capacity(&rs->data.file);
    else
        return ringbuf_capacity(&rs->data.buf);
}

size_t rs_avail(struct lsht_record_store *rs){
    if (RINGFILE == rs->type)
        return ringfile_avail(&rs->data.file);
    else
        return ringbuf_avail(&rs->data.buf);
}

void *rs_data(struct lsht_record_store *rs){
    if (RINGFILE == rs->type)
        return ringfile_data(&rs->data.file);
    else
        return ringbuf_mem(&rs->data.buf);
}
