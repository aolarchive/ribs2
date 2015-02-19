/*
    This file is part of RIBS2.0 (Robust Infrastructure for Backend Systems).
    RIBS is an infrastructure for building great SaaS applications (but not
    limited to).

    Copyright (C) 2012,2014 Adap.tv, Inc.

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
#include "vmbuf.h"
#include "logger.h"
#include <sys/mman.h>
#include "context.h"
#include "memalloc.h"
#include "file_utils.h"

#define MEM_SYNC_ONLY 0x0001

struct vmbuf_mem_funcs {
    size_t (*mem_alloc)(size_t size, void **mem, int fd);
    void (*mem_free)(void *mem, size_t size, int fd);
    size_t (*mem_resize)(void *mem, size_t old_size, size_t new_size, void **new_mem, int fd, uint16_t flags);
};

static inline size_t mem_roundup(size_t size) {
    return (size + 4095) & ~((size_t)4095);
}

/* anonymous buf */
static inline size_t _mem_alloc_ab(size_t size, void **mem, int fd UNUSED_ARG) {
    size = mem_roundup(size);
    *mem = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (MAP_FAILED == *mem)
        return LOGGER_PERROR("mmap"), 0;
    return size;
}

static inline void _mem_free_ab(void *mem, size_t size, int fd UNUSED_ARG) {
    if (0 > munmap(mem, size))
        LOGGER_PERROR("munmap");
}

static inline size_t _mem_resize_ab(void *mem, size_t old_size, size_t new_size, void **new_mem, int fd UNUSED_ARG, uint16_t flags UNUSED_ARG) {
    new_size = mem_roundup(new_size);
    *new_mem = mremap(mem, old_size, new_size, MREMAP_MAYMOVE);
    if (MAP_FAILED == *new_mem)
        return LOGGER_PERROR("mremap"), 0;
    return new_size;
}

/* shared buf */
static inline size_t _mem_alloc_sb(size_t size, void **mem, int fd) {
    size = mem_roundup(size);
    if (0 > ftruncate(fd, size))
        return LOGGER_PERROR("ftruncate"), 0;
    *mem = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (MAP_FAILED == *mem)
        return LOGGER_PERROR("mmap"), 0;
    return size;
}

static inline void _mem_free_sb(void *mem, size_t size, int fd) {
    if (0 > close(fd))
        LOGGER_PERROR("close");
    if (0 > munmap(mem, size))
        LOGGER_PERROR("munmap");
}

static inline size_t _mem_resize_sb(void *mem, size_t old_size, size_t new_size, void **new_mem, int fd, uint16_t flags) {
    new_size = mem_roundup(new_size);
    if (0 == (flags & MEM_SYNC_ONLY)) {
        if (0 > ftruncate(fd, new_size))
            return LOGGER_PERROR("ftruncate"), 0;
    }
    *new_mem = mremap(mem, old_size, new_size, MREMAP_MAYMOVE);
    if (MAP_FAILED == *new_mem)
        return LOGGER_PERROR("mremap"), 0;
    return new_size;
}

/* shared fixed buf */
static inline size_t _mem_alloc_sfb(size_t size, void **mem, int fd UNUSED_ARG) {
    size = mem_roundup(size);
    *mem = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    if (MAP_FAILED == *mem)
        return LOGGER_PERROR("mmap"), 0;
    return size;
}

static inline void _mem_free_sfb(void *mem, size_t size, int fd UNUSED_ARG) {
    if (0 > munmap(mem, size))
        LOGGER_PERROR("munmap");
}

static inline size_t _mem_resize_sfb(void *mem, size_t old_size, size_t new_size, void **new_mem UNUSED_ARG, int fd UNUSED_ARG, uint16_t flags UNUSED_ARG) {
    LOGGER_ERROR("can't resize fixed size buffer: %p, %zu ==> %zu", mem, old_size, new_size);
    return 0;
}


/* mem pool */
static inline size_t _mem_alloc_mp(size_t size, void **mem, int fd UNUSED_ARG) {
    return memalloc_alloc_raw(&current_ctx->memalloc, mem_roundup(size), mem);
}

static inline void _mem_free_mp(void *mem, size_t size UNUSED_ARG, int fd UNUSED_ARG) {
    return memalloc_free_raw(mem);
}

static inline size_t _mem_resize_mp(void *mem, size_t old_size, size_t new_size, void **new_mem, int fd UNUSED_ARG, uint16_t flags UNUSED_ARG) {
    size_t s = memalloc_alloc_raw(&current_ctx->memalloc, new_size, new_mem);
    memcpy(*new_mem, mem, new_size > old_size ? old_size : new_size);
    memalloc_free_raw(mem);
    return s;
}

