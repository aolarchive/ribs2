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
#ifndef _MYSQL_HELPER__H_
#define _MYSQL_HELPER__H_

#include "ribs_defs.h"
#include "vmbuf.h"
#include "logger.h"
#include <mysql/mysql.h>
#include "vmbuf.h"
#include "mysql_common.h"

struct mysql_helper {
    MYSQL mysql;
    MYSQL_STMT *stmt;
    struct vmbuf buf;
    struct vmbuf time_buf;
    char **data;
    unsigned long *length;
    my_bool *is_error;
    my_bool *is_null;
    int8_t *is_str;

    int num_fields;
    int8_t is_connected;
};

struct mysql_helper_column_map {
    const char *name;
    struct {
        unsigned type : 7;
        unsigned is_unsigned : 1;
        uint8_t mysql_type;
        uint8_t size;
    } meta;
    union {
        int8_t *i8;
        uint8_t *u8;
        int16_t *i16;
        uint16_t *u16;
        int32_t *i32;
        uint32_t *u32;
        int64_t *i64;
        uint64_t *u64;
        double *dbl;
        struct tm *ts;
        uint64_t *ts_unix;
        char **str;
        const char **cstr;
        void *ptr;
        uintptr_t ofs;
        const char *custom_str; /* special case */
    } data;
};

#define MYSQL_HELPER_COL_MAP_GET_STR(d,m) (*((char **)((d) + (m)->data.ofs)))

