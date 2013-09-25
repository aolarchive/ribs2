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
