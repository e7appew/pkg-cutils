/*	$Id: parser.y,v 1.26 1997/08/30 01:14:13 sandro Exp $	*/

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
 * This grammar should be full ANSI C compilant.
 */

%{
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <err.h>

#include "config.h"

extern int lineno;
#ifdef YYTEXT_POINTER
extern char *yytext;
#else
extern char yytext[];
#endif
extern FILE *output_file;

static void yyerror(char *);
static char *cat(char *, ...);

static int cast;
static char *ident;

#define dup(s)	xstrdup(s)
%}

%union {
	char *string;
}

%token TYPEDEF EXTERN STATIC AUTO REGISTER
%token VOID CHAR SHORT INT LONG FLOAT DOUBLE SIGNED UNSIGNED
%token CONST VOLATILE STRUCT UNION ENUM
%token ELLIPSIS IDENTIFIER NUMBER

%type <string> declaration_specifiers storage_class_specifier type_specifier
%type <string> tag_specifier struct_or_union_or_enum specifier_qualifier_list
%type <string> type_qualifier identifier declarator direct_declarator
%type <string> pointer type_qualifier_list parameter_type_list parameter_list
%type <string> parameter_declaration identifier_list type_name
%type <string> abstract_declarator direct_abstract_declarator
%type <string> constant_expression storage_class_specifier_opt

%%

file:
	/* empty */
	| declaration_or_cast_list
	;

declaration_or_cast_list:
	declaration_or_cast
	| declaration_or_cast_list declaration_or_cast
	;

declaration_or_cast:
	no_cast declaration
	| init_cast cast
	;

init_cast:
	{ cast = 1; }
	;

no_cast:
	{ cast = 0; }
	;

cast:
	'(' type_name ')' identifier ';'
		{ char *s = cat(dup("cast"), $4, dup(" into"), $2, NULL);
		  fprintf(output_file, "%s;\n", s);
		  free(s); }
	;

declaration:
	storage_class_specifier_opt declaration_specifiers declarator ';'
		{ char *s = cat(dup("declare"), ident, dup(" as"), $1, $3, $2, NULL);
		  fprintf(output_file, "%s;\n", s);
		  free(s); ident = NULL; }
	;

storage_class_specifier_opt:
	/* empty */
		{ $$ = dup(""); }
	| storage_class_specifier
		{ $$ = $1; }
	;

declaration_specifiers:
	type_specifier
		{ $$ = $1; }
	| type_specifier declaration_specifiers
		{ $$ = cat($1, $2, NULL); }
	| type_qualifier
		{ $$ = $1; }
	| type_qualifier declaration_specifiers
		{ $$ = cat($1, $2, NULL); }
	;

storage_class_specifier:
	TYPEDEF
		{ $$ = dup(" typedef"); }
	| EXTERN
		{ $$ = dup(" extern"); }
	| STATIC
		{ $$ = dup(" static"); }
	| AUTO
		{ $$ = dup(" auto"); }
	| REGISTER
		{ $$ = dup(" register"); }
	;

type_specifier:
	VOID
		{ $$ = dup(" void"); }
	| CHAR
		{ $$ = dup(" char"); }
	| SHORT
		{ $$ = dup(" short"); }
	| INT
		{ $$ = dup(" int"); }
	| LONG
		{ $$ = dup(" long"); }
	| FLOAT
		{ $$ = dup(" float"); }
	| DOUBLE
		{ $$ = dup(" double"); }
	| SIGNED
		{ $$ = dup(" signed"); }
	| UNSIGNED
		{ $$ = dup(" unsigned"); }
	| tag_specifier
		{ $$ = $1; }
	;

tag_specifier:
	struct_or_union_or_enum identifier
		{ $$ = cat($1, $2, NULL); }
	;

struct_or_union_or_enum:
	STRUCT
		{ $$ = dup(" struct"); }
	| UNION
		{ $$ = dup(" union"); }
	| ENUM
		{ $$ = dup(" enum"); }
	;

specifier_qualifier_list:
	type_specifier specifier_qualifier_list
		{ $$ = cat($1, $2, NULL); }
	| type_specifier
		{ $$ = $1; }
	| type_qualifier specifier_qualifier_list
		{ $$ = cat($1, $2, NULL); }
	| type_qualifier
		{ $$ = $1; }
	;

type_qualifier:
	CONST
		{ $$ = dup(" const"); }
	| VOLATILE
		{ $$ = dup(" volatile"); }
	;

identifier:
	IDENTIFIER
		{ $$ = cat(dup(" "), dup(yytext), NULL); }
	;

declarator:
	pointer direct_declarator
		{ $$ = cat($2, $1, NULL); }
	| direct_declarator
		{ $$ = $1; }
	;

