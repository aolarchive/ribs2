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

#define _HEAP_DATA(i) ((struct _heap_entry *)(data + (el_size * i)))
struct _heap_entry {
    uint32_t key;
    char user_data[];
};

_RIBS_INLINE_ void *heap_top(struct heap *h) {
    void *data = vmbuf_data(&h->data);
    uint32_t *ofs = (uint32_t *)vmbuf_data(&h->ofs);
    uint32_t el_size = h->el_size;
    return _HEAP_DATA(ofs[0])->user_data;
}

_RIBS_INLINE_ void _heap_fix_down(struct heap *h, uint32_t i) {
    void *data = vmbuf_data(&h->data);
    uint32_t *ofs = (uint32_t *)vmbuf_data(&h->ofs);
    uint32_t child;
    uint32_t num_items = h->num_items;
    uint32_t el_size = h->el_size;

    int (*compar)(void *a, void *b) = h->compar;

    while ((child = (i << 1) + 1) < num_items) {
        if (child + 1 < num_items && 0 > compar(_HEAP_DATA(ofs[child])->user_data, _HEAP_DATA(ofs[child+1])->user_data)) ++child;
        uint32_t parent_ofs = ofs[i];
        uint32_t child_ofs = ofs[child];
        if (0 > compar(_HEAP_DATA(parent_ofs)->user_data, _HEAP_DATA(child_ofs)->user_data)) {
            _HEAP_DATA(child_ofs)->key = parent_ofs;
            _HEAP_DATA(parent_ofs)->key = child_ofs;
            ofs[child] = parent_ofs;
            ofs[i] = child_ofs;
            i = child;
        } else
            break;
    }
}

_RIBS_INLINE_ void heap_remove_item(struct heap *h, uint32_t index) {
    if (index < h->num_items) {
        --h->num_items;
        void *data = vmbuf_data(&h->data);
        uint32_t el_size = h->el_size;
        uint32_t *ofs = (uint32_t *)vmbuf_data(&h->ofs);
        uint32_t o = ofs[index];
        _HEAP_DATA(o)->key = -1;
        ofs[index] = ofs[h->num_items];
        _HEAP_DATA(ofs[index])->key = index;
        ofs[h->num_items] = o;
        _heap_fix_down(h, index);
    }
}

_RIBS_INLINE_ void heap_remove(struct heap *h, uint32_t loc) {
    void *data = vmbuf_data(&h->data);
    uint32_t el_size = h->el_size;
    return heap_remove_item(h, _HEAP_DATA(loc)->key);
}

_RIBS_INLINE_ int heap_full(struct heap *h) {
    return h->num_items == h->max_size;
}

_RIBS_INLINE_ int heap_empty(struct heap *h) {
    return 0 == h->num_items;
}

_RIBS_INLINE_ void heap_build(struct heap *h) {
    uint32_t last_parent = h->num_items >> 1, i;
    for (i = last_parent; i != -1U; --i)
        _heap_fix_down(h, i);
}

_RIBS_INLINE_ void heap_remove_top(struct heap *h) {
    if (0 == h->num_items)
        return;
    --h->num_items;
    void *data = vmbuf_data(&h->data);
    uint32_t *ofs = (uint32_t *)vmbuf_data(&h->ofs);
    uint32_t first = ofs[0];
    uint32_t last = ofs[h->num_items];
    uint32_t el_size = h->el_size;
    --_HEAP_DATA(first)->key; // will set it to -1
    ofs[0] = last;
    ofs[h->num_items] = first;
    _HEAP_DATA(last)->key = 0;
    _heap_fix_down(h, 0);
}
