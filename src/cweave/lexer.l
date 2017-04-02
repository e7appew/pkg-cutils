/*	$Id: lexer.l,v 1.7 1997/07/21 21:44:53 sandro Exp $	*/

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

Digit		[0-9]
Literal		[a-zA-Z_]
Hex		[a-fA-F0-9]
Exp		[Ee][+-]?{Digit}+
FS		(f|F|l|L)
IS		(u|U|l|L)+

%{
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <err.h>

#include "config.h"
#include "tokens.h"

static void string(void);
static void comment(void);
static void directive(void);

static int yywrap(void) { return 1; }
extern int xgetc(void);
extern void xungetc(int c);

/* The following avoids flex reading more than it should. */
#define YY_INPUT(buf,result,max_size)		\
	{					\
		int c = xgetc();		\
		if (c != EOF) {			\
			result = 1;		\
			*buf = c;		\
		} else				\
			result = 0;		\
	}
%}

%%

"#"					{ directive(); return DIRECTIVE; }
"/*"					{ comment(); return COMMENT; }

"auto"					{ return KEYWORD; }
"break"					{ return KEYWORD; }
"case"					{ return KEYWORD; }
"char"					{ return KEYWORD; }
"const"					{ return KEYWORD; }
"continue"				{ return KEYWORD; }
"default"				{ return KEYWORD; }
"do"					{ return KEYWORD; }
"double"				{ return KEYWORD; }
"else"					{ return KEYWORD; }
"enum"					{ return KEYWORD; }
"extern"				{ return KEYWORD; }
"float"					{ return KEYWORD; }
"for"					{ return KEYWORD; }
"goto"					{ return KEYWORD; }
"if"					{ return KEYWORD; }
"int"					{ return KEYWORD; }
"long"					{ return KEYWORD; }
"register"				{ return KEYWORD; }
"return"				{ return KEYWORD; }
"short"					{ return KEYWORD; }
"signed"				{ return KEYWORD; }
"sizeof"				{ return KEYWORD; }
"static"				{ return KEYWORD; }
"struct"				{ return KEYWORD; }
"switch"				{ return KEYWORD; }
"typedef"				{ return KEYWORD; }
"union"					{ return KEYWORD; }
"unsigned"				{ return KEYWORD; }
"void"					{ return KEYWORD; }
"volatile"				{ return KEYWORD; }
"while"					{ return KEYWORD; }

{Literal}({Literal}|{Digit})*		{ return IDENTIFIER; }

0[xX]{Hex}+{IS}?			{ return CONSTANT; }
0{Digit}+{IS}?				{ return CONSTANT; }
{Digit}+{IS}				{ return CONSTANT; }
{Digit}+				{ return CONSTANT; }
'(\\.|[^\\'])+'				{ return CHARACTER; }

{Digit}+{Exp}{FS}?			{ return CONSTANT; }
{Digit}*"."{Digit}+({Exp})?{FS}?	{ return CONSTANT; }
{Digit}+"."{Digit}*({Exp})?{FS}?	{ return CONSTANT; }

"\""					{ string(); return STRING; }

">>="					{ return OPERATOR; }
"<<="					{ return OPERATOR; }
"+="					{ return OPERATOR; }
"-="					{ return OPERATOR; }
"*="					{ return OPERATOR; }
"/="					{ return OPERATOR; }
"%="					{ return OPERATOR; }
"&="					{ return OPERATOR; }
"^="					{ return OPERATOR; }
"|="					{ return OPERATOR; }
">>"					{ return OPERATOR; }
"<<"					{ return OPERATOR; }
"++"					{ return OPERATOR; }
"--"					{ return OPERATOR; }
"->"					{ return OPERATOR; }
"&&"					{ return OPERATOR; }
"||"					{ return OPERATOR; }
"<="					{ return OPERATOR; }
">="					{ return OPERATOR; }
"=="					{ return OPERATOR; }
"!="					{ return OPERATOR; }
"..."					{ return OPERATOR; }
"@@"					{ return ENDOFCODE; }

[ \t\v\n\f]				{ return yytext[0]; }
.					{ return yytext[0]; }

%%

char *token_buffer;

static int maxtoken;

void
init_lex(void)
{
	maxtoken = 40;
	token_buffer = (char *)xmalloc(maxtoken + 1);
}

void
done_lex(void)
{
	free(token_buffer);
}

static char *
extend_token_buffer(char *p)
{
	int offset = p - token_buffer;

	maxtoken = maxtoken * 2 + 10;

	token_buffer = (char *)xrealloc(token_buffer, maxtoken + 2);

	return token_buffer + offset;
}

static void
string(void)
{
	char *p;
	int c;

	p = token_buffer;
	*p++ = '"';
	while ((c = input()) != EOF && c != '"') {
		if (p >= token_buffer + maxtoken)
			p = extend_token_buffer(p);
		*p++ = c;
		if (c == '\\')
			*p++ = input();
	}
	if (c == EOF)
		errx(1, "unexpected end of file in string");
	*p++ = '"';
	*p = '\0';
}

static void
comment(void)
{
	char *p;
	int c;

	p = token_buffer;
	*p++ = '/';
	*p++ = '*';
	while ((c = input()) != EOF) {
resync:		if (p >= token_buffer + maxtoken)
			p = extend_token_buffer(p);
		*p++ = c;
		if (c == '*')
			if ((c = input()) == '/') {
				*p++ = c;
				*p = '\0';
				return;
			} else
				goto resync;
	}
	*p = '\0';
}

static void
directive(void)
{
	char *p;
	int c;

	p = token_buffer;
	*p++ = '#';

	while ((c = input()) != EOF && c != '\n' && isspace(c)) {
		if (p >= token_buffer + maxtoken)
			p = extend_token_buffer(p);
		*p++ = c;
		if (c == '\\')
			*p++ = input();
	}
	while (c != EOF && (isalpha(c) || c == '_')) {
		if (p >= token_buffer + maxtoken)
			p = extend_token_buffer(p);
		*p++ = c;
		if (c == '\\')
			*p++ = input();
		c = input();
	}
	xungetc(c);

	*p = '\0';
}
