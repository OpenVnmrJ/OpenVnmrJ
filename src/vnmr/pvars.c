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
|	Pvars.c
|
|	These procedures are the older P_ variables routines converted to
|	work with the new variable system.
|
|	This module contains the P_ user interface variable
|	procedures to access the variable tree.  These routines
|	allow setting, copying, setting limits, defining
|	groups, setting active, setting protections, etc.
|	These P_ procedures are higher level routines and
|	interface with the lower level routines found in variables1.c
|
|	String arrays are written on multiple lines to try and keep number
|	of characters on a line less than 80.  An individual value can still
|	have more than 80 characters, so we can't guarantee every line will
|	have 80 or fewer characters.  See storeOnDisk		05/08/90  ROL
|
|	Files are first written to filename.tmp.  Then at each step of the
|	process of writing out the data, the program checks for errors.  Only
|	if the entire process proceeds without incident is the original file
|	deleted and the new on moved into its place.  The goal is to prevent
|	files from becoming corrupted if the disk is full or if an error
|	occurs for some other reason.			      07/19/1996  ROL
+-----------------------------------------------------------------------------*/

#include "vnmrsys.h"
#include "group.h"
#include "symtab.h"
#include "params.h"
#include "pvars.h"
#include "variables.h"
#include "tools.h"
#include "allocate.h"
#include "wjunk.h"
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#ifdef UNIX
#include <sys/types.h>
#else 
#include <stat.h>
#endif 

#include <errno.h>

#ifdef  DEBUG
extern int      Dflag;
extern int      Tflag;
#define DPRINT0(level, str) \
	if (Dflag >= level) fprintf(stderr,str)
#define DPRINT1(level, str, arg1) \
	if (Dflag >= level) fprintf(stderr,str,arg1)
#define DPRINT2(level, str, arg1, arg2) \
	if (Dflag >= level) fprintf(stderr,str,arg1,arg2)
#define DPRINT3(level, str, arg1, arg2, arg3) \
	if (Dflag >= level) fprintf(stderr,str,arg1,arg2,arg3)
#define TPRINT0(level, str) \
	if (Tflag >= level) fprintf(stderr,str)
#define TPRINT1(level, str, arg1) \
	if (Tflag >= level) fprintf(stderr,str,arg1)
#define TPRINT2(level, str, arg1, arg2) \
	if (Tflag >= level) fprintf(stderr,str,arg1,arg2)
#define TPRINT3(level, str, arg1, arg2, arg3) \
	if (Tflag >= level) fprintf(stderr,str,arg1,arg2,arg3)
#define TPRINT5(level, str, arg1, arg2, arg3, arg4, arg5) \
	if (Tflag >= level) fprintf(stderr,str,arg1,arg2,arg3,arg4,arg5)
#else 
#define DPRINT0(level, str) 
#define DPRINT1(level, str, arg2) 
#define DPRINT2(level, str, arg1, arg2) 
#define DPRINT3(level, str, arg1, arg2, arg3) 
#define TPRINT0(level, str) 
#define TPRINT1(level, str, arg2) 
#define TPRINT2(level, str, arg1, arg2) 
#define TPRINT3(level, str, arg1, arg2, arg3) 
#define TPRINT5(level, str, arg1, arg2, arg3, arg4, arg5) 
#endif 

#define COMPLETE	0
#define ERROR		1

#define letter(c) ((('a'<=(c))&&((c)<='z'))||(('A'<=(c))&&((c)<='Z'))||((c)=='_')||((c)=='$')||((c)=='#'))
#define digit(c) (('0'<=(c))&&((c)<='9'))

extern varInfo *RcreateUVar(char *n, symbol **pp, int type);
extern int      compareValue(char *s, varInfo *v, int i);
extern int assignReal(double d, varInfo *v, int i);
extern int assignEReal(double d, varInfo *v, int i);
extern int assignString(const char *s, varInfo *v, int i);
extern int assignEString(char *s, varInfo *v, int i);
extern void tossVars(symbol **pp);
extern const char *goodGroupIndex(int group);
extern void disposeStringRvals(Rval *r);
extern void disposeRealRvals(Rval *r);

#ifdef VNMRJ
static int vjcopyflag = -1;
#else
extern const char *getRoot(int index);
extern symbol **getTreeRoot(const char *n);
extern symbol **getTreeRootByIndex(int index);
extern varInfo *RcreateVar(const char *n, symbol **pp, int type);
extern varInfo *rfindVar(const char *n, symbol **pp);
#endif 

/* Prototypes */
static int rscanf(FILE *stream, char *buf, int bufmax);
int storeOnDisk(char *name, varInfo *v, FILE *stream);
int readfromdisk(symbol **root, FILE *stream);
int copytodisk(symbol **root, FILE *stream);
void prunetree(symbol **tp, symbol **root1, symbol **root2);
void delGroupVar(symbol **tp, symbol **root, int group);
static char *readNamesFromDisk(FILE *stream);


/*------------------------------------------------------------------------------
|
|	P_setreal/4
|
|	This function assigns a real value to a variable.
|
+-----------------------------------------------------------------------------*/

int P_setreal(int tree, const char *name, double value, int index)
{  symbol **root;
   varInfo *v;

   if ( (root=getTreeRoot(getRoot(tree))) )
      if ( (v=rfindVar(name,root)) )
	 if (assignReal(value,v,index))
	    return(0);
	 else
	 {
#ifdef VNMRJ
	    appendvarlist(name);
#endif 
	    return(-99); /* unknown error */
	 }
      else
	    return(-2); /* variable doesn't exist */
   else
      return(-1); /* tree doesn't exist */
}

/*------------------------------------------------------------------------------
|
|	P_setstring/4
|
|	This function assigns a string value to a variable.
|
+-----------------------------------------------------------------------------*/

int P_setstring(int tree, const char *name, const char *strptr, int index)
{   symbol **root;

    if ( (root = getTreeRoot(getRoot(tree))) )
    {	varInfo *v;

     	if ( (v = rfindVar(name,root)) )
	{   if(assignString(strptr,v,index))
		return(0);
	    else
	    {
#ifdef VNMRJ
	        appendvarlist(name);
#endif 
		return(-99); /* unknown error */
	    }
	}
	else
	    return(-2); /* variable doesn't exist */
    }
    else
	return(-1); /* tree doesn't exist */
}

/*------------------------------------------------------------------------------
|
|	P_Esetreal/4
|
|	This function assigns a real enumeral value to a variable.
|
+-----------------------------------------------------------------------------*/

int P_Esetreal(int tree, const char *name, double value, int index)
{   symbol **root;

    if ( (root = getTreeRoot(getRoot(tree))) )
    {	varInfo *v;

     	if ( (v = rfindVar(name,root)) )
	{   if(assignEReal(value,v,index))
		return(0);
	    else
		return(-99); /* unknown error */
	}
	else
	    return(-2); /* variable doesn't exist */
    }
    else
	return(-1); /* tree doesn't exist */
}

/*------------------------------------------------------------------------------
|
|	P_Esetstring/4
|
|	This function assigns a string enumeral value to a variable.
|
+-----------------------------------------------------------------------------*/

int P_Esetstring(int tree, const char *name, char *strptr, int index)
{   symbol **root;

    if ( (root = getTreeRoot(getRoot(tree))) )
    {	varInfo *v;

     	if ( (v = rfindVar(name,root)) )
	{   if(assignEString(strptr,v,index))
		return(0);
	    else
		return(-99); /* unknown error */
	}
	else
	    return(-2); /* variable doesn't exist */
    }
    else
	return(-1); /* tree doesn't exist */
}

/*------------------------------------------------------------------------------
|
|	P_setlimits/5
|
|	This function sets the max,min and stepsize of 
|	a variable.  If step size is negative, the variable
|	will be adjusted to the nearest value of the power of
|	the absolute power of the step size.
|	If the variable is a string, max is the maximum number
|	of character allowable in the string.
|
+-----------------------------------------------------------------------------*/

int P_setlimits(int tree, const char *name, double max, double min, double step)
{   symbol **root;

    if ( (root = getTreeRoot(getRoot(tree))) )
    {	varInfo *v;

     	if ( (v = rfindVar(name,root)) )
	{   v->minVal = min;
	    v->maxVal = max;
	    v->step   = step;
	    return(0);
	}
	else
	    return(-2); /* variable doesn't exist */
    }
    else
	return(-1); /* tree doesn't exist */
}

/*------------------------------------------------------------------------------
|
|	P_setgroup/3
|
|	This function sets the group parameter of a variable.
|	The valid groups are G_SAMPLE,G_ACQUISITION,G_PROCESSING, G_DISPLAY,
|	G_SPIN
|
+-----------------------------------------------------------------------------*/

int P_setgroup(int tree, const char *name, int group)
{   symbol **root;

    if ( (root = getTreeRoot(getRoot(tree))) )
    {	varInfo *v;

     	if ( (v = rfindVar(name,root)) )
	{   v->Ggroup = group;
	    return(0);
	}
	else
	    return(-2); /* variable doesn't exist */
    }
    else
	return(-1); /* tree doesn't exist */
}

/*------------------------------------------------------------------------------
|
|	P_setdgroup/3
|
|	This function sets the display group parameter of a variable.
|	The valid groups are D_ALL, D_ACQUISITION, D_2DACQUISITION,
|	D_SAMPLE, D_DECOUPLING, D_AFLAGS, D_PROCESSING, S_SPECIAL,
|	D_DISPLAY, D_REFERENCE, D_PHASE, D_CHART, D_2DDISPLAY, D_INTEGRAL,
|	D_DFLAGS, D_FIO, D_SHIMCOILS, D_AUTOMATION, D_NUMBERS, D_STRINGS
|
+-----------------------------------------------------------------------------*/

int P_setdgroup(int tree, const char *name, int group)
{   symbol **root;

    if ( (root = getTreeRoot(getRoot(tree))) )
    {	varInfo *v;

     	if ( (v = rfindVar(name,root)) )
	{   v->Dgroup = group;
	    return(0);
	}
	else
	    return(-2); /* variable doesn't exist */
    }
    else
	return(-1); /* tree doesn't exist */
}

/*------------------------------------------------------------------------------
|
|	P_setprot/3
|
|	This function sets the protection byte parameter of a variable.
|	The values are defined elsewhere (what a cop out )
|
+-----------------------------------------------------------------------------*/

int P_setprot(int tree, const char *name, int prot)
{   symbol **root;

    if ( (root = getTreeRoot(getRoot(tree))) )
    {	varInfo *v;

     	if ( (v = rfindVar(name,root)) )
	{   v->prot = prot;
	    return(0);
	}
	else
	    return(-2); /* variable doesn't exist */
    }
    else
	return(-1); /* tree doesn't exist */
}

/*------------------------------------------------------------------------------
|
|	P_getactive/2
|
|	This function returns the active state of a variable.
|	The valid active states are ACT_ON and ACT_OFF
|
+-----------------------------------------------------------------------------*/

int P_getactive(int tree, const char *name)
{   symbol **root;

    if ( (root = getTreeRoot(getRoot(tree))) )
    {	varInfo *v;

     	if ( (v = rfindVar(name,root)) )
	{
	    return v->active;
	}
	else
	    return(-2); /* variable doesn't exist */
    }
    else
	return(-1); /* tree doesn't exist */
}

/*------------------------------------------------------------------------------
|
|	P_setactive/3
|
|	This function sets the active parameter of a variable.
|	The valid active states are ACT_ON and ACT_OFF
|
+-----------------------------------------------------------------------------*/

int P_setactive(int tree, const char *name, int act)
{   symbol **root;

    if ( (root = getTreeRoot(getRoot(tree))) )
    {	varInfo *v;

     	if ( (v = rfindVar(name,root)) )
	{   v->active = act;
	    return(0);
	}
	else
	    return(-2); /* variable doesn't exist */
    }
    else
	return(-1); /* tree doesn't exist */
}

/*------------------------------------------------------------------------------
|
|	P_getsubtype/2
|
|	This function returns the subtype of a variable.
|
+-----------------------------------------------------------------------------*/

int P_getsubtype(int tree, const char *name)
{   symbol **root;

    if ( (root = getTreeRoot(getRoot(tree))) )
    {	varInfo *v;

     	if ( (v = rfindVar(name,root)) )
	{
	    return v->subtype;
	}
	else
	    return(-2); /* variable doesn't exist */
    }
    else
	return(-1); /* tree doesn't exist */
}

/*------------------------------------------------------------------------------
|
|	P_getsize/2
|
|	This function returns the array size of a variable.
|       If the third argument is not NULL, it is also set to the size.
|
+-----------------------------------------------------------------------------*/

