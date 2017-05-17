/*
    This file is part of RIBS2.0 (Robust Infrastructure for Backend Systems).
    RIBS is an infrastructure for building great SaaS applications (but not
    limited to).

    Copyright (C)  Adap.tv, Inc.

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
#ifndef _LSHASHTABLE__H_
#define _LSHASHTABLE__H_

#include "ribs.h"
#include "ringbuf.h"
#include "ringfile.h"

/* log structured hashtable */

#define LSHASHTABLE_INITIALIZER { 0, 0, 0, NULL, 0, VMBUF_INITIALIZER, VMBUF_INITIALIZER, _lshashtable_is_expired_noop }

struct lshashtable_val {
    uint32_t val_size;
    void *val;
};

#pragma pack(push,1)
struct lshashtable_entry {
    uint32_t hashcode;
    void *rec;
    uint16_t rs_id;
};

struct lshashtable_rec {
    uint16_t key_size;
    uint32_t val_size;
    char data[];
};

#pragma pack(pop)

struct lshashtable {
    uint32_t mask;
    uint32_t size;
    size_t capacity;
    char *basedir;
    uint16_t next_rs_id;
    struct vmbuf index;
    struct vmbuf record_stores;
    int (*is_expired)(void *rec);
};

struct lshashtable_store {
    uint32_t store_id;
    size_t num_blocks;
    size_t num_free_blocks;
};

int lshashtable_init(struct lshashtable *lsht, const char *basedir, size_t capacity);
static inline int lshashtable_is_init(struct lshashtable *lsht) { return lsht->mask > 0; }
static inline void lshashtable_set_is_expired(struct lshashtable *lsht, int (*is_expired)(void *rec)) { lsht->is_expired = is_expired; }
int lshashtable_close(struct lshashtable *lsht);
void *lshashtable_put_key(struct lshashtable *lsht, const void *key, size_t key_size, size_t val_size, struct lshashtable_val *oldval);
int lshashtable_find_key(struct lshashtable *lsht, const void *key, size_t key_size, struct lshashtable_val *val);
int lshashtable_del_key(struct lshashtable *lsht, const void *key, size_t key_size);
int lshashtable_stats(struct lshashtable *lsht, struct vmbuf *buf); /* TODO: remove */
int lshashtable_test(struct lshashtable *lsht);
static inline int _lshashtable_is_expired_noop(void *rec __attribute__((unused))) { return 0; }
static inline void *lshashtable_get_val(void *rec) { struct lshashtable_rec *r = rec; return r->data + r->key_size; }
int lshashtable_foreach(struct lshashtable *lsht, int (*store_callback)(struct lshashtable_store*), int (*rec_callback)(struct lshashtable_rec*));


#endif // _LSHASHTABLE__H_
