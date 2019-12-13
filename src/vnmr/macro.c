/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
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
#include <stdlib.h>
#include <string.h>
#include "vnmrsys.h"
#include "allocate.h"
#include "tools.h"
#include "wjunk.h"

#ifdef UNIX
#include <unistd.h>
#include <sys/file.h>
#else 
#define F_OK	0
#define W_OK	2
#define R_OK	4
#endif 

extern node   *loadMacro(char *n, int search, int *res);
extern symbol *BaddName(symbol **pp, const char *n);
extern symbol *findName(symbol *p, const char *n);
extern FILE   *popen_call(char *cmdstr, char *mode);
extern int More(FILE *stream, int screenLength);
extern void showTree(int n, char *m, node *p);
extern void dispose(node *p);

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
#else 
#define DPRINT0(level, str) 
#define DPRINT1(level, str, arg1) 
#define DSHOWTREE(level, arg1, arg2, arg3)
#define TPRINT1(str, arg1) 
#endif 


/*-----------------------------------------------------------------------------
|
|	saveMacro/2
|
|	This procedure is used to save a macro (having been translated into
|	a code tree) under a given name.  Note that the "newNode"'s allocated
|	are aged into "oldNode"'s.
|
+----------------------------------------------------------------------------*/

void saveMacro(char *n, node *p, int i)
{
   symbol *q;
   symbol **b;

   DPRINT1(3,"saveMacro: named \"%s\"...\n",n);
   DSHOWTREE(3,0,"saveMacro:    ",p);
   if (i == PERMANENT)
     b = &macroCache;
   else
     b = &tempCache;
   if ( (q=findName(*b,n)) )
     if (q->val) {
         TPRINT1("magic: macro \"%s\" already defined\n",n);
     }
     else {
         q->val = (char *)p;
	 if (i == PERMANENT)
	   renameAllocation("newMacro",q->name);
     }
   else {
      q      = BaddName(b,n);
      q->val = (char *)p;
      if (i == PERMANENT)
	renameAllocation("newMacro",q->name);
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

node *findMacro(char *n)
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

void rmMacro(char *n)
{  symbol *p;

   if ( (p=findName(macroCache,n)) )
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

void purgeOneMacro(char *n)
{  symbol *p;

   if ( (p=findName(macroCache,n)) )
   {
      dispose(p->val);
      p->val = NULL;
      releaseAllWithId(n);
      delNameWithBalance(&macroCache,n);
   }
}

static void purgeMacro(symbol **pp, int i)
{  register symbol *p;

   if ( (p=(*pp)) )
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

void purgeAllMacros()
{
   purgeMacro(&macroCache,PERMANENT);
}

void releaseTempCache()
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

static void showMacro(register symbol *p)
{  if (p)
   {  showMacro(p->left);
      fprintf(stderr,"%s\n\n",p->name);
      showTree(4,NULL,p->val);
      showMacro(p->right);
   }
}

void showMacros()
{  showMacro(macroCache);
}


/*------------------------------------------------------------------------------
|
|	purgeCache
|
|	This command is used to purge the macro cache.
|
+-----------------------------------------------------------------------------*/

int purgeCache(int argc, char *argv[], int retc, char *retv[])
{  if (argc == 1)
   {
      // Schedule purge to run after all macros exit.
      if ( ! Bnmr )
         sendTripleEscToMaster( 'C',"purge(0,0,0,0)");
   }
   else if (argc == 5)
   {  purgeAllMacros();
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
|	macrodir  macrosysdir
|
|	This command lists the directory of macros in the userdir and
|	systemdir paths.
|
|	VMS version uses the DIR command, along with Files-11 syntax.
|
/+--------------------------------------------------------------------------*/
int macroDir(int argc, char *argv[], int retc, char *retv[])
{   char cmdstr[MAXSHSTRING];   

#ifdef UNIX
    strcpy(cmdstr,"/bin/ls -C -F ");
    if ( ! strcmp("macrodir",argv[0]))  /* userdir  listing ?*/
	strncat(cmdstr,userdir,MAXSHSTRING - strlen(cmdstr) -1);
    else
	strncat(cmdstr,systemdir,MAXSHSTRING - strlen(cmdstr) -1);
    strncat(cmdstr,"/maclib",MAXSHSTRING - strlen(cmdstr) -1); 
#else 
    strcpy(cmdstr,"dir ");
    if (strcmp("macrodir",argv[0])==NULL)
	strcat(cmdstr,userdir);
    else
	strcat(cmdstr,systemdir);
    vms_fname_cat(cmdstr,"[.maclib]*.");
#endif 
    TPRINT1("macroDir: command string \"%s\"\n",cmdstr);
    Wsettextdisplay(argv[0]);
    Wshow_text();
    /* Startup the shell command and pipe in the output */
    if ((stream = popen_call(cmdstr,"r"))  == NULL)
    {  Werrprintf("Problem with creating macrols command with popen");
       ABORT;
    }
    More(stream,WscreenSize());  /* more it out to screen */
    RETURN;
}

/*--------------------------------------------------------------------------
|	macrocat  macrosyscat
|
|	This command lists the contents of macros in the userdir and
|	systemdir paths.
|
|       VMS version:  Command is "type <file1>,<file2>..." , with a
|	comma between each file name.  Better way needed to display
|	large macros or multiple macros
|
/+--------------------------------------------------------------------------*/
int macroCat(int argc, char *argv[], int retc, char *retv[])
{   char cmdstr[MAXSHSTRING];   
    char dirname[MAXSHSTRING];   
    int  i;
    int  found = 0;

    if (argc == 1)
    {	Werrprintf("Usage -- %s('macro_name'[,'macro_name'....])",argv[0]);
	ABORT;
    }
#ifdef UNIX
    strcpy(cmdstr,"/bin/cat");
#else 
    strcpy(cmdstr,"typ ");
#endif 
    if ( ! strcmp("macrocat",argv[0]))  /* userdir  directory ?*/
       strcpy(dirname,userdir);
    else
       strcpy(dirname,systemdir);
#ifdef UNIX
    strcat(dirname,"/maclib/");
#else 
    vms_fname_cat(dirname,"[.maclib]");
#endif 
    for (i=1; i<argc; i++)
    {
        char macroname[MAXSHSTRING];

        strcpy(macroname,dirname);
        strcat(macroname,argv[i]);
        if (access(macroname,F_OK|R_OK) == -1)
        {  Werrprintf("Macro %s is not accessible",macroname);
        }
        else
        {
           found = 1;
           strncat(cmdstr," ",MAXSHSTRING - strlen(cmdstr) -1);
	   strncat(cmdstr,macroname,MAXSHSTRING - strlen(cmdstr) -1);
#ifdef VMS
	   if (i < argc-1) strcat(cmdstr,"., ");
	   else            strcat(cmdstr,".");
#endif 
        }
    }
    TPRINT1("macroCat: command string \"%s\"\n",cmdstr);
    if (found)
    {
       Wsettextdisplay(argv[0]);
       Wshow_text();
       /* Startup the shell command and pipe in the output */
       if ((stream = popen_call(cmdstr,"r"))  == NULL)
       {  Werrprintf("Problem with creating macrocat command with popen");
          ABORT;
       }
       More(stream,WscreenSize());  /* more it out to screen */
    }
    RETURN;
}

/*--------------------------------------------------------------------------
|	macrorm  macrosysrm
|
|	These commands remove macros from userdir and systemdir.
|
/+--------------------------------------------------------------------------*/
int macroRm(int argc, char *argv[], int retc, char *retv[])
{   char path[MAXPATHL];   
    int  i;

    if (argc == 1)
    {	Werrprintf("Usage -- %s('macro_name'[,'macro_name'....])",argv[0]);
	ABORT;
    }
    for (i=1; i<argc; i++)
    {	path[0] = '\0';
    	if ( ! strcmp("macrorm",argv[0]) )  /* remove from userdir ?*/
	    strncat(path,userdir,MAXPATHL - strlen(path) -1);
	else  /* remove from systemdir */
	    strncat(path,systemdir,MAXPATHL - strlen(path) -1);
#ifdef UNIX
	strncat(path,"/maclib/",MAXPATHL - strlen(path) -1);
#else 
	vms_fname_cat(path,"[.maclib]");
#endif 
     	strncat(path,argv[i],MAXPATHL - strlen(path) -1);
	TPRINT1("macroRm: unlinking \"%s\"\n",path);
	if (unlink(path)) 
	{   Werrprintf("Problem removing macro '%s'",argv[i]);
	    ABORT;
	}
    }
    RETURN;
}

/*--------------------------------------------------------------------------
|	macrocp macrosyscp 
|
|	These commands copy macros from userdir to userdir or systemdir to
|	userdir
|
/+--------------------------------------------------------------------------*/
int macroCp(int argc, char *argv[], int retc, char *retv[])
{   char  cmdstr[MAXSHSTRING];   

    if (argc < 3)
    {	Werrprintf("Usage -- %s('macro_name','macro_name')",argv[0]);
	ABORT;
    }
#ifdef UNIX
    strcpy(cmdstr,"/bin/cp ");
#else 
    strcpy(cmdstr,"copy ");
#endif 
    /*  from file */
    if ( ! strcmp("macrocp",argv[0]) )  /* copy from userdir ?*/
	strncat(cmdstr,userdir,MAXSHSTRING - strlen(cmdstr) -1);
    else  /* edit from systemdir */
	strncat(cmdstr,systemdir,MAXSHSTRING - strlen(cmdstr) -1);
#ifdef UNIX
    strncat(cmdstr,"/maclib/",MAXSHSTRING - strlen(cmdstr) -1);
#else 
    vms_fname_cat(cmdstr,"[.maclib]");
#endif 
    strncat(cmdstr,argv[1],MAXSHSTRING - strlen(cmdstr) -1);
    /**strncat(cmdstr,".m ",MAXSHSTRING - strlen(cmdstr) -1); **/
    strncat(cmdstr," ",MAXSHSTRING - strlen(cmdstr) -1); 
    /*  to file */
    strncat(cmdstr,userdir,MAXSHSTRING - strlen(cmdstr) -1);
#ifdef UNIX
    strncat(cmdstr,"/maclib/",MAXSHSTRING - strlen(cmdstr) -1);
#else 
    vms_fname_cat(cmdstr,"[.maclib]");
#endif 
    strncat(cmdstr,argv[2],MAXSHSTRING - strlen(cmdstr) -1);
    TPRINT1("macroCp: command string \"%s\"\n",cmdstr);
    system(cmdstr);
    RETURN;
}

/*--------------------------------------------------------------------------
|
|	macroLd
|
|	This command loads a macro into cache memory.  If the macro
|	of the same name already exists in the cache memory, it is first 
|	deleted before the new macro is loaded.
|
/+--------------------------------------------------------------------------*/
int macroLd(int argc, char *argv[], int retc, char *retv[])
{
    int   i;
    node *codeTree;
    int   res;
    int ret = 0;

    if (argc == 1)
    {	Werrprintf("Usage -- %s('macro1'[,'macro2',...])",argv[0]);
	ABORT;
    }
    else
    {
        if (retc > 1)
           WstoreMessageOn();
        for (i=1;i<argc;i++)
	{
	    TPRINT1("macroLd: removing macro \"%s\"\n",argv[i]);
	    rmMacro(argv[i]);
	    renameAllocation("newMacro","tmpSavenewMacro");
#ifndef __CYGWIN__
            if   (argv[i][0] == '/')
#else
            if ( (argv[i][0] == '/') ||
                 ((argv[i][1] == ':') && (argv[i][2] == '/')) )
#endif
            {
               char *s;
               
               s = strrchr(argv[i],'/');
               rmMacro(s+1);
	       if ( (codeTree = loadMacro(argv[i],NOSEARCH, &res)) )
	       {
		   TPRINT1("macroLd: saving macro \"%s\"\n",s+1);
		   saveMacro(s+1,codeTree,PERMANENT);
                   ret = 1;
                   if (!retc)
		      Winfoprintf("Loaded macro '%s'",s+1);
                   else if (retc > 1)
                      sprintf(storeMessage,"Loaded macro '%s'",s+1);
	       }
            }
            else if ( (codeTree = loadMacro(argv[i],SEARCH, &res)) )
	    {
		TPRINT1("macroLd: saving macro \"%s\"\n",argv[i]);
		saveMacro(argv[i],codeTree,PERMANENT);
                ret = 1;
                if (!retc)
		   Winfoprintf("Loaded macro '%s'",argv[i]);
                else if (retc > 1)
                   sprintf(storeMessage,"Loaded macro '%s'",argv[i]);
	    }
            releaseAllWithId("newMacro");
	    renameAllocation("tmpSavenewMacro","newMacro");
	}
        if (retc)
        {
           retv[0] = intString( ret );
           if (retc > 1)
           {
              retv[1] = newString(storeMessage);
              WstoreMessageOff();
           }
        }
    }
    RETURN;
}

int macroLoad(char *name, char *filepath)
{
  node *codeTree;
  int res;

  rmMacro(name);
  renameAllocation("newMacro","tmpSavenewMacro");
  if ( (codeTree = loadMacro(filepath,NOSEARCH, &res)) )
  {
     saveMacro(name,codeTree,PERMANENT);
     releaseAllWithId("newMacro");
     renameAllocation("tmpSavenewMacro","newMacro");
     return(0);
  }
  releaseAllWithId("newMacro");
  renameAllocation("tmpSavenewMacro","newMacro");
  return(-1);
}
