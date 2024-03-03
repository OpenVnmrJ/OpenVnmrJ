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
|	assign.c
|
|	These routines assign real and strings values to a variable.
|	It also assigns enumeral values to variables.  These 
|	routines do not work with variable names, it works with 
|	varInfo pointers to the variable.  These are not user 
|	interface routines but are lower level routines. 
|
+-----------------------------------------------------------------------------*/
#include "group.h"
#include "params.h"
#include "variables.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "allocate.h"
#include "pvars.h"
#include "tools.h"
#include "wjunk.h"

#ifdef  DEBUG
extern int      Dflag;
#define DPRINT0(level, str) \
	if (Dflag >= level) fprintf(stderr,str)
#define DPRINT1(level, str, arg1) \
	if (Dflag >= level) fprintf(stderr,str,arg1)
#define DPRINT2(level, str, arg1, arg2) \
	if (Dflag >= level) fprintf(stderr,str,arg1,arg2)
#define DPRINT3(level, str, arg1, arg2, arg3) \
	if (Dflag >= level) fprintf(stderr,str,arg1,arg2,arg3)
#define DSHOWPAIR(level, arg1, arg2) \
	if (Dflag >= level) showPair(stderr,arg1,arg2)
#define DSHOWVAR(level, arg1, arg2) \
	if (Dflag >= level) showVar(stderr,arg1,arg2)
#else 
#define DPRINT0(level, str) 
#define DPRINT1(level, str, arg2) 
#define DPRINT2(level, str, arg1, arg2) 
#define DPRINT3(level, str, arg1, arg2, arg3) 
#define DSHOWPAIR(level, arg1, arg2) 
#define DSHOWVAR(level, arg1, arg2)
#endif 

#define MAXINT	    0x7FFFFFFF		/* maximum integer          */

extern Rval *addNewRval(varInfo *v);
extern Rval *addNewERval(varInfo *v);
extern Rval *selectRval(varInfo *v, int i);
extern Rval *selectERval(varInfo *v, int i);
extern void disposeRealRvals(Rval *r);
extern void disposeStringRvals(Rval *r);

#ifdef VNMRJ
extern void appendvarlist(char *);
extern void jExpressionError();
#endif

static varInfo storev;
static Tval    storeT;

void initStoredAttr(varInfo *v)
{
  storeT.basicType = v->T.basicType;
  storeT.size    = v->T.size;
  storev.active  = v->active;
  storev.Dgroup  = v->Dgroup;
  storev.Ggroup  = v->Ggroup;
  storev.subtype = v->subtype;
  storev.prot    = v->prot;
  storev.minVal  = v->minVal;
  storev.maxVal  = v->maxVal;
  storev.step    = v->step;
}

int compareStoredAttr(varInfo *v)
{
  int ret;

  if (storeT.basicType != v->T.basicType) ret = 1;
  else if (storeT.size    != v->T.size)   ret = 1;
  else if (storev.active  != v->active)   ret = 1;
  else if (storev.Dgroup  != v->Dgroup)   ret = 1;
  else if (storev.Ggroup  != v->Ggroup)   ret = 1;
  else if (storev.subtype != v->subtype)  ret = 1;
  else if (storev.prot    != v->prot)     ret = 1;
  else if (storev.minVal  != v->minVal)   ret = 1;
  else if (storev.maxVal  != v->maxVal)   ret = 1;
  else if (storev.step    != v->step)     ret = 1;
  else ret = 0;

  storeT.basicType = (short) 0;
  storeT.size    = (short) 0;
  storev.active  = (short) 0;
  storev.Dgroup  = (short) 0;
  storev.Ggroup  = (short) 0;
  storev.subtype = (short) 0;
  storev.prot    = (int)   0;
  storev.minVal  = (double)0.0;
  storev.maxVal  = (double)0.0;
  storev.step    = (double)0.0;

  return(ret);
}

/*------------------------------------------------------------------------------
|
|	compareValue/3
|
|	This procedure compares the current value of a variable with
|	the value to be assigned (with or without indexing).
|	See also assignString(), assignReal().
|
+-----------------------------------------------------------------------------*/

