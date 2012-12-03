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

#ifndef _HASHTABLE_RDONLY_H
#define _HASHTABLE_RDONLY_H

#include "ribs_defs.h"
#include "template.h"
#include "hash_funcs.h"
#include "ilog2.h"
#include "file_mapper.h"
#include "hashtablefile_header.h"

#define HASHTABLE_INITIAL_SIZE_BITS 5
#define HASHTABLE_INITIAL_SIZE (1<<HASHTABLE_INITIAL_SIZE_BITS)

#define HASHTABLEFILE_READONLY_INITIALIZER { FILE_MAPPER_INITIALIZER, NULL, NULL}

struct hashtablefile_readonly {
    struct file_mapper fm;
    struct hashtablefile_header *header;
    void *data;
};

int hashtablefile_readonly_init(struct hashtablefile_readonly *ht, const char *file_name);
int hashtablefile_readonly_close(struct hashtablefile_readonly *ht);
uint32_t hashtablefile_readonly_lookup(struct hashtablefile_readonly *ht, const void *key, size_t key_len);

static inline char *_data_ofs(void *data, size_t ofs){
    return data + ofs;
}

static inline void *hashtablefile_readonly_get_key(struct hashtablefile_readonly *ht, uint32_t rec_ofs) {
    char *rec = _data_ofs(ht->data, rec_ofs);
    return rec + (sizeof(uint32_t) * 2);
}

static inline uint32_t hashtablefile_readonly_get_key_size(struct hashtablefile_readonly *ht, uint32_t rec_ofs) {
    char *rec = _data_ofs(ht->data, rec_ofs);
    return *((uint32_t *)rec);
}

static inline void *hashtablefile_readonly_get_val(struct hashtablefile_readonly *ht, uint32_t rec_ofs) {
    char *rec = _data_ofs(ht->data, rec_ofs);
    return rec + (sizeof(uint32_t) * 2) + *(uint32_t *)rec;
}

static inline uint32_t hashtablefile_readonly_get_val_size(struct hashtablefile_readonly *ht, uint32_t rec_ofs) {
    char *rec = _data_ofs(ht->data, rec_ofs);
    return *((uint32_t *)rec + 1);
}

static inline uint32_t hashtablefile_readonly_get_size(struct hashtablefile_readonly *ht) {
    return ht->header->size;
}

#endif // _HASHTABLE_RDONLY_H
