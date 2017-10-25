/*
    This file is part of RIBS2.0 (Robust Infrastructure for Backend Systems).
    RIBS is an infrastructure for building great SaaS applications (but not
    limited to).

    Copyright (C) 2014 Adap.tv, Inc.

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
#include "ribs_ssl.h"
#include <sys/time.h>
#include <sys/resource.h>
#include "logger.h"

#define _HTTP_SERVER_SSL_CIPHERS "ECDH+AESGCM:DH+AESGCM:ECDH+AES256:DH+AES256:ECDH+AES128:DH+AES:ECDH+3DES:DH+3DES:RSA+AESGCM:RSA+AES:RSA+3DES:!aNULL:!MD5:!DSS"

static SSL **ssl_map = NULL;

void ribs_ssl_init(void) {
    if (NULL != ssl_map)
        return;
    struct rlimit rlim;
    if (0 > getrlimit(RLIMIT_NOFILE, &rlim)) {
        LOGGER_PERROR("getrlimit(RLIMIT_NOFILE)");
        return;
    }
    ssl_map = calloc(rlim.rlim_cur, sizeof(SSL *));
}

SSL *ribs_ssl_get(int fd) {
    return ssl_map[fd];
}

SSL *ribs_ssl_alloc(int fd, SSL_CTX *ssl_ctx) {
    SSL *ssl = SSL_new(ssl_ctx);
    ssl_map[fd] = ssl;
    if (NULL != ssl)
        SSL_set_fd(ssl, fd);
    return ssl;
}

void ribs_ssl_free(int fd) {
    SSL *ssl = ssl_map[fd];
    ssl_map[fd] = NULL;
    if (NULL == ssl) return;
    SSL_free(ssl);
}

int ribs_ssl_set_options(SSL_CTX *ssl_ctx, char *cipher_list) {
    /* bugs */
    SSL_CTX_set_options(ssl_ctx, SSL_OP_ALL); /* almost all bugs */
    SSL_CTX_set_options(ssl_ctx, SSL_OP_NO_SSLv2); /* disable SSLv2 per RFC 6176 */
    SSL_CTX_set_options(ssl_ctx, SSL_OP_NO_SSLv3); /* disable SSLv3. goodbye IE6 */
    SSL_CTX_set_options(ssl_ctx, SSL_OP_NO_COMPRESSION);
    SSL_CTX_set_options(ssl_ctx, SSL_OP_NO_SESSION_RESUMPTION_ON_RENEGOTIATION);

    /* ciphers */
    if (!cipher_list)
        cipher_list = _HTTP_SERVER_SSL_CIPHERS;
    SSL_CTX_set_options(ssl_ctx, SSL_OP_CIPHER_SERVER_PREFERENCE);
    if (0 == SSL_CTX_set_cipher_list(ssl_ctx, cipher_list))
        return LOGGER_ERROR("failed to initialize SSL:cipher_list"), -1;

    /* DH 2048 bits */
    SSL_CTX_set_options(ssl_ctx, SSL_OP_SINGLE_DH_USE);
    DH *dh = DH_new();
    dh->p = get_rfc3526_prime_2048(NULL);
    BN_dec2bn(&dh->g, "2");
    if (0 == SSL_CTX_set_tmp_dh(ssl_ctx, dh))
        return LOGGER_ERROR("failed to initialize SSL:dh"), -1;
    DH_free(dh);

    /* Ecliptic Curve DH */
    SSL_CTX_set_options(ssl_ctx, SSL_OP_SINGLE_ECDH_USE);
    EC_KEY *ecdh = EC_KEY_new_by_curve_name(OBJ_sn2nid("prime256v1"));
    if (ecdh == NULL)
        return LOGGER_ERROR("failed to initialize SSL:edch"), -1;
    SSL_CTX_set_tmp_ecdh(ssl_ctx, ecdh);
    EC_KEY_free(ecdh);

    return 0;
}