int compareValue(char *s, varInfo *v, int i)
{   Rval *q;
    int ret;

    if (v)
    {
	int type;
	type = (int)(v->T.basicType);
	if ((type != T_REAL) && (type != T_STRING))
	{   if (isReal(s))
		type = T_REAL;
	    else
		type = T_STRING;
	}
	if (i) /* variable name with index */
	{   if (i <= v->T.size+1)
	    {	if ((q=selectRval(v,i)) != NULL)
		{   switch( type )
		    {	case T_STRING: default:
	 		  if (strcmp(s, q->v.s) != 0)
			    ret = 1; 
			  else
			    ret = 2;
			  break;
			case T_REAL:
			  { char jstr[32];
			    sprintf(jstr,"%g",q->v.r);
/* compare strings instead of realString() using reals; more reliable */
			    if (strcmp(s,jstr) != 0)
			      ret = 1; 
			    else
			      ret = 2;
			  }
			  break;
		    }
		}
		else
		    ret = 1;
	    }
	    else
		ret = 3;
	}
	else /* variable without index */
	{   q = v->R;
	    if (q)
	    {	switch( type )
		{   case T_STRING: default:
		      if (strcmp(s, q->v.s) != 0)
			ret = 1; 
		      else
			ret = 2;
		      break;
		    case T_REAL:
		      { char jstr[32];
		        sprintf(jstr,"%g",q->v.r);
/* compare strings instead of realString() using reals; more reliable */
		        if (strcmp(s,jstr) != 0)
			  ret = 1;
		        else
			  ret = 2;
		      }
		      break;
		}
	    }
	    else
		ret = 1;
	}
    }
    else
	ret = 0;
    return(ret);
}

/*------------------------------------------------------------------------------
|
|	assignReal/3
|
|	This procedure assigns a real value to a variable (with or without
|	indexing).
|
+-----------------------------------------------------------------------------*/

int assignReal(double d, varInfo *v, int i)
{   Rval *q;

    DPRINT3(3,"assignReal: 0x%08x[%d] <== %lf",v,i,d);
    if (v)
    {   if (v->T.basicType == T_STRING)
	{   WerrprintfWithPos("Can't assign REAL value to STRING variable");
#ifdef VNMRJ
	    jExpressionError();
#endif 
	    return(0);
	}
	else
	{   if (v->T.basicType == T_UNDEF)
	    {
		DPRINT0(3,", newly defined as REAL");
		v->T.basicType = T_REAL;
	    }
	    if (i)
	    {
		DPRINT1(3,", just to element %d\n",i);
		if (i <= v->T.size+1)
		{   if ((q=selectRval(v,i)) == NULL)
		       q = addNewRval(v);
		    q->v.r = d;
		    /* v->active = ACT_ON; */
		    return(1);
		}
		else
		{   WerrprintfWithPos("Assignment to non-existent element (=%d)",i);
#ifdef VNMRJ
		    jExpressionError();
#endif 
		    return(0);
		}
	    }
	    else
	    {
		DPRINT0(3,", to whole thing\n");
		disposeRealRvals(v->R);
		v->T.size = 0;
		v->R      = NULL;
		q         = addNewRval(v);
		q->v.r    = d;
		/* v->active = ACT_ON; */
	    }
	    return(1);
	}
    }
    else
    {
	DPRINT0(3,", NULL var ptr\n");
	return(0);
    }
}

/*------------------------------------------------------------------------------
|
|	assignEReal/3
|
|	This procedure assigns a enum real value to a variable (with or without
|	indexing).
|
+-----------------------------------------------------------------------------*/

