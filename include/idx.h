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
#ifndef _IDX__H_
#define _IDX__H_

#include "ribs_defs.h"
#include "template.h"
#include <stdlib.h>
#include "ds.h"
#include "vmbuf.h"
#include "search.h"

/* public interface */
/* one to one index */
#define IDX_GEN_DS_FILE_O2O(T, BASE, DB, TBL, FIELD) TEMPLATE(index_gen_generate_ds_file_o2o, T)(BASE, DB, TBL, FIELD)
#define IDX_CONTAINER_O2O_LOOKUP(T,IC,KEY,RES) TEMPLATE(index_container_o2o_lookup, T)((IC), KEY, RES)
#define IDX_CONTAINER_O2O_EXIST(T,IC,KEY) TEMPLATE(index_container_o2o_exist, T)((IC), KEY)
/* one to many index */
#define IDX_GEN_DS_FILE_O2M(T, BASE, DB, TBL, FIELD) TEMPLATE(index_gen_generate_ds_file_o2m, T)(BASE, DB, TBL, FIELD)
#define IDX_CONTAINER_O2M_LOOKUP(T,IC,KEY,VECT,SIZE) TEMPLATE(index_container_o2m_lookup, T)((IC), KEY, VECT, SIZE)
#define IDX_CONTAINER_O2M_EXIST(T,IC,KEY) TEMPLATE(index_container_o2m_exist, T)((IC), KEY)

/* internal stuff */
#define IDX_FW_INDEX(T) struct TEMPLATE(index_gen_fw_index, T)
#define IDX_CONTAINER_O2O(T) struct TEMPLATE(index_container_o2o, T)
#define IDX_CONTAINER_O2M(T) struct TEMPLATE(index_container_o2m, T)
#define IDX_CONTAINER_O2O_INIT(T,X,F) TEMPLATE(index_container_o2o_init, T)((X), F)
#define IDX_CONTAINER_O2M_INIT(T,X,F) TEMPLATE(index_container_o2m_init, T)((X), F)

#define IDX_O2O_SIGNATURE "RIBSO2O"
#define IDX_O2M_SIGNATURE "RIBSO2M"

struct index_header_o2o {
    char signature[8];
    uint32_t num_keys;
};

struct index_header_o2m {
    char signature[8];
    uint64_t keys_ofs;
    uint32_t num_keys;
};

#ifdef T
#undef T
#endif

#define T int8_t
#include "index_gen.h"
#include "index_container.h"
#undef T

#define T uint8_t
#include "index_gen.h"
#include "index_container.h"
#undef T

#define T int16_t
#include "index_gen.h"
#include "index_container.h"
#undef T

#define T uint16_t
#include "index_gen.h"
#include "index_container.h"
#undef T

#define T int32_t
#include "index_gen.h"
#include "index_container.h"
#undef T

#define T uint32_t
#include "index_gen.h"
#include "index_container.h"
#undef T

#define T int64_t
#include "index_gen.h"
#include "index_container.h"
#undef T

#define T uint64_t
#include "index_gen.h"
#include "index_container.h"
#undef T

#define T float
#include "index_gen.h"
#include "index_container.h"
#undef T

#define T double
#include "index_gen.h"
#include "index_container.h"
#undef T


#endif // _IDX__H_
