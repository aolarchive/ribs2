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

#include "file_utils.h"
#include "logger.h"
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <stdio.h>
#include <limits.h>

int _mkdir_recursive(char *file) {
    char *cur = file;
    while (*cur) {
        ++cur;
        char *p = strchrnul(cur, '/');
        char c = *p;
        *p = 0;
        if (0 > mkdir(file, 0755) && errno != EEXIST)
            return -1;
        cur = p;
        *p = c;
    }
    return 0;
}

/*
 * Recursively create all directories for the given filename.
 * Example: for "/dir1/dir2/dir3/file" directories dir1, dir2, and dir3
 *          will be created if they don't exist.
 */
int mkdir_for_file_recursive(const char *filename) {
    char file[strlen(filename) + 1];
    strcpy(file, filename);
    char *p = strrchr(file, '/');
    if (NULL == p)
        return 0;
    *p = 0;
    return _mkdir_recursive(file);
}

/*
 * Recursively create all directories for the given directory.
 * Example: for "/dir1/dir2/dir3" directories dir1, dir2, and dir3
 *          will be created if they don't exist.
 */
int mkdir_recursive(const char *dirname) {
    char file[strlen(dirname) + 1];
    strcpy(file, dirname);
    return _mkdir_recursive(file);
}

int ribs_create_temp_file2(const char *dir_path, const char *prefix, char *file_path, size_t file_path_sz) {
    char filename[PATH_MAX];
    if (PATH_MAX <= snprintf(filename, PATH_MAX, "%s/%s_XXXXXX", dir_path, prefix))
        return LOGGER_ERROR("ribs_create_temp_file: filename too long"), -1;
    int fd = mkstemp(filename);
    if (fd < 0)
        return LOGGER_PERROR("mkstemp: %s", filename), -1;
    if (unlink(filename) < 0)
        return LOGGER_PERROR("unlink: %s", filename), close(fd), -1;
    if (NULL != file_path) {
        strncpy(file_path, filename, file_path_sz);
        if (file_path_sz > 0)
            file_path[file_path_sz - 1] = '\0';
    }
    return fd;
}

int ribs_create_temp_file(const char *prefix) {
    return ribs_create_temp_file2("/dev/shm", prefix, NULL, 0);
}
