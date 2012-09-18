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
#include "http_cookies.h"

static inline void add_cookie(struct hashtable *ht, char *str) {
    char *val = strchrnul(str, '=');
    char c = *val;
    if (c)
        *val++ = 0;
    hashtable_insert(ht, str, strlen(str), val, strlen(val) + 1);
    if (c)
        *--val = c;
}

int http_parse_cookies(struct hashtable *ht, char *cookies_str) {

    char *p = cookies_str, *s = p;
    char c;
    while (*p) {
        switch (*p) {
        case ';':
        case ' ':
            c = *p;
            *p = 0;
            add_cookie(ht, s);
            *p = c;
            for (; *p == ';' || *p == ' '; ++p);
            s = p;
        default:
            ++p;
        }
    }
    add_cookie(ht, s);
    return 0;
}
