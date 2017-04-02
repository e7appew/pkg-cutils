/*	$Id: fileio.c,v 1.18 1997/08/30 01:14:28 sandro Exp $	*/

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <err.h>

#include "config.h"
#include "cinfodb.h"

static FILE *cinfodb;
static char *cinfodb_filename;

/*
 * Get an unsigned char from the file.
 */
static unsigned char
fget_uchar(void)
{
	int c;
	if ((c = getc(cinfodb)) == EOF)
		errx(1, "%s: unexpected end of file", cinfodb_filename);
	return c;
}

/*
 * Put an unsigned char into the file.
 */
static void
fput_uchar(unsigned char c)
{
	if (putc(c, cinfodb) == EOF)
		errx(1, "%s: error encourred while writing to file", cinfodb_filename);
}

/*
 * Get an unsigned short from the file.
 */
static unsigned short
fget_ushort(void)
{
#ifdef LITTLE_ENDIAN_BYTE_ORDER
	return (fget_uchar() + (fget_uchar() << 8));
#else
	return ((fget_uchar() << 8) + fget_uchar());
#endif
}

/*
 * Put an unsigned short into the file.
 */
static void
fput_ushort(unsigned short us)
{
#ifdef LITTLE_ENDIAN_BYTE_ORDER
	fput_uchar(us & 255);
	fput_uchar(us >> 8);
#else
	fput_uchar(us >> 8);
	fput_uchar(us & 255);
#endif
}

/*
 * Get an unsigned long from the file.
 */
static unsigned long
fget_ulong(void)
{
#ifdef LITTLE_ENDIAN_BYTE_ORDER
	return (fget_uchar() + (fget_uchar() << 8) +
		(fget_uchar() << 16) + (fget_uchar() << 24));
#else
	return ((fget_uchar() << 24) + (fget_uchar() << 16) +
		(fget_uchar() << 8) + fget_uchar());
#endif
}

/*
 * Put an unsigned long into the file.
 */
static void
fput_ulong(unsigned long ul)
{
#ifdef LITTLE_ENDIAN_BYTE_ORDER
	fput_uchar(ul & 255);
	fput_uchar(ul >> 8);
	fput_uchar(ul >> 16);
	fput_uchar(ul >> 24);
#else
	fput_uchar(ul >> 24);
	fput_uchar(ul >> 16);
	fput_uchar(ul >> 8);
	fput_uchar(ul & 255);
#endif
}

/*
 * Get a string from the file.
 */
static char *
fget_string(void)
{
	char *strbuf, *p;
	int c, i = 0, maxlen = 50;

	strbuf = (char *)xmalloc(maxlen);
	p = strbuf;

	while ((c = getc(cinfodb)) != EOF && c != '\0') {
		if (++i >= maxlen) {
			int offset;
			offset = p - strbuf;
			maxlen *= 2;
			strbuf = (char *)xrealloc(strbuf, maxlen);
			p = strbuf + offset;
		}
		*p++ = c;
	}
	if (c == EOF)
		errx(1, "%s: unexpected end of file", cinfodb_filename);
	*p = '\0';

	return strbuf;
}

/*
 * Put a string into the file.
 */
static void
fput_string(char *p)
{
	if (fwrite(p, strlen(p) + 1, 1, cinfodb) != 1)
		errx(1, "%s: error encourred while writing to file", cinfodb_filename);
}

/*
 * Tree-file functions.
 */

/*
 * Output a variable into the file.
 */
static void
output_variable(struct variable_s *p)
{
	fput_ushort((unsigned short)count_variables(p));

	for (; p != NULL; p = p->next) {
		fput_string(p->name);
		fput_string(p->value);
	}
}

/*
 * Output an object into the file.
 */
static void
output_object(struct object_s *p)
{
	unsigned char flags;

	fput_ushort((unsigned short)count_objects(p));

	for (; p != NULL; p = p->next) {
		fput_string(p->name);

		flags = 0;
		if (count_variables(p->variable_list) >= 1)
			flags |= 1;
		if (count_objects(p->object_list) >= 1)
			flags |= 2;

		fput_uchar(flags);

		if (p->variable_list != NULL)
				output_variable(p->variable_list);
		if (p->object_list != NULL)
				output_object(p->object_list);
	}
}

/*
 * Write the object tree into the cinfo database file.
 */
