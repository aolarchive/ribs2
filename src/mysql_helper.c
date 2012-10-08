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

static int report_error(struct mysql_helper *mysql_helper) {
    LOGGER_ERROR("(MySQL errno: %d) %s", mysql_errno(&mysql_helper->mysql), mysql_error(&mysql_helper->mysql));
    return -1;
}

static int report_stmt_error(struct mysql_helper *mysql_helper) {
    LOGGER_ERROR("%s, %s", mysql_error(&mysql_helper->mysql), mysql_stmt_error(mysql_helper->stmt));
    mysql_stmt_close(mysql_helper->stmt);
    mysql_helper->stmt = NULL;
    return -1;
}

int mysql_helper_connect(struct mysql_helper *mysql_helper, struct mysql_login_info *login_info) {
    memset(mysql_helper, 0, sizeof(struct mysql_helper));
    mysql_init(&mysql_helper->mysql);
    if (NULL == mysql_real_connect(&mysql_helper->mysql,
                                   login_info->host,
                                   login_info->user,
                                   login_info->pass,
                                   login_info->db,
                                   login_info->port,
                                   NULL, 0))
        return report_error(mysql_helper);
    my_bool b_flag = 1;
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

int mysql_helper_tx_begin(struct mysql_helper *mysql_helper)
{
    if (0 != mysql_autocommit(&mysql_helper->mysql, 0))
        return report_error(mysql_helper);
    return 0;
}

int mysql_helper_tx_commit(struct mysql_helper *mysql_helper)
{
    if (0 != mysql_commit(&mysql_helper->mysql))
        return report_error(mysql_helper);
    return 0;
}

int mysql_helper_tx_rollback(struct mysql_helper *mysql_helper)
{
    if (0 != mysql_rollback(&mysql_helper->mysql))
        return report_error(mysql_helper);
    return 0;
}

/*
 * Executes the given SQL query after binding the parameters as indicated
 * by 'params'. The results are bound to the provided pointers with order
 * and types described by 'fields'. Number of var args should match the
 * combined length of 'params' and 'fields'. The number of 'params' must
 * be exactly the same as the number of ?s in the query.
 *
 * Valid format specifies:
 *    d:  MYSQL_TYPE_LONG (signed)
 *    D:  MYSQL_TYPE_LONG (unsigned)
 *    s:  MYSQL_TYPE_STRING
 *    S:  MYSQL_TYPE_STRING, buffer size followed by pointer to the data
 *
 * Examples:
 *     SSTR(query, "SELECT name FROM table WHERE id = ?");
 *     mysql_helper_stmt(&mh, query, SSTRLEN(query), "d", "s", &id, &name);
 *
 *     SSTR(query, "UPDATE table SET name = ? WHERE id = ?");
 *     mysql_helper_stmt(&mh, query, SSTRLEN(query), "sd", "", new_name, &id);
 *
 *     SSTR(query, "INSERT INTO table (status, name) VALUES (?, ?)");
 *     mysql_helper_stmt(&mh, query, SSTRLEN(query), "ss", "", status, name);
 */
int mysql_helper_stmt(struct mysql_helper *mysql_helper,
                      const char *query,
                      size_t query_len,
                      const char *params,
                      const char *fields,
                      ...)
{
    /*
     * 0. Cleanup from previous call
     */
    if (mysql_helper->stmt) {
        mysql_stmt_close(mysql_helper->stmt);
        mysql_helper->stmt = NULL;
        vmbuf_reset(&mysql_helper->buf);
    }
    /*
     * 1. Prepare statement
     */
    mysql_helper->stmt = mysql_stmt_init(&mysql_helper->mysql);
    if (!mysql_helper->stmt)
        return report_error(mysql_helper);

    if (0 != mysql_stmt_prepare(mysql_helper->stmt, query, query_len))
        return report_stmt_error(mysql_helper);
    /*
     * 2. Bind parameters if there are any
     */
    uint32_t nparams = strlen(params);
    uint32_t n = mysql_stmt_param_count(mysql_helper->stmt);
    if (nparams != n) {
        LOGGER_ERROR("num params != num ?s in query (%u != %u)", nparams, n);
        return -1;
    }
    const char *p;
    uint32_t i;
    va_list ap;
    va_start(ap, fields);
    /* TODO: move to internal vmbuf (mysql_helper), so
       mysql_stmt_execute can be called multiple times when inserting
       data */
    size_t plengths[nparams];
    int ptypes[nparams];
    my_bool pnulls[nparams];
    MYSQL_BIND pbind[nparams];

    if (nparams > 0) {
        memset(pbind, 0, sizeof(pbind));
        MYSQL_BIND *pbind_ptr = pbind;
        for (i = 0, p = params; *p; ++p, ++pbind_ptr, ++i) {
            char c = *p;
            char *str;
            switch(tolower(c)) {
            case 'd':
                ptypes[i] = MYSQL_TYPE_LONG;
                pbind_ptr->buffer = va_arg(ap, int *);
                pnulls[i] = (pbind_ptr->buffer == NULL);
                pbind_ptr->is_unsigned = isupper(c) ? 1 : 0;
                break;
            case 's':
                ptypes[i] = MYSQL_TYPE_STRING;
                if (isupper(c)) {
                    plengths[i] = va_arg(ap, size_t);
                    str = va_arg(ap, char *);
                } else {
                    str = va_arg(ap, char *);
                    plengths[i] = strlen(str);
                }
                pnulls[i] = (str == NULL);
                pbind_ptr->buffer = str;
                pbind_ptr->buffer_length = plengths[i];
                pbind_ptr->length = &plengths[i];
                break;
            }
            pbind_ptr->buffer_type = ptypes[i];
            pbind_ptr->is_null = &pnulls[i];
        }

        if (0 != mysql_stmt_bind_param(mysql_helper->stmt, pbind))
            return report_stmt_error(mysql_helper);
    }
    /*
     * 3. Prepare result field bindings
     */
    uint32_t nfields = strlen(fields);
    MYSQL_BIND bind[nfields];
    unsigned long qlength[nfields];
    int ftypes[nfields];
    size_t str_ofs[nfields];

    if (nfields > 0) {
        MYSQL_RES *rs = mysql_stmt_result_metadata(mysql_helper->stmt);
        if (!rs)
            return report_stmt_error(mysql_helper);
        n = mysql_num_fields(rs);
        if (n != nfields) {
            LOGGER_ERROR("num args != num fields in query (%u != %u)", nfields, n);
            mysql_free_result(rs);
            return -1;
        }
        mysql_helper->num_fields = n;
        MYSQL_FIELD *qfields = mysql_fetch_fields(rs);
        for (i = 0, p = fields; *p; ++p, ++i) {
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
            qlength[i] = ribs_mysql_get_storage_size(ftypes[i], qfields[i].length);
        }
        mysql_free_result(rs);
        size_t data_ofs = vmbuf_alloc_aligned(&mysql_helper->buf, sizeof(char **) * nfields);
        size_t length_ofs = vmbuf_alloc_aligned(&mysql_helper->buf, sizeof(unsigned long) * nfields);
        size_t error_ofs = vmbuf_alloc_aligned(&mysql_helper->buf, sizeof(my_bool) * nfields);
        size_t is_null_ofs = vmbuf_alloc_aligned(&mysql_helper->buf, sizeof(my_bool) * nfields);
        size_t is_str_ofs = vmbuf_alloc_aligned(&mysql_helper->buf, sizeof(mysql_helper->is_str[0]) * nfields);
        /* allocate space for the strings */
        for (i = 0, p = fields; *p; ++p, ++i) {
            switch(tolower(*p)) {
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
        memset(mysql_helper->is_str, 0, sizeof(mysql_helper->is_str[0]) * nfields);
        memset(bind, 0, sizeof(MYSQL_BIND) * nfields);
        MYSQL_BIND *bind_ptr = bind;
        for (i = 0, p = fields; *p; ++p, ++bind_ptr, ++i) {
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
    }
    va_end(ap);
    /*
     * 4. Execute the query
     */
    if (0 != mysql_stmt_execute(mysql_helper->stmt))
        return report_stmt_error(mysql_helper);
    /*
     * 5. Bind result fields
     */
    if (nfields > 0 && 0 != mysql_stmt_bind_result(mysql_helper->stmt, bind))
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


