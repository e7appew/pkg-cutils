#include "config.h"

#include <assert.h>
#include <err.h>
#include <stdlib.h>

/*
 * Resize an allocated memory area.
 */
void *
xrealloc(void *ptr, size_t size)
{
	void *newptr;

	assert(ptr != NULL);
	assert(size > 0);

	if ((newptr = realloc(ptr, size)) == NULL)
		err(1, NULL);

	return newptr;
}
