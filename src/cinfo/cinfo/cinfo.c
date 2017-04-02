/*	$Id: cinfo.c,v 1.47 1997/08/30 01:14:20 sandro Exp $	*/

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

static char *rcsid = "$Id: cinfo.c,v 1.47 1997/08/30 01:14:20 sandro Exp $";

#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <err.h>

#include "config.h"
#include "cinfodb.h"

/*
 * The variables where are stored the options flags.
 */
static int opt_case;
static int opt_debug;
static int opt_html;
static int opt_list;
static int opt_pattern;
static int opt_cinfodb_directory;
static char *opt_cinfodb_directory_arg;

static FILE *output_file = stdout;

static int
is_cinfodb_filename(char *filename)
{
	int len = strlen(filename);
	if (len > 9) {
		if (!strcmp(filename + len - 9, ".cinfo.db"))
			return 1;
		if (len > 12 && !strcmp(filename + len - 12, ".cinfo.db.gz"))
			return 2;
	}
	return 0;
}

static struct object_s *
make_object_tree_entry(char *dbdir, char *entry)
{
	struct object_s *op;
	char *p, *cmd;
	int type;

	if ((type = is_cinfodb_filename(entry))) {
		if (opt_debug)
			printf("ok\n");
		p = (char *)xmalloc(strlen(dbdir) + strlen(entry) + 2);
		strcpy(p, dbdir);
		strcat(p, "/");
		strcat(p, entry);
		if (opt_debug)
			printf("loading %s...", p);
		if (type == 2) {
			FILE *fin;
			cmd = (char *)xmalloc(strlen(p) + 10);
			strcpy(cmd, "gzip -dc ");
			strcat(cmd, p);
			if ((fin = popen(cmd, "r")) == NULL)
				err(1, "%s", cmd);
			op = read_cinfodb_stream(fin, p);
			pclose(fin);
			free(cmd);
		} else
			op = read_cinfodb(p);
		free(p);
		if (opt_debug)
			printf(" done\n");
		return op;
	}
	if (opt_debug)
		printf("no\n");
	return NULL;
}

static struct object_s *
make_object_tree(char *dbdir)
{
	DIR *dir;
	struct dirent *de;
	struct object_s *op, *head = NULL, *last;
	int dbcounter = 0;

	if (opt_debug)
		printf("scanning directory %s\n", dbdir);

	if ((dir = opendir(dbdir)) == NULL)
		err(1, "%s", dbdir);
	while ((de = readdir(dir)) != NULL) {
		if (opt_debug)
			printf("checking %s -> ", de->d_name);
		if ((op = make_object_tree_entry(dbdir, de->d_name)) != NULL) {
			dbcounter++;
			if (head == NULL)
				head = op;
			else
				last->next = op;
			last = op;
		}
	}
	closedir(dir);

	if (dbcounter == 0)
		errx(1, "%s: no cinfo database file found", dbdir);

	return head;
}

struct string_s {
	char *s;
	struct string_s *prev;
	struct string_s *next;
};

static struct string_s *string_stack_head;
static struct string_s *string_stack_tail;

static void
string_push(char *s)
{
	struct string_s *sp;

	sp = (struct string_s *)xmalloc(sizeof(struct string_s));
	sp->s = xstrdup(s);

	sp->next = NULL;
	sp->prev = NULL;
	
	if (string_stack_head == NULL)
		string_stack_head = sp;
	else {
		string_stack_tail->next = sp;
		sp->prev = string_stack_tail;
	}
	string_stack_tail = sp;
}

static void
string_pop(void)
{
	struct string_s *sp;
	free(string_stack_tail->s);
	if (string_stack_head == string_stack_tail) {
		free(string_stack_head);
		string_stack_head = string_stack_tail = NULL;
	} else {
		sp = string_stack_tail->prev;
		free(string_stack_tail);
		sp->next = NULL;
		string_stack_tail = sp;
	}
}

static void
output_string_stack_tree(void)
{
	struct string_s *sp = string_stack_head;
	fprintf(output_file, "[");
	for (; sp != NULL; sp = sp->next) {
		if (sp->prev != NULL)
			fprintf(output_file, " -> ");
		if (opt_html)
			fprintf(output_file, "<I>%s</I>", sp->s);
		else
			fprintf(output_file, "%s", sp->s);
	}
	fprintf(output_file, "]\n");
}

