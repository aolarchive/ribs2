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

#endif // _DS_LOADER__H_

#ifndef DS_LOADER_STAGE /* vars */

#define DS_LOADER_BEGIN()

#define DS_LOADER_END()

#define DS_FIELD_LOADER(T,name)                                         \
    DS_FIELD(T) DS_FIELD_MAKE(DB_NAME,TABLE_NAME,name) = DS_FIELD_INITIALIZER;

#undef DS_LOADER_STAGE
#define DS_LOADER_STAGE 1
#include DS_LOADER_CONFIG
#include "ds_loader.h"

#elif DS_LOADER_STAGE==1 /* load */

#undef DS_LOADER_BEGIN
#define DS_LOADER_BEGIN()          \
    static int ds_loader_init(const char *base_dir) {  \
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
    if (0 > (res = DS_FIELD_INIT(T, &(DS_FIELD_MAKE(DB_NAME,TABLE_NAME,name)), vmbuf_data(&vmb)))) \
        goto ds_loader_done;

#undef DS_LOADER_STAGE
#define DS_LOADER_STAGE 2
#include DS_LOADER_CONFIG
#include "ds_loader.h"

#elif DS_LOADER_STAGE==2 /* file list */

#undef DS_LOADER_BEGIN
#undef DS_LOADER_END
#undef DS_FIELD_LOADER

#define DS_LOADER_BEGIN()                       \
    static const char *ds_loader_files[] = {
#define DS_LOADER_END() NULL };
#define DS_FIELD_LOADER(T,name)                 \
    DS_MAKE_PATH(DB_NAME,TABLE_NAME,name),

#undef DS_LOADER_STAGE
#define DS_LOADER_STAGE 3
#include DS_LOADER_CONFIG
#include "ds_loader.h"

#endif

