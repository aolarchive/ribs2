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
#define VMFILE_FTRUNCATE(size,funcname)                                 \
    if (0 > ftruncate(vmb->fd, (size)))                                 \
        return perror("ftruncate, " STRINGIFY(VMBUF_T) "_" funcname), -1;

_RIBS_INLINE_ int TEMPLATE(VMBUF_T,attachfd)(struct VMBUF_T *vmb, int fd, size_t initial_size) {
    if (0 > vmfile_close(vmb))
        return -1;
    vmb->fd = fd;
    initial_size = RIBS_VM_ALIGN(initial_size);
    VMFILE_FTRUNCATE(initial_size, "attachfd");
    vmb->buf = (char *)mmap(NULL, initial_size, PROT_WRITE | PROT_READ, MAP_SHARED, vmb->fd, 0);
    if (MAP_FAILED == vmb->buf) {
        perror("mmap, " STRINGIFY(VMBUF_T) "_attachfd");
        vmb->buf = NULL;
        return -1;
    }
    vmb->capacity = initial_size;
    TEMPLATE(VMBUF_T,reset)(vmb);
    return 0;
}

_RIBS_INLINE_ int TEMPLATE(VMBUF_T,init)(struct VMBUF_T *vmb, const char *filename, size_t initial_size) {
    unlink(filename);
    int fd = open(filename, O_CREAT | O_RDWR, 0644);
    if (fd < 0)
        return perror("open, " STRINGIFY(VMBUF_T) "_init"), -1;
    return vmfile_attachfd(vmb, fd, initial_size);
}

_RIBS_INLINE_ int TEMPLATE(VMBUF_T,resize_to)(struct VMBUF_T *vmb, size_t new_capacity) {
    new_capacity = RIBS_VM_ALIGN(new_capacity);
    VMFILE_FTRUNCATE(new_capacity, "resize_to");
    char *newaddr = (char *)mremap(vmb->buf, vmb->capacity, new_capacity, MREMAP_MAYMOVE);
    if ((void *)-1 == newaddr)
        return perror("mremap, " STRINGIFY(VMBUF_T) "_resize_to"), -1;
    // success
    vmb->buf = newaddr;
    vmb->capacity = new_capacity;
    return 0;
}

_RIBS_INLINE_ int TEMPLATE(VMBUF_T,close)(struct VMBUF_T *vmb) {
    if (vmb->fd < 0)
        return 0;
    VMFILE_FTRUNCATE(vmb->write_loc, "close");
    vmfile_free(vmb);
    int res = close(vmb->fd);
    vmb->fd = -1;
    return res;
}
