
struct TEMPLATE(ds_field,T) {
    T *mem;
    size_t num_elements;
};

static inline int TEMPLATE_FUNC(ds_field,T,free)(struct TEMPLATE(ds_field,T) *dsf);
static inline int TEMPLATE_FUNC(ds_field,T,init)(struct TEMPLATE(ds_field,T) *dsf, const char *filename) {
    TEMPLATE_FUNC(ds_field,T,free)(dsf);
    int fd = open(filename, O_RDONLY | O_CLOEXEC);
    if (fd < 0)
        return LOGGER_PERROR_FUNC("open"), -1;
    struct stat st;
    if (0 > fstat(fd, &st))
        return LOGGER_PERROR_FUNC("fstat"), close(fd), -1;
    off_t size = st.st_size;
    if (size < (off_t)sizeof(int64_t))
        return LOGGER_ERROR("%s, wrong file size: %zd", __FUNCTION__, size), close(fd), -1;
    void *mem = (char *)mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0);
    if (MAP_FAILED == mem)
        return LOGGER_PERROR_FUNC("mmap"), close(fd), -1;
    close(fd);
    int64_t t = *(int64_t *)mem;
    if (t != DS_TYPE(T))
        return LOGGER_ERROR("%s: not the same type ([code] %s != [file] %s)", filename, STRINGIFY(DS_TYPE(T)), ds_type_to_string(t)), munmap(mem, size), -1;
    size -= sizeof(int64_t); /* exclude the header */
    if (0 != (size % sizeof(T)))
        return LOGGER_ERROR("%s: data is misaligned", filename), munmap(mem, size), -1;
    dsf->mem = mem + sizeof(int64_t); /* skip the header */
    dsf->num_elements = size / sizeof(T);
    return 0;
}

static inline int TEMPLATE_FUNC(ds_field,T,free)(struct TEMPLATE(ds_field,T) *dsf) {
    void *mem = dsf->mem;
    if (!mem) return 0;
    mem -= sizeof(int64_t); /* include back the header */
    if (0 > munmap(mem, (dsf->num_elements * sizeof(T)) + sizeof(int64_t)))
        LOGGER_PERROR_FUNC("munmap");
    dsf->mem = NULL;
    return 0;
}

