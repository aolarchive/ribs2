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
#include "http_file_server.h"
#include "http_defs.h"
#include "mime_types.h"
#include "file_mapper.h"
#include <limits.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <libgen.h>

#ifdef HAVE_ZLIB
#include <zlib.h>
#endif

SSTRL(RIBS_GZ_EXT, "._ribs_gz_");

const char *_peer_addr_str(int fd, char *buf) {
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    if (0 == getpeername(fd, (struct sockaddr*)&addr, &len) &&
        NULL != inet_ntop(AF_INET, &addr.sin_addr, buf, INET_ADDRSTRLEN))
        return buf;
    return "0.0.0.0";
}

int http_file_server_list_dir(struct http_file_server *fs, const char *realname) {
    struct http_server_context *ctx = http_server_get_context();
    struct vmbuf *payload = &ctx->payload;
    const char *dir = realname + fs->base_dir_len;
    if (!*dir) dir = "/";
    vmbuf_sprintf(payload, "<html><head><title>Index of %s</title></head>", dir);
    vmbuf_strcpy(payload, "<body>");
    vmbuf_sprintf(payload, "<h1>Index of %s</h1><hr>", dir);
    if (dir[1])
        vmbuf_sprintf(payload, "<a href=\"..\">../</a><br><br>");
    vmbuf_sprintf(payload, "<table width=\"100%%\" border=\"0\">");
    DIR *d = opendir(realname);
    int error = 0;
    if (d) {
        struct dirent de, *dep;
        while (0 == readdir_r(d, &de, &dep) && dep) {
            if (de.d_name[0] == '.')
                continue;
            struct stat st;
            if (0 > fstatat(dirfd(d), de.d_name, &st, 0)) {
                vmbuf_sprintf(payload, "<tr><td>ERROR: %s</td><td>N/A</td></tr>", de.d_name);
                continue;
            }
            const char *slash = (S_ISDIR(st.st_mode) ? "/" : "");
            struct tm t_res, *t;
            t = localtime_r(&st.st_mtime, &t_res);
            vmbuf_strcpy(payload, "<tr>");
            vmbuf_sprintf(payload, "<td><a href=\"%s%s\">%s%s</a></td>", de.d_name, slash, de.d_name, slash);
            vmbuf_strcpy(payload, "<td>");
            if (t)
                vmbuf_strftime(payload, "%F %T", t);
            vmbuf_strcpy(payload, "</td>");
            vmbuf_sprintf(payload, "<td>%jd</td>", (intmax_t)st.st_size);
            vmbuf_strcpy(payload, "</tr>");
        }
        closedir(d);
    }
    vmbuf_strcpy(payload, "<tr><td colspan=3><hr></td></tr></table>");
    vmbuf_sprintf(payload, "<address>RIBS 2.0 Port %hu</address></body>", ctx->server->port);
    vmbuf_strcpy(payload, "</html>");
    return http_server_response(HTTP_STATUS_200, HTTP_CONTENT_TYPE_TEXT_HTML), error;
}

int http_file_server_init(struct http_file_server *fs) {
    char realname[4096];
    if (NULL == realpath(fs->base_dir ? fs->base_dir : ".", realname))
        return -1;
    fs->base_dir = strdup(realname);
    if ((0 > hashtable_init(&fs->ht_ext_whitelist, 0)) ||
        (0 > hashtable_init(&fs->ht_ext_max_age, 0)))
        return -1;

    fs->base_dir_len = strlen(fs->base_dir);
    return 0;
}

static inline int32_t find_max_age(struct http_file_server *fs, const char *ext) {
    int32_t rv = fs->max_age;

    if(0 < hashtable_get_size(&fs->ht_ext_max_age)) {
        uint32_t ofs = hashtable_lookup(&fs->ht_ext_max_age, ext, strlen(ext));
        if(ofs)
            memcpy(&rv, hashtable_get_val(&fs->ht_ext_max_age, ofs), sizeof(int32_t));
    }
    return rv;
}

