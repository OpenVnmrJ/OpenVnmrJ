/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*****************************************************************/
/*  hash.c
 *
 *    Hash table routines
 *
 *  Routines
 *     init_hash  - initialize hash table to null.
 *     load_hash  - Initial load of hash table with parameters.
 *     insert     - Insert a word into the hash table.
 *     find       - find word in hash table, return -1 if not found.
 *     hash       - calculate a hash key based on the word.
 *
 *  Date   6/21/89    Greg Brissey
 *
 *****************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "abort.h"

struct _hash_table
{
   const char           *word;
   int             index;
};
typedef struct _hash_table hash_table;

/*int             hash_entries;*/

/*extern int 	   debug;*/
static int      max_hash;
static int      hash_entries;
static hash_table *htable;
static int      total_cols = 0;

static void insert(const char *word, int index);
static int hash(const char *word);

extern int debug;

/*-------------------------------------------------------
| init_hash(size)   Malloc space for hash table and
|		    initialize hash table to null
+-------------------------------------------------------*/
void init_hash(int size)	/* Initialize hash table to null */
{
   int             i;

   max_hash = size * 10;
   if (debug > 0)
      fprintf(stderr, "Max Hash Table Size : %d \n", max_hash);
   htable = (hash_table *) malloc(max_hash * (sizeof(hash_table)));
   if (htable == 0L)
   {
      text_error("insuffient memory for variable pointer allocation!!");
      psg_abort(0);
   }
   for (i = 0; i < max_hash; i++)
   {
      htable[i].word = NULL;
      htable[i].index = -1;
   }
   hash_entries = 0;
}

/*-------------------------------------------------------
| load_hash(name,size)   Load hash table with the parameter
|			 names & indices
+-------------------------------------------------------*/
/* name	array of parameter names */
/* number of parameters */
void load_hash(char **name, int number)	 /* load into hash table from structure s */
{
   int             i;

   total_cols = 0;
   for (i = 0; i < number; i++)
      insert(name[i], i);
}

/*-------------------------------------------------------
| insert(name,size)   Load hash table with the parameter
|			 names & indices
+-------------------------------------------------------*/
static void
insert(const char *word, int index)		/* enter a word in the hash table */
{
   int             key;

   key = hash(word);	/* calculate hash key */
   if (debug > 2)
      fprintf(stderr,"hash: word=%s key=%d\n", word, key);
   /* Check if hash table slot is free  */
   while (htable[key].word)	/* look for next free slot */
   {
      total_cols++;
      if (debug > 1)
	 fprintf(stderr,"hash:collision with %s and %s, total: %d\n",
		word, htable[key].word, total_cols);
      ++key;	/* increment key */
      key %= max_hash;	/* increment key */
   }
   htable[key].index = index;
   htable[key].word = word;
   if (debug > 1)
      fprintf(stderr,"hash: adding word %s  id %d \n", word, index);
   if (++hash_entries > max_hash)
   {
      text_error("hash: ERROR Number of identifiers exceeds hash table\n");
      psg_abort(17);
   }
}

/*-------------------------------------------------------
| find(word)   find word in hash table
|	       return index, if not found return -1
+-------------------------------------------------------*/
int find(const char *word)
{
   int             key;

   key = hash(word);		/* calculate key */
   while (htable[key].word)	/* while there are entries */
   {
      if (strcmp(htable[key].word, word) == 0)	/* we have a match */
	 return (htable[key].index);
      ++key;	/* increment key */
      key %= max_hash;	/* increment key */
   }
   return -1;			/* not found */
}

/*-------------------------------------------------------
| hash(word)   calculate hash key for word 
+-------------------------------------------------------*/
static int
hash(const char *word)			/* calculate hash key */
{
   const char  *w;
   register int    key;

   key = 1;
   for (w = word; *w; w++)
      key = (key * (((int) *w % 26) + 1)) % max_hash;
   return (key);
}
