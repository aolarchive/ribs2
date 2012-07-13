#ifndef _VMFILE__H_
#define _VMFILE__H_

#include "ribs_defs.h"
#include "template.h"
#include <sys/mman.h>
#include <stdarg.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#include "_vmbuf_preamble.h"
#define VMBUF_T vmfile
#define VMBUF_T_EXTRA                           \
    int fd
#include "_vmbuf.h"

#define VMFILE_INITIALIZER { NULL, 0, 0, 0, -1 }

_RIBS_INLINE_ int TEMPLATE(VMBUF_T,close)(struct VMBUF_T *vmb);

#define VMFILE_FTRUNCATE(size,funcname)                                 \
    if (0 > ftruncate(vmb->fd, (size)))                                 \
        return perror("ftruncate, " STRINGIFY(VMBUF_T) "_" funcname), -1;

_RIBS_INLINE_ int TEMPLATE(VMBUF_T,init)(struct VMBUF_T *vmb, const char *filename, size_t initial_size) {
    if (0 <= vmb->fd && 0 > vmfile_close(vmb))
        return -1;
    unlink(filename);
    vmb->fd = open(filename, O_CREAT | O_RDWR, 0644);
    if (vmb->fd < 0)
        return perror("open, " STRINGIFY(VMBUF_T) "_init"), -1;
    initial_size = vmbuf_align(initial_size);
    VMFILE_FTRUNCATE(initial_size, "init");
    vmb->buf = (char *)mmap(NULL, initial_size, PROT_WRITE | PROT_READ, MAP_SHARED, vmb->fd, 0);
    if (MAP_FAILED == vmb->buf) {
        perror("mmap, " STRINGIFY(VMBUF_T) "_init");
        vmb->buf = NULL;
        return -1;
    }
    vmb->capacity = initial_size;
    TEMPLATE(VMBUF_T,reset)(vmb);
    return 0;
}

_RIBS_INLINE_ int TEMPLATE(VMBUF_T,resize_to)(struct VMBUF_T *vmb, size_t new_capacity) {
    new_capacity = vmbuf_align(new_capacity);
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
    VMFILE_FTRUNCATE(vmb->write_loc, "close");
    vmfile_free(vmb);
    return close(vmb->fd);
}


#endif // _VMFILE__H_
