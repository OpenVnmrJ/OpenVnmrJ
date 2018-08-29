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
|	bgVars.c
|
|	These procedures are used to transfer variables from the
|	variable tree down a pipe. It can be used to pipe variables
|	to another task such as a background task.
|
+-----------------------------------------------------------------------------*/

/* Modified for conditional compliation for jpsg which defines JPSG */

#include "group.h"
#include "symtab.h"
#include "variables.h"
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

#ifdef __INTERIX
#include <arpa/inet.h>
#else
#ifdef SOLARIS
#include <sys/types.h>
#endif
#include <netinet/in.h>
#ifdef SOLARIS
#include <inttypes.h>
#endif
#endif


/*for Linux only,  it's in signals.h of Solaris*/
#ifndef  MAXSIG
#define MAXSIG  46
#endif

#ifndef VNMRJ
extern symbol **getTreeRoot(const char *n);
extern varInfo *rfindVar(const char *n, symbol **pp);
#endif
static int      endop = -1;

#ifdef  DEBUG
extern int      Tflag;
#define TPRINT0(val,str) \
	if (Tflag >= val) fprintf(stderr,str)
#define TPRINT1(val, str, arg1) \
	if (Tflag >= val) fprintf(stderr,str,arg1)
#define TPRINT2(val, str, arg1, arg2) \
	if (Tflag >= val) fprintf(stderr,str,arg1,arg2)
#define TPRINT3(val, str, arg1, arg2, arg3) \
	if (Tflag >= val) fprintf(stderr,str,arg1,arg2,arg3)
#else 
#define TPRINT0(val, str) 
#define TPRINT1(val, str, arg1) 
#define TPRINT2(val, str, arg1, arg2) 
#define TPRINT3(val, str, arg1, arg2, arg3) 
#endif 

sigset_t    blockMask;
sigset_t    saveMask;

#define SIGINTX1     SIGHUP
#define SIGINTX2     SIGCONT
#define SIGINTX3     (MAXSIG-1)

static void sendTree(int, symbol **,  int);
static void sendGroup(int, symbol **, int, int);
static void jSendTree(int, symbol **,  int);
static void jSendGroup(int, symbol **, int, int);
static void jPipeTransfer(char *name, varInfo *v, vInfo *vI, int fd);

void setupBlocking()
{
  /* All write calls need to be protected from signal
   * interrupts.
   */
  sigemptyset( &blockMask );
  sigaddset( &blockMask, SIGINT );
  sigaddset( &blockMask, SIGALRM );
  sigaddset( &blockMask, SIGIO );
  sigaddset( &blockMask, SIGCHLD );
  sigaddset( &blockMask, SIGQUIT );
  sigaddset( &blockMask, SIGTERM );
  sigaddset( &blockMask, SIGUSR1 );
  sigaddset( &blockMask, SIGUSR2 );
  sigaddset( &blockMask, SIGINTX1 );
  sigaddset( &blockMask, SIGINTX2 );
  sigaddset( &blockMask, SIGINTX3 );
}

/*------------------------------------------------------------------------------
|
|	pipeTransfer/3
|
|	This function write one variable to pipe
|
+-----------------------------------------------------------------------------*/

static void
pipeTransfer(int tree, char *name, varInfo *v, vInfo *vI, int fd)
{   
    register int   i;
    int   size;
    register Rval *r;

    size = strlen(name);
    sigprocmask( SIG_BLOCK, &blockMask, &saveMask );
    write(fd,&tree,sizeof(int)); /* send out op code */
    write(fd,&size,sizeof(int));
    write(fd,name,size);
    write(fd,vI,sizeof(vInfo)); 

    /*  write out good stuff */
    if (v->T.basicType == T_STRING)  /* send string values */
    {	i = 1; 
     	r = v->R;
	while (r && i < v->T.size+1)
	{   size = strlen(r->v.s);
	    write(fd,&size,sizeof(int));
	    write(fd,r->v.s,size);
	    i += 1;
	    r = r->next;
	}
    }
    else  /* copy over all reals */
    {	i = 1; 
     	r = v->R;
	while (r && i < v->T.size+1)
	{   write(fd,&r->v.r,sizeof(double));
	    i += 1;
	    r = r->next;
	}
    }
    sigprocmask( SIG_SETMASK, &saveMask, NULL );
}

