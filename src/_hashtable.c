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
#define _HASHTABLE_HEADER() ((struct hashtable_header *)vmbuf_data(&ht->data))
#define _HASHTABLE_REC(rec_ofs) ((union hashtable_rec *)vmbuf_data_ofs(&ht->data, rec_ofs))

static inline void *hashtable_get_key(struct hashtable *ht, uint32_t rec_ofs) {
    union hashtable_rec *rec = _HASHTABLE_REC(rec_ofs);
    return rec->rec.data;
}

static inline uint32_t hashtable_get_key_size(struct hashtable *ht, uint32_t rec_ofs) {
    union hashtable_rec *rec = _HASHTABLE_REC(rec_ofs);
    return rec->rec.key_size;
}

static inline void *hashtable_get_val(struct hashtable *ht, uint32_t rec_ofs) {
    union hashtable_rec *rec = _HASHTABLE_REC(rec_ofs);
    return rec->rec.data + rec->rec.key_size;
}

static inline uint32_t hashtable_get_val_size(struct hashtable *ht, uint32_t rec_ofs) {
    union hashtable_rec *rec = _HASHTABLE_REC(rec_ofs);
    return rec->rec.val_size;
}

static inline uint32_t hashtable_get_size(struct hashtable *ht) {
    return _HASHTABLE_HEADER()->size;
}

static inline const char *hashtable_lookup_str(struct hashtable*ht, const char *key, const char *default_val) {
    uint32_t ofs = hashtable_lookup(ht, key, strlen(key));
    return (ofs ? hashtable_get_val(ht, ofs) : default_val);
}

static inline void hashtable_free(struct hashtable *ht) {
    vmbuf_free(&ht->data);
}
