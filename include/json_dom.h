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
#ifndef _JSON_DOM__H_
#define _JSON_DOM__H_
#include "ribs.h"
#include "vmbuf.h"

#include <stdint.h>

enum {
    JSON_DOM_TYPE_STR,
    JSON_DOM_TYPE_PRIM,
    JSON_DOM_TYPE_OBJ,
    JSON_DOM_TYPE_ARRAY,
};

struct json_dom_node {
    const char *value;
    uint32_t value_len;
    int type;
    struct json_dom_node *parent;
    struct json_dom_node *first_child;
    struct json_dom_node *last_child;
    struct json_dom_node *prev_sibling;
    struct json_dom_node *next_sibling;
};

struct json_dom {
    int level;
    const char *cur;
    const char *err;
    int is_key;
    struct json_dom_node *node;
};

int json_dom_parse(struct json_dom *js, const char *str);
int json_dom_dump(struct json_dom *js);
static inline struct json_dom_node *json_dom_root(struct json_dom *js) { return js->node->first_child; }
struct json_dom_node *json_dom_find_child(struct json_dom_node *node, const char *name);
int json_dom_get_int_val(struct json_dom_node *node, int *int_out);
int json_dom_get_double_val(struct json_dom_node *node, double *double_out);
int json_dom_copy_str_val(struct json_dom_node *node, char **str_out);
int json_dom_get_array_size(struct json_dom_node *node, int *size_out);
int json_dom_build_index(struct json_dom *js, int max_level, struct hashtable *ht);
inline int json_dom_parse_build_index(const char *str, int max_level,struct hashtable *ht);

#define json_dom_int_from_child(node, name, val) json_dom_get_int_val(json_dom_find_child(node, name), val) 
#define json_dom_dub_from_child(node, name, val) json_dom_get_double_val(json_dom_find_child(node, name), val)
#define json_dom_str_from_child(node, name, val) json_dom_copy_str_val(json_dom_find_child(node, name), val)

/*
   'path' conforms to the following patterns, and uses a hashtable built by *_build_index():
   "parent.child.array[12]"
   "result.code"
   "user[14].name"
*/
inline struct json_dom_node *json_dom_index_find_path(struct hashtable *ht, const char *path);

#endif // _JSON_DOM__H_
