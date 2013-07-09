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
#ifndef _OLD_HASHTABLE__H_
#define _OLD_HASHTABLE__H_

#include "ribs_defs.h"
#include "vmbuf.h"
#include "vmfile.h"
#include "template.h"
#include "hash_funcs.h"
#include "ilog2.h"
#include "hashtablefile_header.h"

#define _TEMPLATE_HTBL_FUNC_HELPER(S,T,F) S ## T ##_## F
#define TEMPLATE_HTBL_FUNC(S,T,F) _TEMPLATE_HTBL_FUNC_HELPER(S,T,F)

#define _TEMPLATE_HTBL_HELPER(S,T) S ## T
#define TEMPLATE_HTBL(S,T) _TEMPLATE_HTBL_HELPER(S,T)

#define HASHTABLE_INITIAL_SIZE_BITS 5
#define HASHTABLE_INITIAL_SIZE (1<<HASHTABLE_INITIAL_SIZE_BITS)

#define HASHTABLEFILE_INITIALIZER { VMFILE_INITIALIZER }
#define HASHTABLEFILE_MAKE(x) (x) = (struct hashtablefile)HASHTABLEFILE_INITIALIZER

struct hashtablefile {
    struct vmfile data;
};

/* hashtable_file */
int hashtablefile_init_create(struct hashtablefile *ht, const char *file_name, uint32_t initial_size);
int hashtablefile_finalize(struct hashtablefile *ht);
int hashtablefile_init(struct hashtablefile *ht, uint32_t initial_size);
uint32_t hashtablefile_insert(struct hashtablefile *ht, const void *key, size_t key_len, const void *val, size_t val_len);
uint32_t hashtablefile_insert_new(struct hashtablefile *ht, const void *key, size_t key_len, size_t val_len);
uint32_t hashtablefile_lookup_insert(struct hashtablefile *ht, const void *key, size_t key_len, const void *val, size_t val_len);
uint32_t hashtablefile_lookup(struct hashtablefile *ht, const void *key, size_t key_len);
uint32_t hashtablefile_remove(struct hashtablefile *ht, const void *key, size_t key_len);
int hashtablefile_foreach(struct hashtablefile *ht, int (*func)(uint32_t rec));
static inline void *hashtablefile_get_key(struct hashtablefile *ht, uint32_t rec_ofs);
static inline uint32_t hashtablefile_get_key_size(struct hashtablefile *ht, uint32_t rec_ofs);
static inline void *hashtablefile_get_val(struct hashtablefile *ht, uint32_t rec_ofs);
static inline uint32_t hashtablefile_get_val_size(struct hashtablefile *ht, uint32_t rec_ofs);
static inline uint32_t hashtablefile_get_size(struct hashtablefile *ht);
static inline const char *hashtablefile_lookup_str(struct hashtablefile *ht, const char *key, const char *default_val);
static inline void hashtablefile_free(struct hashtablefile *ht);


#ifdef T
#undef T
#endif

#ifdef TS
#undef TS
#endif

#define T file
#define TS vmfile
#define HT_SIZE (((struct hashtablefile_header *) TEMPLATE(TS, data)(&ht->data))->size)
#define HT_MASK (((struct hashtablefile_header *) TEMPLATE(TS, data)(&ht->data))->mask)
#define HT_SLOT ((uint32_t *) TEMPLATE(TS, data_ofs)(&ht->data, sizeof(struct hashtablefile_header)))
#include "_old_hashtable.h"
#undef HT_SIZE
#undef HT_MASK
#undef HT_SLOT
#undef TS
#undef T

#endif // _HASHTABLE__H_
