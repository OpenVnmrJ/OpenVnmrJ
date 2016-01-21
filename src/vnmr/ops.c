/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*------------------------------------------------------------------------------
|
|	ops.c
|
|	This file contains the various procedures used to implement the various
|	operations (unary and binary).  In general these take several arguments
|	the first of which is a pointer to the desired resultaint pair, the
|	remaining arguments (1 or 2) are the arguments for the operation.
|	If successful a true (non-zero) is returned, if unsuccessful, a false
|	(zero) is returned.
|
|	NOTE:	Most operations deal naturally with REAL values.  In general
|		convertions are NOT performed!
|
+-----------------------------------------------------------------------------*/

#include <stdio.h>
#include <math.h>
#include <string.h>
#include "tools.h"
#include "variables.h"
#include "wjunk.h"

extern Rval    *newRval();
#ifdef VNMRJ
extern void     jExpressionError();
#endif

int     execPError = 0;
#ifdef  DEBUG
extern int      Dflag;
#define DPRINT0(level, str) \
   if (level <= Dflag) fprintf(stderr,str)
#define DPRINT1(level, str, arg1) \
   if (level <= Dflag) fprintf(stderr,str,arg1)
#define DSHOWLEFT(level, op, var) \
   if (level <= Dflag) \
   {  fprintf(stderr,"%s left arg...\n",op); \
      showPair(stderr,op,var); \
   }
#define DSHOWRIGHT(level, op, var) \
   if (level <= Dflag) \
   {  fprintf(stderr,"%s right arg...\n",op); \
      showPair(stderr,op,var); \
   }
#define DSHOWRESULT(level, op, var) \
   if (level <= Dflag) \
   {  fprintf(stderr,"%s result is...\n",op); \
      showPair(stderr,op,var); \
   }
#else 
#define DPRINT0(level, str)
#define DPRINT1(level, str, arg1)
#define DSHOWLEFT(level, op, var)
#define DSHOWRIGHT(level, op, var)
#define DSHOWRESULT(level, op, var)
#endif 

/*------------------------------------------------------------------------------
|
|	do_EQ/3
|
|	This procedure is used to check equality, it returns true (non-zero
|	real value) if the two arguments are equal.
|
|	NOTE:	Strings may be compared with strings and reals may be
|		compared with reals (but no mixing!).
|
+-----------------------------------------------------------------------------*/

int do_EQ(pair *a, pair *b, pair *c)
{  Rval *bp;
   Rval *cp;

   DSHOWLEFT(3, "do_EQ", b);
   DSHOWRIGHT(3, "do_EQ", c);
   if (b && c)
   {  a->T.basicType = T_REAL;
      bp             = b->R;
      cp             = c->R;
      switch (b->T.basicType)
      {
        default:
        case T_UNDEF:  return(0);
	case T_REAL:   switch (c->T.basicType)
		       {
                         default:
                         case T_UNDEF:  return(0);
			 case T_REAL:	a->R       = newRval();
					a->T.size += 1;
					a->R->v.r  = 0.0;
					if (b->T.size == c->T.size)
					{  while (bp && cp)
					   if (bp->v.r != cp->v.r)
					      return(1);
					   else
					   {  bp = bp->next;
					      cp = cp->next;
					   }
					   a->R->v.r = 1.0;
					   return(1);
					}
					else
					   a->R->v.r = 0.0;
					return(1);
			 case T_STRING: 
                              execPError = 1;
                              if (c->R->v.s != NULL)
                                 WerrprintfWithPos("Can't compare REAL %g with STRING %s", b->R->v.r, c->R->v.s);
                              else
                                 WerrprintfWithPos("Can't compare REAL with STRING");
#ifdef VNMRJ
					jExpressionError();
#endif 
					return(0);
		       }
	case T_STRING: switch (c->T.basicType)
		       {
                         default:
                         case T_UNDEF:  return(0);
			 case T_REAL:
                              execPError = 1;
                              if (b->R->v.s != NULL)
                                 WerrprintfWithPos("Can't compare STRING %s with REAL %g", b->R->v.s, c->R->v.r);
                              else
                                 WerrprintfWithPos("Can't compare STRING with REAL");
#ifdef VNMRJ
					jExpressionError();
#endif 
					return(0);
			 case T_STRING: a->R       = newRval();
					a->T.size += 1;
					a->R->v.r  = 0.0;
					if (b->T.size == c->T.size)
					{  while (bp && cp)
					   if (strcmp(bp->v.s,cp->v.s))
					      return(1);
					   else
					   {  bp = bp->next;
					      cp = cp->next;
					   }
					   a->R->v.r = 1.0;
					   return(1);
					}
					else
					   a->R->v.r = 0.0;
					return(1);
		       }
      }
   }
   else
      return(0);
}

