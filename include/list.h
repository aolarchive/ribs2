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
#ifndef _LIST__H_
#define _LIST__H_

#include "ribs_defs.h"

struct list {
    struct list *next, *prev;
};

#define LIST_INITIALIZER(name) { &(name), &(name) }
#define LIST_NULL_INITIALIZER { NULL, NULL }
#define LIST_CREATE(name) struct list name = LIST_INITIALIZER(name);

#define LIST_ENTRY(ptr, type, member)                   \
    ((type *)((char *)(ptr)-offsetof(type, member)))

_RIBS_INLINE_ void list_init(struct list *list) {
    list->next = list;
    list->prev = list;
}

_RIBS_INLINE_ int list_is_null(struct list *list) {
    return NULL == list->next;
}

_RIBS_INLINE_ void list_set_null(struct list *list) {
    list->next = NULL;
    list->prev = NULL;
}

_RIBS_INLINE_ int list_is_head(struct list *head, struct list *iterator) {
    return iterator == head;
}

_RIBS_INLINE_ struct list *list_next(struct list *iterator){
    return iterator->next;
}

_RIBS_INLINE_ struct list *list_prev(struct list *iterator){
    return iterator->prev;
}

_RIBS_INLINE_ void list_insert_head(struct list *list, struct list *entry) {
    entry->next = list->next;
    entry->prev = list;
    list->next->prev = entry;
    list->next = entry;
}

_RIBS_INLINE_ void list_insert_tail(struct list *list, struct list *entry) {
    entry->next = list;
    entry->prev = list->prev;
    list->prev->next = entry;
    list->prev = entry;
}

_RIBS_INLINE_ int list_empty(struct list *list) {
    return list == list->next;
}

_RIBS_INLINE_ struct list *list_head(struct list *list) {
    return list->next;
}

_RIBS_INLINE_ struct list *list_tail(struct list *list) {
    return list->prev;
}

_RIBS_INLINE_ void list_remove(struct list *entry) {
    entry->next->prev = entry->prev;
    entry->prev->next = entry->next;
}

_RIBS_INLINE_ struct list *list_pop_head(struct list *list) {
    struct list *e = list_head(list);
    list_remove(e);
    return e;
}

_RIBS_INLINE_ struct list *list_pop_tail(struct list *list) {
    struct list *e = list_tail(list);
    list_remove(e);
    return e;
}

_RIBS_INLINE_ void list_make_first(struct list *list, struct list *entry) {
    list_remove(entry);
    list_insert_head(list, entry);
}

_RIBS_INLINE_ void list_make_last(struct list *list, struct list *entry) {
    list_remove(entry);
    list_insert_tail(list, entry);
}

#define LIST_FOR_EACH(head, iterator)                                   \
    for ((iterator) = (head)->next; (iterator) != (head); (iterator) = (iterator)->next)

#define LIST_FOR_EACH_REV(head, iterator)                               \
    for ((iterator) = (head)->prev; (iterator) != (head); (iterator) = (iterator)->prev)

#endif // _LIST__H_
