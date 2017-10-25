/*
    This file is part of RIBS2.0 (Robust Infrastructure for Backend Systems).
    RIBS is an infrastructure for building great SaaS applications (but not
    limited to).

    Copyright (C) 2013,2014 Adap.tv, Inc.

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

int main(void){

    /* initialize and load the dump data */
    if (0 > mydump_data_init() ||
                0 > mydump_data_load())
        exit(EXIT_FAILURE);

    int id = 1;

    /* get values from index where id = {1, 2, 3, 4} */
    for(id=1; id <= 4; id++)
        {
        /* the other table columns */
        int32_t age;
        char *name;

        /* location of data from index lookup */
        uint32_t loc = 0;

        /* char pointer to beginning of name in data */
        char *result = "";

        /* size of name data, since variable size */
        size_t result_size = 0;

        /* index lookup will return -1 if key not found */
        if( 0> IDX_CONTAINER_O2O_LOOKUP(int32_t,
                &data[active].test_test_table_id_idx, id, &loc))
        {
                printf("index key [%d] not found\n", id);
        }

        /* else retrieve results */
        else {
            /* get age at loc */
            age = DS_FIELD_GET_VAL(&data[active].test_test_table_age, loc);

            /* get char pointer at beginning of data at loc */
            ds_var_field_get(&data[active].test_test_table_name,
                    loc, &result, &result_size);

            /* allocate memory to name according to result size */
            name = ribs_malloc(result_size + 1);

            /* copy over 'result_size' number of characters from result
                into name */
            strncpy(name, result, result_size);

            /* print result */
            printf("| %d | %s | %d |\n", id, name, age);
        }
    }

    return 0;
}
