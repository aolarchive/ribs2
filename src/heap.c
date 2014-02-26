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
#include "heap.h"


static int _heap_int_compar(void *a, void *b) {
    int *aa = a, *bb = b;
    return (*aa < *bb ? -1 : (*bb < *aa ? 1 : 0));
}

int heap_init(struct heap *h, size_t initial_size, size_t el_size, int (*compar)(void *, void *)) {
    if (0 == el_size) return -1;
    if (!initial_size) initial_size = HEAP_DEFAULT_INITIAL_SIZE;
    el_size += sizeof(struct _heap_entry);
    h->el_size = el_size;
    h->compar = compar ? compar : _heap_int_compar;
    h->num_items = 0;
    h->max_size = initial_size;
    if (0 > vmbuf_init(&h->data, initial_size * el_size) ||
        0 > vmbuf_init(&h->ofs, initial_size * sizeof(uint32_t)))
        return -1;
    void *data = vmbuf_data(&h->data);
    uint32_t *ofs = (uint32_t *)vmbuf_data(&h->ofs);
    size_t i;
    for (i = 0; i < initial_size; ++i) {
        ofs[i] = i;
        _HEAP_DATA(i)->key = -1;
    }
    return -1;
}

void heap_free(struct heap *h) {
    vmbuf_free(&h->data);
    vmbuf_free(&h->ofs);
}

uint32_t heap_insert(struct heap *h, void *user_data) {
    uint32_t el_size = h->el_size;
    if (heap_full(h)) {
        size_t new_size = h->max_size << 1;
        vmbuf_resize_by(&h->data, (new_size - h->max_size) * h->el_size);
        vmbuf_resize_by(&h->ofs, (new_size - h->max_size) * sizeof(uint32_t));
        void *data = vmbuf_data(&h->data);
        uint32_t *ofs = (uint32_t *)vmbuf_data(&h->ofs);
        uint32_t i;
        for (i = h->max_size; i < new_size; ++i) {
            ofs[i] = i;
            _HEAP_DATA(i)->key = -1;
        }
        h->max_size = new_size;
    }
    uint32_t parent, pos;
    void *data = vmbuf_data(&h->data);
    uint32_t *ofs = (uint32_t *)vmbuf_data(&h->ofs);
    int (*compar)(void *a, void *b) = h->compar;
    for (pos = h->num_items; pos > 0; pos = parent) {
        parent = (pos - 1) >> 1;
        if (0 > compar(_HEAP_DATA(ofs[parent])->user_data, user_data)) {
            uint32_t ofs_pos = ofs[pos];
            uint32_t ofs_parent = ofs[parent];
            _HEAP_DATA(ofs_pos)->key = ofs_parent;
            _HEAP_DATA(ofs_parent)->key = ofs_pos;
            ofs[pos] = ofs_parent;
            ofs[parent] = ofs_pos;
        } else
            break;
    }
    uint32_t loc = ofs[pos];
    struct _heap_entry *d = _HEAP_DATA(loc);
    d->key = pos;
    memcpy(d->user_data, user_data, h->el_size - sizeof(struct _heap_entry));
    ++h->num_items;
    return loc;
}
