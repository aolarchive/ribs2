#ifndef _LIST__H_
#define _LIST__H_

#include <stdint.h>
#include <stddef.h>

#define _LIST_INLINE static inline

struct list {
    struct list *next, *prev;
};

#define LIST_INITIALIZER(name) { &(name), &(name) }
#define LIST_CREATE(name) struct list name = LIST_INITIALIZER(name);

#define list_entry(ptr, type, member)                   \
    ((type *)((char *)(ptr)-offsetof(type, member)))

_LIST_INLINE void list_init(struct list *list) {
    list->next = list;
    list->prev = list;
}

_LIST_INLINE void list_insert_head(struct list *list, struct list *entry) {
    entry->next = list->next;
    entry->prev = list;
    list->next->prev = entry;
    list->next = entry;
}

_LIST_INLINE void list_insert_tail(struct list *list, struct list *entry) {
    entry->next = list;
    entry->prev = list->prev;
    list->prev->next = entry;
    list->prev = entry;
}

_LIST_INLINE int list_empty(struct list *list) {
    return list == list->next;
}

_LIST_INLINE struct list *list_head(struct list *list) {
    return list->next;
}

_LIST_INLINE struct list *list_tail(struct list *list) {
    return list->prev;
}

_LIST_INLINE void list_remove(struct list *entry) {
    entry->next->prev = entry->prev;
    entry->prev->next = entry->next;
}

_LIST_INLINE struct list *list_pop_head(struct list *list) {
    struct list *e = list_head(list);
    list_remove(e);
    return e;
}

_LIST_INLINE struct list *list_pop_tail(struct list *list) {
    struct list *e = list_tail(list);
    list_remove(e);
    return e;
}

_LIST_INLINE void list_make_first(struct list *list, struct list *entry) {
    list_remove(entry);
    list_insert_head(list, entry);
}

_LIST_INLINE void list_make_last(struct list *list, struct list *entry) {
    list_remove(entry);
    list_insert_tail(list, entry);
}

#endif // _LIST__H_
