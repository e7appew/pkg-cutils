/*	$Id: parser.y,v 1.22 1997/08/30 01:14:23 sandro Exp $	*/

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
#include <stdlib.h>
#include <string.h>
#include <err.h>

#include "config.h"
#include "cinfodb.h"

#ifdef YYTEXT_POINTER
extern char *yytext;
#else
extern char yytext[];
#endif
extern char *token_buffer;
extern char *current_file_name;

struct object_s *parsing_tree;

static void yyerror(char *s);
%}

%union {
	char *string;
	struct object_s *object;
	struct variable_s *variable;
}

%token STRING IDENTIFIER

%type <string> string identifier
%type <object> object object_list object_list_opt 
%type <variable> variable variable_list variable_list_opt

%%

cinfo_file:
		/* empty */
			{ parsing_tree = NULL; }
		| object_list
			{ parsing_tree = $1; }
		;

object_list:
		object_list object
			{ link_object(last_object($1), $2); $$ = $1; }
		| object
			{ $$ = $1; }
		;

object_list_opt:
		/* empty */
			{ $$ = NULL; }
		| object_list
			{ $$ = $1; }
		;

object:
		'(' string variable_list_opt object_list_opt ')'
			{ $$ = new_object($2); 
			  $$->variable_list = $3;
			  $$->object_list = $4; }
		;

variable_list_opt:
		/* empty */
			{ $$ = NULL; }
		| variable_list
			{ $$ = $1; }
		;

variable_list:
		variable_list variable
			{ link_variable(last_variable($1), $2); $$ = $1; }
		| variable
			{ $$ = $1; }
		;

variable:
		identifier '=' string
			{ $$ = new_variable($1, $3); }
		;

string:
		STRING	
			{ $$ = xstrdup(token_buffer); }
		;

identifier:
		IDENTIFIER
			{ $$ = xstrdup(yytext); }
		;

%%

extern int lineno;

static void
yyerror(char *s)
{
	errx(1, "%s:%d:%s", current_file_name, lineno, s);
}
