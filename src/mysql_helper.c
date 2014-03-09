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
#include "mysql_helper.h"
#include "logger.h"
#include "vmbuf.h"
#include <string.h>
#include <ctype.h>
#include <time.h>

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

void mysql_helper_connect_init(struct mysql_helper *mysql_helper){
    memset(mysql_helper, 0, sizeof(struct mysql_helper));
    mysql_init(&mysql_helper->mysql);
}

int mysql_helper_real_connect(struct mysql_helper *mysql_helper, struct mysql_login_info *login_info){
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

    VMBUF_INIT(mysql_helper->buf);
    vmbuf_init(&mysql_helper->buf, 16384);
    VMBUF_INIT(mysql_helper->time_buf);
    vmbuf_init(&mysql_helper->time_buf, 4096); /* maximum of 102 timestamp bindings */
    return 0;
}

int mysql_helper_connect(struct mysql_helper *mysql_helper, struct mysql_login_info *login_info) {
    mysql_helper_connect_init(mysql_helper);
    return mysql_helper_real_connect(mysql_helper, login_info);
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
 *    f:  MYSQL_TYPE_DOUBLE
 *    s:  MYSQL_TYPE_STRING
 *    S:  MYSQL_TYPE_STRING, buffer size followed by pointer to the data
 *
 *    note that strings are (char *) for input parameters and (char **) for output parameters
 *
 * Examples:
 *     int id;
 *     char *name, *new_name, *status;
 *
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
    unsigned long plengths[nparams];
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
            case 'f':
                ptypes[i] = MYSQL_TYPE_DOUBLE;
                pbind_ptr->buffer = va_arg(ap, double *);
                pnulls[i] = (pbind_ptr->buffer == NULL);
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
            case 'f':
                ftypes[i] = MYSQL_TYPE_DOUBLE;
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
            case 'f':
                bind_ptr->buffer = va_arg(ap, double *);
                bind_ptr->buffer_length = sizeof(double);
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

int mysql_helper_vstmt(struct mysql_helper *mysql_helper,
                       const char *query,
                       size_t query_len,
                       const char *params,
                       const char *fields,
                       void **input)
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
        LOGGER_ERROR("num params != num params in query (%u != %u)", nparams, n);
        return -1;
    }
    const char *p;
    uint32_t i;

    /* TODO: move to internal vmbuf (mysql_helper), so
       mysql_stmt_execute can be called multiple times when inserting
       data */
    unsigned long plengths[nparams];
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
                pbind_ptr->buffer = (int *)(*input);
                pnulls[i] = (pbind_ptr->buffer == NULL);
                pbind_ptr->is_unsigned = isupper(c) ? 1 : 0;
                ++input;
                break;
            case 'l':
                ptypes[i] = MYSQL_TYPE_LONGLONG;
                pbind_ptr->buffer = (long long *)(*input);
                pnulls[i] = (pbind_ptr->buffer == NULL);
                pbind_ptr->is_unsigned = isupper(c) ? 1 : 0;
                ++input;
                break;
            case 'f':
                ptypes[i] = MYSQL_TYPE_DOUBLE;
                pbind_ptr->buffer = (double *)(*input);
                pnulls[i] = (pbind_ptr->buffer == NULL);
                ++input;
                break;
            case 's':
                ptypes[i] = MYSQL_TYPE_STRING;
                if (isupper(c)) {
                    plengths[i] = *(size_t *)input;
                    str = (char *)(*input);
                    ++input;
                } else {
                    str = (char *)(*input);
                    plengths[i] = strlen(str);
                }
                pnulls[i] = (str == NULL);
                pbind_ptr->buffer = str;
                pbind_ptr->buffer_length = plengths[i];
                pbind_ptr->length = &plengths[i];
                ++input;
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
            case 'l':
                ftypes[i] = MYSQL_TYPE_LONGLONG;
                break;
            case 'f':
                ftypes[i] = MYSQL_TYPE_DOUBLE;
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
                bind_ptr->buffer = (int *)(*input);
                bind_ptr->buffer_length = sizeof(int);
                ++input;
                break;
            case 'l':
                bind_ptr->buffer = (long long *)(*input);
                bind_ptr->buffer_length = sizeof(long long);
                ++input;
                break;
            case 'f':
                bind_ptr->buffer = (double *)(*input);
                bind_ptr->buffer_length = sizeof(double);
                ++input;
                break;
            case 's':
                str = (char **)(*input);
                *str = vmbuf_data_ofs(&mysql_helper->buf, str_ofs[i]);
                bind_ptr->buffer = *str;
                bind_ptr->buffer_length = qlength[i];
                mysql_helper->is_str[i] = 1;
                ++input;
                break;
            }
            mysql_helper->data[i] = bind_ptr->buffer;
        }
    }
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

