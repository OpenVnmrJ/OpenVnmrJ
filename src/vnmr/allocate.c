/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*******************************************************************************
*
*	allocate.c
*
*	This file contains various usefull routines for memory allocation.
*	They are designed as a shell surrounding the standard malloc/free
*	tools.  The purpose is to maintain a list of allocated blocks that
*	maybe easily surveyed and/or free'd at once.
*
*	Functions provided:
*
*		allocate/1	   Allocate a block of a given size with the
*				   default id "<no id>".
*		allocateWithId     Allocate a block of a given size with an
*				   Id (appended to the allocated block list).
*		blocksAllocated/1  Return the number of allocated blocks (of a
*				   given size).
*		charsAllocated/1   Return the total size of allocated blocks
*				   (of a given size).
*		release/1	   Release a given block of storage.
*		releaseWithId/1    Release a blocks with a given ID.
*		releaseAll/0	   Release all blocks of storage.
*		renameAllocation/2 Rename all blocks with a given ID to a new
*				   ID (also given).
*
*	Note: The ID used to distinguish allocated blocks should be a pointer
*	      to a non-allocated character string (typically a constant).
*
*******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "allocate.h"
#include "wjunk.h"

#define nil 0
#define UNIQUE 75

int Eflag;

struct block { int           size;
	       const char   *id;
	       struct block *prev;
	       struct block *next;
	       char          data;
	     };
static struct block *first = nil;
static struct block *last  = nil;

/*******************************************************************************
*
*	allocateWithId/2
*
*	This function allocates a block of a given number of characters, a
*	pointer is returned to the first character.  This block is recored
*	an the list of allocated storage.
*
*******************************************************************************/

void *allocateWithId(register size_t n, const char *id)
{  register struct block *p;

   if ( (p=(struct block *)malloc(sizeof(struct block)+(n-1)+1)) )
   {  ((char *)&(p->data))[n] = UNIQUE;
      p->size = n;
      p->id   = id;
      p->next = nil;
      if (last)
	 last->next = p;
      else
	 first = p;
      p->prev = last;
      last    = p;
      return(&(p->data));
   }
   else
      return(nil);
}

void *allocate(size_t n)
{  return(allocateWithId(n,"noid"));
}

int blocksAllocated(int n)
{  register int           count;
   register struct block *p;

   count = 0;
   p     = first;
   while (p)
   {  if (n <= 0 || p->size == n)
	 count += 1;
      p = p->next;
   }
   return(count);
}

int blocksAllocatedTo(const char *id)
{  register int           count;
   register struct block *p;

   count = 0;
   p     = first;
   while (p)
   {  if (id == nil || strcmp(p->id,id)==0)
	 count += 1;
      p = p->next;
   }
   return(count);
}

int blocksAllocatedToOfSize(const char *id, int n)
{  register int           count;
   register struct block *p;

   count = 0;
   p     = first;
   while (p)
   {  if (id==nil || (strcmp(p->id,id)==0 && (n<=0 || p->size==n)))
	 count += 1;
      p = p->next;
   }
   return(count);
}

int charsAllocated(int n)
{  register int           count;
   register struct block *p;

   count = 0;
   p     = first;
   while (p)
   {  if (n <= 0 || p->size == n)
	 count += p->size;
      p = p->next;
   }
   return(count);
}

int charsAllocatedTo(const char *id)
{  register int           count;
   register struct block *p;

   count = 0;
   p     = first;
   while (p)
   {  if (id == nil || strcmp(p->id,id)==0)
	 count += p->size;
      p = p->next;
   }
   return(count);
}

int charsAllocatedToOfSize(const char *id, int n)
{  register int           count;
   register struct block *p;

   count = 0;
   p     = first;
   while (p)
   {  if (id==nil || (strcmp(p->id,id)==0 && (n<=0 || p->size==n)))
	 count += p->size;
      p = p->next;
   }
   return(count);
}

static void releaseBlock(struct block *b)
{  if (b->prev)
      b->prev->next = b->next;
   else
      first = b->next;
   if (b->next)
      b->next->prev = b->prev;
   else
      last = b->prev;
   if (((char *)&(b->data))[b->size]!=UNIQUE)
     Werrprintf("Memory allocation: End of buffer overwritten in %s",b->id);
   free(b);
}

void release(void *p)
{  long           o;
   struct block *b;
   struct block  s;

   o = (long)(&(s.data)) - (long)(&(s));
   b = (struct block *)((long)(p) - o);
   releaseBlock(b);
}

void releaseAll()
{  register struct block *p;
   register struct block *q;

   p = first;
   while (p)
   {  q = p->next;
      if (((char *)&(p->data))[p->size]!=UNIQUE)
        Werrprintf("Memory allocation: End of buffer overwritten in %s",p->id);
      free(p);
      p = q;
   }
   first = nil;
   last  = nil;
}

void releaseAllWithId(const char *id)
{  struct block *p;
   struct block *q;

   p = first;
   while (p)
      if (id == nil || strcmp(p->id,id) == 0)
      {  q = p->next;
	 release(&(p->data));
	 p = q;
      }
      else
	 p = p->next;
}

