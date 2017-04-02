/*	$Id: parser.y,v 1.18 1997/08/30 01:14:38 sandro Exp $	*/

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
#include <stdio.h>
#include <err.h>

#include "config.h"
#include "tree.h"

#ifdef YYTEXT_POINTER
extern char *yytext;
#else
extern char yytext[];
#endif
extern int yylex(void);

struct object_s *parsing_tree;

static void yyerror(char *s);
static void skip_action(void);
%}

%union {
	struct object_s *object;
}

%type <object> rule rule_list
%type <object> or identifier string character
%type <object> component component_list component_list_opt

%token IDENTIFIER STRING CHARACTER
%token TOKEN SECTIONSEP
%token RULE

%%

grammar:
	/* empty */
		{ parsing_tree = NULL; }
	| rule_list
		{ parsing_tree = $1; }
	;

rule_list:
	rule_list rule
		{ link_object(last_object($1), $2); $$ = $1; }
	| rule
		{ $$ = $1; }
	;

rule:
	identifier ':' component_list_opt ';'
		{ $$ = new_object(RULE, $1->value);
		  free_object($1);
		  assoc_object($$, $3); }
	;

component_list_opt:
	/* empty */
		{ $$ = NULL; }
	| component_list
		{ $$ = $1; }
	;

component_list:
	component_list component
		{
			if ($2 != NULL) {
				if ($1 != NULL) {
					link_object(last_object($1), $2);
					$$ = $1;
				} else
					$$ = $2;
			} else
				$$ = $1;
		}
	| component
		{ $$ = $1; }
	;

component:
	action
		{ $$ = NULL; }
	| or
		{ $$ = $1; }
	| identifier
		{ $$ = $1; }
	| string
		{ $$ = $1; }
	| character
		{ $$ = $1; }
	;

action:
	'{'
		{ skip_action(); }
	| '=' '{'
		{ skip_action(); }
	;

or:
	'|'
		{ $$ = new_object('|', yytext); }
	;

identifier:
	IDENTIFIER
		{ $$ = new_object(IDENTIFIER, yytext); }
	;

string:
	STRING
		{ $$ = new_object(STRING, yytext); }
	;

character:
	CHARACTER
		{ $$ = new_object(CHARACTER, yytext); }
	;
	
%%

extern int lineno;

static void
yyerror(char *s)
{
	errx(1, "%d:%s", lineno, s);
}

static void
skip_action(void)
{
	int tk, brace = 1;

	while (brace > 0 && (tk = yylex()) != 0 && tk != SECTIONSEP)
		if (tk == '{')
			brace++;
		else if (tk == '}')
			brace--;
	if (tk == 0)
		errx(1, "unexpected end of file in action");
	else if (tk == SECTIONSEP)
		errx(1, "section separator found in action");
}
