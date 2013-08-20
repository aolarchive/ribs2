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
#ifndef _VM_MISC__H_
#define _VM_MISC__H_

#define RIBS_VM_PAGESIZE 4096
#define RIBS_VM_PAGEMASK (RIBS_VM_PAGESIZE-1)

#define RIBS_VM_ALIGN(x) (((x)+RIBS_VM_PAGEMASK)&~RIBS_VM_PAGEMASK)

#endif // _VM_MISC__H_
