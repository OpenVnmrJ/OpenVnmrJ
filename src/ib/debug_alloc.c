/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <stdio.h>

#include "boolean.h"
#include "error.h"

/* the following #define turns OFF the redefinition of the C library "alloc"
   functions for this module */

#define REAL_ALLOC
#include "debug_alloc.h"
#include <malloc.h>

#define LIST_SIZE 5000

static struct _blk
{
   char    *p;
   unsigned size;
   char     name;
} blk_list [LIST_SIZE];

static int find_free_slot()
{
   int i;

   for (i = 0; i < LIST_SIZE; ++i)
      if (blk_list[i].p == (char *)NULL)
         return i;

   error ("find_free_slot: list size %d exceeded while looking for free slot!",
          LIST_SIZE);

   return (-1);
}

static int find_pointer (p, caller)
 char *p;
 char *caller;
{
   int i;

   for (i = 0; i < LIST_SIZE; ++i)
      if (blk_list[i].p == p)
         return i;

   error ("find_pointer (%s): pointer 0x%08lX not found in list!",
          caller, (unsigned long)p);

   return (-1);
}

char *DEBUG_CALLOC (nelem, elsize)
 unsigned nelem;
 unsigned elsize;
{
   int i;

   if ( (i = find_free_slot()) < 0)
      return (char *)NULL;

   if ( (blk_list[i].p = calloc (nelem, elsize)) != (char *)NULL)
   {
      blk_list[i].size = nelem*elsize;
      blk_list[i].name = 'C';
   }
   return blk_list[i].p;
}

char *DEBUG_MALLOC (size)
 unsigned size;
{
   int i;

   if ( (i = find_free_slot()) < 0)
      return (char *)NULL;

   if ( (blk_list[i].p = malloc (size)) != (char *)NULL)
   {
      blk_list[i].size = size;
      blk_list[i].name = 'M';
   }
   return blk_list[i].p;
}

char *DEBUG_REALLOC (ptr, size)
 char *ptr;
 unsigned size;
{
   int i;
   char *p = (char *)NULL;

   if ( (i = find_pointer(ptr,"DEBUG_REALLOC")) >= 0)
   {
      if ( (p = realloc (ptr, size)) != (char *)NULL)
      {
         blk_list[i].p = p;
         blk_list[i].size = size;
         blk_list[i].name = 'R';
      }
   }
   return p;
}

int DEBUG_FREE (p)
 char *p;
{
   int i;

   if ( (i = find_pointer(p,"DEBUG_FREE")) >= 0)
   {
      free (blk_list[i].p);
      blk_list[i].p = (char *)NULL;
      blk_list[i].size = 0;
      blk_list[i].name = '\0';
   }
   return i;
}

#include <stdio.h>

int check_list()
{
   int i;
   int result = TRUE;

   (void)fprintf (stderr, "check_list:");

   for (i = 0; i < LIST_SIZE; ++i)
   {
      if (blk_list[i].p != NULL)
      {
         (void)fprintf (stderr, "\n 0x%08lX: %3d bytes (%c)",
                (unsigned long)blk_list[i].p, blk_list[i].size, blk_list[i].name);
         result = FALSE;
      }
   }
   if (result == TRUE)
      (void)fprintf (stderr, " O.K.!");

   (void)fprintf (stderr, "\n");

   return result;
}