int assignEReal(double d, varInfo *v, int i)
{   Rval *q;

    DPRINT3(3,"assignEReal: 0x%08x[%d] <== %lf",v,i,d);
    if (v)
    {   if (v->ET.basicType == T_STRING)
	{   WerrprintfWithPos("Can't assign REAL value to STRING variable");
#ifdef VNMRJ
	    jExpressionError();
#endif 
	    return(0);
	}
	else
	{   if (v->ET.basicType == T_UNDEF)
	    {
		DPRINT0(3,", newly defined as REAL");
		v->ET.basicType = T_REAL;
	    }
	    if (i)
	    {
		DPRINT1(3,", just to element %d\n",i);
		if (i <= v->ET.size+1)
		{   if ((q=selectERval(v,i)) == NULL)
		       q = addNewERval(v);
		    q->v.r = d;
		    return(1);
		}
		else
		{   WerrprintfWithPos("Can't assign to non-existent element (%d)\n",i);
#ifdef VNMRJ
		    jExpressionError();
#endif 
		    return(0);
		}
	    }
	    else
	    {
		DPRINT0(3,", to whole thing\n");
		disposeRealRvals(v->E);
		v->ET.size = 0;
		v->E      = NULL;
		q         = addNewERval(v);
		q->v.r    = d;
	    }
	    return(1);
	}
    }
    else
    {
	DPRINT0(3,", NULL var ptr\n");
	return(0);
    }
}

/*------------------------------------------------------------------------------
|
|	assignString/3
|
|	This procedure assigns a string value to a variable (with or without
|	indexing).
|
+-----------------------------------------------------------------------------*/

int assignString(const char *s, varInfo *v, int i)
{   Rval *q;

    DPRINT3(3,"assignString: 0x%08x[%d] <== \"%s\"",v,i,s);
    if (v)
    {   if (v->T.basicType == T_REAL)
	{
            if (strcmp(s,"y") == 0)
              v->active = ACT_ON;
            else if (strcmp(s,"n") == 0)
              v->active = ACT_OFF;
            else
            {
              WerrprintfWithPos("Can't assign STRING value to REAL variable");
#ifdef VNMRJ
	      jExpressionError();
#endif 
	      return(0);
            }
	    return(1);
	}
	else
	{   if (v->T.basicType == T_UNDEF)
	    {
		DPRINT0(3,", newly defined as STRING");
		v->T.basicType = T_STRING;
	    }
	    if (i)
	    {
		DPRINT1(3,", just to element %d\n",i);
		if (i <= v->T.size+1)
		{   if ((q=selectRval(v,i)) == NULL)
		       q = addNewRval(v);
		    else
		       release(q->v.s);
		    q->v.s = newString(s);
		    /* v->active = ACT_ON; */
		    return(1);
		}
		else
		{   WerrprintfWithPos("Can't assign to non-existent element (%d)\n",i);
#ifdef VNMRJ
		    jExpressionError();
#endif 
		    return(0);
		}
	    }
	    else
	    {
		DPRINT0(3,", to whole thing\n");
		disposeStringRvals(v->R);
		v->T.size = 0;
		v->R      = NULL;
		q         = addNewRval(v);
		q->v.s    = newString(s);
/*		v->active = ACT_ON;					      */
	    }
	    return(1);
	}
    }
    else
	return(0);
}

/*------------------------------------------------------------------------------
|
|	assignEString/3
|
|	This procedure assigns an enum string value to a variable 
|	(with or without indexing).
|
+-----------------------------------------------------------------------------*/

int assignEString(char *s, varInfo *v, int i)
{   Rval *q;

    DPRINT3(3,"assignEString: 0x%08x[%d] <== \"%s\"",v,i,s);
    if (v)
    {   if (v->ET.basicType == T_REAL)
	{   WerrprintfWithPos("Can't assign STRING value to REAL variable");
#ifdef VNMRJ
	    jExpressionError();
#endif 
	    return(0);
	}
	else
	{   if (v->ET.basicType == T_UNDEF)
	    {
		DPRINT0(3,", newly defined as STRING");
		v->ET.basicType = T_STRING;
	    }
	    if (i)
	    {
		DPRINT1(3,", just to element %d\n",i);
		if (i <= v->ET.size+1)
		{   if ((q=selectERval(v,i)) == NULL)
		       q = addNewERval(v);
		    else
		       release(q->v.s);
		    q->v.s = newString(s);
		    return(1);
		}
		else
		{   WerrprintfWithPos("Can't assign to non-existent element (%d)\n",i);
#ifdef VNMRJ
		    jExpressionError();
#endif 
		    return(0);
		}
	    }
	    else
	    {
		DPRINT0(3,", to whole thing\n");
		disposeStringRvals(v->E);
		v->ET.size = 0;
		v->E      = NULL;
		q         = addNewERval(v);
		q->v.s    = newString(s);
	    }
	    return(1);
	}
    }
    else
	return(0);
}

