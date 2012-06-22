#ifndef _RIBS_DEFS__H_
#define _RIBS_DEFS__H_

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stddef.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>


#define _RIBS_INLINE_ static inline

#define likely(x)     __builtin_expect((x),1)
#define unlikely(x)   __builtin_expect((x),0)

#endif // _RIBS_DEFS__H_
