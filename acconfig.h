@BOTTOM@

#include "version.h"

#include <stddef.h>

#ifndef HAVE_XMALLOC
extern void *xmalloc(size_t);
#endif

#ifndef HAVE_XREALLOC
extern void *xrealloc(void *, size_t);
#endif

#ifndef HAVE_XSTRDUP
extern char *xstrdup(const char *);
#endif