/*------------------------------------------------------------------------------
|
|	checkParm
|
|	This procedure checks a paramter against it's maxVal, minVal, and
|       step size.  Two parameters (fn and sw) use special step size
|       calculations.
|
+-----------------------------------------------------------------------------*/

static void setfn(double *v)
{
  register int fn;
  register int i;

  i = (int) *v;
  if (i < 0)
    i = 2;
  fn=2;
  while (fn < i)
    fn <<= 1;
  *v = (double) fn;
}

static void setsw(double *v, double step)
{
  double sw;

  sw = 1.0 / (*v);
  sw = step * (double) ((int) ((sw + step / 2.0) / step));
  *v = 1.0 / sw;
}

void setmaxminstep(varInfo *v, double *value, char *name )
{
  double temp, round;
  double cval, maxv, minv, stepv;
  int pindex;
  int error = 0;

/*  If P_MMS bit set, use maxVal, minVal, etc. fields as indicies
    into system global paramters "parmax", "parmin", etc. extracting
    the desired value from the appropriate array.			*/

  cval = *value;
  if (v->prot & P_MMS) {
    pindex = (int) (v->minVal+0.1);
    if (P_getreal( SYSTEMGLOBAL, "parmin", &minv, pindex ))
     minv = -1.0e+30;
    pindex = (int) (v->maxVal+0.1);
    if (P_getreal( SYSTEMGLOBAL, "parmax", &maxv, pindex ))
     maxv = 1.0e+30;
    pindex = (int) (v->step+0.1);
    if (P_getreal( SYSTEMGLOBAL, "parstep", &stepv, pindex ))
     stepv = 0.0;
  }
  else {
    minv = v->minVal;
    maxv = v->maxVal;
    stepv = v->step;
  }

  if (cval > maxv)
  {
   /* Only report error if it is significant */
   if ( !((cval > 1.0) && ((cval - maxv) < 1e-10)) )
      error = 1; /* error for exceeding maximum */
   *value = maxv;
  }
  else if (cval < minv)
  {
   error = 2; /* error for below minimum */
   *value = minv;
  }

/* Two very special cases follow */

  if (stepv < -1.0)
   setfn( value );				/*  fn  */
  else if (stepv < 0.0)
   setsw( value, -stepv );			/*  sw  */
  else if (stepv != 0.0)
  {
    double minval;
    double sgn = 1.0;
    /*  This section needs to handle the following cases:
        Case 1:
        setlimit('val',1,-1,2) val=0 => val=1 and val=-1 => val=-1

        Case 2:
        setlimit('val',1e7,-1e7,0.1) val=0 => val=0,
        not some round-off value like 5.45e-15.

        Case 3:
        setlimit('val',1e7,-1e7,360/8192) should round-off
        correctly.  For this case, if the step size if < 1.0,
        then don't try to offset the value from the minimum,
        as is required for case 1.

        Case 4:
        setlimit('val',64000,-64000,1) for an integer like lsfid.
        Without case 4, lsfid=-8 yields -9.
        This is also true for reals with a step of 1
     */
    if (stepv < 1.0)
       minval = 0.0;      /* Case 3 */
    else
       minval = minv;     /* Case 1 */
    if ( ((v->subtype == ST_INTEGER) &&  (stepv < 1.5) ) || (stepv == 1.0) )
       minval = 0.0;      /* Case 4 */
    round = stepv / 2.0;
    if ( *value < 0.0)
    {
       sgn = -1.0;
       *value = - *value;
    }
    temp = ( (*value-minval) + round) / stepv;
    if (temp < MAXINT)
      *value = stepv * (double) ((int) temp) + minval;
    *value *= sgn;
    if (*value > maxv) *value -= stepv;
    else if (*value < minv) *value += stepv;
    if ( fabs(*value)  < round )  /* handle Case 2 */
    {
       *value = 0.0;
    }
  }
  if (v->subtype == ST_INTEGER)
    *value = (double) ((int) *value);
  if (error == 1)
     Winfoprintf("Parameter %s reset to maximum of %g",name, *value);
  else if (error == 2)
     Winfoprintf("Parameter %s reset to minimum of %g",name, *value);
#ifdef VNMRJ
  if (cval != *value)
    appendvarlist(name);
#endif 
}

