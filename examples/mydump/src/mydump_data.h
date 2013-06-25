#ifndef _MYDUMP_DATA_H_
#define _MYDUMP_DATA_H_

#include "ribs.h"

#undef DS_LOADER_TYPENAME
#undef DS_LOADER_CONFIG

/* define the ds_loader type */
#define DS_LOADER_TYPENAME ds_loader_mydump
/* config to the mydump.ds file */
#define DS_LOADER_CONFIG "mydump.ds"

/* ds_loader.h needs to be included in both
    mydump_data.h and mydump_data.c for
    code generation */
#include "ds_loader.h"

/* data init and load functions */
int mydump_data_init(void);
int mydump_data_load(void);

/* externs to used for accessing the data */
extern int active;
extern ds_loader_mydump_t data[2];

#endif// _MYDUMP_DATA_H_
