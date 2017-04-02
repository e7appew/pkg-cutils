/*	$Id: parser.y,v 1.8 1997/08/30 01:14:41 sandro Exp $	*/

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

/*
 * Based on the original work done by Cathy Taylor and put
 * in the public domain.
 */

%{
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <err.h>

#include "config.h"

extern void on_left(char *, int);
extern void on_right(char *, int);

extern FILE *output_file;
#ifdef YYTEXT_POINTER
extern char *yytext;
#else
extern char yytext[];
#endif
extern int lineno;

static void yyerror(char *);
%}

%token IDENTIFIER CHARACTER NUMBER
%token LEFT RIGHT NONASSOC TOKEN PREC TYPE START UNION
%token DOUBLE_PERCENT CODE_BLOCK ACTION

%%

file:
	definition_list_opt DOUBLE_PERCENT rule_list tail
		{
			fprintf(output_file, "\n\n");
			yyclearin;
			return 0;
		}
	;

tail:
	/* empty */
	| DOUBLE_PERCENT
	| error
	;

definition_list_opt:
	/* empty */
	| definition_list
	;

definition_list:
	definition
	| definition_list definition
	;

definition:
	START IDENTIFIER
	| UNION
	| CODE_BLOCK
	| rword tag nlist
	| tokendef
	| error
	;

rword:
	LEFT
	| RIGHT
	| NONASSOC
	| TYPE
	;

tokendef:
	TOKEN tokenlist
	;

tokenlist:
	IDENTIFIER
		{ on_left(yytext, lineno); }
	| tokenlist comma_opt IDENTIFIER
		{ on_left(yytext, lineno); }
	;

tag:
	/* empty */
	| '<' IDENTIFIER '>'
	;

nlist:
	IDENTIFIER number_opt
	| nlist comma_opt IDENTIFIER number_opt
	;

comma_opt:
	/* empty */
	| ','
	;

number_opt:
	/* empty */
	| NUMBER
	;

rule_list:
	rule
	| rule_list rule
	;

rule:
	IDENTIFIER
		{ on_left(yytext, lineno); }
	':' body ';'
	| error ';'
	;

body:
	body_block
	| body '|' body_block
	;

body_block:
	/* empty */
	| body_block body_entity
	;

body_entity:
	prec_opt id_entity
	| ACTION
	;

id_entity:
	IDENTIFIER
		{ on_right(yytext, lineno); }
	| CHARACTER
	;

prec_opt:
	/* empty */
	| PREC
	;

%%

static void
yyerror(char *s)
{
	errx(1, "%d: %s", lineno, s);
}

void
nline(int ln)
{
	fprintf(output_file, "%4d:\t", ln);
}