int checkParm(pair *p, varInfo *v, char *name)
{ Rval *r;

  if (v)
  { switch (p->T.basicType)
    {
      case T_REAL:    if (v->T.basicType != T_STRING)
                      {
                         r = p->R;
		         while (r)
		         { 
                            setmaxminstep(v,&r->v.r, name);
	  		    r  = r->next;
		         }
		         return(1);
                      }
                      else
                      {
                         WerrprintfWithPos("Can't assign REAL value (%g) to STRING variable \"%s\"",
                                            p->R->v.r,name);
#ifdef VNMRJ
			 jExpressionError();
			 appendvarlist(name);
#endif 
		         return(0);
                      }
      case T_STRING:  if (v->ET.size)
		      {
		        r = p->R;
		        while (r)
		        { Rval *q;
		          register int found = 0;  /* assume no match */

                          if (v->subtype == ST_STRING)
                          {
		             q = v->E;
		             while ((q) && (!found))
		             {
		               found = (strcmp(r->v.s,q->v.s) == 0);
		               q = q->next;
		             }
		             if (!found)
			     {
                                WerrprintfWithPos("Parameter %s cannot be set to '%s'",
                                                   name, r->v.s);
#ifdef VNMRJ
				jExpressionError();
				appendvarlist(name);
#endif 
			     }
                          }
                          else if (v->subtype == ST_FLAG)
                          {
                             register int index = 0;
                             register int ok = 1;
                             register int size = strlen(r->v.s);
                             while ((index < size) && ok)
                             {
                                found = 0; /* assume no match */
		                q = v->E;
		                while ((q) && (!found))
		                {
                                   found = (r->v.s[index] == q->v.s[0]);
		                   q = q->next;
		                }
                                ok = found;
                                index++;
                             }
		             if (!found)
			     {
                                WerrprintfWithPos("Parameter %s cannot use character '%c'",
                                                   name, r->v.s[index - 1]);
#ifdef VNMRJ
				jExpressionError();
				appendvarlist(name);
#endif 
			     }
		          }
		          if (!found)
		            return(0);
		          r = r->next;
		        }
		        return(1);
		      }
		      else if ((v->T.basicType == T_REAL) &&
                              strcmp(p->R->v.s,"y") && strcmp(p->R->v.s,"n") )
                      {
                         WerrprintfWithPos("Can't assign STRING value \"%s\" to REAL variable \"%s\"",
                                            p->R->v.s,name);
#ifdef VNMRJ
			 jExpressionError();
			 appendvarlist(name);
#endif 
		         return(0);
                      }
		      else
			return(1);
      case T_UNDEF:
      default:        WerrprintfWithPos("Entry of parameter %s is undefined",name);
#ifdef VNMRJ
		      jExpressionError();
		      appendvarlist(name);
#endif 
                      return(0);

    }
  }
  return(0);
}

/*------------------------------------------------------------------------------
|
|	comparePair/3
|
|	This procedure compares the current value of a variable with
|	the value to be assigned, using a "pair" node.
|
+-----------------------------------------------------------------------------*/

