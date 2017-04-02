/*	$Id: keywords.h,v 1.16 1997/08/30 01:14:32 sandro Exp $	*/

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

static const char *
reserved_identifiers[] = {
	"main",

	/* assert.h */
	"assert",

	/* ctype.h */
	"isalnum", "isalpha", "iscntrl", "isdigit", "isgraph", "islower",
	"isprint", "ispunct", "isspace", "isupper", "isxdigit", "tolower",
	"toupper",

	/* errno.h */
	"errno", "EDOM", "ERANGE",

	/* float.h */
	"DBL_DIG", "DBL_EPSLON", "DBL_MANT_DIG", "DBL_MAX", "DBL_MAX_10_EXP",
	"DBL_MAX_EXP", "DBL_MIN", "DBL_MIN_10_EXP", "DBL_MIN_EXP", "FLT_DIG",
	"FLT_EPSLON", "FLT_MANT_DIG", "FLT_MAX", "FLT_MAX_10_EXP",
	"FLT_MAX_EXP", "FLT_MIN", "FLT_MIN_10_EXP", "FLT_MIN_EXP",
	"FLT_RADIX", "FLT_ROUNDS", "LDBL_DIG", "LDBL_EPSLON",
	"LDBL_MANT_DIG", "LDBL_MAX",  "LDBL_MAX_10_EXP", "LDBL_MAX_EXP",
	"LDBL_MIN", "LDBL_MIN_10_EXP", "LDBL_MIN_EXP",

	/* limits.h */
	"CHAR_BIT", "SCHAR_MIN", "SCHAR_MAX", "UCHAR_MAX", "CHAR_MIN",
	"CHAR_MAX", "MB_LEN_MAX", "SHRT_MIN", "SHRT_MAX", "USHRT_MAX",
	"INT_MIN", "INT_MAX", "UINT_MAX", "LONG_MIN", "LONG_MAX",
	"ULONG_MAX",

	/* locale.h */
	"lconv", "setlocale", "LC_ALL", "LC_COLLATE", "LC_CTYPE",
	"LC_MONETARY", "LC_NUMERIC", "LC_TIME", "localeconv",

	/* math.h */
	"acos", "asin", "atan", "atan2", "ceil", "cos", "cosh", "exp",
	"fabs", "floor", "frexp", "ldexp", "fmod", "log", "log10", "modf",
	"pow", "sin", "sinh", "sqrt", "tan", "tanh", "HUGE_VAL",

	/* setjmp.h */
	"longjmp", "setjmp", "jmp_buf",

	/* signal.h */
	"raise", "signal", "sig_atomic_t", "SIGABRT", "SIGFPE", "SIGILL",
	"SIGINT", "SIGSEGV", "SIGTERM", "SIG_DFL", "SIG_ERR", "SIG_IGN",

	/* stdarg.h */
	"va_arg", "va_end", "va_start", "va_list",

	/* stddef.h */
	"offsetof", "NULL", "ptrdiff_t", "size_t", "wchar_t",

	/* stdio.h */
	"clearerr", "close", "create", "open", "fclose", "feof", "ferror",
	"fflush", "fgetc", "fgetpos", "fgets", "fopen", "fprintf", "fputc",
	"fputs", "fread", "freopen", "fscanf", "fseek", "fsetpos", "ftell",
	"fwrite", "getc", "getchar", "gets", "perror", "printf", "putc",
	"putchar", "puts", "remove", "rename", "rewind", "scanf", "setbuf",
	"setvbuf", "sprintf", "sscanf", "tmpfile", "tmpnam", "ungetc",
	"vfprintf", "vprintf", "vsprintf", "FILE", "fpos_t", "_IOFBF",
	"_IOLBF", "_IONBF", "BUFSIZ", "EOF", "FILENAME_MAX", "FOPEN_MAX",
	"L_tmpnam", "SEEK_CUR", "SEEK_END", "SEEK_SET", "stderr", "stdin",
	"stdout", "TMP_MAX", 

	/* stdlib.h */
	"abort", "abs", "atof", "atoi", "atol", "atexit", "bsearch", "calloc",
	"div", "exit", "free", "getenv", "labs", "ldiv", "malloc", "mblen",
	"mbstowcs", "mbtowc", "qsort", "rand", "realloc", "system", "srand",
	"strtod", "strtol", "strolul", "wcstombs", "wctomb", "div_t",
	"ldiv_t", "EXIT_FAILURE", "EXIT_SUCCESS", "MB_CUR_MAX", "MB_LEN_MAX",
	"RAND_MAX",

	/* string.h */
	"memchr", "memcmp", "memcpy", "memmove", "memset", "strcat",
	"strchr", "strcmp", "strcoll", "strcpy", "strcspn", "strerror",
	"strlen", "strncat", "strncmp", "strncpy", "strpbrk", "strrchr",
	"strspn", "strstr", "strtok", "strxfrn",

	/* time.h */
	"asctime", "clock", "ctime", "difftime", "gmtime", "localtime",
	"mktime", "strftime", "time", "clock_t", "time_t", "tm",
	"CLOCKS_PER_SEC",

	/* getopt.h */
	"opterr", "optopt", "optind", "optarg", "getopt",
	0
};

static const char *
words_table[] = {
	"foo", "bar", "baz", "fobar", "foobar", "fobaz", "foobaz", "quux",
	"fred", "dog", "cat", "fish", "gasp", "bad", "bug", "silly", "buggy",
	"mum", "dad", "disk", "empty", "full", "fast", "small", "big", "ok",
	"hello", "bye", "magic", "obscure", "speed", "index",
	0
};
