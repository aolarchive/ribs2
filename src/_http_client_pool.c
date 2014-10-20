/*
    This file is part of RIBS2.0 (Robust Infrastructure for Backend Systems).
    RIBS is an infrastructure for building great SaaS applications (but not
    limited to).

    Copyright (C) 2012,2013,2014 Adap.tv, Inc.

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

_RIBS_INLINE_ int http_client_send_request(struct http_client_context *cctx) {
    int fd = cctx->fd;
    int res = 0;
#ifdef RIBS2_SSL
    SSL *ssl = ribs_ssl_get(fd);
    if (!ssl)
#endif
        res = vmbuf_write(&cctx->request, fd);
#ifdef RIBS2_SSL
    else {
        /* don't attempt if not ssl_connected, will be picked up by http_client_main_fiber() */
        if (cctx->ssl_connected) {
            res = SSL_write(ssl, vmbuf_rloc(&cctx->request), vmbuf_ravail(&cctx->request));
            if (res > 0)
                vmbuf_rseek(&cctx->request, res);
            else {
                int err = SSL_get_error(ssl, res);
                if (res < 0 && SSL_ERROR_WANT_WRITE == err)
                    res = 0;
                else {
                    LOGGER_ERROR("SSL_write: %s",ERR_reason_error_string(ERR_peek_last_error()));
                    if (0 == res)
                        res = -1;
                }
            }
        }
    }
#endif
    if (res < 0) {
        LOGGER_PERROR("write request");
        cctx->http_status_code = 500;
        cctx->persistent = 0;
        TIMEOUT_HANDLER_REMOVE_FD_DATA(epoll_worker_fd_map + fd);
        ribs_close(fd);
    }
    return res;
}

_RIBS_INLINE_ char *http_client_response_headers(struct http_client_context *cctx) {
    char *resp = vmbuf_data(&cctx->response);
    char *headers = strstr(resp, "\r\n");
    if (headers) {
        headers += 2;
        char *eoh = strstr(headers, "\r\n\r\n");
        if (eoh)
            *(eoh + 2) = 0;
    }
    return headers;
}