int comparePair(pair *p, varInfo *v, int i)
{   Rval *r;
    int icomp;

/* Note that the return values of compareValue are different than assignReal and
      assignString, so the order of statements is different than assignPair.
   We could compare v->T.size with i while walking down r tree, but this may
      only be done in execS for case EQ, and not in placeMacReturns.          */

    if (v)
    {
	switch (p->T.basicType)
	{ case T_UNDEF: default:    /* unknown basictype */
			    return(0);
	  case T_REAL:      r = p->R;
		            if (v->prot & P_ARR)
		            { if ((r = r->next) || (i > 1)) /* cannot array parameter */
		              {
				 return(0);
		              }
			      r = p->R;
		            }
			    icomp = 1;
			    { char jstr[32];
			      while (r)
			      { sprintf(jstr,"%g",r->v.r);
			    	icomp = compareValue(jstr,v,i);
				switch (icomp)
				{ case 2:
				    if (i == 0)
				      i++; /* bump index to 2 */
				    i += 1;
				    if (r->next)
				      r = r->next;
				    else
				      return(0);
				    break;
				  case 1: case 3:
				    return(1);
				    break;
				  case 0: default:
				    return(0);
				    break;
				}
			      }
			    }
			    return(0);
	  case T_STRING:    r = p->R;
		            if (v->prot & P_ARR)
		            { if ((r = r->next) || (i > 1)) /* cannot array parameter */
		              {
				 return(0);
		              }
			      r = p->R;
			    }
			    icomp = 1;
			    { while (r)
			      {
				icomp = compareValue(r->v.s,v,i);
				switch (icomp)
				{ case 2:
				    if (i == 0)
				      i++; /* bump index to 2 */
				    i += 1;
				    if (r->next)
				      r = r->next;
				    else
				      return(0);
				    break;
				  case 1: case 3:
				    return(1);
				    break;
				  case 0: default:
				    return(0);
				    break;
				}
			      }
			    }
			    return(0);
	}
    }
    return(0);
}

/*------------------------------------------------------------------------------
|
|	assignPair/3
|
|	This procedure is used to make an assignment given a "pair" node.
|
+-----------------------------------------------------------------------------*/

int assignPair(pair *p, varInfo *v, int i)
{   Rval *r;

    DPRINT3(3,"assignPair: pair 0x%08x to var 0x%08x[%d]...\n",p,v,i);
    DSHOWPAIR(3,"assignPair: ",p);
    DSHOWVAR(3,"assignPair: ",v);
    if (v)
    {	switch (p->T.basicType)
	{ case T_UNDEF:     return(0);
	  case T_REAL:	    r = p->R;
		            if (v->prot & P_ARR)
		            { if ((r = r->next) || (i > 1))
		              {
				 WerrprintfWithPos("cannot array parameter");
				 return(0);
		              }
			      r = p->R;
		            }
			    while (r)
			    {
			    	if (assignReal(r->v.r,v,i))
				{   r  = r->next;
				    if (i == 0)
					i++; /* bump index to 2 */
				    i += 1;
				}
				else
				{
				    return(0);
				}
			    }
			    DPRINT0(3,"assignPair: ...after\n");
			    DSHOWVAR(3,"assignPair: ",v);
			    return(1);
	  case T_STRING:    r = p->R;
		            if (v->prot & P_ARR)
		            { if ((r = r->next) || (i > 1))
		              {
				 WerrprintfWithPos("cannot array parameter");
				 return(0);
		              }
			      r = p->R;
			    }
			    while (r)
			    	if (assignString(r->v.s,v,i))
				{   r  = r->next;
				    if (i == 0)
					i++; /* bump index to 2 */
				    i += 1;
				}
				else
				    return(0);
			    DPRINT0(3,"assignPair: ...after\n");
			    DSHOWVAR(3,"assignPair: ",v);
			    return(1);
	  default:	    WerrprintfWithPos("unknown parameter assignment with basictype %d",p->T.basicType);
			    return(0);

	}
    }
    return(0);
}

/*------------------------------------------------------------------------------
|
|	appendPair/2
|
|	This function is used to append one pair to another (forming or
|	lengthening an array).  Notice that links are juggled, Rvals are
|	not copied!  This is because all Rvals attached to a "pair" are
|	not part of a variable!  Rvals are copied from variables (see
|	execR/2 case IDR and LBR).
|
+-----------------------------------------------------------------------------*/

int appendPair(pair *d, pair *s)
{   if (d->T.basicType == s->T.basicType)
	switch (s->T.basicType)
	{ case T_UNDEF:	    return(1);
	  case T_REAL:
	  case T_STRING:    {	Rval *r;
	  
				if ( (r=d->R) )
				{   while (r->next)
					r = r->next;
				    r->next = s->R;
				}
				else
				    d->R = s->R;
				d->T.size += s->T.size;
				s->R       = NULL;
				return(1);
			    }
	  default:	    WerrprintfWithPos("unknown parameter basictype %d",s->T.basicType);
			    return(0);
	}
    else
    {	WerrprintfWithPos("array members must all be strings or all be reals");
	return(0);
    }
}
