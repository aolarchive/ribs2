#include "mime_types.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>
#include "hashtable.h"
#include "sstr.h"
#include "logger.h"

SSTRL(MIME_TYPES, "/etc/mime.types");
SSTRL(MIME_DELIMS, "\t ");
SSTRL(DEFAULT_MIME_TYPE, "text/plain");
static struct hashtable ht_mime_types = HASHTABLE_INITIALIZER;

int mime_types_init(void) {
    if (hashtable_get_size(&ht_mime_types))
        return 1;
    LOGGER_INFO("initializing mime types");
    int fd = open(MIME_TYPES, O_RDONLY);
    struct stat st;
    if (0 > fstat(fd, &st))
        return LOGGER_PERROR("mime_types fstat"), close(fd), -1;
    void *mem = mmap(NULL, st.st_size + 1, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    if (MAP_FAILED == mem)
        return LOGGER_PERROR("mime_types mmap"), close(fd), -1;

    hashtable_init(&ht_mime_types, 4096);
    char *content = (char *)mem;
    char *content_end = content + st.st_size;

    while (content < content_end)
    {
        // find end of line
        char *line = content, *p = line;
        for (; p < content_end && *p != '\n'; ++p);
        *p = 0; // mmap'd one extra byte to avoid checks
        content = p + 1;

        p = strchrnul(line, '#');
        *p = 0; // truncate line after comment

        p = line;
        char *saveptr;
        char *mime = strtok_r(p, MIME_DELIMS, &saveptr);
        if (!mime)
            continue;
        char *ext;
        while (NULL != (ext = strtok_r(NULL, MIME_DELIMS, &saveptr))) {
            for (p = ext; *p; *p = tolower(*p), ++p);
            hashtable_insert(&ht_mime_types, ext, strlen(ext), mime, strlen(mime) + 1);
        }
    }
    munmap(mem, st.st_size + 1);
    close(fd);
    return 0;
}


const char *mime_types_by_ext(const char *ext) {
    const char *res = DEFAULT_MIME_TYPE;
    size_t n = strlen(ext);
    char tmpext[n];
    char *p = tmpext;
    for (; *ext; ++ext)
        *p++ = tolower(*ext);
    // tmpext doesn't have to be \0 terminated
    size_t ofs = hashtable_lookup(&ht_mime_types, tmpext, n);
    if (ofs)
        return hashtable_get_val(&ht_mime_types, ofs);
    return res;
}

const char *mime_types_by_filename(const char *filename) {
    const char *p = strrchr(filename, '.');
    if (p)
        return mime_types_by_ext(p + 1);
    return DEFAULT_MIME_TYPE;
}
