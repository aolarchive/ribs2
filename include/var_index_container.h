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
#ifndef _VAR_IDX_CONT__H_
#define _VAR_IDX_CONT__H_

#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "file_mapper.h"
#include "hashtable.h"
#include "var_index_entry.h"

#define VAR_INDEX_CONTAINER_INITIALIZER { FILE_MAPPER_INITIALIZER, HASHTABLEFILE_READONLY_INITIALIZER}

struct var_index_container_o2m {
    struct file_mapper fm;
    struct hashtable ht_keys;
};

static inline int var_index_container_o2m_init(struct var_index_container_o2m *ic, const char *filename) {
    char idx_filename[PATH_MAX];
    if (PATH_MAX <= snprintf(idx_filename, PATH_MAX, "%s.keys", filename) ||
        0 > hashtable_open(&ic->ht_keys, 0, idx_filename, O_RDONLY))
        return -1;

    if (PATH_MAX <= snprintf(idx_filename, PATH_MAX, "%s.idx", filename) ||
        0 > file_mapper_init(&ic->fm, idx_filename))
        return -1;

    return 0;
}

static inline int var_index_container_o2m_lookup(struct var_index_container_o2m *ic, const char *key, size_t key_len, uint32_t **vect, uint32_t *size) {
    uint32_t rec_ofs = hashtable_lookup(&ic->ht_keys, key, key_len);
    if (0 == rec_ofs)
        return -1;

    struct hashtable_rec rec = hashtable_get_rec(&ic->ht_keys, rec_ofs);
    const struct index_entry *entry = rec.val;
    *vect = ((uint32_t *) file_mapper_data(&ic->fm)) + entry->index_offs;
    *size = entry->size;

    return 0;
}

static inline int var_index_container_o2m_close(struct var_index_container_o2m *ic) {
    if (0 > hashtable_close(&ic->ht_keys) || 0 > file_mapper_free(&ic->fm))
        return -1;
    return 0;
}

#endif // _VAR_IDX_CONT__H_
