/*
    This file is part of RIBS2.0 (Robust Infrastructure for Backend Systems).
    RIBS is an infrastructure for building great SaaS applications (but not
    limited to).

    Copyright (C) 2014 Adap.tv, Inc.

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
#ifndef _THASHTABLE__H_
#define _THASHTABLE__H_

#include "ribs_defs.h"
#include <string.h>

struct thashtable;
struct thashtable_internal_rec;

typedef struct thashtable_internal_rec thashtable_rec_t;

struct thashtable *thashtable_create(void);
thashtable_rec_t *thashtable_insert(struct thashtable *tht, const void *key, size_t key_len, const void *val, size_t val_len, int *inserted);
thashtable_rec_t *thashtable_put(struct thashtable *tht, const void *key, size_t key_len, const void *val, size_t val_len);
thashtable_rec_t *thashtable_insert_alloc(struct thashtable *tht, const void *key, size_t key_len, size_t val_len);
thashtable_rec_t *thashtable_lookup(struct thashtable *tht, const void *key, size_t key_len);
thashtable_rec_t *thashtable_remove(struct thashtable *tht, const void *key, size_t key_len);
int thashtable_foreach(struct thashtable *tht, int (*func)(thashtable_rec_t *rec));

static inline void *thashtable_get_key(thashtable_rec_t *rec);
static inline uint32_t thashtable_get_key_size(thashtable_rec_t *rec);
static inline void *thashtable_get_val(thashtable_rec_t *rec);
static inline uint32_t thashtable_get_val_size(thashtable_rec_t *rec);
static inline uint32_t thashtable_get_size(struct thashtable *tht);
static inline const char *thashtable_lookup_str(struct thashtable *tht, const char *key, const char *default_val);

#include "../src/_thashtable.c"

#endif // _THASHTABLE__H_
