/*
    This file is part of RIBS2.0 (Robust Infrastructure for Backend Systems).
    RIBS is an infrastructure for building great SaaS applications (but not
    limited to).

    Copyright (C) 2012 Adap.tv, Inc.

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
