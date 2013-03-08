/*
    This file is part of RIBS2.0 (Robust Infrastructure for Backend Systems).
    RIBS is an infrastructure for building great SaaS applications (but not
    limited to).

    Copyright (C) 2012,2013 Adap.tv, Inc.

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
#ifndef BASE64_H_
#define BASE64_H_

#include <stdint.h>

#define B64_STANDARD 0     // Standard 'Base64' encoding for RFC 3548 or RFC 4648
#define B64_URL_FRIENDLY 1 // See tv.adap.util.TripleDesCrypter.modifiedUrlEncryptToBase64(String)
#define B64_URL_MODIFIED 2 // See tv.adap.util.URLUtils.makeUrlFriendlyBase64String(String)
#define B64_GOOGLE_URL_FRIENDLY 3 // used for Google AdX communication

#define BASE64_ENCODED_LEN(x) (((x)+2)/3*4)
#define BASE64_DECODED_LEN(x) (((x)+3)/4*3)

int ribs_base64_encode(char *dst, uint32_t *dstLen, const unsigned char *src, uint32_t srcLen, int mode);
int ribs_base64_decode(unsigned char *dst, uint32_t *dstLen, const char *src, uint32_t srcLen, int mode);

#endif /* BASE64_H_ */
