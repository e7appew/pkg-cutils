/*	$Id: cunloop.c,v 1.7 1997/07/01 22:41:13 sandro Exp $	*/

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

static char *rcsid = "$Id: cunloop.c,v 1.7 1997/07/01 22:41:13 sandro Exp $";

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <err.h>

#include "config.h"
#include "tokens.h"

#define DEFAULT_PREFIX	"l_"

/* From lexer.c */
#ifdef YYTEXT_POINTER
extern char *yytext;
#else
extern char yytext[];
#endif
extern char *token_buffer;
extern FILE *yyin;
extern int yylex(void);
extern void init_lex(void);
extern void done_lex(void);

static int lookahead;
static FILE *output_file;

static int opt_prefix;			/* Indentifier prefix option. */
static char *opt_prefix_arg;		/* Option argument. */

#define next_token()	(lookahead = yylex())
#define outstr(s)	fputs(s, output_file)
#define outch(c)	fputc(c, output_file)

static void
outtk(int tk)
{
	switch (tk) {
	case COMMENT:
	case STRING:
	case DIRECTIVE:
		outstr(token_buffer);
		break;
	case KW_BREAK:
	case KW_CONTINUE:
	case KW_DO:
	case KW_ELSE:
	case KW_FOR:
	case KW_IF:
	case KW_SWITCH:
	case KW_WHILE:
	case KEYWORD:
	case IDENTIFIER:
	case CONSTANT:
	case CHARACTER:
	case OPERATOR:
		outstr(yytext);
		break;
	default:
		outch(tk);
	}
}

static int label_counter;
static int label_continue;
static int label_break;

static int parse_until(int untiltk);

static void
do_while(void)
{
	int label_c = ++label_counter;
	int label_b = ++label_counter;
	int save_label_c = label_continue;
	int save_label_b = label_break;

	label_continue = label_c;
	label_break = label_b;

	while (lookahead != '(')
		next_token();
	next_token();

	outstr("{\n");
	fprintf(output_file, "%s%d:\n", opt_prefix_arg, label_c);
	outstr("if (!(");
	parse_until(UNTIL_CLOSEPAREN);
	outstr(")\n");
	fprintf(output_file, "goto %s%d;\n", opt_prefix_arg, label_b);
	parse_until(UNTIL_ENDOFINSTR);
	fprintf(output_file, "goto %s%d;\n", opt_prefix_arg, label_c);
	fprintf(output_file, "%s%d:;\n", opt_prefix_arg, label_b);
	outstr("}\n");

	label_continue = save_label_c;
	label_break = save_label_b;
}

static void
do_do(void)
{
	int label_c = ++label_counter;
	int label_b = ++label_counter;
	int save_label_c = label_continue;
	int save_label_b = label_break;

	label_continue = label_c;
	label_break = label_b;

	outstr("{\n");
	fprintf(output_file, "%s%d:\n", opt_prefix_arg, label_c);
	parse_until(UNTIL_ENDOFINSTR);

	while (lookahead != '(')
		next_token();
	next_token();

        outstr("if (");
	parse_until(UNTIL_CLOSEPAREN);
	fprintf(output_file, "\ngoto %s%d;\n", opt_prefix_arg, label_c);
	fprintf(output_file, "%s%d:;\n", opt_prefix_arg, label_b);
	outstr("}\n");

	label_continue = save_label_c;
	label_break = save_label_b;
}

static void
do_for(void)
{
	int label_l = ++label_counter;
	int label_c = ++label_counter;
	int label_b = ++label_counter;
	int label_i = ++label_counter;
	int save_label_c = label_continue;
	int save_label_b = label_break;

	label_continue = label_c;
	label_break = label_b;

	outstr("{\n");

	while (lookahead != '(')
		next_token();
	next_token();

	parse_until(UNTIL_ENDOFINSTR);

	fprintf(output_file, "%s%d:\n", opt_prefix_arg, label_l);
	outstr("if (!(");
	if (!parse_until(UNTIL_ENDOFINSTR_NOECHO))
		fprintf(output_file, "1");
	outstr("))\n");
	fprintf(output_file, "goto %s%d;\n", opt_prefix_arg, label_b);
	fprintf(output_file, "goto %s%d;\n", opt_prefix_arg, label_i);
	fprintf(output_file, "%s%d:", opt_prefix_arg, label_c);
	parse_until(UNTIL_CLOSEPAREN_NOECHO);
	outstr(";\n");
	fprintf(output_file, "goto %s%d;\n", opt_prefix_arg, label_l);
	fprintf(output_file, "%s%d:\n", opt_prefix_arg, label_i);
	parse_until(UNTIL_ENDOFINSTR);
	fprintf(output_file, "goto %s%d;\n", opt_prefix_arg, label_c);
	fprintf(output_file, "%s%d:;\n", opt_prefix_arg, label_b);
	outstr("}\n");

	label_continue = save_label_c;
	label_break = save_label_b;
}

static void
do_switch(void)
{
	int save_label_b = label_break;

	label_break = 0;

	outstr("switch");
	parse_until(UNTIL_ENDOFINSTR);

	label_break = save_label_b;
}

