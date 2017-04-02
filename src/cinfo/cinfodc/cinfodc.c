/*	$Id: cinfodc.c,v 1.25 1997/08/30 01:14:26 sandro Exp $	*/

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

static char *rcsid = "$Id: cinfodc.c,v 1.25 1997/08/30 01:14:26 sandro Exp $";

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <err.h>

#include "config.h"
#include "cinfodb.h"

static FILE *output_file = stdout;
static int opt_header;

static void
pspaces(int n)
{
	int i;
	for (i = 0; i < n; i++)
		fprintf(output_file, " ");
}

static void
output_variable(struct variable_s *p, int level)
{
	for (; p != NULL; p = p->next) {
		pspaces(level);
		fprintf(output_file, "%s = \"%s\"\n", p->name, p->value);
	}
}

static char *
get_short_description(struct variable_s *p)
{
	for (; p != NULL; p = p->next)
		if (!strcmp(p->name, "short_description"))
			return p->value;
	return NULL;
}

static void
output_object(struct object_s *p, int level)
{
	static int first_obj = 1;
	char *s;

	for (; p != NULL; p = p->next) {
		if (opt_header && p->object_list != NULL) {
			if (!first_obj) {
				if (level <= 4)
					fprintf(output_file, "\f\n");
				else
					fprintf(output_file, "\n");
			}
			fprintf(output_file, "# %s", p->name);
			if ((s = get_short_description(p->variable_list)) != NULL)
				fprintf(output_file, " - %s", s);
			fprintf(output_file, "\n\n");
		}
		first_obj = 0;
		pspaces(level);
		fprintf(output_file, "(\"%s\"\n", p->name);
			if (p->variable_list != NULL)
				output_variable(p->variable_list, level + 4);
			if (p->object_list != NULL)
				output_object(p->object_list, level + 4);
		pspaces(level);
		fprintf(output_file, ")\n");
	}
}

/*
 * Output the program syntax then exit.
 */
static void
usage(void)
{
	fprintf(stderr, "usage: cinfodc [-hV] [-o file] file ...\n");
	exit(1);
}

int
main(int argc, char **argv)
{
	int c;
	struct object_s *op;

	while ((c = getopt(argc, argv, "ho:V")) != -1)
		switch (c) {
		case 'h':
			opt_header = 1;
			break;
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
		usage();

	while (*argv) {
		op = read_cinfodb(*argv++);
		output_object(op, 0);
		free_object_list(op);
	}

	if (output_file != stdout)
		fclose(output_file);

	return 0;
}
