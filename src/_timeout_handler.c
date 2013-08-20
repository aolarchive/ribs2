/*
    This file is part of RIBS2.0 (Robust Infrastructure for Backend Systems).
    RIBS is an infrastructure for building great SaaS applications (but not
    limited to).

    Copyright (C) 2012,2013 Adap.tv, Inc.

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
_RIBS_INLINE_ void timeout_handler_add_fd_data(struct timeout_handler *timeout_handler, struct epoll_worker_fd_data *fd_data) {
    gettimeofday(&fd_data->timestamp, NULL);
    if (list_empty(&timeout_handler->timeout_chain)) {
        struct itimerspec when = {{0,0},{timeout_handler->timeout/1000,(timeout_handler->timeout%1000)*1000000L}};
        if (0 > timerfd_settime(timeout_handler->fd, 0, &when, NULL))
            LOGGER_PERROR("timerfd_settime"), abort();
    }
    list_insert_tail(&timeout_handler->timeout_chain, &fd_data->timeout_chain);
}