int P_getsize(int tree, const char *name, int *val)
{   symbol **root;

    if ( (root = getTreeRoot(getRoot(tree))) )
    {	varInfo *v;

     	if ( (v = rfindVar(name,root)) )
	{
            if (val != NULL)
               *val = v->T.size;
	    return(v->T.size);
	}
	else
	    return(-2); /* variable doesn't exist */
    }
    else
	return(-1); /* tree doesn't exist */
}

#ifdef VNMRJ
/*------------------------------------------------------------------------------
|
|	compareVar/3
|
|	This function compares one variable from one tree with another.
|
+-----------------------------------------------------------------------------*/

static int compareVar(char *n, varInfo *v, symbol **root)
{   int      i; 
    int      icomp = 1;
    Rval    *r;
    varInfo *newv;
    
    if (!(newv=rfindVar(n,root))) /* variable does not exist */
	return(1);
/* else variable exists */

    if (newv->active  != v->active) return(1);
    else if (newv->T.size  != v->T.size) return(1);
    else if (newv->T.basicType  != v->T.basicType) return(1);
    else if (newv->subtype != v->subtype) return(1);
    else if (newv->Dgroup  != v->Dgroup) return(1);
    else if (newv->Ggroup  != v->Ggroup) return(1);
    else if (newv->prot    != v->prot) return(1);
    else if (newv->minVal  != v->minVal) return(1);
    else if (newv->maxVal  != v->maxVal) return(1);
    else if (newv->step    != v->step) return(1);

    i = 1; 
    r = v->R;
    if (v->T.basicType == T_STRING)  /* compare string values */
    {	while (r && i < v->T.size+1)
	{   icomp = compareValue(r->v.s,newv,i);
	    switch(icomp)
	    {	case 2:
		    i += 1;
		    r = r->next;
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
    else  /* compare real values */
    {	char jstr[32];
	while (r && i < v->T.size+1)
	{   sprintf(jstr,"%g",r->v.r);
	    icomp = compareValue(jstr,newv,i);
	    switch(icomp)
	    {	case 2:
		    i += 1;
		    r = r->next;
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
#endif

/*------------------------------------------------------------------------------
|
|	clearVar/1
|
|	This function clears all the values of a parameter without deleting it.
|
+-----------------------------------------------------------------------------*/
void clearVar(char *name)
{
    varInfo *vinfo;

    if ( (vinfo=findVar(name)) )
    {
	if (vinfo->T.basicType == T_STRING) {
	    disposeStringRvals(vinfo->R);
	    disposeStringRvals(vinfo->E);
	} else if (vinfo->T.basicType == T_REAL) {
	    disposeRealRvals(vinfo->R);
	    disposeRealRvals(vinfo->E);
	}
	vinfo->R = vinfo->E = NULL;
	vinfo->T.size = vinfo->ET.size = 0;
	vinfo->T.basicType = vinfo->ET.basicType = T_UNDEF;
    }
}

/*------------------------------------------------------------------------------
|
|	copyVar/2
|
|	This function copies one variable from one tree to another.
|	It first deletes the existing variable and copies over the new one.
|
+-----------------------------------------------------------------------------*/

void copyVar(const char *n, varInfo *v, symbol **root)
{   int      i; 
    Rval    *r;
    varInfo *newv;
    
    if ( (newv=rfindVar(n,root)) ) /* if variable exists, get rid of its values */
    {	if (newv->T.basicType == T_STRING)  
	{   disposeStringRvals(newv->R);
	    disposeStringRvals(newv->E);
	}
	else
	{   disposeRealRvals(newv->R);
	    disposeRealRvals(newv->E);
	}
	newv->R = NULL;
	newv->E = NULL;
	newv->T.size = newv->ET.size = 0;
	newv->T.basicType = v->T.basicType;
	newv->ET.basicType = v->T.basicType;
    }
    else
    {
	newv = RcreateVar(n,root,v->T.basicType);     /*  create the variable */
    }
    newv->active = v->active;
    newv->subtype= v->subtype;
    newv->Dgroup = v->Dgroup;
    newv->Ggroup = v->Ggroup;
    newv->prot   = v->prot;
    newv->minVal = v->minVal;
    newv->maxVal = v->maxVal;
    newv->step   = v->step;
    if (v->T.basicType == T_STRING)  /* copy over string values */
    {	i = 1; 
     	r = v->R;
	while (r && i < v->T.size+1)
	{   assignString(r->v.s,newv,i);
	    i += 1;
	    r = r->next;
	}
	/*  copy over enumeral values */
     	i = 1; 
     	r = v->E;
	while (r && i < v->ET.size+1)
	{   assignEString(r->v.s,newv,i);
	    i += 1;
	    r = r->next;
	}
    }
    else  /* copy over all reals */
    {	i = 1; 
     	r = v->R;
	while (r && i < v->T.size+1)
	{   assignReal(r->v.r,newv,i);
	    i += 1;
	    r = r->next;
	}
	/*  copy over enumeral values */
     	i = 1; 
     	r = v->E;
	while (r && i < v->ET.size+1)
	{   assignEReal(r->v.r,newv,i);
	    i += 1;
	    r = r->next;
	}
    }
}

/*------------------------------------------------------------------------------
|
|	copyVarNB/2
|
|	This function copies one variable from one tree to another.
|	It first deletes the existing variable and copies over the new one.
|       It is the same as copyVar except the tree is not "balanced"
|       after adding each new variable. It uses RcreateUVar.
|
+-----------------------------------------------------------------------------*/

static void copyVarNB(char *n, varInfo *v, symbol **root)
{   int      i; 
    Rval    *r;
    varInfo *newv;
    
    if ( (newv=rfindVar(n,root)) ) /* if variable exists, get rid of its values */
    {	if (newv->T.basicType == T_STRING)  
	{   disposeStringRvals(newv->R);
	    disposeStringRvals(newv->E);
	}
	else
	{   disposeRealRvals(newv->R);
	    disposeRealRvals(newv->E);
	}
	newv->R = NULL;
	newv->E = NULL;
	newv->T.size = newv->ET.size = 0;
	newv->T.basicType = v->T.basicType;
	newv->ET.basicType = v->T.basicType;
    }
    else
	newv = RcreateUVar(n,root,v->T.basicType);     /*  create the variable */
    newv->active = v->active;
    newv->subtype= v->subtype;
    newv->Dgroup = v->Dgroup;
    newv->Ggroup = v->Ggroup;
    newv->prot   = v->prot;
    newv->minVal = v->minVal;
    newv->maxVal = v->maxVal;
    newv->step   = v->step;
    if (v->T.basicType == T_STRING)  /* copy over string values */
    {	i = 1; 
     	r = v->R;
	while (r && i < v->T.size+1)
	{   assignString(r->v.s,newv,i);
	    i += 1;
	    r = r->next;
	}
	/*  copy over enumeral values */
     	i = 1; 
     	r = v->E;
	while (r && i < v->ET.size+1)
	{   assignEString(r->v.s,newv,i);
	    i += 1;
	    r = r->next;
	}
    }
    else  /* copy over all reals */
    {	i = 1; 
     	r = v->R;
	while (r && i < v->T.size+1)
	{   assignReal(r->v.r,newv,i);
	    i += 1;
	    r = r->next;
	}
	/*  copy over enumeral values */
     	i = 1; 
     	r = v->E;
	while (r && i < v->ET.size+1)
	{   assignEReal(r->v.r,newv,i);
	    i += 1;
	    r = r->next;
	}
    }
}

/*------------------------------------------------------------------------------
|
|	copyAllVars/2
|
|	This function copies from one tree to another.
|
+-----------------------------------------------------------------------------*/

static void copyAllVars(symbol **fp, symbol **tp)
{  symbol  *p;
   varInfo *v;
 
   if ( (p=(*fp)) )   /* check if there is at least something in from tree */
   {  if (p->name)
      {
	 DPRINT1(3,"copyAllVars:  working on var \"%s\"\n",p->name);
	 if ( (v=(varInfo *)(p->val)) )
	    copyVarNB(p->name,v,tp);
      }
      if (p->left)
	 copyAllVars(&(p->left),tp);
      if (p->right)
	 copyAllVars(&(p->right),tp);
   }
}

#ifdef VNMRJ
void jsetcopyGVars(int flag)
{
	vjcopyflag = flag;
}
#endif 

/*------------------------------------------------------------------------------
|
|	copyGVars/2
|
|	This function copies all variables of a group 
|	from one tree to another.
|
+-----------------------------------------------------------------------------*/

void copyGVars(symbol **fp, symbol **tp, int group)
{   symbol  *p;
    varInfo *v;
 
    if ( (p=(*fp)) )   /* check if there is at least something in from tree */
    {	if (p->left)
	    copyGVars(&(p->left),tp,group);
	if (p->right)
	    copyGVars(&(p->right),tp,group);
	if (p->name)
    	{
#ifdef      DEBUG
            if (3 <= Dflag)
	    {	v = (varInfo *)p->val;
		DPRINT3(3,"copyGVars:  checking if var \"%s\" in group %s belongs to \"%s\"\n",
		p->name,whatGroup(v->Ggroup),whatGroup(group));
	    }
#endif 
	    if ((v=(varInfo *)(p->val)) && v->Ggroup == group)
	    {
#ifdef VNMRJ
		int diffval = 1;
		if (vjcopyflag > -1)
		   diffval = compareVar(p->name,v,tp);
#endif 
		DPRINT1(3,"copyGVars: copying \"%s\"\n",p->name);
	     	copyVarNB(p->name,v,tp);
#ifdef VNMRJ
		if ((vjcopyflag > -1) && (diffval == 1))
		   appendvarlist(p->name);
#endif 
	    }
	}
    }
}

void P_balance(symbol **tp)
{
   balance(tp);  /* rebalance tree */
}
/*------------------------------------------------------------------------------
|
|	P_pruneTree/2
|
|	This function destroies all variables in tree1 which are not
|	present in tree2.  
|
+-----------------------------------------------------------------------------*/

int P_pruneTree(int tree1, int tree2)
{
   symbol **root1,**root2;

   if ((root1 = getTreeRootByIndex(tree1)) == 0)
     return(-1); /* tree doesn't exist */
   if ((root2 = getTreeRootByIndex(tree2)) == 0)
     return(-1); /* tree doesn't exist */
   prunetree(root1,root1,root2);
   balance(root1);  /* rebalance tree */
   return(0);
}

void prunetree(symbol **tp, symbol **root1, symbol **root2)
{   symbol  *p;
    varInfo *v;

    if ( (p=(*tp)) )   /* check if there is at least something in tree */
    {	if (p->left)
	    prunetree(&(p->left),root1,root2);
	if (p->right)
	    prunetree(&(p->right),root1,root2);
	if (p->name)
	  if ( (v=(varInfo *)(p->val)) )
     	    if ((v = rfindVar(p->name,root2)) == 0)
	    {
	      DPRINT1(1,": pruning \"%s\"\n",p->name);
	      delName(root1,p->name);
	    }
    }
}

/*------------------------------------------------------------------------------
|
|	P_deleteGroupVar/2
|
|	This function destroies all variables of a group 
|	in a tree.  
|
+-----------------------------------------------------------------------------*/

int P_deleteGroupVar(int tree, int group)
{
   symbol **root;

   if ( (root = getTreeRootByIndex(tree)) )
   {  if (goodGroupIndex(group))
      {  delGroupVar(root,root,group);
	 balance(root);  /* rebalance tree */
	 return(0);
      }
      else
	 return(-17); /* bad group index */
   }
   else
      return(-1); /* tree doesn't exist */
}

void delGroupVar(symbol **tp, symbol **root, int group)
{   symbol  *p;
    varInfo *v;
 
    if ( (p=(*tp)) )   /* check if there is at least something in tree */
    {	if (p->left)
	    delGroupVar(&(p->left),root,group);
	if (p->right)
	    delGroupVar(&(p->right),root,group);
	if (p->name)
    	{
#ifdef      DEBUG
            if (3 <= Dflag)
	    {	v = (varInfo *)p->val;
		DPRINT3(3,"delGroupVar:checking if var \"%s\" in group %s belongs to \"%s\"\n",
		p->name,whatGroup(v->Ggroup),whatGroup(group));
	    }
#endif 
	    if ((v=(varInfo *)(p->val)) && v->Ggroup == group)
	    {
		DPRINT1(3,": deleting \"%s\"\n",p->name);
	     	delName(root,p->name);
	    }
	}
    }
}

/*------------------------------------------------------------------------------
|
|	P_testread/1
|
|	This function check tree root is not null
|
+-----------------------------------------------------------------------------*/

int P_testread(int tree)
{
    symbol **tmpRoot;

    if ( (tmpRoot = getTreeRootByIndex(tree)) )
    {
       return( (*tmpRoot == NULL) );
    }
    else
    {
       return(-1);
    }
}

/*------------------------------------------------------------------------------
|
|	P_exch/2
|
|	This function exchanges two trees
|
+-----------------------------------------------------------------------------*/

#ifdef VNMRJ
int P_exch(int tree1, int tree2)
{
    symbol **tmpRoot;
    symbol *tmpVal1;
    symbol *tmpVal2;

    if ((tmpRoot = getTreeRootByIndex(tree1)) && (tmpVal1 = *tmpRoot) &&
        (tmpRoot = getTreeRootByIndex(tree2)) && (tmpVal2 = *tmpRoot) &&
         setTreeRootByIndex(tree1, tmpVal2) &&
         setTreeRootByIndex(tree2, tmpVal1) )
    {
	return(0);
    }
    else
    {
	return(-1); /* one or more trees do not exist */
    }
}
#endif

/*------------------------------------------------------------------------------
|
|	P_copy/2
|
|	This function copies all variables from one tree to 
|	another tree.  It does not reset either tree.  Any variables
|	that already exist on the to-tree are overwritten.
|       Valid trees are GLOBAL, CURRENT,  PROCESSED	
|
+-----------------------------------------------------------------------------*/

int P_copy(int fromtree, int totree)
{   symbol **froot;
    symbol **troot;

    if ((froot = getTreeRoot(getRoot(fromtree))) &&   /* if both trees exists */
       (troot = getTreeRoot(getRoot(totree))))
    {	copyAllVars(froot,troot);
/* don't appendvarlist("seqfil") for VNMRJ - doesn't work within parameter trees? */
	balance(troot);
	return(0);
    }
    else
	return(-1); /* one or more trees do not exist */
}

/*------------------------------------------------------------------------------
|
|	P_copygroup/2
|
|	This function copies all the variables belonging to a
|	group from one tree to another.  If the variable
|	exists in the to-tree, it is overwritten.
|       Valid trees are GLOBAL, CURRENT,  PROCESSED	
|	The valid groups are G_SAMPLE,G_ACQUISITION,G_PROCESSING, G_DISPLAY,
|	G_SPIN
|
+-----------------------------------------------------------------------------*/

int P_copygroup(int fromtree, int totree, int group)
{   symbol **froot;
    symbol **troot;

    if (fromtree == totree)
      return(0);
    TPRINT3(1,"P_copygroup: copying from tree \"%s\" to tree \"%s\" group \"%s\"n",getRoot(fromtree),getRoot(totree),whatGroup(group));
    if ((froot = getTreeRoot(getRoot(fromtree))) &&   /* if both trees exists */
       (troot = getTreeRoot(getRoot(totree))))
    {
#ifdef VNMRJ
        if (totree == CURRENT)
	  jsetcopyGVars( 0 );
#endif 
	copyGVars(froot,troot,group);
        balance(troot);
#ifdef VNMRJ
        if (totree == CURRENT)
	  jsetcopyGVars( -1 );
#endif 
	return(0);
    }
    else
	return(-1); /* one or more trees do not exist */
}

/*------------------------------------------------------------------------------
|
|	P_copyvar/4
|
|	This function copies one variable from one tree to 
|	another tree.  If the variable exists in the to-tree, 
|	it is overwritten. A variable may be renamed.
|       Valid trees are GLOBAL, CURRENT,  PROCESSED, TEMPORARY	
|
+-----------------------------------------------------------------------------*/

int P_copyvar(int fromtree, int totree, const char *name, const char *nname)
{   symbol **froot;
    symbol **troot;
    varInfo *v;

    TPRINT3(1,"P_copyvar: copying from tree \"%s\" to tree \"%s\" var \"%s\"\n",getRoot(fromtree),getRoot(totree),name);
    if ((froot = getTreeRoot(getRoot(fromtree))) &&   /* if both trees exists */
       (troot = getTreeRoot(getRoot(totree))))
    {	if ( (v = rfindVar(name,froot)) ) /* does variable exist */
	{   copyVar(nname,v,troot);
	    return(0);
	}
	else
	{   return(-2);  /* variable doesn't exist */
	}
    }
    else
	return(-1); /* one or more trees do not exist */
}

/*------------------------------------------------------------------------------
|
|	P_deleteVar/2
|
|	This function deletes a variable. Valid trees are
|	GLOBAL, CURRENT,  PROCESSED.  
|
+-----------------------------------------------------------------------------*/

int P_deleteVar(int tree, const char *name)
{   symbol **root;

    if ( (root = getTreeRootByIndex(tree)) )
    {   if ((delNameWithBalance(root,name)) == 0)
	    return 0;
	else
	    return(-2); /* variable doesn't exist */
    }
    else
	return(-1); /* tree doesn't exist */
}

/*------------------------------------------------------------------------------
|
|	P_deleteVarIndex/2
|
|	This function deletes a variable element. Valid trees are
|	GLOBAL, CURRENT,  PROCESSED.  
|
+-----------------------------------------------------------------------------*/

int P_deleteVarIndex(int tree, const char *name, int delindex)
{
   symbol **root;
   varInfo *v;

    if ( (root = getTreeRootByIndex(tree)) )
    {
     	if ( (v = rfindVar(name,root)) )
        {
            Rval *temp, *r;
            temp = r = v->R;
            if (delindex == 1)
            {
               v->T.size--;
               v->R = r->next;
               release(r);
	       return 0;
            }
            while (delindex > 1)
            {
               temp = r;
               r = r->next;
               delindex--;
            }
            v->T.size--;
            temp->next = r->next;
            release(r);
	    return 0;
        }
	else
	    return(-2); /* variable doesn't exist */
    }
    else
	return(-1); /* tree doesn't exist */
}

/*------------------------------------------------------------------------------
|
|	P_creatvar/3
|
|	This function creates a variable. Valid trees are
|	GLOBAL, CURRENT,  PROCESSED.  Valid types are T_UNDEF,
|	T_REAL, and T_STRING
|
+-----------------------------------------------------------------------------*/

int P_creatvar(int tree, const char *name, int type)
{   symbol **root;

    if ( (root = getTreeRoot(getRoot(tree))) )
    {	if (goodName(name))
	{
            RcreateVar(name,root,type);
	    return(0);
	}
	else
	    return(-18); /* bad variable name */
    }
    else
	return(-1); /* tree doesn't exist */
}

/*------------------------------------------------------------------------------
|
|	P_treereset/1
|
|	This function clears out a tree.  Valid trees are
|	GLOBAL, CURRENT,  PROCESSED	
|
+-----------------------------------------------------------------------------*/

int P_treereset(int tree)
{   symbol **root;

    if ( (root = getTreeRoot(getRoot(tree))) )
    {	tossVars(root);
	return(0);
    }
    else
	return(-1); /* tree doesn't exist */
}

/*------------------------------------------------------------------------------
|
|	P_getVarInfoAddr/3
|
|	This function returns the address of the  information packet about
|       a variable.
|	The information packet contains the variables basicType, subtype
|	and number of elements etc. See varInfo struction.
|
+-----------------------------------------------------------------------------*/

varInfo *
P_getVarInfoAddr(int tree, char *name)
{   symbol **root;
    varInfo *v;

    if ( (root = getTreeRoot(getRoot(tree))) )
    {
     	if ( (v = rfindVar(name,root)) )
	{
	    return(v);
	}
    }
    return((varInfo *) -1);
}

/*------------------------------------------------------------------------------
|
|	P_getVarInfo/3
|
|	This function returns an information packet about a variable.  I
|	The information packet contains the variables basicType, subtype
|	and number of elements.  Use P_getreal and P_getstring to get 
|	the value.
|
+-----------------------------------------------------------------------------*/

int P_getVarInfo(int tree, const char *name, vInfo *info)
{   symbol **root;

    if ( (root = getTreeRoot(getRoot(tree))) )
    {	varInfo *v;

     	if ( (v = rfindVar(name,root)) ) 
	{   memset((void *)info, 0, sizeof(vInfo));
	    info->active	= v->active;
	    info->Dgroup	= v->Dgroup;
	    info->group		= v->Ggroup;
	    info->basicType 	= v->T.basicType;
	    info->size      	= v->T.size;
	    info->subtype   	= v->subtype;	
	    info->Esize      	= v->ET.size;
	    info->prot		= v->prot;
	    info->minVal	= v->minVal;
	    info->maxVal	= v->maxVal;
	    info->step		= v->step;
	    return(0);
	}
	else
	    return(-2); /* variable doesn't exist */
    }
    else
	return(-1); /* tree doesn't exist */
}

/*------------------------------------------------------------------------------
|
|	P_getreal/4
|
|	This function returns a real value of a variable based on
|	the index.  The first element starts at index 1.
|
+-----------------------------------------------------------------------------*/

int P_getreal(int tree, const char *name, double *value, int index)
{   symbol **root;

    if ( (root = getTreeRoot(getRoot(tree))) )
    {	varInfo *v;

     	if ( (v = rfindVar(name,root)) )  /* if variable exists */
	{   
	    if ( (0 < index)  && (index <= v->T.size) )  /* within bounds ? */
	    {	int   i;
	     	Rval *r;

		i = 1;
		r = v->R;
		while ( r && i < index ) /* travel down Rvals  */
		{   r = r->next;
		    i += 1;
		}
		if (r)	
		{   *value = r->v.r;
		    return(0);
		}
		else
		{   *value = 0;
		    return(-99);  /* unknown error */
		}
	    }
	    else
		return(-9); /* index out of bounds */
	}
	else
	    return(-2); /* variable doesn't exist */
    }
    else
	return(-1); /* tree doesn't exist */
}

double P_getval(const char *name)
{
   double value = 0.0;
   int ret;
   if ( (ret = P_getreal(CURRENT, name, &value, 1) ) == -2 )
      ret = P_getreal(GLOBAL, name, &value, 1);
   return(value);
}

/*------------------------------------------------------------------------------
|
|	P_getstring/5
|
|	This function returns a string value of a variable based on
|	the index.  The first element starts at index 1.
|	It will add a null at the end of the buffer for saftey.
|
+-----------------------------------------------------------------------------*/

int P_getstring(int tree, const char *name, char *buf, int index, int maxbuf)
{   symbol **root;

    if ( (root = getTreeRoot(getRoot(tree))) )
    {	varInfo *v;

	if (maxbuf <=0)
	    maxbuf = 1; /* for safety */
     	if ( (v = rfindVar(name,root)) )  /* if variable exists */
	{
          if (v->T.basicType == T_STRING)
          {
            if ( 0 < index  && index <= v->T.size )  /* within bounds ? */
	    {	int   i;
	     	Rval *r;

		i = 1;
		r = v->R;
		while ( r && i < index ) /* travel down Rvals  */
		{   r = r->next;
		    i += 1;
		}
		if (r)	
		{   strncpy(buf,r->v.s,maxbuf);   
		    buf[maxbuf-1] = '\0';
		    return(0);
		}
		else
		{   buf[0] = 0;
		    return(-99);  /* unknown error */
		}
	    }
	    else
		return(-9); /* index out of bounds */
          }
          else
	     return(-5); /* parameter not a string */
	}
	else
	    return(-2); /* variable doesn't exist */
    }
    else
	return(-1); /* tree doesn't exist */
}

/*------------------------------------------------------------------------------
|
|	P_Egetreal/4
|
|	This function returns a real enumeral value of a variable based on
|	the index.  The first element starts at index 1.
|
+-----------------------------------------------------------------------------*/

int P_Egetreal(int tree, char *name, double *value, int index)
{   symbol **root;

    if ( (root = getTreeRoot(getRoot(tree))) )
    {	varInfo *v;

     	if ( (v = rfindVar(name,root)) )  /* if variable exists */
	{   
	    if ( 0 < index  && index <= v->ET.size )  /* within bounds ? */
	    {	int   i;
	     	Rval *r;

		i = 1;
		r = v->E;
		while ( r && i < index ) /* travel down Rvals  */
		{   r = r->next;
		    i += 1;
		}
		if (r)	
		{   *value = r->v.r;
		    return(0);
		}
		else
		{   *value = 0;
		    return(-99);  /* unknown error */
		}
	    }
	    else
		return(-9); /* index out of bounds */
	}
	else
	    return(-2); /* variable doesn't exist */
    }
    else
	return(-1); /* tree doesn't exist */
}

/*------------------------------------------------------------------------------
|
|	P_Egetstring/5
|
|	This function returns a string value of a variable based on
|	the index.  The first element starts at index 1.
|
+-----------------------------------------------------------------------------*/

int P_Egetstring(int tree, const char *name, char *buf, int index, int maxbuf)
{   symbol **root;

    if ( (root = getTreeRoot(getRoot(tree))) )
    {	varInfo *v;

     	if ( (v = rfindVar(name,root)) )  /* if variable exists */
	{   if ( 0 < index  && index <= v->ET.size )  /* within bounds ? */
	    {	int   i;
	     	Rval *r;

		i = 1;
		r = v->E;
		while ( r && i < index ) /* travel down Rvals  */
		{   r = r->next;
		    i += 1;
		}
		if (r)	
		{   strncpy(buf,r->v.s,maxbuf);   
		    buf[maxbuf-1] = '\0';
		    return(0);
		}
		else
		{   buf[0] = 0;
		    return(-99);  /* unknown error */
		}
	    }
	    else
		return(-9); /* index out of bounds */
	}
	else
	    return(-2); /* variable doesn't exist */
    }
    else
	return(-1); /* tree doesn't exist */
}

/*------------------------------------------------------------------------------
|
|	listnames/2
|
|	This function copies all variables in a tree to a string
|
+-----------------------------------------------------------------------------*/

static void listnames(symbol **root, char *list)
{  symbol  *p;
 
   if ( (p=(*root)) )   /* check if there is at least something in from tree */
   {  if (p->name)
      {
	 DPRINT1(3,"listnames:  working on var \"%s\"\n",p->name);
         strcat(list," ");
         strcat(list,p->name);
      }
      if (p->left)
      {
	 listnames(&(p->left),list);
      }
      if (p->right)
      {
	 listnames(&(p->right),list);
      }
   }
}

/*------------------------------------------------------------------------------
|
|	P_listnames/2
|
|	This function replaces variables in a tree with those
|	from another tree.  The variable must exist in both
|       trees.
|	Valid trees are GLOBAL, CURRENT,  PROCESSED, TEMPORARY, SYSTEMGLOBAL
|
+-----------------------------------------------------------------------------*/

int P_listnames(int tree, char *list)
{
    symbol **root;

    if ( (root = getTreeRoot(getRoot(tree))) )
    {
        strcpy(list,"");
        listnames(root, list);
        return(0);
    }
    else
	return(-1); /* tree doesn't exist */
}

/*------------------------------------------------------------------------------
|
|	P_save/2
|
|	This function saves variables from a tree on a file.
|	If the file exists, it is overwritten.
|	Vlid trees are GLOBAL, CURRENT,  PROCESSED	
|
+-----------------------------------------------------------------------------*/

int P_save(int tree, const char *filename)
{   FILE    *stream;
    symbol **root;
    char     tmpfilename[ MAXPATH + 20 ];
    int      ival;

    sprintf(tmpfilename, "%s%d", filename, getpid());
    if ( (root = getTreeRoot(getRoot(tree))) )
    {	if ( (stream = fopen(&tmpfilename[ 0 ],"w")) )
	{   ival = copytodisk(root,stream);
            if (ival != 0)
	    {
		fclose(stream);
		unlink( &tmpfilename[ 0 ] );
		return(-19);			/* error writing file */
	    }
	    ival = fclose(stream);		/* fclose returns 0 or -1 */
            if (ival != 0)
	    {
		unlink( &tmpfilename[ 0 ] );
		return(-19);			/* error writing file */
	    }
	    unlink( filename );
	    rename( &tmpfilename[ 0 ], filename );
	    return(0);
	}
	else
	    return(-14); /* not such file */
    }
    else
	return(-1); /* tree doesn't exist */
}

/*------------------------------------------------------------------------------
|
|	copyUnshardToDisk/2
|
|	This function copies all unshared variables and its parameter from a tree
|	to a disk file.
|
+-----------------------------------------------------------------------------*/

static int copyUnsharedToDisk(symbol **root, FILE *stream)
{  symbol  *p;
   varInfo *v;
   int      ival;
 
   if ( (p=(*root)) )   /* check if there is at least something in from tree */
   {  if (p->name)
      {
	 DPRINT1(3,"copytodisk:  working on var \"%s\"\n",p->name);
	 if ( (v=(varInfo *)(p->val)) )
            if ((p->name[0] != '$') &&
                ((v->prot & P_GLO) == P_GLO) &&
                goodName(p->name))
            {
	        ival = storeOnDisk(p->name,v,stream);
                if (ival != 0)
                  return( ival );
            }
      }
      if (p->left)
      {
	 ival = copyUnsharedToDisk(&(p->left),stream);
         if (ival != 0)
           return( ival );
      }
      if (p->right)
      {
	 ival = copyUnsharedToDisk(&(p->right),stream);
         if (ival != 0)
           return( ival );
      }
   }

   return( 0 );
}

/*------------------------------------------------------------------------------
|
|	P_saveUnsharedGlobal/2
|
|	This function saves variables from a tree on a file.
|	If the file exists, it is overwritten.
|       Only called for GLOBAL tree. 
|       Only parameter with the P_GLO bit tuned on are saved.
|
+-----------------------------------------------------------------------------*/

int P_saveUnsharedGlobal(const char *filename)
{   FILE    *stream;
    symbol **root;
    char     tmpfilename[ MAXPATH + 20 ];
    int      ival;

    sprintf(tmpfilename, "%s%d", filename, getpid());
    if ( (root = getTreeRoot("global")) )
    {	if ( (stream = fopen(&tmpfilename[ 0 ],"w")) )
	{   ival = copyUnsharedToDisk(root,stream);
            if (ival != 0)
	    {
		fclose(stream);
		unlink( &tmpfilename[ 0 ] );
		return(-19);			/* error writing file */
	    }
	    ival = fclose(stream);		/* fclose returns 0 or -1 */
            if (ival != 0)
	    {
		unlink( &tmpfilename[ 0 ] );
		return(-19);			/* error writing file */
	    }
	    unlink( filename );
	    rename( &tmpfilename[ 0 ], filename );
	    return(0);
	}
	else
	    return(-14); /* not such file */
    }
    else
	return(-1); /* tree doesn't exist */
}

/*------------------------------------------------------------------------------
|
|	copytodisk/2
|
|	This function copies all variables and its parameter from a tree
|	to a disk file.
|
+-----------------------------------------------------------------------------*/

int copytodisk(symbol **root, FILE *stream)
{  symbol  *p;
   varInfo *v;
   int      ival;
 
   if ( (p=(*root)) )   /* check if there is at least something in from tree */
   {  if (p->name)
      {
	 DPRINT1(3,"copytodisk:  working on var \"%s\"\n",p->name);
	 if ( (v=(varInfo *)(p->val)) )
            if ((p->name[0] != '$') && (goodName(p->name)))
            {
	        ival = storeOnDisk(p->name,v,stream);
                if (ival != 0)
                  return( ival );
            }
      }
      if (p->left)
      {
	 ival = copytodisk(&(p->left),stream);
         if (ival != 0)
           return( ival );
      }
      if (p->right)
      {
	 ival = copytodisk(&(p->right),stream);
         if (ival != 0)
           return( ival );
      }
   }

   return( 0 );
}

/*------------------------------------------------------------------------------
|
|	rtxfromdisk/4
|
|	This function reads in variables off a stream and stores
|	them in a tree, using the rtx rules.
|
+-----------------------------------------------------------------------------*/

static int rtxfromdisk(symbol **root, FILE *stream, int key1IsRt, int key2IsClear)
{
    char        buf[1025];
    char        name[256];
    double      doub;
    int         i;
    int         intptr;
    int         num;
    int         ret;
    varInfo    *v;
    int         dort;
    varInfo     nv;

    ret = -1;
    for(;;)
    {	if(fscanf(stream,"%s",name)==EOF)
	    break;
        
	TPRINT1(2,"rtxfromdisk: reading variable \"%s\"\n",name);
        dort = -1;
        v = NULL;
	if ( (v = rfindVar(name,root)) ) /* does variable already exist */
	{
	    TPRINT1(2,"rtxfromdisk: found variable %s\n",name);
            if (v->prot & P_LOCK)
               dort = 0;
	}
        TPRINT2(1,"rtx variable %s dort= %d\n",name,dort);
	ret =fscanf(stream,"%hd %hd %lf %lf %lf %hd %hd %d %hd %x",
	    &nv.subtype,&nv.T.basicType,&nv.maxVal,&nv.minVal,
	    &nv.step,&nv.Ggroup,&nv.Dgroup,&nv.prot,&nv.active,&intptr);
	TPRINT1(2,"rtxfromdisk: we read  %d things \n",ret);
	TPRINT1(2,"rtxfromdisk: var name \"%s\"\n",name);
	TPRINT2(2,"rtxfromdisk:%d %d\n",nv.subtype,nv.T.basicType);
	TPRINT3(2,"rtxfromdisk:%g %g %g\n",nv.maxVal,nv.minVal,nv.step);
	TPRINT5(2,"rtxfromdisk:%d %d %d %d %x\n",
	    nv.Ggroup,nv.Dgroup,nv.prot,nv.active,intptr);
        if (dort == -1)  /* has not been decided yet */
        {
           if ((nv.prot & P_LOCK) == P_LOCK)
              dort = 1;
           else if (key1IsRt)
              dort = 1;
           else
              dort = 0;
           TPRINT1(1,"rtx final dort= %d\n",dort);
        }
        if (dort)
        {
            if (v == NULL)
            {
               v = RcreateUVar(name,root,T_UNDEF);
	       TPRINT1(2,"rtxfromdisk: creating variable '%s'\n",name);
	       if (v == NULL)
	       {   Werrprintf("rtx: Variable \"%s\" could not be created",name);
	           return(0);
	       }
           }
	   if (v->T.basicType == T_REAL)
           {
	      disposeRealRvals(v->R);
	      v->T.size = 0;
	      v->R      = NULL;
           }
           else if (v->T.basicType == T_STRING)
           {
	      disposeStringRvals(v->R);
	      v->T.size = 0;
	      v->R      = NULL;
           }
           if (key2IsClear)
              nv.prot &= ~P_LOCK;
           v->subtype = nv.subtype;
           v->T.basicType = nv.T.basicType;
           v->maxVal = nv.maxVal;
           v->minVal = nv.minVal;
           v->step = nv.step;
           v->Ggroup = nv.Ggroup;
           v->Dgroup = nv.Dgroup;
           v->prot = nv.prot;
        }
	if (fscanf(stream,"%d",&num)== EOF)
	    break;
	TPRINT1(2,"rtxfromdisk: num of values =%d\n",num);
	switch (nv.T.basicType)
	{ case T_REAL:
		/*  Read the values */
		for (i=1; i<num+1; i++)
		{
#ifdef UNIX
		    if (fscanf(stream,"%lg",&doub)==EOF)
			break;
#else 
		    if (fscanf(stream,"%lf",&doub)==EOF)
			break;
#endif 
                    if (dort)
                    {
		       if (i == 1) /* is this first element */
			   assignReal(doub,v,0); /* clear and set new variable */
		       else
			   assignReal(doub,v,i);
                    }
		    TPRINT1(2,"rtxfromdisk: read T_REAL %g\n",doub);
		}
		/* since assignReal turns active ON, we must turn it back off
		     if necessary */
		if (dort)
                {
		   v->active = nv.active;
		   disposeRealRvals(v->E); /* clear all enumeration values */
		   v->ET.size = 0;
		   v->E = NULL;
                }
		/*  Read the enumeration values */
		if (fscanf(stream,"%d",&num)== EOF)
		    break;
		TPRINT1(2,"rtxfromdisk: num of enums =%d\n",num);
		for (i=1; i<num+1; i++)
		{
#ifdef UNIX
		    if (fscanf(stream,"%lg",&doub)==EOF)
			break;
#else 
		    if (fscanf(stream,"%lf",&doub)==EOF)
			break;
#endif 

                    if (dort)
                    {
		       if (i == 1) /* is this first element */
			   assignEReal(doub,v,0); /* clear and set new variable */
		       else
			   assignEReal(doub,v,i);
                    }
		    TPRINT1(2,"rtxfromdisk: read T_REAL %g\n",doub);
		}
		break;
	  case T_STRING:
		/*  Read values */
		for (i=1; i<num+1; i++)
		{   if (rscanf(stream,buf,1024)<0)
		    {	Werrprintf("rtxfromdisk: premature EOF\n");
			return(0);
		    }
		    else if (dort)
                    {
		     	if (i == 1) /* is this first element ? */
			    assignString(buf,v,0);
			else
			    assignString(buf,v,i);
                    }
		    TPRINT1(2,"rtxfromdisk: read T_STRING %s\n",buf);
		}
		/* since assignString turns active ON, we must turn it back off
		     if necessary */
                if (dort)
                {
		   v->active = nv.active;
		   disposeStringRvals(v->E); /* clear all enumeration values */
		   v->ET.size = 0;
		   v->E = NULL;
                }
		/*  Read the enumeration values */
		if (fscanf(stream,"%d",&num)== EOF)
		    break;
		TPRINT1(2,"rtxfromdisk: num of enums =%d\n",num);
		for (i=1; i<num+1; i++)
		{   if (rscanf(stream,buf,1024)<0)
		    {	Werrprintf("rtxfromdisk: premature EOF\n");
			return(0);
		    }
		    else if (dort)
                    {
		     	if (i == 1) /* is this first element ? */
			    assignEString(buf,v,0);
			else
			    assignEString(buf,v,i);
                    }
		    TPRINT1(2,"rtxfromdisk: read T_STRING %s\n",buf);
		}
		break;
	  default:
		Werrprintf("rtxfromdisk: %s has nonexistent type\n", name);
		return(0);
	}
    }
    return -1;
}

/*------------------------------------------------------------------------------
|
|	P_rtx/2
|
|	This function reads variables subject to the rtx rules.
|
+-----------------------------------------------------------------------------*/

int P_rtx(int tree, const char *filename, int key1IsRt, int key2IsClear)
{   FILE    *streamEx;
    symbol **root;
    int      ival;

    if ( (root = getTreeRootByIndex(tree)) )
    {	
/*
 * The fopen call is sometimes interrupted.  This following will
 * retry the call.
 */
        errno = 0;
        while ( ((streamEx = fopen(filename,"r")) == NULL) && (errno == EINTR) )
        {
          errno = 0;
        }
        if (streamEx)
	{   
	    ival = rtxfromdisk(root,streamEx,key1IsRt,key2IsClear);
            if (ival == 0)
	    {
		fclose(streamEx);
		return(-19);			/* error reading file */
	    }
	    ival = fclose(streamEx);		/* fclose returns 0 or -1 */
            if (ival != 0)
	    {
		return(-19);			/* error reading file */
	    }
	    return(0);
	}
	else
        {
	   return(-14); /* not such file */
        }
    }
    else
    {
	return(-1); /* tree doesn't exist */
    }
}

/*------------------------------------------------------------------------------
|
|	P_read/2
|
|	This function read in variables from a file and loads in into
|	a tree. Valid trees are GLOBAL, CURRENT,  PROCESSED	
|
+-----------------------------------------------------------------------------*/

int P_read(int tree, const char *filename)
{   FILE    *stream;
    symbol **Proot;

    if ( (Proot = getTreeRoot(getRoot(tree))) )
    {
/*
 * The fopen call is sometimes interrupted.  This following will
 * retry the call.
 */
        errno = 0;
        while ( ((stream = fopen(filename,"r")) == NULL) && (errno == EINTR) )
        {
          errno = 0;
        }
     	if (stream)
	{   readfromdisk(Proot,stream);
	    fclose(stream);
	    balance(Proot);
	    return(0);
	}
	else
	    return(-14); /* not such file */
    }
    else
	return(-1); /* tree doesn't exist */
}

/*------------------------------------------------------------------------------
|
|	P_readnames/2
|
|	This function reads in variables from a file and stores parameter
|	names in the supplied string.
|
+-----------------------------------------------------------------------------*/

int P_readnames(const char *filename, char **names)
{   FILE    *stream;

/*
 * The fopen call is sometimes interrupted.  This following will
 * retry the call.
 */
   errno = 0;
   while ( ((stream = fopen(filename,"r")) == NULL) && (errno == EINTR) )
   {
      errno = 0;
   }
   if (stream)
   {
       *names = readNamesFromDisk(stream);
       fclose(stream);
       return(0);
   }
   else
   {
      return(-14); /* not such file */
   }
}

/*------------------------------------------------------------------------------
|
|	storeOnDisk/3
|
|	This function stores onto disk a variable
|
+-----------------------------------------------------------------------------*/

int storeOnDisk(char *name, varInfo *v, FILE *stream)
{   int   dummy;
    int   i;
    Rval *r;
    int   ival;

    dummy = 100;
    /*  write out good stuff */
    /*  Note:  VMS C doesn't support the %hd format on output; only on input  */
#ifdef UNIX
    ival = fprintf(stream,"%s %hd %hd %.12g %.12g %.12g %hd %hd %d %hd %x\n",
	    name,v->subtype,v->T.basicType,v->maxVal,v->minVal,
	    v->step,v->Ggroup,v->Dgroup,v->prot,v->active,dummy);
    if (ival < 0)
      return( -19 );
    ival = fprintf(stream,"%hd ",v->T.size); /* store size */
    if (ival < 0)
      return( -19 );
#else 
    ival = fprintf(stream,"%s %d %d %g %g %g %d %d %d %d %x\n",
	    name,v->subtype,v->T.basicType,v->maxVal,v->minVal,
	    v->step,v->Ggroup,v->Dgroup,v->prot,v->active,dummy);
    if (ival < 0)
      return( -19 );
    ival = fprintf(stream,"%d ",v->T.size); /* store size */
    if (ival < 0)
      return( -19 );
#endif 
    if (v->T.basicType == T_STRING)  /* store string values */
    {	
        register char *sptr;
        i = 1; 
     	r = v->R;
	while (r && i < v->T.size+1) {
           if (strstr(r->v.s,"\"") == NULL)
           {
              ival = fprintf(stream,"\"%s\"\n",r->v.s);
           }
           else
           {
              fprintf(stream,"\"");
              sptr = r->v.s;
              while (*sptr != '\0')
              {
                 if (*sptr == '"')
                    fprintf(stream,"\\\"");
                 else
                    fprintf(stream,"%c", *sptr);
                 sptr++;
              }
              ival = fprintf(stream,"\"\n");
           }
	   if (ival < 0)
	     return( -19 );
	   i += 1;
	   r = r->next;
	}

/*  Write out enumeral values (list of allowed values) */

#ifdef UNIX
    	ival = fprintf(stream,"%hd ",v->ET.size); /* store size */
	if (ival < 0)
	  return( -19 );
#else 
    	ival = fprintf(stream,"%d ",v->ET.size);
	if (ival < 0)
	  return( -19 );
#endif 
     	i = 1; 
     	r = v->E;
	while (r && i < v->ET.size+1) {
	    ival = fprintf(stream,"\"%s\" ",r->v.s);
	    if (ival < 0)
	      return( -19 );
	    i += 1;
	    r = r->next;
	}
	ival = fprintf(stream,"\n");
	if (ival < 0)
	  return( -19 );
    }
    else  /* copy over all reals */
    {	i = 1; 
     	r = v->R;
	while (r && i < v->T.size+1)
	{   ival = fprintf(stream,"%1.12g ",r->v.r);
	    if (ival < 0)
	      return( -19 );
	    i += 1;
	    r = r->next;
	}
	ival = fprintf(stream,"\n");
	if (ival < 0)
	  return( -19 );
	/*  copy over enumeral values */
#ifdef UNIX
    	ival = fprintf(stream,"%hd ",v->ET.size); /* store size */
	if (ival < 0)
	  return( -19 );
#else 
    	ival = fprintf(stream,"%d ",v->ET.size);
	if (ival < 0)
	  return( -19 );
#endif 
     	i = 1; 
     	r = v->E;
	while (r && i < v->ET.size+1)
	{   ival = fprintf(stream,"%1.12g ",r->v.r);
	    if (ival < 0)
	      return( -19 );
	    i += 1;
	    r = r->next;
	}
	ival = fprintf(stream,"\n");
	if (ival < 0)
	  return( -19 );
    }

    return( 0 );
}

/*------------------------------------------------------------------------------
|
|	readfromdisk/2
|
|	This function reads in variables off a stream and stores
|	them in a tree.
|
+-----------------------------------------------------------------------------*/

int readfromdisk(symbol **root, FILE *stream)
{
    char        buf[1025];
    char        name[256];
    double      doub;
    int		active;
    int         i;
    int         intptr;
    int         num;
    int         ret;
    varInfo    *v;

    ret = -1;
    for(;;)
    {	if(fscanf(stream,"%s",name)==EOF)
	    break;
	TPRINT1(2,"readfromdisk: reading variable \"%s\"\n",name);
	if ( (v = rfindVar(name,root)) ) /* does variable already exist */
	{
	    TPRINT1(2,"readfromdisk: found variable %s\n",name);
            /* remove old values now since the new parameter may have
             * a different type, masking the original value type.
             * A core dump can occur if the original type is real and
             * new type is string and system tries to free() a real
             */
	    if (v->T.basicType == T_REAL)
            {
		disposeRealRvals(v->R);
		v->T.size = 0;
		v->R      = NULL;
            }
            else if (v->T.basicType == T_STRING)
            {
		disposeStringRvals(v->R);
		v->T.size = 0;
		v->R      = NULL;
            }
	}
	else 	/* create variable */
	{   v = RcreateUVar(name,root,T_UNDEF);
	    TPRINT1(2,"readfromdisk: creating variable '%s'\n",name);
	}
	if (v == NULL)
	{   Werrprintf("fread:Variable \"%s\" could not be found or created\n",name);
	    return(0);
	}
	ret =fscanf(stream,"%hd %hd %lf %lf %lf %hd %hd %d %hd %x",
	    &v->subtype,&v->T.basicType,&v->maxVal,&v->minVal,
	    &v->step,&v->Ggroup,&v->Dgroup,&v->prot,&v->active,&intptr);
	active = v->active; /* store this because of active kludge */
	TPRINT1(2,"readfromdisk: we read  %d things \n",ret);
	TPRINT1(2,"readfromdisk: var name \"%s\"\n",name);
	TPRINT2(2,"readfromdisk:%d %d\n",v->subtype,v->T.basicType);
	TPRINT3(2,"readfromdisk:%g %g %g\n",v->maxVal,v->minVal,v->step);
	TPRINT5(2,"readfromdisk:%d %d %d %d %x\n",
	    v->Ggroup,v->Dgroup,v->prot,v->active,intptr);
	if (fscanf(stream,"%d",&num)== EOF)
	    break;
	TPRINT1(2,"readfromdisk: num of values =%d\n",num);
	switch (v->T.basicType)
	{ case T_REAL:
		/*  Read the values */
		for (i=1; i<num+1; i++)
		{
#ifdef UNIX
		    if (fscanf(stream,"%lg",&doub)==EOF)
			break;
#else 
		    if (fscanf(stream,"%lf",&doub)==EOF)
			break;
#endif 


#ifdef  A_NO_NO
		  /* not here, goofs up the parmeters when reread, keep mult by 1e-6 */
 		    if (v->subtype == ST_PULSE) /* pulse in usec */
                    {
                         doub *= 1.0e-6;  /* now in sec. */
                    }
#endif

		    if (i == 1) /* is this first element */
			assignReal(doub,v,0); /* clear and set new variable */
		    else
			assignReal(doub,v,i);
		    TPRINT1(2,"readfromdisk: read T_REAL %g\n",doub);
		}
		/* since assignReal turns active ON, we must turn it back off
		     if necessary */
		if (active == ACT_OFF)
		    v->active = ACT_OFF;
		disposeRealRvals(v->E); /* clear all enumeration values */
		v->ET.size = 0;
		v->E = NULL;
		/*  Read the enumeration values */
		if (fscanf(stream,"%d",&num)== EOF)
		    break;
		TPRINT1(2,"readfromdisk: num of enums =%d\n",num);
		for (i=1; i<num+1; i++)
		{
#ifdef UNIX
		    if (fscanf(stream,"%lg",&doub)==EOF)
			break;
#else 
		    if (fscanf(stream,"%lf",&doub)==EOF)
			break;
#endif 

#ifdef A_NO_NO
		  /* not here, goofs up the parmeters when reread, keep mult by 1e-6 */
                    if (v->subtype == ST_PULSE) /* pulse in usec */
                    {
                      doub *= 1.0e-6;  /* now in sec. */
                    }
#endif

		    if (i == 1) /* is this first element */
			assignEReal(doub,v,0); /* clear and set new variable */
		    else
			assignEReal(doub,v,i);
		    TPRINT1(2,"readfromdisk: read T_REAL %g\n",doub);
		}
		break;
	  case T_STRING:
		/*  Read values */
		for (i=1; i<num+1; i++)
		{   if (rscanf(stream,buf,1024)<0)
		    {	Werrprintf("readfromdisk: premature EOF\n");
			return(0);
		    }
		    else
		     	if (i == 1) /* is this first element ? */
			    assignString(buf,v,0);
			else
			    assignString(buf,v,i);
		    TPRINT1(2,"readfromdisk: read T_STRING %s\n",buf);
		}
		/* since assignString turns active ON, we must turn it back off
		     if necessary */
		if (active == ACT_OFF)
		    v->active = ACT_OFF;
		disposeStringRvals(v->E); /* clear all enumeration values */
		v->ET.size = 0;
		v->E = NULL;
		/*  Read the enumeration values */
		if (fscanf(stream,"%d",&num)== EOF)
		    break;
		TPRINT1(2,"readfromdisk: num of enums =%d\n",num);
		for (i=1; i<num+1; i++)
		{   if (rscanf(stream,buf,1024)<0)
		    {	Werrprintf("readfromdisk: premature EOF\n");
			return(0);
		    }
		    else
		     	if (i == 1) /* is this first element ? */
			    assignEString(buf,v,0);
			else
			    assignEString(buf,v,i);
		    TPRINT1(2,"readfromdisk: read T_STRING %s\n",buf);
		}
		break;
	  default:
		Werrprintf("readfromdisk: %s has nonexistent type\n", name);
		return(0);
	}
    }
    return -1;
}

/*------------------------------------------------------------------------------
|
|	skipVar/3
|
|	This function reads and dumps values and enumerals of a variables
|       The stream will then point to the next variable.
|
+-----------------------------------------------------------------------------*/

static void skipVar(char *name, int basicType, FILE *stream)
{
    int num;
    int i;
    double doub;
    char   buf[1025];

    if (fscanf(stream,"%d",&num)== EOF)
       return;
    TPRINT1(2,"skipVar: num of values =%d\n",num);
    switch (basicType)
    { case T_REAL:
    	/*  Read the values */
    	for (i=1; i<num+1; i++)
    	{
    	    if (fscanf(stream,"%lg",&doub)==EOF)
    		return;
    	}
    	/*  Read the enumeration values */
    	if (fscanf(stream,"%d",&num)== EOF)
    	    return;
    	TPRINT1(2,"skipVar: num of enums =%d\n",num);
    	for (i=1; i<num+1; i++)
    	{
    	    if (fscanf(stream,"%lg",&doub)==EOF)
    		return;
    	}
    	break;
      case T_STRING:
    	/*  Read values */
    	for (i=1; i<num+1; i++)
    	{   if (rscanf(stream,buf,1024)<0)
    	    {	Werrprintf("skipVar: premature EOF\n");
    		return;
    	    }
    	}
    	/*  Read the enumeration values */
    	if (fscanf(stream,"%d",&num)== EOF)
    	    return;
    	TPRINT1(2,"skipVar: num of enums =%d\n",num);
    	for (i=1; i<num+1; i++)
    	{   if (rscanf(stream,buf,1024)<0)
    	    {	Werrprintf("skipVar: premature EOF\n");
    		return;
    	    }
    	}
    	break;
      default:
    	Werrprintf("skipVar: %s has nonexistent type\n", name);
    	return;
	}
}

/*------------------------------------------------------------------------------
|
|	readGroupfromdisk/3
|
|	This function reads in variables of the given group off a stream and stores
|	them in a tree. It only reads variables of the specified group.
|
+-----------------------------------------------------------------------------*/

int readGroupfromdisk(symbol **root, FILE *stream, int groupIndex)
{
    char        buf[1025];
    char        name[256];
    double      doub;
    int		active;
    int         i;
    int         intptr;
    int         num;
    int         ret;
    varInfo    *v;
    varInfo    tmp;

    ret = -1;
    for(;;)
    {	if(fscanf(stream,"%s",name)==EOF)
	    break;
	TPRINT1(2,"readfromdisk: reading variable \"%s\"\n",name);
	ret =fscanf(stream,"%hd %hd %lf %lf %lf %hd %hd %d %hd %x",
	    &tmp.subtype,&tmp.T.basicType,&tmp.maxVal,&tmp.minVal,
	    &tmp.step,&tmp.Ggroup,&tmp.Dgroup,&tmp.prot,&tmp.active,&intptr);
	TPRINT1(2,"readfromdisk: we read  %d things \n",ret);
	TPRINT1(2,"readfromdisk: var name \"%s\"\n",name);
	TPRINT2(2,"readfromdisk:%d %d\n",tmp.subtype,tmp.T.basicType);
	TPRINT3(2,"readfromdisk:%g %g %g\n",tmp.maxVal,tmp.minVal,tmp.step);
	TPRINT5(2,"readfromdisk:%d %d %d %d %x\n",
	    tmp.Ggroup,tmp.Dgroup,tmp.prot,tmp.active,intptr);
        if (tmp.Ggroup != groupIndex)
        {
            skipVar(name, tmp.T.basicType, stream);
            continue;
        }
	if ( (v = rfindVar(name,root)) ) /* does variable already exist */
	{
	    TPRINT1(2,"readfromdisk: found variable %s\n",name);
            /* remove old values now since the new parameter may have
             * a different type, masking the original value type.
             * A core dump can occur if the original type is real and
             * new type is string and system tries to free() a real
             */
	    if (v->T.basicType == T_REAL)
            {
		disposeRealRvals(v->R);
		v->T.size = 0;
		v->R      = NULL;
            }
            else if (v->T.basicType == T_STRING)
            {
		disposeStringRvals(v->R);
		v->T.size = 0;
		v->R      = NULL;
            }
	}
	else 	/* create variable */
	{   v = RcreateUVar(name,root,T_UNDEF);
	    TPRINT1(2,"readfromdisk: creating variable '%s'\n",name);
	}
	if (v == NULL)
	{   Werrprintf("fread:Variable \"%s\" could not be found or created\n",name);
	    return(0);
	}
        v->subtype = tmp.subtype;
        v->T.basicType = tmp.T.basicType;
        v->maxVal = tmp.maxVal;
        v->minVal = tmp.minVal;
        v->step = tmp.step;
        v->Ggroup = tmp.Ggroup;
        v->Dgroup = tmp.Dgroup;
        v->prot = tmp.prot;
	active = v->active = tmp.active; /* store this because of active kludge */
	if (fscanf(stream,"%d",&num)== EOF)
	    break;
	TPRINT1(2,"readfromdisk: num of values =%d\n",num);
	switch (v->T.basicType)
	{ case T_REAL:
		/*  Read the values */
		for (i=1; i<num+1; i++)
		{
		    if (fscanf(stream,"%lg",&doub)==EOF)
			break;
		    if (i == 1) /* is this first element */
			assignReal(doub,v,0); /* clear and set new variable */
		    else
			assignReal(doub,v,i);
		    TPRINT1(2,"readfromdisk: read T_REAL %g\n",doub);
		}
		/* since assignReal turns active ON, we must turn it back off
		     if necessary */
		if (active == ACT_OFF)
		    v->active = ACT_OFF;
		disposeRealRvals(v->E); /* clear all enumeration values */
		v->ET.size = 0;
		v->E = NULL;
		/*  Read the enumeration values */
		if (fscanf(stream,"%d",&num)== EOF)
		    break;
		TPRINT1(2,"readfromdisk: num of enums =%d\n",num);
		for (i=1; i<num+1; i++)
		{
		    if (fscanf(stream,"%lg",&doub)==EOF)
			break;
		    if (i == 1) /* is this first element */
			assignEReal(doub,v,0); /* clear and set new variable */
		    else
			assignEReal(doub,v,i);
		    TPRINT1(2,"readfromdisk: read T_REAL %g\n",doub);
		}
		break;
	  case T_STRING:
		/*  Read values */
		for (i=1; i<num+1; i++)
		{   if (rscanf(stream,buf,1024)<0)
		    {	Werrprintf("readfromdisk: premature EOF\n");
			return(0);
		    }
		    else
		     	if (i == 1) /* is this first element ? */
			    assignString(buf,v,0);
			else
			    assignString(buf,v,i);
		    TPRINT1(2,"readfromdisk: read T_STRING %s\n",buf);
		}
		/* since assignString turns active ON, we must turn it back off
		     if necessary */
		if (active == ACT_OFF)
		    v->active = ACT_OFF;
		disposeStringRvals(v->E); /* clear all enumeration values */
		v->ET.size = 0;
		v->E = NULL;
		/*  Read the enumeration values */
		if (fscanf(stream,"%d",&num)== EOF)
		    break;
		TPRINT1(2,"readfromdisk: num of enums =%d\n",num);
		for (i=1; i<num+1; i++)
		{   if (rscanf(stream,buf,1024)<0)
		    {	Werrprintf("readfromdisk: premature EOF\n");
			return(0);
		    }
		    else
		     	if (i == 1) /* is this first element ? */
			    assignEString(buf,v,0);
			else
			    assignEString(buf,v,i);
		    TPRINT1(2,"readfromdisk: read T_STRING %s\n",buf);
		}
		break;
	  default:
		Werrprintf("readfromdisk: %s has nonexistent type\n", name);
		return(0);
	}
    }
    return -1;
}

/*------------------------------------------------------------------------------
|
|	readNamesFromDisk/2
|
|	This function reads in variables off a stream and stores
|	names in a string
|
+-----------------------------------------------------------------------------*/

static char *readNamesFromDisk(FILE *stream)
{
    char        buf[1025];
    char        name[256];
    char       *localnames;
    double      doub;
    short       sval;
    int         ival;
    short       basicType;
    int         i;
    int         intptr;
    int         num;
    int         ret;

    ret = -1;
    localnames = NULL;
    for(;;)
    {	if(fscanf(stream,"%s",name)==EOF)
	    break;
        if (localnames == NULL)
        {
           localnames = newStringId(name,"rnfd"); /* rnfd == read names from disk */
        }
        else
        {
           sprintf(buf," %s",name);
           localnames = newCatId(localnames, buf, "rnfd"); /* rnfd == read names from disk */
        }
	TPRINT1(2,"readNamesFromDisk: reading variable \"%s\"\n",name);
	ret =fscanf(stream,"%hd %hd %lf %lf %lf %hd %hd %d %hd %x",
	    &sval,&basicType,&doub,&doub,&doub,&sval,&sval,&ival,&sval,&intptr);
	TPRINT1(2,"readNamesFromDisk: we read  %d things \n",ret);
	if (fscanf(stream,"%d",&num)== EOF)
	    break;
	TPRINT1(2,"readNamesFromDisk: num of values =%d\n",num);
	switch (basicType)
	{ case T_REAL:
		/*  Read the values */
		for (i=1; i<num+1; i++)
		{
		    if (fscanf(stream,"%lg",&doub)==EOF)
			break;
		}
		/*  Read the enumeration values */
		if (fscanf(stream,"%d",&num)== EOF)
		    break;
		TPRINT1(2,"readNamesFromDisk: num of enums =%d\n",num);
		for (i=1; i<num+1; i++)
		{
		    if (fscanf(stream,"%lg",&doub)==EOF)
			break;
		}
		break;
	  case T_STRING:
		/*  Read values */
		for (i=1; i<num+1; i++)
		{   if (rscanf(stream,buf,1024)<0)
		    {	Werrprintf("readNamesFromDisk: premature EOF\n");
			return(NULL);
		    }
		}
		/*  Read the enumeration values */
		if (fscanf(stream,"%d",&num)== EOF)
		    break;
		TPRINT1(2,"readNamesFromDisk: num of enums =%d\n",num);
		for (i=1; i<num+1; i++)
		{   if (rscanf(stream,buf,1024)<0)
		    {	Werrprintf("readNamesFromDisk: premature EOF\n");
			return(NULL);
		    }
		}
		break;
	  default:
		Werrprintf("readNamesFromDisk: %s has nonexistent type\n", name);
		return(NULL);
	}
    }
    return(localnames);
}

/*------------------------------------------------------------------------------
|
|	readNewVarsfromdisk/2
|
|	This function reads in variables off a stream and stores
|	them in a tree. As distinct from readfromdisk, this
|       procedure will not create new parameters nor change parameter
|       attributes.  If parameter types are different, the original
|       is kept.
|
+-----------------------------------------------------------------------------*/

int readNewVarsfromdisk(symbol **root, FILE *stream)
{
    char        buf[1025];
    char        name[256];
    double      doub;
    int         i;
    int         intptr;
    int         num;
    int         ret;
    int         active;
    varInfo    *v;
    varInfo     dummy;
    int         skip;

    ret = -1;
    for(;;)
    {	if(fscanf(stream,"%s",name)==EOF)
	    break;
	TPRINT1(2,"readfromdisk: reading variable \"%s\"\n",name);
	if ( (v = rfindVar(name,root)) ) /* does variable already exist */
        {
           v = &dummy;
           skip = 1;
        }
	else 	/* create variable */
	{  v = RcreateUVar(name,root,T_UNDEF);
	   TPRINT1(2,"readfromdisk: creating variable '%s'\n",name);
           skip = 0;
	}
	if (v == NULL)
	{   Werrprintf("fread:Variable \"%s\" could not be created\n",name);
	    return(0);
	}
	ret =fscanf(stream,"%hd %hd %lf %lf %lf %hd %hd %d %hd %x",
	    &v->subtype,&v->T.basicType,&v->maxVal,&v->minVal,
	    &v->step,&v->Ggroup,&v->Dgroup,&v->prot,&v->active,&intptr);
	active = v->active; /* store this because of active kludge */
	TPRINT1(2,"readfromdisk: we read  %d things \n",ret);
	TPRINT1(2,"readfromdisk: var name \"%s\"\n",name);
	TPRINT2(2,"readfromdisk:%d %d\n",v->subtype,v->T.basicType);
	TPRINT3(2,"readfromdisk:%g %g %g\n",v->maxVal,v->minVal,v->step);
	TPRINT5(2,"readfromdisk:%d %d %d %d %x\n",
	    v->Ggroup,v->Dgroup,v->prot,v->active,intptr);
	if (fscanf(stream,"%d",&num)== EOF)
	    break;
	TPRINT1(2,"readfromdisk: num of values =%d\n",num);
	switch (v->T.basicType)
	{ case T_REAL:
		/*  Read the values */
		for (i=1; i<num+1; i++)
		{
#ifdef UNIX
		    if (fscanf(stream,"%lg",&doub)==EOF)
			break;
#else 
		    if (fscanf(stream,"%lf",&doub)==EOF)
			break;
#endif 

                    if (!skip)
                    {
	  	      if (i == 1) /* is this first element */
			assignReal(doub,v,0); /* clear and set new variable */
		      else
			assignReal(doub,v,i);
                    }
		    TPRINT1(2,"readfromdisk: read T_REAL %g\n",doub);
		}
		/* since assignReal turns active ON, we must turn it back off
		     if necessary */
		if (!skip)
                {
		   if (active == ACT_OFF)
		      v->active = ACT_OFF;
		   disposeRealRvals(v->E); /* clear all enumeration values */
		   v->ET.size = 0;
		   v->E = NULL;
                }
		/*  Read the enumeration values */
		if (fscanf(stream,"%d",&num)== EOF)
		    break;
		TPRINT1(2,"readfromdisk: num of enums =%d\n",num);
		for (i=1; i<num+1; i++)
		{
#ifdef UNIX
		    if (fscanf(stream,"%lg",&doub)==EOF)
			break;
#else 
		    if (fscanf(stream,"%lf",&doub)==EOF)
			break;
#endif 
                    if (!skip)
                    {
		      if (i == 1) /* is this first element */
			assignEReal(doub,v,0); /* clear and set new variable */
		      else
			assignEReal(doub,v,i);
                    }
		    TPRINT1(2,"readfromdisk: read T_REAL %g\n",doub);
		}
		break;
	  case T_STRING:
		/*  Read values */
		for (i=1; i<num+1; i++)
		{   if (rscanf(stream,buf,1024)<0)
		    {	Werrprintf("readfromdisk: premature EOF\n");
			return(0);
		    }
		    else if (!skip)
                    {
		     	if (i == 1) /* is this first element ? */
			    assignString(buf,v,0);
			else
			    assignString(buf,v,i);
                    }
		    TPRINT1(2,"readfromdisk: read T_STRING %s\n",buf);
		}
		/* since assignString turns active ON, we must turn it back off
		     if necessary */
		if (!skip)
                {
		  if (active == ACT_OFF)
		    v->active = ACT_OFF;
		  disposeStringRvals(v->E); /* clear all enumeration values */
		  v->ET.size = 0;
		  v->E = NULL;
                }
		/*  Read the enumeration values */
		if (fscanf(stream,"%d",&num)== EOF)
		    break;
		TPRINT1(2,"readfromdisk: num of enums =%d\n",num);
		for (i=1; i<num+1; i++)
		{   if (rscanf(stream,buf,1024)<0)
		    {	Werrprintf("readfromdisk: premature EOF\n");
			return(0);
		    }
		    else if (!skip)
                    {
		     	if (i == 1) /* is this first element ? */
			    assignEString(buf,v,0);
			else
			    assignEString(buf,v,i);
                    }
		    TPRINT1(2,"readfromdisk: read T_STRING %s\n",buf);
		}
		break;
	  default:
		Werrprintf("readfromdisk: %s has nonexistent type\n", name);
		return(0);
	}
    }
    return -1;
}

/*------------------------------------------------------------------------------
|
|	readGroupNewVarsfromdisk/2
|
|	This function reads in variables off a stream and stores
|	them in a tree. As distinct from readfromdisk, this
|       procedure will not create new parameters nor change parameter
|       attributes.  If parameter types are different, the original
|       is kept.
|
+-----------------------------------------------------------------------------*/

int readGroupNewVarsfromdisk(symbol **root, FILE *stream, int groupIndex)
{
    char        buf[1025];
    char        name[256];
    double      doub;
    int         i;
    int         intptr;
    int         num;
    int         ret;
    int         active;
    varInfo    *v;
    varInfo     tmp;
    int         skip;

    ret = -1;
    for(;;)
    {	if(fscanf(stream,"%s",name)==EOF)
	    break;
	TPRINT1(2,"readfromdisk: reading variable \"%s\"\n",name);
	ret =fscanf(stream,"%hd %hd %lf %lf %lf %hd %hd %d %hd %x",
	    &tmp.subtype,&tmp.T.basicType,&tmp.maxVal,&tmp.minVal,
	    &tmp.step,&tmp.Ggroup,&tmp.Dgroup,&tmp.prot,&tmp.active,&intptr);
	TPRINT1(2,"readfromdisk: we read  %d things \n",ret);
	TPRINT1(2,"readfromdisk: var name \"%s\"\n",name);
	TPRINT2(2,"readfromdisk:%d %d\n",tmp.subtype,tmp.T.basicType);
	TPRINT3(2,"readfromdisk:%g %g %g\n",tmp.maxVal,tmp.minVal,tmp.step);
	TPRINT5(2,"readfromdisk:%d %d %d %d %x\n",
	    tmp.Ggroup,tmp.Dgroup,tmp.prot,tmp.active,intptr);
        if (tmp.Ggroup != groupIndex)
        {
            skipVar(name, tmp.T.basicType, stream);
            continue;
        }
	if ( (v = rfindVar(name,root)) ) /* does variable already exist */
        {
           v = &tmp;
           skip = 1;
        }
	else 	/* create variable */
	{  v = RcreateUVar(name,root,T_UNDEF);
	   TPRINT1(2,"readfromdisk: creating variable '%s'\n",name);
           skip = 0;
	}
	if (v == NULL)
	{   Werrprintf("fread:Variable \"%s\" could not be created\n",name);
	    return(0);
	}
        v->subtype = tmp.subtype;
        v->T.basicType = tmp.T.basicType;
        v->maxVal = tmp.maxVal;
        v->minVal = tmp.minVal;
        v->step = tmp.step;
        v->Ggroup = tmp.Ggroup;
        v->Dgroup = tmp.Dgroup;
        v->prot = tmp.prot;
	active = v->active = tmp.active; /* store this because of active kludge */
	if (fscanf(stream,"%d",&num)== EOF)
	    break;
	TPRINT1(2,"readfromdisk: num of values =%d\n",num);
	switch (v->T.basicType)
	{ case T_REAL:
		/*  Read the values */
		for (i=1; i<num+1; i++)
		{
		    if (fscanf(stream,"%lg",&doub)==EOF)
			break;
                    if (!skip)
                    {
	  	      if (i == 1) /* is this first element */
			assignReal(doub,v,0); /* clear and set new variable */
		      else
			assignReal(doub,v,i);
                    }
		    TPRINT1(2,"readfromdisk: read T_REAL %g\n",doub);
		}
		/* since assignReal turns active ON, we must turn it back off
		     if necessary */
		if (!skip)
                {
		   if (active == ACT_OFF)
		      v->active = ACT_OFF;
		   disposeRealRvals(v->E); /* clear all enumeration values */
		   v->ET.size = 0;
		   v->E = NULL;
                }
		/*  Read the enumeration values */
		if (fscanf(stream,"%d",&num)== EOF)
		    break;
		TPRINT1(2,"readfromdisk: num of enums =%d\n",num);
		for (i=1; i<num+1; i++)
		{
		    if (fscanf(stream,"%lg",&doub)==EOF)
			break;
                    if (!skip)
                    {
		      if (i == 1) /* is this first element */
			assignEReal(doub,v,0); /* clear and set new variable */
		      else
			assignEReal(doub,v,i);
                    }
		    TPRINT1(2,"readfromdisk: read T_REAL %g\n",doub);
		}
		break;
	  case T_STRING:
		/*  Read values */
		for (i=1; i<num+1; i++)
		{   if (rscanf(stream,buf,1024)<0)
		    {	Werrprintf("readfromdisk: premature EOF\n");
			return(0);
		    }
		    else if (!skip)
                    {
		     	if (i == 1) /* is this first element ? */
			    assignString(buf,v,0);
			else
			    assignString(buf,v,i);
                    }
		    TPRINT1(2,"readfromdisk: read T_STRING %s\n",buf);
		}
		/* since assignString turns active ON, we must turn it back off
		     if necessary */
		if (!skip)
                {
		  if (active == ACT_OFF)
		    v->active = ACT_OFF;
		  disposeStringRvals(v->E); /* clear all enumeration values */
		  v->ET.size = 0;
		  v->E = NULL;
                }
		/*  Read the enumeration values */
		if (fscanf(stream,"%d",&num)== EOF)
		    break;
		TPRINT1(2,"readfromdisk: num of enums =%d\n",num);
		for (i=1; i<num+1; i++)
		{   if (rscanf(stream,buf,1024)<0)
		    {	Werrprintf("readfromdisk: premature EOF\n");
			return(0);
		    }
		    else if (!skip)
                    {
		     	if (i == 1) /* is this first element ? */
			    assignEString(buf,v,0);
			else
			    assignEString(buf,v,i);
                    }
		    TPRINT1(2,"readfromdisk: read T_STRING %s\n",buf);
		}
		break;
	  default:
		Werrprintf("readfromdisk: %s has nonexistent type\n", name);
		return(0);
	}
    }
    return -1;
}

/*------------------------------------------------------------------------------
|
|	readGroupvaluesfromdisk/2
|
|	This function reads in variables off a stream and stores
|	them in a tree. As distinct from readfrom disk, this
|       procedure will not create new parameters nor change parameter
|       attributes.  If parameter types are different, the original
|       is kept.
|
+-----------------------------------------------------------------------------*/

int readGroupvaluesfromdisk(symbol **root, FILE *stream, int groupIndex)
{
    char        buf[1025];
    char        name[256];
    double      doub;
    int         i;
    int         intptr;
    int         setvalue;
    int         num;
    int         ret;
    varInfo    *v;
    varInfo    tmp;

    ret = -1;
    for(;;)
    {	if(fscanf(stream,"%s",name)==EOF)
	    break;
	TPRINT1(2,"readvaluesfromdisk: reading variable \"%s\"\n",name);
	ret =fscanf(stream,"%hd %hd %lf %lf %lf %hd %hd %d %hd %x",
	    &tmp.subtype,&tmp.T.basicType,&tmp.maxVal,&tmp.minVal,
	    &tmp.step,&tmp.Ggroup,&tmp.Dgroup,&tmp.prot,&tmp.active,&intptr);
	TPRINT1(2,"readfromdisk: we read  %d things \n",ret);
	TPRINT1(2,"readfromdisk: var name \"%s\"\n",name);
	TPRINT2(2,"readfromdisk:%d %d\n",tmp.subtype,tmp.T.basicType);
	TPRINT3(2,"readfromdisk:%g %g %g\n",tmp.maxVal,tmp.minVal,tmp.step);
	TPRINT5(2,"readfromdisk:%d %d %d %d %x\n",
	    tmp.Ggroup,tmp.Dgroup,tmp.prot,tmp.active,intptr);
        if (tmp.Ggroup != groupIndex)
        {
            skipVar(name, tmp.T.basicType, stream);
            continue;
        }
	if ( (v = rfindVar(name,root)) ) /* does variable already exist */
	{
	   TPRINT1(2,"readvaluesfromdisk: found variable %s\n",name);
	   if (fscanf(stream,"%d",&num)== EOF)
	      break;
	   TPRINT1(2,"readvaluesfromdisk: num of values =%d\n",num);
           setvalue = (v->T.basicType == tmp.T.basicType);
	   switch (v->T.basicType)
	   { case T_REAL:
		/*  Read the values */
		for (i=1; i<num+1; i++)
		{
		    if (fscanf(stream,"%lg",&doub)==EOF)
			break;
                    if (setvalue)
                    {
		    if (i == 1) /* is this first element */
			assignReal(doub,v,0); /* clear and set new variable */
		    else
			assignReal(doub,v,i);
                    }
		    TPRINT1(2,"readvaluesfromdisk: read T_REAL %g\n",doub);
		}
		/*  Read the enumeration values */
		if (fscanf(stream,"%d",&num)== EOF)
		    break;
		TPRINT1(2,"readvaluesfromdisk: num of enums =%d\n",num);
		for (i=1; i<num+1; i++)
		{
		    if (fscanf(stream,"%lg",&doub)==EOF)
			break;
		}
		break;
	     case T_STRING:
		/*  Read values */
		for (i=1; i<num+1; i++)
		{   if (rscanf(stream,buf,1024)<0)
		    {	Werrprintf("readvaluesfromdisk: premature EOF\n");
			return(0);
		    }
		    else if (setvalue)
                    {
		     	if (i == 1) /* is this first element ? */
			    assignString(buf,v,0);
			else
			    assignString(buf,v,i);
                    }
		    TPRINT1(2,"readvaluesfromdisk: read T_STRING %s\n",buf);
		}
		/*  Read the enumeration values */
		if (fscanf(stream,"%d",&num)== EOF)
		    break;
		TPRINT1(2,"readvaluesfromdisk: num of enums =%d\n",num);
		for (i=1; i<num+1; i++)
		{   if (rscanf(stream,buf,1024)<0)
		    {	Werrprintf("readvaluesfromdisk: premature EOF\n");
			return(0);
		    }
		}
		break;
	     default:
		Werrprintf("readvaluesfromdisk: %s has nonexistent type\n", name);
		return(0);
	   }
	}
        else
        {
           skipVar(name, tmp.T.basicType, stream);
        }
    }
    return -1;
}

/*------------------------------------------------------------------------------
|
|	readvaluesfromdisk/2
|
|	This function reads in variables off a stream and stores
|	them in a tree. As distinct from readfrom disk, this
|       procedure will not create new parameters nor change parameter
|       attributes.  If parameter types are different, the original
|       is kept.
|
+-----------------------------------------------------------------------------*/

int readvaluesfromdisk(symbol **root, FILE *stream)
{
    char        buf[1025];
    char        name[256];
    double      doub;
    int         i;
    int         intptr;
    int         setvalue;
    int         num;
    int         ret;
    short       newBasic;
    varInfo    *v;

    ret = -1;
    for(;;)
    {	if(fscanf(stream,"%s",name)==EOF)
	    break;
	TPRINT1(2,"readvaluesfromdisk: reading variable \"%s\"\n",name);
	if ( (v = rfindVar(name,root)) ) /* does variable already exist */
	{
           double dumd1,dumd2,dumd3;
           short  dums1,dums2,dums3,dums4;
           int    dumi1;

	   TPRINT1(2,"readvaluesfromdisk: found variable %s\n",name);
	   ret =fscanf(stream,"%hd %hd %lf %lf %lf %hd %hd %d %hd %x",
	               &dums1,&newBasic,&dumd1,&dumd2,
	               &dumd3,&dums2,&dums3,&dumi1,&dums4,&intptr);
	   TPRINT1(2,"readvaluesfromdisk: we read  %d things \n",ret);
	   TPRINT1(2,"readvaluesfromdisk: var name \"%s\"\n",name);
	   TPRINT2(2,"readvaluesfromdisk:%d %d\n",dums1,newBasic);
	   TPRINT3(2,"readvaluesfromdisk:%g %g %g\n",dumd1,dumd2,dumd3);
	   if (fscanf(stream,"%d",&num)== EOF)
	      break;
	   TPRINT1(2,"readvaluesfromdisk: num of values =%d\n",num);
           setvalue = (v->T.basicType == newBasic);
	   switch (v->T.basicType)
	   { case T_REAL:
		/*  Read the values */
		for (i=1; i<num+1; i++)
		{
		    if (fscanf(stream,"%lg",&doub)==EOF)
			break;
                    if (setvalue)
                    {
		    if (i == 1) /* is this first element */
			assignReal(doub,v,0); /* clear and set new variable */
		    else
			assignReal(doub,v,i);
                    }
		    TPRINT1(2,"readvaluesfromdisk: read T_REAL %g\n",doub);
		}
		/*  Read the enumeration values */
		if (fscanf(stream,"%d",&num)== EOF)
		    break;
		TPRINT1(2,"readvaluesfromdisk: num of enums =%d\n",num);
		for (i=1; i<num+1; i++)
		{
		    if (fscanf(stream,"%lg",&doub)==EOF)
			break;
		}
		break;
	     case T_STRING:
		/*  Read values */
		for (i=1; i<num+1; i++)
		{   if (rscanf(stream,buf,1024)<0)
		    {	Werrprintf("readvaluesfromdisk: premature EOF\n");
			return(0);
		    }
		    else if (setvalue)
                    {
		     	if (i == 1) /* is this first element ? */
			    assignString(buf,v,0);
			else
			    assignString(buf,v,i);
                    }
		    TPRINT1(2,"readvaluesfromdisk: read T_STRING %s\n",buf);
		}
		/*  Read the enumeration values */
		if (fscanf(stream,"%d",&num)== EOF)
		    break;
		TPRINT1(2,"readvaluesfromdisk: num of enums =%d\n",num);
		for (i=1; i<num+1; i++)
		{   if (rscanf(stream,buf,1024)<0)
		    {	Werrprintf("readvaluesfromdisk: premature EOF\n");
			return(0);
		    }
		}
		break;
	     default:
		Werrprintf("readvaluesfromdisk: %s has nonexistent type\n", name);
		return(0);
	   }
	}
        else
        {
           double dumd1,dumd2,dumd3;
           short  dums1,dums2,dums3,dums4;
           int    dumi1;

	   TPRINT1(2,"readvaluesfromdisk: skip variable %s\n",name);
	   ret =fscanf(stream,"%hd %hd %lf %lf %lf %hd %hd %d %hd %x",
	               &dums1,&newBasic,&dumd1,&dumd2,
	               &dumd3,&dums2,&dums3,&dumi1,&dums4,&intptr);
	   TPRINT1(2,"readvaluesfromdisk: we read  %d things \n",ret);
	   TPRINT1(2,"readvaluesfromdisk: var name \"%s\"\n",name);
	   TPRINT2(2,"readvaluesfromdisk:%d %d\n",dums1,newBasic);
	   TPRINT3(2,"readvaluesfromdisk:%g %g %g\n",dumd1,dumd2,dumd3);
	   if (fscanf(stream,"%d",&num)== EOF)
	      break;
	   TPRINT1(2,"readvaluesfromdisk: num of values =%d\n",num);
	   switch (newBasic)
	   { case T_REAL:
		/*  Read the values */
		for (i=1; i<num+1; i++)
		{
		    if (fscanf(stream,"%lg",&doub)==EOF)
			break;
		    TPRINT1(2,"readvaluesfromdisk: read T_REAL %g\n",doub);
		}
		/*  Read the enumeration values */
		if (fscanf(stream,"%d",&num)== EOF)
		    break;
		TPRINT1(2,"readvaluesfromdisk: num of enums =%d\n",num);
		for (i=1; i<num+1; i++)
		{
		    if (fscanf(stream,"%lg",&doub)==EOF)
			break;
		}
		break;
	     case T_STRING:
		/*  Read values */
		for (i=1; i<num+1; i++)
		{   if (rscanf(stream,buf,1024)<0)
		    {	Werrprintf("readvaluesfromdisk: premature EOF\n");
			return(0);
		    }
		    TPRINT1(2,"readvaluesfromdisk: read T_STRING %s\n",buf);
		}
		/*  Read the enumeration values */
		if (fscanf(stream,"%d",&num)== EOF)
		    break;
		TPRINT1(2,"readvaluesfromdisk: num of enums =%d\n",num);
		for (i=1; i<num+1; i++)
		{   if (rscanf(stream,buf,1024)<0)
		    {	Werrprintf("readvaluesfromdisk: premature EOF\n");
			return(0);
		    }
		}
		break;
	     default:
		Werrprintf("readvaluesfromdisk: %s has nonexistent type\n", name);
		return(0);
	   }
        }
    }
    return -1;
}

/* return next string delimited by "*/
static int rscanf(FILE *stream, char *buf, int bufmax)
{   register int c;
    register int i;

    i=0;
    /*  find beginning " */
    c = fgetc(stream);
    while(c!='"'&& c!=EOF)
	c=fgetc(stream);   
    if (c==EOF) /* error */
       return(-1);
    while ((c=fgetc(stream))!='"')
    {
        if (c == '\\')
        {
          c=fgetc(stream);
        }
        buf[i++]=c;
	if(bufmax -1 <= i)
	{   Werrprintf("rscanf:ERROR in rscan, string > %d\n",bufmax);
	    buf[bufmax-1]=0;
	    return bufmax-1;
	}
    }
    buf[i]=0; /* set last value to zero */
    TPRINT1(2,"rscan:returning string \"%s\"\n",buf);
    return (i);
}


/*---------------------------------------
|					|
|	    P_getparinfo()/4		|
|					|
|   This function retrieves the value	|
|   of the parameter and its status,	|
|   i.e., active/inactive.		|
|					|
+--------------------------------------*/
int P_getparinfo(int tree, const char *pname, double *pval, int *pstatus)
{
  int	r;
  vInfo	info;

  if ( (r = P_getreal(tree, pname, pval, 1)) )
  {
     return(ERROR);
  }

  if (pstatus != NULL)
  {
     if ( (r = P_getVarInfo(tree, pname, &info)) )
     {
        P_err(r, "info?", pname);
        return(ERROR);
     }

     *pstatus = info.active;
  }

  return(COMPLETE);
}

/*------------------------------------------------------------------------------
|
|	P_getmax/4
|
|	This function returns the parameter's maximum value.
|
+-----------------------------------------------------------------------------*/

int P_getmax(int tree, const char *pname, double *pval)
{
  int	r;
  vInfo	v;

  if ( (r = P_getVarInfo(tree, pname, &v)) )
  {
     return(r);
  }

  if (v.prot & P_MMS)
  {
     int pindex;
     double maxv;

     pindex = (int) (v.maxVal+0.1);
     if ( (r = P_getreal( SYSTEMGLOBAL, "parmax", &maxv, pindex )) )
     {
        return(r);
     }
     *pval = maxv;
       
  }
  else
  {
     *pval = v.maxVal;
  }
  return(0);
}

int P_getmin(int tree, const char *pname, double *pval)
{
  int	r;
  vInfo	v;

  if ( (r = P_getVarInfo(tree, pname, &v)) )
  {
     return(r);
  }

  if (v.prot & P_MMS)
  {
     int pindex;
     double minv;

     pindex = (int) (v.minVal+0.1);
     if ( (r = P_getreal( SYSTEMGLOBAL, "parmin", &minv, pindex )) )
     {
        return(r);
     }
     *pval = minv;
       
  }
  else
  {
     *pval = v.minVal;
  }
  return(0);
}

/*------------------------------------------------------------------------------
|
|	P_err/3
|
|	This routine prints out an error message based on the res code.
|	res is the result code; s is the message; t is the variable name.
|
+-----------------------------------------------------------------------------*/
#define MESSIZE 128
void P_err(int res, const char *s, const char *t)
{   char littleBuf[MESSIZE];
    char biggerBuf[MESSIZE + 30];
    
    sprintf(littleBuf,"%s%s",s,t);
    switch(res)
    { case -1:
		sprintf(biggerBuf,"%sTree doesn't exist",littleBuf);
		break;
      case -2:
		sprintf(biggerBuf,"%sVariable doesn't exist",littleBuf);
		break;
      case -3:
		sprintf(biggerBuf,"%sValue exceeds upper limit",littleBuf);
		break;
      case -4:
		sprintf(biggerBuf,"%sValue exceeds lower limit",littleBuf);
		break;
      case -5:
		sprintf(biggerBuf,"%sValue violates step size",littleBuf);
		break;
      case -6:
		sprintf(biggerBuf,"%sValue violates enum type",littleBuf);
		break;
      case -7:
		sprintf(biggerBuf,"%sVariable is write protected",littleBuf);
		break;
      case -8:
		sprintf(biggerBuf,"%sMalloc error (couldn't get any memory) ",
			littleBuf);
		break;
      case -9:
		sprintf(biggerBuf,"%sIndex out-of-bounds",littleBuf);
		break;
      case -10:
		sprintf(biggerBuf,"%sWrong type ",littleBuf);
		break;
      case -11:
		sprintf(biggerBuf,"%sString larger than max length",littleBuf);
		break;
      case -12:
		sprintf(biggerBuf,
			"%sReal variable value adjusted to nearest enumeral",
			littleBuf);
		break;
      case -13:
		sprintf(biggerBuf,"%sString Enumeral value not matched",
			littleBuf);
		break;
      case -14:
		sprintf(biggerBuf,"%sNo such file",littleBuf);
		break;
      case -15:
		sprintf(biggerBuf,"%sPremature EOF (ran out of file)",littleBuf);
		break;
      case -16:
		sprintf(biggerBuf,"%sError number out of range",littleBuf);
		break;
      case -17:
		sprintf(biggerBuf,"%sInvalid group",littleBuf);
		break;
      case -18:
		sprintf(biggerBuf,"%sBad filename ",littleBuf);
		break;
      case -19:
		sprintf(biggerBuf,"%serror writing file",littleBuf);
		break;
     default:
		sprintf(biggerBuf,"%sUdefined error number %d",
				littleBuf,res);
		break;
     }
     Werrprintf(biggerBuf);
}
