/*	$Id: parser.y,v 1.14 2001/07/13 19:09:56 sandro Exp $	*/

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
 * This grammar is based on `cdecl' written and put in the public
 * domain by Graham Ross and enhanced by other guys.
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
static void unsupp(char *, char *);
static void docast(char *, char *, char *, char *);
static void dodeclare(char *, char *, char *, char *, char *);

static char prev;
static int arbdims = 1;

#define dup(s)	xstrdup(s)
%}

%union {
	char *string;
	struct {
		char *left;
		char *right;
		char *type;
	} halves;
}

%token IDENTIFIER NUMBER ELLIPSIS
%token ARRAY AS CAST DECLARE FUNCTION
%token INTO OF POINTER RETURNING TO
%token CHAR CONST VOLATILE DOUBLE ENUM FLOAT INT LONG
%token SHORT SIGNED STRUCT UNION UNSIGNED VOID
%token TYPEDEF AUTO EXTERN REGISTER STATIC

%type <string> identifier number
%type <string> declaration_list declaration_list2 dimension
%type <string> type_qualifier_list type_modifier_list type_modifier_list2
%type <string> type_modifier type_qualifier_list_opt storage_class_opt
%type <string> storage_class type_tag type_name type type_qualifier
%type <halves> declaration

%%

file:
	/* empty */
	| command_list
	;

command_list:
	command
	| command_list command
	;

command:
	DECLARE identifier AS storage_class_opt declaration ';'
		{ dodeclare($2, $4, $5.left, $5.right, $5.type); }
	| CAST identifier INTO declaration ';'
		{ docast($2, $4.left, $4.right, $4.type); }
	;

declaration_list:
	/* empty */
		{ $$ = dup(""); }
	| declaration_list2
		{ $$ = $1; }
	| declaration_list ',' declaration_list2
		{ $$ = cat($1, dup(", "), $3, NULL); }
	;

declaration_list2:
	identifier
		{ $$ = $1; }
	| declaration
		{ char *p = cat($1.left, $1.right, NULL);
		  $$ = cat($1.type, dup(strlen(p) > 0 ? " " : ""), p, NULL); }
	| identifier AS declaration
		{ $$ = cat($3.type, dup(" "), $3.left, $1, $3.right, NULL); }
	| ELLIPSIS
		{ $$ = dup("..."); }
	;

declaration:
	FUNCTION RETURNING declaration
		{ if (prev == 'f')
			unsupp("function returning function", "function returning pointer to function");
		  else if (prev == 'A' || prev == 'a')
			unsupp("function returning array", "function returning pointer");
		  $$.left = $3.left;
		  $$.right = cat(dup("()"), $3.right, NULL);
		  $$.type = $3.type;
		  prev = 'f';
		}
	| FUNCTION '(' declaration_list ')' RETURNING declaration
		{ if (prev == 'f')
			unsupp("function returning function", "function returning pointer to function");
		  else if (prev == 'A' || prev == 'a')
			unsupp("function returning array", "function returning pointer");
		  $$.left = $6.left;
		  $$.right = cat(dup("("), $3, dup(")"), $6.right, NULL);
		  $$.type = $6.type;
		  prev = 'f'; }
	| ARRAY dimension OF declaration
		{ if (prev == 'f')
			unsupp("array of function", "array of pointer to function");
		  else if (prev == 'a')
			unsupp("inner array of unspecified size", "array of pointer");
		  else if (prev == 'v')
			unsupp("array of void", "pointer to void");
		  if (arbdims)
			prev = 'a';
		  else
			prev = 'A';
		  $$.left = $4.left;
		  $$.right = cat($2, $4.right, NULL);
		  $$.type = $4.type; }
	| type_qualifier_list_opt POINTER TO declaration
		{ char *op = "", *cp = "", *sp = "";

		  if (prev == 'a')
			unsupp("pointer to array of unspecified dimension", "pointer to object");
		  if (prev == 'a' || prev == 'A' || prev == 'f') {
			op = "(";
			cp = ")";
		  }
		  if (strlen($1) > 0)
			sp = " ";
		  $$.left = cat($4.left, dup(op), dup("*"), dup(sp), $1, dup(sp), NULL);
		  $$.right = cat(dup(cp), $4.right, NULL);
		  $$.type = $4.type;
		  prev = 'p'; }
	| type_qualifier_list_opt type
		{ $$.left = dup("");
		  $$.right = dup("");
		  $$.type = cat($1, dup(strlen($1) > 0 ? " " : ""), $2, NULL);
		  if (!strcmp($2, "void"))
			prev = 'v';
		  else if (!strcmp($2, "struct"))
			prev = 's';
		  else
			prev = 't'; }
	;

dimension:
	/* empty */
		{ arbdims = 1; $$ = dup("[]"); }
	| number
		{ arbdims = 0;
		  $$ = cat(dup("["), $1, dup("]"), NULL); }
	| '[' number ']'
		{ arbdims = 0;
		  $$ = cat(dup("["), $2, dup("]"), NULL); }
	;

type:
	type_modifier_list
		{ $$ = $1; }
	| type_name
		{ $$ = $1; }
	| type_modifier_list type_name
		{ $$ = cat($1, dup(" "), $2, NULL); }
	| type_tag
		{ $$ = $1; }
	;

type_tag:
	STRUCT identifier
		{ $$ = cat(dup("struct"), dup(" "), $2, NULL); }
	| ENUM identifier
		{ $$ = cat(dup("enum"), dup(" "), $2, NULL); }
	| UNION identifier
		{ $$ = cat(dup("union"), dup(" "), $2, NULL); }
	;

type_name:
	INT
		{ $$ = dup("int"); }
	| CHAR
		{ $$ = dup("char"); }
	| FLOAT
		{ $$ = dup("float"); }
	| DOUBLE
		{ $$ = dup("double"); }
	| VOID
		{ $$ = dup("void"); }
	;

type_modifier_list:
	type_modifier type_modifier_list2
		{ $$ = cat($1, dup(" "), $2, NULL); }
	| type_modifier
		{ $$ = $1; }
	;

type_modifier_list2:
	type_modifier_list
		{ $$ = $1; }
	| type_qualifier
		{ $$ = $1; }
	;

type_modifier:
	UNSIGNED
		{ $$ = dup("unsigned"); }
	| SIGNED
		{ $$ = dup("signed"); }
	| LONG
		{ $$ = dup("long"); }
	| SHORT
		{ $$ = dup("short"); }
	;

type_qualifier_list_opt:
	/* empty */
		{ $$ = dup(""); }
	| type_qualifier_list
		{ $$ = $1; }
	;

type_qualifier_list:
	type_qualifier type_qualifier_list_opt
		{ $$ = cat($1, dup(strlen($2) > 0 ? " " : ""), $2, NULL); }
	;

storage_class:
	TYPEDEF
		{ $$ = dup("typedef"); }
	| AUTO
		{ $$ = dup("auto"); }
	| EXTERN
		{ $$ = dup("extern"); }
	| REGISTER
		{ $$ = dup("register"); }
	| STATIC
		{ $$ = dup("static"); }
	;

storage_class_opt:
	storage_class
		{ $$ = $1; }
	| /* empty */
		{ $$ = dup(""); }
	;

type_qualifier:
	CONST
		{ $$ = dup("const"); }
	| VOLATILE
		{ $$ = dup("volatile"); }
	;

identifier:
	IDENTIFIER
		 { $$ = dup(yytext); }
	;

number:
	NUMBER
		 { $$ = dup(yytext); }
	;

%%

static void unsupp(char *s1, char *s2)
{
	if (s2 != NULL)
		warnx("%d: unsupported \"%s\" (maybe you mean \"%s\")", lineno, s1, s2);
	else
		warnx("%d: unsupported \"%s\"", lineno, s1);
}

static void yyerror(char *s)
{
	errx(1, "%d: %s", lineno, s);
}

/*
 * Catenate the null-terminated strings into a newly allocated string.
 * The parameter list is terminated by a NULL pointer.
 * The parameters are deallocated after copy.
 */
static char *cat(char *s, ...)
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

static void docast(char *name, char *left, char *right, char *type)
{
	int lenl = strlen(left), lenr = strlen(right);

	if (prev == 'f')
		unsupp("cast into function", "cast into pointer to function");
	else if (prev == 'A' || prev == 'a')
		unsupp("cast into array", "cast into pointer");
	printf("(%s%*s%s)%s;\n", type, lenl + lenr ? lenl + 1 : 0, left, right, name);

	free(left);
	free(right);
	free(type);
        free(name);
}

static void dodeclare(char *name, char *storage, char *left, char *right, char *type)
{
	if (prev == 'v')
		unsupp("variable of type void", "variable of type pointer to void");

	if (*storage == 'r')
		switch (prev) {
		case 'f':
			unsupp("register function", NULL);
			break;
		case 'A':
		case 'a':
			unsupp("register array", NULL);
			break;
		case 's':
			unsupp("register struct/class", NULL);
			break;
		}

	if (*storage)
		printf("%s ", storage);
	printf("%s %s%s%s;\n", type, left, name, right);

	free(storage);
	free(left);
	free(right);
	free(type);
	free(name);
}
