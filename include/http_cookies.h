#ifndef _HTTP_COOKIES__H_
#define _HTTP_COOKIES__H_

#include "ribs_defs.h"
#include <hashtable.h>

int http_parse_cookies(struct hashtable *ht, char *cookies_str);

#endif // _HTTP_COOKIES__H_
