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
#ifndef _RIBS_ZLIB__H_
#define _RIBS_ZLIB__H_

#include "ribs_defs.h"
#include "vmbuf.h"

int vmbuf_deflate(struct vmbuf *buf);
int vmbuf_deflate2(struct vmbuf *inbuf, struct vmbuf *outbuf);
int vmbuf_inflate(struct vmbuf *buf);
int vmbuf_inflate2(struct vmbuf *inbuf, struct vmbuf *outbuf);

#endif // _RIBS_ZLIB__H_
