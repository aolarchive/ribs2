#include "ds_var_field.h"
#include "vmbuf.h"
#include "logger.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int ds_var_field_init(struct ds_var_field *dsvf, const char *filename) {
    if (0 > ds_var_field_free(dsvf))
        return -1;
    int n = strlen(filename);
    char fn_dat[n + 4 + 1];
    char fn_ofs[n + 4 + 1];
    strcat(strcpy(fn_dat, filename), ".dat");
    strcat(strcpy(fn_ofs, filename), ".ofs");
    if (0 > file_mapper_init(&dsvf->data, fn_dat) ||
        0 > file_mapper_init(&dsvf->ofs_table, fn_ofs))
        return -1;
    dsvf->num_elements = (file_mapper_size(&dsvf->ofs_table) / sizeof(size_t)) - 1;
    size_t *ofs_table = file_mapper_data(&dsvf->ofs_table);
    if (ofs_table[dsvf->num_elements] != file_mapper_size(&dsvf->data))
        return LOGGER_ERROR_FUNC("%s: corrupted data detected", filename), -1;
    return 0;
}

int ds_var_field_free(struct ds_var_field *dsvf) {
    return file_mapper_free(&dsvf->ofs_table) + file_mapper_free(&dsvf->data);
}
