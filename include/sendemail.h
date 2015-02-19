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
#ifndef _SENDMAIL__H_
#define _SENDMAIL__H_

#include "ribs.h"
#include "timeout_handler.h"
#include <netinet/in.h>

/*
  Example of sending email to multiple recipients
  if (0 > sendemail(&(struct email) {
  .from = "from@email",
  .rcpt = {.to = "to1@email", .next = &(struct rcptlist) {
           .to = "to2@email", .next = NULL } },
  .data="Subject: subject\r\nFrom: from\r\n\r\body" } ) )...
*/

struct sendemail_mta {
    char *myhost;
    struct sockaddr_in saddr;
    struct timeout_handler timeout_handler;
};

#define SENDEMAIL_MTA_DEFAULTS .myhost = "localhost", .timeout_handler.timeout = 5000, .saddr = { .sin_addr.s_addr = 0x0100007f, .sin_port=0x1900, .sin_family=AF_INET }
#define SENDEMAIL_MTA_INITIALIZER { SENDEMAIL_MTA_DEFAULTS }

struct rcptlist {
    char *to;
    struct rcptlist *next;
};

struct email {
    char *from;
    struct rcptlist rcpt;
    size_t data_len;  // if 0, we use strlen(data), i.e. assume data contains only text
    char *data;
};

int sendemail_init(struct sendemail_mta *mta);
int sendemail2(struct sendemail_mta *mta, struct email *email, int *code);
int sendemail(struct email *email);
int sendemail_with_code(struct email *email, int *code);

#endif /* _SENDMAIL__H_ */
