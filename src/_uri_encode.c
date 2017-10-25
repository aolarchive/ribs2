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
/*
 * inline
 */
_RIBS_INLINE_ int http_uri_encode(const char *in, struct vmbuf *out) {

    static uint8_t bits[] = {
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x9B, 0x00, 0xFC,
            0x01, 0x00, 0x00, 0x78, 0x01, 0x00, 0x00, 0xF8,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
    };
    static char hex_chars[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
            'A', 'B', 'C', 'D', 'E', 'F' };
    const uint8_t *inbuf = (const uint8_t *)in;

    vmbuf_resize_if_less(out, vmbuf_wlocpos(out)+(strlen(in)*3)+1);
    uint8_t *outbuf = (uint8_t *)vmbuf_wloc(out);

    if (outbuf == NULL) {
        return -1;
    }

    for ( ; *inbuf; ++inbuf)
    {
        register uint8_t c = *inbuf;
        if (c == ' ')
        {
            *outbuf++ = '+';
        } else if ((bits[c >> 3] & (1 << (c & 7))) > 0)
        {
            *outbuf++ = '%';
            *outbuf++ = hex_chars[c >> 4];
            *outbuf++ = hex_chars[c & 15];
        }
        else
            *outbuf++ = c;

    }

    vmbuf_wseek(out, outbuf-(uint8_t *)vmbuf_wloc(out));
    return 0;

}
