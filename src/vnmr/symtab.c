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
|	symtab.c
|
|	These functions and procedures are used to create, access and maintain
|	binary name trees (general purpose symbol tables).  Routines are also
|	provided to balance such trees.
|
+-----------------------------------------------------------------------------*/

#include "symtab.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "allocate.h"
#include "symtab.h"
#include "tools.h"

extern void tossVar(register symbol *p);
extern int   Dflag;

int depth(symbol *p);
int deleteName(symbol **pp, symbol *q, const char *n);
/*------------------------------------------------------------------------------
|
|	showTable/3
|
|	This procedure is used to output a name table showing its tree
|	structure (for debugging and such).
|
+-----------------------------------------------------------------------------*/

void showTable(FILE *f, symbol *p, int d)
{   int i;

    if (p)
    {	showTable(f,p->left,d+1);
	if (p->val)
	    fprintf(f,"%3d: * ",depth(p));
	else
	    fprintf(f,"%3d:   ",depth(p));
	for (i=0; i<d; ++i)
	    fprintf(f,"  ");
	fprintf(f,"%s\n",p->name);
	showTable(f,p->right,d+1);
    }
}

/*------------------------------------------------------------------------------
|
|	depth/1
|
|	This function is used to calculate the height of a tree at a given node
|	(the depth to the most distant leaf).
|
+-----------------------------------------------------------------------------*/

int depth(symbol *p)
{  int Ld,Rd;

   if (p)
      if ((Ld=depth(p->left)) < (Rd=depth(p->right)))
	 return(Rd+1);
      else
	 return(Ld+1);
   else
      return(0);
}

/*------------------------------------------------------------------------------
|
|	balance/1
|
|	This procedure balances a given tree.  Nodes are juggled as req'd
|	to ensure a resulting balanced tree.
|
+-----------------------------------------------------------------------------*/

void balance(symbol **pp)
{  int Ld,Rd;
   symbol *A,*B,*C,*x,*y;

   if ( (A=(*pp)) )
   {  balance(&(A->left));
      balance(&(A->right));
      if (((Ld=depth(A->left))+2) <= (Rd=depth(A->right)))
      {  if (A->right->left)
	 {  B = A->right->left;
	    C = A->right;
	    y = B->right;
	 }
	 else
	 {  B = A->right;
	    C = B->right;
	    y = B->left;
	 }
	 x        = B->left;
	 (*pp)    = B;
	 A->right = x;
	 B->left  = A;
	 B->right = C;
	 C->left  = y;
	 balance(pp);
      }
      else
	 if ((Rd+2) <= Ld)
	 {  if (A->left->right)
	    {  B = A->left->right;
	       C = A->left;
	       x = B->left;
	    }
	    else
	    {  B = A->left;
	       C = B->left;
	       x = C->right;
	    }
	    y        = B->right;
	    (*pp)    = B;
	    C->right = x;
	    B->left  = C;
	    B->right = A;
	    A->left  = y;
	    balance(pp);
	 }
   }
}

/*------------------------------------------------------------------------------
|
|	findName/2
|
|	This function is used to search a given name tree for a given name.
|	If found, a pointer to the name table packet is returned.  If not,
|	NULL is returned.
|
+-----------------------------------------------------------------------------*/


symbol *findName(symbol *p, const char *n)
{  register int d;

   while (p)
      if ((d=strcmp(p->name,n)) == 0)
	 break;
      else
	 if (d < 0)
	    p = p->right;
	 else
	    p = p->left;
   return(p);
}

/*------------------------------------------------------------------------------
|
|	delName/2
|
|	This function is used delete a given name in a given tree. This
|	means deleting a variable and all its junk from a tree. Note
|	that a pointer to the root pointer of the tree must be given.  
|	A return code of 0 indicates success, -1 indicates failure.
|	This routine does not rebalances the tree after the delete.
|
+-----------------------------------------------------------------------------*/

