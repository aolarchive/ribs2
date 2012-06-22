#include "mysql_helper.h"
#include "logger.h"
#include <mysql/mysql.h>

static int report_error(MYSQL *mysql) {
    LOGGER_ERROR("%s", mysql_error(mysql));
    return -1;
}

int mysql_helper_select(const char *host, const char *user, const char *pass, const char *db, unsigned int port, const char *query, size_t query_len, const char *fields, ...) {
    MYSQL mysql;
    MYSQL_STMT *stmt = NULL;
    mysql_init(&mysql);
    if (NULL == mysql_real_connect(&mysql, host, user, pass, db, port, NULL, 0))
        return report_error(&mysql);
        my_bool b_flag = 0;
    if (0 != mysql_options(&mysql, MYSQL_REPORT_DATA_TRUNCATION, (const char *)&b_flag))
        return report_error(&mysql);

    stmt = mysql_stmt_init(&mysql);
    if (!stmt)
        return report_error(&mysql);

    if (0 != mysql_stmt_prepare(stmt, query, query_len))
        return report_error(&mysql);

    int nargs = strlen(fields);
    MYSQL_BIND bind[nargs];
    unsigned long length[nargs];
    my_bool error[nargs];
    my_bool is_null[nargs];

    memset(bind, 0, sizeof(MYSQL_BIND) * nargs);

    char *p;
    for (p = fields; *p; ++p) {
        char c = *p;
        islower
        switch(*p) {
        case 'd':

        }
    }

    return 0;
}
