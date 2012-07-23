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
/*
 * inline
 */
_RIBS_INLINE_ struct ribs_context *ctx_pool_get(struct ctx_pool *cp) {
   if (NULL == cp->freelist && 0 != ctx_pool_createstacks(cp, cp->grow_by, cp->stack_size, cp->reserved_size))
      return NULL;
   struct ribs_context *ctx = cp->freelist;
   cp->freelist = ctx->next_free;
   return ctx;
}

_RIBS_INLINE_ void ctx_pool_put(struct ctx_pool *cp, struct ribs_context *ctx) {
   ctx->next_free = cp->freelist;
   cp->freelist = ctx;
}