/*------------------------------------------------------------------------------
|
|	do_NE/3
|
|	This procedure is used to check equality, it returns true (non-zero
|	real value) if the two arguments are NOT equal.  Note that we use
|	do_EQ/3 to do the dirty work, then just flip the results.
|
+-----------------------------------------------------------------------------*/

int do_NE(pair *a, pair *b, pair *c)
{
   DPRINT0(3,"do_NE: hand-off to do_EQ...\n");
   if (do_EQ(a,b,c))
   {  if (a->R->v.r == 0.0)
	 a->R->v.r = 1.0;
      else
	 a->R->v.r = 0.0;
      DPRINT1(3,"do_NE: ...ok! (result is %f)\n",a->R->v.r);
      return(1);
   }
   else
      return(0);
}

/*------------------------------------------------------------------------------
|
|	do_LT/3
|
|	This procedure is used to check for less than relation.  Note that
|	this is only defined for scalars!
|
+-----------------------------------------------------------------------------*/

int do_LT(pair *a, pair *b, pair *c)
{
   DSHOWLEFT(3, "do_LT", b);
   DSHOWRIGHT(3, "do_LT", c);
   if (b && c)
   {  switch (b->T.basicType)
      {
        default:
        case T_UNDEF:  return(0);
	case T_REAL:   switch (c->T.basicType)
		       {
                         default:
                         case T_UNDEF:  return(0);
			 case T_REAL:   a->T.size     += 1;
					a->T.basicType = T_REAL;
					a->R           = newRval();
					a->R->v.r      = 0.0;
					if (b->T.size >= 1 && c->T.size >= 1)
					{  if (b->R->v.r < c->R->v.r)
					      a->R->v.r = 1.0;
					   else
					      a->R->v.r = 0.0;
					   return(1);
					}
					else
					{
                                           execPError = 1;
                                           WerrprintfWithPos("Can't apply '<' to an array");
#ifdef VNMRJ
					   jExpressionError();
#endif 
					   return(0);
					}
			 case T_STRING: 
                                  execPError = 1;
                                  if (c->R->v.s != NULL)
                                       WerrprintfWithPos("Can't compare REAL %g  with STRING %s", b->R->v.r, c->R->v.s);
                                  else
                                        WerrprintfWithPos("Can't compare REAL with STRING");
#ifdef VNMRJ
					jExpressionError();
#endif 
					return(0);
		       }
	case T_STRING: switch (c->T.basicType)
		       {
                         default:
                         case T_UNDEF:  return(0);
			 case T_REAL:   
                                 execPError = 1;
                                 if (b->R->v.s != NULL)
                                     WerrprintfWithPos("Can't compare STRING %s with REAL %g", b->R->v.s, c->R->v.r);
                                 else
                                     WerrprintfWithPos("Can't compare STRING with REAL");
#ifdef VNMRJ
					jExpressionError();
#endif 
					return(0);
			 case T_STRING: a->T.size     += 1;
					a->T.basicType = T_REAL;
					a->R           = newRval();
					a->R->v.r      = 0.0;
					if (b->T.size >= 1 && c->T.size >= 1)
					{  if (strcmp(b->R->v.s,c->R->v.s) < 0)
					      a->R->v.r = 1.0;
					   else
					      a->R->v.r = 0.0;
					   return(1);
					}
					else
					{
                                           execPError = 1;
                                           if (b->R->v.s != NULL)
                                               WerrprintfWithPos("Can't apply '<' to an array %s", b->R->v.s);
                                           else
                                               WerrprintfWithPos("Can't apply '<' to an array");
#ifdef VNMRJ
					   jExpressionError();
#endif 
					   return(0);
					}
		       }
      }
   }
   else
      return(0);
}

