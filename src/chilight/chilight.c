/*	$Id: chilight.c,v 1.41 2001/07/15 15:03:25 ncvs Exp $	*/

/*
 * Copyright (c) 1995-2001 Sandro Sigala.  All rights reserved.
 * Copyright (c) 2001 Jukka A. Ukkonen <jau@iki.fi>.  All rights reserved.
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

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <err.h>

#include "config.h"
#include "tokens.h"

enum {
	OPT_FORMAT_ANSI_COLOR,
	OPT_FORMAT_ANSI_BOLD,
	OPT_FORMAT_HTML_COLOR,
	OPT_FORMAT_HTML_FONT,
	OPT_FORMAT_ROFF,
	OPT_FORMAT_TTY
};

static int opt_format = OPT_FORMAT_TTY;
static char *opt_title = NULL;
static unsigned long line_col = 0;
static unsigned long opt_tab_width = 8;

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
} format[MAX_TOKENS];

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
	format[tk].defined = 1;		       	\
	format[tk].before_text = b;       	\
	format[tk].after_text = a;		\
	format[tk].text = t

static int new_line = 0;

static void register_format(void)
{
	switch (opt_format) {
	case OPT_FORMAT_ANSI_COLOR:
		ENTRY(ALL, ANSI_ATTR_NORMAL, NULL, NULL);
		ENTRY(IDENTIFIER, ANSI_ATTR_NORMAL, NULL, NULL);
		ENTRY(CONSTANT, ANSI_ATTR_CYAN, NULL, NULL);
		ENTRY(KEYWORD, ANSI_ATTR_MAGENTA, NULL, NULL);
		ENTRY(TYPE, ANSI_ATTR_MAGENTA, NULL, NULL);
		ENTRY(COMMENT, ANSI_ATTR_RED, NULL, NULL);
		ENTRY(DIRECTIVE, ANSI_ATTR_BLUE, NULL, NULL);
		ENTRY(STRING, ANSI_ATTR_GREEN, NULL, NULL);
		ENTRY(CHARACTER, ANSI_ATTR_GREEN, NULL, NULL);
		break;
	case OPT_FORMAT_ANSI_BOLD:
		ENTRY(ALL, ANSI_ATTR_NORMAL, NULL, NULL);
		ENTRY(KEYWORD, ANSI_ATTR_BOLD, NULL, NULL);
		ENTRY(TYPE, ANSI_ATTR_BOLD, NULL, NULL);
		break;
	case OPT_FORMAT_HTML_COLOR:
		ENTRY(KEYWORD, "<b><font color=black>", NULL, "</font></b>");
		ENTRY(TYPE, "<font color=\"#b682b4\">", NULL, "</font>");
		ENTRY(COMMENT, "<font color=\"#00008b\">", NULL, "</font>");
		ENTRY(DIRECTIVE, "<font color=\"#0000cd\">", NULL, "</font>");
		ENTRY(STRING, "<font color=\"#8b8b00\">", NULL, "</font>");
		ENTRY(CHARACTER, "<font color=\"#8b8b00\">", NULL, "</font>");
		break;
	case OPT_FORMAT_HTML_FONT:
		ENTRY(KEYWORD, "<b>", NULL, "</b>");
		ENTRY(TYPE, "<b>", NULL, "</b>");
		ENTRY(COMMENT, "<i>", NULL, "</i>");
		ENTRY(DIRECTIVE, "<i>", NULL, "</i>");
		break;
	case OPT_FORMAT_ROFF:
		ENTRY(STRING, "\\fI", NULL, "\\fP");
		ENTRY(CHARACTER, "\\fI", NULL, "\\fP");
		ENTRY(KEYWORD, "\\fB", NULL, "\\fP");
		ENTRY(TYPE, "\\fB", NULL, "\\fP");
		ENTRY(COMMENT, "\\f(NR", NULL, "\\fP");
		ENTRY(DIRECTIVE, "\\fB", NULL, "\\fP");
		break;
	case OPT_FORMAT_TTY:
		ENTRY(STRING, "I", NULL, NULL);
		ENTRY(CHARACTER, "I", NULL, NULL);
		ENTRY(KEYWORD, "B", NULL, NULL);
		ENTRY(TYPE, "B", NULL, NULL);
		ENTRY(COMMENT, "I", NULL, NULL);
		ENTRY(DIRECTIVE, "B", NULL, NULL);
		break;
	}
}

static char *filter_buf;
static int max_buf;

static unsigned long next_tab(unsigned long col)
{
	return col + (opt_tab_width - col % opt_tab_width);
}

static void fputs_backtick(const char *s, FILE *stream)
{
	unsigned i;
	unsigned long tab_pos;
	unsigned long tab_len;

	for (; *s != '\0'; ++s)
		switch (*s) {
		case '\t':
			tab_pos = next_tab(line_col);
			tab_len = tab_pos - line_col;
			for (i = 0; i < tab_len; ++i)
				fputc(' ', stream);
			line_col = tab_pos;
			break;
		case '\\':
			fputs("\\\\", stream);
			++line_col;
			break;
		case '\n':
		case '\r':
		case '\f':
			fputc(*s, stream);
			line_col = 0;
			break;
		default:
			fputc(*s, stream);
			++line_col;
		}
}

static void fputs_bold(const char *s, FILE *stream)
{
	for (; *s != '\0'; ++s) {
		if (!isspace(*s)) {
			fputc(*s, stream);
			fputc('\b', stream);
		}
		fputc(*s, stream);
	}
}

static void fputs_emphasis(const char *s, FILE *stream)
{
	for (; *s != '\0'; ++s) {
		if (!isspace(*s)) {
			fputc('_', stream);
			fputc('\b', stream);
		}
		fputc(*s, stream);
	}
}

static char *extend_filter_buf(char *p)
{
	int offset = p - filter_buf;

	max_buf = max_buf * 2 + 10;
	filter_buf = (char *)xrealloc(filter_buf, max_buf + 2);

	return filter_buf + offset;
}

static char *filter_html_markups(char *s)
{
	char *p;

	if (filter_buf == NULL) {
		max_buf = strlen(s) + 10;
		filter_buf = (char *)xmalloc(max_buf);
	}
	p = filter_buf;

	for (; *s != '\0'; ++s) {
		if (p >= filter_buf + max_buf)
			p = extend_filter_buf(p);
		switch (*s) {
		case '<':
			*p++ = '&';
			*p++ = 'l';
			*p++ = 't';
			*p++ = ';';
			break;
		case '>':
			*p++ = '&';
			*p++ = 'g';
			*p++ = 't';
			*p++ = ';';
			break;
		default:
			*p++ = *s;
		}
	}
	*p = '\0';

	return filter_buf;
}

static void process_token(int token, char *buf)
{
	static int last_token = -1;

	if ((opt_format == OPT_FORMAT_HTML_COLOR ||
	     opt_format == OPT_FORMAT_HTML_FONT) && token != ALL)
	    	buf = filter_html_markups(buf);
	if (opt_format == OPT_FORMAT_ROFF && new_line) {
		fputs("\\&", output_file);
		new_line = 0;
	}

	if (opt_format == OPT_FORMAT_TTY) {
		if (!format[token].defined)
			fputs(format[token].text != NULL ?
			      format[token].text : buf, output_file);
		else if (format[token].before_text) {
			if (format[token].before_text[0] == 'I')
				fputs_emphasis(format[token].text != NULL ?
					       format[token].text : buf,
					       output_file);
			else if (format[token].before_text[0] == 'B')
				fputs_bold(format[token].text != NULL ?
					   format[token].text : buf,
					   output_file);
			else
				fputs(format[token].text != NULL ?
				      format[token].text : buf, output_file);
		}
	} else if (format[token].defined || token == ALL) {
		if (last_token != -1 && last_token != token &&
		    format[last_token].after_text != NULL)
			fputs(format[last_token].after_text, output_file);

		if (last_token != token && format[token].before_text != NULL)
			fputs(format[token].before_text, output_file);

		if (opt_format == OPT_FORMAT_ROFF)
			fputs_backtick(format[token].text != NULL ?
				       format[token].text : buf, output_file);
		else
			fputs(format[token].text != NULL ?
			      format[token].text : buf, output_file);

		last_token = token;
	} else
		process_token(ALL, buf);

	new_line = (token == '\n') ? 1 : 0;
}

static void parse(void)
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

static void process_file(char *filename)
{
	if (filename != NULL && strcmp(filename, "-") != 0) {
		if ((yyin = fopen(filename, "r")) == NULL)
			err(1, "%s", filename);
	} else
		yyin = stdin;

	new_line = 1;
	line_col = 0;

	init_lex();
	parse();
	done_lex();

	if (yyin != stdin)
		fclose(yyin);
}

/*
 * Output the program syntax then exit.
 */