/*------------------------------------------------------------------------------
|
|	P_sendTPVars/2
|
|	This function sends variables from a Tree down a Pipe.
|	Valid trees are GLOBAL, CURRENT,  PROCESSED	
|
+-----------------------------------------------------------------------------*/

int P_sendTPVars(int tree, int fd)
{   symbol **root;

    if ( (root = getTreeRoot(getRoot(tree))) )
    {	sendTree(tree,root,fd);
	return(0);
    }
    else
	return(-1); /* tree doesn't exist */
}

/*------------------------------------------------------------------------------
|
|	P_endPipe/2
|
|	This function sends an endop character down the pipe 
|
+-----------------------------------------------------------------------------*/

void P_endPipe(int fd)
{   
    sigprocmask( SIG_BLOCK, &blockMask, &saveMask );
    write(fd,&endop,sizeof(int));
    sigprocmask( SIG_SETMASK, &saveMask, NULL );
}

/*------------------------------------------------------------------------------
|
|	P_sendGPVars/3
|
|	This function sends variables from a Group down a Pipe.
|	Valid trees are GLOBAL, CURRENT,  PROCESSED	
|	The valid groups are G_SAMPLE,G_ACQUISITION,G_PROCESSING, G_DISPLAY,
|	G_SPIN
|
+-----------------------------------------------------------------------------*/

int P_sendGPVars(int tree, int group, int fd)
{   symbol **root;

    TPRINT3(1,"P_sendGPVars: piping from tree\"%s\" group\"%s\" pipe \"%d\"\n",
               getRoot(tree),whatGroup(group),fd);
    if ( (root = getTreeRoot(getRoot(tree))) )
    {	sendGroup(tree,root,group,fd);
	return(0);
    }
    else
	return(-1); /* tree doesn't exist */
}

/*------------------------------------------------------------------------------
|
|	P_sendVPVars/3
|
|	This function sends a single variable from a Tree down a Pipe.
|	Valid trees are GLOBAL, CURRENT,  PROCESSED	
|
+-----------------------------------------------------------------------------*/

int P_sendVPVars(int tree, char *name, int fd)
{   symbol  *p;
    symbol **root;
    varInfo *v;
    vInfo   vI;

    if ( (root = getTreeRoot(getRoot(tree))) )
    {	if ( (p=(*root)) )   /* check if there is at least something in from tree */
     	{
            TPRINT3(1,"P_sendVPVars: sending \"%s\" tree \"%s\" down pipe %d\n",
                       name,getRoot(tree),fd);
	    if ( (v=(varInfo *) rfindVar(name,root)) && (name[0] != '$') )
	    {	P_getVarInfo(tree,name,&vI);
		pipeTransfer(tree,name,v,&vI,fd);
		return(0);
	    }
	}
	return(-2); /* variable doesn't exist */
    }
    else
	return(-1); /* tree doesn't exist */
}

/*------------------------------------------------------------------------------
|
|	sendTree/3
|
|	This function copies all variables and its parameter from a tree
|	to a pipe.
|
+-----------------------------------------------------------------------------*/

static void sendTree(int tree, symbol **root, int fd)
{   symbol  *p;
    varInfo *v;
    vInfo    vI;
 
    if ( (p=(*root)) )   /* check if there is at least something in from tree */
    {	if (p->left)
	    sendTree(tree,&(p->left),fd);
	if (p->right)
	    sendTree(tree,&(p->right),fd);
        TPRINT1(3,"sendTree:  working on var \"%s\"\n",p->name);
	if (p->name)
	{   if ( (v=(varInfo *)(p->val)) && (p->name[0] != '$') )
	    {	P_getVarInfo(tree,p->name,&vI);
		pipeTransfer(tree,p->name,v,&vI,fd);
	    }
	}
    }
}

/*------------------------------------------------------------------------------
|
|	sendGroup/4
|
|	This function copies all variables and its parameter from a group
|	in a tree to a pipe.
|
+-----------------------------------------------------------------------------*/