/*------------------------------------------------------------------------------
|
|	do_GT/3
|
|	This procedure is used to check for greater than relation.  Note that
|	this is only defined for scalars!
|
+-----------------------------------------------------------------------------*/

int do_GT(pair *a, pair *b, pair *c)
{
   DSHOWLEFT(3, "do_GT", b);
   DSHOWRIGHT(3, "do_GT", c);
   if (b && c)
   {  switch (b->T.basicType)
      {
        default:
        case T_UNDEF:  return(0);
	case T_REAL:   switch (c->T.basicType)
		       {
                         default:
                         case T_UNDEF:  return(0);
			 case T_REAL:   a->T.size     += 1;
					a->T.basicType = T_REAL;
					a->R           = newRval();
					a->R->v.r      = 0.0;
					if (b->T.size >= 1 && c->T.size >= 1)
					{  if (b->R->v.r > c->R->v.r)
					      a->R->v.r = 1.0;
					   else
					      a->R->v.r = 0.0;
					   return(1);
					}
					else
					{
                                           execPError = 1;
                                           if (b->R->v.s != NULL)
                                              WerrprintfWithPos("Can't apply '>' to an array %s", b->R->v.s);
                                           else
                                              WerrprintfWithPos("Can't apply '>' to an array");
#ifdef VNMRJ
					   jExpressionError();
#endif 
					   return(0);
					}
			 case T_STRING: 
                                        execPError = 1;
                                        if (c->R->v.s != NULL)
                                           WerrprintfWithPos("Can't compare REAL %g with STRING %s", b->R->v.r, c->R->v.s);
                                        else
                                           WerrprintfWithPos("Can't compare REAL with STRING");
#ifdef VNMRJ
					jExpressionError();
#endif 
					return(0);
		       }
	case T_STRING: switch (c->T.basicType)
		       {
                         default:
                         case T_UNDEF:  return(0);
			 case T_REAL:   
                                        execPError = 1;
                                        if (b->R->v.s != NULL)
                                           WerrprintfWithPos("Can't compare STRING %s with REAL %g", b->R->v.s, c->R->v.r);
                                        else
                                           WerrprintfWithPos("Can't compare STRING with REAL");
#ifdef VNMRJ
					jExpressionError();
#endif 
					return(0);
			 case T_STRING: a->T.size     += 1;
					a->T.basicType = T_REAL;
					a->R           = newRval();
					a->R->v.r      = 0.0;
					if (b->T.size >= 1 && c->T.size >= 1)
					{  if (strcmp(b->R->v.s,c->R->v.s) > 0)
					      a->R->v.r = 1.0;
					   else
					      a->R->v.r = 0.0;
					   return(1);
					}
					else
					{ 
                                           execPError = 1;
                                           if (b->R->v.s != NULL)
                                              WerrprintfWithPos("Can't apply '>' to an array %s", b->R->v.s);
                                           else
                                              WerrprintfWithPos("Can't apply '>' to an array");
#ifdef VNMRJ
					   jExpressionError();
#endif 
					   return(0);
					}
		       }
      }
   }
   else
      return(0);
}

/*------------------------------------------------------------------------------
|
|	do_LE/3
|
|	This procedure is used to check equality, it returns true (non-zero
|	real value) if the two arguments are NOT greater-than.  Note that we
|	use do_GT/3 to do the dirty work, then just flip the results.
|
+-----------------------------------------------------------------------------*/

int do_LE(pair *a, pair *b, pair *c)
{
   DPRINT0(3,"do_LE: hand-off to do_GT...\n");
   if (do_GT(a,b,c))
   {  if (a->R->v.r == 0.0)
	 a->R->v.r = 1.0;
      else
	 a->R->v.r = 0.0;
      DPRINT1(3,"do_LE: ...ok! (result is %f)\n",a->R->v.r);
      return(1);
   }
   else
      return(0);
}

/*------------------------------------------------------------------------------
|
|	do_GE/3
|
|	This procedure is used to check equality, it returns true (non-zero
|	real value) if the two arguments are NOT less-than.  Note that we
|	use do_LT/3 to do the dirty work, then just flip the results.
|
+-----------------------------------------------------------------------------*/

