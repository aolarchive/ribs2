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
#ifndef _HTTP_HEADERS__H_
#define _HTTP_HEADERS__H_

#include "ribs_defs.h"
#include <arpa/inet.h>

/*
 * common http headers
 */
struct http_headers{
    char *referer;
    char *user_agent;
    char *cookie;
    char *x_forwarded_for;
    char *host;
    char *accept_encoding;
    char *content_type;
    char *if_none_match;
    char *accept_language;
    char *origin;
    uint8_t accept_encoding_mask;
    char peer_ip_addr[INET_ADDRSTRLEN];
};

/*
 * HTTP Accept Encoding
 */
enum {
    HTTP_AE_IDENTITY = 0x00,
    HTTP_AE_GZIP = 0x01,
    HTTP_AE_COMPRESS = 0x02,
    HTTP_AE_DEFLATE = 0x04,
    HTTP_AE_ALL = 0xFF
};

int http_headers_init(void);
void http_headers_parse(char *headers, struct http_headers *h);

#endif // _HTTP_HEADERS__H_
