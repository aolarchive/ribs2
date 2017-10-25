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
#include "ringfile.h"
#include "file_utils.h"
#include "logger.h"
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "vm_misc.h"
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>

static inline int _ringfile_read_header(const char *filename, struct ringfile_header *header) {
    size_t header_size = RIBS_VM_ALIGN(sizeof(struct ringfile_header));
    int fd = open(filename, O_RDONLY | O_NOATIME);
    if (0 > fd) {
        if (ENOENT != errno)
            return LOGGER_PERROR("%s", filename), -1;
        memset(header, 0, sizeof(struct ringfile_header));
        return 0;
    }
    struct stat st;
    if (0 > fstat(fd, &st))
        return LOGGER_PERROR("fstat: %d, %s", fd, filename), close(fd), -1;
    if (st.st_size < (off_t)sizeof(struct ringfile_header))
        return LOGGER_PERROR("invalid size: %jd < %zu", (intmax_t)st.st_size, sizeof(struct ringfile_header)), close(fd), -1;
    void *mem = mmap(NULL, header_size, PROT_READ, MAP_SHARED, fd, 0);
    if (MAP_FAILED == mem)
        return LOGGER_PERROR("mmap"), close(fd), -1;
    memcpy(header, mem, sizeof(struct ringfile_header));
    if (0 > close(fd))
        LOGGER_PERROR("close: %s", filename);
    if (0 > munmap(mem, header_size))
        LOGGER_PERROR("munmap: %s", filename);
    return 0;
}

int _ringfile_resize(const char *filename, size_t capacity, size_t reserved, void (*evict_one_rec)(struct ringfile*)) {
    char filename_new[PATH_MAX];
    const char NEW_EXT[] = "new.temp";
    if (PATH_MAX <= snprintf(filename_new, PATH_MAX, "%s.%s", filename, NEW_EXT))
        return LOGGER_ERROR("filename too long: %s.%s", filename, NEW_EXT), -1;
    struct ringfile_header header;
    if (0 > _ringfile_read_header(filename, &header))
        return LOGGER_ERROR("failed to read header: %s", filename), -1;
    if (0 == header.capacity)
        return 0;
    size_t reserved_aligned = RIBS_VM_ALIGN(reserved + sizeof(struct ringfile_header));
    size_t capacity_aligned = RIBS_VM_ALIGN(capacity);
    if (header.capacity == capacity_aligned && header.reserved_size == reserved_aligned)
        return 0;
    struct ringfile old = RINGFILE_INITIALIZER, new = RINGFILE_INITIALIZER;
    if (0 > ringfile_init(&old, filename, header.capacity, header.reserved_size - sizeof(struct ringfile_header)))
        return LOGGER_ERROR("failed to open existing ringfile"), -1;
    LOGGER_INFO("resizing: %s, reserved: %zu ==> %zu, size: %zu ==> %zu", filename, header.reserved_size, reserved_aligned, header.capacity, capacity_aligned);
    if (evict_one_rec) {
        while (ringfile_size(&old) > capacity_aligned) {
            evict_one_rec(&old);
        }
    } else {
        if (ringfile_size(&old) > capacity_aligned)
            ringfile_rseek(&old, ringfile_size(&old) - capacity_aligned);
    }
    if (0 > unlink(filename_new) && errno != ENOENT)
        return LOGGER_PERROR("%s", filename_new), ringfile_free(&old), -1;
    if (0 > ringfile_init(&new, filename_new, capacity, reserved))
        return LOGGER_ERROR("failed to create new ringfile: %s", filename_new), ringfile_free(&old), -1;
    /* reserved */
    memcpy(ringfile_get_reserved(&new),
           ringfile_get_reserved(&old),
           ribs_min(header.reserved_size - sizeof(struct ringfile_header),
                    reserved_aligned - sizeof(struct ringfile_header)));
    /* data */
    memcpy(ringfile_wloc(&new), ringfile_rloc(&old), ringfile_size(&old));
    ringfile_wseek(&new, ringfile_size(&old));

    ringfile_free(&old);
    ringfile_free(&new);
    if (0 > rename(filename_new, filename))
        return LOGGER_PERROR("rename: %s ==> %s", filename_new, filename), -1;
    return 0;
}

