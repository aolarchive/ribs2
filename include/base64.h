#ifndef BASE64_H_
#define BASE64_H_

#include <stdint.h>

#define B64_STANDARD 0     // Standard 'Base64' encoding for RFC 3548 or RFC 4648
#define B64_URL_FRIENDLY 1 // See tv.adap.util.TripleDesCrypter.modifiedUrlEncryptToBase64(String)
#define B64_URL_MODIFIED 2 // See tv.adap.util.URLUtils.makeUrlFriendlyBase64String(String)
#define B64_GOOGLE_URL_FRIENDLY 3 // used for Google AdX communication

uint32_t ribs_base64_encode_len(uint32_t n);
uint32_t ribs_base64_decode_len(uint32_t n);

int ribs_base64_encode(char *dst, uint32_t *dstLen, const unsigned char *src, uint32_t srcLen, int mode);
int ribs_base64_decode(unsigned char *dst, uint32_t *dstLen, const char *src, uint32_t srcLen, int mode);

#endif /* BASE64_H_ */
