/*	$Id: ctangle.c,v 1.10 1997/06/12 16:22:06 sandro Exp $	*/

/*
 * Copyright (c) 1997
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

static char *rcsid = "$Id: ctangle.c,v 1.10 1997/06/12 16:22:06 sandro Exp $";

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <err.h>

#include "config.h"

static FILE *input_file = stdin;
static FILE *output_file = stdout;
static char *input_filename;

static int lineno;

static int xbuf[16];
static int *xptr;
#define xgetc()		((xptr > xbuf) ? *--xptr : fgetc(input_file))
#define xungetc(c)	(*xptr++ = c)
#define xputc(c)	fputc(c, output_file)

static void
do_code(void)
{
	int c, c1;

	fprintf(output_file, "#line %d \"%s\"\n", lineno+1, input_filename);

	while (isspace(c = xgetc()) && c != '\n')
		;
	if (c == '\n')
		lineno++;
	while ((c = xgetc()) != EOF)
		if (c == '@') {
			if ((c1 = xgetc()) == '@')
				break;
			else {
				if (c1 == '\n')
					lineno++;
				xputc(c);
				xputc(c1);
			}
		} else {
			if (c == '\n')
				lineno++;
			xputc(c);
		}
}

static void
do_tangle(void)
{
	int c, c1;

	while ((c = xgetc()) != EOF)
		if (c == '@') {
			if ((c1 = xgetc()) == '@')
				do_code();
			else
				xungetc(c1);
		} else if (c == '\n')
			lineno++;
}

static void
process_file(char *filename)
{
	if (filename != NULL && strcmp(filename, "-") != 0) {
		if ((input_file = fopen(filename, "r")) == NULL)
			err(1, "%s", filename);
		input_filename = filename;
	} else {
		input_file = stdin;
		input_filename = "<stdin>";
	}

	lineno = 1;
	xptr = xbuf;
	do_tangle();

	if (input_file != stdin)
		fclose(input_file);
}

/*
 * Output the program syntax then exit.
 */
static void
usage(void)
{
	fprintf(stderr, "usage: ctangle [-V] [-o file] [file ...]\n");
	exit(1);
}

int
main(int argc, char **argv)
{
	int c;

	while ((c = getopt(argc, argv, "o:V")) != -1)
		switch (c) {
		case 'o':
			if (output_file != stdout)
				fclose(output_file);
			if ((output_file = fopen(optarg, "w")) == NULL)
				err(1, "%s", optarg);
			break;
		case 'V':
			fprintf(stderr, "%s - %s\n", CUTILS_VERSION, rcsid);
			exit(0);
		case '?':
		default:
			usage();
			/* NOTREACHED */
		}
	argc -= optind;
	argv += optind;

	if (argc < 1)
		process_file(NULL);
	else
		while (*argv)
			process_file(*argv++);

	return 0;
}
