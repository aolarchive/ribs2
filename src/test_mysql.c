#include "ribs.h"
#include <mysql/mysql.h>
#include "mysql_helper.h"

void report_error(MYSQL *mysql) {
    LOGGER_ERROR("%s", mysql_error(mysql));
    exit(EXIT_FAILURE);
}

void mysql_fiber() {
    LOGGER_INFO("in %s", __FUNCTION__);
    struct mysql_helper m;
    const char QUERY[] = "select id, name from test1";
    int id;
    char name[128];
    unsigned long name_len = sizeof(name);
    if (0 > mysql_helper_select(&m, "127.0.0.1", "ribs", "12345", "ribs_test", 3306, QUERY, sizeof(QUERY) - 1, "ds", &id, &name_len, name))
        exit(EXIT_FAILURE);
    while (mysql_helper_fetch(&m) > 0) {
        printf("id = %d, name = %.*s\n", id, (int)name_len, name);
    }
    mysql_helper_free(&m);
    exit(EXIT_SUCCESS);
}

int main(void) {
    if (epoll_worker_init() < 0)
        exit(EXIT_FAILURE);

    mysql_fiber();

    return 0;
}
