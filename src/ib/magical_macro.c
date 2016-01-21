/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* 
*/
/*-----------------------------------------------------------------------------
|
|	macro.c
|
|	This modules contain code to store and manipulate 
|	macros.  Once loaded, the system keeps the macro 
|	in memory on a macro table to speed up re-execution 
|	of the macro.  This module also contains the following 
|	commands. 
|
|	macrodir    - list of macros in userdir directory 
|	macrosysdir - list of macros in systemdir directory
|	macrocat    - cat contents of macro in userdir 
|	macrosyscat - cat contents of macro in systemdir 
|	macrorm     - remove a macro from userdir 
|	macrosysrm  - remove a macro from systemdir 
|	macrocp     - copy macro in userdir 
|	macrosyscp  - cp macro from systemdir to userdir 
|	macrold     - load a macro from userdir 
|	macrosysld  - load a macro from systemdir 
|
|   01/08/91   RL   Allocated memory tagged with ID "newMacro" was released
|		    in several places, mostly under error conditions.  This
|		    no longer occurs here, but is deferred to loadAndExec
|		    in lexjunk.c, where it is released at the conclusion of
|		    each user-entered command.
|
|		    The saveMacro routine has a 3rd argument to tell it
|		    where to save the macro, in the permenenet cache or
|		    in a cache that is deleted when the current keyboard
|		    command completes.
+----------------------------------------------------------------------------*/

#define  MAXSHSTRING	1024
#define  TOOL		"vxrTool"

#include "node.h"
#include "symtab.h"
#include <stdio.h>
#include "vnmrsys.h"
#include <errno.h>

#ifdef UNIX
#include <unistd.h>
#include <sys/file.h>
#else
#define F_OK	0
#define W_OK	2
#define R_OK	4
#endif

extern node   *load();
extern symbol *BaddName();
extern symbol *findName();
extern FILE   *popen_call();
static FILE   *stream;
static symbol *macroCache = NULL;
static symbol *tempCache = NULL;

#ifdef  DEBUG
extern int     Dflag;
extern int     Tflag;
#define DPRINT0(level, str) \
	if (Dflag >= level) fprintf(stderr,str)
#define DPRINT1(level, str, arg1) \
	if (Dflag >= level) fprintf(stderr,str,arg1)
#define DSHOWTREE(level, arg1, arg2, arg3) \
	if (Dflag >= level) showTree(arg1,arg2,arg3)
#define TPRINT1(str, arg1) \
	if (Tflag) fprintf(stderr,str,arg1)
#else   DEBUG
#define DPRINT0(level, str) 
#define DPRINT1(level, str, arg1) 
#define DSHOWTREE(level, arg1, arg2, arg3)
#define TPRINT1(str, arg1) 
#endif  DEBUG


/*-----------------------------------------------------------------------------
|
|	saveMacro/2
|
|	This procedure is used to save a macro (having been translated into
|	a code tree) under a given name.  Note that the "newNode"'s allocated
|	are aged into "oldNode"'s.
|
+----------------------------------------------------------------------------*/
void
saveMacro(n,p,i)
  char *n;
  node *p;
  int i;
{
    symbol *q;
    symbol **b;

    DPRINT1(3,"saveMacro: named \"%s\"...\n",n);
    DSHOWTREE(3,0,"saveMacro:    ",p);
    if (i == PERMANENT){
	b = &macroCache;
    }else{
	b = &tempCache;
    }
    if (q=findName(*b,n)){
	if (q->val){
	    TPRINT1("magic: macro \"%s\" already defined\n",n);
	}else{
	    q->val = (char *)p;
	    if (i == PERMANENT){
		renameAllocation("newMacro",q->name);
	    }
	}
    }else{
	q      = BaddName(b,n);
	q->val = (char *)p;
	if (i == PERMANENT){
	    renameAllocation("newMacro",q->name);
	}
    }
}

/*-----------------------------------------------------------------------------
|
|	findMacro/1
|
|	This function returns a pointer to a code tree given a macro name.
|	If the given macro name can't be found, NULL is returned.
|
+-----------------------------------------------------------------------------*/
node *
findMacro(n)			char *n;
{   symbol *p;

    DPRINT1(3,"findMacro: named \"%s\"...",n);
    p=findName(macroCache,n);
    if (p == NULL)
      p = findName(tempCache,n);

    if (p != NULL)
    {
	DPRINT0(3," found!\n");
	return((node *)(p->val));
    }
    else
    {
	DPRINT0(3," NOT found!\n");
	return(NULL);
    }
}

