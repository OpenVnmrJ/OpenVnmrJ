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
|	variables.c
|
|	These procedures are used to create and maintain the various symbol
|	tables containing variables.
|
|	This is the real meat of the variable system. This module contains
|	routine to setup variable tree systems, values, parameters, create,
|	dispose, conversion routines between ascii names and addresses,
|	handling temporary $ variables, pushing and popping trees for nesting
|	macro variable trees, initializing variable parameters and alot of
|	other things. 
|
|   Modified   Author     Purpose
|   --------   ------     -------
|   3/1/89     Greg B.    1. Changed presetVal() to set the Protection field from 
|			     the preset structure 
|                         2. Change RshowVar() so that if the Parameter has indexed
|			     Max,Min,Step values, the index and value are printed 
|			     instead of just the index.
|			     Order of Min Max was swapped to consistent with
|			     setlimit() order of Max,Min,Step
|   4/19/89    Greg B.    1. Changed RshowVar() printf format to %1.12g for proper
|			     displaying of parameters. 
+-----------------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifdef VNMRJ
#include "graphics.h"
#endif
#include "group.h"
#include "params.h"
#include "symtab.h"
#include "variables.h"
#include "init.h"  /* must come after variables.h */
#include "allocate.h"  /* must come after variables.h */
#include "pvars.h"
#include "tools.h"
#include "wjunk.h"

#define letter(c) ((('a'<=(c))&&((c)<='z'))||(('A'<=(c))&&((c)<='Z'))||((c)=='_')||((c)=='$')||((c)=='#')||((c)=='/'))
#define digit(c) (('0'<=(c))&&((c)<='9'))

extern symbol *addName(symbol **pp, const char *n);
extern symbol *BaddName(symbol **pp, const char *n);
extern symbol *findName(symbol *p, const char *n);
extern int assignReal(double d, varInfo *v, int i);
extern int assignString(const char *s, varInfo *v, int i);
extern int  writelineToVnmrJ(const char *cmd, const char *message );

static int      level = -1;
static symbol  *current;	/* current tree root */
static symbol  *global;		/* global tree root */
static symbol  *local[256];
static symbol  *processed;	/* processed tree root */
static symbol  *systemglobal;	/* systemglobal tree root */
static symbol  *temporary;	/* temporary tree root */
static symbol  *usertree;	/* user tree root */
static symbol  *temp;

static void presetVal(varInfo *v, int type);

#ifdef  DEBUG
extern int      Dflag;
#define DPRINT0(level, str) \
	if (Dflag >= level) fprintf(stderr,str)
#define DPRINT1(level, str, arg1) \
	if (Dflag >= level) fprintf(stderr,str,arg1)
#define DPRINT2(level, str, arg1, arg2) \
	if (Dflag >= level) fprintf(stderr,str,arg1,arg2)
#define DSHOWPAIR(level, arg1, arg2) \
	if (Dflag >= level) showPair(stderr,arg1,arg2)
#define DSHOWVAR(level, arg1, arg2) \
	if (Dflag >= level) showVar(stderr,arg1,arg2)
#else 
#define DPRINT0(level, str) 
#define DPRINT1(level, str, arg2) 
#define DPRINT2(level, str, arg1, arg2) 
#define DSHOWPAIR(level, arg1, arg2) 
#define DSHOWVAR(level, arg1, arg2)
#endif 

/*------------------------------------------------------------------------------
|
|	newVar/0
|
|	This function returns a pointer to a fresh varInfo packet.
|
+-----------------------------------------------------------------------------*/

varInfo *newVar()
{  varInfo *v;

   if ((v=(varInfo *)allocateWithId(sizeof(varInfo),"newVar")) == NULL)
   {  fprintf(stderr,"FATAL! out of memory\n");
      exit(1);
   }
   else
   {
      DPRINT1(4,"newVar: varInfo pkt at 0x%06x\n",v);
      return(v);
   }
}

/*------------------------------------------------------------------------------
|
|	getRoot/1
|
|	This function returns a pointer to the name of a tree
|	based on the tree index.
|
+-----------------------------------------------------------------------------*/

const char *getRoot(int index)
{   
    switch (index)
    { case CURRENT:	return("current");
      case GLOBAL:	return("global");
      case PROCESSED:	return("processed");
      case TEMPORARY:	return("temporary");
      case SYSTEMGLOBAL:return("systemglobal");
      case USERTREE:    return("usertree");
      default:		return("unknown");
    }
}

/*------------------------------------------------------------------------------
|
|	getTreeIndex/1
|
|	This function returns the tree index number based on symbolic name
|
+-----------------------------------------------------------------------------*/

int getTreeIndex(const char *n)
{  if (strcmp(n,"current") == 0)      return(CURRENT);
   if (strcmp(n,"global") == 0)       return(GLOBAL);
   if (strcmp(n,"processed") == 0)    return(PROCESSED);
   if (strcmp(n,"temporary") == 0)    return(TEMPORARY);
   if (strcmp(n,"systemglobal") == 0) return(SYSTEMGLOBAL);
   if (strcmp(n,"usertree") == 0)     return(USERTREE);
   return(-1);
}

/*------------------------------------------------------------------------------
|
|	setTreeRootByIndex/1
|
|	This function set a pointer to the proper root pointer (if any)
|	for a given tree index (current,global,processed,temporary).
|
+-----------------------------------------------------------------------------*/

int setTreeRootByIndex(int index, symbol *root)
{
   switch (index)
   { case CURRENT:   current = root;
                     return(1);

     case GLOBAL:    global = root;
                     return(1);

     case PROCESSED: processed = root;
                     return(1);

     case TEMPORARY: temporary = root;
                     return(1);

     case SYSTEMGLOBAL: systemglobal = root;
                     return(1);

     case USERTREE:  usertree = root;
                     return(1);

     default:        return(0);
   }
}

/*------------------------------------------------------------------------------
|
|	getTreeRootByIndex/1
|
|	This function returns a pointer to the proper root pointer (if any)
|	for a given tree index (current,global,processed,temporary).
|
+-----------------------------------------------------------------------------*/