int delName(symbol **pp, const char *n)
{   int ret;
   
    ret = deleteName(pp,*pp,n);
    return (ret);
}

/*------------------------------------------------------------------------------
|
|	delNameWithBalance/2
|
|	This function is used delete a given name in a given tree. This
|	means deleting a variable and all its junk from a tree. Note
|	that a pointer to the root pointer of the tree must be given.  
|	A return code of 0 indicates success, -1 indicates failure.
|	This routine rebalances the tree after the delete.
|
+-----------------------------------------------------------------------------*/

int delNameWithBalance(symbol **pp, const char *n)
{  int ret;
   
   ret = deleteName(pp,*pp,n);
   if (ret == 0)
      balance(pp);
   return(ret);
}

/*------------------------------------------------------------------------------
|
|	deleteName/3
|	
|	This function is called recursively until it finds the name
|	to delete. It then connects the left son (if it exists) to its
|	parents pointer and connects the right son to the right most
|	leaf of the left son.  If one or more of the sons do not exist
|	other logical things will happen.
|
+-----------------------------------------------------------------------------*/

int deleteName(symbol **pp, symbol *q, const char *n)
{   int      d;

    if (*pp)
	if ((d=strcmp(q->name,n)) == 0)
	{   if (q->left)
	    {	*pp = q->left;
		if (q->right)
		{   symbol *qp;

		    qp = q->left;
		    while (qp->right)
			qp = qp->right;
		    qp->right = q->right;
		}
	    }
	    else
		if (q->right)
		    *pp = q->right;      
		else
		    *pp = NULL;
	    tossVar(q); /* get rid of stuff */
	    return(0); /* success */
	}
	else
	    if (d < 0)
		return(deleteName(&q->right,q->right,n));
	    else
		return(deleteName(&q->left,q->left,n));
    else
	return(-1); /* failure, could not find name */
}


/*------------------------------------------------------------------------------
|
|	addName/2
|
|	This function is used add a given name to a given name table.  Note
|	that a pointer to the root pointer must be given.  A pointer to the
|	newly inserted name packet is returned.
|
+-----------------------------------------------------------------------------*/

symbol *addName(symbol **pp, const char *n)
{  register symbol *p;
   register symbol *q;

   while ( (p=(*pp)) )
   {  if (strcmp(p->name,n) < 0)
	 pp = &(p->right);
      else
	 pp = &(p->left);
   }
   if ( (q=(*pp)=(symbol *)allocateWithId(sizeof(symbol),"addName")) )
   {  q->name  = newString(n);
      q->val   = NULL;
      q->left  = NULL;
      q->right = NULL;
      return(q);
   }
   else
   {  fprintf(stderr,"symtab:out of memory\n");
      exit(1);
   }
}

/*------------------------------------------------------------------------------
|
|	BaddName/2
|
|	Just like addName/2, but balances after insert.
|
+-----------------------------------------------------------------------------*/

symbol *BaddName(symbol **pp, const char *n)
{   symbol *p;

    if ( (p=addName(pp,n)) )
	balance(pp);
    return(p);
}

/*------------------------------------------------------------------------------
|
|	firstName/2
|
|	finds the first name in the (sub)tree p.
|
+-----------------------------------------------------------------------------*/

symbol *firstName(symbol *p)
{   symbol *pptr;

    pptr = p;
    if (pptr)  {
      while (pptr->left)  
        pptr = pptr->left;
      }
    return(pptr);
}

/*------------------------------------------------------------------------------
|
|	nextName/2
|
|	finds the next name in the (sub)tree p after v
|
+-----------------------------------------------------------------------------*/

symbol *nextName(symbol *p, const char *n)
{   symbol *larger;
    int d;

   larger = NULL;
   while (p)  {
      d=strcmp(p->name,n);
      if (d <= 0)  {
	p = p->right;
	}
      else  {
	larger = p;
	p = p->left;
	}
      }
    return larger;
}
