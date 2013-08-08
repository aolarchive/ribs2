#ifndef _BASE64__H_
#define _BASE64__H_

#include <stddef.h>

#define BASE64_ENCODED_LEN(x) (((x)+2)/3*4)
#define BASE64_DECODED_LEN(x) (((x)+3)/4*3)

#define BASE64_ENCODED_SIZE(x) (BASE64_ENCODED_LEN(x)+1)
#define BASE64_DECODED_SIZE(x) (BASE64_DECODED_LEN(x)+1)

unsigned char *base64_encode(void *dst, size_t *dstsize, const void *src, size_t srcsize, int padding);
unsigned char *base64_decode(void *dst, size_t *dstsize, const void *src, size_t srcsize);

#endif // _BASE64__H_
