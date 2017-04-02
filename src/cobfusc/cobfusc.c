/*	$Id: cobfusc.c,v 1.62 1997/11/22 18:38:47 sandro Exp $	*/

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

static char *rcsid = "$Id: cobfusc.c,v 1.62 1997/11/22 18:38:47 sandro Exp $";

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <err.h>

#include "config.h"
#include "hash.h"
#include "tokens.h"
#include "keywords.h"

#define DEFAULT_PREFIX	"q"
#define DEFAULT_WIDTH	78

/* Return an integer where 1 <= integer <= max. */
#define RANDOM(_max) (rand()%_max+1)

enum {
	OPT_IDENTIFIER_GARBLING_NO = 0,
	OPT_IDENTIFIER_GARBLING_NUMERIC,
	OPT_IDENTIFIER_GARBLING_WORD,
	OPT_IDENTIFIER_GARBLING_RANDOM,
	OPT_IDENTIFIER_CASE_NO = 0,
	OPT_IDENTIFIER_CASE_UPPER,
	OPT_IDENTIFIER_CASE_LOWER,
	OPT_IDENTIFIER_CASE_SCREW,
	OPT_IDENTIFIER_CASE_RANDOM,
};

/* From lexer.c */
#ifdef YYTEXT_POINTER
extern char *yytext;
#else
extern char yytext[];
#endif
extern char *token_buffer;
extern FILE *yyin;
extern int yylex(void);
extern void init_lex(void);
extern void done_lex(void);

static FILE *output_file = stdout;

/*
 * The variables where are stored the options flags.
 */
static int opt_compact_white_spaces;	/* Compact whitespaces option. */
static int opt_compact_macros;		/* Compact macros option. */
static int opt_strip_comments;		/* Strip comments option. */
static int opt_identifier_garbling;	/* Identifier garbling option. */
static int opt_integer_garbling;	/* Integer garbling option. */
static int opt_identifier_case;		/* Identifier case option. */
static int opt_trigraphize;		/* Trigraphize option. */
int opt_digraphize;			/* Digraphize option. */
static int opt_share_symbol_table;	/* Symbol table sharing option. */
static int opt_string_garbling;		/* String garbling option. */
static int opt_prefix;			/* Indentifier prefix option. */
static char *opt_prefix_arg;		/* Option argument. */
static int opt_width;			/* Output width option. */
static int opt_width_arg = DEFAULT_WIDTH;/* Option argument. */
static int opt_random_seed;		/* Random seed option. */
static int opt_random_seed_arg;		/* Option argument. */
static int opt_identifier_file;		/* Add identifiers file option. */
static char *opt_identifier_file_arg;	/* Option argument. */
static int opt_dump;			/* Dump symbol table option. */
static FILE *dump_file;

/* The width counter. */
static int column;
/* Set to true when we are in a directive. */
static int in_directive;
static int directive_ws;

/* 
 * Output a backslash if we are in a macro then a newline.
 */
static void
outnl(void)
{
	if ((opt_compact_white_spaces || opt_compact_macros) && in_directive)
		putc('\\', output_file);

	putc('\n', output_file);
}

/*
 * Output a character and update the width counter. Output a newline
 * only if it is required.
 */
static void
outch(char c)
{
	if (c == '\n')
		column = 0;
	else if (++column > opt_width_arg && opt_width) {
		outnl();
		column = 1;
	}

	putc(c, output_file);
}

/*
 * Output a string and update the width counter.
 */
static void
outstr(char *s)
{
	if (((column += strlen(s)) > opt_width_arg) && opt_width) {
		if ((column - strlen(s)) != 0)
			outnl();
		column = strlen(s);
	}
	fputs(s, output_file);
}

/* This variable contains the last written character. */
static int last_ws = '\n';

/* Output a newline only if it is required. */
#define nl_req()							\
	do {								\
		if (opt_compact_white_spaces && last_ws != '\n')	\
			outch('\n');					\
	} while (0)

/* Output a whitespace only if it is required. */
#define ws_req()							\
	do {								\
		if (opt_compact_white_spaces && !last_ws)		\
			outch(' ');					\
	} while (0)

/* Output a string that is not a whitespace delimiter. */
#define out_string(s)							\
	do {								\
		outstr(s);						\
		last_ws = 0;						\
	} while (0)

