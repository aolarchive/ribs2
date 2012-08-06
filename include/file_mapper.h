#ifndef _FILE_MAPPER__H_
#define _FILE_MAPPER__H_

#include "ribs_defs.h"

#define FILE_MAPPER_INITIALIZER { NULL, 0 }

struct file_mapper {
    void *mem;
    size_t size;
};

int file_mapper_init(struct file_mapper *fm, const char *filename);
int file_mapper_free(struct file_mapper *fm);

_RIBS_INLINE_ void *file_mapper_data(struct file_mapper *fm) {
    return fm->mem;
}

_RIBS_INLINE_ size_t file_mapper_size(struct file_mapper *fm) {
    return fm->size;
}

#endif // _FILE_MAPPER__H_