/*------------------------------------------------------------------------------
|
|	rmMacro/1
|
|	This procedure is used to "remove" a macro.  Note that the name is
|	not really removed from the table, but the attached code is disposed
|	and the "val" pointer is NULL'd out (thus findMacro/1 will return NULL
|	just asthough the macro isn't there).  SaveMacro/2 may then be used to
|	"redefine" the "noexistant" macro if desired.
|
+-----------------------------------------------------------------------------*/
void
rmMacro(n)				char *n;
{  symbol *p;

   if (p=findName(macroCache,n))
   {  dispose(p->val);
      p->val = NULL;
   }
}

/*-----------------------------------------------------------------------------
|
|	purgeAllMacros/0
|	purgeMacro/1
|
|	This procedure releases the storage tied up in the macro cache.
|
+----------------------------------------------------------------------------*/
void
purgeOneMacro(n)
char *n;
{  symbol *p;

   if (p=findName(macroCache,n))
   {
      dispose(p->val);
      p->val = NULL;
      releaseAllWithId(n);
      delNameWithBalance(&macroCache,n);
   }
}

static void
purgeMacro(pp,i)
symbol **pp;
int i;
{  register symbol *p;

   if (p=(*pp))
   {  purgeMacro(&(p->left),i);
      purgeMacro(&(p->right),i);
      dispose(p->val);
      if (i == PERMANENT)
         releaseAllWithId(p->name);
      release(p->name);
      release(p);
      *pp = NULL;
   }
}

void
purgeAllMacros()
{
   purgeMacro(&macroCache,PERMANENT);
}

void
releaseTempCache()
{
   purgeMacro(&tempCache,EXTENT_OF_CMD);
}

/*-----------------------------------------------------------------------------
|
|	showMacros/0
|	showMacro/1
|
|	These procedures are used list all macros (for debugging purposes).
|
+----------------------------------------------------------------------------*/
static
void showMacro(p)		register symbol *p;
{  if (p)
   {  showMacro(p->left);
      fprintf(stderr,"%s\n\n",p->name);
      showTree(4,NULL,p->val);
      showMacro(p->right);
   }
}

void
showMacros()
{  showMacro(macroCache);
}

/*------------------------------------------------------------------------------
|
|	purgeCache
|
|	This command is used to purge the macro cache.
|
+-----------------------------------------------------------------------------*/
int
purgeCache(argc,argv,retc,retv)	int argc,retc; char *argv[],*retv[];
{  if (argc == 1)
   {  purgeAllMacros();
      ABORT;
   }
   else if (argc == 2)
   {
      if (findName(macroCache,argv[1]))
      {
         purgeOneMacro(argv[1]);
         RETURN;
      }
   }
   RETURN;
}

/*--------------------------------------------------------------------------
|
|	loadMacro
|
|	This command loads a macro into cache memory.  If the macro
|	of the same name already exists in the cache memory, it is first 
|	deleted before the new macro is loaded.
|
/+--------------------------------------------------------------------------*/
int
loadMacro(argc,argv,retc,retv)	int argc,retc; char *argv[],*retv[];
{
    int   i;
    node *codeTree;

    if (argc == 1)
    {	Werrprintf("Usage -- %s('macro1'[,'macro2',...])",argv[0]);
	ABORT;
    }
    else
    {	for (i=1;i<argc;i++)
	{
	    TPRINT1("loadMacro: removing macro \"%s\"\n",argv[i]);
	    rmMacro(argv[i]);
	    renameAllocation("newMacro","tmpSavenewMacro");
            if (argv[i][0] == '/')
            {
               char *s;
               extern char *strrchr();
               
               s = strrchr(argv[i],'/');
               rmMacro(s+1);
	       if (codeTree = load(argv[i],NOSEARCH))
	       {
		   TPRINT1("loadMacro: saving macro \"%s\"\n",s+1);
		   saveMacro(s+1,codeTree,PERMANENT);
                   if (!retc)
		      Winfoprintf("Loaded macro '%s'",s+1);
	       }
            }
            else if (codeTree = load(argv[i],SEARCH))
	    {
		TPRINT1("loadMacro: saving macro \"%s\"\n",argv[i]);
		saveMacro(argv[i],codeTree,PERMANENT);
                if (!retc)
		   Winfoprintf("Loaded macro '%s'",argv[i]);
	    }
            releaseAllWithId("newMacro");
	    renameAllocation("tmpSavenewMacro","newMacro");
	}
    }
    RETURN;
}
