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
|	io.c
|
|	These procedures and functions are used for the loading of magic
|	source code (macros).  Notice that commands come from three different
|	sources: terminal input (a buffer filled by the notifier), macro
|	files (not notifier driven), or strings.  This is noted by the
|	"fromFile" and "fromString" flags.  When "fromFile" is set, the
|	characters are comming from a macro file.  When unset input either
|	comes from a string (fromString is set) or from the keyboard.  The
|	lexer routines "input" and "unput" uses these to determine what is
|	going on.
|
+----------------------------------------------------------------------------*/

#include "vnmrsys.h"
#include "group.h"
#include "node.h"
#ifdef UNIX
#include "magic.gram.h"
#else 
#include "magic_gram.h"
#endif 
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "allocate.h"
#include "wjunk.h"

char         fileName[MAXPATH];
extern char *newTempName();
extern char *tempID;			/* temp ID pointers */
extern int   Dflag;
extern int   Lflag;
extern node *newNode();
extern char  macroBuffer[];
extern char *macroBp;
FILE        *macroFile;
int          columnNumber = 0;
int          lineNumber   = 0;
int          fromFile     = 0;
int          fromString   = 0;
node        *codeTree;

extern int appdirFind(char *filename, char *lib, char *fullpath,
                      char *suffix, int perm);
extern int yyparse();
extern void showTree(int n, char *m, node *p);
extern int p11_saveUserMacro(char* name);

static int macroOpen(char *fullpath);

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
|       When searching for a macro, we search the appdir directories.
|
+-----------------------------------------------------------------------------*/

int init_input(char *n, int search)
{
   char fullpath[MAXPATH];

   macroBp        = macroBuffer;
   macroBuffer[0] = '\0';

   if (n)
      if (search)  
      {
         if ( appdirFind(n, "maclib", fullpath, "", R_OK) )
            if (macroOpen(fullpath))
	       return(1);
         return(0);
      }
      else  /* just try to open a single file */
      {  if (macroOpen(n))
            return(1);
         else
	    return(0);
      }
   else /* We will get input from the keyboard instead of a file */
   {  lineNumber   = 0;
      columnNumber = 0;
      fileName[0]  = '\0';
      fromFile     = 0;
      fromString   = 0;
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

static int macroOpen(char *fullpath)
{
   fileName[0] = '\0';
   strncat(fileName, fullpath ,MAXPATH - 1);
/**   strncat(fileName,".m",MAXPATH - strlen(fileName) -1); **/
   if (Lflag)
      fprintf(stderr,"init_input: try to open \"%s\"\n",fileName);
   if ( (macroFile = fopen(fileName,"r")) )
   {  if (Lflag)
	 fprintf(stderr,"init_input: ...open!\n");
      else
	 p11_saveUserMacro(fileName);
      lineNumber   = 1;
      columnNumber = 1;
      fromFile     = 1;
      fromString   = 0;


      return(1);
   }
   else
   {  if (Lflag)
	 fprintf(stderr,"init_input: ...Can't open!\n");
      lineNumber   = 0;
      columnNumber = 0;
      fileName[0]  = '\0';
      fromFile     = 0;
      fromString   = 0;
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

void cleanUp(char *n)
{  if (2 <= Lflag)
   {
      if (n)
	 fprintf(stderr,"cleanUp: All done, close \"%s\"\n",n);
      else
	 fprintf(stderr,"cleanUp: All done, ( don't need to close)\n");
   }
   if (n)
      fclose(macroFile);
   columnNumber = 0;
   lineNumber   = 0;
   fileName[0]  = '\0';
}

/*------------------------------------------------------------------------------
|
|	loadMacro/3
|
|	This function is called to load source code from a given file and
|	generate a code tree.  A pointer to the resulting code tree is
|	returned.  Load can be selected to load a specific file, or
|	search a set of directories.
|
+----------------------------------------------------------------------------*/

node *loadMacro(char *n, int search, int *res)
{   char *prevTempID;
    node *finalTree;
    node *p;

    *res = 0;
    if (3 <= Dflag)
	fprintf(stderr,"load: macro named \"%s\"...\n",n);
    if (init_input(n,search))
    {	finalTree = NULL;
	if (3 <= Dflag)
	    fprintf(stderr,"load: ...initialization of file %s is complete\n"
		   ,fileName
		   );
	prevTempID = tempID; tempID = newTempName("tmpLoadID");
	while (1)
	{   codeTree   = NULL;
	    if (3 <= Dflag)
		fprintf(stderr,"load: ...off to the parser!\n");
	    if (yyparse())
	    {   if (3 <= Dflag)
		    fprintf(stderr,"load: ...parser bombed!\n");
		finalTree = NULL;
                *res = 1;
		releaseWithId(tempID);
		break;
	    }
	    else
	    {   renameAllocation(tempID,"newMacro");
	        if (3 <= Dflag)
		{   fprintf(stderr,"load: ...parser done\n");
		    if (codeTree)
		    {   fprintf(stderr,"load: ...got...\n");
			showTree(0,"load:    ",codeTree);
		    }
		}
		if (codeTree && codeTree->flavour != ENDFILE)
		{   if (codeTree->flavour != EOL)
			if (finalTree)
			{   if (3 <= Dflag)
				fprintf(stderr,"load: ...tie it in\n");
			    p = newNode(CM,&(codeTree->location));
			    addLeftSon(p,codeTree);
			    addLeftSon(p,finalTree);
			    finalTree = p;
			}
			else
			{   if (3 <= Dflag)
				fprintf(stderr,"load: ...first one\n");
			    finalTree = codeTree;
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
           *res = 1;
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
