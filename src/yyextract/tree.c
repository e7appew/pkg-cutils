/*	$Id: tree.c,v 1.12 2001/07/13 19:09:56 sandro Exp $	*/

/*
 * Copyright (c) 1995-2001 Sandro Sigala.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <err.h>

#include "config.h"
#include "tree.h"

/*
 * Build a new object allocating a memory area.
 */
struct object_s *new_object(int type, char *value)
{
	struct object_s *p;

	p = (struct object_s *)xmalloc(sizeof(struct object_s));

	p->value = xstrdup(value);
	p->type = type;

	p->next = NULL;
	p->assoc = NULL;

	return p;
}

/*
 * Return the last object of an object list.
 * This function assumes that p != NULL.
 */
struct object_s *last_object(struct object_s *p)
{
	while (p->next != NULL)
		p = p->next;
	return p;
}

/*
 * Count the number of objects that are in an object list.
 */
int count_objects(struct object_s *p)
{
	int count = 0;
	for (; p != NULL; p = p->next)
		count++;
	return count;
}

/*
 * Link an object to an object list.
 */
struct object_s *link_object(struct object_s *dest, struct object_s *next)
{
	dest->next = next;
	return dest;
}	

/*
 * Associate an object to an object list.
 */
struct object_s *assoc_object(struct object_s *dest, struct object_s *assoc)
{
	dest->assoc = assoc;
	return dest;
}	

/*
 * Deallocate the memory area used by an object.
 */
void free_object(struct object_s *p)
{
	if (p->value != NULL)
		free(p->value);
	free(p);
}

/*
 * Deallocate an object list.
 */
void free_object_list(struct object_s *p)
{
	struct object_s *next;
	while (p != NULL) {
		next = p->next;
		if (p->assoc != NULL)
			free_object_list(p->assoc);
		free_object(p);
		p = next;
	}
}
