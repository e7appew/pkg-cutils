/*	$Id: cobfusc.c,v 1.74 2001/07/14 16:18:16 sandro Exp $	*/

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

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <err.h>

#include "config.h"
#include "htable.h"
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

static FILE *output_file;

/*
 * The variables where are stored the options flags.
 */
static char *opt_separate_output = NULL;/* An output file per input file. */
static int opt_exclusive;               /* Excl-replace for the identifier. */
static int opt_compact_white_spaces;	/* Compact whitespaces option. */
static int opt_compact_macros;		/* Compact macros option. */
static int opt_strip_comments;		/* Strip comments option. */
static int opt_identifier_garbling;	/* Identifier garbling option. */
static int opt_integer_garbling;	/* Integer garbling option. */
static int opt_identifier_case;		/* Identifier case option. */
static int opt_trigraphize;		/* Trigraphize option. */
int opt_digraphize;			/* Digraphize option. */
static int opt_string_garbling;		/* String garbling option. */
static char *opt_prefix = DEFAULT_PREFIX;/* Identifier prefix option. */
static int opt_width = DEFAULT_WIDTH;	/* Output width option. */
static int opt_random_seed;		/* Random seed option. */
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
static void outnl(void)
{
	if ((opt_compact_white_spaces || opt_compact_macros) && in_directive)
		putc('\\', output_file);

	putc('\n', output_file);
}

/*
 * Output a character and update the width counter. Output a newline
 * only if it is required.
 */
static void outch(char c)
{
	if (c == '\n')
		column = 0;
	else if (++column > opt_width) {
		outnl();
		column = 1;
	}

	putc(c, output_file);
}

/*
 * Output a string and update the width counter.
 */
static void outstr(char *s)
{
	int len = (int)strlen(s);
	if (column + len > opt_width && len < opt_width) {
		outnl();
		column = 0;
	}
	for (; *s != '\0'; ++s) {
		if (*s == '\n')
			column = 0;
		else
			++column;
		putc(*s, output_file);
	}
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
		if (opt_compact_white_spaces				\
		    && *sp == lasttk && last_ws < 2)			\
			outch(' ');					\
		out_ws_string(sp);					\
	} while (0)

/* Hash table for read-only identifiers. */
static htable ro_table;
/* Hash table for garbled identifiers. */
static htable id_table;

/* Index used for generating the numeric identifiers. */
static int idnum = 0;

/* Index used for generating the identifier from words. */
static int wordnum = 0;
static int wordrenum = 0;

#define CASE_RANDOM	0
#define CASE_UPPER	1
#define CASE_LOWER	2

static void allocate_tables(void)
{
	ro_table = htable_new();
	id_table = htable_new();
}

static void free_tables(void)
{
	alist a;
	hpair *hp;
	a = htable_list(id_table);
	for (hp = alist_first(a); hp != NULL; hp = alist_next(a))
		free(hp->data);
	alist_delete(a);
	htable_delete(ro_table);
	htable_delete(id_table);
}

/*
 * Convert the string case.
 */
static char *convert_case(char *buf, int c)
{
	char *p;

	switch (c) {
	case CASE_UPPER:
		/*
		 * Convert the string to uppercase.
		 */
		for (p = buf; *p != '\0'; ++p)
			*p = toupper(*p);
		break;
	case CASE_LOWER:
		/*
		 * Convert the string to lowercase.
		 */
		for (p = buf; *p != '\0'; ++p)
			*p = tolower(*p);
		break;
	default:
		/*
		 * Convert the string to random case.
		 */
		for (p = buf; *p != '\0'; ++p)
			if (RANDOM(2) == 1)
				*p = toupper(*p);
			else
				*p = tolower(*p);
	}

	return buf;
}

/*
 * Add an identifier to the hash table.  The identifier
 * is garbled if the garbling option is enabled.  The identifier
 * case is changed if the case option is enabled.
 */
static char *add_identifier(char *s)
{
	char buf[128];
	int identifier_garbling, identifier_case;

	/*
	 * If the identifier is already in the hash table, return it.
	 */
	if (htable_exists(id_table, s))
		return (char *)htable_fetch(id_table, s);

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
		strcpy(buf, opt_prefix);

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
		sprintf(buf, "%s%d", opt_prefix, idnum++);
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
		convert_case(buf, CASE_UPPER);
		break;
	case OPT_IDENTIFIER_CASE_LOWER:
		/*
		 * Change the identifier case to lower.
		 */
		convert_case(buf, CASE_LOWER);
		break;
	case OPT_IDENTIFIER_CASE_SCREW:
		/*
		 * Change the identifier case to random.
		 */
		convert_case(buf, CASE_RANDOM);
	}
	
	/*
	 * Store the new identifier into the hash table.
	 */
	htable_store(id_table, s, xstrdup(buf));
	return htable_fetch(id_table, s);
}

/*
 * Add a file of identifiers to replace to the identifiers hash table.
 */
static void add_replace_file(char *fname)
{
	FILE *f;
	char buf[255], buf1[255], buf2[255];

	if ((f = fopen(fname, "r")) == NULL)
		err(1, "%s", fname);

	while (fgets(buf, 255, f) != NULL) {
		if (strlen(buf) <= 1)
			continue;
		buf1[0] = '\0';
		buf2[0] = '\0';
		sscanf(buf,"%s %s", buf1, buf2);
		if (strlen(buf2) < 1)
			add_identifier(buf1);
		else {
			/* If the identifier is already in the hash table,
			 * print eror message. */
			if (htable_exists(id_table, buf1))
				printf("Identifier %s already defined", buf1);
			else
				htable_store(id_table, buf1, xstrdup(buf2));
		}
        }
	fclose(f);
}

