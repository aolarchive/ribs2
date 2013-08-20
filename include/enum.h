/*
    This file is part of RIBS2.0 (Robust Infrastructure for Backend Systems).
    RIBS is an infrastructure for building great SaaS applications (but not
    limited to).

    Copyright (C) 2012,2013 Adap.tv, Inc.

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
#ifndef _ENUM__H_
#define _ENUM__H_

#define ENUM_2_STORAGE(e, t) \
    case e:                  \
    return (sizeof(t))

#define ENUM_2_STORAGE_STR(e, l) \
    case e:                      \
    return (l);

#define ENUM_TO_STRING(e) \
    case e:               \
    return (#e)

#endif // _ENUM__H_
