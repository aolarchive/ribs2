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
#ifndef _HEAP__H_
#define _HEAP__H_

#include "ribs_defs.h"
#include <unistd.h>
#include <stdint.h>
#include "vmbuf.h"

struct heap {
    struct vmbuf data;
    struct vmbuf ofs;
    uint32_t el_size;
    uint32_t max_size;
    uint32_t num_items;
    int (*compar)(void *a, void *b);
};

#define HEAP_DEFAULT_INITIAL_SIZE 128
#define HEAP_INITIALIZER { VMBUF_INITIALIZER, VMBUF_INITIALIZER, 0, 0, 0, NULL }

/*
  // example:
  heap_init(&heap, 16, sizeof(int), NULL);

  // Note: if compar is NULL, the default is int comparison

  // writing compar function:
  struct my_data {
      int x,y;
  };

  int compar_func(void *a, void *b) {
      struct my_data *aa = a, *bb = b;
      return (aa->x < bb->x ? -1 : (bb->x < aa->x ? 1 : 0));
  }
  heap_init(&heap, 16, sizeof(struct my_data), compar_func);

  insert:
  uint32_t loc = heap_insert(&heap, &my_data);
  // remove 5th item:
  heap_remove_item(&heap, 5);

  // remove previously inserted item, when index is unknown
  // but loc is available
  heap_remove(&heap, loc);

  // get the top element without removing it
  my_data_ptr = heap_top(&heap);

  // remove the top element
  heap_remove_top(&heap);
*/

int heap_init(struct heap *h, size_t initial_size, size_t el_size, int (*compar)(void *, void *));
void heap_free(struct heap *h);
_RIBS_INLINE_ void *heap_top(struct heap *h);
_RIBS_INLINE_ void heap_remove_item(struct heap *h, uint32_t index);
_RIBS_INLINE_ void heap_remove(struct heap *h, uint32_t loc);
_RIBS_INLINE_ int heap_empty(struct heap *h);
uint32_t heap_insert(struct heap *h, void *user_data);
_RIBS_INLINE_ void heap_build(struct heap *h);
_RIBS_INLINE_ void heap_remove_top(struct heap *h);

#include "../src/_heap.c"

#endif // _HEAP__H_
