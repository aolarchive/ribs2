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
