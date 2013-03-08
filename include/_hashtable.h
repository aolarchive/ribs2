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
static inline void *TEMPLATE_HTBL_FUNC(hashtable, T, get_key)(struct TEMPLATE_HTBL(hashtable, T) *ht, uint32_t rec_ofs) {
    char *rec = TEMPLATE(TS, data_ofs)(&ht->data, rec_ofs);
    return rec + (sizeof(uint32_t) * 2);
}

static inline uint32_t TEMPLATE_HTBL_FUNC(hashtable, T, get_key_size)(struct TEMPLATE_HTBL(hashtable, T) *ht, uint32_t rec_ofs) {
    char *rec = TEMPLATE(TS, data_ofs)(&ht->data, rec_ofs);
    return *((uint32_t *)rec);
}

static inline void *TEMPLATE_HTBL_FUNC(hashtable, T, get_val)(struct TEMPLATE_HTBL(hashtable, T) *ht, uint32_t rec_ofs) {
    char *rec = TEMPLATE(TS, data_ofs)(&ht->data, rec_ofs);
    return rec + (sizeof(uint32_t) * 2) + *(uint32_t *)rec;
}

static inline uint32_t TEMPLATE_HTBL_FUNC(hashtable, T, get_val_size)(struct TEMPLATE_HTBL(hashtable, T) *ht, uint32_t rec_ofs) {
    char *rec = TEMPLATE(TS, data_ofs)(&ht->data, rec_ofs);
    return *((uint32_t *)rec + 1);
}

static inline uint32_t TEMPLATE_HTBL_FUNC(hashtable, T, get_size)(struct TEMPLATE_HTBL(hashtable, T) *ht) {
    return HT_SIZE;
}

static inline const char *TEMPLATE_HTBL_FUNC(hashtable, T, lookup_str)(struct TEMPLATE_HTBL(hashtable, T) *ht, const char *key, const char *default_val) {
    uint32_t ofs = TEMPLATE_HTBL_FUNC(hashtable, T, lookup)(ht, key, strlen(key));
    return (ofs ? TEMPLATE_HTBL_FUNC(hashtable, T, get_val)(ht, ofs) : default_val);
}

static inline void TEMPLATE_HTBL_FUNC(hashtable, T, free)(struct TEMPLATE_HTBL(hashtable, T) *ht) {
    TEMPLATE(TS, free)(&ht->data);
}