/* Output a character that is not a whitespace delimiter. */
#define out_char(c)							\
	do {								\
		outch(c);						\
		last_ws = 0;						\
	} while (0)

/* Output a string as a whitespace delimiter. */
#define out_ws_string(s)						\
	do {								\
		outstr(s);						\
		last_ws = 1;						\
	} while (0)

/* Output a character as a whitespace delimiter. */
#define out_ws_char(c)							\
	do {								\
		outch(c);						\
		last_ws = 1;						\
	} while (0)

/* Output a character as a whitespace delimiter if it is required. */
#define out_ws(c)							\
	do {								\
		if (opt_compact_white_spaces) {				\
			if (!last_ws) {					\
				outch(c);				\
				last_ws = c;				\
			}						\
    		} else							\
    			outch(c);					\
	} while (0)

/* Output an operator. */
#define out_op(s)							\
	do {								\
		char *sp = s;						\
		if (opt_compact_white_spaces &&				\
		    *sp == lasttk && last_ws < 2)			\
			outch(' ');					\
		out_ws_string(sp);					\
	} while (0)

/* Hash table for reserved identifiers. */
static htablep unmodifiable_table;

/* Hash table for public garbled identifiers. */
static htablep public_table;

/* Hash table for private garbled identifiers. */
static htablep private_table;

/* Index used for generating the numeric identifiers. */
static int idnum = 0;

/* Index used for generating the identifier from words. */
static int wordnum = 0;
static int wordrenum = 0;

/*
 * Reinitialize the index variables.
 */
static void
reinit_vars(void)
{
	idnum = 0;
	wordnum = 0;
	wordrenum = 0;
}

/*
 * Build the hash table where are kept the unmodifiable identifiers.
 */
static void
build_unmodifiable_table(void)
{
	unmodifiable_table = hash_table_build_default();
}

/*
 * Build the hash table where are kept the public identifiers.
 */
static void
build_public_table(void)
{
	public_table = hash_table_build_default();
}

/*
 * Build the hash table where are kept the private identifiers.
 */
static void
build_private_table(void)
{
	private_table = hash_table_build_default();
}

/*
 * Return true if the specified identifier is unmodifiable,
 * otherwise false.
 */
static int
is_unmodifiable_identifier(char *s)
{
	return hash_table_exists(unmodifiable_table, s);
}

/*
 * Return true if the specified identifier is public,
 * otherwise false.
 */
static int
is_public_identifier(char *s)
{
	return hash_table_exists(public_table, s);
}

/*
 * Return true if the specified identifier is private,
 * otherwise false.
 */
static int
is_private_identifier(char *s)
{
	return hash_table_exists(private_table, s);
}

/*
 * Deallocate the memory used by the public identifiers hash table.
 */
static void
free_public_table(void)
{
	hash_table_free(public_table);
}

/*
 * Deallocate the memory used by the private identifiers hash table.
 */
static void
free_private_table(void)
{
	hash_table_free(private_table);
}

/*
 * Deallocate the memory used by the unmodifiable identifiers hash table.
 */
static void
free_unmodifiable_table(void)
{
	hash_table_free(unmodifiable_table);
}

/*
 * Convert the string case
 *  - c = 1 to uppercase
 *  - c = 2 to lowercase
 *  - otherwise to random case.
 */
static char *
convert_case(char *buf, int c)
{
	char *p = buf;

	switch (c) {
	case 1:
		/*
		 * Convert the string to uppercase.
		 */
		while (*p)
			*p = toupper(*p++);
		break;
	case 2:
		/*
		 * Convert the string to lowercase.
		 */
		while (*p)
			*p = tolower(*p++);
		break;
	default:
		/*
		 * Convert the string to random case.
		 */
		while (*p)
			if (RANDOM(2) == 1)
				*p = toupper(*p++);
			else
				*p = tolower(*p++);
	}

	return buf;
}

/*
 * Add an identifier to the specified hash table.  The identifier
 * is garbled if the garbling option is enabled.  The identifier
 * case is changed if the case option is enabled.
 */
