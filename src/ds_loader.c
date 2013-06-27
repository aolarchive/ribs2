#include "logger.h"
#include <limits.h>
#include <stdio.h>

int ds_loader_verify_files(const char *path, const char **files) {
    LOGGER_INFO("verifying files...");
    const char **fn = files;
    char fname[PATH_MAX];
    for (;*fn;++fn) {
        if (PATH_MAX <= snprintf(fname, PATH_MAX, "%s/%s", path, *fn))
            return LOGGER_ERROR("filename too long: %.*s", PATH_MAX, fname), -1;
        if (0 != access(fname, F_OK))
            return LOGGER_PERROR("FILE: %s", fname), -1;
    }
    LOGGER_INFO("verifying files...done");
    return 0;
}
