/*	$Id: yyextract.c,v 1.39 2001/07/15 13:32:19 sandro Exp $	*/

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <err.h>

#include "config.h"
#include "parser.h"
#include "tree.h"

extern struct object_s *parsing_tree;

/* From lexer.c */
extern int lineno;
#ifdef YYTEXT_POINTER
extern char *yytext;
#else
extern char yytext[];
#endif
extern char *token_buffer;
extern FILE *yyin;
extern int yylex(void);
extern int yyparse(void);

int parser_run;

enum {
	OPT_FORMAT_YACC = 0,
	OPT_FORMAT_BNF,
	OPT_FORMAT_EBNF
};

static int opt_format;
static int opt_html;
static int opt_width;
static int opt_width_arg = 78;
static char *opt_title = NULL;

FILE *output_file;

struct token_s {
	char *token;
	struct token_s *next;
};

/*
 * This should be redone using hash tables.
 */
static struct token_s *token_list_head = NULL;
static struct token_s *token_list_tail = NULL;

void add_token(char *token)
{
	struct token_s *new;

	new = (struct token_s *)xmalloc(sizeof(struct token_s));
	new->token = xstrdup(token);

	new->next = NULL;
	if (token_list_head == NULL)
		token_list_head = token_list_tail = new;
	else {
		token_list_tail->next = new;
		token_list_tail = new;
	}
}

static int is_token(char *token)
{
	struct token_s *ptr;
	for (ptr = token_list_head; ptr != NULL; ptr = ptr->next)
		if (!strcmp(ptr->token, token))
			return 1;
	return 0;
}

static void free_token_list(void)
{
	struct token_s *ptr = token_list_head, *next;

	while (ptr != NULL) {
		next = ptr->next;
		free(ptr->token);
		free(ptr);
		ptr = next;
	}

	token_list_head = token_list_tail = NULL;
}

static void output_tokens(void)
{
	struct token_s *ptr;
	int col = 8, first = 1;

	if (token_list_head == NULL)
		return;

	fprintf(output_file, "%%token ");

	for (ptr = token_list_head; ptr != NULL; ptr = ptr->next) {
		if (strlen(ptr->token) + 1 + col >= opt_width_arg) {
			fprintf(output_file, "\n%%token ");
			col = 8;
			first = 1;
		}
		if (!first)
			fprintf(output_file, " ");
		if (opt_html)
			fprintf(output_file, "<a name=\"token_%s\"><b>%s</b></a>", ptr->token, ptr->token);
		else
			fprintf(output_file, "%s", ptr->token);
		col += strlen(ptr->token) + 1;
		first = 0;
	}

	fprintf(output_file, "\n\n");
}

static void bnf_output_tokens(void)
{
	struct token_s *ptr;
	int col = 1, first = 1;

	if (token_list_head == NULL)
		return;

	fprintf(output_file, "The terminal tokens defined in the grammar:\n");

	for (ptr = token_list_head; ptr != NULL; ptr = ptr->next) {
		if (strlen(ptr->token) + 1 + col >= opt_width_arg) {
			if (ptr != token_list_head)
				fprintf(output_file, ",");
			fprintf(output_file, "\n");
			col = 1;
			first = 1;
		}
		if (!first)
			fprintf(output_file, ", ");
		if (opt_html)
			fprintf(output_file, "<b>%s</b>", ptr->token);
		else
			fprintf(output_file, "%s", ptr->token);
		col += strlen(ptr->token) + 2;
		first = 0;
	}

	fprintf(output_file, "\n\n");
}

static void skip_section(void)
{
	int tk;

	parser_run = 0;

	while ((tk = yylex()) != 0 && tk != SECTIONSEP)
		;

	if (tk == 0)
		errx(1, "unexpected end of file in the declaration section");
}

static void output_components(struct object_s *p)
{
	struct object_s *last = NULL;
	int num = 0;

	fprintf(output_file, "\t\t");
	for (; p != NULL; p = p->next) {
		if (p->type == '|') {
			if (num == 0)
				fprintf(output_file, "/* empty */");
			fprintf(output_file, "\n\t\t| ");
			num = 0;
		} else {
			if (opt_html && *p->value != '\'') {
				if (is_token(p->value))
					fprintf(output_file, "<a href=\"#token_%s\"><b>%s</b></a> ", p->value, p->value);
				else
					fprintf(output_file, "<a href=\"#rule_%s\">%s</a> ", p->value, p->value);
			} else
				fprintf(output_file, "%s ", p->value);
			num++;
		}
		last = p;
	}
	if (last != NULL && last->type == '|' && num == 0)
		fprintf(output_file, "/* empty */");
}

static void output_tree(struct object_s *p)
{
	int num = 0;
	for (; p != NULL; p = p->next) {
		if (num > 0)
			fprintf(output_file, "\n");
		if (opt_html)
			fprintf(output_file, "<a name=\"rule_%s\">%s</a>:\n", p->value, p->value);
		else
			fprintf(output_file, "%s:\n", p->value);
		if (p->assoc != NULL) {
			output_components(p->assoc);
			fprintf(output_file, "\n");
		}
		fprintf(output_file, "\t\t;\n");
		num++;
	}
}

static void pspaces(int n)
{
	int i;
	for (i = 0; i < n; i++)
		fprintf(output_file, " ");
}


