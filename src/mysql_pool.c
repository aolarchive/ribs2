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
#include "mysql_pool.h"
#include "hashtable.h"
#include "logger.h"
#include "list.h"

static struct hashtable ht_idle_connections;
static struct vmbuf misc;

static int create_entry(struct list *l) {
    struct mysql_pool_entry *entry = (struct mysql_pool_entry *)calloc(1, sizeof(struct mysql_pool_entry));
    if (!mysql_init(&(entry->helper.mysql))) {
        free(entry);
        return -1;
    }
    list_insert_head(l, &entry->l);
    return 0;
}

static int get_free_entry(struct mysql_login_info *info, struct list *l, struct mysql_pool_entry **mysql) {
    if (list_empty(l))
        return -1;
    struct list *item = list_pop_head(l);
    *mysql = LIST_ENTRY(item, struct mysql_pool_entry, l);
    // connect if haven't already done so.
    if (!((*mysql)->helper).is_connected) {
        if (0 > mysql_helper_connect(&(*mysql)->helper, info))
            return -1;
        ((*mysql)->helper).is_connected = 1;
    }
    return 0;
}

static inline int make_ht_key(struct vmbuf *buf, const struct mysql_login_info *info) {
    vmbuf_reset(buf);
    if (0 > vmbuf_sprintf(buf, "%s,%s,%s,%s,%u",
                          info->host, info->user, info->pass, info->db, info->port))
        return -1;
    return 0;
}

int mysql_pool_init() {
    if (0 > hashtable_init(&ht_idle_connections, 128))
        return -1;
    if (0 > vmbuf_init(&misc, 4096))
        return -1;
    return 0;
}

int mysql_pool_get(struct mysql_login_info *info, struct mysql_pool_entry **mysql) {
    if (0 > make_ht_key(&misc, info))
        return -1;
    char *ht_key = vmbuf_data(&misc);
    size_t key_len = vmbuf_wlocpos(&misc);

    uint32_t ofs = hashtable_lookup(&ht_idle_connections, ht_key, key_len);
    if (0 == ofs) {
        struct list *l = (struct list *)calloc(1, sizeof(struct list));
        list_init(l);
        ofs = hashtable_insert(&ht_idle_connections,
                               ht_key, key_len,
                               &l, sizeof(struct list *));
        if (0 == ofs) // unable to insert
            return -1;
        // add one element since we know there isn't any
        if (0 > create_entry(l))
            return -1;
        // get first free element
        if (0 > get_free_entry(info, l, mysql))
            return -1;
    }
    struct list *l = *(struct list **)hashtable_get_val(&ht_idle_connections, ofs);
    if (list_empty(l)) {
        LOGGER_INFO("adding one more entry in the list");
        if (0 > create_entry(l))
            return -1;
    }
    if (0 > get_free_entry(info, l, mysql))
        return -1;
    return 0;
}

int mysql_pool_free(struct mysql_login_info *info, struct mysql_pool_entry *mysql) {
    if (0 > make_ht_key(&misc, info))
        return -1;
    char *ht_key = vmbuf_data(&misc);
    size_t key_len = vmbuf_wlocpos(&misc);

    uint32_t ofs = hashtable_lookup(&ht_idle_connections, ht_key, key_len);
    if (0 == ofs)
        return -1;
    struct list *l = *(struct list **)hashtable_get_val(&ht_idle_connections, ofs);
    list_insert_head(l, &(mysql->l));
    return 0;
}
