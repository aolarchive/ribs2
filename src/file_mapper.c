#include "file_mapper.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "logger.h"
#include "vmbuf.h"
#include <sys/mman.h>

int file_mapper_init(struct file_mapper *fm, const char *filename) {
    if (0 > file_mapper_free(fm))
        return -1;
    int fd = open(filename, O_RDONLY | O_CLOEXEC);
    if (fd < 0)
        return LOGGER_PERROR_FUNC("open: %s", filename), -1;
    struct stat st;
    if (0 > fstat(fd, &st))
        return LOGGER_PERROR_FUNC("fstat %s", filename), close(fd), -1;
    fm->size = st.st_size;
    fm->mem = (char *)mmap(NULL, vmbuf_align(fm->size), PROT_READ, MAP_SHARED, fd, 0);
    close(fd);
    if (MAP_FAILED == fm->mem)
        return LOGGER_PERROR_FUNC("mmap %s", filename), close(fd), -1;
    return 0;
}

int file_mapper_free(struct file_mapper *fm) {
    if (NULL == fm->mem)
        return 0;
    int res;
    if (0 > (res = munmap(fm->mem, vmbuf_align(fm->size))))
        LOGGER_PERROR_FUNC("munmap");
    fm->mem = NULL;
    fm->size = 0;
    return res;
}
