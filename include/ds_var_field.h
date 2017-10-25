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
#ifndef _DS_VAR_FIELD__h_
#define _DS_VAR_FIELD__h_

#include "ribs_defs.h"
#include "file_mapper.h"
#include "file_writer.h"
#include "vmbuf.h"

#define DS_VAR_FIELD_INITIALIZER { FILE_MAPPER_INITIALIZER, NULL, 0 }
#define DS_VAR_FIELD_WRITER_INITIALIZER { VMBUF_INITIALIZER, FILE_WRITER_INITIALIZER }
#define DS_VAR_FIELD_INIT(var) (var) = (struct ds_var_field)DS_VAR_FIELD_INITIALIZER
#define DS_VAR_FIELD_WRITER_INIT(var) (var) = (struct ds_var_field_writer)DS_VAR_FIELD_WRITER_INITIALIZER

struct ds_var_field {
    struct file_mapper data;
    size_t *ofs_table;
    size_t num_elements;
};

struct ds_var_field_writer {
    struct vmbuf ofs_table;
    struct file_writer data;
};

int ds_var_field_init(struct ds_var_field *dsvf, const char *filename);
int ds_var_field_free(struct ds_var_field *dsvf);
_RIBS_INLINE_ int ds_var_field_get(struct ds_var_field *dsvf, size_t index, char **ret_ptr, size_t *ret_size);
_RIBS_INLINE_ const char *ds_var_field_get_cstr(struct ds_var_field *dsvf, size_t index);

int ds_var_field_writer_init(struct ds_var_field_writer *dsvfw, const char *filename);
int ds_var_field_writer_write(struct ds_var_field_writer *dsvfw, const void *data, size_t data_size);
int ds_var_field_writer_append(struct ds_var_field_writer *dsvfw, const void *data, size_t data_size);
int ds_var_field_writer_close(struct ds_var_field_writer *dsvfw);

#include "../src/_ds_var_field.c"

#endif // _DS_VAR_FIELD__h_
