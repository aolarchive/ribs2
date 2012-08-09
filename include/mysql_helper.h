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
#include <mysql/mysql.h>
#include "vmbuf.h"

struct mysql_login_info {
    const char *host;
    const char *user;
    const char *pass;
    const char *db;
    unsigned int port;
};

struct mysql_helper {
    MYSQL mysql;
    MYSQL_STMT *stmt;
    struct vmbuf buf;
};

int mysql_helper_connect(struct mysql_helper *mysql_helper, struct mysql_login_info *login_info);
int mysql_helper_select(struct mysql_helper *mysql_helper, const char *query, size_t query_len, const char *fields, ...);
/* executes arbitrary queries */
int mysql_helper_execute(struct mysql_helper *mysql_helper, const char *query, unsigned long *ar);
int mysql_helper_free(struct mysql_helper *mysql_helper);
int mysql_helper_fetch(struct mysql_helper *mysql_helper);

#endif // _MYSQL_HELPER__H_
