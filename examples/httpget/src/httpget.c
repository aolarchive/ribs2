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
#include "ribs.h"
#include "logger.h"
#include <getopt.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>

void usage(char *progname) {
    dprintf(STDERR_FILENO,
            "Usage: %s [options]... <URL>\n"
            "\t-o, --outfile <FILE>\tOutput to file. Ignores -n,-h,-H,-s\n"
            "\t-c, --cacert <FILE>\tRead Certificate Authorities certificates file, turns on certificate validation\n"
            "\t-h, --headers\t\tPrint headers too\n"
            "\t-H, -hh, --headers-only\tPrint headers only\n"
            "\t-s, --silent\t\tSilent mode. Print nothing\n"
            "\t-n, --nreqs\t\tNumber of request\n", progname);
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {

    static struct option long_options[] = {
        {"outfile", 1, 0, 'o'},
        {"cacert", 1, 0, 'c'},
        {"headers", 0, 0, 'h'},
        {"nreqs", 1, 0, 'n'},
        {"silent", 0, 0, 's'},
        {"headers-only", 0, 0, 'H'},
        {0, 0, 0, 0}
    };

    int silent = 0;
    int64_t nreqs = 0;
    int headers = 0;
    char *cacert = NULL;
    char *file = NULL;

    for (;;) {
        int option_index = 0;
        int c = getopt_long(argc, argv, "o:c:hn:sH", long_options, &option_index);
        if (c == -1)
            break;
        switch (c) {
        case 'o':
            file = optarg;
            break;
        case 'c':
            cacert = optarg;
            break;
        case 'h':
            ++headers;
            break;
        case 'n':
            nreqs = atoi(optarg);
            break;
        case 's':
            silent = 1;
            break;
        case 'H':
            headers += 2;
            break;
        default:
            LOGGER_ERROR("Wrong argument:-%c", c);
            usage(argv[0]);
        }
    }

    if (optind == argc) {
        LOGGER_ERROR("Missing URL");
        usage(argv[0]);
    }
    if (optind+1 != argc) {
        LOGGER_ERROR("Too many parameters");
        usage(argv[0]);
    }

    if (nreqs < 1) nreqs = 1;

    if (file) {
        if (nreqs > 1)
            LOGGER_INFO("File mode, nreqs will be ignored");
        if (silent)
            LOGGER_INFO("File mode, silent has no meaning");
        if (headers)
            LOGGER_INFO("Headers are not supported in file mode");
    }

    char *url = argv[optind];

    /* initialize the event loop */
    if (0 > epoll_worker_init())
        exit(EXIT_FAILURE);

    struct http_client_pool client_pool = {
        .timeout_handler.timeout = 1000*10, /* 10 secs */
        .timeout_handler_persistent.timeout = 1000*60*5 /* 5 minutes */
    };

    uint16_t port = 80;

    if (!strncasecmp("http://", url, 7)) {
        url += 7;
    } else if (!strncasecmp("https://", url, 8)) {
        if (http_client_pool_init_ssl(&client_pool, 1, 1, cacert))
            exit(EXIT_FAILURE);
        url += 8;
        port = 443;
    }

    if (80 == port)
        if (http_client_pool_init(&client_pool, 1, 1))
            exit(EXIT_FAILURE);

    char *hostandport = strtok(url, "/");
    if (!hostandport) {
        LOGGER_ERROR("No host?");
        usage(argv[0]);
    }

    char *uri = strtok(NULL, "");
    if (!uri) uri = "";

    char *host = strtok(hostandport, ":");

    char *sport = strtok(NULL, "");
    if (sport)
        port = atoi(sport);

    struct hostent *h = gethostbyname(host);
    if (NULL == h || NULL == (struct in_addr *)h->h_addr_list) {
        LOGGER_ERROR("Could not resolve: %s", host);
        exit(EXIT_FAILURE);
    }
    struct in_addr addr;
    addr = *(struct in_addr *)h->h_addr_list[0];

    if (file) {
        struct vmfile infile = VMFILE_INITIALIZER;
        if (0 != vmfile_init(&infile, file, 4096)) {
            LOGGER_PERROR("Error opening file for write");
            exit(EXIT_FAILURE);
        }
        int file_compressed = 0;
        int res;
        if (200 != (res = http_client_get_file(&client_pool, &infile, addr, port, host, 0, &file_compressed, "/%s", uri))) {
            LOGGER_PERROR("Error sending request: %d", res);
            exit(EXIT_FAILURE);
        }
        vmfile_close(&infile);

    } else {
        struct http_client_context *rcctx;
    LOOP:
        rcctx = http_client_pool_create_client2(&client_pool, addr, port, host, NULL);
        if (NULL == rcctx) {
            LOGGER_PERROR("Error sending request");
            exit(EXIT_FAILURE);
        }
        vmbuf_sprintf(&rcctx->request, "GET /%s HTTP/1.1\r\nHost: %s\r\n\r\n", uri, host);
        int persistent = 0;
        for (; nreqs > 0; --nreqs) {
            if (0 > http_client_send_request(rcctx)) {
                if (!persistent) --nreqs;
                http_client_free(rcctx);
                goto LOOP;
            }
            /* Try to read right away, without epoll_wait */
            ribs_swapcurcontext(RIBS_RESERVED_TO_CONTEXT(rcctx));
            //yield();
            /* rcctx = http_client_get_last_context(); */
            if (!silent) {
                if (headers > 1)
                    vmbuf_wlocset(&rcctx->response, vmbuf_rlocpos(&rcctx->response)-4);
                if (headers)
                    vmbuf_rlocset(&rcctx->response, 0);
                vmbuf_write(&rcctx->response, STDOUT_FILENO);
                printf("\n");
            }
            /* Not persistent? */
            if (!rcctx->persistent) {
                --nreqs;
                http_client_free(rcctx);
                goto LOOP;
            }
            persistent = 1;
            /* clear the response buffer */
            vmbuf_reset(&rcctx->response);
            /* rollback and reuse the request */
            vmbuf_rreset(&rcctx->request);
            /* reset the fiber */
            http_client_reuse_context(rcctx);
         }
    }
    return 0;
}
