#ifndef _HASH_FUNCS__H_
#define _HASH_FUNCS__H_

#if 0
static inline uint32_t hashcode(const void *key, size_t n) {
    register const unsigned char *p = (const unsigned char *)key;
    register const unsigned char *end = p + n;
    uint32_t h = 5381;
    for (; p != end; ++p)
        h = ((h << 5) + h) ^ *p;
    return h;
}
#endif
static inline uint32_t hashcode(const void *key, size_t n)
{
    register const unsigned char *p = (const unsigned char *)key;
    register const unsigned char *end = p + n;
    register uint32_t h = 0;
    const uint32_t prime = 0x811C9DC5;
    for (; p != end; ++p)
        h = (h ^ *p) * prime;
    return h;
}



#endif // _HASH_FUNCS__H_
