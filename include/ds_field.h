
struct TEMPLATE(ds_field,T) {
    T *mem;
    size_t num_elements;
};

static inline int TEMPLATE_FUNC(ds_field,T,free)(struct TEMPLATE(ds_field,T) *dsf);
static inline int TEMPLATE_FUNC(ds_field,T,init)(struct TEMPLATE(ds_field,T) *dsf, const char *filename) {
    TEMPLATE_FUNC(ds_field,T,free)(dsf);
    int fd = open(filename, O_RDONLY | O_CLOEXEC);
    if (fd < 0)
        return LOGGER_PERROR_FUNC("open: %s", filename), -1;
    struct stat st;
    if (0 > fstat(fd, &st))
        return LOGGER_PERROR_FUNC("fstat %s", filename), close(fd), -1;
    off_t size = st.st_size;
    if (size < (off_t)sizeof(int64_t))
        return LOGGER_ERROR_FUNC("%s, wrong file size: %zd", __FUNCTION__, size), close(fd), -1;
    if (0 != ((size - sizeof(int64_t)) % sizeof(T)))
        return LOGGER_ERROR_FUNC("%s: data is misaligned", filename), close(fd), -1;
    void *mem = (char *)mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0);
    if (MAP_FAILED == mem)
        return LOGGER_PERROR_FUNC("mmap %s", filename), close(fd), -1;
    close(fd);
    int64_t t = *(int64_t *)mem;
    if (t != DS_TYPE(T))
        return LOGGER_ERROR_FUNC("%s: not the same type ([code] %s != [file] %s)", filename, STRINGIFY(DS_TYPE(T)), ds_type_to_string(t)), munmap(mem, size), -1;
    size -= sizeof(int64_t); /* exclude the header */
    dsf->mem = mem + sizeof(int64_t); /* skip the header */
    dsf->num_elements = size / sizeof(T);
    return 0;
}

static inline int TEMPLATE_FUNC(ds_field,T,free)(struct TEMPLATE(ds_field,T) *dsf) {
    if (!dsf->mem) return 0;
    int res;
    if (0 > (res = munmap(dsf->mem - sizeof(int64_t) /* include the header */, (dsf->num_elements * sizeof(T)) + sizeof(int64_t))))
        LOGGER_PERROR_FUNC("munmap");
    dsf->mem = NULL;
    return res;
}
