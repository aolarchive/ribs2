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
/*
 * inline
 */
_RIBS_INLINE_ struct ribs_context *http_client_get_ribs_context(struct http_client_context *cctx) {
    return RIBS_RESERVED_TO_CONTEXT(cctx);
}

_RIBS_INLINE_ struct http_client_context *http_client_get_last_context(void) {
    return (struct http_client_context *)epoll_worker_get_last_context()->reserved;
}

_RIBS_INLINE_ int http_client_send_request(struct http_client_context *cctx) {
    int fd = cctx->fd;
    int res = vmbuf_write(&cctx->request, fd);
    if (res < 0) {
        LOGGER_PERROR("write request");
        cctx->http_status_code = 500;
        cctx->persistent = 0;
        close(fd);
        TIMEOUT_HANDLER_REMOVE_FD_DATA(epoll_worker_fd_map + fd);
    }
    return res;
}