static char *last_standard;

static void
output_container_variables(char *name, struct variable_s *p)
{
	for (; p != NULL; p = p->next) {
		if (!strcmp(p->name, "short_description")) {
			if (opt_html)
				fprintf(output_file, "<B>%s:</B> %s\n", name, p->value);
			else
				fprintf(output_file, "%s: %s\n", name, p->value);
		} else if (!strcmp(p->name, "long_description")) {
				fprintf(output_file, "%s\n", p->value);
		} else if (!strcmp(p->name, "standard")) {
			if (opt_html)
				fprintf(output_file, "\n<B>Standard%s%s\n", strchr(last_standard, '\n') == NULL ? ":</B> " : "s:</B>\n", p->value);
			else
				fprintf(output_file, "\nStandard%s%s\n", strchr(last_standard, '\n') == NULL ? ": " : "s:\n", p->value);
		} else {
			if (opt_html)
				fprintf(output_file, "<B>%s:</B> %s\n", p->name, p->value);
			else
				fprintf(output_file, "%s: %s\n", p->name, p->value);
		}
	}
}

static char *
get_standard_variable(struct variable_s *p)
{
	for (; p != NULL; p = p->next)
		if (!strcmp(p->name, "standard"))
			return p->value;
	return NULL;
}

static void
output_symbol_variables(char *name, struct variable_s *p)
{
	char *flags = NULL, *prototype = NULL;
	char *line = NULL, *remarks = NULL, *example = NULL;
	int noheader = 0;

	for (; p != NULL; p = p->next) {
		if (!strcmp(p->name, "header") && !strcmp(p->value, "no"))
			noheader = 1;
		else if (strlen(p->name) == 1)
			switch (*p->name) {
			case 'd':
				prototype = p->value;
				break;
			case 'f':
				flags = p->value;
				break;
			case 'l':
				line = p->value;
				break;
			case 'r':
				remarks = p->value;
				break;
			case 'e':
				example = p->value;
				break;
			case 't':
				fprintf(output_file, "%s\n", p->value);
				break;
			default:
				warnx("invalid variable `%s'", p->name);
			}
		else {
			if (opt_html)
				fprintf(output_file, "<B>%s:</B> %s\n", p->name, p->value);
			else
				fprintf(output_file, "%s: %s\n", p->name, p->value);
		}
	}

	if (flags != NULL) {
		int comma = 0;
		if (opt_html)
			fprintf(output_file, "\n<B>Flags:</B> %s (", flags);
		else
			fprintf(output_file, "\nFlags: %s (", flags);
		
		while (*flags) {
			char *flag;
			if (comma)
				fprintf(output_file, ", ");
			switch (*flags++) {
			case 'b': flag = "builtin"; noheader = 1; break;
			case 'f': flag = "function"; break;
			case 'l': flag = "lvalue"; break;
			case 'm': flag = "macro"; break;
			case 's': flag = "struct"; break;
			case 't': flag = "type"; break;
			case 'u': flag = "union"; break;
			default: flag = "?";
			}
			comma = 1;
			if (opt_html)
				fprintf(output_file, "<I>%s</I>", flag);
			else
				fprintf(output_file, "%s", flag);
		}
		fprintf(output_file, ")\n");
	}

	if (line != NULL)
		fprintf(output_file, "\n%s\n", line);

	if (prototype != NULL) {
		if (opt_html)
			fprintf(output_file, "\n<B>Declaration:</B>\n");
		else
			fprintf(output_file, "\nDeclaration:\n");
		if (!noheader)
			if (opt_html)
				fprintf(output_file, "<I>#include &lt;%s&gt;</I>\n\n", string_stack_tail->prev->s);
			else
				fprintf(output_file, "#include <%s>\n\n", string_stack_tail->prev->s);
		fprintf(output_file, "%s\n", prototype);
	}

	if (remarks != NULL) {
		if (opt_html)
			fprintf(output_file, "\n<B>Remarks:</B>\n");
		else
			fprintf(output_file, "\nRemarks:\n");
		fprintf(output_file, "%s\n", remarks);
	}

	if (example != NULL) {
		if (opt_html)
			fprintf(output_file, "\n<B>Example:</B>\n");
		else
			fprintf(output_file, "\nExample:\n");
		fprintf(output_file, "%s\n", example);
	}

	if (last_standard != NULL)
		if (opt_html)
			fprintf(output_file, "\n<B>Standard%s%s\n",
				strchr(last_standard, '\n') == NULL ? ":</B> " : "s:</B>\n",
				last_standard);
		else
			fprintf(output_file, "\nStandard%s%s\n",
				strchr(last_standard, '\n') == NULL ? ": " : "s:\n",
				last_standard);
}

