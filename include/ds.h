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
#ifndef _DS__H_
#define _DS__H_

#include "ribs_defs.h"
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdio.h>
#include <string.h>
#include "file_writer.h"
#include "template.h"
#include "logger.h"
#include "ds_var_field.h"
#include "var_index_container.h"

/* public interface */
#define DS_FIELD_INITIALIZER { NULL, 0 }
#define DS_FIELD(T) struct TEMPLATE(ds_field,T)
#define DS_FIELD_INIT(T,x,f) TEMPLATE_FUNC(ds_field,T,init)((x),f)
#define DS_FIELD_FREE(T,x) TEMPLATE_FUNC(ds_field,T,free)((x))
#define DS_FIELD_BEGIN(x) ((x)->mem)
#define DS_FIELD_END(x) (DS_FIELD_BEGIN(x) + (x)->num_elements)
#define DS_FIELD_GET_VAL(x,i) (*(DS_FIELD_BEGIN(x) + i))
#define DS_FIELD_NUM_ELEMENTS(x) ((x)->num_elements)

#define DS_FIELD_WRITER_INITIALIZER { FILE_WRITER_INITIALIZER, 0 }
#define DS_FIELD_WRITER(T) struct TEMPLATE(ds_field_writer,T)
#define DS_FIELD_WRITER_INIT(T,x,f) TEMPLATE_FUNC(ds_field_writer,T,init)((x),f)
#define DS_FIELD_WRITER_CLOSE(T,x) TEMPLATE_FUNC(ds_field_writer,T,close)((x))
#define DS_FIELD_WRITER_WRITE(T,x,v) TEMPLATE_FUNC(ds_field_writer,T,write)((x),(v))
#define DS_FIELD_WRITER_NUM_ELEMENTS(x) ((x)->num_elements)

/*
 * example:
 *   DS_FIELD(uint32_t) ds_my_field = DS_FIELD_INITIALIZER;
 *   DS_FIELD_INIT(uint32_t, &ds_my_field, "my_field.ds");
 *   printf("my_field = %u\n", DS_FIELD_GET_VAL(&ds_my_field, record));
 *
 * using begin and end:
 *   uint32_t *my_field = DS_FIELD_BEGIN(&ds_my_field);
 *   uint32_t *my_field1_end = DS_FIELD_END(&ds_my_field);
 */

/*
 * writer example:
 *   DS_FIELD_WRITER(uint32_t) ds_my_w_field = DS_FIELD_WRITER_INITIALIZER;
 *   DS_FIELD_WRITER_INIT(uint32_t, &ds_my_w_field, "my_field.ds");
 *   DS_FIELD_WRITER_WRITE(uint32_t, &ds_my_w_field, 1234);
 *   DS_FIELD_WRITER_CLOSE(uint32_t, &ds_my_w_field);
 */


/* internal stuff */
#ifdef T
#undef T
#endif

enum
{
    ds_type_int8_t,
    ds_type_uint8_t,
    ds_type_int16_t,
    ds_type_uint16_t,
    ds_type_int32_t,
    ds_type_uint32_t,
    ds_type_int64_t,
    ds_type_uint64_t,
    ds_type_float,
    ds_type_double,
};

#define _DS_TYPE(x) ds_type_ ## x
#define DS_TYPE(x) _DS_TYPE(x)
#define _DS_TYPE_TO_STR(x) case ds_type_ ## x: return STRINGIFY(DS_TYPE(x))

static inline const char *ds_type_to_string(int64_t t) {
    switch(t) {
        _DS_TYPE_TO_STR(int8_t);
        _DS_TYPE_TO_STR(uint8_t);
        _DS_TYPE_TO_STR(int16_t);
        _DS_TYPE_TO_STR(uint16_t);
        _DS_TYPE_TO_STR(int32_t);
        _DS_TYPE_TO_STR(uint32_t);
        _DS_TYPE_TO_STR(int64_t);
        _DS_TYPE_TO_STR(uint64_t);
        _DS_TYPE_TO_STR(float);
        _DS_TYPE_TO_STR(double);
    default:
        return "unknown";
    }
}

#define T int8_t
#include "ds_field.h"
#undef T

#define T uint8_t
#include "ds_field.h"
#undef T

#define T int16_t
#include "ds_field.h"
#undef T

#define T uint16_t
#include "ds_field.h"
#undef T

#define T int32_t
#include "ds_field.h"
#undef T

#define T uint32_t
#include "ds_field.h"
#undef T

#define T int64_t
#include "ds_field.h"
#undef T

#define T uint64_t
#include "ds_field.h"
#undef T

#define T float
#include "ds_field.h"
#undef T

#define T double
#include "ds_field.h"
#undef T

#endif // _DS__H_