int do_GE(pair *a, pair *b, pair *c)
{
   DPRINT0(3,"do_GE: hand-off to do_LT...\n");
   if (do_LT(a,b,c))
   {  if (a->R->v.r == 0.0)
	 a->R->v.r = 1.0;
      else
	 a->R->v.r = 0.0;
      DPRINT1(3,"do_GE: ...ok! (result is %f)\n",a->R->v.r);
      return(1);
   }
   else
      return(0);
}

/*------------------------------------------------------------------------------
|
|	do_PLUS/3
|
|	Add two "Rvals" giving third.  Strings are delt with in an interesting
|	manner: adding two strings gives the concatenation.  Adding two reals
|	gives the expected real sum.  Arrays are added by adding each element
|	the resulting array has the same number of elements as the smaller array
|	given.
|
+-----------------------------------------------------------------------------*/

int do_PLUS(pair *a, pair *b, pair *c)
{  Rval **app;
   Rval  *bp;
   Rval  *cp;

   DSHOWLEFT(3, "do_PLUS", b);
   DSHOWRIGHT(3, "do_PLUS", c);
   if (b && c)
   {  a->T.size      = 0;
      a->T.basicType = b->T.basicType;
      a->R           = NULL;
      app            = &(a->R);
      bp             = b->R;
      cp             = c->R;
      switch (b->T.basicType)
      {
        default:
        case T_UNDEF:  return(0);
	case T_REAL:   switch (c->T.basicType)
		       {
                         default:
                         case T_UNDEF:	return(0);
			 case T_REAL:   while (bp && cp)
					{  *app        = newRval();
					   (*app)->v.r = bp->v.r + cp->v.r;
					   app         = &((*app)->next);
					   bp          = bp->next;
					   cp          = cp->next;
					   a->T.size  += 1;
					}
                                        DSHOWRESULT(3, "do_PLUS", a);
					return(1);
			 case T_STRING: 
                                        execPError = 1;
                                        if (c->R->v.s != NULL)
                                            WerrprintfWithPos("Can't add a REAL and %g a STRING %s", b->R->v.r, c->R->v.s);
                                        else
                                            WerrprintfWithPos("Can't add a REAL and a STRING");
#ifdef VNMRJ
					jExpressionError();
#endif 
					return(0);
		       }
	case T_STRING: switch (c->T.basicType)
		       {
                         default:
                         case T_UNDEF:  return(0);
			 case T_REAL:   
                                        execPError = 1;
                                        if (b->R->v.s != NULL)
                                           WerrprintfWithPos("Can't add a STRING and %s a REAL %g", b->R->v.s, c->R->v.r);
                                        else
                                           WerrprintfWithPos("Can't add a STRING and a REAL");
#ifdef VNMRJ
					jExpressionError();
#endif 
					return(0);
			 case T_STRING: while (bp && cp)
					{  *app        = newRval();
					   (*app)->v.s = newCatIdDontTouch(bp->v.s,cp->v.s,"do_PLUS");
					   app         = &((*app)->next);
					   bp          = bp->next;
					   cp          = cp->next;
					   a->T.size  += 1;
					}
                                        DSHOWRESULT(3, "do_PLUS", a);
					return(1);
		       }
      }
   }
   else
      return(0);
}

/*------------------------------------------------------------------------------
|
|	do_MINUS/3
|
|	Subtract two "Rvals" giving third.  The result is always REAL.
|
+-----------------------------------------------------------------------------*/

int do_MINUS(pair *a, pair *b, pair *c)
{  Rval **app;
   Rval  *bp;
   Rval  *cp;

   DSHOWLEFT(3, "do_MINUS", b);
   DSHOWRIGHT(3, "do_MINUS", c);
   if (b && c)
   {  app            = &(a->R);
      a->T.size      = 0;
      a->T.basicType = T_REAL;
      a->R           = NULL;
      if (b->T.basicType==T_REAL && c->T.basicType==T_REAL)
      {  bp = b->R;
	 cp = c->R;
	 while (bp && cp)
	 {  *app        = newRval();
	    (*app)->v.r = bp->v.r - cp->v.r;
	    app         = &((*app)->next);
	    bp          = bp->next;
	    cp          = cp->next;
	    a->T.size  += 1;
	 }
         DSHOWRESULT(3, "do_MINUS", a);
	 return(1);
      }
      else
      {  if (b->T.basicType==T_STRING || c->T.basicType==T_STRING)
	 {
            execPError = 1;
            if (b->T.basicType==T_STRING && b->R->v.s != NULL)
                WerrprintfWithPos("Can't apply `-' to a STRING %s ", b->R->v.s);
            else if (c->T.basicType==T_STRING && c->R->v.s != NULL)
                WerrprintfWithPos("Can't apply `-' to a STRING %s ", c->R->v.s);
            else
                WerrprintfWithPos("Can't apply `-' to a STRING value");
#ifdef VNMRJ
	    jExpressionError();
#endif 
	 }
	 return(0);
      }
   }
   else
      return(0);
}

