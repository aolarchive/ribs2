_RIBS_INLINE_ void file_writer_make(struct file_writer *fw) {
    *fw = (struct file_writer)FILE_WRITER_INITIALIZER;
}

_RIBS_INLINE_ int file_writer_init(struct file_writer *fw, const char *filename) {
    file_writer_close(fw);
    unlink(filename);
    fw->mem = NULL;
    fw->base_loc = fw->write_loc = 0;
    fw->fd = open(filename, O_RDWR | O_CREAT, 0644);
    if (0 > fw->fd)
        return perror("open, file_writer_init"), -1;

    if (0 > ftruncate(fw->fd, fw->buffer_size))
        return perror("ftruncate, file_writer_init"), -1;

    fw->mem = mmap(NULL, fw->buffer_size, PROT_READ | PROT_WRITE, MAP_SHARED, fw->fd, 0);
    if (MAP_FAILED == fw->mem)
        return perror("mmap, file_writer_init"), -1;
    fw->capacity = fw->buffer_size;
    return 0;
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
    if (fw->write_loc == fw->capacity) {
        fw->base_loc = fw->write_loc;
        fw->capacity += fw->buffer_size;
        if (0 > ftruncate(fw->fd, fw->capacity))
            return perror("ftruncate, file_writer_wseek"), -1;
        fw->mem = mmap(fw->mem, fw->buffer_size, PROT_READ | PROT_WRITE, MAP_FIXED | MAP_SHARED, fw->fd, fw->base_loc);
        if (MAP_FAILED == fw->mem)
            return perror("mmap, file_writer_init"), -1;
    }
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

_RIBS_INLINE_ int file_writer_close(struct file_writer *fw) {
    if (fw->fd < 0)
        return 0;
    if (0 > munmap(fw->mem, fw->buffer_size))
        perror("munmap, file_writer_close");
    if (0 > ftruncate(fw->fd, fw->write_loc))
        perror("ftruncate, file_writer_close");
    close(fw->fd);
    fw->fd = -1;
    return 0;
}
