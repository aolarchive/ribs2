/*
    This file is part of RIBS2.0 (Robust Infrastructure for Backend Systems).
    RIBS is an infrastructure for building great SaaS applications (but not
    limited to).

    Copyright (C) 2013,2014 Adap.tv, Inc.

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
#define RINGFILE_HEADER (ringfile_get_header(rb))
_RIBS_INLINE_ void *ringfile_data(struct ringfile *rb) {
    return rb->rbuf;
}

_RIBS_INLINE_ struct ringfile_header *ringfile_get_header(struct ringfile *rb) {
    return rb->mem;
}

_RIBS_INLINE_ void *ringfile_get_reserved(struct ringfile *rb) {
    return RINGFILE_HEADER->reserved;
}

_RIBS_INLINE_ int ringfile_reset(struct ringfile *rb) {
    RINGFILE_HEADER->read_loc = RINGFILE_HEADER->write_loc = 0;
    return 0;
}

_RIBS_INLINE_ void *ringfile_wloc(struct ringfile *rb) {
    return rb->rbuf + RINGFILE_HEADER->write_loc;
}

_RIBS_INLINE_ void *ringfile_rloc(struct ringfile *rb) {
    return rb->rbuf + RINGFILE_HEADER->read_loc;
}

_RIBS_INLINE_ size_t ringfile_rlocpos(struct ringfile *rb) {
    return RINGFILE_HEADER->read_loc;
}

_RIBS_INLINE_ size_t ringfile_wlocpos(struct ringfile *rb) {
    return RINGFILE_HEADER->write_loc;
}

_RIBS_INLINE_ void ringfile_wseek(struct ringfile *rb, size_t by) {
    RINGFILE_HEADER->write_loc += by;
}

_RIBS_INLINE_ void ringfile_rseek(struct ringfile *rb, size_t by) {
    RINGFILE_HEADER->read_loc += by;
    if (unlikely(RINGFILE_HEADER->read_loc >= RINGFILE_HEADER->capacity)) {
        RINGFILE_HEADER->read_loc -= RINGFILE_HEADER->capacity;
        RINGFILE_HEADER->write_loc -= RINGFILE_HEADER->capacity;
    }
}

_RIBS_INLINE_ size_t ringfile_size(struct ringfile *rb) {
    return RINGFILE_HEADER->write_loc - RINGFILE_HEADER->read_loc;
}

_RIBS_INLINE_ size_t ringfile_avail(struct ringfile *rb) {
    return RINGFILE_HEADER->capacity - ringfile_size(rb);
}

_RIBS_INLINE_ size_t ringfile_capacity(struct ringfile *rb) {
    return RINGFILE_HEADER->capacity;
}

_RIBS_INLINE_ int ringfile_empty(struct ringfile *rb) {
    return RINGFILE_HEADER->write_loc == RINGFILE_HEADER->read_loc;
}

_RIBS_INLINE_ void *ringfile_pop(struct ringfile *rb, size_t n) {
    void *ret = rb->rbuf + RINGFILE_HEADER->read_loc;
    ringfile_rseek(rb, n);
    return ret;
}

_RIBS_INLINE_ void *ringfile_push(struct ringfile *rb, size_t n) {
    void *ret = rb->rbuf + RINGFILE_HEADER->write_loc;
    ringfile_wseek(rb, n);
    return ret;
}

_RIBS_INLINE_ void *ringfile_rolling_push(struct ringfile *rb, size_t n) {
    while (ringfile_avail(rb) < n)
        ringfile_rseek(rb, n);
    return ringfile_push(rb, n);
}
