#ifndef _URI_DECODE__H_
#define _URI_DECODE__H_

#include "ribs_defs.h"
#include "hashtable.h"

_RIBS_INLINE_ size_t http_uri_decode(char *uri, char *target);
_RIBS_INLINE_ void http_uri_decode_query_params(char *query_params, struct hashtable *params);
#include "../src/_uri_decode.c"

#endif // _URI_DECODE__H_
