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
    return 0;
}
