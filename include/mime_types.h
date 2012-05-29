#ifndef _MIME_TYPES__H_
#define _MIME_TYPES__H_

#include "ribs_defs.h"

int mime_types_init(void);
const char *mime_types_by_ext(const char *ext);
const char *mime_types_by_filename(const char *filename);

#endif // _MIME_TYPES__H_
