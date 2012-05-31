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
