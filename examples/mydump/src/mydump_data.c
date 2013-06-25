#include "ribs.h"
#include "mydump_data.h"
/* must include again for stage 1 & 2 code generation */
#include "ds_loader.h"

/*extern*/
int active = 0;
ds_loader_mydump_t data[2];

/* initialize the dump data */
int mydump_data_init(void){
    memset(&data, 0, sizeof(data));
    return 0;
}

/* load the dump data in the 'data/' directory */
int mydump_data_load(void) {
    ++active;
    if(active >1) active = 0;
    LOGGER_INFO("listing files...");
    const char **fn = ds_loader_mydump_files;
    for(;*fn;++fn){
        LOGGER_INFO("FILE: %s", *fn);
    }

    int res = ds_loader_mydump_init(&data[active], "data/");
    LOGGER_INFO("listing files...done");
        LOGGER_INFO("loading data...");
    return res;
}
