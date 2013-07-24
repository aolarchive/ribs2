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
#include "http_headers.h"
#include "hashtable.h"
#include "sstr.h"
#include "logger.h"
#include <ctype.h>

static struct hashtable ht_request_headers = HASHTABLE_INITIALIZER;

struct request_headers {
    const char *name;
    uint32_t ofs;
};

static struct request_headers request_headers[] = {
    { "referer",         offsetof(struct http_headers, referer)         },
    { "user-agent",      offsetof(struct http_headers, user_agent)      },
    { "cookie",          offsetof(struct http_headers, cookie)          },
    { "x-forwarded-for", offsetof(struct http_headers, x_forwarded_for) },
    { "host",            offsetof(struct http_headers, host)            },
    { "accept-encoding", offsetof(struct http_headers, accept_encoding) },
    { "content-type",    offsetof(struct http_headers, content_type)    },
    { "if-none-match",   offsetof(struct http_headers, if_none_match)   },
    { "accept-language", offsetof(struct http_headers, accept_language) },
    { "origin",          offsetof(struct http_headers, origin)          },
    /* terminate the list */
    { NULL, 0 }
};


int http_headers_init(void) {
    if (hashtable_is_initialized(&ht_request_headers))
        return 1;
    hashtable_init(&ht_request_headers, 64);
    struct request_headers *rh = request_headers;
    for (; rh->name; ++rh)
        hashtable_insert(&ht_request_headers, rh->name, strlen(rh->name), &rh->ofs, sizeof(rh->ofs));
    return 0;
}

static inline void add_to_hashtable(char *name, char *value, struct http_headers *h) {
    char *p;
    for (p = name; *p; *p = tolower(*p), ++p);
    uint32_t ofs = hashtable_lookup(&ht_request_headers, name, p - name);
    if (ofs)
        *(char **)((char *)h + *(uint32_t *)hashtable_get_val(&ht_request_headers, ofs)) = value;
}

static void http_header_decode_accept_encoding(struct http_headers *h) {
    static const char QVAL[] = "q=";
    static const char GZIP[] = "gzip";
    static const char COMPRESS[] = "compress";
    static const char DEFLATE[] = "deflate";
    char *p = h->accept_encoding;
    if (*p == '-') { /* wasn't specified */
        h->accept_encoding_mask = HTTP_AE_IDENTITY;
    } else if (*p == '*') {  /* accept all */
        h->accept_encoding_mask = HTTP_AE_ALL;
    } else {
        while (*p) {
            char *val = p;
            p = strchrnul(p, ',');
            if (*p) *p++ = 0;
            for (; *p == ' '; *p++ = '\0');
            float qval = 1.0;
            char *qvalstr = strchrnul(val, ';');
            if (*qvalstr) {
                *qvalstr++ = 0;
                for (; *qvalstr == ' '; *qvalstr++ = '\0');
                if (0 == SSTRNCMP(QVAL, qvalstr))
                    qval = atof(qvalstr + SSTRLEN(QVAL));
            }
            if (qval > 0.0001) {
                if (*val == '*')
                    h->accept_encoding_mask |= HTTP_AE_ALL;
                else if (0 == SSTRNCMP(GZIP, val))
                    h->accept_encoding_mask |= HTTP_AE_GZIP;
                else if (0 == SSTRNCMP(COMPRESS, val))
                    h->accept_encoding_mask |= HTTP_AE_COMPRESS;
                else if (0 == SSTRNCMP(DEFLATE, val))
                    h->accept_encoding_mask |= HTTP_AE_DEFLATE;
            }
        }
    }
}

void http_headers_parse(char *headers, struct http_headers *h) {
    static char no_value[] = { '-', 0 };
    *h = (struct http_headers) {
        no_value,
        no_value,
        no_value,
        no_value,
        no_value,
        no_value,
        no_value,
        no_value,
        no_value,
        no_value,
        HTTP_AE_IDENTITY,
        { '-', 0 }
    };
    while (*headers) {
        char *p = strchrnul(headers, '\r'); // find the end
        if (*p) *p++ = 0;
        if (*p == '\n') ++p;
        char *p2 = strchrnul(headers, ':');
        if (*p2) *p2++ = 0;
        if (*p2 == ' ') *p2++ = 0; // skip the space
        add_to_hashtable(headers, p2, h);
        headers = p; // proceed to next pair
    }
    http_header_decode_accept_encoding(h);
}