static void bnf_output_components(struct object_s *p, int spaces)
{
	struct object_s *last = NULL;
	int num = 0;

	for (; p != NULL; p = p->next) {
		if (p->type == '|') {
			if (num == 0)
				fprintf(output_file, "/* empty */");
			fprintf(output_file, "\n");
			pspaces(spaces);
/*			fprintf(output_file, "| "); */
			num = 0;
		} else {
			if (p->type == CHARACTER || p->type == STRING)
				fprintf(output_file, "%s ", p->value);
			else if (opt_html && *p->value != '\'') {
				if (is_token(p->value))
					fprintf(output_file, "&lt;<a href=\"#token_%s\">%s</a>&gt; ", p->value, p->value);
				else
					fprintf(output_file, "&lt;<a href=\"#rule_%s\">%s</a>&gt; ", p->value, p->value);
			} else
				fprintf(output_file, "<%s> ", p->value);
			num++;
		}
		last = p;
	}
	if (last != NULL && last->type == '|' && num == 0)
		fprintf(output_file, "/* empty */");
}

static void bnf_output_tree(struct object_s *p)
{
	int num = 0;
	for (; p != NULL; p = p->next) {
		if (num > 0)
			fprintf(output_file, "\n");
		if (opt_html)
			fprintf(output_file, "&lt;<a name=\"rule_%s\">%s</a>&gt; ::= ", p->value, p->value);
		else
			fprintf(output_file, "<%s> ::= ", p->value);
		if (p->assoc != NULL)
			bnf_output_components(p->assoc, strlen(p->value) + 7);
		fprintf(output_file, "\n");
		num++;
	}
}

static int bnf_find_empty(struct object_s *p)
{
	int num = 0;
	for (; p != NULL; p = p->next)
		if (p->type == '|') {
			if (num == 0)
				return 1;
			num = 0;
		} else
			num++;
	return (num == 0);
}

static void ebnf_output_components(struct object_s *p, int spaces)
{
	int empty, num = 0;

	if ((empty = bnf_find_empty(p)))
		fprintf(output_file, "[ ");

	for (; p != NULL; p = p->next) {
		if (p->type == '|') {
			if (num > 0 && p->next != NULL) {
				fprintf(output_file, "\n");
				pspaces(spaces);
/*				fprintf(output_file, "| "); */
			}
			num = 0;
		} else {
			if (p->type == CHARACTER || p->type == STRING)
				fprintf(output_file, "%s ", p->value);
			else if (opt_html && *p->value != '\'') {
				if (is_token(p->value))
					fprintf(output_file, "&lt;<a href=\"#token_%s\">%s</a>&gt; ", p->value, p->value);
				else
					fprintf(output_file, "&lt;<a href=\"#rule_%s\">%s</a>&gt; ", p->value, p->value);
			} else
				fprintf(output_file, "<%s> ", p->value);
			num++;
		}
	}
	if (empty)
		fprintf(output_file, "]");
}


static void ebnf_output_tree(struct object_s *p)
{
	int num = 0;
	for (; p != NULL; p = p->next) {
		if (num > 0)
			fprintf(output_file, "\n");
		if (opt_html)
			fprintf(output_file, "&lt;<a name=\"rule_%s\">%s</a>&gt; ::= ", p->value, p->value);
		else
			fprintf(output_file, "<%s> ::= ", p->value);
		if (p->assoc != NULL)
			ebnf_output_components(p->assoc, strlen(p->value) + 7);
		fprintf(output_file, "\n");
		num++;
	}
}

static void process_file(char *filename)
{
	if (filename != NULL && strcmp(filename, "-") != 0) {
		if ((yyin = fopen(filename, "r")) == NULL)
			err(1, "%s", filename);
	} else
		yyin = stdin;

	lineno = 1;

	skip_section();

	parser_run = 1;
	yyparse();

	if (yyin != stdin)
		fclose(yyin);

	if (parsing_tree != NULL) {
		static int html_head = 0;
		if (opt_html && !html_head) {
			fprintf(output_file, "<html>\n"
				"<head><title>%s</title></head>\n"
				"<body>\n"
				"<pre>\n", opt_title != NULL ? opt_title : "");
			html_head = 1;
		}
		if (opt_format == OPT_FORMAT_YACC) {
			output_tokens();
			output_tree(parsing_tree);
		} else {
			bnf_output_tokens();
			if (opt_format == OPT_FORMAT_BNF)
				bnf_output_tree(parsing_tree);
			else
				ebnf_output_tree(parsing_tree);
		}
		free_object_list(parsing_tree);
		free_token_list();
	} else
		errx(1, "no grammar found");
}

/*
 * Output the program syntax then exit.
 */
static void usage(void)
{
	fprintf(stderr, "usage: yyextract [-behyV] [-o file] [-t title] [-w cols] [file ...]\n");
	exit(1);
}

/*
 * Used by the err() functions.
 */
char *progname;

int main(int argc, char **argv)
{
	int c;

	progname = argv[0];
	output_file = stdout;

	while ((c = getopt(argc, argv, "beho:t:yw:V")) != -1)
		switch (c) {
		case 'b':
			opt_format = OPT_FORMAT_BNF;
			break;
		case 'e':
			opt_format = OPT_FORMAT_EBNF;
			break;
		case 'h':
			opt_html = 1;
			break;
		case 'o':
			if (output_file != stdout)
				fclose(output_file);
			if ((output_file = fopen(optarg, "w")) == NULL)
				err(1, "%s", optarg);
			break;
		case 't':
			opt_title = optarg;
			break;
		case 'w':
			opt_width = 1;
			if ((opt_width_arg = atoi(optarg)) < 10)
				opt_width_arg = 10;
			break;
		case 'y':
			opt_format = OPT_FORMAT_YACC;
			break;
		case 'V':
			fprintf(stderr, "%s\n", CUTILS_VERSION);
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

	if (opt_html) {
		time_t t = time(NULL);
		fprintf(output_file, "</pre>\n"
			"<hr>\n"
			"Generated by <a href=\"http://www.sigala.it/sandro/\">" CUTILS_VERSION "</a> yyextract - %s"
			"</body>\n", ctime(&t));
	}

	return 0;
}