static char *
add_identifier(htablep table, char *s)
{
	char buf[128];
	int identifier_garbling, identifier_case;

	/*
	 * If the identifier is already in the hash table, return it.
	 */
	if (hash_table_exists(table, s))
		return (char *)hash_table_fetch(table, s);

	hash_table_store_key(table, s);
 
	strcpy(buf, s);

	/*
	 * If the user has specified the random integer garbling option,
	 * then select a random option.
	 */
	if (opt_identifier_garbling == OPT_IDENTIFIER_GARBLING_RANDOM)
		identifier_garbling = RANDOM(2);
	else
		identifier_garbling = opt_identifier_garbling;

	/*
	 * If the user has specified the random identifier case option,
	 * then select a random option.
	 */
	if (opt_identifier_case == OPT_IDENTIFIER_CASE_RANDOM)
		identifier_case = RANDOM(3);
	else
		identifier_case = opt_identifier_case;

	/*
	 * If the identifier garbling option is enabled, then
	 * change the identifier.
	 */
	switch (identifier_garbling) {
	case OPT_IDENTIFIER_GARBLING_WORD:
		/*
		 * Change the identifier with a word.
		 */
		if (!words_table[wordnum]) {
			wordnum = 0;
			wordrenum++;
		}
		if (!opt_prefix)
			buf[0] = '\0';
		else
			strcpy(buf, opt_prefix_arg);

		if (wordrenum) {
			strcat(buf, words_table[wordrenum]);
			strcat(buf, "_");
		}
		strcat(buf, words_table[wordnum++]);
		break;
	case OPT_IDENTIFIER_GARBLING_NUMERIC:
		/*
		 * Change the identifier with a number generated word.
		 */
		sprintf(buf, "%s%d", opt_prefix_arg, idnum++);
	}
	
	/*
	 * If the identifier case option is enabled, then
	 * change the identifier case.
	 */
	switch (identifier_case) {
	case OPT_IDENTIFIER_CASE_UPPER:
		/*
		 * Change the identifier case to upper.
		 */
		convert_case(buf, 1);
		break;
	case OPT_IDENTIFIER_CASE_LOWER:
		/*
		 * Change the identifier case to lower.
		 */
		convert_case(buf, 2);
		break;
	case OPT_IDENTIFIER_CASE_SCREW:
		/*
		 * Change the identifier case to random.
		 */
		convert_case(buf, 0);
	}
	
	/*
	 * Store the new identifier into the hash table.
	 */
	hash_table_store_data(table, s, buf);

	return hash_table_fetch(table, s);
}

/*
 * Add an identifier to the public identifiers hash table.
 */
static char *
add_public_identifier(char *s)
{
	return add_identifier(public_table, s);
}

/*
 * Add an identifier to the private identifiers hash table.
 */
static char *
add_private_identifier(char *s)
{
	return add_identifier(private_table, s);
}

/*
 * Add an identifier to the unmodifiable identifiers hash table.
 */
static void
add_unmodifiable_identifier(char *s)
{
	if (!hash_table_exists(unmodifiable_table, s))
		hash_table_store_key(unmodifiable_table, s);
}

/*
 * Add a file of identifiers to the public identifiers hash table.
 */
static void
add_public_file(char *fname)
{
	FILE *f;
	char buf[255];

	if ((f = fopen(fname, "r")) == NULL)
		err(1, "%s", fname);

	while (fgets(buf, 255, f) != NULL) {
		if (strlen(buf) > 1) {
			buf[strlen(buf) - 1] = '\0';
			add_private_identifier(buf);
		}
	}

	fclose(f);
}

/*
 * Add a file of identifiers to the unmodifiable identifiers hash table.
 */
static void
add_unmodifiable_file(char *fname)
{
	FILE *f;
	char buf[255];

	if ((f = fopen(fname, "r")) == NULL)
		err(1, "%s", fname);

	while (fgets(buf, 255, f) != NULL) {
 		if (strlen(buf) > 1) {
			buf[strlen(buf) - 1] = '\0';
			add_unmodifiable_identifier(buf);
		}
	}

	fclose(f);
}

/*
 * Make a less readable integer expression from an integer.
 */
