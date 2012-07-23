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
#ifdef VMBUF_T
#undef VMBUF_T
#endif

#ifdef VMBUF_T_EXTRA
#undef VMBUF_T_EXTRA
#endif


#ifndef VMBUF_ONE_TIME
#define VMBUF_ONE_TIME

#define PAGEMASK 4095
#define PAGESIZE 4096

_RIBS_INLINE_ off_t vmbuf_align(off_t off) {
    off += PAGEMASK;
    off &= ~PAGEMASK;
    return off;
}

#endif // VMBUF_ONE_TIME