static void sendGroup(int tree, symbol **root, int group, int fd)
{   
    symbol  *p;
    varInfo *v;
    vInfo    vI;
 
    if ( (p=(*root)) )   /* check if there is at least something in from tree */
    {	if (p->left)
	    sendGroup(tree,&(p->left),group,fd);
	if (p->right)
	    sendGroup(tree,&(p->right),group,fd);
        TPRINT1(3,"sendGroup:  working on var \"%s\"\n",p->name);
	if (p->name)
	{   if ( (v=(varInfo *)(p->val)) && (p->name[0] != '$') )
	    {	P_getVarInfo(tree,p->name,&vI);
		if (group == vI.group) /* if group match */
		{   
		    pipeTransfer(tree,p->name,v,&vI,fd);
		}
	    }
	}
    }
}

/* --------------------------------------------------------------------------------
 * ---------------    JPSG Varients of the Piping routines
 * ---------------      (actually we uses sockets)
 *  the main difference between these and the old psg counterparts  is
 *  that none of the enumeration/info structures are sent only
 *  the variable name, type, units, and values are sent
 * --------------------------------------------------------------------------------*/




/*------------------------------------------------------------------------------
|
|	J_sendTPVars/2
|
|	This function sends variables from a Tree down a Pipe.
|	Valid trees are GLOBAL, CURRENT,  PROCESSED	
|
+-----------------------------------------------------------------------------*/

int J_sendTPVars(int tree, int fd)
{   symbol **root;

    if ( (root = getTreeRoot(getRoot(tree))) )
    {	jSendTree(tree,root,fd);
	return(0);
    }
    else
	return(-1); /* tree doesn't exist */
}

/*------------------------------------------------------------------------------
|
|	J_sendGPVars/3
|
|	This function sends variables from a Group down a Pipe.
|	Valid trees are GLOBAL, CURRENT,  PROCESSED	
|	The valid groups are G_SAMPLE,G_ACQUISITION,G_PROCESSING, G_DISPLAY,
|	G_SPIN
|
+-----------------------------------------------------------------------------*/

int J_sendGPVars(int tree, int group, int fd)
{   symbol **root;

    TPRINT3(1,"J_sendGPVars: piping from tree\"%s\" group\"%s\" pipe \"%d\"\n",
               getRoot(tree),whatGroup(group),fd);
    if ( (root = getTreeRoot(getRoot(tree))) )
    {	jSendGroup(tree,root,group,fd);
	return(0);
    }
    else
	return(-1); /* tree doesn't exist */
}

/*------------------------------------------------------------------------------
|
|	J_sendVPVars/3
|
|	This function sends a single variable from a Tree down a Pipe.
|	Valid trees are GLOBAL, CURRENT,  PROCESSED	
|
+-----------------------------------------------------------------------------*/

int J_sendVPVars(int tree, char *name, int fd)
{   symbol  *p;
    symbol **root;
    varInfo *v;
    vInfo   vI;
    extern varInfo *rfindVar();


    if ( (root = getTreeRoot(getRoot(tree))) )
    {	if ( (p=(*root)) )   /* check if there is at least something in from tree */
     	{
            TPRINT3(1,"J_sendVPVars: sending \"%s\" tree \"%s\" down pipe %d\n",
                       name,getRoot(tree),fd);
	    if ( (v=(varInfo *) rfindVar(name,root)) && (name[0] != '$') )
	    {	P_getVarInfo(tree,name,&vI);
		/* write(fd,&tree,sizeof(int));  send out op code, not for Jpsg */
		jPipeTransfer(name,v,&vI,fd);
		return(0);
	    }
	}
	return(-2); /* variable doesn't exist */
    }
    else
	return(-1); /* tree doesn't exist */
}

/*------------------------------------------------------------------------------
|
|	jSendTree/3
|
|	This function copies all variables and its parameter from a tree
|	to a pipe.
|
+-----------------------------------------------------------------------------*/

