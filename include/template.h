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
#ifndef _TEMPLATE__H_
#define _TEMPLATE__H_

#define _MACRO_CONCAT(A,B) A ## B
#define MACRO_CONCAT(A,B) _MACRO_CONCAT(A,B)

#define _TEMPLATE_HELPER(S,T) S ## _ ## T
#define TEMPLATE(S,T) _TEMPLATE_HELPER(S,T)

#define _TEMPLATE_FUNC_HELPER(S,T,F) S ## _ ## T ## _ ## F
#define TEMPLATE_FUNC(S,T,F) _TEMPLATE_FUNC_HELPER(S,T,F)

#define _STRINGIFY(T) #T
#define STRINGIFY(T) _STRINGIFY(T)


#endif // _TEMPLATE__H_
