/*	$Id: cinfoc.c,v 1.36 1997/08/30 01:14:23 sandro Exp $	*/

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

static char *rcsid = "$Id: cinfoc.c,v 1.36 1997/08/30 01:14:23 sandro Exp $";

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <err.h>

#include "config.h"
#include "cinfodb.h"

int opt_debug = 0;

char *current_file_name;

extern struct object_s *parsing_tree;

extern void init_lex(void);
extern void done_lex(void);

extern int lineno;
extern FILE *yyin;
extern int yyparse(void);

static void
process_file(char *filename)
{
	char *buf;

	if (opt_debug)
		printf("===> opening %s\n", filename);

	if ((yyin = fopen(filename, "r")) == NULL)
		err(1, "%s", filename);
	current_file_name = xstrdup(filename);

	init_lex();
	lineno = 1;
	yyparse();
	done_lex();
	fclose(yyin);

	free(current_file_name);

	if (opt_debug)
		printf("<=== closing %s\n", filename);

	/* filename + .cinfo.db + \0 */
	buf = (char *)xmalloc(strlen(filename) + 10);
	strcpy(buf, filename);

	if (strlen(buf) > 6 && !strcmp(buf + strlen(buf) - 6, ".cinfo"))
		strcat(buf, ".db");
	else
		strcat(buf, ".cinfo.db");

	if (opt_debug)
		printf("outputting %s\n", buf);

	write_cinfodb(buf, parsing_tree);
	free_object_list(parsing_tree);
	parsing_tree = NULL;

	free(buf);
}

/*
 * Output the program syntax then exit.
 */
static void
usage(void)
{
	fprintf(stderr, "usage: cinfoc [-dV] file ...\n");
	exit(1);
}

int
main(int argc, char **argv)
{
	int c;

	while ((c = getopt(argc, argv, "dV")) != -1)
		switch (c) {
		case 'd':
			opt_debug = 1;
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
		usage();

	while (*argv)
		process_file(*argv++);

	return 0;
}
