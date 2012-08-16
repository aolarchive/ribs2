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
#include "mysql_helper.h"
#include "logger.h"
#include "vmbuf.h"
#include <string.h>
#include <ctype.h>
#include "mysql_common.h"

static int report_error(struct mysql_helper *mysql_helper) {
    LOGGER_ERROR("%s", mysql_error(&mysql_helper->mysql));
    return -1;
}

static int report_stmt_error(struct mysql_helper *mysql_helper) {
    LOGGER_ERROR("%s, %s", mysql_error(&mysql_helper->mysql), mysql_stmt_error(mysql_helper->stmt));
    mysql_stmt_close(mysql_helper->stmt);
    mysql_helper->stmt = NULL;
    return -1;
}

int mysql_helper_connect(struct mysql_helper *mysql_helper, struct mysql_login_info *login_info) {
    mysql_init(&mysql_helper->mysql);
    if (NULL == mysql_real_connect(&mysql_helper->mysql,
                                   login_info->host,
                                   login_info->user,
                                   login_info->pass,
                                   login_info->db,
                                   login_info->port,
                                   NULL, 0))
        return report_error(mysql_helper);
    my_bool b_flag = 0;
    if (0 != mysql_options(&mysql_helper->mysql, MYSQL_REPORT_DATA_TRUNCATION, (const char *)&b_flag))
        return report_error(mysql_helper);
    b_flag = 1;
    if (0 != mysql_options(&mysql_helper->mysql, MYSQL_OPT_RECONNECT, (const char *)&b_flag))
        return report_error(mysql_helper);
    vmbuf_make(&mysql_helper->buf); // TODO: add initializer
    vmbuf_init(&mysql_helper->buf, 16384);
    return 0;
}

int mysql_helper_execute(struct mysql_helper *mysql_helper, const char *query, unsigned long *affected_rows) {
    if (0 != mysql_query(&mysql_helper->mysql, query))
        return report_error(mysql_helper);
    *affected_rows = (unsigned long)mysql_affected_rows(&mysql_helper->mysql);
    return 0;
}