static void
do_if(void)
{
	int label_l = ++label_counter;
	int label_e;

	while (lookahead != '(')
		next_token();
	next_token();

	outstr("{\n");
	outstr("if (!(");
	parse_until(UNTIL_CLOSEPAREN);
	outstr(")\n");
	fprintf(output_file, "goto %s%d;\n", opt_prefix_arg, label_l);

	parse_until(UNTIL_ENDOFINSTR);

	while (isspace(lookahead))
		next_token();
	if (lookahead == KW_ELSE) {
		label_e = ++label_counter;
		fprintf(output_file, "goto %s%d;\n", opt_prefix_arg, label_e);
	}
	fprintf(output_file, "%s%d:;\n", opt_prefix_arg, label_l);
	if (lookahead == KW_ELSE) {
		next_token();
		parse_until(UNTIL_ENDOFINSTR);
		fprintf(output_file, "%s%d:;\n", opt_prefix_arg, label_e);
	}

	outstr("}\n");
}

/*
 * The main parsing function.
 */
static int
parse_until(int untiltk)
{
	int nparens = 0, nblocks = 0;
	int isexpr = 0;

	if (untiltk == UNTIL_CLOSEPAREN || untiltk == UNTIL_CLOSEPAREN_NOECHO)
		nparens++;

	while (lookahead != 0)
		switch (lookahead) {
		case '(':
			next_token();
			isexpr = 1;
			if (nblocks == 0)
				nparens++;
			outch('(');
			break;
		case ')':
			next_token();
			isexpr = 1;
			if (nblocks == 0)
				nparens--;
			if (untiltk == UNTIL_CLOSEPAREN_NOECHO && nparens == 0)
				return isexpr;
			outch(')');
			if (untiltk == UNTIL_CLOSEPAREN && nparens == 0)
				return isexpr;
			break;
		case '{':
			next_token();
			isexpr = 1;
			nblocks++;
			outch('{');
			break;
		case '}':
			next_token();
			isexpr = 1;
			nblocks--;
			outch('}');
			if (untiltk == UNTIL_ENDOFINSTR && nblocks == 0)
				return isexpr;
			break;
		case ';':
			next_token();
			if (untiltk == UNTIL_ENDOFINSTR_NOECHO && nblocks == 0)
				return isexpr;
			outch(';');
			if (untiltk == UNTIL_ENDOFINSTR && nblocks == 0)
				return isexpr;
			break;
		case KW_DO:
			next_token();
			do_do();
			if (untiltk == UNTIL_ENDOFINSTR && nblocks == 0)
				return isexpr;
			break;
		case KW_WHILE:
			next_token();
			do_while();
			if (untiltk == UNTIL_ENDOFINSTR && nblocks == 0)
				return isexpr;
			break;
		case KW_FOR:
			next_token();
			do_for();
			if (untiltk == UNTIL_ENDOFINSTR && nblocks == 0)
				return isexpr;
			break;
		case KW_SWITCH:
			next_token();
			do_switch();
			if (untiltk == UNTIL_ENDOFINSTR && nblocks == 0)
				return isexpr;
			break;
		case KW_IF:
			next_token();
			do_if();
			if (untiltk == UNTIL_ENDOFINSTR && nblocks == 0)
				return isexpr;
			break;
		case KW_BREAK:
			next_token();
			if (label_break > 0)
				fprintf(output_file, "goto %s%d", opt_prefix_arg, label_break);
			else
				outstr("break");
			break;
		case KW_CONTINUE:
			next_token();
			if (label_continue > 0)
				fprintf(output_file, "goto %s%d", opt_prefix_arg, label_continue);
			else
				outstr("continue");
			break;
		default:
			if (!isspace(lookahead))
				isexpr = 1;
			outtk(lookahead);
			next_token();
		}

	return isexpr;
}

static void
parse(void)
{
	next_token();
	parse_until(0);
}

static void
process_file(char *filename)
{
	if (filename != NULL && strcmp(filename, "-") != 0) {
		if ((yyin = fopen(filename, "r")) == NULL)
			err(1, "%s", filename);
	} else
		yyin = stdin;

	init_lex();
	label_continue = label_break = label_counter = 0;
	parse();
	done_lex();

	if (yyin != stdin)
		fclose(yyin);
}

/*
 * Output the program syntax then exit.
 */
static void
usage(void)
{
	fprintf(stderr, "usage: cunroll [-V] [-o file] [-p prefix] [file ...]\n");
	exit(1);
}

int
main(int argc, char **argv)
{
	int c;
        output_file = stdout;

	while ((c = getopt(argc, argv, "Vo:p:")) != -1)
		switch (c) {
		case 'o':
			if (output_file != stdout)
				fclose(output_file);
			if ((output_file = fopen(optarg, "w")) == NULL)
				err(1, "%s", optarg);
			break;
		case 'p':
			opt_prefix = 1;
			opt_prefix_arg = optarg;
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

	if (!opt_prefix)
		opt_prefix_arg = DEFAULT_PREFIX;

	if (argc < 1)
		process_file(NULL);
	else
		while (*argv)
			process_file(*argv++);

	if (output_file != stdout)
		fclose(output_file);

	return 0;
}