static int _mysql_helper_init_bind_map(void *data, size_t n, struct mysql_helper_column_map *map, MYSQL_BIND *pbind, unsigned long *plengths, my_bool *pnulls, struct vmbuf *tb) {
    if (n == 0) return 0;
    memset(pbind, 0, sizeof(MYSQL_BIND) * n);
    struct mysql_helper_column_map *p;
    size_t i;
    MYSQL_BIND *pbind_ptr = pbind;
    MYSQL_TIME tmp;
    struct tm *t;
    for (i = 0, p = map; i < n; ++p, ++pbind_ptr, ++i) {
        pbind_ptr->buffer_type = p->meta.mysql_type;
        pbind_ptr->is_unsigned = p->meta.is_unsigned;
        switch(p->meta.type) {
        case mysql_helper_field_type_str:
        case mysql_helper_field_type_cstr:
            if (NULL != MYSQL_HELPER_COL_MAP_GET_STR(data,p)) {
                plengths[i] = strlen(MYSQL_HELPER_COL_MAP_GET_STR(data,p));
                pnulls[i] = 0;
            } else {
                plengths[i] = 0;
                pnulls[i] = 1;
            }
            pbind_ptr->buffer = MYSQL_HELPER_COL_MAP_GET_STR(data,p);
            pbind_ptr->buffer_length = plengths[i];
            pbind_ptr->length = &plengths[i];
            break;
        case mysql_helper_field_type_ts:
            t = (struct tm *)(data + p->data.ofs);
            tmp.year= t->tm_year + 1900;
            tmp.month= t->tm_mon + 1;
            tmp.day= t->tm_mday;
            tmp.hour= t->tm_hour;
            tmp.minute= t->tm_min;
            tmp.second= t->tm_sec;
            size_t ofs = vmbuf_wlocpos(tb);
            if (vmbuf_capacity(tb) < ofs + sizeof(tmp))
                return LOGGER_ERROR("too many timestamps to bind"), -1;
            vmbuf_memcpy(tb, &tmp, sizeof(tmp));
            pbind_ptr->buffer = (char *)((MYSQL_TIME *)vmbuf_data_ofs(tb, ofs));
            pbind_ptr->length = 0;
            pnulls[i] = 0;
            break;
        default:
            pbind_ptr->buffer = data + p->data.ofs;
            pnulls[i] = 0;
            plengths[i] = p->meta.size;
            pbind_ptr->buffer_length = plengths[i];
            pbind_ptr->length = &plengths[i];
        }
        pbind_ptr->is_null = pnulls + i;
    }
    return 0;
}

