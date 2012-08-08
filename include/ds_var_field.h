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
#ifndef _DS_VAR_FIELD__h_
#define _DS_VAR_FIELD__h_

#include "ribs_defs.h"
#include "file_mapper.h"

#define DS_VAR_FIELD_INITIALIZER { FILE_MAPPER_INITIALIZER, FILE_MAPPER_INITIALIZER, 0 }

struct ds_var_field {
    struct file_mapper ofs_table;
    struct file_mapper data;
    size_t num_elements;
};

int ds_var_field_init(struct ds_var_field *dsvf, const char *filename);
int ds_var_field_free(struct ds_var_field *dsvf);
_RIBS_INLINE_ int ds_var_field_get(struct ds_var_field *dsvf, size_t index, char **ret_ptr, size_t *ret_size);
_RIBS_INLINE_ const char *ds_var_field_get_cstr(struct ds_var_field *dsvf, size_t index);

#include "../src/_ds_var_field.c"

#endif // _DS_VAR_FIELD__h_
