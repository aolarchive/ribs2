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
#ifndef _DS_LOADER__H_
#define _DS_LOADER__H_

#define _DS_FIELD_MAKE(DB,TABLE,NAME) DB ## _ ## TABLE ## _ ## NAME
#define DS_FIELD_MAKE(DB,TABLE,NAME) _DS_FIELD_MAKE(DB,TABLE,NAME)
#define _DS_MAKE_PATH(DB,TABLE,NAME) #DB "/" #TABLE "/" #NAME
#define DS_MAKE_PATH(DB,TABLE,NAME) _DS_MAKE_PATH(DB,TABLE,NAME)
#define _DS_STRIGIFY(x) #x
#define DS_STRIGIFY(x) _DS_STRIGIFY(x)

struct ds_loader_file {
    const char *filename;
};

#ifdef DS_LOADER_STAGE
#   undef DS_LOADER_STAGE
#endif

#endif // _DS_LOADER__H_

#ifndef DS_LOADER_STAGE /* vars */

#define DS_LOADER_BEGIN() typedef struct ds_loader {

#define DS_LOADER_END() } ds_loader_t;

#define DS_FIELD_LOADER(T,name)                                         \
    DS_FIELD(T) DS_FIELD_MAKE(DB_NAME,TABLE_NAME,name);// = DS_FIELD_INITIALIZER;

#define DS_VAR_FIELD_LOADER(name)                                     \
    struct ds_var_field DS_FIELD_MAKE(DB_NAME,TABLE_NAME,name);// = DS_VAR_FIELD_INITIALIZER;

#undef DS_LOADER_STAGE
#define DS_LOADER_STAGE 1

#include DS_LOADER_CONFIG
/*
 * not including next stages yet, to allow creating .h file with the
 * ds_loader_t declaration
 */
/* #include "ds_loader.h" */

#elif DS_LOADER_STAGE==1 /* load */

#undef DS_LOADER_BEGIN
#define DS_LOADER_BEGIN()          \
    static int ds_loader_init(ds_loader_t *ds_loader, const char *base_dir) { \
    int res = 0;                                       \
    struct vmbuf vmb = VMBUF_INITIALIZER;              \
    vmbuf_init(&vmb, 4096);

#undef DS_LOADER_END
#define DS_LOADER_END() \
    ds_loader_done:     \
    vmbuf_free(&vmb);   \
    return res;         \
    }

#undef DS_FIELD_LOADER
#define DS_FIELD_LOADER(T,name)                                         \
    vmbuf_reset(&vmb);                                                  \
    vmbuf_sprintf(&vmb, "%s/%s/%s/%s", base_dir, DS_STRIGIFY(DB_NAME), DS_STRIGIFY(TABLE_NAME), #name); \
    if (0 > (res = DS_FIELD_INIT(T, &(ds_loader->DS_FIELD_MAKE(DB_NAME,TABLE_NAME,name)), vmbuf_data(&vmb)))) \
        goto ds_loader_done;

#undef DS_VAR_FIELD_LOADER
#define DS_VAR_FIELD_LOADER(name)                                     \
    vmbuf_reset(&vmb);                                                  \
    vmbuf_sprintf(&vmb, "%s/%s/%s/%s", base_dir, DS_STRIGIFY(DB_NAME), DS_STRIGIFY(TABLE_NAME), #name); \
    if (0 > (res = ds_var_field_init(&(ds_loader->DS_FIELD_MAKE(DB_NAME,TABLE_NAME,name)), vmbuf_data(&vmb)))) \
        goto ds_loader_done;


#undef DS_LOADER_STAGE
#define DS_LOADER_STAGE 2
#include DS_LOADER_CONFIG
#include "ds_loader.h"

#elif DS_LOADER_STAGE==2 /* file list */

#undef DS_LOADER_BEGIN
#undef DS_LOADER_END
#undef DS_FIELD_LOADER
#undef DS_VAR_FIELD_LOADER

#define DS_LOADER_BEGIN()                       \
    static const char *ds_loader_files[] = {
#define DS_LOADER_END() NULL };
#define DS_FIELD_LOADER(T,name)                 \
    DS_MAKE_PATH(DB_NAME,TABLE_NAME,name),
#define DS_VAR_FIELD_LOADER(name)             \
    DS_MAKE_PATH(DB_NAME,TABLE_NAME,name),

#undef DS_LOADER_STAGE
#define DS_LOADER_STAGE 3
#include DS_LOADER_CONFIG
#include "ds_loader.h"

#endif