/* field declaration helper macros */
#define _MYSQL_HELPER_COL_MAP(type,name,is_un,mysqlt,field)             \
    {#name,{mysql_helper_field_type_##field,is_un,mysqlt,sizeof(((struct mysql_helper_column_map *)0)->data.field[0])},{.field=&((type *)0)->name}}

#define MYSQL_HELPER_COL_MAP_I8(type,name) _MYSQL_HELPER_COL_MAP(type, name, 0, MYSQL_TYPE_TINY, i8)
#define MYSQL_HELPER_COL_MAP_U8(type,name) _MYSQL_HELPER_COL_MAP(type, name, 1, MYSQL_TYPE_TINY, u8)
#define MYSQL_HELPER_COL_MAP_I16(type,name) _MYSQL_HELPER_COL_MAP(type, name, 0, MYSQL_TYPE_SHORT, i16)
#define MYSQL_HELPER_COL_MAP_U16(type,name) _MYSQL_HELPER_COL_MAP(type, name, 1, MYSQL_TYPE_SHORT, u16)
#define MYSQL_HELPER_COL_MAP_I32(type,name) _MYSQL_HELPER_COL_MAP(type, name, 0, MYSQL_TYPE_LONG, i32)
#define MYSQL_HELPER_COL_MAP_U32(type,name) _MYSQL_HELPER_COL_MAP(type, name, 1, MYSQL_TYPE_LONG, u32)
#define MYSQL_HELPER_COL_MAP_I64(type,name) _MYSQL_HELPER_COL_MAP(type, name, 0, MYSQL_TYPE_LONGLONG, i64)
#define MYSQL_HELPER_COL_MAP_U64(type,name) _MYSQL_HELPER_COL_MAP(type, name, 1, MYSQL_TYPE_LONGLONG, u64)
#define MYSQL_HELPER_COL_MAP_DBL(type,name) _MYSQL_HELPER_COL_MAP(type, name, 0, MYSQL_TYPE_DOUBLE, dbl)
#define MYSQL_HELPER_COL_MAP_TS(type,name) _MYSQL_HELPER_COL_MAP(type, name, 0, MYSQL_TYPE_DATETIME, ts)
#define MYSQL_HELPER_COL_MAP_TS_UNIX(type,name) _MYSQL_HELPER_COL_MAP(type, name, 0, MYSQL_TYPE_LONGLONG, ts_unix)
#define MYSQL_HELPER_COL_MAP_STR(type,name) _MYSQL_HELPER_COL_MAP(type, name, 0, MYSQL_TYPE_STRING, str)
#define MYSQL_HELPER_COL_MAP_CSTR(type,name) _MYSQL_HELPER_COL_MAP(type, name, 0, MYSQL_TYPE_STRING, cstr)
// NOTE: Don't use the following macro for col binding!
#define MYSQL_HELPER_COL_MAP_CUSTOM_STR(name,value) \
    {name,{mysql_helper_field_type_cstr,0,MYSQL_TYPE_STRING,0},{.custom_str=value}}

#define MYSQL_HELPER_COL_MAP_ADD(name, class, type, buf)                \
    {                                                                   \
        struct mysql_helper_column_map __field__ =                      \
            MYSQL_HELPER_COL_MAP_##type(class, name);                   \
        vmbuf_memcpy(&buf, &__field__, sizeof(__field__));              \
    }

#define MYSQL_HELPER_COL_MAP_ADD_IF(name, class, type, buf)             \
    if (name && *name)                                                  \
        MYSQL_HELPER_COL_MAP_ADD(name, class, type, buf)

void mysql_helper_connect_init(struct mysql_helper *mysql_helper);
int mysql_helper_real_connect(struct mysql_helper *mysql_helper, struct mysql_login_info *login_info);
int mysql_helper_connect(struct mysql_helper *mysql_helper, struct mysql_login_info *login_info);
int mysql_helper_stmt(struct mysql_helper *mysql_helper, const char *query, size_t query_len, const char *params, const char *fields, ...);
int mysql_helper_vstmt(struct mysql_helper *mysql_helper, const char *query, size_t query_len, const char *params, const char *fields, void ** input);
int mysql_helper_hstmt(struct mysql_helper *helper, const char *query, size_t query_len, const char *input_param_types, void **input_params, const char *output_field_types, ...);
int mysql_helper_stmt_col_map(struct mysql_helper *mysql_helper,
                              const char *query, size_t query_len,
                              void *params, struct mysql_helper_column_map *params_map, size_t nparams, /* primary params */
                              void *sparams, struct mysql_helper_column_map *sparams_map, size_t nsparams, /* secondary params, used in insert ... where sparam=?... */
                              void *result, struct mysql_helper_column_map *result_map, size_t nresult);
/* executes arbitrary queries */
int mysql_helper_execute(struct mysql_helper *mysql_helper, const char *query, unsigned long *ar);
int mysql_helper_free(struct mysql_helper *mysql_helper);
int mysql_helper_fetch(struct mysql_helper *mysql_helper);
int mysql_helper_tx_begin(struct mysql_helper *mysql_helper);
int mysql_helper_tx_commit(struct mysql_helper *mysql_helper);
int mysql_helper_tx_rollback(struct mysql_helper *mysql_helper);
#define mysql_helper_last_insert_id(helper) mysql_insert_id(&((helper)->mysql))
#define mysql_helper_affected_rows(helper) mysql_affected_rows(&((helper)->mysql))
#define mysql_helper_warning_counts(helper) mysql_warning_count(&((helper)->mysql))
#define mysql_helper_mysql_error(helper) mysql_error(&((helper)->mysql))
void mysql_helper_generate_select(struct vmbuf *outbuf, const char *table,
                                  struct mysql_helper_column_map *columns, size_t n);
int mysql_helper_generate_insert(struct vmbuf *outbuf, const char *table,
                                 struct mysql_helper_column_map *params, size_t nparams,
                                 struct mysql_helper_column_map *fixed_values, size_t nfixed_values);
int mysql_helper_generate_update(struct vmbuf *outbuf, const char *table,
                                 struct mysql_helper_column_map *params, size_t nparams,
                                 struct mysql_helper_column_map *fixed_values, size_t nfixed_values,
                                 struct mysql_helper_column_map *wparams, size_t nwparams);
/*
 * internal stuff
 */
enum {
    mysql_helper_field_type_i8,
    mysql_helper_field_type_u8,
    mysql_helper_field_type_i16,
    mysql_helper_field_type_u16,
    mysql_helper_field_type_i32,
    mysql_helper_field_type_u32,
    mysql_helper_field_type_i64,
    mysql_helper_field_type_u64,
    mysql_helper_field_type_dbl,
    mysql_helper_field_type_str,
    mysql_helper_field_type_cstr,
    mysql_helper_field_type_ts,
    mysql_helper_field_type_ts_unix
};

/* get data from field */
#define _MYSQL_HELPER_FIELD_TYPE_TO_STR(T) case mysql_helper_field_type_##T: return #T

static inline const char *_mysql_helper_field_type_to_str(int type) {
    switch(type) {
        _MYSQL_HELPER_FIELD_TYPE_TO_STR(i8);
        _MYSQL_HELPER_FIELD_TYPE_TO_STR(u8);
        _MYSQL_HELPER_FIELD_TYPE_TO_STR(i16);
        _MYSQL_HELPER_FIELD_TYPE_TO_STR(u16);
        _MYSQL_HELPER_FIELD_TYPE_TO_STR(i32);
        _MYSQL_HELPER_FIELD_TYPE_TO_STR(u32);
        _MYSQL_HELPER_FIELD_TYPE_TO_STR(i64);
        _MYSQL_HELPER_FIELD_TYPE_TO_STR(u64);
        _MYSQL_HELPER_FIELD_TYPE_TO_STR(dbl);
        _MYSQL_HELPER_FIELD_TYPE_TO_STR(str);
        _MYSQL_HELPER_FIELD_TYPE_TO_STR(cstr);
        _MYSQL_HELPER_FIELD_TYPE_TO_STR(ts);
        _MYSQL_HELPER_FIELD_TYPE_TO_STR(ts_unix);
        default: return "unknown";
    }
}

#endif // _MYSQL_HELPER__H_
