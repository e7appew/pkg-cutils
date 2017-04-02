/*	$Id: cinfodb.h,v 1.19 1997/08/30 01:14:27 sandro Exp $	*/

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

#ifndef _CINFODB_H_
#define _CINFODB_H_

#define CINFODB_VERSION_HIGH	1
#define CINFODB_VERSION_LOW	0

#define CINFODB_HEADER \
	0177, 'C', 'I', 'N', 'F', 'O', \
	CINFODB_VERSION_HIGH, CINFODB_VERSION_LOW, \
	0, 0, 0, 0, 0, 0, 0, 0

#define CINFODB_HEADER_SIZE	16

struct object_s {
	char *name;
	struct object_s *next;
	struct object_s *object_list;
	struct variable_s *variable_list;
};

struct variable_s {
	char *name;
	char *value;
	struct variable_s *next;
};

/* tree.c */
extern struct object_s *new_object(char *);
extern struct object_s *last_object(struct object_s *);
extern struct object_s *link_object(struct object_s *, struct object_s *);
extern struct variable_s *new_variable(char *, char *);
extern struct variable_s *last_variable(struct variable_s *);
extern struct variable_s *link_variable(struct variable_s *, struct variable_s *);
extern int count_objects(struct object_s *);
extern int count_variables(struct variable_s *);
extern void free_object(struct object_s *);
extern void free_variable(struct variable_s *);
extern void free_variable_list(struct variable_s *);
extern void free_object_list(struct object_s *);

/* fileio.c */
extern void write_cinfodb(char *, struct object_s *);
extern void write_cinfodb_stream(FILE *, char *, struct object_s *);
extern struct object_s *read_cinfodb(char *);
extern struct object_s *read_cinfodb_stream(FILE *, char *);

#endif /* !_CINFODB_H_ */
