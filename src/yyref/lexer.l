/*	$Id: lexer.l,v 1.9 2001/07/13 19:09:56 sandro Exp $	*/

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

/*
 * Based on the original work done by Cathy Taylor and put
 * in the public domain.
 */

%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"

extern void nline(int ln);
extern FILE *output_file;

int lineno;

static void action(void);
static void comment(void);
static void code_block(void);

static int yywrap(void) { return 1; }
%}

digit			[0-9]
u_case			[A-Z]
l_case			[a-z]
id_char			[A-Za-z0-9_\.]
letter			[A-Za-z\._]
white			[\t ]

%%

"/*"			{ comment(); }
"%{"			{ code_block(); return CODE_BLOCK; }
"{"			{ action(); return ACTION; }

{letter}{id_char}*	{ ECHO; return IDENTIFIER; }
"'"(\\.|[^'])+"'"	{ ECHO; return CHARACTER; }
{white}+		{ ECHO;	}
{digit}+		{ ECHO; return NUMBER; }
"%"{white}*"left"	{ ECHO; return LEFT; }
"%"{white}*"right"	{ ECHO; return RIGHT; }
"%"{white}*"nonassoc"	{ ECHO; return NONASSOC; }
"%"{white}*"token"	{ ECHO; return TOKEN; }
"%"{white}*"prec"	{ ECHO; return PREC; }
"%"{white}*"type"	{ ECHO; return TYPE; }
"%"{white}*"start"	{ ECHO; return START; }
"%"{white}*"union"	{ ECHO; return UNION; }
"%%"			{ ECHO; return DOUBLE_PERCENT; }
":"			{ ECHO; return ':'; }
";"			{ ECHO; return ';'; }
","			{ ECHO; return ','; }
"|"			{ ECHO; return '|'; }
"<"			{ ECHO; return '<'; }
">"			{ ECHO; return '>'; }
"\n"			{ ECHO; nline(++lineno); }

%%

static char lastchar, thischar;

static int nextchar(void)
{
	char c;
	c = input();
	putc(c, output_file);
	if (c == '\n')
		nline(++lineno);
	lastchar = thischar;
	thischar = c;
	return c;
}

static void comment(void)
{
	int c;
	ECHO;
	c = nextchar();
	do {
		while (c != '*')
			c = nextchar();
		c = nextchar();
	} while (c != '/');
}

static void code_block(void)
{
	int c;
	ECHO;
	c = nextchar();
	do {
		while (c != '%')
			c = nextchar();
		c = nextchar();
	} while (c != '}');
}

static void action(void)
{
	char c;
	int scope;

	ECHO;
	c = nextchar();
	scope = 1;

	while (scope > 0)
		switch (c = nextchar()) {
		case '*':
			if (lastchar == '/')
				do
					c = nextchar();
				while (lastchar != '*' || c != '/');
			break;
		case '"':
			do
				c = nextchar();
			while (lastchar == '\\' || c != '"');
			break;
		case '{': 
			scope++; 
			break;
		case '}': 
			scope--; 
			break;
		default:
			break;
		}
}