/*------------------------------------------------------------------------------
|
|	do_MULT/3
|
|	Multiply two "Rvals" giving third.  The result is always REAL. 
|
+-----------------------------------------------------------------------------*/

int do_MULT(pair *a, pair *b, pair *c)
{  Rval **app;
   Rval  *bp;
   Rval  *cp;

   DSHOWLEFT(3, "do_MULT", b);
   DSHOWRIGHT(3, "do_MULT", c);
   if (b && c)
   {  app            = &(a->R);
      a->T.size      = 0;
      a->T.basicType = T_REAL;
      a->R           = NULL;
      if (b->T.basicType==T_REAL && c->T.basicType==T_REAL)
      {  bp = b->R;
	 cp = c->R;
	 while (bp && cp)
	 {  *app        = newRval();
	    (*app)->v.r = bp->v.r * cp->v.r;
	    app         = &((*app)->next);
	    bp          = bp->next;
	    cp          = cp->next;
	    a->T.size  += 1;
	 }
         DSHOWRESULT(3, "do_MULT", a);
	 return(1);
      }
      else
      {  if (b->T.basicType==T_STRING || c->T.basicType==T_STRING)
	 {
            execPError = 1;
            if (b->T.basicType==T_STRING && b->R->v.s != NULL)
                WerrprintfWithPos("Can't apply `*' to a STRING %s ", b->R->v.s);
            else if (c->T.basicType==T_STRING && c->R->v.s != NULL)
                WerrprintfWithPos("Can't apply `*' to a STRING %s ", c->R->v.s);
            else
	        WerrprintfWithPos("Can't apply `*' to a STRING value");
#ifdef VNMRJ
	    jExpressionError();
#endif 
	 }
	 return(0);
      }
   }
   else
      return(0);
}

/*------------------------------------------------------------------------------
|
|	do_DIV/3
|
|	Divide two "Rvals" giving third.  The result is always REAL. 
|
+-----------------------------------------------------------------------------*/

int do_DIV(pair *a, pair *b, pair *c)
{  Rval **app;
   Rval  *bp;
   Rval  *cp;

   DSHOWLEFT(3, "do_DIV", b);
   DSHOWRIGHT(3, "do_DIV", c);
   if (b && c)
   {  app            = &(a->R);
      a->T.size      = 0;
      a->T.basicType = T_REAL;
      a->R           = NULL;
      if (b->T.basicType==T_REAL && c->T.basicType==T_REAL)
      {  bp = b->R;
	 cp = c->R;
	 while (bp && cp)
	 {  if (cp->v.r == 0)
	    {  
               WerrprintfWithPos("Division by zero!");
               execPError = 1;
#ifdef VNMRJ
	       jExpressionError();
#endif 
	       return(0);
	    }
	    *app        = newRval();
	    (*app)->v.r = bp->v.r / cp->v.r;
	    app         = &((*app)->next);
	    bp          = bp->next;
	    cp          = cp->next;
	    a->T.size  += 1;
	 }
         DSHOWRESULT(3, "do_DIV", a);
	 return(1);
      }
      else
      {  if (b->T.basicType==T_STRING || c->T.basicType==T_STRING)
	 {
            if (b->T.basicType==T_STRING && b->R->v.s != NULL)
                WerrprintfWithPos("Can't apply `/' to a STRING %s ", b->R->v.s);
            else if (c->T.basicType==T_STRING && c->R->v.s != NULL)
                WerrprintfWithPos("Can't apply `/' to a STRING %s ", c->R->v.s);
            else
	       WerrprintfWithPos("Can't apply `/' to a STRING value");
            execPError = 1;
#ifdef VNMRJ
	    jExpressionError();
#endif 
	 }
	 return(0);
      }
   }
   else
      return(0);
}

