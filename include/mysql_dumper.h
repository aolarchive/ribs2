/*
    This file is part of RIBS2.0 (Robust Infrastructure for Backend Systems).
    RIBS is an infrastructure for building great SaaS applications (but not
    limited to).

    Copyright (C) 2012,2014 Adap.tv, Inc.

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
#ifndef _MYSQL_DUMPER__H_
#define _MYSQL_DUMPER__H_

#include "ribs_defs.h"
#include "mysql_common.h"
#include "mysql_helper.h"

#define MYSQL_DUMPER_UNSIGNED   0x0001
#define MYSQL_DUMPER_SIGNED     0x0000
#define MYSQL_DUMPER_CSTR       0x0002

struct mysql_dumper_type {
    const char *name;
    int mysql_type;
    uint32_t flags;
};

int mysql_dumper_dump(struct mysql_login_info *mysql_login_info, const char *outputdir, const char *dbname, const char *tablename, const char *query, size_t query_len, struct mysql_dumper_type *types);

#endif // _MYSQL_DUMPER__H_
