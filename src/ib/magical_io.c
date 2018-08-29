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
|	io.c
|
|	These procedures and functions are used for the loading of magic
|	source code (macros).  Notice that commands come from three different
|	sources: terminal input (a buffer filled by the notifier), macro
|	files (not notifier driven), or strings.  This is noted by the
|	"magicFromFile" and "magicFromString" flags.
|	When "magicFromFile" is set, the
|	characters are comming from a macro file.  When unset input either
|	comes from a string (magicFromString is set) or from the keyboard.  The
|	lexer routines "input" and "unput" uses these to determine what is
|	going on.
|
+----------------------------------------------------------------------------*/

#define MAXNAME 1024

#include <stdio.h>

/*#include "vnmrsys.h"*/
#include "group.h"
#include "node.h"
#include "magic.gram.h"
#include "magical_io.h"

static char *systemdir = ".";
static char *userdir = ".";
static int   Dflag = 0;
static int   Lflag = 0;

char magicFileName[MAXNAME];
char **magicMacroDirList = NULL;

extern char *newTempName();
extern char *tempID;			/* temp ID pointers */
extern FILE *fopen();
extern node *newNode();
/*extern char  magicMacroBuffer[];
extern char *magicMacroBp;*/
FILE        *macroFile;
int          columnNumber = 0;
int          lineNumber   = 0;

/*------------------------------------------------------------------------------
|
|	init_input/2
|
|	This procedure is used to initialize input from a new file.  All file
|	opening is done here, line numbers and file names are initialized and
|	tucked away here also.  Init_input has an option to open a direct 
|       macro file or search the places where macros might live.
|	Notice that the macro character buffer is flushed here.
|       Remember that nesting is not really required!
|
|       When searching for a macro, we first search the userdir maclib 
|	directory, then the maclibpath directory, then the sysmaclibpath
|	directory, and finally the systemdir maclib directory.
|
+-----------------------------------------------------------------------------*/
int
init_input(n,search)
  char *n;
  int search;
{
    char **pdir;

   magicMacroBp = magicMacroBuffer;
   magicMacroBuffer[0] = '\0';

   if (n){
      if (search){
	  for (pdir=magicMacroDirList; *pdir; pdir++){
	      if (macroOpen(n, *pdir, 0)){
		  return 1;
	      }
	  }
	  return 0;
      }else{
	 /* just try to open a single file */
         if (macroOpen(n,NULL,0))
            return(1);
         else
	    return(0);
      }
   }else{
      /* We will get input from the keyboard instead of a file */
      lineNumber   = 0;
      columnNumber = 0;
      magicFileName[0]  = '\0';
      magicFromFile     = 0;
      magicFromString   = 0;
      return(0);
   }
}

/*-----------------------------------------------------------------------------
|
| 	macroOpen/3
|
|	This routine attemps to open macro files, it is given a path
|	and a file name.  It returns 1 if successful, 0 if not.
|
/+---------------------------------------------------------------------------*/

macroOpen(n,path,maclib)	char *n,*path;  int maclib;
{  if (path) /* if we have a path */
   {  strncpy(magicFileName,path,MAXNAME);
      strncat(magicFileName,"/",MAXNAME - strlen(magicFileName) -1);
      if (maclib)
      {
	 strncat(magicFileName,"maclib",MAXNAME - strlen(magicFileName) -1);
	 strncat(magicFileName,"/",MAXNAME - strlen(magicFileName) -1);
      }
   }
   else
      magicFileName[0] = NULL;
   strncat(magicFileName,n,MAXNAME - strlen(magicFileName) -1);
/**   strncat(magicFileName,".m",MAXNAME - strlen(magicFileName) -1); **/
   if (Lflag)
      fprintf(stderr,"init_input: try to open \"%s\"\n",magicFileName);
   if (magicMacroFile = fopen(magicFileName,"r"))
   {  if (Lflag)
	 fprintf(stderr,"init_input: ...open!\n");
      lineNumber   = 1;
      columnNumber = 1;
      magicFromFile     = 1;
      magicFromString   = 0;
      return(1);
   }
   else
   {  if (Lflag)
	 fprintf(stderr,"init_input: ...Can't open!\n");
      lineNumber   = 0;
      columnNumber = 0;
      magicFileName[0]  = '\0';
      magicFromFile     = 0;
      magicFromString   = 0;
      return(0);
   }
}

