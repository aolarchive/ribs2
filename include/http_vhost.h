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
#ifndef _HTTP_VHOST__H_
#define _HTTP_VHOST__H_

#include "ribs_defs.h"
#include "hashtable.h"
#include "http_headers.h"

#define HTTP_VHOST_INITIALIZER { 1, HASHTABLE_INITIALIZER, HASHTABLE_INITIALIZER }

struct http_vhost {
    /* configurable */
    int fallback_to_url;
    /* internal use */
    struct hashtable ht_vhosts;
    struct hashtable ht_vhosts_url;
};

int http_vhost_init(struct http_vhost *vh);
void http_vhost_set_fallback_to_url(struct http_vhost *vh, int value);
void http_vhost_add(struct http_vhost *vh, const char *name, const char *url, void (*func)(struct http_headers *));
void http_vhost_run(struct http_vhost *vh);

#endif // _HTTP_VHOST__H_
