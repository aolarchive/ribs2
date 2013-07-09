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
#ifndef _HASHTABLE__H_
#define _HASHTABLE__H_

#include "ribs_defs.h"
#include "vmbuf.h"

#define HASHTABLE_INITIALIZER { VMBUF_INITIALIZER, 0, 0 }
#define HASHTABLE_MAKE(x) (x) = (struct hashtable)HASHTABLE_INITIALIZER

struct hashtable {
    struct vmbuf data;
    uint32_t mask;
    uint32_t size;
};

/* hashtable */
int hashtable_init(struct hashtable *ht, uint32_t initial_size);
uint32_t hashtable_insert(struct hashtable *ht, const void *key, size_t key_len, const void *val, size_t val_len);
uint32_t hashtable_insert_new(struct hashtable *ht, const void *key, size_t key_len, size_t val_len);
uint32_t hashtable_lookup_insert(struct hashtable *ht, const void *key, size_t key_len, const void *val, size_t val_len);
uint32_t hashtable_lookup(struct hashtable *ht, const void *key, size_t key_len);
uint32_t hashtable_remove(struct hashtable *ht, const void *key, size_t key_len);
int hashtable_foreach(struct hashtable *ht, int (*func)(uint32_t rec));
static inline void *hashtable_get_key(struct hashtable *ht, uint32_t rec_ofs);
static inline uint32_t hashtable_get_key_size(struct hashtable *ht, uint32_t rec_ofs);
static inline void *hashtable_get_val(struct hashtable *ht, uint32_t rec_ofs);
static inline uint32_t hashtable_get_val_size(struct hashtable *ht, uint32_t rec_ofs);
static inline uint32_t hashtable_get_size(struct hashtable *ht);
static inline const char *hashtable_lookup_str(struct hashtable *ht, const char *key, const char *default_val);
static inline void hashtable_free(struct hashtable *ht);

#include "../src/_hashtable.c"

#endif // _HASHTABLE__H_