static int
pattern_match(const char *buffer, const char *pattern)
{
	int i;

	if ((*pattern == '\0') && (*buffer == '\0'))
		return 1;

	if (*pattern == '\0')
		return 0;

	if (*pattern == '*') {
		if (*(pattern + 1) == '\0')
			return 1;

		for (i = 0; i <= strlen(buffer); i++)
			if (((*(buffer + i) == *(pattern + 1)) ||
			    (*(pattern + 1) == '?')) &&
			    (pattern_match(buffer + i + 1, pattern + 2) == 1))
				return 1;
    	} else {
		if (*buffer == '\0')
			return 0;

		if (((*pattern == '?') || (*pattern == *buffer)) &&
		    (pattern_match(buffer + 1, pattern + 1) == 1))
			return 1;
	}

	return 0;
}

static int
_user_cmp(char *s1, char *s2)
{
	if (opt_pattern)
		return (pattern_match(s1, s2));
	else
		return (!strcmp(s1, s2));
}

static char *
strlower(char *s)
{
	char *p = s;
	for (; *p != '\0'; p++)
		*p = tolower(*p);
	return s;
}

static int
user_cmp(char *s1, char *s2)
{
	if (opt_case) {
		int retval;
		char *ps1, *ps2;
		ps1 = xstrdup(s1);
		ps2 = xstrdup(s2);
		strlower(ps1);
		strlower(ps2);
		retval = _user_cmp(ps1, ps2);
		free(ps1);
		free(ps2);
		return retval;
	} else
		return (_user_cmp(s1, s2));
}

static void
search_symbol(struct object_s *object_tree, char *symbol)
{
	struct object_s *p = object_tree;

	for (; p != NULL; p = p->next) {
		string_push(p->name);
		if (p->object_list != NULL) {
			last_standard = get_standard_variable(p->variable_list);
			if (user_cmp(p->name, symbol)) {
				output_string_stack_tree();
				if (!opt_list)
					output_container_variables(p->name, p->variable_list);
			}
			search_symbol(p->object_list, symbol);
		} else if (user_cmp(p->name, symbol)) {
			output_string_stack_tree();
			if (!opt_list)
				output_symbol_variables(p->name, p->variable_list);
		}
		string_pop();
	}
}

/*
 * Output the program syntax then exit.
 */
static void
usage(void)
{
	fprintf(stderr, "usage: cinfo [-cdhlpsV] [-o file] [-L dir] symbol ...\n");
	exit(1);
}

int
main(int argc, char **argv)
{
	int c;
	struct object_s *op;

	while ((c = getopt(argc, argv, "cdhlo:psL:V")) != -1)
		switch (c) {
		case 'c':
			opt_case = 1;
			break;
		case 'd':
			opt_debug = 1;
			break;
		case 'h':
			opt_html = 1;
			break;
		case 'l':
			opt_list = 1;
			break;
		case 'o':
			if (output_file != stdout)
				fclose(output_file);
			if ((output_file = fopen(optarg, "w")) == NULL)
				err(1, "%s", optarg);
			break;
		case 'p':
			opt_pattern = 1;
			break;
		case 's':
			opt_pattern = 0;
			break;
		case 'L':
			opt_cinfodb_directory = 1;
			opt_cinfodb_directory_arg = optarg;
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

	if (!opt_cinfodb_directory) {
		char *p;
		if ((p = getenv("CINFODBDIR")) != NULL)
			op = make_object_tree(p);
		else
			op = make_object_tree(_PATH_CINFODB);
	} else
		op = make_object_tree(opt_cinfodb_directory_arg);

	if (opt_html)
		fprintf(output_file, "<PRE>\n");

	while (*argv)
		search_symbol(op, *argv++);

	if (opt_html)
		fprintf(output_file, "</PRE>\n");

	if (output_file != stdout)
		fclose(output_file);
	free_object_list(op);

	return 0;
}
