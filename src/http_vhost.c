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
#include "http_vhost.h"
#include "http_server.h"
#include "http_defs.h"

int http_vhost_init(struct http_vhost *vh) {
    if (0 > hashtable_init(&vh->ht_vhosts, 0) ||
        0 > hashtable_init(&vh->ht_vhosts_url, 0))
        return -1;
    return 0;
}

void http_vhost_set_fallback_to_url(struct http_vhost *vh, int value) {
    vh->fallback_to_url = value;
}

void http_vhost_add(struct http_vhost *vh, const char *name, const char *url, void (*func)(struct http_headers *)) {
    hashtable_insert(&vh->ht_vhosts, name, strlen(name), &func, sizeof(func));
    if (url)
        hashtable_insert(&vh->ht_vhosts_url, url, strlen(url), &func, sizeof(func));
}

void http_vhost_run(struct http_vhost *vh) {
    struct http_server_context *ctx = http_server_get_context();
    struct http_headers headers;
    http_headers_parse(ctx->headers, &headers);
    char *p = strchrnul(headers.host, ':');
    *p = 0;
    uint32_t ofs = hashtable_lookup(&vh->ht_vhosts, headers.host, strlen(headers.host));
    do {
        if (0 < ofs)
            return (*((void (**)(struct http_headers *))hashtable_get_val(&vh->ht_vhosts, ofs)))(&headers);
        if (!vh->fallback_to_url || 0 == *ctx->uri)
            break;
        char *p = strchrnul(ctx->uri+1, '/');
        ofs = hashtable_lookup(&vh->ht_vhosts_url, ctx->uri, p - ctx->uri);
        if (0 == ofs)
            break;
        ctx->uri = p;
        return (*((void (**)(struct http_headers *))hashtable_get_val(&vh->ht_vhosts_url, ofs)))(&headers);
    } while (0);
    return http_server_response_sprintf(HTTP_STATUS_403, HTTP_CONTENT_TYPE_TEXT_PLAIN, "%s\n", HTTP_STATUS_403);

}
