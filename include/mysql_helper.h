#ifndef _MYSQL_HELPER__H_
#define _MYSQL_HELPER__H_

#include "ribs_defs.h"
#include <mysql/mysql.h>

struct mysql_helper {
    MYSQL mysql;
    MYSQL_STMT *stmt;
};

int mysql_helper_select(struct mysql_helper *mysql_helper, const char *host, const char *user, const char *pass, const char *db, unsigned int port, const char *query, size_t query_len, const char *fields, ...);
int mysql_helper_free(struct mysql_helper *mysql_helper);
int mysql_helper_fetch(struct mysql_helper *mysql_helper);

#endif // _MYSQL_HELPER__H_
