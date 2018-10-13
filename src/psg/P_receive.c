/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <sys/file.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include "group.h"
#include "abort.h"
#include "symtab.h"

#define VNMRJ
#include "variables.h"

extern int bgflag;
static int havelockfreq = 0;  /* flag used to determine if lockfreq was obtained
			       * twice sysglobal & global */

extern int assignReal(double d, varInfo *v, int i);
extern int assignString(char *s, varInfo *v, int i);
extern void disposeRealRvals(Rval *r);
extern void disposeStringRvals(Rval *r);

int P_loadVar(int tree, char *name,  vInfo *v, int *fd);

/*----------------------------------------------------------------------------
|	P_receive/1
|
|	This routine receive variables from the parent pipe and stores 
|	them in trees.
|
+----------------------------------------------------------------------------*/

int P_receive(int *fd)
{   char    name[128];
    int     nread;
    int     nsize;
    int     op;
    vInfo   v;

    if (bgflag > 2)
    {
	fprintf(stderr,"P_receive: fd = %d\n",fd[0]);
    }
    while(1)
    {	nread = read(fd[0],&op,sizeof(int)); /* read operation code */ 
	if (bgflag > 2)
	    fprintf(stderr,"bg: Op code = %d\n",op);
	if (op < 0)
        {
	   if (havelockfreq == -1)
	   {
	      return(-1);
           }
	   return(1);
        }
	/* read in variables */
	nread = read(fd[0],&nsize, sizeof(int)); /* get length of name */
	if (bgflag > 2)
	    fprintf(stderr,"bg: name length = %d\n",nsize);
	nread = read(fd[0],name,nsize);
	name[nsize] = '\0'; /* put a null on it */
	if (bgflag > 2)
	    fprintf(stderr,"bg: name = \"%s\"\n",name);
	nread = read(fd[0],&v,sizeof(vInfo)); /* read over variable info */
	if (bgflag > 2)
	    fprintf(stderr,"bg: %hd %hd %hd %hd %hd %hd %hd %d %g %g %g\n",
	    v.active,v.Dgroup,v.group,v.basicType,v.size,v.subtype,
	    v.Esize,v.prot,v.minVal,v.maxVal,v.step);
	P_loadVar(op,name,&v,fd);
    }
}

/*------------------------------------------------------------------------------
|
|	P_loadVar/4
|
|	This function copies one variable from pipe to a tree 
|	It first deletes the existing variable and copies over the new one.
|     Added 5/26/87   GB
|	As it loads the new values if the subtype == ST_PULSE the value
|	is multiply by 1e-6 to obtain pulse in seconds not usec.
|
+-----------------------------------------------------------------------------*/

int P_loadVar(int tree, char *name,  vInfo *v, int *fd) 	
{   char            buf[4097];
    double          dvalue;
    int             i; 
    int             length;
    int             nread;
    symbol        **root;
    varInfo        *newv;
    
/* If the variable was passed as a system global, store it
   in the global variable tree.					*/

/* if lockfreq is passed from the system global and global annouce the fact and
   abort  */
    if (tree==SYSTEMGLOBAL)	/* break if in two for speed,avoid strcmp if possible */
    {
      if (strcmp(name,"lockfreq") == 0) 
      {
         if (!havelockfreq)
	    havelockfreq=1;
         else
         {
	   text_error(
	    "lockfreq in both conpar and global, remove occurrence in global.\n");
           havelockfreq = -1;
         }
      }
    }
    if (tree==GLOBAL)	/* break if in two for speed,avoid strcmp if possible */
    {
      if (strcmp(name,"lockfreq") == 0) 
      {
         if (!havelockfreq)
	    havelockfreq=1;
         else
         {
	   text_error(
	    "lockfreq in both conpar and global, remove occurrence in global.\n");
           havelockfreq = -1;
         }
      }
    }
    if (tree==SYSTEMGLOBAL) tree = GLOBAL;
    if ( (root = getTreeRoot(getRoot(tree))) )
    {	if ( (newv=rfindVar(name,root)) ) /* if variable exists, get rid of it*/
	{   if (newv->T.basicType == T_STRING)  
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
	    newv->T.basicType  = v->basicType;
	    newv->ET.basicType = v->basicType;
	}
	else
	    newv = RcreateVar(name,root,v->basicType); /* create the variable */
	newv->active = v->active;
	newv->subtype= v->subtype;
	newv->Dgroup = v->Dgroup;
	newv->Ggroup = v->group;
	newv->prot   = v->prot;
	newv->minVal = v->minVal;
	newv->maxVal = v->maxVal;
	newv->step   = v->step;
	if (v->basicType == T_STRING)
	{    for (i=0 ; i<v->size ; i++)
	    {	nread = read(fd[0],&length,sizeof(int)); 
		nread = read(fd[0],buf,length);
		buf[length] = '\0';
		if (bgflag > 2)
		    fprintf(stderr,"bg: STRING[%d] = \"%s\"\n",i,buf);
		assignString(buf,newv,i+1);
	    }
	    /* copy over enumerals */
/* ---------------------- deleted
	    for (i=0 ; i<v->Esize ; i++)
	    {	nread = read(fd[0],&length,sizeof(int)); 
		nread = read(fd[0],buf,length);
		buf[length] = NULL;
		if (bgflag > 2)
		    fprintf(stderr,"bg: Enum STRING[%d] = \"%s\"\n",i,buf);
		assignEString(buf,newv,i+1); 
	    }
+------------------------- */
	}
	else /* assume T_REAL */
	{   for (i=0 ; i<v->size ; i++)
	    {	nread = read(fd[0],&dvalue,sizeof(double));
		/* convert usec pulse values in to seconds */
                if (v->subtype == ST_PULSE) /* pulse in usec */ 
                { 
                     dvalue *= 1.0e-6; 	/* now in sec. */
                }
		if (bgflag > 2)
		    fprintf(stderr,"bg: REAL[%d] = \"%g\"\n",i,dvalue);
		assignReal(dvalue,newv,i+1);
	    }
	    /*  copy over enumeral values */
/* ---------------------- deleted
	    for (i=0 ; i<v->Esize ; i++)
	    {	nread = read(fd[0],&dvalue,sizeof(double));
		if (bgflag > 2)
		    fprintf(stderr,"bg: Enum REAL[%d] = \"%g\"\n",i,dvalue);
		assignEReal(dvalue,newv,i+1);  
	    }
+------------------------- */
	}
    }
    else
    {
	fprintf(stderr,"P_loadVar: fatal error, cannot find tree %d\n",tree);
        havelockfreq = -1;  /* make PSG abort */
    }
    return(0);
}
