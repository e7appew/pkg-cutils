/*	$Id: chilight.c,v 1.31 1997/08/30 01:14:17 sandro Exp $	*/

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

static char *rcsid = "$Id: chilight.c,v 1.31 1997/08/30 01:14:17 sandro Exp $";

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <err.h>

#include "config.h"
#include "tokens.h"

enum {
	OPT_STYLE_ANSI_COLOR,
	OPT_STYLE_ANSI_BOLD,
	OPT_STYLE_HTML_COLOR,
	OPT_STYLE_HTML_FONT
};

int opt_directives;
static int opt_style = OPT_STYLE_ANSI_COLOR;

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

static FILE *output_file;

static struct {
	int defined;
	char *before_text;
	char *after_text;
	char *text;
} style[MAX_TOKENS];

#define MAKE_ANSI_ATTR(color)	"\033[" #color "m"

#define ANSI_ATTR_NORMAL	MAKE_ANSI_ATTR(0)
#define ANSI_ATTR_BOLD		MAKE_ANSI_ATTR(1)

#define ANSI_ATTR_BLACK		MAKE_ANSI_ATTR(30)
#define ANSI_ATTR_RED		MAKE_ANSI_ATTR(31)
#define ANSI_ATTR_GREEN		MAKE_ANSI_ATTR(32)
#define ANSI_ATTR_BROWN		MAKE_ANSI_ATTR(33)
#define ANSI_ATTR_BLUE		MAKE_ANSI_ATTR(34)
#define ANSI_ATTR_MAGENTA	MAKE_ANSI_ATTR(35)
#define ANSI_ATTR_CYAN		MAKE_ANSI_ATTR(36)
#define ANSI_ATTR_WHITE		MAKE_ANSI_ATTR(37)

#define ENTRY(tk, b, t, a)			\
	style[tk].defined = 1;		       	\
	style[tk].before_text = b;       	\
	style[tk].after_text = a;		\
	style[tk].text = t

static void
register_style(void)
{
	switch (opt_style) {
	case OPT_STYLE_ANSI_COLOR:
		ENTRY(ALL, ANSI_ATTR_WHITE, NULL, NULL);
		ENTRY(IDENTIFIER, ANSI_ATTR_GREEN, NULL, NULL);
		ENTRY(CONSTANT, ANSI_ATTR_CYAN, NULL, NULL);
		ENTRY(KEYWORD, ANSI_ATTR_MAGENTA, NULL, NULL);
		ENTRY(COMMENT, ANSI_ATTR_BLUE, NULL, NULL);
		ENTRY(DIRECTIVE, ANSI_ATTR_RED, NULL, NULL);
		ENTRY(STRING, ANSI_ATTR_BROWN, NULL, NULL);
		ENTRY(CHARACTER, ANSI_ATTR_BLUE, NULL, NULL);
		break;
	case OPT_STYLE_ANSI_BOLD:
		ENTRY(ALL, ANSI_ATTR_NORMAL, NULL, NULL);
		ENTRY(KEYWORD, ANSI_ATTR_BOLD, NULL, NULL);
		break;
	case OPT_STYLE_HTML_COLOR:
		ENTRY(KEYWORD, "<b><font color=blue>", NULL, "</font></b>");
		ENTRY(CONSTANT, "<font color=green>", NULL, "</font>");
		ENTRY(COMMENT, "<font color=cyan>", NULL, "</font>");
		ENTRY(DIRECTIVE, "<font color=red>", NULL, "</font>");
		ENTRY(STRING, "<font color=yellow>", NULL, "</font>");
		ENTRY(CHARACTER, "<font color=yellow>", NULL, "</font>");
		break;
	case OPT_STYLE_HTML_FONT:
		ENTRY(KEYWORD, "<b>", NULL, "</b>");
		ENTRY(COMMENT, "<i>", NULL, "</i>");
		ENTRY(DIRECTIVE, "<i>", NULL, "</i>");
		break;
	}
}

static char *filter_buf;
static int max_buf;

static char *
extend_filter_buf(char *p)
{
	int offset = p - filter_buf;

	max_buf = max_buf * 2 + 10;

	filter_buf = (char *)xrealloc(filter_buf, max_buf + 2);

	return filter_buf + offset;
}

static char *
filter_html_markups(char *string)
{
	char *p;

	if (filter_buf == NULL) {
		max_buf = strlen(string) + 10;
		filter_buf = (char *)xmalloc(max_buf);
	}
	p = filter_buf;
	
	while (*string != '\0') {
		if (p >= filter_buf + max_buf)
			p = extend_filter_buf(p);
		switch (*string++) {
		case '<':
			*p++ = '&'; *p++ = 'l'; *p++ = 't'; *p++ = ';';
			break;
		case '>':
			*p++ = '&'; *p++ = 'g'; *p++ = 't'; *p++ = ';';
			break;
		default:
			*p++ = *(string - 1);
		}
	}
	*p = '\0';

	return filter_buf;
}

static void
process_token(int token, char *buf)
{
	static int last_token = -1;

	if ((opt_style == OPT_STYLE_HTML_COLOR ||
	    opt_style == OPT_STYLE_HTML_FONT) && token != ALL)
	    	buf = filter_html_markups(buf);

	if (style[token].defined || token == ALL) {
		if (last_token != -1 && last_token != token &&
		    style[last_token].after_text != NULL)
			fputs(style[last_token].after_text, output_file);

		if (last_token != token && style[token].before_text != NULL)
			fputs(style[token].before_text, output_file);

		if (style[token].text != NULL)
			fputs(style[token].text, output_file);
		else
			fputs(buf, output_file);
		last_token = token;
	} else
		process_token(ALL, buf);
}

static void
parse(void)
{
	int tk;

	while ((tk = yylex()) != 0)
		switch (tk) {
		case COMMENT:
		case DIRECTIVE:
		case STRING:
			process_token(tk, token_buffer);
			break;
		default:
			process_token(tk, yytext);
			break;
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

	init_lex();
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
	fprintf(stderr, "usage: chilight [-dV] [-o file] [-s style] [file ...]\n");
	exit(1);
}

int
main(int argc, char **argv)
{
	int c;
        output_file = stdout;

	while ((c = getopt(argc, argv, "do:s:V")) != -1)
		switch (c) {
		case 'd':
			opt_directives = 1;
			break;
		case 'o':
			if (output_file != stdout)
				fclose(output_file);
			if ((output_file = fopen(optarg, "w")) == NULL)
				err(1, "%s", optarg);
			break;
		case 's':
			if (!strcmp(optarg, "ansi_color"))
				opt_style = OPT_STYLE_ANSI_COLOR;
			else if (!strcmp(optarg, "ansi_bold"))
				opt_style = OPT_STYLE_ANSI_BOLD;
			else if (!strcmp(optarg, "html_color"))
				opt_style = OPT_STYLE_HTML_COLOR;
			else if (!strcmp(optarg, "html_font"))
				opt_style = OPT_STYLE_HTML_FONT;
			else
				errx(1, "invalid style `%s'", optarg);
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

	register_style();

	if (opt_style == OPT_STYLE_HTML_COLOR ||
	    opt_style == OPT_STYLE_HTML_FONT)
		fprintf(output_file, "<pre>\n");

	if (argc < 1)
		process_file(NULL);
	else
		while (*argv)
			process_file(*argv++);

	if (opt_style == OPT_STYLE_HTML_COLOR ||
	    opt_style == OPT_STYLE_HTML_FONT)
		fprintf(output_file, "</pre>\n");

	if (output_file != stdout)
		fclose(output_file);

	return 0;
}