symbol **getTreeRootByIndex(int index)
{
   switch (index)
   { case CURRENT:   return(&current);
     case GLOBAL:    return(&global);
     case PROCESSED: return(&processed);
     case TEMPORARY: return(&temporary);
     case SYSTEMGLOBAL: return(&systemglobal);
     case USERTREE:  return(&usertree);
     default:        return(NULL);
   }
}

/*------------------------------------------------------------------------------
|
|	getTreeRoot/1
|
|	This function returns a pointer to the proper root pointer (if any)
|	for a given tree name (current,global,processed,temporary).
|
+-----------------------------------------------------------------------------*/

symbol **getTreeRoot(const char *n)
{  if (strcmp(n,"current") == 0)   return(&current);
   if (strcmp(n,"global") == 0)    return(&global);
   if (strcmp(n,"processed") == 0) return(&processed);
   if (strcmp(n,"temporary") == 0) return(&temporary);
   if (strcmp(n,"systemglobal") == 0) return(&systemglobal);
   if (strcmp(n,"usertree") == 0)  return(&usertree);
   return(NULL);
}


/*------------------------------------------------------------------------------
|
|	selectVarTree/1
|
|	This function returns a pointer to the proper root pointer (if any)
|	for a given variable name.
|
+-----------------------------------------------------------------------------*/

symbol **selectVarTree(char *n)
{  if (*n == '$')/* for a local variable, return pointer to local tree */
      if (0 <= level)
	 return(&(local[level]));
      else
	 return(&current);
   else
      if (*n == '%') /* return temp tree pointer. Used for passing params */
	 return(&temp);
      else
	 return(&current);
}

/*------------------------------------------------------------------------------
|
|	disposeRealRvals/1
|
|	This procedure disposes a chain of Rval packets representing reals,
|	no special release is need for real Rvals.
|
+-----------------------------------------------------------------------------*/

void disposeRealRvals(Rval *r)
{
   Rval *temp;

   while (r)
   {
      temp = r->next;
      release(r);
      r = temp;
   }
}

/*------------------------------------------------------------------------------
|
|	disposeStringRvals/1
|
|	This procedure disposes a chain of Rval packets representing strings,
|	before this is done, each attached string (if any) is released.
|
+-----------------------------------------------------------------------------*/

void disposeStringRvals(Rval *r)
{
   Rval *temp;

   while (r)
   {  if (r->v.s)
      {
	  DPRINT2(4,"disposeStringRvals: release string \"%s\" (at 0x%08x)\n",
                  r->v.s,r->v.s);
	  release(r->v.s);
      }
      temp = r->next;
      release(r);
      r = temp;
   }
}

/*------------------------------------------------------------------------------
|
|	tossVar/1
|
|	Release storage tied up with a single variable 
|
+-----------------------------------------------------------------------------*/

void tossVar(register symbol *p)
{  register varInfo *v;

   if (p)
   {
      DPRINT1(3,"tossVar: symbol 0x%06x",p);
      if (p->name)
      {
	 DPRINT2(3,", name is \"%s\" (at 0x%06x)",p->name,p->name);
	 release(p->name);
      }
      if ( (v=(varInfo *)(p->val)) )
      {
	 DPRINT1(3,", varInfo 0x%06x",v);
	 if (v->T.basicType == T_STRING)
	 {
	    DPRINT0(3,", STRING\n");
	    disposeStringRvals(v->R);
	    disposeStringRvals(v->E);
	 }
	 else
	 {
	    DPRINT0(3,", REAL\n");
	    disposeRealRvals(v->R);
	    disposeRealRvals(v->E);
	 }
#ifndef VNMRJ
#ifdef VNMRTCL
         /* This function is used by the TCL interface
          * The function is in socket.c in the acqcomm library
          */
         unsetMagicVar(v);
#endif 
#endif 
	 release(v);
      }
      release(p);
   }
}

/*------------------------------------------------------------------------------
|
|	tossVars/1
|
|	Release storage tied up in a local symbol table.
|
+-----------------------------------------------------------------------------*/

void tossVars(symbol **pp)
{  register symbol  *p;

   if ( (p=(*pp)) )
   {  if (p->left)
	 tossVars(&(p->left));
      if (p->right)
	 tossVars(&(p->right));
      tossVar(p);
      (*pp) = NULL;
   }
}

/*------------------------------------------------------------------------------
|
|	tossTemps/0
|
|	Release storage tied up in the temp symbol table (used when done
|	using the temp table for holding return values).
|
+-----------------------------------------------------------------------------*/

void tossTemps()
{
    DPRINT0(3,"tossTemps: just like it says\n");
    tossVars(&temp);
}


/*------------------------------------------------------------------------------
|
|	fixNames/1
|
|	Race through a tree and rename those that start with '%' to start with
|	'$'.  Called by pushTree/0 when the temp tree is moved to be the new
|	local tree.
|
+-----------------------------------------------------------------------------*/

void fixNames(register symbol *p)
{  if (p)
   {  fixNames(p->left);
      if (p->name[0] == '%')
	 p->name[0] = '$';
      fixNames(p->right);
   }
}

/*------------------------------------------------------------------------------
|
|	pushTree/0
|
|	Push to a new local environment.
|
+-----------------------------------------------------------------------------*/

void pushTree()
{   if ((level+1) < 256)
    {
	DPRINT2(3,"pushTree: from level %d to %d\n",level,level+1);
	level       += 1;
	local[level] = temp;
	temp         = NULL;
	fixNames(local[level]);
	DPRINT0(3,"pushTree: ...done\n");
    }
    else
    {	fprintf(stderr,"magic: environment nesting is too deep\n");
	exit(1);
    }
}

/*------------------------------------------------------------------------------
|
|	popTree/0
|
|	Pop to a previous local environment.
|
+-----------------------------------------------------------------------------*/

void popTree()
{   if (0 <= level)
    {	tossVars(&(local[level]));
	local[level] = NULL;
	level       -= 1;
	DPRINT1(3,"popTree: back to level %d\n",level);
    }
    else
	fprintf(stderr,"magic: POP without PUSH\n");
}

