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
_RIBS_INLINE_ void *ringbuf_wloc(struct ringbuf *rb) {
    return rb->mem + rb->write_loc;
}

_RIBS_INLINE_ void *ringbuf_rloc(struct ringbuf *rb) {
    return rb->mem + rb->read_loc;
}

_RIBS_INLINE_ void ringbuf_wseek(struct ringbuf *rb, size_t by) {
    rb->write_loc += by;
}

_RIBS_INLINE_ void ringbuf_rseek(struct ringbuf *rb, size_t by) {
    rb->read_loc += by;
    if (unlikely(rb->read_loc >= rb->capacity)) {
        rb->read_loc -= rb->capacity;
        rb->write_loc -= rb->capacity;
    }
}

_RIBS_INLINE_ size_t ringbuf_size(struct ringbuf *rb) {
    return rb->write_loc - rb->read_loc;
}

_RIBS_INLINE_ size_t ringbuf_avail(struct ringbuf *rb) {
    return rb->capacity - ringbuf_size(rb);
}

_RIBS_INLINE_ int ringbuf_empty(struct ringbuf *rb) {
    return rb->write_loc == rb->read_loc;
}

_RIBS_INLINE_ void *ringbuf_pop(struct ringbuf *rb, size_t n) {
    void *ret = rb->mem + rb->read_loc;
    ringbuf_rseek(rb, n);
    return ret;
}

_RIBS_INLINE_ void *ringbuf_push(struct ringbuf *rb, size_t n) {
    void *ret = rb->mem + rb->write_loc;
    ringbuf_wseek(rb, n);
    return ret;
}

_RIBS_INLINE_ void *ringbuf_rolling_push(struct ringbuf *rb, size_t n) {
    while (ringbuf_avail(rb) < n)
        ringbuf_rseek(rb, n);
    return ringbuf_push(rb, n);
}
