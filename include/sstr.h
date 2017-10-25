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
#ifndef _SSTR__H_
#define _SSTR__H_

#include <string.h>

#define SSTR(varname, val) \
    const char varname[] = val

#define SSTRE(varname, val) \
    extern const char varname[] = val

// local string (via static)
#define SSTRL(varname, val) \
    static const char varname[] = val

#define SSTREXTRN(varname) \
    extern const char varname[]

#define SSTRLEN(var) \
    (sizeof(var) - 1)

#define SSTRNCMP(var, with) \
    (strncmp(var, with, SSTRLEN(var)))

#define SSTRNCMPI(var, with) \
    (strncasecmp(var, with, SSTRLEN(var)))

#define SSTRCMP(var, with) \
    (strncmp(var, with, sizeof(var)))

#define SSTRISEMPTY(var) \
    ((var == NULL)||(var[0]=='\0'))

#define SSTRCMPI(var, with) \
    (strncasecmp(var, with, sizeof(var)))

#endif // _SSTR__H_
