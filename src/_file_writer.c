/*
    This file is part of RIBS2.0 (Robust Infrastructure for Backend Systems).
    RIBS is an infrastructure for building great SaaS applications (but not
    limited to).

    Copyright (C) 2012,2013,2014 Adap.tv, Inc.

    RIBS is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, version 2.1 of the License.

    RIBS is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with RIBS.  If not, see <http://www.gnu.org/licenses/>.
*/
_RIBS_INLINE_ void file_writer_make(struct file_writer *fw) {
    *fw = (struct file_writer)FILE_WRITER_INITIALIZER;
}

_RIBS_INLINE_ int file_writer_attachfd(struct file_writer *fw, int fd, size_t initial_size) {
    if (0 > file_writer_close(fw))
        return -1;
    fw->mem = NULL;
    fw->base_loc = fw->write_loc = 0;
    fw->fd = fd;

    if (initial_size < 4096)
        fw->buffer_size = 4096;
    else
        fw->buffer_size = next_p2(initial_size);

    if (0 > fw->fd)
        return perror("open, file_writer_attachfd"), -1;

    if (0 > ftruncate(fw->fd, fw->buffer_size))
        return perror("ftruncate, file_writer_init"), -1;

    fw->mem = mmap(NULL, fw->buffer_size, PROT_READ | PROT_WRITE, MAP_SHARED, fw->fd, 0);
    if (MAP_FAILED == fw->mem)
        return perror("mmap, file_writer_init"), -1;
    fw->capacity = fw->buffer_size;
    fw->next_loc = fw->base_loc + fw->buffer_size;
    return 0;
}

_RIBS_INLINE_ int file_writer_init(struct file_writer *fw, const char *filename) {
    unlink(filename);
    int fd = open(filename, O_RDWR | O_CREAT, 0644);
    return file_writer_attachfd(fw, fd, fw->buffer_size);
}

_RIBS_INLINE_ size_t file_writer_wavail(struct file_writer *fw) {
    return fw->capacity - fw->write_loc;
}

_RIBS_INLINE_ char *file_writer_wloc(struct file_writer *fw) {
    return fw->mem + fw->write_loc - fw->base_loc;
}

_RIBS_INLINE_ size_t file_writer_wlocpos(struct file_writer *fw) {
    return fw->write_loc;
}

_RIBS_INLINE_ int file_writer_wseek(struct file_writer *fw, size_t n) {
    fw->write_loc += n;
    if (fw->write_loc == fw->next_loc) {
        fw->base_loc = fw->write_loc;
        fw->next_loc = fw->base_loc + fw->buffer_size;
        if (fw->write_loc == fw->capacity) { /* can be false if used lseek */
            fw->capacity += fw->buffer_size;
            if (0 > ftruncate(fw->fd, fw->capacity))
                return perror("ftruncate, file_writer_wseek"), -1;
        }
        fw->mem = mmap(fw->mem, fw->buffer_size, PROT_READ | PROT_WRITE, MAP_FIXED | MAP_SHARED, fw->fd, fw->base_loc);
        if (MAP_FAILED == fw->mem)
            return perror("mmap, file_writer_init"), -1;
    }
    return 0;
}

_RIBS_INLINE_ int file_writer_lseek(struct file_writer *fw, off_t offset, int whence) {
    switch(whence) {
    case SEEK_SET:
        fw->write_loc = offset;
        break;
    case SEEK_CUR:
        fw->write_loc += offset;
        break;
    case SEEK_END:
        return errno = EINVAL, -1; /* not supported */
    }

    if (fw->write_loc > fw->capacity) {
        fw->capacity = (fw->write_loc + fw->buffer_size - 1) & ~(fw->buffer_size - 1);
        if (0 > ftruncate(fw->fd, fw->capacity))
            return perror("ftruncate, file_writer_wseek"), -1;
    }
    fw->base_loc = fw->write_loc & ~(fw->buffer_size - 1);
    fw->next_loc = fw->base_loc + fw->buffer_size;
    fw->mem = mmap(fw->mem, fw->buffer_size, PROT_READ | PROT_WRITE, MAP_FIXED | MAP_SHARED, fw->fd, fw->base_loc);
    if (MAP_FAILED == fw->mem)
        return perror("mmap, file_writer_init"), -1;
    return 0;
}

_RIBS_INLINE_ int file_writer_write(struct file_writer *fw, const void *buf, size_t size) {
    for (;;) {
        size_t wav = file_writer_wavail(fw);
        size_t n = wav >= size ? size : wav;
        memcpy(file_writer_wloc(fw), buf, n);
        file_writer_wseek(fw, n);
        size -= n;
        if (0 == size) break;
        buf += n;
    }
    return 0;
}

_RIBS_INLINE_ int file_writer_align(struct file_writer *fw) {
    return file_writer_wseek(fw, ((fw->write_loc + (size_t)7) & ~((size_t)7)) - fw->write_loc);
}

_RIBS_INLINE_ int file_writer_close(struct file_writer *fw) {
    int res = 0;
    if (fw->fd < 0)
        return 0;
    if (0 > munmap(fw->mem, fw->buffer_size))
        perror("munmap, file_writer_close");
    if (0 > ftruncate(fw->fd, fw->write_loc))
        perror("ftruncate, file_writer_close"), res = -1;
    close(fw->fd);
    fw->fd = -1;
    return res;
}
