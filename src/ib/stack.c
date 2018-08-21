/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <memory.h>
#include <stdio.h>
#ifndef __OS3__
#include <stdlib.h>
#endif

#include "error.h"
#ifdef DEBUG_ALLOC
#include "debug_alloc.h"
#endif
#include "stack.h"

typedef struct {
   char    *curr;
   unsigned count;
   unsigned esize;
} STACK;

#ifdef __STDC__
Stack Stack_init (unsigned count, unsigned esize)
#else
Stack Stack_init (count, esize)
 unsigned count;
 unsigned esize;
#endif
{
   STACK *stack;

   /* get enough memory for the stack header and array */
   if ( (stack = (STACK *)malloc (sizeof(STACK) + count*esize)) == (STACK *)NULL)
   {
      SYS_ERROR(("Stack_init: malloc %d %d-wide stack entries",count,esize));
   }
   else
   {
      /* record the dimensions of the stack array */
      stack->count = count;
      stack->esize = esize;

      /* put the pointer at the top of the stack */
      stack->curr = (char *)stack + count*esize;
   }
   return ((Stack)stack);

}  /* end of function "Stack_init" */

#ifdef __STDC__
Stack Stack_adjust (Stack stack, unsigned count)
#else
Stack Stack_adjust (stack, count)
 Stack    stack;
 unsigned count;
#endif
{
   STACK *stk;
   unsigned esize;

   if ( (stk = (STACK *)stack) == (STACK *)NULL)
   {
      ERROR(("Stack_adjust: stack pointer is (STACK *)NULL"));
   }
   /* get the size of a stack entry before trying to reallocate the array */
   else
   {
      esize = stk->esize;

      if (count > stk->count)
      {
         stk = (STACK *)realloc ((void *)stk, sizeof(STACK) + count*esize);
         if (stk == (STACK *)NULL)
         {
            SYS_ERROR(("Stack_adjust: realloc %d %d-wide stack entries",count,esize));
            return ((Stack)NULL);
         }
         /* record the new dimensions of the stack array */
         stk->count = count;
         stk->esize = esize;
      }
      /* put the pointer at the top of the stack */
      stk->curr = (char *)stk + count*esize;
   }
   return ((Stack)stk);

}  /* end of function "Stack_adjust" */

#ifdef __STDC__
int Stack_push (Stack stack, void *entry)
#else
int Stack_push (stack, entry)
 Stack stack;
 void *entry;
#endif
{
   STACK *stk;

   if ( (stk = (STACK *)stack) == (STACK *)NULL)
   {
      ERROR(("Stack_push: stack pointer is (STACK *)NULL"));
      return 0;
   }
   /* check for room left on the stack */
   if (stk->curr <= (char *)stk)
   {
      ERROR(("Stack_push: out of stack"));
      return 0;
   }
   /* move to the next available location in the stack and save the input */
   stk->curr -= stk->esize;
   (void)memcpy (stk->curr, (char *)entry, (int)stk->esize);

   return 1;

}  /* end of function "Stack_push" */

#ifdef __STDC__
int Stack_pop (Stack stack, void *entry)
#else
int Stack_pop (stack, entry)
 Stack stack;
 void *entry;
#endif
{
   int result;

   if ( (result = Stack_top (stack, entry)) == 1)
      ((STACK *)stack)->curr += ((STACK *)stack)->esize;

   return (result);

}  /* end of function "Stack_pop" */

#ifdef __STDC__
int Stack_top (Stack stack, void *entry)
#else
int Stack_top (stack, entry)
 Stack stack;
 void *entry;
#endif
{
   STACK *stk;

   if ( (stk = (STACK *)stack) == (STACK *)NULL)
   {
      ERROR(("Stack_top: stack pointer is (STACK *)NULL"));
      return 0;
   }
   /* check for something on the stack */
   if (stk->curr >= (char *)stk + stk->count * stk->esize)
   {
      ERROR(("S_top: nothing on the stack"));
      return 0;
   }
   /* return the contents of the top-of-stack to the caller */
   (void)memcpy ((char *)entry, stk->curr, (int)stk->esize);

   return (1);

}  /* end of function "Stack_top" */