static char *
make_expression(char *buf, int n)
{
	int i1, i2, i3, i4, i5, i6;

	switch (RANDOM(3)) {
	case 1:
		i1 = RANDOM(n);
		i2 = n / i1;
		i3 = n % i1;
		sprintf(buf, "(%d*%d+%d)", i1, i2, i3);
		break;
	case 2:
		i1 = RANDOM(n);
		i2 = n / i1;
		i3 = n % i1;
		i4 = RANDOM(i2);
		i5 = i2 / i4;
		i6 = i2 % i4;
		sprintf(buf, "(%d*(%d*%d+%d)+%d)", i1, i4, i5, i6, i3);
		break;
	case 3:
		i1 = n / 2;
		i2 = n % 2;
		if (RANDOM(2) == 1)
			sprintf(buf, "(%d+%d)", i1, i1 + i2);
		else
			sprintf(buf, "(%d+%d)", i1 + i2, i1);
		break;
	}
	return buf;
}

/*
 * Make an octalized string from a literal one.  The already existent
 * escapes are not clobbered.
 */
static char *
octalize_string(char *buf, char *s)
{
	char *sp = s, *dp = buf;
	char buf1[12];
	int i;

	while (*sp)
		switch (*sp) {
		case '\n':
		case '\t':
		case '\v':
		case '\f':
		case '\r':
			/*
			 * The white spaces are just echoed.
			 */
			*dp++ = *sp++;
			break;
		case '\\':
			/*
			 * We must not clobber the existent string escapes.
			 */
			*dp++ = *sp++;
			switch (*sp) {
			case '\0':
				break;
			case '0':
			case '1': case '2': case '3':
			case '4': case '5': case '6':
			case '7': case '8': case '9':
				/*
				 * An octal escape.
				 */
				while (isdigit(*sp))
					*dp++ = *sp++;
				break;
			case 'x':
				/*
				 * An hexadecimal escape.
				 */
				*dp++ = *sp++;
				i = 0;
				while (isxdigit(*sp) && i++ < 2)
					*dp++ = *sp++;
				break;
			default:
				/*
				 * If it is not an octal or hexadecimal escape,
				 * it is a single character one.
				 */
				*dp++ = *sp++;
			}
			break;
		default:
			/*
			 * Output the octal escape.
			 */
			sprintf(buf1, "\\%o", *sp++);
			for (i = 0; i < strlen(buf1); i++)
				*dp++ = buf1[i];
		}

	*dp = '\0';

	return buf;
}

/*
 * Make a digraph respelling for a character.
 */
static char *
digraphize_char(char c)
{
	char *p;

	switch (c) {
	case '[':
		p = "<:";
		break;
	case ']':
		p = ":>";
		break;
	case '{':
		p = "<%";
		break;
	case '}':
		p = "%>";
		break;
	default:
		p = yytext;
	}

	return p;
}

/*
 * Make a trigraph sequence from a character.
 */
static char *
trigraphize_char(char c)
{
	char *p;

	switch (c) {
	case '[':
		p = "\?\?(";
		break;
	case ']':
		p = "\?\?)";
		break;
	case '{':
		p = "\?\?<";
		break;
	case '}':
		p = "\?\?>";
		break;
	case '^':
		p = "\?\?'";
		break;
	case '|':
		p = "\?\?!";
		break;
	case '~':
		p = "\?\?-";
		break;
	default:
		p = yytext;
	}

	return p;
}

/*
 * The main parsing function.
 */
