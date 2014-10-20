/*
    This file is part of RIBS2.0 (Robust Infrastructure for Backend Systems).
    RIBS is an infrastructure for building great SaaS applications (but not
    limited to).

    Copyright (C) 2013,2014 Adap.tv, Inc.

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
#include "vmallocator.h"
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "logger.h"

static inline int _resize_buf(struct vmallocator *v, size_t new_size) {
    void *mem = mremap(v->mem, v->capacity, new_size, MREMAP_MAYMOVE);
    if (MAP_FAILED == mem)
        return LOGGER_PERROR("mremap, new size = %zu", new_size), -1;
    v->capacity = new_size;
    v->mem = mem;
    return 0;
}

static inline int _resize_file(struct vmallocator *v, size_t new_size) {
    if (0 > ftruncate(v->fd, new_size))
        return LOGGER_PERROR("ftruncate, new size = %zu", new_size), -1;
    return _resize_buf(v, new_size);
}

int vmallocator_open(struct vmallocator *v, const char *filename, int flags) {
    if (v->mem)
        vmallocator_free(v);
    int fd = open(filename, flags, 0644);
    if (0 > fd)
        return LOGGER_PERROR("open %s", filename), -1;
    struct stat st;
    if (0 > fstat(fd, &st))
        return LOGGER_PERROR("fstat, %s", filename), close(fd), -1;
    v->wlocpos = st.st_size;
    v->capacity = st.st_size;
    int prot = PROT_READ;
    if (O_RDWR & flags || O_WRONLY & flags) {
        if (flags & O_TRUNC)
            v->wlocpos = v->capacity = 0;
        v->capacity = VM_ALLOC_MIN_SIZE > v->capacity ? VM_ALLOC_MIN_SIZE : VM_ALLOC_ALIGN(v->capacity, VM_ALLOC_MIN_SIZE);
        prot = PROT_READ | PROT_WRITE;
        if (0 > ftruncate(fd, v->capacity))
            return LOGGER_PERROR("ftruncate, %s", filename), close(fd), -1;
    }
    void *mem;
    if (0 == v->capacity) {
        /* empty file */
        close(fd);
        mem = NULL;
    } else {
        mem = mmap(NULL, v->capacity, prot, MAP_SHARED, fd, 0);
        if (MAP_FAILED == mem)
            return LOGGER_PERROR("mmap"), close(fd), -1;
    }
    v->mem = mem;
    v->fd = fd;
    v->resize_func = _resize_file;
    return 0;
}

int vmallocator_close(struct vmallocator *v) {
    int res = 0;
    if (0 <= v->fd && v->capacity != v->wlocpos && 0 > (res = ftruncate(v->fd, v->wlocpos)))
        LOGGER_PERROR("ftruncate: %d", v->fd);
    vmallocator_free(v);
    return res;
}

int vmallocator_init(struct vmallocator *v) {
    if (v->mem) {
        vmallocator_reset(v);
        return 0;
    };
    v->fd = -1;
    v->capacity = VM_ALLOC_MIN_SIZE;
    void *mem = mmap(NULL, v->capacity, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (MAP_FAILED == v->mem)
        return LOGGER_PERROR("mmap"), -1;
    v->mem = mem;
    v->wlocpos = 0;
    v->resize_func = _resize_buf;
    return 0;
}

void vmallocator_free(struct vmallocator *v) {
    if (NULL == v->mem)
        return;
    if (0 <= v->fd) {
        close(v->fd);
        v->fd = -1;
    }
    munmap(v->mem, v->capacity);
    v->mem = NULL;
    v->capacity = 0;
    v->wlocpos = 0;
}
