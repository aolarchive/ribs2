#ifndef _MYSQL_COMMON__H_
#define _MYSQL_COMMON__H_

#include "ribs_defs.h"

size_t ribs_mysql_get_storage_size(int type, size_t length);
const char *ribs_mysql_get_type_name(int type);

#endif // _MYSQL_COMMON__H_
