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
#include "json_dom.h"

inline struct json_dom_node *_new_node(void) {
    return ribs_malloc(sizeof(struct json_dom_node));
}

inline struct json_dom_node *child_node(struct json_dom_node *parent, struct json_dom_node *node) {
    if (parent) {
        node->parent = parent;
        if (parent->last_child) {
            node->prev_sibling = parent->last_child;
            node->next_sibling = NULL;
            parent->last_child->next_sibling = node;
            parent->last_child = node;
        } else {
            parent->last_child = parent->first_child = node;
            node->prev_sibling = node->next_sibling = NULL;
        }
    } else
        node->parent = node->prev_sibling = node->next_sibling = NULL;
    return node;
}

static int _parse_string(struct json_dom *js) {
    ++js->cur; /* skip the starting character, assume is \" */
    const char *start = js->cur;
    for (; *js->cur; ++js->cur) {
        if ('\"' == *js->cur) {
            break;
        } else if ('\\' == *js->cur) {
            ++js->cur;
            if (0 == *js->cur) /* some sort of unexpected error */
                return -1;
        }
    }
    if ('\"' != *js->cur) {
        js->err = "string";
        return -1; /* this is how we report back an error */
    }
    struct json_dom_node *node = _new_node();
    *node = (struct json_dom_node){.value = start, .value_len = js->cur - start, .type = JSON_DOM_TYPE_STR};
    if (js->is_key)
        child_node(js->node->last_child, node);
    else
        child_node(js->node, node);
    ++js->cur; /* skip the ending character assuming is a \" */
    return 0;
}

static int _parse_primitive(struct json_dom *js) {
    const char *start = js->cur;
    struct json_dom_node *node;
    for (; *js->cur; ++js->cur) {
        switch (*js->cur) {
        case ']':
        case '}':
        case '\t':
        case '\r':
        case '\n':
        case ' ':
        case ',':
            node = _new_node();
            *node = (struct json_dom_node){.value = start, .value_len = js->cur - start, .type = JSON_DOM_TYPE_PRIM};
            if (js->is_key)
                child_node(js->node->last_child, node);
            else
                child_node(js->node, node);
            return 0;
        }
    }
    js->err = "primitive";
    return -1;
}

int json_dom_parse(struct json_dom *js, const char *str) {
    struct json_dom_node *root, *node;
    js->node = root = _new_node();
    *root = (struct json_dom_node){"", 4, JSON_DOM_TYPE_OBJ, NULL, NULL, NULL, NULL, NULL};
    js->level = 0;
    js->cur = str;
    js->err = NULL;
    js->is_key = 0;
    for (; *js->cur; ) {
        switch(*js->cur) {
        case '{':
        case '[':
            if (js->is_key) {
                js->is_key = 0;
                js->node->last_child->type = (*js->cur=='[' ? JSON_DOM_TYPE_ARRAY : JSON_DOM_TYPE_OBJ);
                js->node = js->node->last_child;
            } else {
                node = _new_node();
                *node = (struct json_dom_node){.value = NULL, .value_len = 0, .type = (*js->cur=='[' ? JSON_DOM_TYPE_ARRAY : JSON_DOM_TYPE_OBJ)};
                js->node = child_node(js->node, node);
            }
            ++js->level;
            break;
        case '}':
        case ']':
            if (js->level == 0)
                return js->err = "unbalanced parenthesis", -1;
            js->node = js->node->parent;
            js->is_key = 0;
            --js->level;
            break;
        case '\"':
            if (0 > _parse_string(js))
                return -1;
            continue;
        case '-':
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
        case 't':
        case 'f':
        case 'n':
            if (0 > _parse_primitive(js))
                return -1;
            continue;
        case ':':
            if (JSON_DOM_TYPE_ARRAY == js->node->type)
                return js->err = "wrong value in array", -1;
            if (NULL == js->node->last_child)
                return js->err = "malformed", -1;
            js->is_key = 1;
            break;
        case '\t':
        case '\r':
        case '\n':
        case ' ':
            break;
        case ',':
            js->is_key = 0;
            break;
        default:
            break;
        }
        ++js->cur;
    }
    js->node = root;
    return 0;
}

static int _dump(struct json_dom_node *node, int level) {
    const char *TYPES[] = {"S","P","O","A"};
    int i = 0;
    for (; node; ++i) {
        if (node->parent->type == JSON_DOM_TYPE_ARRAY)
            printf("%*c [%s][%d] %.*s\n", level*4, ' ', TYPES[node->type], i, node->value_len, node->value);
        else
            printf("%*c [%s] %.*s\n", level*4, ' ', TYPES[node->type], node->value_len, node->value);
        _dump(node->first_child, level + 1);
        node = node->next_sibling;
    }
    return 0;
}

int json_dom_dump(struct json_dom *js) {
    return _dump(js->node->first_child, 0);
}

