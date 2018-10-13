/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include "group.h"
#include "symtab.h"
#include "params.h"
#include "tools.h"
#include "abort.h"
#include "pvars.h"
#define VNMRJ
#include "variables.h"

#define OK 0
#define MAXSTR 256

extern int      bgflag;

static  void getNames(symbol *s, char **nameptr, int *numvar, int maxptr, int *i);
static void getValues(symbol *s, char **nameptr, int *numvar, int maxptr, int *i);

/*------------------------------------------------------------------------------
|
|	whattype(tree,name)
|
|	This function returns the address of a real value of a variable based on
+----------------------------------------------------------------------------*/
int whattype(int tree, const char *name)
{
   int             ret;
   vInfo           varinfo;	/* variable information structure */

   if ( (ret = P_getVarInfo(tree, name, &varinfo)) )
   {
      text_error("Cannot find the variable: %s", name);
      if (bgflag)
	 P_err(ret, name, ": ");
      return (-1);
   }
   return (varinfo.basicType);
}

/*------------------------------------------------------------------------------
|
|	parmult(tree,name)
|
|	This function returns the multiplier of a real parameter
+----------------------------------------------------------------------------*/
double parmult(int tree, const char *name)
{
   int             ret;
   vInfo           varinfo;	/* variable information structure */

   if ( (ret = P_getVarInfo(tree, name, &varinfo)) )
   {
      text_error("parmult(): cannot find variable %s", name);
      if (bgflag)
	 P_err(ret, name, ": ");
      return (-1.0);
   }
   return( (varinfo.subtype == ST_PULSE) ? 1e-6 : 1.0);
}

/*------------------------------------------------------------------------------
|
|	maxminlimit(tree,name)
|
|	This function returns the max & min limit of the real variable based
|				Author: Greg Brissey  8-18-95
+----------------------------------------------------------------------------*/
int par_maxminstep(int tree, const char *name, double *maxv, double *minv, double *stepv)
{
   int             ret,pindex;
   vInfo           varinfo;	/* variable information structure */

   if ( (ret = P_getVarInfo(tree, name, &varinfo)) )
   {
      text_error("Cannot find the variable: %s", name);
      if (bgflag)
	 P_err(ret, name, ": ");
      return (-1);
   }
   if (varinfo.basicType != ST_REAL)
   {
      text_error("The variable '%s' is not a type 'REAL'", name);
   }
   if (varinfo.prot & P_MMS)
   {
      pindex = (int) (varinfo.minVal+0.1);
      if (P_getreal( SYSTEMGLOBAL, "parmin", minv, pindex ))
         *minv = -1.0e+30;
      pindex = (int) (varinfo.maxVal+0.1);
      if (P_getreal( SYSTEMGLOBAL, "parmax", maxv, pindex ))
         *maxv = 1.0e+30;
      pindex = (int) (varinfo.step+0.1);
      if (P_getreal( SYSTEMGLOBAL, "parstep", stepv, pindex ))
            *stepv = 0.0;
   }
   else
   {
       *maxv = varinfo.maxVal;
       *minv = varinfo.minVal;
       *stepv = varinfo.step;
   }
   return (0);
}

/*------------------------------------------------------------------------------
|
|	A_getreal/4
|
|	This function returns the address of a real value of a variable based on
|	the index.  The first element starts at index 1.
|
+-----------------------------------------------------------------------------*/

int
A_getreal(int tree, const char *name, double **valaddr, int index)
{
   symbol        **root;

   if ( (root = getTreeRoot(getRoot(tree))) )
   {
      varInfo        *v;

      if ( (v = rfindVar(name, root)) )	/* if variable exists */
      {
	 if (0 < index && index <= v->T.size)	/* within bounds ? */
	 {
	    int             i;
	    Rval           *r;

	    i = 1;
	    r = v->R;
	    while (r && i < index)	/* travel down Rvals  */
	    {
	       r = r->next;
	       i += 1;
	    }
	    if (r)
	    {
	       *valaddr = &r->v.r;	/* pass address back */
	       if (bgflag > 1)
	       {
		  fprintf(stderr,
		     "A_getstring: Address to put address of value: %p , ",
			  (void *) valaddr);
		  fprintf(stderr, "Address of value: %p \n",
			  (void *) *valaddr);
	       }
	       return (0);
	    }
	    else
	    {
	       return (-99);	/* unknown error */
	    }
	 }
	 else
	    return (-9);	/* index out of bounds */
      }
      else
	 return (-2);		/* variable doesn't exist */
   }
   else
      return (-1);		/* tree doesn't exist */
}

/*------------------------------------------------------------------------------
|
|	A_getstring/5
|
|	This function returns the address of a string value of a
|	variable based on the index.
|	The first element starts at index 1.
|	It will add a null at the end of the buffer for saftey.
|
+-----------------------------------------------------------------------------*/

int A_getstring(int tree, const char *name, char **straddr, int index)
{
   symbol        **root;

   if ( (root = getTreeRoot(getRoot(tree))) )
   {
      varInfo        *v;

      if ( (v = rfindVar(name, root)) )	/* if variable exists */
      {
	 if (0 < index && index <= v->T.size)	/* within bounds ? */
	 {
	    int             i;
	    Rval           *r;

	    i = 1;
	    r = v->R;
	    while (r && i < index)	/* travel down Rvals  */
	    {
	       r = r->next;
	       i += 1;
	    }
	    if (r)
	    {
	       *straddr = r->v.s;
	       if (bgflag > 1)
	       {
		  fprintf(stderr,
		    "A_getstring: Address to put address of string: %p , ",
			  (void *) straddr);
		  fprintf(stderr, "Address of string: %p \n",
			  (void *) *straddr);
	       }
	       return (0);
	    }
	    else
	    {
	       return (-99);	/* unknown error */
	    }
	 }
	 else
	    return (-9);	/* index out of bounds */
      }
      else
	 return (-2);		/* variable doesn't exist */
   }
   else
      return (-1);		/* tree doesn't exist */
}

