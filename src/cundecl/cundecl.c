/*	$Id: cundecl.c,v 1.12 1997/08/30 01:14:35 sandro Exp $	*/

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

static char *rcsid = "$Id: cundecl.c,v 1.12 1997/08/30 01:14:35 sandro Exp $";

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <err.h>

#include "config.h"

extern int lineno;
extern FILE *yyin;
extern int yyparse(void);

FILE *output_file = stdout;

static void
process_file(char *filename)
{
	if (filename != NULL && strcmp(filename, "-") != 0) {
		if ((yyin = fopen(filename, "r")) == NULL)
			err(1, "%s", filename);
	} else
		yyin = stdin;

	lineno = 1;
	yyparse();

	if (yyin != stdin)
		fclose(yyin);
}

/*
 * Output the program syntax then exit.
 */
static void
usage(void)
{
	fprintf(stderr, "usage: cundecl [-V] [-o file] [file ...]\n");
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
