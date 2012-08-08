#include "file_utils.h"
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>


int mkdir_recursive(const char *filename) {
    char file[strlen(filename) + 1];
    strcpy(file, filename);
    char *p = strrchr(file, '/');
    if (NULL == p)
        return 0;
    *p = 0;
    char *cur = file;
    while (*cur) {
        ++cur;
        char *p = strchrnul(cur, '/');
        char c = *p;
        *p = 0;
        if (0 > mkdir(file, 0755) && errno != EEXIST)
            return -1;
        cur = p;
        *p = c;
    }
    *p = '/'; // restore
    return 0;
}