/*------------------------------------------------------------------------------
|
|	A_getnames/4
|
|	This function loads an array of pointers to character strings with
|	pointers to the names of variables in a tree.  It sets numvar to
|	the number of variables in the tree.
|	If array of pointers address is 0 then just count variables
|
+-----------------------------------------------------------------------------*/

int A_getnames(int tree, char **nameptr, int *numvar, int maxptr)
{
   int             i;
   symbol        **root;

   i = 0;
   *numvar = 0;
   if ( (root = getTreeRoot(getRoot(tree))) )
   {
      getNames(*root, nameptr, numvar, maxptr, &i);
   }
   else
      return (-1);		/* tree doesn't exist */
   return (OK);
}

/*----------------------------------------------------------------------------
|
|	getNames/5
|
|	This modules recursively travels down a tree, sets an array of
|	pointer to the variables name strings and keeps count of them.
+-----------------------------------------------------------------------------*/

static  void
getNames(symbol *s, char **nameptr, int *numvar, int maxptr, int *i)
{
   if (s)
   {
      getNames(s->left, nameptr, numvar, maxptr, i);
      if (*i < maxptr)
      {
	 /* v = (varInfo *)s->val; */
	 /* if (v->T.size > 1) */
	 {
	    if (nameptr != NULL)/* if nameptr is null just count vars */
	    {
	       nameptr[*i] = s->name;
	       *i += 1;
	    }
	    *numvar += 1;
	 }
	 if (bgflag > 1)
	    fprintf(stderr, "name: '%s', i = %d, number: %d \n",
		    s->name, *i, *numvar);
      }
      else
      {
	 *numvar = -1;		/* mark error */
      }
      getNames(s->right, nameptr, numvar, maxptr, i);
   }
}

/*------------------------------------------------------------------------------
|
|	A_getvalues/4
|
|	This function loads an array of pointers to values with
|	pointers to the values of variables in a tree.  It sets numvar to
|	the number of variables in the tree.
|	If array of pointers address is 0 then just count variables
|
+-----------------------------------------------------------------------------*/

int
A_getvalues(int tree, char **nameptr,  int *numvar, int maxptr)
{
   int             i;
   symbol        **root;

   i = 0;
   *numvar = 0;
   if ( (root = getTreeRoot(getRoot(tree))) )
   {
      getValues(*root, nameptr, numvar, maxptr, &i);
   }
   else
      return (-1);		/* tree doesn't exist */
   return (OK);
}

/*----------------------------------------------------------------------------
|
|	getValues/5
|
|	This modules recursively travels down a tree, sets an array of
|	pointer to the variables values and keeps count of them.
+-----------------------------------------------------------------------------*/

static void 
getValues(symbol *s, char **nameptr, int *numvar, int maxptr, int *i)
{
   varInfo        *v;

   if (s)
   {
      getValues(s->left, nameptr, numvar, maxptr, i);
      if (*i < maxptr)
      {
	 Rval           *r;

	 v = (varInfo *) s->val;
	 r = v->R;
	 if (v->T.basicType == T_REAL)
	 {
	    if (nameptr != NULL)/* if nameptr is 0  count vars */
	    {
	       nameptr[*i] = (char *) &r->v.r;	/* pass address of real  */
	       *i += 1;
	    }
	    *numvar += 1;
	 }
	 else
	 {
	    if (nameptr != NULL)/* if nameptr is null just count vars */
	    {
	       nameptr[*i] = r->v.s;
	       *i += 1;
	    }
	    *numvar += 1;
	 }
	 if (bgflag > 1)
	    fprintf(stderr, "name: '%s', i = %d, number: %d \n",
		    s->name, *i, *numvar);
      }
      else
      {
	 *numvar = -1;		/* mark error */
      }
      getValues(s->right, nameptr, numvar, maxptr, i);
   }
}

/*------------------------------------------------------------------------------
|
|	getvalue(name,varible)
|
|	This function returns the value of the  variable
+----------------------------------------------------------------------------*/
/*
getvalue(name,value)
char *name;
double *value;
{
    type = whattype(CURRENT,name);
    if (type == ST_REAL)
    {
    if (bgflag)
      fprintf(stderr,
    "SETUP: #var = %d  var name: '%s', value = %5.2lf, &value = %lx \n",
		lpel[lpindex]->numvar,lpel[lpindex]->lpvar[varindex],
		*( *((double **) lpel[lpindex]->varaddr[varindex,0])),
		(lpel[lpindex]->varaddr[varindex,0]));
    *value = *( *((double **) lpel[lpindex]->varaddr[varindex,0])),
    }
    else
    {
    if (bgflag)
      fprintf(stderr,
    "SETUP: #var = %d  var name: '%s', value = '%s', &value = %lx \n",
		lpel[lpindex]->numvar,lpel[lpindex]->lpvar[varindex],
		*((char **) lpel[lpindex]->varaddr[varindex,0]),
		(lpel[lpindex]->varaddr[varindex,0]));
    }
}
*/
