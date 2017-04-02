/*	$Id: hash.c,v 1.5 1997/06/19 21:41:12 sandro Exp $	*/

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "hash.h"

typedef struct hash_table_bucket *bucketp;

struct hash_table_bucket {
	char *key;
	char *data;
	bucketp next;
};

struct hash_table {
	unsigned long size;
	bucketp *table;
	hashing_function_t hashing_function;
};

/*
 * Hashing function.
 */
static unsigned long
hash_table_hash(const char *p, unsigned long size)
{
	unsigned long hash = 0, g;

	while (*p != '\0') {
		hash = (hash << 4) + *p++;
		if ((g = hash & 0xF0000000) != 0)
			hash ^= g >> 24;
		hash &= ~g;
	}

	return hash % size;
}

static bucketp
build_bucket(const char *key)
{
	bucketp ptr;

	ptr = (bucketp)xmalloc(sizeof(*ptr));
	memset(ptr, 0, sizeof(*ptr));
	ptr->key = xstrdup(key);

	return ptr;
}

static void
free_bucket(bucketp ptr)
{
	if (ptr->key != NULL)
		free(ptr->key);
	if (ptr->data != NULL)
		free(ptr->data);

	free(ptr);
}

htablep
hash_table_build(unsigned long size)
{
	htablep ptr;

	ptr = (htablep)xmalloc(sizeof(*ptr));
	ptr->size = size;
	ptr->table = (bucketp *)xmalloc(ptr->size * sizeof(bucketp));
	memset(ptr->table, 0, ptr->size * sizeof(bucketp));
	ptr->hashing_function = hash_table_hash;

	return ptr;
}

htablep
hash_table_build_default(void)
{
	return hash_table_build(HASH_TABLE_DEFAULT_SIZE);
}

void
hash_table_set_hashing_function(htablep table, hashing_function_t function)
{
	table->hashing_function = function;
}

int
hash_table_free(htablep table)
{
	bucketp bucket, next;
	int i;

	for (i = 0; i < table->size; i++) {
		bucket = table->table[i];
		while (bucket != NULL) {
			next = bucket->next;
			free_bucket(bucket);
			bucket = next;
		}
	}

	free(table->table);
	free(table);

	return 0;
}

int
hash_table_store_key(htablep table, const char *key)
{
	unsigned long hash;

	if (hash_table_exists(table, key))
		return -1;

	hash = table->hashing_function(key, table->size);

	if (table->table[hash] == NULL)
		table->table[hash] = build_bucket(key);
	else {
		bucketp bucket = build_bucket(key);
		bucket->next = table->table[hash];
		table->table[hash] = bucket;
	}

	return 0;
}

int
hash_table_exists(htablep table, const char *key)
{
	bucketp bucket;
	unsigned long hash;

	hash = table->hashing_function(key, table->size);

	for (bucket = table->table[hash]; bucket != NULL; bucket = bucket->next)
		if (!strcmp(bucket->key, key))
			return 1;

	return 0;
}

int
hash_table_delete(htablep table, const char *key)
{
	bucketp bucket, last;
	unsigned long hash;

	hash = table->hashing_function(key, table->size);

	for (bucket = table->table[hash]; bucket != NULL; last = bucket, bucket = bucket->next) {
		if (!strcmp(bucket->key, key)) {
			if (bucket == table->table[hash])
				table->table[hash] = bucket->next;
			else
				last->next = bucket->next;
			free_bucket(bucket);
			return 0;
		}
	}

	return -1;
}

int
hash_table_store_dataptr(htablep table, const char *key, const char *data)
{
	bucketp bucket;
	unsigned long hash;

	hash = table->hashing_function(key, table->size);

	for (bucket = table->table[hash]; bucket != NULL; bucket = bucket->next)
		if (!strcmp(bucket->key, key)) {
			bucket->data = (char *)data;
			return 0;
		}

	return -1;
}

int
hash_table_store_data(htablep table, const char *key, const char *data)
{
	char *p = (char *)data;

	if (data != NULL)
		p = xstrdup(data);

	return hash_table_store_dataptr(table, key, p);
}

int
hash_table_store(htablep table, const char *key, const char *data)
{
	if (hash_table_exists(table, key))
		hash_table_delete(table, key);

	hash_table_store_key(table, key);

	return hash_table_store_dataptr(table, key, data);
}

char *
hash_table_fetch(htablep table, const char *key)
{
	bucketp bucket;
	unsigned long hash;

	hash = table->hashing_function(key, table->size);

	for (bucket = table->table[hash]; bucket != NULL; bucket = bucket->next)
		if (!strcmp(bucket->key, key))
			return bucket->data;

	return NULL;
}

static int
sort_func(const void *ptr1, const void *ptr2)
{
	return strcmp((*(bucketp *)ptr1)->key, (*(bucketp *)ptr2)->key);
}

void
hash_table_dump(htablep table, FILE *fout)
{
	bucketp bucket, *sorted_table;
	int i, j, n;

	/*
	 * Count the hash table buckets.
	 */
	for (i = n = 0; i < table->size; i++)
		for (bucket = table->table[i]; bucket != NULL; bucket = bucket->next)
			n++;

	/*
	 * Allocate space for the sorted table.
	 */
	sorted_table = (bucketp *)xmalloc(sizeof(bucketp) * n);

	/*
	 * Add the buckets to the sorted table.
	 */
	for (i = j = 0; i < table->size; i++)
		for (bucket = table->table[i]; bucket != NULL; bucket = bucket->next)
			sorted_table[j++] = bucket;

	/*
	 * Sort the table.
	 */
	qsort(sorted_table, n, sizeof(struct bucket_s *), sort_func);

	/*
	 * Output the table.
	 */
	for (i = 0; i < n; i++)
		if (sorted_table[i]->data != NULL)
			fprintf(fout, "%s %s\n", sorted_table[i]->key, sorted_table[i]->data);
		else
			fprintf(fout, "%s\n", sorted_table[i]->key);

	free(sorted_table);
}
