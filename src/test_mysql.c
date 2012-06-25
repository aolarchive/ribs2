#include "ribs.h"
#include <mysql/mysql.h>

void report_error(MYSQL *mysql) {
    LOGGER_ERROR("%s", mysql_error(mysql));
    exit(EXIT_FAILURE);
}

void mysql_fiber() {
    LOGGER_INFO("in %s", __FUNCTION__);
    MYSQL mysql;
    MYSQL_STMT *stmt = NULL;
    mysql_init(&mysql);
    if (NULL == mysql_real_connect(&mysql, "192.168.32.254", "ribs", "12345", "ribs_test", 3306, NULL, 0))
        report_error(&mysql);
    my_bool b_flag = 0;
    if (0 != mysql_options(&mysql, MYSQL_REPORT_DATA_TRUNCATION, (const char *)&b_flag))
        report_error(&mysql);

    stmt = mysql_stmt_init(&mysql);
    if (!stmt)
        return report_error(&mysql);

    const char QUERY[] = "select name from test1";
    if (0 != mysql_stmt_prepare(stmt, QUERY, sizeof(QUERY)))
        return report_error(&mysql);

    MYSQL_BIND bind[1];
    unsigned long length[1];
    my_bool error[1];
    my_bool is_null[1];

    memset(bind, 0, sizeof(bind));
    bind[0].is_unsigned = 0;
    bind[0].buffer_type = MYSQL_TYPE_STRING;
    bind[0].buffer_length = 1024;
    bind[0].buffer = malloc(1024);
    bind[0].is_null = &is_null[0];
    bind[0].length = &length[0];
    bind[0].error = &error[0];

    // execute
    if (0 != mysql_stmt_execute(stmt))
        return report_error(&mysql);

    // bind
    if (0 != mysql_stmt_bind_result(stmt, bind)) {
        LOGGER_ERROR("%s", mysql_stmt_error(stmt));
        return report_error(&mysql);
    }

    int err;
    while (0 == (err = mysql_stmt_fetch(stmt))) {
        LOGGER_INFO("%.*s", (int)bind[0].buffer_length, (char *)bind[0].buffer);
    }
    exit(EXIT_SUCCESS);
}

int main(void) {
    if (epoll_worker_init() < 0)
        exit(EXIT_FAILURE);

    mysql_fiber();

    return 0;
}
