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
#ifndef _MYSQL_POOL__H_
#define _MYSQL_POOL__H_

#include "ribs_defs.h"
#include "vmbuf.h"
#include "mysql_helper.h"
#include "list.h"

#define POOL_HT_SIZE 64

struct mysql_pool_entry
{
    struct mysql_helper helper;
    struct list l;
};

int mysql_pool_init(uint32_t ht_size);
int mysql_pool_get(struct mysql_login_info *info, struct mysql_pool_entry **mysql);
int mysql_pool_free(struct mysql_login_info *info, struct mysql_pool_entry *mysql);

#endif //_MYSQL_POOL__H_
