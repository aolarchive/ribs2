#ifndef _HTTP_FILE_SERVER__H_
#define _HTTP_FILE_SERVER__H_

#include "ribs_defs.h"
#include "http_server.h"
#include "hashtable.h"

struct http_file_server {
    /* configurable */
    const char *base_dir;
    int allow_list;
    int max_age;
    /* internal use */
    size_t base_dir_len;
    struct hashtable ht_ext_whitelist;
};

#define HTTP_FILE_SERVER_INITIALIZER { NULL, 0, 0, 0, HASHTABLE_INITIALIZER }

int http_file_server_init(struct http_file_server *fs);
int http_file_server_run(struct http_file_server *fs);
int http_file_server_run2(struct http_file_server *fs, struct http_headers *headers, const char *file);
static inline void http_file_server_gzip_ext(struct http_file_server *fs, const char *ext);

#include "../src/_http_file_server.c"

#endif // _HTTP_FILE_SERVER__H_
