/*	$Id: lexer.l,v 1.22 1997/08/30 01:14:23 sandro Exp $	*/

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
#include "parser.h"

extern int opt_debug;
extern char *current_file_name;

#define MAX_INCLUDE_DEPTH 128

struct include_s {
	char *filename;
	char *path;
	int lineno;
	YY_BUFFER_STATE buffer_state;
};

struct include_s *include_stack[MAX_INCLUDE_DEPTH];

static int include_stack_idx = 0;

int lineno = 1;

static void string(void);
static void comment(void);
static void include(void);
static void include_push(char *);
static void include_pop(void);

static int yywrap(void) { return 1; }
%}

%%

\n			{ lineno++; }

"#"			{ comment(); }

".include"		{ include(); }

[a-zA-Z_][a-zA-Z_0-9]*	{ return IDENTIFIER; }

"\""			{ string(); return STRING; }

"("			{ return '('; }
")"			{ return ')'; }
"="			{ return '='; }

[ \t\v\f]+		/* eat up whitespaces */

<<EOF>>			{
				if (--include_stack_idx < 0)
					yyterminate();
				else
					include_pop();
			}

.			{ errx(1, "%s:%d: unrecognized character `%s'", current_file_name, lineno, yytext); }

%%

char *token_buffer;

/* Max allowed token size. */
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
	while ((c = input()) != EOF && c != '"') {
		if (c == '\n')
			lineno++;
		if (p >= token_buffer + maxtoken)
			p = extend_token_buffer(p);
		if (c == '\\') {
			if ((c = input()) == '\n')
				lineno++;
			else
				*p++ = c;
		} else
			*p++ = c;
	}
	if (c == EOF)
		errx(1, "%s:%d: unexpected end of file in string", current_file_name, lineno);
	*p = '\0';
}

static void
comment(void)
{
	char c;
	while ((c = input()) != EOF && c != '\n')
		;
	lineno++;
}


static char *
build_full_include_path(void)
{
	char *p;
	int i, strsize = 0;

	for (i = 0; i <= include_stack_idx; i++)
		if (include_stack[i]->path != NULL)
			strsize += strlen(include_stack[i]->path) + 1;

	p = (char *)xmalloc(strsize + strlen(include_stack[include_stack_idx]->filename) + 1);

	*p = '\0';

	for (i = 0; i <= include_stack_idx; i++)
		if (include_stack[i]->path != NULL) {
			strcat(p, include_stack[i]->path);
			strcat(p, "/");
		}

	strcat(p, include_stack[include_stack_idx]->filename);

	return p;
}

static void
include(void)
{
	char *p;
	int c;

	while ((c = input()) == ' ' || c == '\t' || c == '\v' ||
	       c == '\f' || c == '\r')
	       	;

	if (c != '"')
		errx(1, "%s:%d: expected `\"' after include directive", current_file_name, lineno);

	p = token_buffer;
	while ((c = input()) != EOF && c != '"') {
		if (p >= token_buffer + maxtoken)
			p = extend_token_buffer(p);
		*p++ = c;
	}
	if (c == EOF)
		errx(1, "%s:%d: unexpected end of file in string", current_file_name, lineno);
	*p = '\0';

	include_push(token_buffer);
}

static void
include_push(char *fname)
{
	char *p;

	if (include_stack_idx >= MAX_INCLUDE_DEPTH)
		errx(1, "%s:%d: includes nested too deeply", current_file_name, lineno);

	include_stack[include_stack_idx] = (struct include_s *)xmalloc(sizeof(struct include_s));

	/*
	 * Check if the include directive changes the directory.
	 */
	if ((p = strchr(fname, '/')) != NULL) {
		include_stack[include_stack_idx]->path = (char *)xmalloc(strlen(fname) - (p - fname) + 1);
		strncpy(include_stack[include_stack_idx]->path, fname, p - fname);
		include_stack[include_stack_idx]->path[p - fname] = '\0';
		include_stack[include_stack_idx]->filename = xstrdup(++p);
	} else {
		include_stack[include_stack_idx]->path = NULL;
		include_stack[include_stack_idx]->filename = xstrdup(fname);
	}

	include_stack[include_stack_idx]->buffer_state = YY_CURRENT_BUFFER;
	include_stack[include_stack_idx]->lineno = lineno;

	p = build_full_include_path();

	free(current_file_name);
	current_file_name = xstrdup(fname);

	if (opt_debug)
		printf("===> including %s\n", p);

	include_stack_idx++;

	if ((yyin = fopen(p, "r")) == NULL)
		err(1, "%s", p);
	free(p);

	lineno = 1;

	yy_switch_to_buffer(yy_create_buffer(yyin, YY_BUF_SIZE));

	BEGIN(INITIAL);
}

static void
include_pop(void)
{
	yy_delete_buffer(YY_CURRENT_BUFFER);

	yy_switch_to_buffer(include_stack[include_stack_idx]->buffer_state);
	free(current_file_name);

	if (opt_debug) {
	        char *p;
	        p = build_full_include_path();
		printf("<=== exiting %s\n", p);
		free(p);
        }

	if (include_stack[include_stack_idx]->path != NULL) {
		current_file_name = (char *)xmalloc(strlen(include_stack[include_stack_idx]->path) + strlen(include_stack[include_stack_idx]->filename) + 1);
		strcpy(current_file_name, include_stack[include_stack_idx]->path);
	} else {
		current_file_name = (char *)xmalloc(strlen(include_stack[include_stack_idx]->filename) + 1);
		*current_file_name = '\0';
	}
	strcat(current_file_name, include_stack[include_stack_idx]->filename);

	if (include_stack[include_stack_idx]->path != NULL)
		free(include_stack[include_stack_idx]->path);
	free(include_stack[include_stack_idx]->filename);

	lineno = include_stack[include_stack_idx]->lineno;

	free(include_stack[include_stack_idx]);
}
