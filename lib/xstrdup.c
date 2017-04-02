/* $Id: xstrdup.c,v 1.4 2001/07/13 18:28:45 sandro Exp $ */

#include <string.h>

#include "config.h"

/*
 * Duplicate a string.
 */
char *(xstrdup)(const char *s)
{
	return strcpy(xmalloc(strlen(s) + 1), s);
}