/*------------------------------------------------------------------------------
|
|	do_MOD/3
|
|	Modulo first "Rvals" by the second giving third.  
|       The result is always REAL. 
|
+-----------------------------------------------------------------------------*/

int do_MOD(pair *a, pair *b, pair *c)
{  Rval **app;
   Rval  *bp;
   Rval  *cp;

   DSHOWLEFT(3, "do_MOD", b);
   DSHOWRIGHT(3, "do_MOD", c);
   if (b && c)
   {  app            = &(a->R);
      a->T.size      = 0;
      a->T.basicType = T_REAL;
      a->R           = NULL;
      if (b->T.basicType==T_REAL && c->T.basicType==T_REAL)
      {  bp = b->R;
	 cp = c->R;
	 while (bp && cp)
	 {  if ( (int) cp->v.r == 0)
	    {  WerrprintfWithPos("Mod by zero!");
               execPError = 1;
#ifdef VNMRJ
	       jExpressionError();
#endif 
	       return(0);
	    }
	    *app        = newRval();
	    (*app)->v.r = (int)bp->v.r % (int)cp->v.r;
	    app         = &((*app)->next);
	    bp          = bp->next;
	    cp          = cp->next;
	    a->T.size  += 1;
	 }
         DSHOWRESULT(3, "do_MOD", a);
	 return(1);
      }
      else
      {  if (b->T.basicType==T_STRING || c->T.basicType==T_STRING)
	 {
            if (b->T.basicType==T_STRING && b->R->v.s != NULL)
                WerrprintfWithPos("Can't apply `MOD' to a STRING %s ", b->R->v.s);
            else if (c->T.basicType==T_STRING && c->R->v.s != NULL)
                WerrprintfWithPos("Can't apply `MOD' to a STRING %s ", c->R->v.s);
            else
	        WerrprintfWithPos("Can't apply `MOD' to a STRING value");
            execPError = 1;
#ifdef VNMRJ
	    jExpressionError();
#endif 
	 }
	 return(0);
      }
   }
   else
      return(0);
}

/*------------------------------------------------------------------------------
|
|	do_NEG/2
|
|	Negate an "Rvals" giving a second.  The result is always REAL.
|
+-----------------------------------------------------------------------------*/

int do_NEG(pair *a, pair *b)
{  Rval **app;
   Rval  *bp;

   DSHOWLEFT(3, "do_NEG", b);
   if (b)
   {  app            = &(a->R);
      a->T.size      = 0;
      a->T.basicType = T_REAL;
      a->R           = NULL;
      if (b->T.basicType==T_REAL)
      {  bp = b->R;
	 while (bp)
	 {  *app        = newRval();
	    (*app)->v.r = -(bp->v.r);
	    app         = &((*app)->next);
	    bp          = bp->next;
	    a->T.size  += 1;
	 }
         DSHOWRESULT(3, "do_NEG", a);
	 return(1);
      }
      else
      {  if (b->T.basicType == T_STRING)
	 {
            if (b->R->v.s != NULL)
	        WerrprintfWithPos("Can't negate a STRING %s", b->R->v.s);
            else
	        WerrprintfWithPos("Can't negate a STRING");
            execPError = 1;
#ifdef VNMRJ
	    jExpressionError();
#endif 
	 }
	 return(0);
      }
   }
   else
      return(0);
}

/*------------------------------------------------------------------------------
|
|	do_TRUNC/2
|
|	Truncate an "Rvals" giving a second.  The result is always REAL.
|
+-----------------------------------------------------------------------------*/

int do_TRUNC(pair *a, pair *b)
{  Rval **app;
   Rval  *bp;

   DSHOWLEFT(3, "do_TRUNC", b);
   if (b)
   {  app            = &(a->R);
      a->T.size      = 0;
      a->T.basicType = T_REAL;
      a->R           = NULL;
      if (b->T.basicType==T_REAL)
      {  bp = b->R;
	 while (bp)
	 {  *app        = newRval();
	    (*app)->v.r = (int)(bp->v.r);
	    app         = &((*app)->next);
	    bp          = bp->next;
	    a->T.size  += 1;
	 }
         DSHOWRESULT(3, "do_TRUNC", a);
	 return(1);
      }
      else
      {  if (b->T.basicType == T_STRING)
	 {
            if (b->R->v.s != NULL)
	        WerrprintfWithPos("Can't trunc a STRING %s", b->R->v.s);
            else
	        WerrprintfWithPos("Can't trunc a STRING");
            execPError = 1;
#ifdef VNMRJ
	    jExpressionError();
#endif 
	 }
	 return(0);
      }
   }
   else
      return(0);
}