/*------------------------------------------------------------------------------
|
|	rfindVar/2
|
|	This function returns a pointer to the variable info packet for a
|	given variable name and given tree root.  Returns NULL if none found.
|
+-----------------------------------------------------------------------------*/

varInfo *rfindVar(const char *n, symbol **pp)
{   symbol  *p;

    if ( (p=findName((*pp),n)) )
	return((varInfo *)(p->val));
    else
	return(NULL); 
}

/*------------------------------------------------------------------------------
|
|	findVar/1
|
|	This function returns a pointer to the variable info packet for a
|	given variable name.  Returns NULL if none found.
|
+-----------------------------------------------------------------------------*/

varInfo *findVar(const char *n)
{  symbol **pp;
   varInfo *v;

   if ( (pp=selectVarTree(n)) )
   {  if (pp == &current)	/* if select tree is current, check it first */
      {  if ( (v = rfindVar(n,pp)) )
	    return(v);
	 else
	    if ( (v = rfindVar(n,&global)) )		/*  then check global */
	       return(v);
	    else
	       if ( (v = rfindVar(n,&systemglobal)) )  /*then check systemglobal */
	          return(v);
	       else
	          return(NULL);
      }
      else
	 return(rfindVar(n,pp));
   }
   else
      return(NULL);
}

/*------------------------------------------------------------------------------
|
|	newRval/0
|
|	This function allocates a new Rval packet.
|
+-----------------------------------------------------------------------------*/

Rval *newRval()
{  Rval *r;

   if ((r=(Rval *)allocateWithId(sizeof(Rval),"newRval")) == NULL)
   {  fprintf(stderr,"FATAL! out of memory\n");
      exit(1);
   }
   else
   {
      DPRINT1(4,"newRval: new Rval packet at 0x%06x\n",r);
      r->next = NULL;
      return(r);
   }
}

/*------------------------------------------------------------------------------
|
|	RcreateVar/3
|	RcreateUVar/3
|
|	This function creates an "empty" variable packet given a name, type  
|       and tree root.  If the name already exists, NULL is returned.  If
|	successful, a pointer to the varInfo packet for the new variable is
|	returned.  The "RcreateVar" form forces balancing, "RcreateUVare" does
|	not balance (used for loading from disc).
|
+-----------------------------------------------------------------------------*/

varInfo *RcreateVar(const char *n, symbol **pp, int type)
{  symbol   *p;
   varInfo  *v;

   if (findName((*pp),n))
      return(NULL);
   else
   {  p      = BaddName(pp,n);		/* add name to tree */
      v      = newVar();		/* get new variable packet */
      p->val = (char *)v;
      presetVal(v,type);		/* preset values based on type */
   }
   return(v);
}

varInfo *RcreateUVar(char *n, symbol **pp, int type)
{  symbol   *p;
   varInfo  *v;

   if (findName((*pp),n))
      return(NULL);
   else
   {  p      = addName(pp,n);		/* add name to tree */
      v      = newVar();		/* get new variable packet */
      p->val = (char *)v;
      presetVal(v,type);		/* preset values based on type */
   }
   return(v);
}

/*------------------------------------------------------------------------------
|
|	presetVal/2
|
|	This function preset values of variables created based on the type
|
|   Modified   Author     Purpose
|   --------   ------     -------
|   3/1/89     Greg B.    1. Set the Protection field from the preset structure 
+-----------------------------------------------------------------------------*/

static void presetVal(varInfo *v, int type)
{
    v->active      = ACT_ON;
    v->subtype     = type;
    v->prot	   = preset[type].Prot;
    v->Dgroup      = preset[type].Dgroup;
    v->Ggroup      = preset[type].Ggroup;
    v->minVal      = preset[type].minVal;
    v->maxVal      = preset[type].maxVal;
    v->step        = preset[type].step;
    v->R           = NULL;
    v->E           = NULL;
    v->T.size      = 0;
    v->ET.size     = 0;
    v->T.basicType = preset[type].type;
    v->ET.basicType= v->T.basicType;
    if(v->T.basicType == T_REAL) /* set inital value to zero */
	assignReal(0.0,v,0);
    if(v->T.basicType == T_STRING) /* set inital value to null */
	assignString("",v,0);
}


/*------------------------------------------------------------------------------
|
|	createVar/1
|
|	This function creates an "empty" variable packet given a name to be
|	created.  The proper tree is selected via clues from the given name.
|	If the name already exists, NULL is returned.  If successful, a pointer
|	to the varInfo packet for the new variable is returned.
|
+-----------------------------------------------------------------------------*/

varInfo *createVar(char *n)
{
    symbol  **pp;

    DPRINT1(3,"createVar: creating \"%s\"...\n",n);
    if ( (pp=selectVarTree(n)) )
	return(RcreateVar(n,pp,ST_UNDEF));
    else
    {
	DPRINT0(3,"createVar: could not select tree \n");
	return(NULL);
    }
}


/*------------------------------------------------------------------------------
|
|	createReal/1
|
|	This function is used to create a simple real variable with a given
|	name.
|
+-----------------------------------------------------------------------------*/

varInfo *createReal(char *n)
{   varInfo  *v;

    if ( (v=createVar(n)) )
    {	v->T.basicType = T_REAL;
	return(v);
    }
    else
	return(NULL);
}

/*------------------------------------------------------------------------------
|
|	createString/1
|
|	This function is used to create a simple string variable with a given
|	name.
|
+-----------------------------------------------------------------------------*/

varInfo *createString(char *n)
{   varInfo  *v;

    if ( (v=createVar(n)) )
    {	v->T.basicType = T_STRING;
	return(v);
    }
    else
	return(NULL);
}

/*------------------------------------------------------------------------------
|
|	varSize/1
|
|	This function returns the size of a variable by counting the number
|	of Rval packets attached.  Sometimes it is hard to esimate the size
|	of a variable directly, so this function is provided for those rough
|	times.
|
+-----------------------------------------------------------------------------*/

