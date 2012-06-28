#include "mysql_helper.h"
#include "logger.h"
#include "vmbuf.h"
#include <string.h>
#include <ctype.h>

static int report_error(struct mysql_helper *mysql_helper) {
    LOGGER_ERROR("%s", mysql_error(&mysql_helper->mysql));
    return -1;
}

static int report_stmt_error(struct mysql_helper *mysql_helper) {
    LOGGER_ERROR("%s, %s", mysql_error(&mysql_helper->mysql), mysql_stmt_error(mysql_helper->stmt));
    mysql_stmt_close(mysql_helper->stmt);
    mysql_helper->stmt = NULL;
    return -1;
}

int mysql_helper_select(struct mysql_helper *mysql_helper, const char *query, size_t query_len, const char *fields, ...) {
    mysql_init(&mysql_helper->mysql);
    if (NULL == mysql_real_connect(&mysql_helper->mysql,
                                   mysql_helper->host,
                                   mysql_helper->user,
                                   mysql_helper->pass,
                                   mysql_helper->db,
                                   mysql_helper->port,
                                   NULL, 0))
        return report_error(mysql_helper);
        my_bool b_flag = 0;
    if (0 != mysql_options(&mysql_helper->mysql, MYSQL_REPORT_DATA_TRUNCATION, (const char *)&b_flag))
        return report_error(mysql_helper);

    mysql_helper->stmt = mysql_stmt_init(&mysql_helper->mysql);
    if (!mysql_helper->stmt)
        return report_error(mysql_helper);

    if (0 != mysql_stmt_prepare(mysql_helper->stmt, query, query_len))
        return report_stmt_error(mysql_helper);

    int nargs = strlen(fields);
    MYSQL_BIND bind[nargs];
    unsigned long length[nargs];
    my_bool error[nargs];
    my_bool is_null[nargs];

    memset(bind, 0, sizeof(MYSQL_BIND) * nargs);

    MYSQL_BIND *bind_ptr = bind;

    const char *p;
    unsigned long *len;
    int i = 0;
    va_list ap;
    va_start(ap, fields);
    for (p = fields; *p; ++p, ++bind_ptr, ++i) {
        char c = *p;
        bind_ptr->is_unsigned = isupper(c) ? 1 : 0;
        bind_ptr->is_null = &is_null[i];
        bind_ptr->error = &error[i];
        bind_ptr->length = &length[i];
        c = tolower(c);
        switch(c) {
        case 'd':
            bind_ptr->buffer = va_arg(ap, int *);
            bind_ptr->buffer_length = sizeof(int);
            bind_ptr->buffer_type = MYSQL_TYPE_LONG;
            break;
        case 's':
            len = va_arg(ap, unsigned long *);
            bind_ptr->length = len;
            bind_ptr->buffer = va_arg(ap, char *);
            bind_ptr->buffer_length = *len;
            bind_ptr->buffer_type = MYSQL_TYPE_STRING;
            break;
        }
    }
    va_end(ap);

    // execute
    if (0 != mysql_stmt_execute(mysql_helper->stmt))
        return report_stmt_error(mysql_helper);

    // bind
    if (0 != mysql_stmt_bind_result(mysql_helper->stmt, bind))
        return report_stmt_error(mysql_helper);

    return 0;
}

int mysql_helper_free(struct mysql_helper *mysql_helper) {
    if (mysql_helper->stmt) {
        mysql_stmt_close(mysql_helper->stmt);
        mysql_helper->stmt = NULL;
    }
    mysql_close(&mysql_helper->mysql);
    return 0;
}

int mysql_helper_fetch(struct mysql_helper *mysql_helper) {
    int err = mysql_stmt_fetch(mysql_helper->stmt);
    if (err != 0) {
        if (err != MYSQL_NO_DATA)
            return report_stmt_error(mysql_helper);
        return 0;
    }
    return 1;
}


