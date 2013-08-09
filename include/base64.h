/*
    This file is part of RIBS2.0 (Robust Infrastructure for Backend Systems).
    RIBS is an infrastructure for building great SaaS applications (but not
    limited to).

    Copyright (C) 2013 Adap.tv, Inc.

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