static void
parse(void)
{
	int tk, lasttk = 0;
        char *p;

	while ((tk = yylex()) != 0) {
		switch (tk) {
		case '\n':
			/*
			 * The directives are finished by a newline
			 * and if the compact white spaces option is
			 * enabled, the newlines are outputted.
			 */
			if (in_directive) {
				outch('\n');
				last_ws = '\n';
				in_directive = 0;
			} else
				if (!opt_compact_white_spaces)
					outch(tk);
			break;
		case ' ':
		case '\t':
		case '\v':
		case '\f':
		case '\r':
			if (opt_compact_macros && in_directive) {
				if (directive_ws < 2) {
					directive_ws++;
					if (tk == '\t')
						out_ws_char(' ');
					else
						out_ws_char(tk);
				}
			} else
				if (!opt_compact_white_spaces)
				    outch(tk);
			break;
		case '[':
		case ']':
		case '{':
		case '}':
			if (opt_digraphize) {
				out_ws_string(digraphize_char(tk));
				break;
			}
			/* FALLTRHU */
		case '^':
		case '|':
		case '~':
			if (opt_trigraphize)
				out_ws_string(trigraphize_char(tk));
			else
				out_ws_char(tk);
			break;
		case COMMENT:
			/*
			 * The comments are stripped if the strip comments
			 * option is enabled.
			 */
			if (!opt_strip_comments) {
				lasttk = tk;
				out_ws_string(token_buffer);
			}
			break;
		case IDENTIFIER:
			if (is_unmodifiable_identifier(yytext))
				p = yytext;
			else {
				if (opt_share_symbol_table)
					p = add_public_identifier(yytext);
				else
					p = add_private_identifier(yytext);
			}
			ws_req();
			out_string(p);
			break;
		case CONSTANT:
			/*
			 * The constants are simply outputted literally.
			 */
			ws_req();
			out_string(yytext);
			break;
		case INTEGER:
			ws_req();
			if (opt_integer_garbling) {
				int toint;
				char buf[256];
				if ((toint = atoi(yytext)) > 0 && toint < 50)
					out_string(make_expression(buf, toint));
				else
					out_string(yytext);
			} else
				out_string(yytext);
			break;
		case CHARACTER:
			out_ws_string(yytext);
			break;
		case STRING:
			if (opt_string_garbling) {
				char *buf, *buf1;
				buf = (char *)xmalloc(strlen(token_buffer) * 4 + 1);
				buf1 = (char *)xmalloc(strlen(token_buffer) * 4 + 1);
				strcpy(buf1, token_buffer);
				buf1[strlen(buf1) - 1] = '\0';
				octalize_string(buf, buf1 + 1);
				strcpy(buf1, "\"");
				strcat(buf1, buf);
				strcat(buf1, "\"");
				out_ws_string(buf1);
				free(buf);
				free(buf1);
			} else
				out_ws_string(token_buffer);
			break;
		case INCLUDE_DIRECTIVE:
			nl_req();
			outstr(token_buffer);
			if (opt_compact_white_spaces) {
				outch('\n');
				last_ws = '\n';
	    		}
			break;
		case DIRECTIVE:
			nl_req();
			out_string(token_buffer);
			in_directive = 1;
			directive_ws = 0;
			break;
		case KEYWORD:
			ws_req();
			out_string(yytext);
			break;
		case OPERATOR:
			out_op(yytext);
			break;
		case '\\':
			if (opt_compact_macros && in_directive) {
				yylex();
				if (directive_ws < 2)
					outch(' ');
			} else
				if (opt_compact_white_spaces) {
					out_char('\\');
					out_ws(yylex());
				} else
					out_ws_char(tk);
			break;
		case HASHHASH:
			if (opt_digraphize)
				out_ws_string("%:%:");
			else
				out_ws_string("##");
			break;
		default:
			out_ws_char(tk);
		}
		if (tk != COMMENT && tk != '\n' && tk != ' ' &&
		    tk != '\t' && tk != '\v' && tk != '\f' && tk != '\r')
			lasttk = tk;
	}
}

static void
process_file(char *filename)
{
	if (filename != NULL && strcmp(filename, "-") != 0) {
		if ((yyin = fopen(filename, "r")) == NULL)
			err(1, "%s", filename);
	} else
		yyin = stdin;

	if (!opt_share_symbol_table)
		reinit_vars();

	build_private_table();

	if (opt_identifier_file)
		add_public_file(opt_identifier_file_arg);

	init_lex();
	parse();
	done_lex();

	if (opt_dump) {
		fprintf(dump_file, "%s: private symbols\n", filename == NULL ? "<stdin>" : filename);
		hash_table_dump(private_table, dump_file);
		fprintf(dump_file, "\n");
	}

	if (opt_compact_white_spaces)
		outch('\n');

	if (yyin != stdin)
		fclose(yyin);

	free_private_table();
}

/*
 * Output the program syntax then exit.
 */
static void
usage(void)
{
	fprintf(stderr, "\
usage: cobfusc [-abdehmntV] [-c no | lower | upper] [-g file]\n\
	       [-i no | numeric | word] [-o file] [-p file] [-r prefix]\n\
	       [-s seed] [-u file] [-w cols] [file ...]\n");
	exit(1);
}

