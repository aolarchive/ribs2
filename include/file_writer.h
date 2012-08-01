#ifndef _FILE_WRITER__H_
#define _FILE_WRITER__H_

#include "logger.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define FILE_WRITER_INITIALIZER { -1, NULL, 0, 0, 0, 1024*1024 }

struct file_writer {
    int fd;
    char *mem;
    size_t base_loc;
    size_t write_loc;
    size_t capacity;
    size_t buffer_size;
};

_RIBS_INLINE_ void file_writer_make(struct file_writer *fw);
_RIBS_INLINE_ int file_writer_init(struct file_writer *fw, const char *filename);
_RIBS_INLINE_ size_t file_writer_wavail(struct file_writer *fw);
_RIBS_INLINE_ char *file_writer_wloc(struct file_writer *fw);
_RIBS_INLINE_ size_t file_writer_wlocpos(struct file_writer *fw);
_RIBS_INLINE_ int file_writer_wseek(struct file_writer *fw, size_t n);
_RIBS_INLINE_ int file_writer_write(struct file_writer *fw, const void *buf, size_t size);
_RIBS_INLINE_ int file_writer_close(struct file_writer *fw);

#include "../src/_file_writer.c"

#endif // _FILE_WRITER__H_
