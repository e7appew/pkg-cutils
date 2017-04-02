/*	$Id: tree.c,v 1.16 1997/08/30 01:14:29 sandro Exp $	*/

/*
 * Copyright (c) 1995, 1996, 1997
 *	Sandro Sigala, Brescia, Italy.  All rights reserved.
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
#include "cinfodb.h"

/*
 * Build a new object allocating a memory area.
 */
struct object_s *
new_object(char *name)
{
	struct object_s *p;

	p = (struct object_s *)xmalloc(sizeof(struct object_s));

	p->name = name;
	p->next = NULL;
	p->object_list = NULL;
	p->variable_list = NULL;

	return p;
}

/*
 * Build a new variable allocating a memory area.
 */
struct variable_s *
new_variable(char *name, char *value)
{
	struct variable_s *p;

	p = (struct variable_s *)xmalloc(sizeof(struct variable_s));

	p->name = name;
	p->value = value;
	p->next = NULL;

	return p;
}

/*
 * Return the last object of an object list.
 * This function assumes that p != NULL.
 */
struct object_s *
last_object(struct object_s *p)
{
	while (p->next != NULL)
		p = p->next;
	return p;
}

/*
 * Return the last variable of a variable list.
 * This function assumes that p != NULL.
 */
struct variable_s *
last_variable(struct variable_s *p)
{
	while (p->next != NULL)
		p = p->next;
	return p;
}

/*
 * Count the number of objects that are in an object list.
 */
int
count_objects(struct object_s *p)
{
	int count = 0;
	for (; p != NULL; p = p->next)
		count++;
	return count;
}

/*
 * Count the number of variables that are is a variable list.
 */
int
count_variables(struct variable_s *p)
{
	int count = 0;
	for (; p != NULL; p = p->next)
		count++;
	return count;
}

/*
 * Link an object to an object list.
 */
struct object_s *
link_object(struct object_s *dest, struct object_s *next)
{
	dest->next = next;
	return dest;
}	

/*
 * Link a variable to a structure list.
 */
struct variable_s *
link_variable(struct variable_s *dest, struct variable_s *next)
{
	dest->next = next;
	return dest;
}	

/*
 * Deallocate the memory area used by an object.
 */
void
free_object(struct object_s *p)
{
	if (p->name != NULL)
		free(p->name);
	free(p);
}

/*
 * Deallocate the memory area used by a variable.
 */
void
free_variable(struct variable_s *p)
{
	if (p->name != NULL)
		free(p->name);
	if (p->value != NULL)
		free(p->value);
	free(p);
}

/*
 * Deallocate an object list.
 */
void
free_object_list(struct object_s *p)
{
	struct object_s *next;
	while (p != NULL) {
		next = p->next;
		if (p->variable_list != NULL)
			free_variable_list(p->variable_list);
		if (p->object_list != NULL)
			free_object_list(p->object_list);
		free_object(p);
		p = next;
	}
}

/*
 * Deallocate a variable list.
 */
void
free_variable_list(struct variable_s *p)
{
	struct variable_s *next;
	while (p != NULL) {
		next = p->next;
		free_variable(p);
		p = next;
	}
}
