/*
    This file is part of RIBS (Robust Infrastructure for Backend Systems).
    RIBS is an infrastructure for building great SaaS applications (but not
    limited to).

    Copyright (C) 2011 Adap.tv, Inc.

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

#include <stdint.h>

static inline uint32_t ilog2(uint32_t x)
{
    uint32_t res;
    asm ("bsr %[x], %[res]"
         : [res] "=r" (res)
         : [x] "mr" (x));
    return res;
}

static inline uint32_t next_p2(uint32_t x)
{
    return 2U << ilog2(x-1);
}

#endif // _ILOG2__H_