int varSize(varInfo *v)
{   int   c;
    Rval *p;

    if (v)
    {	c = 0;
	p = v->R;
	while (p)
	{   c += 1;
	    p  = p->next;
	}
	return(c);
    }
    else
	return(0);
}

/*------------------------------------------------------------------------------
|
|	selectRval/2
|
|	This function returns a pointer to a given "Rval" packet of a given
|	variable.  The "Rval" packet is selected by index (1 for 1st, 2 for 2nd,
|	and so on...).
|
+-----------------------------------------------------------------------------*/

Rval *selectRval(varInfo *v, int i)
{   int   j;
    Rval *p;

    DPRINT2(3,"selectRval: find %dth Rval of var 0x%08x...\n",i,v);
    if (0 < i)
    {	p = v->R;
	j = 1;
	while ((j < i) && p)
	{
	    DPRINT2(3,"selectRval: ...not %dth (was 0x%08x)\n",j,p);
	    p  = p->next;
	    j += 1;
	}
#ifdef DEBUG
	if (p)
	   DPRINT2(3,"selectRval: SUCCEEDED, %dth is 0x%08x\n",i,p);
	else
	   DPRINT0(3,"selectRval: FAILED, ran out of Rval packets!\n");
#endif 
	return(p);
    }
    else
    {
	DPRINT0(3,"selectRval: FAILED, can't select 0th value\n");
	return(NULL);
    }
}

/*------------------------------------------------------------------------------
|
|	selectERval/2
|
|	This function returns a pointer to a given enum "Rval" packet of a given
|	variable.  The "Rval" packet is selected by index (1 for 1st, 2 for 2nd,
|	and so on...).
|
+-----------------------------------------------------------------------------*/

Rval *selectERval(varInfo *v, int i)
{   int   j;
    Rval *p;

    DPRINT2(3,"selectERval: find %dth ERval of var 0x%08x...\n",i,v);
    if (0 < i)
    {	p = v->E;
	j = 1;
	while ((j < i) && p)
	{
	    DPRINT2(3,"selectERval: ...not %dth (was 0x%08x)\n",j,p);
	    p  = p->next;
	    j += 1;
	}
#ifdef DEBUG
	if (p)
	   DPRINT2(3,"selectERval: SUCCEEDED, %dth is 0x%08x\n",i,p);
	else
	   DPRINT0(3,"selectERval: FAILED, ran out of ERval packets!\n");
#endif 
	return(p);
    }
    else
    {
	DPRINT0(3,"selectERval: FAILED, can't select 0th value\n");
	return(NULL);
    }
}

/*------------------------------------------------------------------------------
|
|	RshowRvals/3
|
|	This function displays all Rvals of a rval list.
|
+-----------------------------------------------------------------------------*/

static void RshowRvals(int b, Rval *r)
{   if (r)
    {	int   i;

	i = 1;
	while (r)
	{   Wscrprintf("[%d] = ",i);   
	    switch (b)
	    { case T_UNDEF:	Wscrprintf("undef\n");
				break;
	      case T_REAL:	Wscrprintf("%g\n",r->v.r);
				break;
	      case T_STRING:	Wscrprintf("\"%s\"\n",r->v.s);
				break;
	      default:		Wscrprintf("unknown (=%d)\n",b);
				break;
	    }
	    i += 1;
	    r  = r->next;
	}
    }
    else
	Wscrprintf("   ** no values **\n");
}

void printRvals(FILE *f, int index, char *s, int b, Rval *r)
{
    fprintf(f,"%s",(index==1) ? s : ",");
     if (r)
    {	int   i;

	i = 1;
	while (r)
	{
	    switch (b)
	    { case T_UNDEF:	fprintf(f,"%sundef", (i>1) ? " " : "");
				break;
	      case T_REAL:	fprintf(f,"%s%g", (i>1) ? " " : "",r->v.r);
				break;
	      case T_STRING:	fprintf(f,"%s'%s'", (i>1) ? " " : "",r->v.s);
				break;
	      default:		fprintf(f,"%sunknown(=%d)", (i>1) ? " " : "",b);
				break;
	    }
	    i += 1;
	    r  = r->next;
	}
    }
    else
	fprintf(f,"null");
}

void printRvals2(FILE *f, int b, Rval *r)
{
     if (r)
    {	int   i;

	i = 1;
	while (r)
	{
	    switch (b)
	    { case T_UNDEF:	fprintf(f,"%sundef", (i>1) ? "," : "");
				break;
	      case T_REAL:	fprintf(f,"%s%g", (i>1) ? "," : "",r->v.r);
				break;
	      case T_STRING:	fprintf(f,"%s'%s'", (i>1) ? "," : "",r->v.s);
				break;
	      default:		fprintf(f,"%sunknown(=%d)", (i>1) ? "," : "",b);
				break;
	    }
	    i += 1;
	    r  = r->next;
	}
    }
    else
	fprintf(f,"null");
}

void showRvals(FILE *f, char *m, int b, Rval *r)
{   if (r)
    {	int   i;

	fprintf(f,"%s   values...\n",msg(m));
	i = 1;
	while (r)
	{   fprintf(f,"%s      [%d] ",msg(m),i);
	    switch (b)
	    { case T_UNDEF:	fprintf(f,"undef\n");
				break;
	      case T_REAL:	fprintf(f,"%g\n",r->v.r);
				break;
	      case T_STRING:	fprintf(f,"\"%s\"\n",r->v.s);
				break;
	      default:		fprintf(f,"unknown (=%d)\n",b);
				break;
	    }
	    i += 1;
	    r  = r->next;
	}
    }
    else
	fprintf(f,"%s   ** no values **\n",msg(m));
}

/*------------------------------------------------------------------------------
|
|	RshowTval/2
|
|	This procedure is used to show-off a vairables Tvals.
|
+-----------------------------------------------------------------------------*/