/*------------------------------------------------------------------------------
|
|	do_SQRT/2
|
|       Take the square root of a "Rval" and gives a second.
|
+-----------------------------------------------------------------------------*/

int do_SQRT(pair *a, pair *b)
{  Rval **app;
   Rval  *bp;

   DSHOWLEFT(3, "do_SQRT", b);
   if (b)
   {  app            = &(a->R);
      a->T.size      = 0;
      a->T.basicType = T_REAL;
      a->R           = NULL;
      if (b->T.basicType==T_REAL)
      {  bp = b->R;
	 while (bp)
	 {  *app        = newRval();
	    (*app)->v.r = sqrt(bp->v.r);
	    app         = &((*app)->next);
	    bp          = bp->next;
	    a->T.size  += 1;
	 }
         DSHOWRESULT(3, "do_SQRT", a);
	 return(1);
      }
      else
      {  if (b->T.basicType == T_STRING)
	 {
            if (b->R->v.s != NULL)
	        WerrprintfWithPos("Can't trunc a STRING %s", b->R->v.s);
            else
	        WerrprintfWithPos("Can't trunc a STRING");
            execPError = 1;
#ifdef VNMRJ
	    jExpressionError();
#endif 
	 }
	 return(0);
      }
   }
   else
      return(0);
}

/*------------------------------------------------------------------------------
|
|	do_NOT/2
|
|	Not an "Rval" giving a second.  The result is always REAL.
|       If the value of the Rval is 0, the negation is 1. If the
|       value of the Rval is not 0, the negation is 0
|
+-----------------------------------------------------------------------------*/

int do_NOT(pair *a, pair *b)
{  Rval **app;
   Rval  *bp;

   DSHOWLEFT(3, "do_NOT", b);
   if (b)
   {  app            = &(a->R);
      a->T.size      = 0;
      a->T.basicType = T_REAL;
      a->R           = NULL;
      if (b->T.basicType==T_REAL)
      {  bp = b->R;
	 while (bp)
	 {  *app        = newRval();
	    if (bp->v.r)
	       (*app)->v.r = 0.0;
	    else
	       (*app)->v.r = 1.0;
	    app         = &((*app)->next);
	    bp          = bp->next;
	    a->T.size  += 1;
	 }
         DSHOWRESULT(3, "do_NOT", a);
	 return(1);
      }
      else
      {  if (b->T.basicType == T_STRING)
	 {
            if (b->R->v.s != NULL)
	        WerrprintfWithPos("Can't trunc a STRING %s", b->R->v.s);
            else
	        WerrprintfWithPos("Can't trunc a STRING");
            execPError = 1;
#ifdef VNMRJ
	    jExpressionError();
#endif 
	 }
	 return(0);
      }
   }
   else
      return(0);
}

/*------------------------------------------------------------------------------
|
|	do_AND/3
|
|	"AND" two "Rvals" giving third.  This is a logical AND (not bitwide).
|
+-----------------------------------------------------------------------------*/

int do_AND(pair *a, pair *b, pair *c)
{  Rval **app;
   Rval  *bp;
   Rval  *cp;

   DSHOWLEFT(3, "do_AND", b);
   DSHOWRIGHT(3, "do_AND", c);
   if (b && c)
   {  app            = &(a->R);
      a->T.size      = 0;
      a->T.basicType = T_REAL;
      a->R           = NULL;
      if (b->T.basicType==T_REAL && c->T.basicType==T_REAL)
      {  bp = b->R;
	 cp = c->R;
	 while (bp && cp)
	 {  *app        = newRval();
	    (*app)->v.r = bp->v.r && cp->v.r;
	    app         = &((*app)->next);
	    bp          = bp->next;
	    cp          = cp->next;
	    a->T.size  += 1;
	 }
         DSHOWRESULT(3, "do_PLUS", a);
	 return(1);
      }
      else
      {  if (b->T.basicType==T_STRING || c->T.basicType==T_STRING)
	 {
            if (b->T.basicType==T_STRING && b->R->v.s != NULL)
	        WerrprintfWithPos("Can't apply `AND' to STRING %s ", b->R->v.s);
            if (c->T.basicType==T_STRING && c->R->v.s != NULL)
	        WerrprintfWithPos("Can't apply `AND' to STRING %s ", c->R->v.s);
            else
	        WerrprintfWithPos("Can't apply `AND' to STRING value");
            execPError = 1;
#ifdef VNMRJ
	    jExpressionError();
#endif 
	 }
	 return(0);
      }
   }
   else
      return(0);
}

