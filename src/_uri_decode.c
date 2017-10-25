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
/*
 * inline
 */
_RIBS_INLINE_ char hex_val_char(char c) {
    if (c >= '0' && c <= '9')
        return c - '0';
    else if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    else if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    return 0;
}

_RIBS_INLINE_ char hex_val(char **h) {
    char *s = *h;
    char val = 0;
    if (*s) {
        val = hex_val_char(*s) << 4;
        ++s;
        if (*s)
            val += hex_val_char(*s++);
    }
    *h = s;
    return val;
}

_RIBS_INLINE_ size_t http_uri_decode(char *uri, char *target) {
    char *p1 = uri, *p2 = target;
    const char SPACE = ' ';
    while (*p1) {
        switch (*p1) {
        case '%':
            ++p1; // skip the %
            *p2++ = hex_val(&p1);
            break;
        case '+':
            *p2++ = SPACE;
            ++p1;
            break;
        default:
            *p2++ = *p1++;
        }
    }
    *p2 = 0;
    return p2 - target + 1; /* include \0 */
}

_RIBS_INLINE_ void http_uri_decode_query_params(char *query_params, struct hashtable *params) {
    while (*query_params) {
        char saved_char[2] = {0, 0};
        char *key = query_params;
        char *p1 = strchrnul(query_params, '&');
        if (*p1) {
            saved_char[0] = *p1;
            *p1 = 0;
            query_params = p1 + 1;
        } else
            query_params = p1;
        char *p2 = strchrnul(key, '='), *val = p2;
        size_t key_len = val - key;
        if (*val) {
            saved_char[1] = *val;
            *val++ = 0;
        }
        uint32_t htofs = hashtable_lookup(params, key, key_len);
        if (!htofs) {
            htofs = hashtable_insert_alloc(params, key, key_len, p1 - val + 1);
            http_uri_decode(val, hashtable_get_val(params, htofs));
        }
        *p2 = saved_char[1];
        *p1 = saved_char[0];
    }
}
