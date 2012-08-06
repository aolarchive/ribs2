_RIBS_INLINE_ int ds_var_field_get(struct ds_var_field *dsvf, size_t index, char **ret_ptr, size_t *ret_size) {
    size_t *ofs_table = file_mapper_data(&dsvf->ofs_table);
    size_t ofs = ofs_table[index];
    *ret_ptr = file_mapper_data(&dsvf->data) + ofs;
    *ret_size = ofs_table[index + 1] - ofs;
    return 0;
}

_RIBS_INLINE_ const char *ds_var_field_get_cstr(struct ds_var_field *dsvf, size_t index) {
    size_t *ofs_table = file_mapper_data(&dsvf->ofs_table);
    return file_mapper_data(&dsvf->data) + ofs_table[index];
}
