/* $Id: xrealloc.c,v 1.5 2001/07/13 18:28:45 sandro Exp $ */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "config.h"

/*
 * Resize an allocated memory area.
 */
void *(xrealloc)(void *ptr, size_t size)
{
	void *newptr;

	assert(ptr != NULL);
	assert(size > 0);

	if ((newptr = realloc(ptr, size)) == NULL) {
		fprintf(stderr, "zile: cannot allocate memory\n");
		exit(1);
	}

	return newptr;
}
