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
#ifndef _BITSWAP__H_
#define _BITSWAP__H_

static inline uint32_t bitswap32(uint32_t x) {
    asm ("bswap %0": "=a" (x) : "0" (x) );
    return x;
}

static inline uint64_t bitswap64(uint64_t x) {
    asm ("bswap %0": "=a" (x) : "0" (x) );
    return x;
}


#endif // _BITSWAP__H_
