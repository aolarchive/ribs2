/*
    This file is part of RIBS2.0 (Robust Infrastructure for Backend Systems).
    RIBS is an infrastructure for building great SaaS applications (but not
    limited to).

    Copyright (C) 2012,2013 Adap.tv, Inc.

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

static inline void add_cookie(struct hashtable *ht, const char *str) {
    const char *name = str;
    const char *name_end = strchrnul(str, '=');
    const char *val = *name_end ? name_end + 1 : name_end;

    /* get rid of quotes */
    const char *val_st = val, *val_st_end = val + strlen(val);
    if ('"' == *val_st)
        ++val_st;
    if (val_st != val_st_end && '"' == *(val_st_end - 1))
        --val_st_end;

    size_t l = val_st_end - val_st;
    uint32_t loc = hashtable_insert(ht, name, name_end - name, val_st, l + 1);
    *(char *)(hashtable_get_val(ht, loc) + l) = 0;
}

int http_parse_cookies(struct hashtable *ht, char *cookies_str) {

    char *p = cookies_str, *s = p;
    char c;
    for (;;) {
        switch (*p) {
        case 0:
        case ';':
        case ' ':
            c = *p;
            *p = 0;
            if (*s)
                add_cookie(ht, s);
            *p = c;
            for (; *p == ';' || *p == ' '; ++p);
            if (0 == *p) return 0;
            s = p;
            break;
        default:
            ++p;
        }
    }
}