#define HTTP_FILE_SERVER_ERROR(code) \
    http_server_response_sprintf(HTTP_STATUS_##code, HTTP_CONTENT_TYPE_TEXT_PLAIN, "%s\n", HTTP_STATUS_##code)

int http_file_server_run(struct http_file_server *fs) {
    struct http_server_context *ctx = http_server_get_context();
    struct http_headers headers;
    http_headers_parse(ctx->headers, &headers);
    if (0 == *ctx->uri)
        return HTTP_FILE_SERVER_ERROR(403), -1;
    http_server_decode_uri(ctx->uri);
    return http_file_server_run2(fs, &headers, ctx->uri + 1);
}

int http_file_server_run2(struct http_file_server *fs, struct http_headers *headers, const char *file) {
    struct http_server_context *ctx = http_server_get_context();
    const char *ext = strrchr(file, '.');
    if (ext)
        ++ext;
    else
        ext = "";
    static struct vmbuf tmp = VMBUF_INITIALIZER;
    vmbuf_init(&tmp, 4096);
    vmbuf_sprintf(&tmp, "%s/%s", fs->base_dir, file);

    char realname[PATH_MAX];
    char addr_str[INET_ADDRSTRLEN];
    if (NULL == realpath(vmbuf_data(&tmp), realname)) {
        LOGGER_PERROR("[%s / %s] realpath (404): [%s]", _peer_addr_str(ctx->fd, addr_str), headers->x_forwarded_for, vmbuf_data(&tmp));
        return HTTP_FILE_SERVER_ERROR(404), -1;
    }
    if (0 != strncmp(realname, fs->base_dir, fs->base_dir_len)) {
        LOGGER_ERROR("[%s / %s] rejecting (403): [%s]",  _peer_addr_str(ctx->fd, addr_str), headers->x_forwarded_for, realname);
        return HTTP_FILE_SERVER_ERROR(403), -1;
    }
    int ffd;
    int compressed;
    struct stat st, orig_st;
#ifdef HAVE_ZLIB
    if (0 != (headers->accept_encoding_mask & HTTP_AE_GZIP) && hashtable_lookup(&fs->ht_ext_whitelist, ext, strlen(ext))) {
        if (0 > stat(realname, &orig_st))
            return HTTP_FILE_SERVER_ERROR(404), -1;
        if (S_ISDIR(orig_st.st_mode)) {
            if (fs->allow_list)
                return http_file_server_list_dir(fs, realname);
            return HTTP_FILE_SERVER_ERROR(403), -1;
        }
        char realname_compressed[PATH_MAX];
        char *p = strrchr(realname, '/');
        if (p) {
            if (PATH_MAX <= snprintf(realname_compressed, PATH_MAX, "%.*s/%s.%s", (int)(p - realname), realname, RIBS_GZ_EXT, p + 1))
                return HTTP_FILE_SERVER_ERROR(403), -1;
        } else {
            if (PATH_MAX <= snprintf(realname_compressed, PATH_MAX, "%s.%s", RIBS_GZ_EXT, realname))
                return HTTP_FILE_SERVER_ERROR(403), -1;
        }
        ffd = open(realname_compressed, O_RDONLY);
        for (;;) {
            if (unlikely(0 > ffd)) {
                gzFile file = gzopen(realname_compressed, "wb");
                if (NULL == file)
                    return HTTP_FILE_SERVER_ERROR(500), -1;
                LOGGER_INFO("compressing [%s] to [%s]", realname, realname_compressed);
                struct file_mapper fm = FILE_MAPPER_INITIALIZER;
                if (0 > file_mapper_init(&fm, realname))
                    return HTTP_FILE_SERVER_ERROR(404), gzclose(file), -1;
                ssize_t file_size = file_mapper_size(&fm);
                ssize_t size = gzwrite(file, file_mapper_data(&fm), file_size);
                if (Z_OK != gzclose(file))
                    size = -1;
                file_mapper_free(&fm);
                if (size != file_size)
                    return HTTP_FILE_SERVER_ERROR(500), -1;
                ffd = open(realname_compressed, O_RDONLY);
                if (0 > ffd)
                    return HTTP_FILE_SERVER_ERROR(404), -1;
                if (0 > fstat(ffd, &st))
                    return HTTP_FILE_SERVER_ERROR(500), close(ffd), -1;
            } else {
                if (0 > fstat(ffd, &st))
                    return HTTP_FILE_SERVER_ERROR(500), close(ffd), -1;
                if (st.st_mtime < orig_st.st_mtime) {
                    close(ffd);
                    ffd = -1;
                    continue;
                }
            }
            break;
        }
        compressed = 1;
    } else
#endif
    {
        compressed = 0;
        ffd = open(realname, O_RDONLY);
        if (0 > ffd)
            return HTTP_FILE_SERVER_ERROR(404), -1;
        if (0 > fstat(ffd, &st))
            return HTTP_FILE_SERVER_ERROR(500), close(ffd), -1;
        orig_st = st;
    }

    if (S_ISDIR(st.st_mode)) {
        close(ffd);
        if (fs->allow_list)
            return http_file_server_list_dir(fs, realname);
        return http_server_response_sprintf(HTTP_STATUS_403, HTTP_CONTENT_TYPE_TEXT_PLAIN, "%s\n", HTTP_STATUS_403), -1;
    }

    struct tm tm;
    gmtime_r(&orig_st.st_mtime, &tm);
    char etag[128];
    intmax_t size = st.st_size;
    int32_t max_age = find_max_age(fs, ext);

    int n = strftime(etag, sizeof(etag), "\r\nETag: \"%d%m%Y%H%M%S", &tm);
    if (0 == n || (int)sizeof(etag) - n <= snprintf(etag + n, sizeof(etag) - n, "%jd\"", size))
        return HTTP_FILE_SERVER_ERROR(500), close(ffd), -1;
    int include_payload;
    if (0 == strcmp(headers->if_none_match, etag + 8))
        http_server_header_start_no_body(HTTP_STATUS_304), include_payload = 0;
    else
        http_server_header_start(HTTP_STATUS_200, mime_types_by_ext(ext)), include_payload = 1;
    vmbuf_strcpy(&ctx->header, etag);
    if (compressed && include_payload)
        vmbuf_strcpy(&ctx->header, "\r\nContent-Encoding: gzip");
    vmbuf_sprintf(&ctx->header, "\r\nCache-Control: max-age=%d", max_age);
    time_t t = time(NULL);
    gmtime_r(&t, &tm);
    vmbuf_strftime(&ctx->header, "\r\nDate: %a, %d %b %Y %H:%M:%S GMT", &tm);
    t += max_age;
    gmtime_r(&t, &tm);
    vmbuf_strftime(&ctx->header, "\r\nExpires: %a, %d %b %Y %H:%M:%S GMT", &tm);
    gmtime_r(&orig_st.st_mtime, &tm);
    vmbuf_strftime(&ctx->header, "\r\nLast-Modified: %a, %d %b %Y %H:%M:%S GMT", &tm);
    if (include_payload)
        vmbuf_sprintf(&ctx->header, "\r\nContent-Length: %jd", size);
    vmbuf_strcpy(&ctx->header, "\r\n\r\n");
    int res = 0;
    if (include_payload && 0 > (res = http_server_sendfile_payload(ffd, st.st_size)))
        LOGGER_PERROR("%s", realname);
    close(ffd);
    return res;
}