int mysql_helper_select(struct mysql_helper *mysql_helper, const char *query, size_t query_len, const char *fields, ...) {
    int i;

    mysql_helper->stmt = mysql_stmt_init(&mysql_helper->mysql);
    if (!mysql_helper->stmt)
        return report_error(mysql_helper);

    if (0 != mysql_stmt_prepare(mysql_helper->stmt, query, query_len))
        return report_stmt_error(mysql_helper);

    MYSQL_RES *rs = mysql_stmt_result_metadata(mysql_helper->stmt);
    if (!rs)
        return report_stmt_error(mysql_helper);

    int nargs = strlen(fields);
    unsigned int n = mysql_num_fields(rs);
    if ((int)n != nargs)
        return LOGGER_ERROR("num args != num fields in query (%u != %u)", nargs, n), mysql_free_result(rs), -1;

    mysql_helper->num_fields = nargs;
    unsigned long qlength[n];
    MYSQL_FIELD *qfields = mysql_fetch_fields(rs);
    unsigned int j;

    const char *p;
    int ftypes[nargs];
    for (i=0, p = fields; *p; ++p, ++i) {
        ftypes[i] = qfields[i].type;
        char c = *p;
        c = tolower(c);
        switch(c) {
        case 'd':
            ftypes[i] = MYSQL_TYPE_LONG;
            break;
        case 's':
            ftypes[i] = MYSQL_TYPE_STRING;
            break;
        }
    }

    for (j = 0; j < n; ++j) {
        qlength[j] = ribs_mysql_get_storage_size(ftypes[j], qfields[j].length);
    }
    mysql_free_result(rs);

    MYSQL_BIND bind[nargs];
    size_t data_ofs = vmbuf_alloc_aligned(&mysql_helper->buf, sizeof(char **) * nargs);
    size_t length_ofs = vmbuf_alloc_aligned(&mysql_helper->buf, sizeof(unsigned long) * nargs);
    size_t error_ofs = vmbuf_alloc_aligned(&mysql_helper->buf, sizeof(my_bool) * nargs);
    size_t is_null_ofs = vmbuf_alloc_aligned(&mysql_helper->buf, sizeof(my_bool) * nargs);
    size_t is_str_ofs = vmbuf_alloc_aligned(&mysql_helper->buf, sizeof(mysql_helper->is_str[0]) * nargs);
    size_t str_ofs[nargs];
    /* allocate space for the strings */
    for (i=0, p = fields; *p; ++p, ++i) {
        char c = *p;
        c = tolower(c);
        switch(c) {
        case 's':
            str_ofs[i] = vmbuf_alloc_aligned(&mysql_helper->buf, qlength[i] + 1);
            break;
        }
    }

    mysql_helper->data = (char **)vmbuf_data_ofs(&mysql_helper->buf, data_ofs);
    mysql_helper->length = (unsigned long *)vmbuf_data_ofs(&mysql_helper->buf, length_ofs);
    mysql_helper->is_error = (my_bool *)vmbuf_data_ofs(&mysql_helper->buf, error_ofs);
    mysql_helper->is_null = (my_bool *)vmbuf_data_ofs(&mysql_helper->buf, is_null_ofs);
    mysql_helper->is_str = (int8_t *)vmbuf_data_ofs(&mysql_helper->buf, is_str_ofs);

    memset(mysql_helper->is_str, 0, sizeof(mysql_helper->is_str[0]) * nargs);
    memset(bind, 0, sizeof(MYSQL_BIND) * nargs);

    MYSQL_BIND *bind_ptr = bind;
    va_list ap;
    va_start(ap, fields);

    for (i=0, p = fields; *p; ++p, ++bind_ptr, ++i) {
        char c = *p;
        bind_ptr->is_unsigned = isupper(c) ? 1 : 0;
        bind_ptr->is_null = &mysql_helper->is_null[i];
        bind_ptr->error = &mysql_helper->is_error[i];
        bind_ptr->length = &mysql_helper->length[i];
        bind_ptr->buffer_type = ftypes[i];
        c = tolower(c);
        char **str;
        switch(c) {
        case 'd':
            bind_ptr->buffer = va_arg(ap, int *);
            bind_ptr->buffer_length = sizeof(int);
            break;
        case 's':
            str = va_arg(ap, char **);
            *str = vmbuf_data_ofs(&mysql_helper->buf, str_ofs[i]);
            bind_ptr->buffer = *str;
            bind_ptr->buffer_length = qlength[i];
            mysql_helper->is_str[i] = 1;
            break;
        }
        mysql_helper->data[i] = bind_ptr->buffer;
    }
    va_end(ap);

    // execute
    if (0 != mysql_stmt_execute(mysql_helper->stmt))
        return report_stmt_error(mysql_helper);

    // bind
    if (0 != mysql_stmt_bind_result(mysql_helper->stmt, bind))
        return report_stmt_error(mysql_helper);

    return 0;
}

int mysql_helper_free(struct mysql_helper *mysql_helper) {
    if (mysql_helper->stmt) {
        mysql_stmt_close(mysql_helper->stmt);
        mysql_helper->stmt = NULL;
    }
    mysql_close(&mysql_helper->mysql);
    vmbuf_free(&mysql_helper->buf);
    return 0;
}

int mysql_helper_fetch(struct mysql_helper *mysql_helper) {
    int err = mysql_stmt_fetch(mysql_helper->stmt);
    if (err != 0) {
        switch(err) {
        case MYSQL_NO_DATA:
            return 0;
        case MYSQL_DATA_TRUNCATED:
            LOGGER_ERROR_FUNC("MYSQL_DATA_TRUNCATED");
            return -1;
        default:
            LOGGER_ERROR_FUNC("unknown mysql error: %d", err);
            return report_stmt_error(mysql_helper);
        }
        return 0;
    }
    int i;
    for (i = 0; i < mysql_helper->num_fields; ++i) {
        if (mysql_helper->is_str[i])
            mysql_helper->data[i][mysql_helper->length[i]] = 0;
    }
    return 1;
}


