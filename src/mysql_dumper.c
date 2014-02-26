/*
    This file is part of RIBS2.0 (Robust Infrastructure for Backend Systems).
    RIBS is an infrastructure for building great SaaS applications (but not
    limited to).

    Copyright (C) 2012,2013,2014 Adap.tv, Inc.

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
#include "mysql_dumper.h"
#include "logger.h"
#include "vmbuf.h"
#include "vmfile.h"
#include "file_writer.h"
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "ds.h"
#include "hashtable.h"
#include "file_utils.h"
#include "mysql_common.h"

#define IS_UNSIGNED(f)  ((f) & UNSIGNED_FLAG)

static int report_error(MYSQL *mysql) {
    LOGGER_ERROR("(%d) %s", mysql_errno(mysql), mysql_error(mysql));
    mysql_close(mysql);
    return -1;
}

static int report_stmt_error(MYSQL *mysql, MYSQL_STMT *stmt) {
    LOGGER_ERROR("%s, %s", mysql_error(mysql), mysql_stmt_error(stmt));
    mysql_stmt_close(stmt);
    mysql_close(mysql);
    return -1;
}

static int is_var_length_field(int type) {
    switch(type) {
    case MYSQL_TYPE_VARCHAR:
    case MYSQL_TYPE_VAR_STRING:
    case MYSQL_TYPE_STRING:
    case MYSQL_TYPE_BLOB:
    case MYSQL_TYPE_TINY_BLOB:
    case MYSQL_TYPE_MEDIUM_BLOB:
    case MYSQL_TYPE_LONG_BLOB:
    case MYSQL_TYPE_DECIMAL:
    case MYSQL_TYPE_NEWDECIMAL:
    case MYSQL_TYPE_ENUM:
    case MYSQL_TYPE_SET:
    case MYSQL_TYPE_BIT:
        return 1;
    default:
        return 0;
    }
}

#define MYSQL_TYPE_TO_DS(m, is_un, ts, tus)         \
    case m:                                         \
    return is_un ? ds_type_##tus : ds_type_##ts;

static int get_ds_type(int type, int is_unsigned) {
    switch(type) {
        MYSQL_TYPE_TO_DS(MYSQL_TYPE_TINY, is_unsigned, int8_t, uint8_t);
        MYSQL_TYPE_TO_DS(MYSQL_TYPE_SHORT, is_unsigned, int16_t, uint16_t);
        MYSQL_TYPE_TO_DS(MYSQL_TYPE_LONG, is_unsigned, int32_t, uint32_t);
        MYSQL_TYPE_TO_DS(MYSQL_TYPE_LONGLONG, is_unsigned, int64_t, uint64_t);
        MYSQL_TYPE_TO_DS(MYSQL_TYPE_FLOAT, is_unsigned, float, float);
        MYSQL_TYPE_TO_DS(MYSQL_TYPE_DOUBLE, is_unsigned, double, double);
        MYSQL_TYPE_TO_DS(MYSQL_TYPE_INT24, is_unsigned, int32_t, uint32_t);
        MYSQL_TYPE_TO_DS(MYSQL_TYPE_YEAR, is_unsigned, uint16_t, uint16_t);
    }
    return -1;
}

#define DS_TYPE_MAPPER_ENUM_TO_STR(t) case ds_type_##t: return (#t);

static const char *get_ds_type_str(int e) {
    switch(e) {
        DS_TYPE_MAPPER_ENUM_TO_STR(int8_t);
        DS_TYPE_MAPPER_ENUM_TO_STR(uint8_t);
        DS_TYPE_MAPPER_ENUM_TO_STR(int16_t);
        DS_TYPE_MAPPER_ENUM_TO_STR(uint16_t);
        DS_TYPE_MAPPER_ENUM_TO_STR(int32_t);
        DS_TYPE_MAPPER_ENUM_TO_STR(uint32_t);
        DS_TYPE_MAPPER_ENUM_TO_STR(int64_t);
        DS_TYPE_MAPPER_ENUM_TO_STR(uint64_t);
        DS_TYPE_MAPPER_ENUM_TO_STR(float);
        DS_TYPE_MAPPER_ENUM_TO_STR(double);
    }
    return "N/A";
}


int mysql_dumper_dump(struct mysql_login_info *mysql_login_info, const char *outputdir, const char *dbname, const char *tablename, const char *query, size_t query_len, struct mysql_dumper_type *types) {
    MYSQL mysql;
    MYSQL_STMT *stmt = NULL;
    mysql_init(&mysql);
    my_bool b_flag = 1;
    if (0 != mysql_options(&mysql, MYSQL_OPT_RECONNECT, (const char *)&b_flag))
        return report_error(&mysql);

    if (NULL == mysql_real_connect(&mysql, mysql_login_info->host, mysql_login_info->user, mysql_login_info->pass, mysql_login_info->db, mysql_login_info->port, NULL, CLIENT_COMPRESS))
        return report_error(&mysql);

    b_flag = 0;
    if (0 != mysql_options(&mysql, MYSQL_REPORT_DATA_TRUNCATION, (const char *)&b_flag))
        return report_error(&mysql);

    stmt = mysql_stmt_init(&mysql);
    if (!stmt)
        return report_error(&mysql);

    if (0 != mysql_stmt_prepare(stmt, query, query_len))
        return report_stmt_error(&mysql, stmt);

    MYSQL_RES *rs = mysql_stmt_result_metadata(stmt);
    if (!rs)
        return report_stmt_error(&mysql, stmt);

    unsigned int n = mysql_num_fields(rs);
    MYSQL_FIELD *fields = mysql_fetch_fields(rs);
    int field_types[n];
    MYSQL_BIND bind[n];
    my_bool is_null[n];
    unsigned long length[n];
    my_bool error[n];
    memset(bind, 0, sizeof(MYSQL_BIND) * n);
    int null_terminate_str[n];
    memset(null_terminate_str, 0, sizeof(int) * n);

    struct file_writer ffields[n];
    struct ds_var_field_writer vfields[n];

    struct vmbuf buf = VMBUF_INITIALIZER;
    vmbuf_init(&buf, 4096);
    vmbuf_sprintf(&buf, "%s/%s/%s/schema.txt", outputdir, dbname, tablename);
    mkdir_for_file_recursive(vmbuf_data(&buf));
    int fdschema = creat(vmbuf_data(&buf), 0644);
    struct vmfile ds_txt = VMFILE_INITIALIZER;
    vmbuf_reset(&buf);
    vmbuf_sprintf(&buf, "%s/%s/%s/ds.txt", outputdir, dbname, tablename);
    if (0 > vmfile_init(&ds_txt, vmbuf_data(&buf), 4096))
        return LOGGER_ERROR("failed to create: %s", vmbuf_data(&buf)), vmbuf_free(&buf), -1;
    vmfile_sprintf(&ds_txt, "DS_LOADER_BEGIN()\n");
    vmfile_sprintf(&ds_txt, "/*\n * DB: %s\n */\n", dbname);
    vmfile_sprintf(&ds_txt, "#undef DB_NAME\n#define DB_NAME %s\n", dbname);
    ssize_t len = 80 - strlen(tablename);
    if (len < 0) len = 4;
    char header[len];
    memset(header, '=', len);
    vmfile_sprintf(&ds_txt, "/* %.*s[ %s ]%.*s */\n", (int)len / 2, header, tablename, (int)(len - (len / 2)), header);
    vmfile_sprintf(&ds_txt, "#   undef TABLE_NAME\n#   define TABLE_NAME %s\n", tablename);
    /*
     * initialize output files
     */
    unsigned int i;
    for (i = 0; i < n; ++i) {
        file_writer_make(&ffields[i]);
        DS_VAR_FIELD_WRITER_INIT(vfields[i]);
    }

    struct hashtable ht_types = HASHTABLE_INITIALIZER;
    hashtable_init(&ht_types, 32);
    if (NULL != types) {
        struct mysql_dumper_type *t = types;
        for (; t->name; ++t) {
            /* storing ptr here which points to static str */
            hashtable_insert(&ht_types, t->name, strlen(t->name), t, sizeof(struct mysql_dumper_type));
        }
    }
    /*
     * parse meta data and construct bind array
     */
    int err = 0;
    for (i = 0; i < n; ++i) {
        field_types[i] = fields[i].type;
        bind[i].is_unsigned = IS_UNSIGNED(fields[i].flags);

        int64_t ds_type = -1;
        const char *ds_type_str = "VAR";
        /*
         * handle overrides
         */
        while (NULL != types) {
            uint32_t ofs = hashtable_lookup(&ht_types, fields[i].name, strlen(fields[i].name));
            if (!ofs) break;
            struct mysql_dumper_type *type = (struct mysql_dumper_type *)hashtable_get_val(&ht_types, ofs);
            null_terminate_str[i] = MYSQL_DUMPER_CSTR & type->flags;
            if (type->mysql_type)
                field_types[i] = type->mysql_type;
            bind[i].is_unsigned = (type->flags & MYSQL_DUMPER_UNSIGNED) > 0 ? 1 : 0;
            break;
        }
        vmbuf_reset(&buf);
        vmbuf_sprintf(&buf, "%s/%s/%s/%s", outputdir, dbname, tablename, fields[i].name);

        mkdir_for_file_recursive(vmbuf_data(&buf));
        if (is_var_length_field(field_types[i])) {
            if (0 > (err = ds_var_field_writer_init(&vfields[i], vmbuf_data(&buf))))
                break;
        } else {
            ds_type = get_ds_type(field_types[i], bind[i].is_unsigned);
            const char *s = get_ds_type_str(ds_type);
            if (*s) ds_type_str = s;
            if (0 > (err = file_writer_init(&ffields[i], vmbuf_data(&buf))) ||
                0 > (err = file_writer_write(&ffields[i], &ds_type, sizeof(ds_type))))
                break;;
        }
        len = ribs_mysql_get_storage_size(field_types[i], fields[i].length);
        if (fdschema > 0)
            dprintf(fdschema, "%03d  name = %s, size=%zu, length=%lu, type=%s (%s), is_prikey=%d, ds_type=%s\n", i, fields[i].name, len, fields[i].length, ribs_mysql_get_type_name(field_types[i]), bind[i].is_unsigned ? "unsigned" : "signed", IS_PRI_KEY(fields[i].flags), ds_type_str);
        if (is_var_length_field(field_types[i])) {
            vmfile_sprintf(&ds_txt, "        DS_VAR_FIELD_LOADER(%s)\n", fields[i].name);
        } else {
            vmfile_sprintf(&ds_txt, "        DS_FIELD_LOADER(%s, %s)\n", ds_type_str, fields[i].name);
        }
        bind[i].buffer_type = field_types[i];
        bind[i].buffer_length = len;
        bind[i].buffer = malloc(len);
        bind[i].is_null = &is_null[i];
        bind[i].length = &length[i];
        bind[i].error = &error[i];
    }
    hashtable_free(&ht_types);
    mysql_free_result(rs);
    close(fdschema);
    //vmfile_sprintf(&ds_txt, "/*\n * TABLE END: %s\n */\n", tablename);
    vmfile_sprintf(&ds_txt, "DS_LOADER_END()\n");
    vmfile_close(&ds_txt);
    /*
     * execute & bind
     */
    if (0 != err ||
        0 != mysql_stmt_execute(stmt) ||
        0 != mysql_stmt_bind_result(stmt, bind)) {
        err = -1;
        report_stmt_error(&mysql, stmt);
        goto dumper_close_writer;
    }
    char zeros[4096];
    memset(zeros, 0, sizeof(zeros));
    int mysql_err = 0;
    size_t count = 0, num_rows_errors = 0;
    /*
     * write all rows to output files
     */
    while (0 == (mysql_err = mysql_stmt_fetch(stmt))) {
        int b = 0;
        for (i = 0; i < n && !b; ++i)
            b = b || error[i];
        if (b) {
            ++num_rows_errors;
            continue;
        }
        for (i = 0; i < n; ++i) {
            if (is_var_length_field(field_types[i])) {
                if (0 > (err = ds_var_field_writer_write(&vfields[i], is_null[i] ? NULL : bind[i].buffer, is_null[i] ? 0 : length[i])))
                    goto dumper_error;
                if (null_terminate_str[i]) {
                    const char c = '\0';
                    if (0 > (err = ds_var_field_writer_append(&vfields[i], &c, sizeof(c))))
                        goto dumper_error;
                }
            } else {
                if (0 > (err = file_writer_write(&ffields[i], is_null[i] ? zeros : bind[i].buffer, bind[i].buffer_length)))
                    goto dumper_error;
            }
        }
        ++count;
    }
    /* no dumper errors */
    goto dumper_ok;
 dumper_error:
    LOGGER_ERROR("failed to write data, aborting");
 dumper_ok:
    /* we are done with mysql, close it */
    mysql_stmt_close(stmt);
    mysql_close(&mysql);
    LOGGER_INFO("%s: %zu records, %zu skipped", tablename, count, num_rows_errors);
    /* check for mysql errors */
    if (mysql_err != MYSQL_NO_DATA) {
        LOGGER_ERROR("mysql_stmt_fetch returned an error (code=%d)", mysql_err);
        err = -1;
    }
 dumper_close_writer:
    /*
     * finalize & free memory
     */
    for (i = 0; i < n; ++i) {
        if (is_var_length_field(field_types[i])) {
            if (0 > (err = ds_var_field_writer_close(&vfields[i])))
                LOGGER_ERROR("failed to write offset");
        } else {
            file_writer_close(&ffields[i]);
        }
        free(bind[i].buffer);
    }
    vmbuf_free(&buf);
    return err;
}
