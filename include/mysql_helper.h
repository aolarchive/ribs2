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
#ifndef _MYSQL_HELPER__H_
#define _MYSQL_HELPER__H_

#include "ribs_defs.h"
#include "vmbuf.h"
#include <mysql/mysql.h>
#include "vmbuf.h"
#include "mysql_common.h"

struct mysql_helper {
    MYSQL mysql;
    MYSQL_STMT *stmt;
    struct vmbuf buf;
    char **data;
    unsigned long *length;
    my_bool *is_error;
    my_bool *is_null;
    int8_t *is_str;

    int num_fields;
    int8_t is_connected;
};

int mysql_helper_connect(struct mysql_helper *mysql_helper, struct mysql_login_info *login_info);
int mysql_helper_stmt(struct mysql_helper *mysql_helper, const char *query, size_t query_len, const char *params, const char *fields, ...);
/* executes arbitrary queries */
int mysql_helper_execute(struct mysql_helper *mysql_helper, const char *query, unsigned long *ar);
int mysql_helper_free(struct mysql_helper *mysql_helper);
int mysql_helper_fetch(struct mysql_helper *mysql_helper);
int mysql_helper_tx_begin(struct mysql_helper *mysql_helper);
int mysql_helper_tx_commit(struct mysql_helper *mysql_helper);
int mysql_helper_tx_rollback(struct mysql_helper *mysql_helper);
#define mysql_helper_last_insert_id(helper) mysql_insert_id(&((helper)->mysql))

#endif // _MYSQL_HELPER__H_
