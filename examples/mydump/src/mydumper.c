#include "ribs.h"
#include "file_utils.h"
#include "mysql_dumper.h"

/* DB_TEST representing the name of the database
    being connected to */
SSTRL(DB_TEST,                "test");

/* set the client pool timeout */
struct http_client_pool client_pool =
{.timeout_handler.timeout = 60000};

/* the function to dump the test db data table */
int dump_test_data(
        const char *base_path,
        struct mysql_login_info login_db_test,
        struct vmbuf *query) {

    LOGGER_INFO("dumping test_table");

    /* 'DUMPER' macro from mysql_common.h will dump the
        result of the mysql query on the 'DB_TEST' db into
        the dump 'test_table' directory */
    DUMPER(&login_db_test, DB_TEST, "test_table",
            "SELECT id, name, age FROM data;");

    /* INDEXER_O2O macro will create a one-to-one index on
       the table column 'id' in the 'test_table' dump */
    INDEXER_O2O(int32_t, DB_TEST, "test_table", "id");

    /* return 0 if dump and index were created successfully.
        Macros will return -1 if an error occurs */
    return 0;
}

/* the main */
int main(void){
    /* the base directory to be used/created */
    const char *base_dir = "data";
    /* the default test db connection string */
    char DEFAULT_TEST_DB_CONN_STR[] = "@localhost";
    char *test_db = DEFAULT_TEST_DB_CONN_STR;

    /* create the target directory where the dump(s) will be stored */
    if( 0 > mkdir_recursive(base_dir)) {
        LOGGER_ERROR("Failed to create target directory: [%s]", base_dir);
        exit(EXIT_FAILURE);
    }
    LOGGER_INFO("target directory: [%s]", base_dir);

    /* the mysql_login_info struct will be used to access the database */
    struct mysql_login_info login_db_test;

    /* parse the test_db connection string in case of any errors */
    /* not necessarily used in this example unless you use a database
        connection string other than '@localhost' */
    if (0 > ribs_mysql_parse_db_conn_str(test_db, &login_db_test)) {
        LOGGER_ERROR("failed to parse DB connection string: [%s]", test_db);
        exit(EXIT_FAILURE);
    }
    login_db_test.db = DB_TEST;

    /* initialize the event loop */
    if( 0 > epoll_worker_init()) {
        LOGGER_ERROR("epoll_worker_init failed");
        exit(EXIT_FAILURE);
    }

    /* initialize the client pool */
    http_client_pool_init(&client_pool, 10, 10);

    /* initialize the query buffer used in dump_test_data() */
    struct vmbuf query = VMBUF_INITIALIZER;
    vmbuf_init(&query, 65536);

    /* dump the test db data table into its target directory */
    if( 0 > dump_test_data(base_dir, login_db_test, &query)) {
        LOGGER_ERROR("Faied to dump data from test db");
        exit(EXIT_FAILURE);
    }

    /* return when successful */
    LOGGER_INFO("completed successfully");
    exit(EXIT_SUCCESS);
    return 0;
}
