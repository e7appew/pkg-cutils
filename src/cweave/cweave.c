/*	$Id: cweave.c,v 1.13 1997/07/01 13:12:24 sandro Exp $	*/

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

static char *rcsid = "$Id: cweave.c,v 1.13 1997/07/01 13:12:24 sandro Exp $";

#ifndef _PATH_ETCDIR
#define _PATH_ETCDIR	"/usr/share/cutils"
#endif

#define CWEBMACFILE	_PATH_ETCDIR "/cweb.mac"

#define MAX_REF_SIZE	256

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <err.h>

#include "config.h"
#include "tokens.h"

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

static int opt_hilight;
static int opt_tab_width = 8;

static FILE *input_file = stdin;
static FILE *output_file = stdout;

static int xbuf[16];
static int *xptr;

static int col;

void
xungetc(int c)
{
	*xptr++ = c;
}

int
xgetc(void)
{
	int c, i;
	if (xptr > xbuf)
		return *--xptr;
	c = fgetc(input_file);

	if (c == '\t') {
		for (i = opt_tab_width - col % opt_tab_width; i > 1; i--)
			xungetc(' '), col++;
		col++;
		return ' ';
	} else if (c == '\n') {
		col = 0;
		return '\n';
	} else {
		col++;
		return c;
	}
}

static int lastch;
static int spbuf;

static void
xputc(int c)
{
	if (!opt_hilight) {
		fputc(c, output_file);
		return;
	}

	if (c == ' ') {
		spbuf++;
		return;
	} else if (spbuf > 0) {
		fprintf(output_file, "\\hspace*{%d\\indentation}", spbuf);
		spbuf = 0;
	}

	switch (c) {
	case '#':
	case '$':
	case '%':
	case '&':
	case '_':
	case '{':
	case '}':
		fputc('\\', output_file);
		fputc(c, output_file);
		break;
	case '|':
		fprintf(output_file, "$\\vert$");
		break;
	case '^':
		fprintf(output_file, "$\\wedge$");
		break;
	case '<':
		fprintf(output_file, "$<$");
		break;
	case '>':
		fprintf(output_file, "$>$");
		break;
	case '~':
		fprintf(output_file, "$\\sim$");
		break;
	case '\\':
		fprintf(output_file, "$\\backslash$");
		break;
	case '*':
		fprintf(output_file, "$\\ast$");
		break;
	case '\n':
		fprintf(output_file, "\\mbox{}\\\\\n");
		break;
	default:
		fputc(c, output_file);
	}
	lastch = c;
}

static void
xputstr(char *s)
{
	for (; *s != '\0'; s++)
		xputc(*s);
}

static void
do_code(void)
{
	int c, c1;

	fprintf(output_file, "\\begin{verbatim}\n");
	while (isspace(c = xgetc()) && c != '\n')
		;
	while ((c = xgetc()) != EOF)
		if (c == '@') {
			if ((c1 = xgetc()) == '@')
				break;
			else {
				xputc(c);
				xungetc(c1);
			}
		} else
			xputc(c);
	while (isspace(c = xgetc()) && c != '\n')
		;
	fprintf(output_file, "\\end{verbatim}\n");
}

static void
do_code_hilight(void)
{
	int c;

	fprintf(output_file, "\\begin{flushleft}\n");
	while (isspace(c = xgetc()) && c != '\n')
		;
	yyin = input_file;
	init_lex();
	while ((c = yylex()) != 0 && c != ENDOFCODE)
		switch (c) {
		case STRING:
		case DIRECTIVE:
			fprintf(output_file, "\\texttt{");
			xputstr(token_buffer);
			fprintf(output_file, "}");
			break;
		case CHARACTER:
		case OPERATOR:
		case CONSTANT:
		case KEYWORD:
			fprintf(output_file, "\\texttt{");
			xputstr(yytext);
			fprintf(output_file, "}");
			break;
		case COMMENT:
			fprintf(output_file, "\\texttt{\\textit{");
			xputstr(token_buffer);
			fprintf(output_file, "}}");
			break;
		case IDENTIFIER:
			fprintf(output_file, "\\texttt{\\textit{");
			xputstr(yytext);
			fprintf(output_file, "}}");
			break;
		default:
			if (c != '\n' && c != ' ') {
				fprintf(output_file, "\\texttt{");
				xputc(c);
				fprintf(output_file, "}");
			} else
				xputc(c);
		}
	done_lex();
	while (isspace(c = xgetc()) && c != '\n')
		;
	fprintf(output_file, "\\end{flushleft}\n");
}

static int
match(char *s)
{
	int c[16], *cp = c;
	while (*s != '\0') {
		*cp = xgetc();
		if (*s != *cp) {
			for (; cp >= c; cp--)
				xungetc(*cp);
			return 0;
		}
		s++; cp++;
	}
	return 1;
}

