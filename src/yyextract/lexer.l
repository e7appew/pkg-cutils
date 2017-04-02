/*	$Id: lexer.l,v 1.19 1997/11/23 12:50:55 sandro Exp $	*/

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

%{
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <err.h>

#include "parser.h"

extern int parser_run;
extern void add_token(char *);

int lineno = 1;

static void comment(void);
static void directive(void);
static void token(void);

static int yywrap(void) { return 1; }
%}

%%

"\n"				{ lineno++; }
[ \t\v\f\r]			/* eat whitespaces */

"/*"				{ comment(); }

"#"				{ directive(); }

[a-zA-Z_\.][a-zA-Z_\.0-9]*	{ return IDENTIFIER; }

'(\\.|[^\\'])+'			{ return CHARACTER; }
\"(\\.|\\\n|[^\\"])*\"		{ return STRING; }

"%%"				{ if (!parser_run)
					return SECTIONSEP;
				  else
					return 0; }

"%"[ ]*"token"			{ token(); return TOKEN; }

"%"[a-zA-Z_]+			{ directive(); }

.				{ return yytext[0]; }

%%

static void
comment(void)
{
	int c;

	while ((c = input()) != EOF) {
resync:
		if (c == '\n')
			lineno++;
		else if (c == '*')
			if ((c = input()) == '/')
				return;
			else if (c == '\n')
				lineno++;
			else
				goto resync;
	}
}

static void
directive(void)
{
	int c;

	while ((c = input()) != EOF && c != '\n')
		if (c == '\\' && input() == '\n')
				lineno++;
	lineno++;
}

static void
token(void)
{
	char buf[128];
	int c, i;

	while ((c = input()) != EOF && c != '\n') {
		if (c == '\'') {
			i = 0;
			buf[i++] = '\'';
			while ((c = input()) != EOF && c != '\n')
				buf[i++] = c;
			if (c == EOF)
				errx(1, "%d: unexpected end of file", lineno);
			else if (c == '\n')
				errx(1, "%d: unexpected newline", lineno);
			buf[i++] = '\'';
			buf[i++] = '\0';
			add_token(buf);
		} else if (c == '<') {
			/*
			 * Skip the type definition.
			 */
			while ((c = input()) != '>' && c != EOF && c != '\n')
				;
			if (c == EOF)
				errx(1, "%d: unexpected end of file", lineno);
			else if (c == '\n')
				errx(1, "%d: unexpected newline", lineno);
		} else if (isalpha(c)) {
			i = 0;
			buf[i++] = c;
			while (isalnum(c = input()) || c == '_')
				buf[i++] = c;
			if (c == EOF)
				errx(1, "%d: unexpected end of file", lineno);
			unput(c);
			buf[i++] = '\0';
			add_token(buf);
		} else if (c != ' ' && c != '\t')
			errx(1, "%d: unexpected character `%c'", lineno, c);
	}
	if (c == EOF)
		errx(1, "%d: unexpected end of file", lineno);
	lineno++;
}
