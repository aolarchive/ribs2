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
#ifndef _BINARY_SEARCH__H_
#define _BINARY_SEARCH__H_

#include "ribs_defs.h"
#include "template.h"

/*
  example:
  ========
      typedef struct mystruct {
          int x;
          int y;
      } mystruct_t;
      int mystruct_compar(const mystruct_t *a, const mystruct_t *b) {
          return a->x < b->x;
      }
      BINARY_SEARCH_PRED(mystruct_t, mystruct_compar);
      LOWER_BOUND_PRED(mystruct_t, mystruct_compar);
      void finder(void) {
          struct mystruct data[] = {{1,100},{2,200},{50,1234}};
          struct mystruct findme = {2,0};
          const struct mystruct *res;
          if (0 == binary_search_mystruct_t(data, ARRAY_SIZE(data), &findme, &res))
              printf("res = %d   %d\n", res->x, res->y);
          else
              printf("%d not found\n", findme.x);
          findme.x = 3;
          res = lower_bound_mystruct_t(data, ARRAY_SIZE(data), &findme);
          if (res < data + ARRAY_SIZE(data))
              printf("pos = %zu, res = %d   %d\n", res - data, res->x, res->y);
          else
              printf("pos = %zu, end of array\n", res - data);
      }
*/

#define LOWER_BOUND(T) \
    static inline uint32_t TEMPLATE(lower_bound,T)(const T *data, uint32_t n, T v) { \
        uint32_t l = 0, h = n;                                          \
        while (l < h) {                                                 \
            uint32_t m = (l + h) >> 1;                                  \
            if (data[m] < v) {                                          \
                l = m;                                                  \
                ++l;                                                    \
            } else {                                                    \
                h = m;                                                  \
            }                                                           \
        }                                                               \
        return l;                                                       \
    }

#define BINARY_SEARCH(T) \
    static inline int TEMPLATE(binary_search,T)(const T *data, uint32_t n, T v, T **res) { \
        uint32_t l = 0, h = n;                                          \
        while (l < h) {                                                 \
            uint32_t m = (l + h) >> 1;                                  \
            if (data[m] < v) {                                          \
                l = m;                                                  \
                ++l;                                                    \
            } else {                                                    \
                h = m;                                                  \
            }                                                           \
        }                                                               \
        T *p = data + l;                                                \
        if (l >= n || *ptr < v || v < *ptr)                             \
            return *res = NULL, -1;                                     \
        return *res = p, 0;                                             \
    }


#define LOWER_BOUND_PRED(T,LESSTHAN)                                  \
    static inline const T *TEMPLATE(lower_bound,T)(const T *data, uint32_t n, T *v) { \
        uint32_t l = 0, h = n;                                          \
        while (l < h) {                                                 \
            uint32_t m = (l + h) >> 1;                                  \
            if (LESSTHAN(data + m,v)) {                                 \
                l = m;                                                  \
                ++l;                                                    \
            } else {                                                    \
                h = m;                                                  \
            }                                                           \
        }                                                               \
        return data + l;                                                \
    }

#define BINARY_SEARCH_PRED(T,LESSTHAN)                                  \
    static inline int TEMPLATE(binary_search,T)(const T *data, uint32_t n, T *v, const T **res) { \
        uint32_t l = 0, h = n;                                          \
        while (l < h) {                                                 \
            uint32_t m = (l + h) >> 1;                                  \
            if (LESSTHAN(data + m,v)) {                                 \
                l = m;                                                  \
                ++l;                                                    \
            } else {                                                    \
                h = m;                                                  \
            }                                                           \
        }                                                               \
        const T *p = data + l;                                          \
        if (l >= n || LESSTHAN(p,v) || LESSTHAN(v,p))                   \
            return *res = NULL, -1;                                     \
        return *res = p, 0;                                             \
    }

#endif // _BINARY_SEARCH__H_
