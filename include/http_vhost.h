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
