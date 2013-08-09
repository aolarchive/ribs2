#ifndef _RIBS_ZLIB__H_
#define _RIBS_ZLIB__H_

#include "ribs_defs.h"
#include "vmbuf.h"

int vmbuf_deflate(struct vmbuf *buf);
int vmbuf_deflate2(struct vmbuf *inbuf, struct vmbuf *outbuf);
int vmbuf_inflate(struct vmbuf *buf);
int vmbuf_inflate2(struct vmbuf *inbuf, struct vmbuf *outbuf);

#endif // _RIBS_ZLIB__H_
