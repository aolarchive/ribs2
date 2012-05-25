#include "uri_decode.h"

static inline char hex_val_char(char c) {
    if (c >= '0' && c <= '9')
        return c - '0';
    else if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    else if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    return 0;
}

static inline char hex_val(char **h) {
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


inline void http_uri_decode(char *uri) {
    char *p1 = uri, *p2 = p1;
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
}
