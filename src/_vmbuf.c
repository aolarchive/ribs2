_RIBS_INLINE_ int TEMPLATE(VMBUF_T,init)(struct VMBUF_T *vmb, size_t initial_size) {
    if (NULL == vmb->buf) {
        initial_size = RIBS_VM_ALIGN(initial_size);
        vmb->buf = (char *)mmap(NULL, initial_size, PROT_WRITE | PROT_READ, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
        if (MAP_FAILED == vmb->buf) {
            perror("mmap, vmbuf_init");
            vmb->buf = NULL;
            return -1;
        }
        vmb->capacity = initial_size;
    } else if (vmb->capacity < initial_size)
        TEMPLATE(VMBUF_T,resize_to)(vmb, initial_size);
    TEMPLATE(VMBUF_T,reset)(vmb);
    return 0;
}

_RIBS_INLINE_ int TEMPLATE(VMBUF_T,resize_to)(struct VMBUF_T *vmb, size_t new_capacity) {
    new_capacity = RIBS_VM_ALIGN(new_capacity);
    char *newaddr = (char *)mremap(vmb->buf, vmb->capacity, new_capacity, MREMAP_MAYMOVE);
    if ((void *)-1 == newaddr)
        return perror("mremap, " STRINGIFY(VMBUF_T) "_resize_to"), -1;
    // success
    vmb->buf = newaddr;
    vmb->capacity = new_capacity;
    return 0;
}
