#include "config.h"

#include <assert.h>
#include <err.h>
#include <stdlib.h>

/*
 * Return an allocated memory area.
 */
void *
xmalloc(size_t size)
{
	void *ptr;

	assert(size > 0);

	if ((ptr = malloc(size)) == NULL)
		err(1, NULL);

	return ptr;
}
