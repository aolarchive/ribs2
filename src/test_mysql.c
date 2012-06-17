#include "vmbuf.h"
#include "context.h"
#include "ctx_pool.h"
#include "hashtable.h"
#include <sys/time.h>
#include "list.h"
#include "mime_types.h"
#include "mysql.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include "epoll_worker.h"

int ribs_connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen, int timeout) {
    (void)timeout;
    int flags = fcntl(sockfd, F_GETFL);
    if (0 > fcntl(sockfd, F_SETFL, flags | O_NONBLOCK))
        return perror("mysql_client: fcntl"), -1;
    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLOUT | EPOLLET;
    ev.data.fd = sockfd;

    int res = connect(sockfd, addr, addrlen);
    if (res < 0 && errno != EAGAIN && errno != EINPROGRESS) {
        perror("mysql_client: connect");
        return res;
    }
    if (0 > epoll_ctl(ribs_epoll_fd, EPOLL_CTL_ADD, sockfd, &ev))
        return perror("mysql_client: epoll_ctl"), -1;

    return 0;
}

ssize_t ribs_read(int fd, void *buf, size_t count) {
    int res;
    epoll_worker_fd_map[fd].ctx = current_ctx;
    while ((res = read(fd, buf, count)) < 0) {
        if (errno != EAGAIN) {
            perror("read");
            break;
        }
        dprintf(2, "waiting for read...\n");
        yield();
        dprintf(2, "waiting for read...done\n");
    }
    epoll_worker_fd_map[fd].ctx = &main_ctx;
    return res;
}

ssize_t ribs_write(int fd, const void *buf, size_t count) {
    int res;
    epoll_worker_fd_map[fd].ctx = current_ctx;
    while ((res = write(fd, buf, count)) < 0) {
        if (errno != EAGAIN) {
            perror("write\n");
            break;
        }
        dprintf(2, "waiting for write...\n");
        yield();
        dprintf(2, "waiting for write...done\n");
    }
    epoll_worker_fd_map[fd].ctx = &main_ctx;
    return res;
}

void report_error(MYSQL *mysql) {
    printf("ERROR: %s\n", mysql_error(mysql));
    exit(EXIT_FAILURE);
}

void mysql_fiber() {
    printf("here\n");
    MYSQL mysql;
    MYSQL_STMT *stmt = NULL;
    mysql_init(&mysql);
    if (NULL == mysql_real_connect(&mysql, "127.0.0.1", "ribs", "12345", "ribs_test", 0, NULL, 0))
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
        printf("ERROR: %s\n", mysql_stmt_error(stmt));
        return report_error(&mysql);
    }

    int err;
    while (0 == (err = mysql_stmt_fetch(stmt))) {
        printf("%.*s\n", (int)bind[0].buffer_length, (char *)bind[0].buffer);
    }
    exit(EXIT_SUCCESS);
}

int main(void) {
    if (epoll_worker_init() < 0)
        exit(EXIT_FAILURE);

    /* need a fiber so clients have a place to return to */
    ribs_swapcurcontext(ribs_context_create(1024*1024, mysql_fiber));

    epoll_worker_loop();

    return 0;
}
