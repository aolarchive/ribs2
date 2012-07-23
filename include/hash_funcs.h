/*
    This file is part of RIBS2.0 (Robust Infrastructure for Backend Systems).
    RIBS is an infrastructure for building great SaaS applications (but not
    limited to).

    Copyright (C) 2012 Adap.tv, Inc.

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
