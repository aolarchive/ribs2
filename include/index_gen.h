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

struct TEMPLATE(index_gen_fw_index, T) {
    T key;
    uint32_t row_loc;
};

struct TEMPLATE(index_key_rec_o2o, T) {
    T key;
    uint32_t row;
};

struct TEMPLATE(index_key_rec_o2m, T) {
    T key;
    uint32_t size;
    uint32_t vect;
};

static inline int TEMPLATE(index_gen_fw_compar, T)(const void *a, const void *b) {
    struct TEMPLATE(index_gen_fw_index, T) *aa = (struct TEMPLATE(index_gen_fw_index, T) *)a;
    struct TEMPLATE(index_gen_fw_index, T) *bb = (struct TEMPLATE(index_gen_fw_index, T) *)b;
    return aa->key < bb->key ? -1 : (bb->key < aa->key ? 1 : (aa->row_loc < bb->row_loc ? -1 : (bb->row_loc < aa->row_loc ? 1 : 0))) ;
}

static inline int TEMPLATE(index_gen_generate_mem_o2o, T)(struct TEMPLATE(index_gen_fw_index, T) *fw_index, size_t n, const char *filename) {
    qsort(fw_index, n, sizeof(IDX_FW_INDEX(T)), TEMPLATE(index_gen_fw_compar, T));
    IDX_FW_INDEX(T) *fw = fw_index, *fw_end = fw + n;
    struct file_writer fw_idx = FILE_WRITER_INITIALIZER;
    if (0 > file_writer_init(&fw_idx, filename))
        return -1;
    struct index_header_o2o header;
    memset(&header, 0, sizeof(header));
    file_writer_write(&fw_idx, &header, sizeof(header));
    uint32_t num_keys = 0;
    for (; fw != fw_end; ++num_keys) {
        struct TEMPLATE(index_key_rec_o2o, T) output_rec = { fw->key, fw->row_loc };
        T key = fw->key;
        for (++fw; fw != fw_end && key == fw->key; ++fw);
        file_writer_write(&fw_idx, &output_rec, sizeof(output_rec));
    }
    size_t ofs = file_writer_wlocpos(&fw_idx);
    file_writer_lseek(&fw_idx, 0, SEEK_SET);
    header = (struct index_header_o2o){ IDX_O2O_SIGNATURE, num_keys };
    file_writer_write(&fw_idx, &header, sizeof(header));
    file_writer_lseek(&fw_idx, ofs, SEEK_SET);
    file_writer_close(&fw_idx);
    return 0;
}

static inline int TEMPLATE(index_gen_generate_mem_o2m, T)(struct TEMPLATE(index_gen_fw_index, T) *fw_index, size_t n, const char *filename) {
    qsort(fw_index, n, sizeof(IDX_FW_INDEX(T)), TEMPLATE(index_gen_fw_compar, T));
    IDX_FW_INDEX(T) *fw = fw_index, *fw_end = fw + n;
    struct file_writer fw_idx = FILE_WRITER_INITIALIZER;
    if (0 > file_writer_init(&fw_idx, filename))
        return -1;
    struct index_header_o2m header;
    memset(&header, 0, sizeof(header));
    file_writer_write(&fw_idx, &header, sizeof(header));
    struct vmbuf vmb_keys = VMBUF_INITIALIZER;
    vmbuf_init(&vmb_keys, 1024*1024);
    uint32_t vect = 0;
    uint32_t num_keys = 0;
    for (; fw != fw_end; ++num_keys) {
        T key = fw->key;
        uint32_t size = 0;
        for (; fw != fw_end && key == fw->key; ++fw, ++size) {
            file_writer_write(&fw_idx, &fw->row_loc, sizeof(uint32_t));
        }
        struct TEMPLATE(index_key_rec_o2m, T) output_rec = { key, size, vect };
        vmbuf_memcpy(&vmb_keys, &output_rec, sizeof(output_rec));
        vect += size;
    }
    uint64_t keys_ofs = file_writer_wlocpos(&fw_idx);
    file_writer_write(&fw_idx, vmbuf_data(&vmb_keys), vmbuf_wlocpos(&vmb_keys));
    vmbuf_free(&vmb_keys);
    size_t ofs = file_writer_wlocpos(&fw_idx);
    file_writer_lseek(&fw_idx, 0, SEEK_SET);
    header = (struct index_header_o2m){ IDX_O2M_SIGNATURE, keys_ofs, num_keys };
    file_writer_write(&fw_idx, &header, sizeof(header));
    file_writer_lseek(&fw_idx, ofs, SEEK_SET);
    file_writer_close(&fw_idx);
    return 0;
}

static inline int TEMPLATE(_index_gen_generate_ds_file, T)(const char *base_path, const char *db, const char *table, const char *field, int (*coalesce_func)(struct TEMPLATE(index_gen_fw_index, T) *fw_index, size_t n, const char *filename)) {
    char output_filename[PATH_MAX];
    if (PATH_MAX <= snprintf(output_filename, PATH_MAX, "%s/%s/%s/%s.idx", base_path, db, table, field))
        return LOGGER_ERROR("filename too long"), -1;
    DS_FIELD(T) ds = DS_FIELD_INITIALIZER;
    char filename[PATH_MAX];
    // strlen("%s/%s/%s/%s") < strlen("%s/%s/%s/%s.idx"); not checking again
    snprintf(filename, PATH_MAX, "%s/%s/%s/%s", base_path, db, table, field);
    if (0 > DS_FIELD_INIT(T, &ds, filename))
        return LOGGER_ERROR("failed to init datastore"), -1;

    struct vmbuf fw_idx = VMBUF_INITIALIZER;
    vmbuf_init(&fw_idx, sizeof(IDX_FW_INDEX(T)) * DS_FIELD_NUM_ELEMENTS(&ds));
    IDX_FW_INDEX(T) *fw = (IDX_FW_INDEX(T) *)vmbuf_data(&fw_idx);
    T *rec_begin = DS_FIELD_BEGIN(&ds), *rec_end = DS_FIELD_END(&ds), *rec = rec_begin;
    for (; rec != rec_end; ++rec, ++fw) {
        fw->key = *rec;
        fw->row_loc = rec - rec_begin;
    }
    int res = coalesce_func((IDX_FW_INDEX(T) *)vmbuf_data(&fw_idx), rec - rec_begin, output_filename);
    DS_FIELD_FREE(T, &ds);
    vmbuf_free(&fw_idx);
    return res;
}

static inline int TEMPLATE(index_gen_generate_ds_file_o2o, T)(const char *base_path, const char *db, const char *table, const char *field) {
    return TEMPLATE(_index_gen_generate_ds_file, T)(base_path, db, table, field,  TEMPLATE(index_gen_generate_mem_o2o, T));
}

static inline int TEMPLATE(index_gen_generate_ds_file_o2m, T)(const char *base_path, const char *db, const char *table, const char *field) {
    return TEMPLATE(_index_gen_generate_ds_file, T)(base_path, db, table, field,  TEMPLATE(index_gen_generate_mem_o2m, T));
}
