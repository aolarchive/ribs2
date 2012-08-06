#ifndef _MYSQL_DUMPER__H_
#define _MYSQL_DUMPER__H_

#include "ribs_defs.h"
#include "mysql_helper.h"

#define MYSQL_DUMPER_UNSIGNED   0x0001
#define MYSQL_DUMPER_SIGNED     0x0000
#define MYSQL_DUMPER_CSTR       0x0002

struct mysql_dumper_type {
    const char *name;
    int mysql_type;
    uint32_t flags;
};

int mysql_dumper_dump(struct mysql_login_info *mysql_login_info, const char *outputdir, const char *dbname, const char *tablename, const char *query, size_t query_len, struct mysql_dumper_type *types);

#endif // _MYSQL_DUMPER__H_
