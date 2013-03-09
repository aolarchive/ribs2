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
#include <limits.h>
/*
 * one to one index
 */
struct TEMPLATE(index_container_o2o, T) {
    struct file_mapper fm;
    struct TEMPLATE(index_key_rec_o2o, T) *data;
    struct index_header_o2o *header;
};

static inline int TEMPLATE(index_container_o2o_bsearch, T)(struct TEMPLATE(index_key_rec_o2o, T) *a, size_t n, T key, uint32_t *loc) {
    uint32_t l = 0, h = n;
    while (l < h) {
        uint32_t m = (l + h) >> 1;
        if (a[m].key < key) {
            l = m;
            ++l;
        } else {
            h = m;
        }
    }
    if (l < n && a[l].key == key)
        return *loc = a[l].row, 0;
    return -1;
}

static inline int TEMPLATE(index_container_o2o_init, T)(IDX_CONTAINER_O2O(T) *ic, const char *filename) {
    if (0 > file_mapper_init(&ic->fm, filename))
        return -1;
    if (file_mapper_size(&ic->fm) < sizeof(struct index_header_o2o))
        return LOGGER_ERROR("invalid file size: %s", filename), file_mapper_free(&ic->fm), -1;
    ic->header = file_mapper_data(&ic->fm);
    ic->data = file_mapper_data(&ic->fm) + sizeof(struct index_header_o2o);
    if (0 != memcmp(ic->header->signature, IDX_O2O_SIGNATURE, sizeof(ic->header->signature)))
        return LOGGER_ERROR("corrupted file: %s", filename), file_mapper_free(&ic->fm), -1;
    return 0;
}

static inline int TEMPLATE(index_container_o2o_init2, T)(IDX_CONTAINER_O2O(T) *ic, const char *base_path, const char *db, const char *table, const char *field) {
    char filename[PATH_MAX];
    if (PATH_MAX <= snprintf(filename, PATH_MAX, "%s/%s/%s/%s.idx", base_path, db, table, field))
        return LOGGER_ERROR("filename too long"), -1;
    return file_mapper_init(&ic->fm, filename);
}

static inline int TEMPLATE(index_container_o2o_lookup, T)(IDX_CONTAINER_O2O(T) *ic, T key, uint32_t *loc) {
    return TEMPLATE(index_container_o2o_bsearch, T)(ic->data, ic->header->num_keys, key, loc);
}

static inline int TEMPLATE(index_container_o2o_exist, T)(IDX_CONTAINER_O2O(T) *ic, T key) {
    uint32_t loc;
    return TEMPLATE(index_container_o2o_bsearch, T)(ic->data, ic->header->num_keys, key, &loc);
}


/*
 * one to many index
 */
struct TEMPLATE(index_container_o2m, T) {
    struct file_mapper fm;
    struct index_header_o2m *header;
    uint32_t *vect;
    struct TEMPLATE(index_key_rec_o2m, T) *keys;
};

static inline int TEMPLATE(index_container_o2m_init, T)(IDX_CONTAINER_O2M(T) *ic, const char *filename) {
    if (0 > file_mapper_init(&ic->fm, filename))
        return -1;
    if (file_mapper_size(&ic->fm) < sizeof(struct index_header_o2m))
        return LOGGER_ERROR("invalid file size: %s", filename), file_mapper_free(&ic->fm), -1;
    ic->header = file_mapper_data(&ic->fm);
    if (0 != memcmp(ic->header->signature, IDX_O2M_SIGNATURE, sizeof(ic->header->signature)))
        return LOGGER_ERROR("corrupted file: %s", filename), file_mapper_free(&ic->fm), -1;
    ic->vect = file_mapper_data(&ic->fm) + sizeof(struct index_header_o2m);
    ic->keys = file_mapper_data(&ic->fm) + ic->header->keys_ofs;
    return 0;
}

static inline int TEMPLATE(index_container_o2m_bsearch, T)(struct TEMPLATE(index_key_rec_o2m, T) *a, size_t n, T key, uint32_t *loc) {
    uint32_t l = 0, h = n;
    while (l < h) {
        uint32_t m = (l + h) >> 1;
        if (a[m].key < key) {
            l = m;
            ++l;
        } else {
            h = m;
        }
    }
    if (l < n && a[l].key == key)
        return *loc = l, 0;
    return -1;
}

static inline int TEMPLATE(index_container_o2m_lookup, T)(IDX_CONTAINER_O2M(T) *ic, T key, uint32_t **vect, uint32_t *size) {
    uint32_t loc;
    if (0 > TEMPLATE(index_container_o2m_bsearch, T)(ic->keys, ic->header->num_keys, key, &loc))
        return -1;
    struct TEMPLATE(index_key_rec_o2m, T) *key_rec = ic->keys + loc;
    *vect = ic->vect + key_rec->vect;
    *size = key_rec->size;
    return 0;
}

static inline int TEMPLATE(index_container_o2m_exist, T)(IDX_CONTAINER_O2M(T) *ic, T key) {
    uint32_t loc;
    return TEMPLATE(index_container_o2m_bsearch, T)(ic->keys, ic->header->num_keys, key, &loc);
}
