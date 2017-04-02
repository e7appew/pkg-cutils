/*	$Id: yyref.c,v 1.10 1997/08/30 01:14:42 sandro Exp $	*/

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

static char *rcsid = "$Id: yyref.c,v 1.10 1997/08/30 01:14:42 sandro Exp $";

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <err.h>

#include "config.h"

extern int lineno;
extern FILE *yyin, *yyout;
extern int yyparse(void);
extern void nline(int ln);

FILE *output_file;

#define	MAXIDENTS	1000
#define	MAXCHARS	100
#define	MAXDEFS		20
#define	MAXOCCS		1000

struct {
	char ident[MAXCHARS];
	int desc[MAXDEFS];
	int nextdesc;
	int occ[MAXOCCS];
	int nextocc;
} table[MAXIDENTS];

static int
yyaction(char *s)
{
	int i;

	for (i = 0; strcmp(table[i].ident, s) != 0 &&
		    strcmp(table[i].ident, "") != 0; i++)
		;

	if (strcmp(table[i].ident, s) != 0) {
		strcpy(table[i].ident, s);
		table[i].nextdesc = 0;
		table[i].nextocc = 0;
	}

	return i;
}

void
on_left(char *s, int lineno)
{
	int i;
	i = yyaction(s);
	table[i].desc[table[i].nextdesc++] = lineno;
}

void
on_right(char *s, int lineno)
{
	int i;
	i = yyaction(s);
	table[i].occ[table[i].nextocc++] = lineno;
}

static void
print_refs(void)
{
	int i, ind;

	for (i = 0; strcmp(table[i].ident, "") != 0; i++) {
		fprintf(output_file, "\n%-20s ", table[i].ident);
		if (table[i].nextdesc == 0) {
			if (!strcmp(table[i].ident, "error"))
				fprintf(output_file, "    :");
			else
				fprintf(output_file, "    : never declared");
		} else {
			ind = 0;
			fprintf(output_file, "%4d:", table[i].desc[ind++]);
			for (ind = 1; ind < table[i].nextdesc; ind++)
				fprintf(output_file, " *%4d", table[i].desc[ind]);
		}
		if (table[i].occ[0] == 0)
			fprintf(output_file, " never occurs on rhs of rule - start rule?");
		else
			for (ind = 0; ind < table[i].nextocc; ind++)
				fprintf(output_file, " %4d", table[i].occ[ind]);
	}
}

static void
process_file(char *filename)
{
	if (filename != NULL && strcmp(filename, "-") != 0) {
		if ((yyin = fopen(filename, "r")) == NULL)
			err(1, "%s", filename);
	} else
		yyin = stdin;

	strcpy(table[0].ident, "");

	lineno = 1;
	nline(lineno);
	yyparse();

	if (yyin != stdin)
		fclose(yyin);

	print_refs();
}

/*
 * Output the program syntax then exit.
 */
static void
usage(void)
{
	fprintf(stderr, "usage: yyref [-V] [-o file] [file ...]\n");
	exit(1);
}

int
main(int argc, char **argv)
{
	int c;

	yyout = output_file = stdout;

	while ((c = getopt(argc, argv, "o:V")) != -1)
		switch (c) {
		case 'o':
			if (output_file != stdout)
				fclose(output_file);
			if ((output_file = fopen(optarg, "w")) == NULL)
				err(1, "%s", optarg);
			yyout = output_file;
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
