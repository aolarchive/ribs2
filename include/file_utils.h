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
#ifndef _FILE_UTILS__H_
#define _FILE_UTILS__H_

#include "ribs_defs.h"

int mkdir_recursive(const char *dirname);
int mkdir_for_file_recursive(const char *filename);
int ribs_create_temp_file(const char *prefix);
int ribs_create_temp_file2(const char *dir_path, const char *prefix, char *file_path, size_t file_path_sz);

#endif // _FILE_UTILS__H_
