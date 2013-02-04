#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "base64.h"

#define B64_DEC_MIN 43
#define B64_DEC_MAX 122
#define B64_DEC_OFFSET 43
#define B64_DEC_ASCII_OFFSET 62
#define ENC_WIDTH 4
#define DEC_WIDTH 3
#define INVALID '$'

static const char b64_enc_tables[][65] = {
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/", // STANDARD
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+_", // URL_FRIENDLY
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-/", // URL_MODIFIED
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_"  // GOOGLE_URL_FRIENDLY
};

static const char pads[] = {
        '=', // STANDARD
        '=', // URL_FRIENDLY
        '_', // URL_MODIFIED
        '='  // GOOGLE_URL_FRIENDLY
};

static const char b64_dec_tables[][82] = {
        "|$$$}rstuvwxyz{$$$$$$$>?@ABCDEFGHIJKLMNOPQRSTUVW$$$$$$XYZ[\\]^_`abcdefghijklmnopq", // STANDARD
        "|$$$$rstuvwxyz{$$$$$$$>?@ABCDEFGHIJKLMNOPQRSTUVW$$$$}$XYZ[\\]^_`abcdefghijklmnopq", // URL_FRIENDLY
        "$$|$}rstuvwxyz{$$$$$$$>?@ABCDEFGHIJKLMNOPQRSTUVW$$$$$$XYZ[\\]^_`abcdefghijklmnopq", // URL_MODIFIED
        "$$|$$rstuvwxyz{$$$$$$$>?@ABCDEFGHIJKLMNOPQRSTUVW$$$$}$XYZ[\\]^_`abcdefghijklmnopq"  // GOOGLE_URL_FRIENDLY
};

int _ribs_base64_encode(char *dst, uint32_t *dstLen, const unsigned char *src, uint32_t srcLen, const char b64_enc[], const char pad);
int _ribs_base64_decode(unsigned char *dst, uint32_t *dstLen, const char *src, uint32_t srcLen, const char b64_dec[], const char pad);

int ribs_base64_encode(
        char *dst,                  // Use 'base64_decode_len()' for required result buffer size
        uint32_t *dstLen,           // The length of the result string
        const unsigned char *src,   // Source buffer
        uint32_t srcLen,            // Length of the source buffer
        int mode                    // Base64 encoding mode B64_STANDARD, B64_URL_FRIENDLY, B64_URL_MODIFIED
) {
    return _ribs_base64_encode(dst, dstLen, src, srcLen, b64_enc_tables[mode], pads[mode]);
}

int _ribs_base64_encode(
        char *dst,                  // Use 'base64_decode_len()' for required result buffer size
        uint32_t *dstLen,           // The length of the result string
        const unsigned char *src,   // Source buffer
        uint32_t srcLen,            // Length of the source buffer
        const char b64_enc[],       // encoding table to use (depends on mode)
        const char pad              // padding character to use (depends on mode)
) {
    if (dst == NULL || src == NULL)
        return -1;

    uint32_t srcPos, dstChars = DEC_WIDTH;
    unsigned char triple[DEC_WIDTH];

    *dstLen = 0;
    for (srcPos = 0; srcPos < srcLen; srcPos += DEC_WIDTH, *dstLen += ENC_WIDTH) {
        if (srcLen - srcPos < DEC_WIDTH) {
            dstChars = srcLen - srcPos;
            memset(&triple, 0, DEC_WIDTH);
        }
        memcpy(&triple, src + srcPos, dstChars);

        dst[*dstLen] = b64_enc[(triple[0] & 0xFC) >> 2];
        dst[*dstLen + 1] = b64_enc[((triple[0] & 0x03) << 4) | ((triple[1] & 0xF0) >> 4)];

        if (dstChars < 2)
            dst[*dstLen + 2] = pad;
        else
            dst[*dstLen + 2] = b64_enc[((triple[1] & 0x0F) << 2) | ((triple[2] & 0xC0) >> 6)];

        if (dstChars < 3)
            dst[*dstLen + 3] = pad;
        else
            dst[*dstLen + 3] = b64_enc[triple[2] & 0x3F];
    }
    return 0;
}

int ribs_base64_decode(
        unsigned char *dst,   // Use 'base64_decode_len()' for required result buffer size
        uint32_t *dstLen,     // The length of the result used in the dst buffer
        const char *src,      // Source string
        uint32_t srcLen,      // Length of the source string
        int mode              // Base64 encoding mode B64_STANDARD, B64_URL_FRIENDLY, B64_URL_MODIFIED
) {
    return _ribs_base64_decode(dst, dstLen, src, srcLen, b64_dec_tables[mode], pads[mode]);
}

int _ribs_base64_decode(
        unsigned char *dst,   // Use 'base64_decode_len()' for required result buffer size
        uint32_t *dstLen,     // The length of the result used in the dst buffer
        const char *src,      // Source string
        uint32_t srcLen,      // Length of the source string
        const char b64_dec[], // decoding table to use (depends on mode)
        const char pad        // padding character to use (depends on mode)
) {
    if (dst == NULL || src == NULL)
        return -1;

    char in[ENC_WIDTH];
    char c;
    uint32_t i, srcOff, dstOff, dstBytes;

    if (srcLen % ENC_WIDTH != 0)
        return -1;

    *dstLen = 0;
    for (srcOff = 0, dstOff = 0; srcOff < srcLen; srcOff += ENC_WIDTH, dstOff += DEC_WIDTH) {
        dstBytes = DEC_WIDTH;
        for (i = 0; i < ENC_WIDTH; i++ ) {
            c = src[srcOff + i];
            if (c < B64_DEC_MIN || c > B64_DEC_MAX) {
                return -1;
            } else if (c == pad) {
                in[i] = '\0';
                if (dstBytes > 0) --dstBytes;
            } else {
                c = b64_dec[c - B64_DEC_OFFSET];
                if (c == INVALID)
                    return -1;
                in[i] = c - B64_DEC_ASCII_OFFSET;
            }
        }
        *dstLen += dstBytes;
        dst[dstOff]     = in[0] << 2 | in[1] >> 4;
        dst[dstOff + 1] = in[1] << 4 | in[2] >> 2;
        dst[dstOff + 2] = ((in[2] << 6) & 0xc0) | in[3];
    }
    return 0;
}
