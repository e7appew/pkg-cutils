#include "config.h"

#include <string.h>

/*
 * Duplicate a string.
 */
char *
xstrdup(const char *s)
{
	return strcpy(xmalloc(strlen(s) + 1), s);
}
