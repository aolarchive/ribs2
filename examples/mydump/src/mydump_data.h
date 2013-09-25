/*
    This file is part of RIBS2.0 (Robust Infrastructure for Backend Systems).
    RIBS is an infrastructure for building great SaaS applications (but not
    limited to).

    Copyright (C) 2013 Adap.tv, Inc.

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
#ifndef _MYDUMP_DATA_H_
#define _MYDUMP_DATA_H_

#include "ribs.h"

#undef DS_LOADER_TYPENAME
#undef DS_LOADER_CONFIG

/* define the ds_loader type */
#define DS_LOADER_TYPENAME ds_loader_mydump
/* config to the mydump.ds file */
#define DS_LOADER_CONFIG "mydump.ds"

/* ds_loader.h needs to be included in both
    mydump_data.h and mydump_data.c for
    code generation */
#include "ds_loader.h"

/* data init and load functions */
int mydump_data_init(void);
int mydump_data_load(void);

/* externs to used for accessing the data */
extern int active;
extern ds_loader_mydump_t data[2];

#endif// _MYDUMP_DATA_H_
