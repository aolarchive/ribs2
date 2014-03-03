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
#ifndef _ILOG2__H_
#define _ILOG2__H_

static inline uint32_t ilog2(uint32_t x) {
#if defined(__i386__) || defined(__x86_64__)
    uint32_t res;
    asm ("bsr %[x], %[res]"
         : [res] "=r" (res)
         : [x] "mr" (x));
    return res;
#else
    if (!x)
        return 0;
    return __builtin_clz(x) ^ 31;
#endif
}

static inline uint32_t next_p2(uint32_t x) {
    return 2U << ilog2(x-1);
}

static inline uint64_t ilog2_64(uint64_t x) {
#if defined(__i386__) || defined(__x86_64__)
    uint64_t res;
    asm ("bsr %[x], %[res]"
         : [res] "=r" (res)
         : [x] "mr" (x));
    return res;
#else
    if (!x)
        return 0;
    return __builtin_clz(x) ^ 63;
#endif
}

static inline uint64_t next_p2_64(uint64_t x) {
    return 2ULL << ilog2_64(x-1);
}

#endif // _ILOG2__H_