/*------------------------------------------------------------------------------
|
|	cleanUp/1
|
|	This procedure is used to close-up input from a given source file.
|
+-----------------------------------------------------------------------------*/

cleanUp(n)				char *n;
{  if (2 <= Lflag)
      if (n)
	 fprintf(stderr,"cleanUp: All done, close \"%s\"\n",n);
      else
	 fprintf(stderr,"cleanUp: All done, ( don't need to close)\n");
   if (n)
      fclose(magicMacroFile);
   columnNumber = 0;
   lineNumber   = 0;
   magicFileName[0]  = '\0';
}

/*------------------------------------------------------------------------------
|
|	load/2
|
|	This function is called to load source code from a given file and
|	generate a code tree.  A pointer to the resulting code tree is
|	returned.  Load can be selected to load a specific file, or
|	search a set of directories.
|
+----------------------------------------------------------------------------*/

node *load(n,search)				char *n;  int search;
{   char *prevTempID;
    node *finalTree;
    node *p;

    if (3 <= Dflag)
	fprintf(stderr,"load: macro named \"%s\"...\n",n);
    if (init_input(n,search))
    {	finalTree = NULL;
	if (3 <= Dflag)
	    fprintf(stderr,"load: ...initialization of file %s is complete\n"
		   ,magicFileName
		   );
	prevTempID = tempID; tempID = newTempName("tmpLoadID");
	while (1)
	{   magicCodeTree   = NULL;
	    if (3 <= Dflag)
		fprintf(stderr,"load: ...off to the parser!\n");
	    if (magic_yyparse())
	    {   if (3 <= Dflag)
		    fprintf(stderr,"load: ...parser bombed!\n");
		finalTree = NULL;
		releaseWithId(tempID);
		break;
	    }
	    else
	    {   renameAllocation(tempID,"newMacro");
	        if (3 <= Dflag)
		{   fprintf(stderr,"load: ...parser done\n");
		    if (magicCodeTree)
		    {   fprintf(stderr,"load: ...got...\n");
			showTree(0,"load:    ",magicCodeTree);
		    }
		}
		if (magicCodeTree && magicCodeTree->flavour != ENDFILE)
		{   if (magicCodeTree->flavour != EOL)
			if (finalTree)
			{   if (3 <= Dflag)
				fprintf(stderr,"load: ...tie it in\n");
			    p = newNode(CM,&(magicCodeTree->location));
			    addLeftSon(p,magicCodeTree);
			    addLeftSon(p,finalTree);
			    finalTree = p;
			}
			else
			{   if (3 <= Dflag)
				fprintf(stderr,"load: ...first one\n");
			    finalTree = magicCodeTree;
			}
		    else
			if (3 <= Dflag)
			    fprintf(stderr,"load: ...just noise\n");
		}
		else
		{   if (3 <= Dflag)
		    {   fprintf(stderr,"load: ...have...\n");
			showTree(0,"load:    ",finalTree);
		    }
		    break;
		}
	    }
	}
	if (3 < Dflag)
	   fprintf(stderr,"load: time to clean-up!\n");
	free(tempID); tempID = prevTempID;
	cleanUp(n);
	return(finalTree);
    }
    else
   {
           Werrprintf("Command or macro \"%s\" does not exist",n);
           /*   Werrprintf("Can't load \"%s\"",n); */
	   return(NULL);
   }
}
 

/*------------------------------------------------------------------------------
|
|	yywrap/0
|
|	This function is used by the lexer on end of file.  Always returns 1.
|
+-----------------------------------------------------------------------------*/

int yywrapbk()
{   return(1);
}