static char *
make_label(char *s)
{
	static char buf[MAX_REF_SIZE];
	char *p;

	strcpy(buf, "web:"); p = buf + 4;

	for (; *s != '\0'; s++)
		if (isalnum(*s))
			*p++ = *s;
		else
			*p++ = '-';
	*p = '\0';

	return buf;
}

#define MIN_DECLS	64

static char **declvec;
static int declsmax;
static int decli;

static int
isdecl(char *s)
{
	int i;

	if (declvec == NULL) {
		declsmax = MIN_DECLS;
		declvec = (char **)xmalloc(declsmax * sizeof(char *));
		decli = 0;
	}

	for (i = 0; i < decli; i++)
		if (!strcmp(s, declvec[i]))
			return 1;
	if (decli >= declsmax - 1) {
		declsmax += 20;
		declvec = (char **)xrealloc(declvec, declsmax * sizeof(char *));
	}

	declvec[decli] = xstrdup(s);
	decli++;

	return 0;
}

static void
put_label(char *label)
{
	fprintf(output_file, "\\label{%s}\n", label);
}

static void
put_webdecl(char *decl, char *label)
{
	fprintf(output_file, "\\webdecl{%s}{\\ref{%s}}\n", decl, label);
}

static void
put_webadecl(char *decl, char *label)
{
	fprintf(output_file, "\\webadecl{%s}{\\ref{%s}}\n", decl, label);
}

static void
put_webref(char *ref, char *label)
{
	fprintf(output_file, "\\webref{%s}{\\ref{%s}}\n", ref, label);
}

static void
do_decl(void)
{
	int c;
	char buf[MAX_REF_SIZE], *p = buf, *label;

	while (isspace(c = xgetc()))
		;
	if (c != '[')
		errx(1, "expected `['");
	while ((c = xgetc()) != ']' && c != EOF) {
		*p++ = c;
	}
	*p = '\0';
	while (isspace(c = xgetc()) && c != '\n')
		;

	label = make_label(buf);

	if (isdecl(buf))
		put_webadecl(buf, label);
	else {
		put_label(label);
		put_webdecl(buf, label);
	}
}

static void
do_ref(void)
{
	int c;
	char buf[MAX_REF_SIZE], *p = buf, *label;

	while (isspace(c = xgetc()))
		;
	if (c != '[')
		errx(1, "expected `['");
	while ((c = xgetc()) != ']' && c != EOF) {
		*p++ = c;
	}
	*p = '\0';
	while (isspace(c = xgetc()) && c != '\n')
		;

	label = make_label(buf);

	put_webref(buf, label);
}

static void
do_weave(void)
{
	int c, c1;
	int last = 0;

	while ((c = xgetc()) != EOF)
		if (c == '@') {
			if ((c1 = xgetc()) == '@') {
				if (opt_hilight)
					do_code_hilight();
				else
					do_code();
				last = 0;
			} else if (c1 == 'd') {
				if (match("ecl")) {
					if (last)
						fprintf(output_file, "\\\\\n");
					do_decl();
					last = 1;
				} else
					xungetc(c1);
			} else if (c1 == 'r') {
				if (match("ef")) {
					if (last)
						fprintf(output_file, "\\\\\n");
					do_ref();
					last = 1;
				} else
					xungetc(c1); 
			} else {
				fputc(c, output_file);
				xungetc(c1);
				last = 0;
			}
		} else {
			fputc(c, output_file);
			last = 0;
		}
}

static void
process_file(char *filename)
{
	if (filename != NULL && strcmp(filename, "-") != 0) {
		if ((input_file = fopen(filename, "r")) == NULL)
			err(1, "%s", filename);
	} else
		input_file = stdin;

	fprintf(output_file, "%% generated by cweave (%s)\n\n", CUTILS_VERSION);
	fprintf(output_file, "\\input{%s}\n\n", CWEBMACFILE);
	xptr = xbuf;
	decli = col = lastch = spbuf = 0;
	do_weave();

	if (input_file != stdin)
		fclose(input_file);
}

/*
 * Output the program syntax then exit.
 */
static void
usage(void)
{
	fprintf(stderr, "usage: cweave [-hV] [-o file] [-t width] [file ...]\n");
	exit(1);
}

int
main(int argc, char **argv)
{
	int c;

	while ((c = getopt(argc, argv, "ho:t:V")) != -1)
		switch (c) {
		case 'h':
			opt_hilight = 1;
			break;
		case 'o':
			if (output_file != stdout)
				fclose(output_file);
			if ((output_file = fopen(optarg, "w")) == NULL)
				err(1, "%s", optarg);
			break;
		case 't':
			if ((opt_tab_width = atoi(optarg)) < 1)
				opt_tab_width = 1;
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