void
write_cinfodb_stream(FILE *_cinfodb, char *filename, struct object_s *object_tree)
{
	char header[] = { CINFODB_HEADER };

	cinfodb = _cinfodb;

	cinfodb_filename = xstrdup(filename);

	if (fwrite(header, sizeof(header), 1, cinfodb) != 1)
		errx(1, "%s: error encourred while writing to file", cinfodb_filename);

	output_object(object_tree);

	fclose(cinfodb);

	free(cinfodb_filename);
}

/*
 * Write the object tree into the cinfo database file.
 */
void
write_cinfodb(char *filename, struct object_s *object_tree)
{
	FILE *_cinfodb;

	if ((_cinfodb = fopen(filename, "w")) == NULL)
		err(1, "%s", filename);

	write_cinfodb_stream(_cinfodb, filename, object_tree);
}

/*
 * Check if we are at the end of the file and warn the user if there is
 * some other stuff before the end.
 */
static void
check_tail(void)
{
	if (getc(cinfodb) != EOF)
		warnx("%s: trailing garbage ignored", cinfodb_filename);
}

/*
 * Read a variable from the file.
 */
static struct variable_s *
input_variable(void)
{
	struct variable_s *vp;

	vp = (struct variable_s *)xmalloc(sizeof(struct variable_s));

	vp->name = fget_string();
	vp->value = fget_string();
	vp->next = NULL;

	return vp;
}

static struct variable_s *input_variable_list(void);
static struct object_s *input_object_list(void);

/*
 * Read an object from the file.
 */
static struct object_s *
input_object(void)
{
	struct object_s *op;
	unsigned char flags;

	op = (struct object_s *)xmalloc(sizeof(struct object_s));

	op->name = fget_string();
	flags = fget_uchar();

	if (flags & 1)
		op->variable_list = input_variable_list();
	else
		op->variable_list = NULL;

	if (flags & 2)
		op->object_list = input_object_list();
	else
		op->object_list = NULL;

	op->next = NULL;

	return op;
}

/*
 * Read a variable list from the file.
 */
static struct variable_s *
input_variable_list(void)
{
	struct variable_s *head = NULL, *last, *vp;
	int i, vars;

	/*
	 * Get the number of variables.
	 */
	vars = fget_ushort();

	/*
	 * Build the variable list.
	 */
	for (i = 0; i < vars; i++) {
		vp = input_variable();
		if (head == NULL)
			head = vp;
		else
			last->next = vp;
		last = vp;
	}

	return head;
}

/*
 * Read an object list from the file.
 */
static struct object_s *
input_object_list(void)
{
	struct object_s *head = NULL, *last, *op;
	int i, objs;

	/*
	 * Get the number of objects.
	 */
	objs = fget_ushort();

	/*
	 * Build the object list.
	 */
	for (i = 0; i < objs; i++) {
		op = input_object();
		if (head == NULL)
			head = op;
		else
			last->next = op;
		last = op;
	}

	return head;
}

/*
 * Read a cinfo database file and build up the object tree.
 */
struct object_s *
read_cinfodb_stream(FILE *_cinfodb, char *filename)
{
	struct object_s *op;
	char header[] = { CINFODB_HEADER };
	char buf[CINFODB_HEADER_SIZE];

	cinfodb = _cinfodb;

	/*
	 * Read and check for consistency the file header.
	 */
	if (fread(buf, sizeof(header), 1, cinfodb) != 1)
		errx(1, "%s: unexpected end of file", filename);
	if (memcmp(header, buf, 6) != 0)
		errx(1, "%s: unrecognized file format", filename);
	if (header[6] != buf[6] || header[7] != buf[7])
		errx(1, "%s: file version doesn't match", filename);

	/*
	 * Build up the object tree.  The other functions need to know
	 * the filename when they output the error messages, so we must
	 * use a global filename string.
	 */
	cinfodb_filename = xstrdup(filename);
	op = input_object_list();
	free(cinfodb_filename);

	/*
	 * Check if we are at the end of the file then close the file.
	 */
	check_tail();
	fclose(cinfodb);

	return op;
}

/*
 * Read a cinfo database file and build up the object tree.
 */
struct object_s *
read_cinfodb(char *filename)
{
	FILE *_cinfodb;

	if ((_cinfodb = fopen(filename, "r")) == NULL)
		err(1, "%s", filename);

	return (read_cinfodb_stream(_cinfodb, filename));
}