static void usage(void)
{
	fprintf(stderr, "\
usage: chilight [-V] [-f format] [-o file] [-t title] [-w width] [file ...]\n\
\n\
       Format can be one of:\n\
       ansi_color, ansi_bold, html_color, html_font, roff, tty.\n");
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

	while ((c = getopt(argc, argv, "f:o:t:w:V")) != -1)
		switch (c) {
		case 'f':
			if (!strcmp(optarg, "ansi_color"))
				opt_format = OPT_FORMAT_ANSI_COLOR;
			else if (!strcmp(optarg, "ansi_bold"))
				opt_format = OPT_FORMAT_ANSI_BOLD;
			else if (!strcmp(optarg, "html_color"))
				opt_format = OPT_FORMAT_HTML_COLOR;
			else if (!strcmp(optarg, "html_font"))
				opt_format = OPT_FORMAT_HTML_FONT;
			else if (!strcmp(optarg, "roff"))
				opt_format = OPT_FORMAT_ROFF;
			else if (!strcmp(optarg, "tty"))
				opt_format = OPT_FORMAT_TTY;
			else
				errx(1, "invalid format `%s'", optarg);
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
			opt_tab_width = atoi(optarg);
			if (opt_tab_width < 2 || opt_tab_width > 16)
				errx(1, "invalid tab width `%s'", optarg);
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

	register_format();

	switch (opt_format) {
	case OPT_FORMAT_HTML_COLOR:
	case OPT_FORMAT_HTML_FONT:
		fprintf(output_file, "<html>\n"
			"<head><title>%s</title></head>\n"
			"<body>\n"
			"<pre>\n", opt_title != NULL ? opt_title : "");
		break;
	case OPT_FORMAT_ROFF:
		fprintf(output_file, ".ft C\n.nf\n");
		break;
	}

	if (argc < 1)
		process_file(NULL);
	else
		while (*argv)
			process_file(*argv++);

	switch (opt_format) {
	case OPT_FORMAT_HTML_COLOR:
	case OPT_FORMAT_HTML_FONT: {
		time_t t = time(NULL);
		fprintf(output_file, "</pre>\n"
			"<hr>\n"
			"Generated by <a href=\"http://www.sigala.it/sandro/\">" CUTILS_VERSION "</a> chilight - %s"
			"</body>\n", ctime(&t));
		break;
	}
	case OPT_FORMAT_ANSI_COLOR:
		fprintf(output_file, "\033[0m");
		break;
	case OPT_FORMAT_ROFF:
		fprintf(output_file, ".fi\n");
		break;
	}

	if (output_file != stdout)
		fclose(output_file);

	return 0;
}