struct vmbuf_mem_funcs _mem_funcs_anon_buf = { _mem_alloc_ab, _mem_free_ab, _mem_resize_ab };
struct vmbuf_mem_funcs _mem_funcs_shared_buf = { _mem_alloc_sb, _mem_free_sb, _mem_resize_sb };
struct vmbuf_mem_funcs _mem_funcs_shared_fixed_buf = { _mem_alloc_sfb, _mem_free_sfb, _mem_resize_sfb };
struct vmbuf_mem_funcs _mem_funcs_mem_pool = { _mem_alloc_mp, _mem_free_mp, _mem_resize_mp };

static int _vmbuf_init(struct vmbuf *vmbuf, size_t initial_size) {
    size_t size = vmbuf->funcs->mem_alloc(initial_size + sizeof(struct vmbuf_header), &vmbuf->mem, vmbuf->fd);
    if (0 == size)
        return -1;
    struct vmbuf_header *header = vmbuf->mem;
    vmbuf->mem += sizeof(struct vmbuf_header); /* skip header */
    vmbuf->capacity = header->capacity = size - sizeof(struct vmbuf_header);
    vmbuf->rloc = header->wloc = 0;
    return 0;
}


int vmbuf_init(struct vmbuf *vmbuf, size_t initial_size) {
    if (vmbuf->mem) {
        vmbuf_reset(vmbuf);
        return 0;
    }
    vmbuf->funcs = &_mem_funcs_anon_buf;
    return _vmbuf_init(vmbuf, initial_size);
}

int vmbuf_init_tmp(struct vmbuf *vmbuf, size_t initial_size) {
    vmbuf_free(vmbuf);
    vmbuf->funcs = &_mem_funcs_mem_pool;
    return _vmbuf_init(vmbuf, initial_size);
}

int vmbuf_init_shared(struct vmbuf *vmbuf, size_t initial_size) {
    vmbuf_free(vmbuf);
    vmbuf->funcs = &_mem_funcs_shared_buf;
    vmbuf->fd = ribs_create_temp_file("ribs2vmb");
    if (0 > vmbuf->fd)
        return -1;
    return _vmbuf_init(vmbuf, initial_size);
}

int vmbuf_init_shared_fixed(struct vmbuf *vmbuf, size_t max_size) {
    vmbuf_free(vmbuf);
    vmbuf->funcs = &_mem_funcs_shared_fixed_buf;
    return _vmbuf_init(vmbuf, max_size);
}

int vmbuf_free(struct vmbuf *vmbuf) {
    if (NULL == vmbuf->mem)
        return 0;
    vmbuf->funcs->mem_free(vmbuf->mem - sizeof(struct vmbuf_header), vmbuf->capacity + sizeof(struct vmbuf_header), vmbuf->fd);
    vmbuf->mem = NULL;
    return 0;
}

static inline int _vmbuf_change_capacity(struct vmbuf *vmbuf, size_t new_capacity) {
    new_capacity = vmbuf->funcs->mem_resize(vmbuf->mem - sizeof(struct vmbuf_header), vmbuf->capacity + sizeof(struct vmbuf_header), new_capacity, &vmbuf->mem, vmbuf->fd, 0);
    if (0 == new_capacity)
        return -1;
    vmbuf->mem += sizeof(struct vmbuf_header);
    vmbuf->capacity = vmbuf_get_header(vmbuf)->capacity = new_capacity - sizeof(struct vmbuf_header);
    return 0;
}

int vmbuf_add_capacity(struct vmbuf *vmbuf) {
    return _vmbuf_change_capacity(vmbuf, (vmbuf->capacity << 1)  + sizeof(struct vmbuf_header));
}

int vmbuf_resize_to(struct vmbuf *vmbuf, size_t size) {
    return _vmbuf_change_capacity(vmbuf, size  + sizeof(struct vmbuf_header));
}

int vmbuf_sync(struct vmbuf *vmbuf) {
    struct vmbuf_header *header = vmbuf_get_header(vmbuf);
    if (vmbuf->capacity != header->capacity) {
        if (0 == vmbuf->funcs->mem_resize(vmbuf->mem - sizeof(struct vmbuf_header),
                                          vmbuf->capacity + sizeof(struct vmbuf_header), /* old size */
                                          header->capacity + sizeof(struct vmbuf_header), /* new size */
                                          &vmbuf->mem, vmbuf->fd, MEM_SYNC_ONLY))
            return LOGGER_ERROR("vmbuf_sync failed"), -1;
        vmbuf->mem += sizeof(struct vmbuf_header);
        header = vmbuf_get_header(vmbuf); /* pointer may change after resize */
        vmbuf->capacity = header->capacity;
    }
    return 0;
}
