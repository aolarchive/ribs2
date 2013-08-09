#include "ribs_zlib.h"
#include <zlib.h>

int vmbuf_deflate(struct vmbuf *buf) {
    static struct vmbuf outbuf = VMBUF_INITIALIZER;
    vmbuf_init(&outbuf, vmbuf_ravail(buf));
    if (0 > vmbuf_deflate2(buf, &outbuf))
        return -1;
    vmbuf_swap(&outbuf, buf);
    return 0;
}

int vmbuf_deflate2(struct vmbuf *inbuf, struct vmbuf *outbuf) {
    z_stream strm;
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    if (Z_OK != deflateInit2(&strm, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 15+16, 8, Z_DEFAULT_STRATEGY))
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