static int _build_index(struct vmbuf *path_buf, struct json_dom_node *node, int max_level, int level, struct hashtable *ht) {
    if (level == max_level)
        return 0;
    int i = 0;
    size_t marker = vmbuf_wlocpos(path_buf);
    for (; node; ++i, node = node->next_sibling) {
        vmbuf_wlocset(path_buf, marker);

        if (node->parent->type == JSON_DOM_TYPE_ARRAY) {
            if (node->type==JSON_DOM_TYPE_OBJ || node->type==JSON_DOM_TYPE_ARRAY)
                vmbuf_sprintf(path_buf, "[%d]%.*s", i, node->value_len, node->value);
            else
                vmbuf_sprintf(path_buf, "[%d]", i);
        } else {
            if (node->value_len)
                vmbuf_sprintf(path_buf, ".%.*s", node->value_len, node->value);
        }

        struct json_dom_node *node_to_insert = node;
        if(node->parent->type != JSON_DOM_TYPE_ARRAY && node->type==JSON_DOM_TYPE_PRIM)
            continue;

        if(node->parent->type!=JSON_DOM_TYPE_ARRAY && node->first_child!=NULL && node->first_child->type==JSON_DOM_TYPE_PRIM)
            node_to_insert = node->first_child;

        if (vmbuf_wlocpos(path_buf) > 0)
            hashtable_insert(ht, vmbuf_data(path_buf)+1, vmbuf_wlocpos(path_buf) - 1, &node_to_insert, sizeof(node_to_insert));
        _build_index(path_buf, node->first_child, max_level, level + 1, ht);
    }
    return 0;
}

int json_dom_build_index(struct json_dom *js, int max_level, struct hashtable *ht) {
    static struct vmbuf path_buf = VMBUF_INITIALIZER;
    vmbuf_init(&path_buf, 4096);
    return _build_index(&path_buf, js->node->first_child, max_level, 0, ht);
}

struct json_dom_node *json_dom_find_child(struct json_dom_node *node, const char *name) {
    node = node->first_child;
    size_t l = strlen(name);
    for (; node; node = node->next_sibling) {
        if (l == node->value_len && 0 == strncmp(name, node->value, node->value_len)) {
            /* found the node, if it has a solo primitive child, return the child */
            if(node->type!=JSON_DOM_TYPE_ARRAY && node->first_child!=NULL &&
               (node->first_child->type==JSON_DOM_TYPE_PRIM || node->first_child->type==JSON_DOM_TYPE_STR))
                return node->first_child;
            else
                return node;
        }
    }
    return NULL;
}

inline int json_dom_parse_build_index(const char *str, int max_level,struct hashtable *ht) {
    struct json_dom *js = ribs_malloc(sizeof(struct json_dom));
    *ht = (struct hashtable)HASHTABLE_INITIALIZER;
    return
         (0 <= hashtable_init(ht, max_level * 12)             &&
          0 <= json_dom_parse(js, str)                        &&
          0 <= json_dom_build_index(js, max_level, ht)) ? 0 : -1;
}

inline struct json_dom_node *json_dom_index_find_path(struct hashtable *ht, const char *path) {
    struct json_dom_node** node_ptr = (struct json_dom_node**) hashtable_lookup_str(ht, path, NULL);
    return node_ptr != NULL ? *node_ptr : NULL;
}

int json_dom_get_int_val(struct json_dom_node *node, int *int_out) {
    if(node==NULL) return -1;

    char s[node->value_len + 1];
    memcpy(s, node->value, node->value_len);
    s[sizeof(s)-1] = 0;

    char *end_ptr;
    errno = 0;
    *int_out = (int)strtoul(s, &end_ptr, 0);
    if(errno || end_ptr==s)
        return -1;
    return 0;
}

int json_dom_get_double_val(struct json_dom_node *node, double *double_out) {
    if(node==NULL) return -1;

    char s[node->value_len +1];
    memcpy(s, node->value, node->value_len);
    s[sizeof(s)-1] = 0;

    char *end_ptr;
    errno = 0;
    *double_out = strtod(s, &end_ptr);
    if(errno || end_ptr==s)
        return -1;
    return 0;
}

int json_dom_copy_str_val(struct json_dom_node *node, char **str_out) {
    if(node==NULL) return -1;

    *str_out = ribs_malloc(node->value_len+1);
    if(*str_out==NULL) return -1;

    memcpy(*str_out, node->value, node->value_len);
    (*str_out)[node->value_len] = 0;
    return 0;
}

int json_dom_get_array_size(struct json_dom_node *node, int *size_out) {
    *size_out = 0;
    if(node==NULL || node->type!=JSON_DOM_TYPE_ARRAY) return -1;

    for(node = node->first_child; node!=NULL; (*size_out)++)
        node = node->next_sibling;

    return 0;
}