int
main(int argc, char **argv)
{
	int i, c;

	build_unmodifiable_table();
	build_public_table();

	for (i = 0; reserved_identifiers[i] != NULL; i++)
		add_unmodifiable_identifier((char *)reserved_identifiers[i]);

	while ((c = getopt(argc, argv, "abc:deg:hi:mno:p:r:s:tu:Vw:")) != -1)
		switch (c) {
		case 'a':
			opt_string_garbling = 1;
			break;
		case 'b':
			opt_digraphize = 1;
			break;
		case 'c':
			if (!strcmp(optarg, "no"))
				opt_identifier_case = OPT_IDENTIFIER_CASE_NO;
			else if (!strcmp(optarg, "screw"))
				opt_identifier_case = OPT_IDENTIFIER_CASE_SCREW;
			else if (!strcmp(optarg, "random"))
				opt_identifier_case = OPT_IDENTIFIER_CASE_RANDOM;
			else if (!strcmp(optarg, "lower"))
				opt_identifier_case = OPT_IDENTIFIER_CASE_LOWER;
			else if (!strcmp(optarg, "upper"))
				opt_identifier_case = OPT_IDENTIFIER_CASE_UPPER;
			else {
				errx(1, "invalid conversion case `%s'", optarg);
				exit(1);
			}
			break;
		case 'd':
			opt_compact_macros = 1;
			opt_width = 1;
			break;
		case 'e':
			opt_compact_white_spaces = 1;
			opt_width = 1;
			break;
		case 'g':
			opt_identifier_file = 1;
			opt_identifier_file_arg = optarg;
			break;
		case 'h':
			opt_share_symbol_table = 1;
			break;
		case 'i':
			if (!strcmp(optarg, "no"))
				opt_identifier_garbling = OPT_IDENTIFIER_GARBLING_NO;
			else if (!strcmp(optarg, "numeric"))
				opt_identifier_garbling = OPT_IDENTIFIER_GARBLING_NUMERIC;	 
			else if (!strcmp(optarg, "word"))
				opt_identifier_garbling = OPT_IDENTIFIER_GARBLING_WORD;
			else if (!strcmp(optarg, "random"))
				opt_identifier_garbling = OPT_IDENTIFIER_GARBLING_RANDOM;
			else
				errx(1, "invalid identifier garbling mode `%s'", optarg);
			break;
		case 'm':
			opt_strip_comments = 1;
			break;
		case 'n':
			opt_integer_garbling = 1;
			break;
		case 'o':
			if (output_file != stdout)
				fclose(output_file);
			if ((output_file = fopen(optarg, "w")) == NULL)
				err(1, "%s", optarg);
			break;
		case 'p':
			add_unmodifiable_file(optarg);
			break;
		case 'r':
			opt_prefix = 1;
			opt_prefix_arg = optarg;
			break;
		case 's':
			opt_random_seed = 1;
			opt_random_seed_arg = atoi(optarg);
			break;
		case 't':
			opt_trigraphize = 1;
			break;
		case 'u':
			opt_dump = 1;
			if (dump_file != NULL)
				fclose(dump_file);
			if ((dump_file = fopen(optarg, "w")) == NULL)
				err(1, "%s", optarg);
			break;
		case 'V':
			fprintf(stderr, "%s - %s\n", CUTILS_VERSION, rcsid);
			exit(0);
		case 'w':
			opt_width = 1;
			if ((opt_width_arg = atoi(optarg)) < 2)
				opt_width_arg = 2;
			break;
		case '?':
		default:
			usage();
			/* NOTREACHED */
		}
	argc -= optind;
	argv += optind;

	if (!opt_prefix)
		opt_prefix_arg = DEFAULT_PREFIX;

#ifndef DEBUG
	if (!opt_random_seed)
		opt_random_seed_arg = (int)time((time_t *)NULL) + getpid();
#endif
	srand(opt_random_seed_arg);

	if (argc < 1)
		process_file(NULL);
	else
		while (*argv)
			process_file(*argv++);

	if (opt_dump) {
		if (opt_identifier_file || opt_share_symbol_table) {
			fprintf(dump_file, "public symbols\n");
			hash_table_dump(public_table, dump_file);
			fprintf(dump_file, "\n");
		}
		fprintf(dump_file, "unmodifiable symbols\n");
		hash_table_dump(unmodifiable_table, dump_file);
		fclose(dump_file);
	}

	if (output_file != stdout)
		fclose(output_file);
	free_unmodifiable_table();
	free_public_table();

	return 0;
}
