#ifndef _HASHTABLE__H_
#define _HASHTABLE__H_

#include "ribs_defs.h"
#include "vmbuf.h"

struct hashtable {
    struct vmbuf data;
    uint32_t mask;
    uint32_t size;
};

int hashtable_init(struct hashtable *ht, uint32_t nel);
uint32_t hashtable_insert(struct hashtable *ht, const void *key, size_t key_len, const void *val, size_t val_len);
uint32_t hashtable_lookup(struct hashtable *ht, const void *key, size_t key_len);

static inline void *hashtable_get_key(struct hashtable *ht, uint32_t rec_ofs) {
    char *rec = vmbuf_data_ofs(&ht->data, rec_ofs);
    return rec + (sizeof(uint32_t) * 2);
}

static inline uint32_t hashtable_get_key_size(struct hashtable *ht, uint32_t rec_ofs) {
    char *rec = vmbuf_data_ofs(&ht->data, rec_ofs);
    return *((uint32_t *)rec);
}

static inline void *hashtable_get_val(struct hashtable *ht, uint32_t rec_ofs) {
    char *rec = vmbuf_data_ofs(&ht->data, rec_ofs);
    return rec + (sizeof(uint32_t) * 2) + *(uint32_t *)rec;
}

static inline uint32_t hashtable_get_val_size(struct hashtable *ht, uint32_t rec_ofs) {
    char *rec = vmbuf_data_ofs(&ht->data, rec_ofs);
    return *((uint32_t *)rec + 1);
}


#endif // _HASHTABLE__H_
