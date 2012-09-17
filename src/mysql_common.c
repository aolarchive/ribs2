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

#include "enum.h"
#include "mysql_common.h"
#include <mysql/mysql.h>

size_t ribs_mysql_get_storage_size(int type, size_t length) {
    switch (type) {
        ENUM_2_STORAGE_STR(MYSQL_TYPE_DECIMAL, length);
        ENUM_2_STORAGE(MYSQL_TYPE_TINY, signed char);
        ENUM_2_STORAGE(MYSQL_TYPE_SHORT, short int);
        ENUM_2_STORAGE(MYSQL_TYPE_LONG, int);
        ENUM_2_STORAGE(MYSQL_TYPE_FLOAT, float);
        ENUM_2_STORAGE(MYSQL_TYPE_DOUBLE, double);
        ENUM_2_STORAGE_STR(MYSQL_TYPE_NULL, length);
        ENUM_2_STORAGE(MYSQL_TYPE_TIMESTAMP, MYSQL_TIME);
        ENUM_2_STORAGE(MYSQL_TYPE_LONGLONG, long long int);
        ENUM_2_STORAGE(MYSQL_TYPE_INT24, int);
        ENUM_2_STORAGE(MYSQL_TYPE_DATE, MYSQL_TIME);
        ENUM_2_STORAGE(MYSQL_TYPE_TIME, MYSQL_TIME);
        ENUM_2_STORAGE(MYSQL_TYPE_DATETIME, MYSQL_TIME);
        ENUM_2_STORAGE(MYSQL_TYPE_YEAR, short int);
        ENUM_2_STORAGE(MYSQL_TYPE_NEWDATE, MYSQL_TIME);
        ENUM_2_STORAGE_STR(MYSQL_TYPE_VARCHAR, length);
        ENUM_2_STORAGE_STR(MYSQL_TYPE_BIT, length);
        ENUM_2_STORAGE_STR(MYSQL_TYPE_NEWDECIMAL, length);
        ENUM_2_STORAGE_STR(MYSQL_TYPE_ENUM, length);
        ENUM_2_STORAGE_STR(MYSQL_TYPE_SET, length);
        ENUM_2_STORAGE_STR(MYSQL_TYPE_TINY_BLOB, length);
        ENUM_2_STORAGE_STR(MYSQL_TYPE_MEDIUM_BLOB, length);
        ENUM_2_STORAGE_STR(MYSQL_TYPE_LONG_BLOB, length);
        ENUM_2_STORAGE_STR(MYSQL_TYPE_BLOB, length);
        ENUM_2_STORAGE_STR(MYSQL_TYPE_VAR_STRING, length);
        ENUM_2_STORAGE_STR(MYSQL_TYPE_STRING, length);
    }
    return 0;
}

const char *ribs_mysql_get_type_name(int type) {
    switch (type) {
        ENUM_TO_STRING(MYSQL_TYPE_DECIMAL);
        ENUM_TO_STRING(MYSQL_TYPE_TINY);
        ENUM_TO_STRING(MYSQL_TYPE_SHORT);
        ENUM_TO_STRING(MYSQL_TYPE_LONG);
        ENUM_TO_STRING(MYSQL_TYPE_FLOAT);
        ENUM_TO_STRING(MYSQL_TYPE_DOUBLE);
        ENUM_TO_STRING(MYSQL_TYPE_NULL);
        ENUM_TO_STRING(MYSQL_TYPE_TIMESTAMP);
        ENUM_TO_STRING(MYSQL_TYPE_LONGLONG);
        ENUM_TO_STRING(MYSQL_TYPE_INT24);
        ENUM_TO_STRING(MYSQL_TYPE_DATE);
        ENUM_TO_STRING(MYSQL_TYPE_TIME);
        ENUM_TO_STRING(MYSQL_TYPE_DATETIME);
        ENUM_TO_STRING(MYSQL_TYPE_YEAR);
        ENUM_TO_STRING(MYSQL_TYPE_NEWDATE);
        ENUM_TO_STRING(MYSQL_TYPE_VARCHAR);
        ENUM_TO_STRING(MYSQL_TYPE_BIT);
        ENUM_TO_STRING(MYSQL_TYPE_NEWDECIMAL);
        ENUM_TO_STRING(MYSQL_TYPE_ENUM);
        ENUM_TO_STRING(MYSQL_TYPE_SET);
        ENUM_TO_STRING(MYSQL_TYPE_TINY_BLOB);
        ENUM_TO_STRING(MYSQL_TYPE_MEDIUM_BLOB);
        ENUM_TO_STRING(MYSQL_TYPE_LONG_BLOB);
        ENUM_TO_STRING(MYSQL_TYPE_BLOB);
        ENUM_TO_STRING(MYSQL_TYPE_VAR_STRING);
        ENUM_TO_STRING(MYSQL_TYPE_STRING);
        ENUM_TO_STRING(MYSQL_TYPE_GEOMETRY);
    }
    return "UNKNOWN";
}