/*******************************************************************************
*
*	scanFor/4
*
*	This function is used to walk though the mollusk chain of allocated
*	blocks.  The first parameter is used to limit the walk for blocks
*	marked with a given id (supplying nil matches any id).  The second
*	parameter provides the last packet examined (specify nil for get the
*	first).  The third parameter provides the size of the packet.  The
*	fourth parameter provides the id of the packet (incase the first
*	parameter was nil).  For example...
*
*	{  int   n;
*	   char *id;
*	   char *p;
*
*	   p = nil;
*          while (scanFor(nil,&p,&n,&id))
*	      printf("%d char packet at 0x%06x allocated to %s\n",n,p,id);
*	}
*
*	Note that the address of the data packet is also passed back as the
*	return value (for convienence).  If nil is returned then there are
*	no more packets (with the specified id).
*
*******************************************************************************/

void *scanFor(const char *id, char **p, int *n, const char **i)
{  long                   o;
   register struct block *q;
   struct block           s;

   if (*p == nil)
      q = first;
   else
   {  o = (long)(&(s.data)) - (long)(&(s));
      q = (struct block *)((long)(*p) - o);
      q = q->next;
   }
   while (q)
      if ( (id==nil) || (strcmp(q->id,id)==0) )
      {  *n = q->size;
	 *p = &(q->data);
	 if (id == nil)
         {
	    if (q->id)
	       *i = q->id;
	    else
	       *i = "<no id>";
         }
	 return(&(q->data));
      }
      else
	 q = q->next;
   *n = 0;
   *p = nil;
   *i = "<no id>";
   return(nil);
}

/*******************************************************************************
*
*	skyallocateWithId
*	skyrelease
*	skyreleaseAllWithId
*	skyreleaseAll
*
*	These routines perform the same functions as the routines without
*	the 'sky' prefix except they will allocate and deallocate sky
*	memory if available.  If there is no extra memory for the array
*	processor the skymalloc, skyfree calls will default to malloc and
*	free calls.  In releasing memory these routines will check to see
*	if the memory is in the skymemory first. If it is in skymemory, it
*	will release it, otherwise it will call the free routine.****************
****************************************************************/

void *skyallocateWithId(size_t n, const char *id)
{  struct block *p;

   if (id == nil)
      id = "<no id>";
#ifdef AP
   if (p=(struct block *)skymalloc(sizeof(struct block)+(n-1)+1))
#else 
   if ( (p=(struct block *)malloc(sizeof(struct block)+(n-1)+1)) )
#endif 
   {  p->size                 = n;
      (&(p->data))[n]         = UNIQUE;
    /*((char *)&(p->data))[n] = UNIQUE;*/
      p->id                   = id;
      p->next                 = nil;
      if (last)
	 last->next = p;
      else
	 first = p;
      p->prev = last;
      last    = p;
#ifdef DEBUG
      if (Eflag)
        fprintf(stderr,"(block 0x%08x) gives 0x%08x\n",p,&(p->data));
#endif
      return(&(p->data));
   }
   else
   {  if (Eflag)
        fprintf(stderr," gives nil\n");
      return(nil);
   }
}

void skyrelease(void *p)
{  long           o;
   struct block *b;
   struct block  s;

#ifdef DEBUG
   if (Eflag)
      fprintf(stderr,"skyrelease(0x%08x)... ",p);
#endif
   o = (long)(&(s.data)) - (long)(&(s));
   b = (struct block *)((long)(p) - o);
   if (b->prev)
      b->prev->next = b->next;
   else
      first = b->next;
   if (b->next)
      b->next->prev = b->prev;
   else
      last = b->prev;
#ifdef DEBUG
   if (Eflag)
      fprintf(stderr,"(block 0x%08x) ",b);
#endif
   if (((char *)&(b->data))[b->size]!=UNIQUE)
     Werrprintf("Memory allocation: End of buffer overwritten in %s",b->id);
#ifdef AP
   skyfree(b);
#else 
   free(b);
#endif 
   if (Eflag)
      fprintf(stderr,"done!\n");
}

void skyreleaseAll()
{  register struct block *p;
   register struct block *q;

   if (Eflag)
     fprintf(stderr,"skyReleaseAll... ");
   p = first;
   while (p)
   {  q = p->next;
      if (((char *)&(p->data))[p->size]!=UNIQUE)
         Werrprintf("Memory allocation: End of buffer overwritten in %s",p->id);
#ifdef AP
      skyfree(p);
#else 
      free(p);
#endif 
      p = q;
   }
   first = nil;
   last  = nil;
   if (Eflag)
     fprintf(stderr,"done!\n");
}

void skyreleaseAllWithId(const char *id)
{  struct block *p;
   struct block *q;

   if (Eflag)
     fprintf(stderr,"skyReleaseWithId(%s)...\n",id);
   p = first;
   while (p)
      if (id == nil || strcmp(p->id,id) == 0)
      {  q = p->next;
	 skyrelease(&(p->data));
	 p = q;
      }
      else
	 p = p->next;
   if (Eflag)
     fprintf(stderr,"skyReleaseAll is done!\n");
}

void renameAllocation(const char *oldmac, const char *newmac)
{  register struct block *p;

   p = first;
   while (p)
   {  if (strcmp(p->id,oldmac) == 0)
	 p->id = newmac;
      p = p->next;
   }
}

void releaseWithId(const char *id)
{  register struct block *p;
   register struct block *q;

   p = first;
   while (p)
      if (strcmp(p->id,id) == 0)
      {  q = p->next;
	 releaseBlock(p);
	 p = q;
      }
      else
	 p = p->next;
}