int ringfile_init(struct ringfile *rb, const char *filename, size_t size, size_t reserved) {
    reserved = RIBS_VM_ALIGN(reserved + sizeof(struct ringfile_header));
    size_t capacity = RIBS_VM_ALIGN(size);
    int fd = open(filename, O_RDWR | O_CREAT | O_NOATIME, 0644);
    if (0 > fd) return LOGGER_PERROR("%s", filename), -1;
    if (0 > ftruncate(fd, capacity + reserved))
        return LOGGER_PERROR("ftruncate"), close(fd), -1;
    size_t total_vmsize = reserved + (capacity << 1);
    rb->mem = mmap(NULL, total_vmsize, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (MAP_FAILED == rb->mem)
        return LOGGER_PERROR("mmap"), close(fd), -1;
    void *tmp = mmap(rb->mem, reserved + capacity, PROT_READ | PROT_WRITE, MAP_FIXED | MAP_SHARED, fd, 0);
    if (MAP_FAILED == tmp)
        return LOGGER_PERROR("mmap"), close(fd), -1;
    if (RINGFILE_HEADER->capacity != capacity || RINGFILE_HEADER->reserved_size != reserved)
        RINGFILE_HEADER->read_loc = RINGFILE_HEADER->write_loc = 0; /* reset */
    RINGFILE_HEADER->capacity = capacity;
    RINGFILE_HEADER->reserved_size = reserved;
    rb->rbuf = rb->mem + reserved;
    tmp = mmap(rb->rbuf + capacity, capacity, PROT_READ | PROT_WRITE, MAP_FIXED | MAP_SHARED, fd, reserved);
    if (MAP_FAILED == tmp)
        return LOGGER_PERROR("mmap"), munmap(rb->mem, total_vmsize), close(fd), -1;
    close(fd);
    return 0;
}

int ringfile_init_with_resize(struct ringfile *rb, const char *filename, size_t size, size_t reserved, void (*evict_one_rec)(struct ringfile *)) {
    if (0 > _ringfile_resize(filename, size, reserved, evict_one_rec))
        return -1;
    return ringfile_init(rb, filename, size, reserved);
}

int ringfile_init_safe_resize(struct ringfile *rb, const char *filename, size_t size, size_t reserved) {
    struct ringfile_header header;
    if (0 > _ringfile_read_header(filename, &header))
        return LOGGER_ERROR("failed to read header: %s", filename), -1;
    size_t reserved_aligned = RIBS_VM_ALIGN(reserved + sizeof(struct ringfile_header));
    size_t capacity_aligned = RIBS_VM_ALIGN(size);
    if (0 != header.capacity) {
        if (header.reserved_size != reserved_aligned)
            return LOGGER_ERROR("reserved size mismatch, %zu != %zu", header.reserved_size, reserved_aligned), -1;
        if (capacity_aligned < header.capacity)
            return LOGGER_ERROR("capacity can't be reduced, %zu < %zu", capacity_aligned, header.capacity), -1;
    }
    return ringfile_init(rb, filename, size, reserved);
}

int ringfile_free(struct ringfile *rb) {
    if (rb->mem && 0 > munmap(rb->mem, RINGFILE_HEADER->reserved_size + (RINGFILE_HEADER->capacity << 1)))
        return LOGGER_PERROR("munmap"), -1;
    rb->mem = NULL;
    return 0;
}

int ringfile_sync(struct ringfile *rb) {
    if (rb->mem && 0 > msync(rb->mem, RINGFILE_HEADER->reserved_size + RINGFILE_HEADER->capacity, MS_SYNC))
        return LOGGER_PERROR("msync"), -1;
    return 0;
}
