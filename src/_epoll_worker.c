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
_RIBS_INLINE_ void epoll_worker_ignore_events(int fd) {
    epoll_worker_fd_map[fd].ctx = event_loop_ctx;
}

_RIBS_INLINE_ void epoll_worker_resume_events(int fd) {
    epoll_worker_fd_map[fd].ctx = current_ctx;
}

_RIBS_INLINE_ struct ribs_context *epoll_worker_get_last_context(void) {
    return epoll_worker_fd_map[last_epollev.data.fd].ctx;
}

_RIBS_INLINE_ void epoll_worker_set_fd_ctx(int fd, struct ribs_context* ctx) {
    epoll_worker_fd_map[fd].ctx = ctx;
}

_RIBS_INLINE_ void epoll_worker_set_last_fd(int fd) {
    last_epollev.data.fd = fd;
}