static void RshowTval(Tval *t)
{   Wscrprintf("   size      = %d\n",t->size);
    Wscrprintf("   basicType = ");
    switch (t->basicType)
    { case T_UNDEF:	Wscrprintf("UNDEFINED\n");
			break;
      case T_REAL:	Wscrprintf("REAL\n");
			break;
      case T_STRING:	Wscrprintf("STRING\n");
			break;
      default:		Wscrprintf("unknown (=%d)\n",t->basicType);
			break;
    }
}

void showTval(FILE *f, char *m, Tval *t)
{   fprintf(f,"%s   size      = %d\n",msg(m),t->size);
    fprintf(f,"%s   basicType = ",msg(m));
    switch (t->basicType)
    { case T_UNDEF:	fprintf(f,"UNDEFINED\n");
			break;
      case T_REAL:	fprintf(f,"REAL\n");
			break;
      case T_STRING:	fprintf(f,"STRING\n");
			break;
      default:		fprintf(f,"unknown (=%d)\n",t->basicType);
			break;
    }
}

/*------------------------------------------------------------------------------
|
|	showPair/3
|
|	This procedure is used to show-off a given pair node.  Usually used
|	for debugging.
|
+-----------------------------------------------------------------------------*/

void showPair(FILE *f, char *m, pair *p)
{
    if (p)
    {	fprintf(f,"%spair ...\n",msg(m));
	showTval(f,m,&(p->T));
	showRvals(f,m,p->T.basicType,p->R);
    }
    else
	fprintf(f,"%sNULL pair pointer\n",msg(m));
}

int isActive(varInfo *v)
{   if (v)
    {	if (v->active)
	    return(ACT_ON);
	else
	    return(ACT_OFF);
    }
    else
	return(ACT_OFF);
}

void showVar(FILE *f, char *m, varInfo *v)
{   if (v)
    {	fprintf(f,"%svar ...\n",msg(m));
	fprintf(f,"%s   active    = %d\n",msg(m),v->active);
	fprintf(f,"%s   Dgroup    = %d\n",msg(m),v->Dgroup);
	fprintf(f,"%s   subtype   = %d\n",msg(m),v->subtype);
	fprintf(f,"%s   prot      = %d\n",msg(m),v->prot);
	fprintf(f,"%s   group     = %d\n",msg(m),v->Ggroup);
	fprintf(f,"%s   minVal    = %g\n",msg(m),v->minVal);
	fprintf(f,"%s   maxVal    = %g\n",msg(m),v->maxVal);
	fprintf(f,"%s   step      = %g\n",msg(m),v->step);
	showTval(f,m,&(v->T));
	showRvals(f,m,v->T.basicType,v->R);
    }
    else
	fprintf(f,"%sNULL var\n",msg(m));
}

/*------------------------------------------------------------------------------
|
|	RshowVar/2
|
|	This procedure is used to dump out a specific variable.
|
|   Modified   Author     Purpose
|   --------   ------     -------
|   3/1/89     Greg B.    1. If the Parameter has index Max,Min,Step values 
|			     the index and value are printed instead of just the
|			     index.
|			  2. Order of Min Max was swapped to consistent with
|			     setlimit() order of Max,Min,Step
+-----------------------------------------------------------------------------*/

void RshowVar(FILE *f, varInfo *v)
{   
    double maxv, minv, stepv;
    int i,pindex;
    unsigned int p;

    (void) f;
    if (v)
    {	
	RshowRvals(v->T.basicType,v->R);
	RshowTval(&(v->T));
	if(v->ET.size) /* if there are enums */
	{   Wscrprintf("Enumerals\n");
	    RshowRvals(v->T.basicType,v->E);
	}
	Wscrprintf("subtype = %s   ",whatType(v->subtype));
	Wscrprintf("active = %s  ",whatActive(v->active));
	Wscrprintf("group = %s   ",whatGroup(v->Ggroup));
	Wscrprintf("Dgroup = %d\n",v->Dgroup);
	Wscrprintf("protection = ");
	p = v->prot;
	for (i=15; i >= 0; i--)
	  Wscrprintf("%1d%c",(p >> i) & 1,(i != 0) ? ' ' : '\n');

/*  If P_MMS bit set, use maxVal, minVal, etc. fields as indicies
    into system global paramters "parmax", "parmin", etc. extracting
    the desired value from the appropriate array.                       */

        if (v->prot & P_MMS)
        {
           pindex = (int) (v->minVal+0.1);
           if (P_getreal( SYSTEMGLOBAL, "parmin", &minv, pindex ))
            minv = -1.0e+30;
           pindex = (int) (v->maxVal+0.1);
           if (P_getreal( SYSTEMGLOBAL, "parmax", &maxv, pindex ))
            maxv = 1.0e+30;
           pindex = (int) (v->step+0.1);
           if (P_getreal( SYSTEMGLOBAL, "parstep", &stepv, pindex ))
            stepv = 0.0;
	   Wscrprintf("maxVal[%d] = %1.12g   ",(int)(v->maxVal+0.1),maxv);
	   Wscrprintf("minVal[%d] = %1.12g   ",(int)(v->minVal+0.1),minv);
	   Wscrprintf("step[%d] = %1.12g\n\n",
		(int)(v->step+0.1),stepv);
	}
	else
	{
	   Wscrprintf("maxVal = %1.12g   ",v->maxVal);
	   Wscrprintf("minVal = %1.12g   ",v->minVal);
	   Wscrprintf("step = %1.12g\n\n",v->step);
	}
    }
    else
	Wscrprintf("NULL var\n");
}

/*------------------------------------------------------------------------------
|
|	RshowVals/2
|
|	This procedure is used to dump out a variable names and values of
|        variable symbol table.
|      
|
+-----------------------------------------------------------------------------*/

void RshowVals(FILE *f, symbol *s)
{   varInfo *v;

    if (s)
    {	RshowVals(f,s->left);
	Wscrprintf("Variable \"%s\" ",s->name);
	v = (varInfo *)s->val;
	RshowRvals(v->T.basicType,v->R);
	RshowVals(f,s->right);
    }
}

static size_t outjsize;
static int outj;
static char *outjlist;