int mysql_helper_stmt_col_map(struct mysql_helper *mysql_helper,
                              const char *query, size_t query_len,
                              void *params, struct mysql_helper_column_map *params_map, size_t nparams, /* primary params */
                              void *sparams, struct mysql_helper_column_map *sparams_map, size_t nsparams, /* secondary params, used in insert ... where sparam=?... */
                              void *result, struct mysql_helper_column_map *result_map, size_t nresult) {
    /*
     * 0. Cleanup from previous call
     */
    if (mysql_helper->stmt) {
        mysql_stmt_close(mysql_helper->stmt);
        mysql_helper->stmt = NULL;
        vmbuf_reset(&mysql_helper->buf);
        vmbuf_reset(&mysql_helper->time_buf);
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
    size_t num_all_params = nparams + nsparams;
    uint32_t n = mysql_stmt_param_count(mysql_helper->stmt);
    if (num_all_params != n) {
        LOGGER_ERROR("num params != num params in query (%zu != %u)", num_all_params, n);
        return -1;
    }
    struct mysql_helper_column_map *p, *rend = result_map + nresult;
    uint32_t i;
    /* TODO: move to internal vmbuf (mysql_helper), so
       mysql_stmt_execute can be called multiple times when inserting
       data */
    unsigned long plengths[num_all_params];
    my_bool pnulls[num_all_params];
    MYSQL_BIND pbind[num_all_params];
    _mysql_helper_init_bind_map(params, nparams, params_map, pbind, plengths, pnulls, &mysql_helper->time_buf);
    _mysql_helper_init_bind_map(sparams, nsparams, sparams_map, pbind + nparams, plengths + nparams, pnulls + nparams, &mysql_helper->time_buf);
    if (num_all_params > 0 && 0 != mysql_stmt_bind_param(mysql_helper->stmt, pbind))
        return report_stmt_error(mysql_helper);
    /*
     * 3. Prepare result field bindings
     */
    MYSQL_BIND bind[nresult];
    size_t str_ofs[nresult];

    if (nresult > 0) {
        MYSQL_RES *rs = mysql_stmt_result_metadata(mysql_helper->stmt);
        if (!rs)
            return report_stmt_error(mysql_helper);
        n = mysql_num_fields(rs);
        if (n != nresult) {
            LOGGER_ERROR("num args != num fields in query (%zu != %u)", nresult, n);
            mysql_free_result(rs);
            return -1;
        }
        mysql_helper->num_fields = n;
        MYSQL_FIELD *qfields = mysql_fetch_fields(rs);
        size_t data_ofs = vmbuf_alloc_aligned(&mysql_helper->buf, sizeof(char **) * nresult);
        size_t length_ofs = vmbuf_alloc_aligned(&mysql_helper->buf, sizeof(unsigned long) * nresult);
        size_t error_ofs = vmbuf_alloc_aligned(&mysql_helper->buf, sizeof(my_bool) * nresult);
        size_t is_null_ofs = vmbuf_alloc_aligned(&mysql_helper->buf, sizeof(my_bool) * nresult);
        size_t is_str_ofs = vmbuf_alloc_aligned(&mysql_helper->buf, sizeof(mysql_helper->is_str[0]) * nresult);
        /* allocate space for the strings */
        for (i = 0, p = result_map; p != rend; ++p, ++i) {
            if (p->meta.type == mysql_helper_field_type_str ||
                p->meta.type == mysql_helper_field_type_cstr)
                str_ofs[i] = vmbuf_alloc_aligned(&mysql_helper->buf, qfields[i].length + 1);
        }
        mysql_helper->data = (char **)vmbuf_data_ofs(&mysql_helper->buf, data_ofs);
        mysql_helper->length = (unsigned long *)vmbuf_data_ofs(&mysql_helper->buf, length_ofs);
        mysql_helper->is_error = (my_bool *)vmbuf_data_ofs(&mysql_helper->buf, error_ofs);
        mysql_helper->is_null = (my_bool *)vmbuf_data_ofs(&mysql_helper->buf, is_null_ofs);
        mysql_helper->is_str = (int8_t *)vmbuf_data_ofs(&mysql_helper->buf, is_str_ofs);
        memset(mysql_helper->is_str, 0, sizeof(mysql_helper->is_str[0]) * nresult);
        memset(bind, 0, sizeof(MYSQL_BIND) * nresult);
        MYSQL_BIND *bind_ptr = bind;
        for (i = 0, p = result_map; p != rend; ++p, ++bind_ptr, ++i) {
            if (strcmp(qfields[i].name, p->name) != 0) {
                LOGGER_ERROR("column name mismatch, '%s' != '%s'", qfields[i].name, p->name);
                mysql_free_result(rs);
                return -1;
            }
            bind_ptr->is_unsigned = p->meta.is_unsigned;
            bind_ptr->is_null = &mysql_helper->is_null[i];
            bind_ptr->error = &mysql_helper->is_error[i];
            bind_ptr->length = &mysql_helper->length[i];
            bind_ptr->buffer_type = p->meta.mysql_type;
            switch(p->meta.type) {
            case mysql_helper_field_type_str:
            case mysql_helper_field_type_cstr:
                bind_ptr->buffer = vmbuf_data_ofs(&mysql_helper->buf, str_ofs[i]);
                MYSQL_HELPER_COL_MAP_GET_STR(result,p) = bind_ptr->buffer;
                bind_ptr->buffer_length = qfields[i].length;
                mysql_helper->is_str[i] = 1;
                break;
            default:
                bind_ptr->buffer = result + p->data.ofs;
                bind_ptr->buffer_length = p->meta.size;
            }
            mysql_helper->data[i] = bind_ptr->buffer;
        }
        mysql_free_result(rs);
    }
    /*
     * 4. Execute the query
     */
    if (0 != mysql_stmt_execute(mysql_helper->stmt))
        return report_stmt_error(mysql_helper);
    /*
     * 5. Bind result fields
     */
    if (nresult > 0 && 0 != mysql_stmt_bind_result(mysql_helper->stmt, bind))
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

void mysql_helper_generate_select(struct vmbuf *outbuf, const char *table, struct mysql_helper_column_map *columns, size_t n) {
    vmbuf_strcpy(outbuf, "SELECT ");
    size_t i;
    for (i = 0; i < n; ++i) {
        if (columns[i].meta.type == mysql_helper_field_type_ts_unix) {
            vmbuf_sprintf(outbuf, "UNIX_TIMESTAMP(`%s`),", columns[i].name);
        } else {
            vmbuf_sprintf(outbuf, "`%s`,", columns[i].name);
        }
    }
    vmbuf_remove_last_if(outbuf, ',');
    vmbuf_sprintf(outbuf, " FROM %s", table);
}

int mysql_helper_generate_insert(struct vmbuf *outbuf, const char *table,
                                 struct mysql_helper_column_map *params, size_t nparams,
                                 struct mysql_helper_column_map *fixed_values, size_t nfixed_values) {
    size_t i;
    vmbuf_sprintf(outbuf, "INSERT INTO `%s` (", table);
    for (i = 0; i < nparams; ++i) {
        vmbuf_sprintf(outbuf, "%s,", params[i].name);
    }
    for (i = 0; i < nfixed_values; ++i) {
        vmbuf_sprintf(outbuf, "%s,", fixed_values[i].name);
    }
    vmbuf_remove_last_if(outbuf, ',');
    vmbuf_strcpy(outbuf, ") VALUES (");
    for (i = 0; i < nparams; ++i) {
        if (params[i].meta.type == mysql_helper_field_type_ts_unix) {
            vmbuf_strcpy(outbuf, "FROM_UNIXTIME(?),");
        } else {
            vmbuf_strcpy(outbuf, "?,");
        }
    }
    for (i = 0; i < nfixed_values; ++i)
        vmbuf_sprintf(outbuf, "%s,", fixed_values[i].data.custom_str);
    vmbuf_replace_last_if(outbuf, ',', ')');
    return 0;
}

int mysql_helper_generate_update(struct vmbuf *outbuf, const char *table,
                                 struct mysql_helper_column_map *params, size_t nparams,
                                 struct mysql_helper_column_map *fixed_values, size_t nfixed_values,
                                 struct mysql_helper_column_map *wparams, size_t nwparams) {
    size_t i;
    vmbuf_sprintf(outbuf, "UPDATE `%s` SET", table);
    for (i = 0; i < nparams; ++i) {
        if (params[i].meta.type == mysql_helper_field_type_ts_unix) {
            vmbuf_sprintf(outbuf, "`%s`=FROM_UNIXTIME(?),", params[i].name);
        } else {
            vmbuf_sprintf(outbuf, "`%s`=?,", params[i].name);
        }
    }
    for (i = 0; i < nfixed_values; ++i) {
        vmbuf_sprintf(outbuf, " `%s`=%s,", fixed_values[i].name, fixed_values[i].data.custom_str);
    }
    vmbuf_remove_last_if(outbuf, ',');
    if (wparams) {
        vmbuf_strcpy(outbuf, " WHERE");
        for (i = 0; i < nwparams; ++i) {
            vmbuf_sprintf(outbuf, "%s `%s`=?", i > 0 ? " AND" : "", wparams[i].name);
        }
    }
    return 0;
}