direct_declarator:
	identifier
		{ if (ident == NULL && !cast) {
			ident = $1;
			$$ = dup("");
		  } else
			$$ = cat($1, dup(" as"), NULL); }
	| '(' declarator ')'
		{ $$ = $2; }
	| direct_declarator '[' constant_expression ']'
		{ $$ = cat($1, dup(" array["), $3, dup("] of"), NULL); }
	| direct_declarator '[' ']'
		{ $$ = cat($1, dup(" array of"), NULL); }
	| direct_declarator '(' parameter_type_list ')'
		{ $$ = cat($1, dup(" function that expects ("), dup($3+1), dup(") returning"), NULL);
		  free($3); }
	| direct_declarator '(' identifier_list ')'
		{ $$ = cat($1, dup(" function that expects ("), dup($3+1), dup(") returning"), NULL);
		  free($3); }
	| direct_declarator '(' ')'
		{ $$ = cat($1, dup(" function returning"), NULL); }
	;

pointer:
	'*'
		{ $$ = dup(" pointer to"); }
	| '*' type_qualifier_list
		{ $$ = cat($2, dup(" pointer to"), NULL); }
	| '*' pointer
		{ $$ = cat($2, dup(" pointer to"), NULL); }
	| '*' type_qualifier_list pointer
		{ $$ = cat($3, $2, dup(" pointer to"), NULL); }
	;

type_qualifier_list:
	type_qualifier
		{ $$ = $1; }
	| type_qualifier_list type_qualifier
		{ $$ = cat($1, $2, NULL); }
	;

parameter_type_list:
	parameter_list
		{ $$ = $1; }
	| parameter_list ',' ELLIPSIS
		{ $$ = cat($1, dup(", ..."), NULL); }
	;

parameter_list:
	parameter_declaration
		{ $$ = $1; }
	| parameter_list ',' parameter_declaration
		{ $$ = cat($1, dup(","), $3, NULL); }
	;

parameter_declaration:
	declaration_specifiers declarator
		{ $$ = cat($2, $1, NULL); }
	| declaration_specifiers abstract_declarator
		{ $$ = cat($2, $1, NULL); }
	| declaration_specifiers
		{ $$ = $1; }
	;

identifier_list:
	identifier
		{ $$ = $1; }
	| identifier_list ',' identifier
		{ $$ = cat($1, dup(","), $3, NULL); }
	;

type_name:
	specifier_qualifier_list
		{ $$ = $1; }
	| specifier_qualifier_list abstract_declarator
		{ $$ = cat($2, $1, NULL); }
	;

abstract_declarator:
	pointer
		{ $$ = $1; }
	| direct_abstract_declarator
		{ $$ = $1; }
	| pointer direct_abstract_declarator
		{ $$ = cat($2, $1, NULL); }
	;

direct_abstract_declarator:
	'(' abstract_declarator ')'
		{ $$ = $2; }
	| '[' ']'
		{ $$ = dup(" array of"); }
	| '[' constant_expression ']'
		{ $$ = cat(dup(" array["), $2, dup("] of"), NULL); }
	| direct_abstract_declarator '[' ']'
		{ $$ = cat($1, dup(" array of"), NULL); }
	| direct_abstract_declarator '[' constant_expression ']'
		{ $$ = cat($1, dup(" array["), $3, dup("] of"), NULL); }
	| '(' ')'
		{ $$ = dup(" function returning"); }
	| '(' parameter_type_list ')'
		{ $$ = cat(dup(" function that expectes ("), dup($2+1), dup(") returning"), NULL);
		  free($2); }
	| direct_abstract_declarator '(' ')'
		{ $$ = cat($1, dup(" function returning"), NULL); }
	| direct_abstract_declarator '(' parameter_type_list ')'
		{ $$ = cat($1, dup(" function that expects ("), dup($3+1), dup(") returning"), NULL);
		  free($3); }
	;

constant_expression:
	NUMBER
		{ $$ = dup(yytext); }
	;

%%

static void
yyerror(char *s)
{
	errx(1, "%d: %s", lineno, s);
}

/*
 * Catenate the null-terminated strings into a newly allocated string.
 * The parameter list is terminated by a NULL pointer.
 * The parameters are deallocated after copy.
 */
static char *
cat(char *s, ...)
{
	va_list ap;
	int size;
	char *p, *newstr;

	size = strlen(s) + 1;

	va_start(ap, s);
	while ((p = va_arg(ap, char *)) != NULL)
		size += strlen(p) + 1;
	va_end(ap);

	newstr = (char *)xmalloc(size);

	va_start(ap, s);
	strcpy(newstr, s);
	free(s);
	while ((p = va_arg(ap, char *)) != NULL) {
		strcat(newstr, p);
		free(p);
	}
	va_end(ap);

	return newstr;
}