/*------------------------------------------------------------------------------
|
|	RcountJVals/1
|
|	This procedure is used to count variable names of
|        variable symbol table, for vnmrj.
|
+-----------------------------------------------------------------------------*/
void RcountJVals(symbol *s)
{   varInfo *v;

    if (s)
    {	RcountJVals(s->left);
	if (s->name[0] != '$')
	{
	  v = (varInfo *)s->val;
/* check if parameter is arrayable */
	  if ((v->prot) & 1) {}
	  else
	  {
	    if (((v->Ggroup == 2) || (v->Ggroup == 0)) &&
		((v->subtype == 3) || (v->subtype == 5) || (v->subtype == 6)) )
	    {
/* if (v->subtype == 1, 2, 3, 4, 5, 6, 7) ... */ /* real, string, delay, flag, freq, pulse, int */
	      outj += 1;
	      outjsize += ( strlen(s->name) + 1 );
	    }
	  }
	}
	RcountJVals(s->right);
    }
}
/*------------------------------------------------------------------------------
|
|	RshowJVals/1
|
|	This procedure is used to dump out variable names of
|        variable symbol table, for vnmrj.
|
|   maybe should write to a user-editable file instead of socket?
|
+-----------------------------------------------------------------------------*/
void RshowJVals(symbol *s)
{   varInfo *v;

    if (s)
    {	RshowJVals(s->left);
	if (s->name[0] != '$')
	{
	  v = (varInfo *)s->val;
/* check if parameter is arrayable */
	  if ((v->prot) & 1) {}
	  else
	  {
	    if (((v->Ggroup == 2) || (v->Ggroup == 0)) &&
		((v->subtype == 3) || (v->subtype == 5) || (v->subtype == 6)) )
	    {
/* if (v->subtype == 1, 2, 3, 4, 5, 6, 7) ... */ /* real, string, delay, flag, freq, pulse, int */
/*	      Wscrprintf("Variable \"%s\" \n",s->name); */
	      if (strlen(outjlist)+strlen(s->name)+1 < outjsize)
	      {
	        strcat(outjlist,s->name);
	        strcat(outjlist," ");
	      }
	    }
	  }
	}
	RshowJVals(s->right);
    }
}

void outjCtInit()
{
  outj = 0;
  outjsize = 0;
}

int outjInit()
{
  if (outjsize > 0)
  {
    outjsize += 4;
    if ((outjlist = (char *)malloc(sizeof(char) * (outjsize)))==0)
    {
      Werrprintf("jsendArrayMenu: error allocating memory\n");
#ifdef VNMRJ
      writelineToVnmrJ("ARRAYM","-1");
#endif 
      return(1);
    }
    strcpy(outjlist,"");
  }
  return(0);
}

void outjShow()
{
/*  Winfoprintf("arrayMenu: ct=%d size=%d\n",outj,outjsize); */
  Winfoprintf("arrayMenu = %s\n",outjlist);
#ifdef VNMRJ
  writelineToVnmrJ("ARRAYM",outjlist);
#endif 
  free((char *)outjlist);
}

/*------------------------------------------------------------------------------
|
|	RshowVars/2
|
|	This procedure is used to dump out a specific variable symbol table.
|
+-----------------------------------------------------------------------------*/

void RshowVars(FILE *f, symbol *s)
{   if (s)
    {	RshowVars(f,s->left);
	Wscrprintf("Variable \"%s\"\n",s->name);
	RshowVar(f,s->val);
	RshowVars(f,s->right);
    }
}

/*------------------------------------------------------------------------------
|
|	showVars/3
|
|	This procedure is used to dump out a specific variable symbol table.
|
+-----------------------------------------------------------------------------*/

void showVars(FILE *f, char *m, symbol *s)
{   if (s)
    {	showVars(f,m,s->left);
	fprintf(f,"%s%s\n",msg(m),s->name);
	showVar(f,m,s->val);
	showVars(f,m,s->right);
    }
}

/*------------------------------------------------------------------------------
|
|	showGlobalVars/2
|	showLocalVars/2
|	showTempVars/2
|
|	These procedures are used to dump out specific variable symbol tables.
|
+-----------------------------------------------------------------------------*/

void showGlobalVars(FILE *f, char *m)
{   if (global)
	showVars(f,m,global);
    else
	fprintf(f,"%sNo global vars\n",msg(m));
}

void showLocalVars(FILE *f, char *m)
{   if ((0 <= level) && (local[level]))
	showVars(f,m,local[level]);
    else
	fprintf(f,"%sNo local vars\n",msg(m));
}

void showTempVars(FILE *f, char *m)
{   if (temp)
	showVars(f,m,temp);
    else
	fprintf(f,"%sNo temp vars\n",msg(m));
}

/*------------------------------------------------------------------------------
|
|	showAllVars/2
|
|	This procedure is used to dump out ALL variable symbol tables.
|
+-----------------------------------------------------------------------------*/

void showAllVars(FILE *f, char *m)
{   showGlobalVars(f,m);
    showLocalVars(f,m);
    showTempVars(f,m);
}

/*------------------------------------------------------------------------------
|
|	valueOf/3
|
|	This function is used to set a given "pair" packet to reflect values
|	retrived from a given "varInfo" packet.  Note that the "pair" is
|	given copies of any Rvals from a variable (see assign.c:appendPair/2).
|
|	Using the varInfo to define a Variable and the integer to specify
|	the index, it obtains the values for the pair.  This subroutine
|       MUST take responsibilty for initializing the pair data structure,
|	even if no actual value is found and the routine returns 0.
+-----------------------------------------------------------------------------*/

