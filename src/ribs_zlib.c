/*
    This file is part of RIBS2.0 (Robust Infrastructure for Backend Systems).
    RIBS is an infrastructure for building great SaaS applications (but not
    limited to).

    Copyright (C) 2013,2014 Adap.tv, Inc.

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
#include "ribs_zlib.h"
#include <zlib.h>
#include <limits.h>

int vmbuf_deflate(struct vmbuf *buf) {
    return vmbuf_deflate3(buf, Z_DEFAULT_COMPRESSION);
}

int vmbuf_deflate3(struct vmbuf *buf, int level) {
    static struct vmbuf outbuf = VMBUF_INITIALIZER;
    vmbuf_init(&outbuf, vmbuf_ravail(buf));
    if (0 > vmbuf_deflate4(buf, &outbuf, level))
        return -1;
    vmbuf_swap(&outbuf, buf);
    return 0;
}


int vmbuf_deflate2(struct vmbuf *inbuf, struct vmbuf *outbuf) {
    return vmbuf_deflate4(inbuf, outbuf, Z_DEFAULT_COMPRESSION);
}

int vmbuf_deflate4(struct vmbuf *inbuf, struct vmbuf *outbuf, int level) {
    z_stream strm;
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    if (Z_OK != deflateInit2(&strm, level, Z_DEFLATED, 15+16, 8, Z_DEFAULT_STRATEGY))
        return -1;
    int flush;
    do {
        strm.next_in = (uint8_t *)vmbuf_rloc(inbuf);
        strm.avail_in = vmbuf_ravail(inbuf);
        vmbuf_resize_if_less(outbuf, strm.avail_in << 1);
        strm.next_out = (uint8_t *)vmbuf_wloc(outbuf);
        strm.avail_out = vmbuf_wavail(outbuf);
        flush = vmbuf_ravail(inbuf) == 0 ? Z_FINISH : Z_NO_FLUSH;
        if (Z_STREAM_ERROR == deflate(&strm, flush)) {
            deflateEnd(&strm);
            return -1;
        }
        vmbuf_wseek(outbuf, vmbuf_wavail(outbuf) - strm.avail_out);
        vmbuf_rseek(inbuf, vmbuf_ravail(inbuf) - strm.avail_in);
    } while (flush != Z_FINISH);
    deflateEnd(&strm);
    return 0;
}

int vmbuf_inflate(struct vmbuf *buf) {
    static struct vmbuf outbuf = VMBUF_INITIALIZER;
    vmbuf_init(&outbuf, vmbuf_ravail(buf));
    if (0 > vmbuf_inflate2(buf, &outbuf))
        return -1;
    vmbuf_swap(&outbuf, buf);
    return 0;
}

int vmbuf_inflate2(struct vmbuf *inbuf, struct vmbuf *outbuf) {
    z_stream strm;
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    if (Z_OK != inflateInit2(&strm, 15+16))
        return -1;
    for (;;)
    {
        strm.next_in = (uint8_t *)vmbuf_rloc(inbuf);
        strm.avail_in = vmbuf_ravail(inbuf);
        if (0 == strm.avail_in)
            break;
        vmbuf_resize_if_less(outbuf, strm.avail_in << 1);
        strm.next_out = (uint8_t *)vmbuf_wloc(outbuf);
        strm.avail_out = vmbuf_wavail(outbuf);
        int res = inflate(&strm, Z_NO_FLUSH);
        vmbuf_wseek(outbuf, vmbuf_wavail(outbuf) - strm.avail_out);
        if (res == Z_STREAM_END)
            break;
        if (Z_OK != res)
            return inflateEnd(&strm), -1;
        vmbuf_rseek(inbuf, vmbuf_ravail(inbuf) - strm.avail_in);
    }
    inflateEnd(&strm);
    return 0;
}

int vmbuf_inflate_gzip(void *inbuf, size_t in_size, struct vmbuf *outbuf)
{
    if (NULL == inbuf || 0 == in_size)
        return -1;
    z_stream strm;
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.next_in = (uint8_t*)inbuf;
    size_t avail_in = (in_size > UINT_MAX) ? UINT_MAX : in_size;
    strm.avail_in = avail_in;
    if (Z_OK != inflateInit2(&strm, 15 + 32)) {
        return -1;
    }
    size_t total_read = 0;
    for (;;)
    {
        if (0 == strm.avail_in)
            break;
        if (0 > vmbuf_resize_if_less(outbuf, ((size_t)strm.avail_in) << 1)) {
            return inflateEnd(&strm), -1;
        }
        strm.next_out = (uint8_t*)vmbuf_wloc(outbuf);
        size_t avail_out = vmbuf_wavail(outbuf);
        avail_out = (avail_out > UINT_MAX) ? UINT_MAX : avail_out;
        strm.avail_out = avail_out;
        int ret = inflate(&strm, Z_NO_FLUSH);
        vmbuf_wseek(outbuf, avail_out - strm.avail_out);
        if (ret == Z_STREAM_END)
            break;
        if (Z_OK != ret) {
            return inflateEnd(&strm), -1;
        }
        total_read += avail_in - strm.avail_in;
        strm.next_in = (uint8_t*)inbuf + total_read;
        avail_in = in_size - total_read;
        avail_in = (avail_in > UINT_MAX) ? UINT_MAX : avail_in;
        strm.avail_in = avail_in;
    }
    inflateEnd(&strm);
    return 0;
}
