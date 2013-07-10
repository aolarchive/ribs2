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
#ifndef _VAR_IDX_GEN__H_
#define _VAR_IDX_GEN__H_

#include <limits.h>
#include "ds_var_field.h"
#include "hashtable.h"
#include "file_writer.h"
#include "logger.h"
#include "var_index_entry.h"

struct var_index_gen_fw_index {
    uint32_t ht_offs;
    uint32_t row_loc;
};

static inline void _exit_clean(struct ds_var_field *var_field) {
  	ds_var_field_free(var_field);
}

static inline int var_index_gen_fw_compar(const void *a, const void *b) {
    struct var_index_gen_fw_index *aa = (struct var_index_gen_fw_index *) a;
    struct var_index_gen_fw_index *bb = (struct var_index_gen_fw_index *)b;
    return aa->ht_offs < bb->ht_offs ? -1 : (bb->ht_offs < aa->ht_offs ? 1 : (aa->row_loc < bb->row_loc ? -1 : (bb->row_loc < aa->row_loc ? 1 : 0))) ;
}

static inline int var_index_gen_generate_mem_o2m(struct var_index_gen_fw_index *fw_index, size_t n, struct hashtable *ht_keys, const char *filename) {
    qsort(fw_index, n, sizeof(struct var_index_gen_fw_index), var_index_gen_fw_compar);
    struct var_index_gen_fw_index *fw = fw_index, *fw_end = fw + n;
    struct file_writer fw_idx = FILE_WRITER_INITIALIZER;
    if (0 > file_writer_init(&fw_idx, filename))
        return -1;

    uint32_t vect = 0;
    for (; fw != fw_end;) {
        uint32_t ht_offs = fw->ht_offs;
        uint32_t size = 0;
        for (; fw != fw_end && ht_offs == fw->ht_offs; ++fw, ++size) {
            file_writer_write(&fw_idx, &fw->row_loc, sizeof(uint32_t));
        }
        struct hashtable_rec rec = hashtable_get_rec(ht_keys, ht_offs);
        if (rec.val_size == sizeof(struct index_entry)) {
            struct index_entry *entry = rec.val;
            entry->index_offs = vect;
            entry->size = size;
        } else {
            return LOGGER_ERROR("hashtable corrupted"), -1;
        }
        vect += size;
    }
    file_writer_close(&fw_idx);
    return 0;
}

static inline int var_index_gen_generate_ds_file (const char *base_path, const char *db, const char *table, const char *field) {
    char output_filename[PATH_MAX];
    if (PATH_MAX <= snprintf(output_filename, PATH_MAX, "%s/%s/%s/%s.idx", base_path, db, table, field))
        return LOGGER_ERROR("output filename too long"), -1;
    char filename[PATH_MAX];
    // strlen("%s/%s/%s/%s") < strlen("%s/%s/%s/%s.idx"); not checking again
    snprintf(filename, PATH_MAX, "%s/%s/%s/%s", base_path, db, table, field);

    struct ds_var_field var_field = DS_VAR_FIELD_INITIALIZER;
    if (0 > ds_var_field_init(&var_field, filename))
        return LOGGER_ERROR("failed to init datastore"), -1;

    strcat(filename, ".keys");
    struct hashtable ht_keys = HASHTABLE_INITIALIZER;
    if (0 > hashtable_create(&ht_keys, 0, filename))
        return LOGGER_ERROR("failed to init hashtable"), _exit_clean(&var_field), -1;

    struct index_entry temp_entry;
    temp_entry.index_offs = 0;
    temp_entry.size = 0;

    struct vmbuf fw_idx = VMBUF_INITIALIZER;
    vmbuf_init(&fw_idx, sizeof(struct var_index_gen_fw_index) * var_field.num_elements);
    struct var_index_gen_fw_index *fw = (struct var_index_gen_fw_index *)vmbuf_data(&fw_idx);
    void *data = file_mapper_data(&var_field.data);
    size_t *rec_begin = var_field.ofs_table;
    size_t *rec_end = var_field.ofs_table + var_field.num_elements;
    size_t *rec = rec_begin;
    char *str = NULL;
    for (; rec != rec_end; ++rec, ++fw) {
        size_t *next_rec = rec + 1;
        str = (char *) (data + *rec);
        uint32_t ofs = hashtable_lookup(&ht_keys, str, (*next_rec - *rec));
        if (0 == ofs)
            ofs = hashtable_insert(&ht_keys, str, (*next_rec - *rec), &temp_entry, sizeof(temp_entry));
        fw->ht_offs = ofs;
        fw->row_loc = rec - rec_begin;
    }
    int res = var_index_gen_generate_mem_o2m((struct var_index_gen_fw_index *)vmbuf_data(&fw_idx), rec - rec_begin, &ht_keys, output_filename);
    vmbuf_free(&fw_idx);
    hashtable_close(&ht_keys);
    _exit_clean(&var_field);
    return res;
}

#endif // _VAR_IDX_GEN__H_
