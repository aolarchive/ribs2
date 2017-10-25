/*
    This file is part of RIBS2.0 (Robust Infrastructure for Backend Systems).
    RIBS is an infrastructure for building great SaaS applications (but not
    limited to).

    Copyright (C) 2013 Adap.tv, Inc.

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
#include "ringbuf.h"
#include "file_utils.h"
#include "logger.h"
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "vm_misc.h"

int ringbuf_init(struct ringbuf *rb, size_t size) {
    rb->capacity = RIBS_VM_ALIGN(size);
    rb->read_loc = rb->write_loc = 0;
    int fd = ribs_create_temp_file("ribs_rb");
    if (0 > fd) return -1;
    if (0 > ftruncate(fd, rb->capacity))
        return LOGGER_PERROR("ftruncate"), close(fd), -1;
    size_t total_vmsize = rb->capacity << 1;
    rb->mem = mmap(NULL, total_vmsize, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (MAP_FAILED == rb->mem)
        return LOGGER_PERROR("mmap"), close(fd), -1;
    void *tmp = mmap(rb->mem, rb->capacity, PROT_READ | PROT_WRITE, MAP_FIXED | MAP_SHARED, fd, 0);
    if (MAP_FAILED == tmp)
        return LOGGER_PERROR("mmap"), munmap(rb->mem, total_vmsize), close(fd), -1;
    tmp = mmap(rb->mem + rb->capacity, rb->capacity, PROT_READ | PROT_WRITE, MAP_FIXED | MAP_SHARED, fd, 0);
    if (MAP_FAILED == tmp)
        return LOGGER_PERROR("mmap"), munmap(rb->mem, total_vmsize), close(fd), -1;
    close(fd);
    return 0;
}

int ringbuf_free(struct ringbuf *rb) {
    if (rb->mem && 0 > munmap(rb->mem, rb->capacity << 1))
        return LOGGER_PERROR("munmap"), -1;
    rb->mem = NULL;
    return 0;
}
