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
#ifndef _MYSQL_COMMON__H_
#define _MYSQL_COMMON__H_

#include "ribs_defs.h"

#define QUERY_INIT(q, ...) vmbuf_reset(query); vmbuf_sprintf(query, q, ##__VA_ARGS__)
#define QUERY() (vmbuf_data(query))
#define QUERY_LEN() (vmbuf_wlocpos(query))
#define DUMPER(conn, db, tbl, q,...) QUERY_INIT(q, ##__VA_ARGS__); if (0 > mysql_dumper_dump(conn, base_path, db, tbl, QUERY(), QUERY_LEN(), NULL)) return -1
#define DUMPER_WITH_TYPES(conn, db, tbl, types, q,...) QUERY_INIT(q, ##__VA_ARGS__); if (0 > mysql_dumper_dump(conn, base_path, db, tbl, QUERY(), QUERY_LEN(), types)) return -1
#define MAKE_PAIR64(a,b) ((((uint64_t)(a)) << 32) | (b))

#define INDEXER_O2O(T,db, tbl, field) if (0 > IDX_GEN_DS_FILE_O2O(T, base_path, db, tbl, field)) return -1
#define INDEXER_O2M(T,db, tbl, field) if (0 > IDX_GEN_DS_FILE_O2M(T, base_path, db, tbl, field)) return -1
#define VAR_INDEXER_O2M(db, tbl, field) if (0 > var_index_gen_generate_ds_file(base_path, db, tbl, field)) return -1

struct mysql_login_info {
    const char *host;
    const char *user;
    const char *pass;
    const char *db;
    unsigned int port;
};

size_t ribs_mysql_get_storage_size(int type, size_t length);
const char *ribs_mysql_get_type_name(int type);
void ribs_mysql_mask_db_pass(char *str);
int ribs_mysql_parse_db_conn_str(char *str, struct mysql_login_info *info);

#endif // _MYSQL_COMMON__H_
