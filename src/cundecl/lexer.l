/*	$Id: lexer.l,v 1.13 2001/07/13 19:09:56 sandro Exp $	*/

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

%{
#include <err.h>

#include "parser.h"

int lineno = 1;

static int yywrap(void) { return 1; }
%}

%%

"\n"			{ lineno++; }
[ \t\v\f\r]		/* eat whitespaces */

"array"			{ return ARRAY; }
"as"			{ return AS; }
"cast"			{ return CAST; }
"declare"|"decl"	{ return DECLARE; }
"expects"		/* ignore */
"function"|"func"	{ return FUNCTION; }
"into"			{ return INTO; }
"of"			{ return OF; }
"pointer"|"ptr"		{ return POINTER; }
"returning"|"ret"	{ return RETURNING; }
"to"			{ return TO; }
"that"			/* ignore */

"auto"			{ return AUTO; }
"char"			{ return CHAR; }
"const"			{ return CONST; }
"double"		{ return DOUBLE; }
"enum"			{ return ENUM; }
"extern"		{ return EXTERN; }
"float"			{ return FLOAT; }
"int"			{ return INT; }
"long"			{ return LONG; }
"register"		{ return REGISTER; }
"short"			{ return SHORT; }
"signed"		{ return SIGNED; }
"static"		{ return STATIC; }
"struct"		{ return STRUCT; }
"typedef"		{ return TYPEDEF; }
"union"			{ return UNION; }
"unsigned"		{ return UNSIGNED; }
"void"			{ return VOID; }
"volatile"		{ return VOLATILE; }

[a-zA-Z_][a-zA-Z_0-9]*	{ return IDENTIFIER; }
[0-9]+			{ return NUMBER; }

","			{ return ','; }
";"			{ return ';'; }
"("			{ return '('; }
")"			{ return ')'; }
"["			{ return '['; }
"]"			{ return ']'; }
"..."			{ return ELLIPSIS; }

.			{ errx(1, "%d: unexpected character `%c'", lineno, yytext[0]); }

%%
