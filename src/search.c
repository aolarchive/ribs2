/*
    This file is part of RIBS2.0 (Robust Infrastructure for Backend Systems).
    RIBS is an infrastructure for building great SaaS applications (but not
    limited to).

    Copyright (C) 2011,2012 Adap.tv, Inc.

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

#include "search.h"

const void *binary_search(const void *key, const void *base, size_t nmemb, size_t size, int (*compar)(const void *, const void *)) {

    size_t low = 0, high = nmemb;

    while (low < high)
    {
        size_t curr = (low + high) >> 1;
        const void *memb = base + (size * curr);

        if(0 < compar(key, memb)) {
            low = curr;
            ++low;
        } else{
            high = curr;
        }
    }

    if (low >= nmemb)
        return NULL;

    const void *res = base + (size * low);
    if(0 != compar(key, res))
        return NULL;

    return res;
}