static void jSendTree(int tree, symbol **root, int fd)
{   symbol  *p;
    varInfo *v;
    vInfo    vI;
 
    if ( (p=(*root)) )   /* check if there is at least something in from tree */
    {	if (p->left)
	    jSendTree(tree,&(p->left),fd);
	if (p->right)
	    jSendTree(tree,&(p->right),fd);
        TPRINT1(3,"jSendTree:  working on var \"%s\"\n",p->name);
	if (p->name)
	{   if ( (v=(varInfo *)(p->val)) && (p->name[0] != '$') )
	    {	P_getVarInfo(tree,p->name,&vI);
		/* write(fd,&tree,sizeof(int));  send out op code, not for Jpsg */
		jPipeTransfer(p->name,v,&vI,fd);
	    }
	}
    }
}

/*------------------------------------------------------------------------------
|
|	jSendGroup/4
|
|	This function copies all variables and its parameter from a group
|	in a tree to a pipe.
|
+-----------------------------------------------------------------------------*/

static void jSendGroup(int tree, symbol **root, int group, int fd) 
{   symbol  *p;
    varInfo *v;
    vInfo    vI;
 
    if ( (p=(*root)) )   /* check if there is at least something in from tree */
    {	if (p->left)
	    jSendGroup(tree,&(p->left),group,fd);
	if (p->right)
	    jSendGroup(tree,&(p->right),group,fd);
        TPRINT1(3,"sendGroup:  working on var \"%s\"\n",p->name);
	if (p->name)
	{   if ( (v=(varInfo *)(p->val)) && (p->name[0] != '$') )
	    {	P_getVarInfo(tree,p->name,&vI);
		if (group == vI.group) /* if group match */
		{   
		    /* write(fd,&tree,sizeof(int));  send out op code, not for Jpsg */
		    jPipeTransfer(p->name,v,&vI,fd);
		}
	    }
	}
    }
}

#ifdef LINUX
struct dswapbyte
{
   int s1;
   int s2;
};

typedef union
{
   double *in1;
   struct dswapbyte *out;
} doubleUnion;
#endif

/*------------------------------------------------------------------------------
|
|	jPipeTransfer/3
|
|	This function write one variable to pipe
|       write the following:
|       name len
|	name
|	type
|	units
|	number of values
|	value(s)
|
+-----------------------------------------------------------------------------*/

static void jPipeTransfer(char *name, varInfo *v, vInfo *vI, int fd)
{   
    register int   i;
    int   size;
    register Rval *r;
    int   longSwap;
    short int shortSwap;
#ifdef LINUX
    doubleUnion swap;
#endif
    double dtmp;

    size = strlen(name);
    longSwap = htonl(size);
    sigprocmask( SIG_BLOCK, &blockMask, &saveMask );
    write(fd,&longSwap,sizeof(int));
    write(fd,name,size);
    /* write(fd,vI,sizeof(vInfo));  not for Jpsg but the following 3 items instead */
    shortSwap = htons(vI->subtype);
    write(fd,&shortSwap,sizeof(short));
    shortSwap = htons(vI->basicType);
    write(fd,&shortSwap,sizeof(short));
    shortSwap = htons(v->T.size);
    write(fd,&shortSwap,sizeof(short));

    /*  write out good stuff */
    if (v->T.basicType == T_STRING)  /* send string values */
    {	i = 1; 
     	r = v->R;
	while (r && i < v->T.size+1)
	{   size = strlen(r->v.s);
            longSwap = htonl(size);
	    write(fd,&longSwap,sizeof(int));
	    write(fd,r->v.s,size);
	    i += 1;
	    r = r->next;
	}
    }
    else  /* copy over all reals */
    {	i = 1; 
     	r = v->R;
	while (r && i < v->T.size+1)
	{
            int tmp;
            dtmp = r->v.r;
#ifdef LINUX
            swap.in1 = &dtmp;
            tmp = htonl(swap.out->s1);
            swap.out->s1 = htonl(swap.out->s2);
            swap.out->s2 = tmp;
#endif
            write(fd,&dtmp,sizeof(double));
	    i += 1;
	    r = r->next;
	}
    }
    sigprocmask( SIG_SETMASK, &saveMask, NULL );
}