/*------------------------------------------------------------------------------
|
|	do_OR/3
|
|	"OR" two "Rvals" giving third.  This is a logical OR (not bitwide).
|
+-----------------------------------------------------------------------------*/

int do_OR(pair *a, pair *b, pair *c)
{  Rval **app;
   Rval  *bp;
   Rval  *cp;

   DSHOWLEFT(3, "do_OR", b);
   DSHOWRIGHT(3, "do_OR", c);
   if (b && c)
   {  app            = &(a->R);
      a->T.size      = 0;
      a->T.basicType = T_REAL;
      a->R           = NULL;
      if (b->T.basicType==T_REAL && c->T.basicType==T_REAL)
      {  bp = b->R;
	 cp = c->R;
	 while (bp && cp)
	 {  *app        = newRval();
	    (*app)->v.r = bp->v.r || cp->v.r;
	    app         = &((*app)->next);
	    bp          = bp->next;
	    cp          = cp->next;
	    a->T.size  += 1;
	 }
         DSHOWRESULT(3, "do_OR", a);
	 return(1);
      }
      else
      {  if (b->T.basicType==T_STRING || c->T.basicType==T_STRING)
	 {
            if (b->T.basicType==T_STRING && b->R->v.s != NULL)
	        WerrprintfWithPos("Can't apply `OR' to STRING %s ", b->R->v.s);
            if (c->T.basicType==T_STRING && c->R->v.s != NULL)
	        WerrprintfWithPos("Can't apply `OR' to STRING %s ", c->R->v.s);
            else
	        WerrprintfWithPos("Can't apply `OR' to STRING value");
            execPError = 1;
#ifdef VNMRJ
	    jExpressionError();
#endif 
	 }
	 return(0);
      }
   }
   else
      return(0);
}

/*------------------------------------------------------------------------------
|
|	do_SIZE/2
|
|	Size returns the number of elements of the variable name represented
|	by the string.  If the variable doesn't exist, the return integer
|	is zero.  The Rval always is successful
|
+-----------------------------------------------------------------------------*/

int do_SIZE(pair *a, pair *b)
{  varInfo *v;

   DSHOWLEFT(3, "do_SIZE", b);
   a->T.size          = 1;
   a->T.basicType     = T_REAL;
   a->R               = newRval();
   a->R->v.r          = 0;
   if (b)
   {  if (b->T.basicType == T_STRING)
      {  if ( (v = findVar(b->R->v.s)) )
	 {  a->R->v.r = v->T.size;
            DSHOWRESULT(3, "do_SIZE", a);
	    return(1);
	 }
	 else
	    return(1);
      }
      else
	 return(1);
   }
   else
      return(1);
}

/*------------------------------------------------------------------------------
|
|	do_TYPEOF/2
|
|	TYPEOF return a zero if the variable is a real or if it doesn't
|	exist and a non-zero if variable is a string.
|
+-----------------------------------------------------------------------------*/

int do_TYPEOF(pair *a, pair *b)
{  varInfo *v;

   DSHOWLEFT(3, "do_TYPEOF", b);
   a->T.size          = 1;
   a->T.basicType     = T_REAL;
   a->R               = newRval();
   a->R->v.r          = 0;
   if (b)
   {  if (b->T.basicType == T_STRING)
      {  if ( (v = findVar(b->R->v.s)) )
	 {  if (v->T.basicType == T_STRING)
	       a->R->v.r = 1.0;
            DSHOWRESULT(3, "do_TYPEOF", a);
	    return(1);
	 }
	 else
         {
	    return(0);
         }
      }
      else
	 return(0);
   }
   else
      return(0);
}
