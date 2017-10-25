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
#include "logger.h"
#include <limits.h>
#include <stdio.h>

int ds_loader_verify_files(const char *path, const char **files) {
    LOGGER_INFO("verifying files...");
    const char **fn = files;
    char fname[PATH_MAX];
    for (;*fn;++fn) {
        if (PATH_MAX <= snprintf(fname, PATH_MAX, "%s/%s", path, *fn))
            return LOGGER_ERROR("filename too long: %.*s", PATH_MAX, fname), -1;
        if (0 != access(fname, F_OK))
            return LOGGER_PERROR("FILE: %s", fname), -1;
    }
    LOGGER_INFO("verifying files...done");
    return 0;
}
