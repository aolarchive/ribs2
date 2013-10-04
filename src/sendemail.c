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
#include "sendemail.h"
#include "logger.h"
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <sys/uio.h>

SSTRL(CRLFCRLF, "\r\n\r\n");
SSTRL(CRLF, "\r\n");

static struct sendemail_mta _global_mta = SENDEMAIL_MTA_INITIALIZER;
static struct sendemail_mta *global_mta = NULL;

int sendemail_init(struct sendemail_mta *mta) {
    mta->saddr.sin_family = AF_INET;
    return timeout_handler_init(&mta->timeout_handler);
}

static void sendemail_yield(struct timeout_handler *timeout_handler, int cfd) {
    struct epoll_worker_fd_data *fd_data = epoll_worker_fd_map + cfd;
    timeout_handler_add_fd_data(timeout_handler, fd_data);
    yield();
    TIMEOUT_HANDLER_REMOVE_FD_DATA(fd_data);
}

_RIBS_INLINE_ char *strrstr(char *str1, const char *str2) {
    char *p = str1, *r = p;

    while (p) {
        p = strstr(p, str2);
        if (p) {
            r = p;
            p += strlen(str2);
        }
    }
    return r;
}

_RIBS_INLINE_ int read_data(struct timeout_handler *timeout_handler, int cfd, struct vmbuf *response, size_t ofs) {
    int res = vmbuf_read(response, cfd);
    if (0 > res) {
        ribs_close(cfd);
        return -1;
    }
    *vmbuf_wloc(response) = 0;
    char *data, *line;
    /* Keep reading if
     * 1.you don't see a CRLF
     * OR
     * 2.see a dash and CRLF in the current line
     */
    while ((NULL == strstr((data = vmbuf_data_ofs(response, ofs)), CRLF))
           || ((NULL != strchr((line = strrstr(data, CRLF)), '-')) && (NULL != strstr(line, CRLF)))) {
        if (1 == res)
            sendemail_yield(timeout_handler, cfd);
        if (0 >= (res = vmbuf_read(response, cfd))) {
            LOGGER_PERROR("read");
            ribs_close(cfd);
            return -1;
        }
    }
    *vmbuf_wloc(response) = 0;
    return 0;
}

_RIBS_INLINE_ int write_data(int cfd, struct vmbuf *request) {
    int sres = vmbuf_write(request, cfd);
    if (0 > sres) {
        LOGGER_PERROR("write request");
        ribs_close(cfd);
        return -1;
    }
    return 0;
}

#define SEND_DATA                                                       \
    if (0 > write_data(cfd, &request))                                  \
        return LOGGER_ERROR("write_data"), -1

#define CHECK_CODE(c)                                                   \
    code = parse_response(&response, ofs);                              \
    if (c != code)                                                      \
        return LOGGER_ERROR("Returned %d. Expected: %d", code, c), -1

#define READ_AND_CHECK(c)                                               \
    if (0 > read_data(&mta->timeout_handler, cfd, &response, ofs))      \
        return LOGGER_ERROR("read_data"), -1;                           \
    CHECK_CODE(c);                                                      \
    ofs = vmbuf_wlocpos(&response)

static int parse_response(struct vmbuf *response, size_t ofs) {
    char *data = vmbuf_data_ofs(response, ofs);
    char *ds = data + 3;
    switch (*ds) {
    case '-': /* multi line */
    case ' ': /* single line */
        {
            *ds = 0;
            int code = atoi(data);
            *ds = ' ';
            return code;
        }
    default:
        return -1;
    }
}

int sendemail2(struct sendemail_mta *mta, struct email *email) {
    if (NULL == mta) {
        mta = global_mta;
    }

    int cfd = socket(PF_INET, SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_TCP);
    if (0 > cfd)
        return LOGGER_PERROR("sendemail: socket"), -1;

    const int option = 1;
    if (0 > setsockopt(cfd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option)))
        return LOGGER_PERROR("sendemail: setsockopt SO_REUSEADDR"), close(cfd), -1;

    if (0 > setsockopt(cfd, IPPROTO_TCP, TCP_NODELAY, &option, sizeof(option)))
        return LOGGER_PERROR("sendemail: setsockopt TCP_NODELAY"), close(cfd), -1;

    if (0 > connect(cfd, (struct sockaddr *)&mta->saddr, sizeof(mta->saddr)) && EINPROGRESS != errno)
        return LOGGER_PERROR("sendemail: connect"), close(cfd), -1;

    if (0 > ribs_epoll_add(cfd, EPOLLIN | EPOLLOUT | EPOLLET, current_ctx))
        return close(cfd), -1;

    struct vmbuf response = VMBUF_INITIALIZER;
    struct vmbuf request  = VMBUF_INITIALIZER;
    vmbuf_init(&response, 4096);
    vmbuf_init(&request, 4096);

    int code = -1;
    size_t ofs = 0;

    READ_AND_CHECK(220);

    vmbuf_sprintf(&request, "EHLO %s\r\n", mta->myhost);
    SEND_DATA;

    READ_AND_CHECK(250);

    vmbuf_sprintf(&request, "MAIL FROM:<%s>\r\n", email->from);
    SEND_DATA;

    READ_AND_CHECK(250);

    struct rcptlist *rcpt = &email->rcpt;
    while (rcpt) {
        vmbuf_sprintf(&request, "RCPT TO:<%s>\r\n", rcpt->to);
        SEND_DATA;

        READ_AND_CHECK(250);

        rcpt = rcpt->next;
    }

    vmbuf_strcpy(&request, "DATA\r\n");
    SEND_DATA;

    READ_AND_CHECK(354);

    struct iovec iov[2]= {
        [0] = {
            .iov_base = email->data,
            .iov_len = strlen(email->data)
        },
        [1] = {
            .iov_base = "\r\n.\r\n",
            .iov_len = 5
        }
    };

    ssize_t num_write;
    for (;;sendemail_yield(&mta->timeout_handler, cfd)) {
        num_write = writev(cfd, iov, iov[1].iov_len ? 2 : 1);
        if (0 > num_write) {
            if (EAGAIN == errno) {
                continue;
            } else {
                LOGGER_ERROR("writev");
                ribs_close(cfd);
                return -1;
            }
        } else {
            if (num_write >= (ssize_t)iov[0].iov_len) {
                num_write -= iov[0].iov_len;
                iov[0].iov_len = iov[1].iov_len - num_write;
                if (iov[0].iov_len == 0)
                    break;
                iov[0].iov_base = iov[1].iov_base + num_write;
                iov[1].iov_len = 0;
            } else {
                iov[0].iov_len -= num_write;
                iov[0].iov_base += num_write;
            }
        }
    }

    READ_AND_CHECK(250);

    vmbuf_strcpy(&request, "QUIT\r\n");
    SEND_DATA;

    *vmbuf_data(&response) = 0;
    int res = 0;

    while (NULL == strstr(vmbuf_data_ofs(&response, ofs), CRLF)) {
        if (1 == res)
            sendemail_yield(&mta->timeout_handler, cfd);
        if (0 >= (res = vmbuf_read(&response, cfd)))
            break;
    }
    *vmbuf_data(&response) = 0;
    CHECK_CODE(221);
    ribs_close(cfd);
    return 0;
}

int sendemail(struct email *email) {
    if (NULL == global_mta) {
        global_mta = &_global_mta;
        sendemail_init(global_mta);
    }
    return sendemail2(global_mta, email);
}