/*
 * Add an identifier to the unmodifiable identifiers hash table.
 */
static void add_ro_identifier(char *s)
{
	htable_store_key(ro_table, s);
}

/*
 * Add a file of identifiers to an hash table.
 */
static void add_file_to_table(char *fname, int readonly)
{
	FILE *f;
	char buf[255];

	if ((f = fopen(fname, "r")) == NULL)
		err(1, "%s", fname);

	while (fgets(buf, 255, f) != NULL) {
		if (strlen(buf) > 1) {
			buf[strlen(buf) - 1] = '\0';
			if (readonly)
				add_ro_identifier(buf);
			else
				add_identifier(buf);
		}
	}

	fclose(f);
}

/*
 * Make a less readable integer expression from an integer.
 */
static char *make_expression(char *buf, int n)
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
static char *octalize_string(char *buf, char *s)
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
			for (i = 0; i < (int)strlen(buf1); ++i)
				*dp++ = buf1[i];
		}

	*dp = '\0';

	return buf;
}

/*
 * Make a digraph respelling for a character.
 */
static char *digraphize_char(char c)
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
static char *trigraphize_char(char c)
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
static void parse(void)
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
					++directive_ws;
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
			if ((p = htable_fetch(id_table, yytext)) == NULL) {
				if (htable_exists(ro_table, yytext) ||
				    opt_exclusive)
					p = yytext;
				else
					p = add_identifier(yytext);
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
			if (in_directive && opt_compact_macros) {
				yylex();
				if (directive_ws < 2)
					outch(' ');
			} else if (opt_compact_white_spaces) {
				if (in_directive && !last_ws)
					outch(' ');
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
		if (tk != COMMENT && tk != '\n' && tk != ' '
		    && tk != '\t' && tk != '\v' && tk != '\f' && tk != '\r')
			lasttk = tk;
	}
}

static void process_file(char *filename)
{
	if (filename != NULL && strcmp(filename, "-") != 0) {
		if ((yyin = fopen(filename, "r")) == NULL)
			err(1, "%s", filename);
		if (opt_separate_output != NULL) {
			char oname[255];
			sprintf(oname, "%s%s", filename, opt_separate_output);
			if (output_file != stdout)
				fclose(output_file);
			if ((output_file = fopen(oname, "w")) == NULL)
				err(1, "%s", oname);
		}
	} else
		yyin = stdin;

	init_lex();
	parse();
	done_lex();

	if (opt_compact_white_spaces)
		outch('\n');

	if (yyin != stdin)
		fclose(yyin);
}

/*
 * Output the program syntax then exit.
 */
static void usage(void)
{
	fprintf(stderr, "\
usage: cobfusc [-AabdemntxV] [-c no | lower | upper | screw | random]\n\
               [-f suffix] [-g file] [-i no | numeric | word | random]\n\
               [-o file] [-p prefix] [-r file] [-s seed] [-u file] [-w cols]\n\
               [-z file] [file ...]\n");
	exit(1);
}

/*
 * Used by the err() functions.
 */
char *progname;

int main(int argc, char **argv)
{
	int i, c;

	progname = argv[0];
	output_file = stdout;

	allocate_tables();

	for (i = 0; reserved_identifiers[i] != NULL; i++)
		add_ro_identifier((char *)reserved_identifiers[i]);

	while ((c = getopt(argc, argv, "Aabc:def:g:i:mno:p:r:s:tu:Vw:xz:")) != -1)
		switch (c) {
		case 'A':
			/*
			 * Enable the -ademt -inumeric options.
			 */
			opt_string_garbling = 1;
			opt_compact_macros = 1;
			opt_compact_white_spaces = 1;
			opt_strip_comments = 1;
			opt_trigraphize = 1;
			opt_identifier_garbling = OPT_IDENTIFIER_GARBLING_NUMERIC;
			break;
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
			break;
		case 'e':
			opt_compact_white_spaces = 1;
			break;
		case 'f':
			opt_separate_output = optarg;
			break;
		case 'g':
			add_file_to_table(optarg, 0);
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
			opt_prefix = optarg;
			break;
		case 'r':
			add_file_to_table(optarg, 1);
			break;
		case 's':
			opt_random_seed = atoi(optarg);
			srand(opt_random_seed);
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
			fprintf(stderr, "%s\n", CUTILS_VERSION);
			exit(0);
		case 'w':
			if ((opt_width = atoi(optarg)) < 2)
				opt_width = 2;
			break;
		case 'x':
			opt_exclusive = 1;
			break;
		case 'z':
			add_replace_file(optarg);
			break;
		case '?':
		default:
			usage();
			/* NOTREACHED */
		}
	argc -= optind;
	argv += optind;

	if (argc < 1)
		process_file(NULL);
	else
		while (*argv)
			process_file(*argv++);

	if (opt_dump) {
		alist a;
		hpair *hp;
		fprintf(dump_file, "### scrambled symbols ###\n");
		a = htable_list(id_table);
		for (hp = alist_first(a); hp != NULL; hp = alist_next(a))
			fprintf(dump_file, "%s %s\n", hp->key, (char *)hp->data);
		alist_delete(a);
		fprintf(dump_file, "\n### unmodifiable symbols ###\n");
		a = htable_list(ro_table);
		for (hp = alist_first(a); hp != NULL; hp = alist_next(a))
			fprintf(dump_file, "%s\n", hp->key);
		alist_delete(a);
		fclose(dump_file);
	}

	if (output_file != stdout)
		fclose(output_file);
	free_tables();

	return 0;
}