int valueOf(varInfo *v, int i, pair *p)
{   Rval **pp;
    Rval  *q;
    Rval  *r;

    DPRINT2(3,"valueOf: find value of 0x%08x[%d]...\n",v,i);
    DSHOWVAR(3,"valueOf:    ",v);
    if (v)
	if (i == 0)
	{
	    DPRINT1(3,"valueOf: ...whole thing...\n",p);
	    p->T.size      = 0;
	    p->T.basicType = v->T.basicType;
	    p->R           = NULL;
	    pp		   = &(p->R);
	    r              = v->R;
	    while (r)
	    {	(*pp) = newRval();
		switch (v->T.basicType)
		{ case T_UNDEF:	    (*pp)->v.r = 0.0;
				    break;
		  case T_REAL:	    (*pp)->v.r = r->v.r;
				    break;
		  case T_STRING:    (*pp)->v.s = newStringId(r->v.s,"valueOf(STRING)/1");
				    break;
		  default:	    fprintf(stderr,"magic: bad type encountered during variable value retrieval (=%d)\n",v->T.basicType);
				    exit(1);
		}
		p->T.size += 1;
		pp         = &((*pp)->next);
		r          = r->next;
	    }
	    DSHOWPAIR(3,"valueOf:    ",p);
	    return( (p->R != NULL) ? 1 : 0);
	}
	else
	{
	    DPRINT1(3,"valueOf: ...just element %d...\n",i);
	    if ((0 < i) && (i <= v->T.size))
	    {	p->T.size      = 1;
		p->T.basicType = v->T.basicType;
		p->R           = newRval();
		q              = selectRval(v,i);
		switch (v->T.basicType)
		{ case T_REAL:	    p->R->v.r = q->v.r;
				    break;
		  case T_STRING:    p->R->v.s = newStringId(q->v.s,"valueOf(STRING)/2");
				    break;
		  case T_UNDEF:     fprintf(stderr,"Oh My! an T_UNDEF!\n");
				    exit(1);
		  default:          fprintf(stderr,"Oh My! a funny type!\n");
				    exit(1);
		}
		DSHOWPAIR(3,"valueOf:    ",p);
		return(1);
	    }
	    else
	    {
		DPRINT1(3,"valueOf: ...FAILED, index out of range 1..%d\n",
                           v->T.size);
	    	p->T.size      = 0;
		p->T.basicType = T_UNDEF;
		p->R           = NULL;
		return(0);
	    }
	}
    else
    {
	DPRINT0(3,"valueOf: ...FAILED, NULL varInfo packet ptr\n");
	p->T.size      = 0;
	p->T.basicType = T_UNDEF;
	p->R           = NULL;
	return(0);
    }
}

/*------------------------------------------------------------------------------
|
|	buildValue/1
|
|	This procedure is used to generate the value output for the "SHOW"
|	operation (ie. "show v" or "v=").  Note that arrays are just announced
|	not output (?). buildValue returns a pointer to a string
|
+-----------------------------------------------------------------------------*/

char *buildValue(pair *p)
{   static char s[1024];   

    if (p->T.size == 1)
	switch (p->T.basicType)
	{ case T_REAL:      sprintf(s,"%g",p->R->v.r);
			    break;
	  case T_STRING:    snprintf(s, 1023, "'%s'",p->R->v.s);
			    break;
	  case T_UNDEF:     sprintf(s,"undef");
			    break;
	  default:          sprintf(s,"unknown");
			    break;
	}
    else
    {	switch (p->T.basicType)
	{ case T_REAL:	    sprintf(s,"array of %d real(s)",p->T.size);
			    break;
	  case T_STRING:    sprintf(s,"array of %d string(s)",p->T.size);
			    break;
	  case T_UNDEF:     sprintf(s,"array of %d undef(s)",p->T.size);
			    break;
	  default:          printf(s,"array of %d unknown(s)",p->T.size);
			    break;
	}
    }
    return(s);
}

#ifdef VNMRJ
/*------------------------------------------------------------------------------
|
|	showValue/1
|
|	This procedure is used to generate the value output for the "SHOW"
|	operation (ie. "show v" or "v=").  Note that arrays are just announced
|	not output (?).
|
+-----------------------------------------------------------------------------*/

void showValue(pair *p)
{   if (p->T.size == 1)
	switch (p->T.basicType)
	{ case T_REAL:      Wprintfpos(W_STATUS,0,-1,"%g",p->R->v.r);
			    break;
	  case T_STRING:    Wprintfpos(W_STATUS,0,-1,"\"%s\"",p->R->v.s);
			    break;
	  case T_UNDEF:     Wprintfpos(W_STATUS,0,-1,"undef");
			    break;
	  default:          Wprintfpos(W_STATUS,0,-1,"unknown");
			    break;
	}
    else
    {	Wprintfpos(W_STATUS,0,-1,"array of %d ",p->T.size);
	switch (p->T.basicType)
	{ case T_REAL:	    Wprintfpos(W_STATUS,0,-1,"real(s)");
			    break;
	  case T_STRING:    Wprintfpos(W_STATUS,0,-1,"string(s)");
			    break;
	  case T_UNDEF:     Wprintfpos(W_STATUS,0,-1,"undef(s)");
			    break;
	  default:          Wprintfpos(W_STATUS,0,-1,"unknown(s)");
			    break;
	}
    }
}
#endif

/*------------------------------------------------------------------------------
|
|	addNewRval/1
|
|	This function adds a new Rval packet to a variable (updating the
|	variable size) and returns a pointer to this new packet.
|
+-----------------------------------------------------------------------------*/

Rval *addNewRval(varInfo *v)
{   Rval *p;
    Rval *q;

    DPRINT1(3,"addNewRval: to 0x%08x...\n",v);
    p = newRval();
    if ( (q=v->R) )
    {	while (q->next)
	    q = q->next;
    	v->T.size += 1;
	q->next    = p;
    }
    else
    {	v->T.size = 1;
	v->R      = p;
    }
    DPRINT1(3,"addNewRval: ...size is now %d\n",v->T.size);
    return(p);
}

/*------------------------------------------------------------------------------
|
|	addNewERval/1
|
|	This function adds a new ERval packet to a variable (updating the
|	variable size) and returns a pointer to this new packet.
|
+-----------------------------------------------------------------------------*/

Rval *addNewERval(varInfo *v)
{   Rval *p;
    Rval *q;

    DPRINT1(3,"addNewERval: to 0x%08x...\n",v);
    p = newRval();
    if ( (q=v->E) )
    {	while (q->next)
	    q = q->next;
    	v->ET.size += 1;
	q->next    = p;
    }
    else
    {	v->ET.size = 1;
	v->E      = p;
    }
    DPRINT1(3,"addNewERval: ...size is now %d\n",v->ET.size);
    return(p);
}

/*------------------------------------------------------------------------------
|
|	getBasicType/1
|
|	This function takes a subtype index and returns a basicType index.
|
+-----------------------------------------------------------------------------*/

int  getBasicType(int subtype)
{
    switch (subtype)
    { case ST_UNDEF:		return(T_UNDEF);
      case ST_REAL:		return(T_REAL);
      case ST_STRING:		return(T_STRING);
      case ST_DELAY:		return(T_REAL);
      case ST_FLAG:		return(T_STRING);
      case ST_FREQUENCY:	return(T_REAL);
      case ST_PULSE:		return(T_REAL);
      case ST_INTEGER:		return(T_REAL);
      default:			return(T_UNDEF);
    }
}

/*------------------------------------------------------------------------------
|
|	whatGroup/1
|
|	This function takes a group index and returns a pointer to a 
|       group constant.
|
+-----------------------------------------------------------------------------*/

const char  *whatGroup(int group)
{
    switch (group)
    { case G_ALL:		return("all");
      case G_SAMPLE:		return("sample");
      case G_ACQUISITION:	return("acquisition");
      case G_PROCESSING:	return("processing");
      case G_DISPLAY:		return("display");
      case G_SPIN:		return("spin");
      default:			return("unknown");
    }
}

/*------------------------------------------------------------------------------
|
|	whatActive/1
|
|	This function takes a active index and returns a pointer to a 
|       active constant.
|
+-----------------------------------------------------------------------------*/

const char  *whatActive(int active)
{
    switch (active)
    { case ACT_OFF:		return("OFF");
      case ACT_ON:		return("ON");
      default:			return("unknown");
    }
}

/*------------------------------------------------------------------------------
|
|	whatType/1
|
|	This function takes a typeindex and returns a pointer to a 
|       type constant.
|
+-----------------------------------------------------------------------------*/

const char  *whatType(int type)
{
    switch (type)
    { case ST_UNDEF:		return("undefined");
      case ST_REAL:		return("real");
      case ST_STRING:		return("string");
      case ST_DELAY:		return("delay");
      case ST_FLAG:		return("flag");
      case ST_FREQUENCY:	return("frequency");
      case ST_PULSE:		return("pulse");
      case ST_INTEGER:		return("integer");
      default:			return("unknown");
    }
}

/*------------------------------------------------------------------------------
|
|	goodGroupIndex/1
|
|	This function checks a group index to deterine if it is good. It
|	returns the address of a string if it is, returns 0 if it is not.
|
+-----------------------------------------------------------------------------*/

const char *goodGroupIndex(int group)
{
    switch (group)
    { case G_ALL:		return("all");
      case G_SAMPLE:		return("sample");
      case G_ACQUISITION:	return("acquisition");
      case G_PROCESSING:	return("processing");
      case G_DISPLAY:		return("display");
      case G_SPIN:		return("spin");
      default:			return(NULL);
    }
}

/*------------------------------------------------------------------------------
|
|	goodGroup/1
|
|	This function checks a string to see if it is a valid group
|	and return integer.
|
+-----------------------------------------------------------------------------*/

int goodGroup(char *group)
{
    if ( ! strcmp(group,"all") )
	return(G_ALL);
    if ( ! strcmp(group,"sample") )
	return(G_SAMPLE);
    if ( ! strcmp(group,"acquisition") )
	return(G_ACQUISITION);
    if ( ! strcmp(group,"processing") )
	return(G_PROCESSING);
    if ( ! strcmp(group,"display") )
	return(G_DISPLAY);
    if ( ! strcmp(group,"spin") )
	return(G_SPIN);
    return -1;
}

/*------------------------------------------------------------------------------
|
|	goodType/1
|
|	This function checks a string to see if it is a valid type.
|
+-----------------------------------------------------------------------------*/

int goodType(char *type)
{
    if ( ! strcmp(type,"real") )
	return(ST_REAL);
    if ( ! strcmp(type,"string") )
	return(ST_STRING);
    if ( ! strcmp(type,"delay") )
	return(ST_DELAY);
    if ( ! strcmp(type,"flag") )
	return(ST_FLAG);
    if ( ! strcmp(type,"frequency") )
	return(ST_FREQUENCY);
    if ( ! strcmp(type,"pulse") )
	return(ST_PULSE);
    if ( ! strcmp(type,"integer") )
	return(ST_INTEGER);
    return(0);
}

/*------------------------------------------------------------------------------
|
|	goodName/1
|
|	This function checks a string to see if it is a valid identifier.
|
+-----------------------------------------------------------------------------*/

int goodName(const char *n)
{   int i;
    int L;

    DPRINT1(3,"goodName: name = \"%s\"\n",n);
    if (n)
	if ( (L= (int) strlen(n)) )
	{   for (i=0; i<L; ++i)
		if (letter(n[i]))
		{
		    continue;
		}
		else
		{
		    if ((0 < i) && (digit(n[i])))
			continue;
		    else
			return(0);
		}
	    return(1);
	}
	else
	    return(0);
    else
	return(0);
}

/*------------------------------------------------------------------------------
|
|	cleanPair/1
|
|	This procedure is used to release storage associated with a given
|	"pair".
|
+-----------------------------------------------------------------------------*/

void cleanPair(pair *p)
{   if (p)
    {
	DPRINT1(3,"cleanPair: pair 0x%08x...\n",p);
	if (p->R)
	{
	    DPRINT0(3,"cleanPair: ...releasing attached Rvals\n");
	    if (p->T.basicType == T_STRING)
		disposeStringRvals(p->R);
	    else
		disposeRealRvals(p->R);
	    p->R = NULL;
	}
	DPRINT1(3,"cleanPair: ...pair 0x%08x is now clean\n",p);
    }
}
