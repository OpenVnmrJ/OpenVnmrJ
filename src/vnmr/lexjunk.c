/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*-------------------------------------------------------------------------
|
|	lexjunk.c
|
|	This file contains some of the code used by the lexer and
|	parser. It also contains various other junk such as 
|	autoRedisplay, execString and loadAndExec.
|
+---------------------------------------------------------------------------*/

#include "vnmrsys.h"
#include "node.h"
#include "variables.h"
#ifdef UNIX
#include "magic.gram.h"
#else 
#include "magic_gram.h"
#endif 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "allocate.h"
#include "tools.h"
#include "wjunk.h"

extern char   *Dx_string;
extern char   *Jx_string;
extern char   *newTempName();
extern char   *tempID;
extern char   *Wgettextdisplay();
extern int     Bnmr;
extern int     buttons_active;
extern int     menuflag,menuon;
extern int     fromFile;
extern int     fromString;
extern int     working;
extern int     interuption;
extern node   *codeTree;

extern char *execN(node *n, int *shouldFree, char *macroName);
extern int  execS(node *n, char *macroName);
extern int  execR(node *n, pair *p, char *macroName);
extern int  isTrue(pair *p);
extern void cleanPair(pair *p);
extern void Wclearerr();
extern void clearMustReturn();
extern void releaseTempCache();
extern void resetjEvalGlo();
extern void releaseJvarlist();
extern void jAutoRedisplayList();
extern int  sendTripleEscToMaster(char code, char *string_to_send );
extern int  isReexecCmd(char *sName);
extern void showFlavour(FILE *f, node *p);
extern void showTree(int n, char *m, node *p);
extern int  yyparse();
extern int aspFrame(char *keyword, int frameID, int x, int y, int w, int h);

int            ignoreEOL = 0;
static const char   *bp;
static int     oldColumnNumber;

#ifdef  DEBUG
extern int     Lflag;
extern int     Tflag;
#define LPRINT0(level, str) \
	if (Lflag >= level) fprintf(stderr,str)
#define LPRINT1(level, str, arg1) \
	if (Lflag >= level) fprintf(stderr,str,arg1)
#define LPRINT2(level, str, arg1, arg2) \
	if (Lflag >= level) fprintf(stderr,str,arg1,arg2)
#define TPRINT1(str, arg1) \
	if (Tflag) fprintf(stderr,str,arg1)
#define TPRINT3(str, arg1, arg2, arg3) \
	if (Tflag) fprintf(stderr,str,arg1,arg2,arg3)
#else 
#define LPRINT0(level, str) 
#define LPRINT1(level, str, arg1) 
#define LPRINT2(level, str, arg1, arg2) 
#define TPRINT1(str, arg1) 
#define TPRINT3(str, arg1, arg2, arg3)
#endif 

#ifdef VNMRJ
extern void setAutoRedisplayMode(int on_off);
#endif 

/*-----------------------------------------------------------------------------
|                                                                              *
|       releasevarlist                                                         *
|                                                                              *
|	free any stored command arguments
|                                                                              *
+----------------------------------------------------------------------------*/
void releasevarlist()
{
  if (Dx_string)
  {
    release(Dx_string);
    Dx_string = NULL;
  }
}


/******************************************************************************
*                                                                             *
*       evalName/3                                                            *
*                                                                             *
*       var - variable, strval - buffer to return string value, 	      *
*       maxlen - length of buffer					      *
*       This routine obtains the value of string variable 		      *
*       If variable does not exit, null string returned.		      *
*       If variable is not a string, null string returned.		      *
*       If string value length is greater than maxlen then only maxlen-1 char *
*	   are returned. 						      *
*									      *
*       an IF-THEN-FI execS expression. Take expression and generate an       *
*       if ( expression ) then $i=0 endif, give it to the parser , then use   *
*	execN() to obtain value. 					      *
*                                                                             *
*				Author: Greg Brissey 6/6/90		      *
******************************************************************************/
int execName(const char *var, char *strval, int maxlen)	
{  
   char *expbuf;
   int   explen;
   char *prevTempID;
   const char *oldbp;
   int   oldFromFile;
   int   oldFromString;
   node *oldcodeTree;
   int   returnCode=0;

   LPRINT0(1,"execName: starting...\n");
   LPRINT1(1,"execName: expression ='%s'\n",var);
   if ( (explen = strlen(var)) > 0)
   {
  
     expbuf = (char *)allocateWithId( (explen + 35) * sizeof(char), "execName");
     if (expbuf == NULL)
     {
        Werrprintf("cannot allocate memory for expression string");
        ABORT;
     }
     /* for parser: 'if ( expression ) then statement endif'  */
     strcpy(expbuf,"if ( ");
     strcat(expbuf,var);
     strcat(expbuf," ) then $i=0 endif \n");
     LPRINT1(1,"execName: parser expression ='%s'\n",expbuf);
   
      oldFromFile   = fromFile;		/* push existing file info */
      oldFromString = fromString;
      oldbp         = bp;
      oldcodeTree   = codeTree;

      fromFile      = 0;
      fromString    = 1;
      bp            = expbuf;
      codeTree      = NULL;

      prevTempID    = tempID; tempID = newTempName("tmpExecExpID");

      /* parse expression IF-THEN-FI */
      switch (yyparse())
      { case 0:
		LPRINT0(1,"execName: ...parser gives code 0, go do it!\n");
#ifdef  DEBUG
		if ( 1 < Lflag)
                {
		  showTree(0,"execName:    ",codeTree);
		  showFlavour(stderr,codeTree);
                }
#endif 
		switch(codeTree->flavour)
		{
		   case THEN:  	/* It has to be this or something is real wrong. */
			{ 
			    char    *name;
                            int      shouldFree;
                            varInfo *v;
  			    pair p;

                            if ( (name=execN(codeTree->Lson->Lson,&shouldFree, NULL)) )
			    {
                              if ( (v=findVar(name)) ) /* does variable exist? */
			      {
                                if (execR(codeTree->Lson,&p, NULL)) /* obtain value info */
			        {
				   if (p.T.basicType == T_STRING)
				   {
				     if ( (int) strlen(p.R->v.s) >= maxlen)
				     {
					strncpy(strval,p.R->v.s,maxlen-1);
					strval[maxlen-1]=0; /* Null terminate string */
				     }
				     else
				     {
				       strcpy(strval,p.R->v.s);
				     }
				   }
				   else
			 	     *strval = 0;
				}
				else
			 	   *strval = 0;
                                cleanPair(&p);
			      }
                              else
                              { 
                                 *strval = 0; /* return null string */
                              }
			    }
                            if (shouldFree)
                            {
                                LPRINT1(3,"execName: ...releasing name %s\n",name);
                                release(name);
                            }
			    returnCode = strlen(strval);
                         }
			break;
                   default:
		        WerrprintfWithPos(
		         "BUG! execName has a bad flavour (=%d)",codeTree->flavour);
                        returnCode = -1;
			break;
         	}
		break;
	case 1:
		LPRINT0(1,"execName: ...parser gives code 1, drop everything!\n");
		ignoreEOL  = 0;		/* had a syntax error, drop it */
		returnCode = -1;
		break;
	case 2:
		LPRINT0(1,
	         "execName: ...parser gives code 2, incomplete expression, abort\n");
		ignoreEOL  = 0;		/* had a syntax error, drop it       */
		returnCode = -1;
		break;
      }
      codeTree = NULL;

      releaseWithId(tempID); free(tempID); tempID = prevTempID;

      fromFile   = oldFromFile;		/*  restore old file/kbd/string state*/
      fromString = oldFromString;
      bp         = oldbp;
      codeTree   = oldcodeTree;
      releaseAllWithId("execName");
      LPRINT0(1,"execName: finishing...\n");
   }
   else
   {  fprintf(stderr,"execName: string (=\"%s\") is null.\n",var);
      returnCode = -1;
   }
   return(returnCode);
}

/******************************************************************************
*                                                                             *
*       execExp/1                                                             *
*                                                                             *
*       This routine attempts to evaulate the string expression as if it was  *
*       an IF-THEN-FI execS expression. Take expression and generate an       *
*       if ( expression ) then $i=0 endif, give it to the parser , then use   *
*	execR() to evaluate it. 					      *
*       For single variable expressions 				      *
*	 if not present, expression is false				      *
*	 if real variable & inactive, expression is false		      *
*	 if string & null string value, expression is false		      *
*       For Multiple variable expressions				      *
*        if a variable is not present, Error is reported & error returned     *
*       Parser errors return -1					              *
*                                                                             *
*				Author: Greg Brissey 5/31/90		      *
******************************************************************************/
int execExp(const char *buffer)	
{  
   char *expbuf;
   int   explen;
   char *prevTempID;
   const char *oldbp;
   int   oldFromFile;
   int   oldFromString;
   node *oldcodeTree;
   int   returnCode=0;
   int   gocheckit;

   LPRINT0(1,"execExp: starting...\n");
   LPRINT1(1,"execExp: expression ='%s'\n",buffer);
   if ( (explen = strlen(buffer)) > 0)
   {
  
     expbuf = allocateWithId( (explen + 35) * sizeof(char), "execExp");
     if (expbuf == NULL)
     {
        Werrprintf("cannot allocate memory for expression string");
        ABORT;
     }
     /* for parser: 'if ( expression ) then statement endif'  */
     strcpy(expbuf,"if ( ");
     strcat(expbuf,buffer);
     strcat(expbuf," ) then $i=0 endif \n");
     LPRINT1(1,"execExp: parser expression ='%s'\n",expbuf);
   
      oldFromFile   = fromFile;		/* push existing file info */
      oldFromString = fromString;
      oldbp         = bp;
      oldcodeTree   = codeTree;

      fromFile      = 0;
      fromString    = 1;
      bp            = expbuf;
      codeTree      = NULL;

      prevTempID    = tempID; tempID = newTempName("tmpExecExpID");

      /* parse expression IF-THEN-FI */
      switch (yyparse())
      { case 0:
		LPRINT0(1,"execExp: ...parser gives code 0, go do it!\n");
#ifdef  DEBUG
		if ( 1 < Lflag)
                {
		  showTree(0,"execExp:    ",codeTree);
		  showFlavour(stderr,codeTree);
                }
#endif 
		switch(codeTree->flavour)
		{
		   case THEN:  	/* It has to be this or something is real wrong. */
			{ 
			    char    *name;
                            int      shouldFree;
                            varInfo *v;
  			    pair p;

			    /* was it a single variable expression ?*/
			    /* if not flavour,  will be an operator not ID or LB */
                            gocheckit = 0;
                            v = NULL;
                            if ((codeTree->Lson->flavour == ID) ||
				(codeTree->Lson->flavour == LB) )
			    {
                              if ( (name=execN(codeTree->Lson->Lson,&shouldFree,NULL)) )
			      {
                                if ( (v=findVar(name)) )
				{
                                  gocheckit = 2; /* variable present, check active */
				}
                                else
                                { 
                                  gocheckit = 0; /* variable not present */
                                }
			      }
                              if (shouldFree)
                              {
                                LPRINT1(3,"execExp: ...releasing name %s\n",name);
                                release(name);
                              }
			    }
                            else
                              gocheckit = 1;	/* two variable expression */


			   if (gocheckit)
			   {
                            if (execR(codeTree->Lson,&p, NULL))
			    {
			       if((gocheckit > 1) && (p.T.basicType == T_REAL)	)
			       {
				  if (!v->active)
				  {
			             returnCode = 0;
                            	     cleanPair(&p);
				     break;
				  }	
			       }
			       if((gocheckit > 1) && (p.T.basicType == T_STRING)	)
			       {
				  if (strcmp(p.R->v.s,"n") == 0)
				  {
			             returnCode = 0;
                            	     cleanPair(&p);
				     break;
				  }	
			       }
                               if (isTrue(&p))
                               {
                                 LPRINT0(1,"execExp: IF-THEN-FI exp is true\n");
                                 returnCode = 1;
                               }
                               else
                               {   
                                LPRINT0(1,"execExp: IF-THEN-FI exp is false\n");
                                returnCode = 0;
                               }
			    }
                            else
                            {   
                               LPRINT0(1,"execExp: IF-THEN-FI exp failed\n");
                               returnCode = -1;
                            }
                            cleanPair(&p);
			   }
			   else
			     returnCode = 0;	/* variable not present */
                         }
			break;
                   default:
		        WerrprintfWithPos(
		         "BUG! execExp has a bad flavour (=%d)",codeTree->flavour);
                        returnCode = -1;
			break;
         	}
		break;
	case 1:
		LPRINT0(1,"execExp: ...parser gives code 1, drop everything!\n");
		ignoreEOL  = 0;		/* had a syntax error, drop it */
		returnCode = -1;
		break;
	case 2:
		LPRINT0(1,"execExp: ...parser gives code 2, incomplete expression, abort\n");
		ignoreEOL  = 0;		/* had a syntax error, drop it       */
		returnCode = -1;
		break;
      }
      codeTree = NULL;

      releaseWithId(tempID); free(tempID); tempID = prevTempID;

      fromFile   = oldFromFile;		/*  restore old file/kbd/string state*/
      fromString = oldFromString;
      bp         = oldbp;
      codeTree   = oldcodeTree;
      releaseAllWithId("execExp");
      LPRINT0(1,"execExp: finishing...\n");
   }
   else
   {  fprintf(stderr,"execExp: string (=\"%s\") is null.\n",buffer);
      returnCode = -1;
   }
   return(returnCode);
}

/******************************************************************************
*                                                                             *
*       execString/1                                                          *
*                                                                             *
*       This routine should attempt to execute the string as if it was entered*
*       on the terminal.  The string must have a carriage return.             *
*                                                                             *
******************************************************************************/

int execString(const char *buffer)
{  char *prevTempID;
   const char *oldbp;
   int   oldFromFile;
   int   oldFromString;
   node *oldcodeTree;
   int   oldworking;
   int   oldinteruption;
   int   returnCode=0;
    
   if (0 < strlen(buffer) && buffer[strlen(buffer)-1] == '\n')
   {
      LPRINT0(1,"execString: starting...\n");
      oldFromFile   = fromFile;		/* push existing file info */
      oldFromString = fromString;
      oldbp         = bp;
      oldcodeTree   = codeTree;
      oldinteruption= interuption;
      oldworking    = working;

      fromFile      = 0;
      fromString    = 1;
      bp            = buffer;
      codeTree      = NULL;
      working       = 1;
      interuption   = 0;

      prevTempID    = tempID; tempID = newTempName("tmpExecStringID");

      switch (yyparse())
      { case 0:
		LPRINT0(1,"execString: ...parser gives code 0, go do it!\n");
		if (codeTree && codeTree->flavour != ENDFILE)
                {
		   if (execS(codeTree, NULL))
		      returnCode = 0;
		   else
		      returnCode = 1;
                }
		break;
	case 1:
		LPRINT0(1,"execString: ...parser gives code 1, drop everything!\n");
		ignoreEOL  = 0;		/* had a syntax error, drop it        */
		returnCode = 1;
		break;
	case 2:
		LPRINT0(1,"execString: ...parser gives code 2, incomplete expression, abort\n");
		ignoreEOL  = 0;		/* had a syntax error, drop it       */
		returnCode = 1;
		break;
      }
      codeTree = NULL;

      releaseWithId(tempID); free(tempID); tempID = prevTempID;

      fromFile   = oldFromFile;		/*  restore old file/kbd/string state*/
      fromString = oldFromString;
      bp         = oldbp;
      codeTree   = oldcodeTree;
      interuption= oldinteruption;
      working    = oldworking;
      LPRINT0(1,"execString: finsihing...\n");
   }
   else
   {  fprintf(stderr,"execString: string (=\"%s\") does not end with \\n\n",buffer);
      returnCode = 1;
   }
   return(returnCode);
}

/*-----------------------------------------------------------------------------
|                                                                              *
|       autoRedisplay                                                          *
|                                                                              *
|	This routine checks if there has been any variables changed by
|	looking in the DX_string.  If there has it determines if it
|	can update the screen by executing the last command.  If it
|	can, it builds a string and sends it to execString.
|	If Wget____display returns a pointer to a null (not a null
|	pointer!) or returns a pointer to the string "", we figure
|	that we can not update the screen.
|                                                                              *
+----------------------------------------------------------------------------*/

void autoRedisplay()
{  char           function[20];
   char          *screenCommand;
   static int     recursive = 0;
   extern int     interuption;
   extern int   textIsOn;
   extern int   grafIsOn;
   extern int   autotextIsOn;
   extern int   autografIsOn;
   int          tmpOn;

   if (!recursive)
   {  recursive = 1;
#ifdef VNMRJ
      if (Jx_string)   /* if there are any changed variables */
      {  if (!Bnmr)     /* if not in background */
	 {
            jAutoRedisplayList();
	 }
	 releaseJvarlist();
      }
      resetjEvalGlo();
#endif 
      if (Dx_string)   /* if there are any changed variables */
      {  if (!Bnmr)     /* if not in background */
	 {
            screenCommand = Wgettextdisplay(function,20);
	    if (*screenCommand && (strcmp(screenCommand,"") != 0) &&
		 isReexecCmd(screenCommand))
	    {  screenCommand = newStringId(screenCommand,"autoRedisplay");
	       screenCommand = newCatId(screenCommand,"(","autoRedisplay");
	       screenCommand = newCatId(screenCommand,Dx_string,"autoRedisplay");
	       screenCommand = newCatId(
			screenCommand,",'redisplay parameters')\n","autoRedisplay"
	       );
	       TPRINT1("autoRedisplay: text \"%s\"\n",screenCommand);
               tmpOn = textIsOn;
               if (!autotextIsOn)
                  textIsOn = 0;
	       execString(screenCommand);
               textIsOn = tmpOn;
	       release(screenCommand);
	    }
	    if (Wissun())  /* if text is separate from graphics */
	    {  screenCommand = Wgetgraphicsdisplay(function,20);
	       if (*screenCommand && (strcmp(screenCommand,"") != 0) &&
		    isReexecCmd(screenCommand))
	       {  screenCommand = newStringId(screenCommand,"autoRedisplay");
		  if (Dx_string)
		  {  screenCommand = newCatId(screenCommand,"(","autoRedisplay");
		     screenCommand = newCatId(screenCommand,Dx_string,"autoRedisplay");
		     screenCommand = newCatId(
			screenCommand,",'redisplay parameters')\n","autoRedisplay"
	             );
		  }
		  else
		     screenCommand = newCatId(
			screenCommand,"('redisplay parameters')\n","autoRedisplay"
	             );
		  TPRINT1("autoRedisplay: graphics \"%s\"\n",screenCommand);
                  tmpOn = grafIsOn;
                  if (!autografIsOn)
                     grafIsOn = 0;
#ifdef VNMRJ
                  setAutoRedisplayMode(1);
#endif
		  if(strstr(screenCommand,"asp")) aspFrame("redraw",0,0,0,0,0);
		  else execString(screenCommand);
                  grafIsOn = tmpOn;
		  release(screenCommand);
#ifdef VNMRJ
                  setAutoRedisplayMode(0);
#endif
	       }
	    }
	 }
	 releasevarlist();
      }
      TPRINT3("menu test: menus %s %s active; buttons %s active\n"
		       ,(menuflag) ? "on" : "off",(menuon) ? "and" : "but not"
		       ,(buttons_active) ? "are" : "not");
      if (menuflag && !menuon && !buttons_active && !Bnmr)
      {
         interuption = 0;       /* turn off effects of cancelCmd button */
	 execString("menu\n");
      }
      recursive = 0;
#ifdef VNMRJ
      if (!Bnmr)
	 sendTripleEscToMaster( 'p', "" );
#else 
#ifdef VNMRTCL
      if (!Bnmr)
      {
         if (updateMagicVar())
            sendTripleEscToMaster( 'T', "" );
      }
#endif 
#endif 
   }
}

/* Variable to determine if error line should be cleared. */
static int doErrClear = 1;
void skipErrClear()
{
   doErrClear = 0;
}
/*******************************************************************************
*                                                                              *
*       loadAndExec/1                                                          *
*                                                                              *
*       This routine is triggered by terminal_main_loop when a carriage 
*       return is intercepted.  Notice the special return from 
*       yyparse (suspended) for dealing with the notifier driven environment.   
*                                                                              *
*******************************************************************************/

int loadAndExec(char *buffer)
{  int returnCode = 0;

   LPRINT1(1,"loadAndExec: have... \"%s\" starting...\n",buffer);
   fromFile     = 0;
   fromString   = 0;
   tempID       = "loadAndExec";
   bp           = buffer;
   switch (yyparse())
   { case 0:
	     LPRINT0(1,"loadAndExec: ...parser gives code 0, go do it!\n");
	     if (codeTree)
	     {
                if (doErrClear)
                   Wclearerr(); /* clear error line */
                doErrClear = 1;
		if (codeTree->flavour != ENDFILE)
		{
                   clearMustReturn();
                   if (execS(codeTree, NULL))
		      returnCode = 0;
		   else
		      returnCode = 1;
                   if (strncmp(buffer,"jMove", 5) != 0) /* mouse move? */
		      autoRedisplay(); /* if variable changed re-display */ 
                   else if (Jx_string) {
                      if (!Bnmr)
                         jAutoRedisplayList();
                   }
		}
		codeTree = NULL;
	     }
	     releaseTempCache();	/* of macro code trees */
	     releaseWithId(tempID);
	     releaseWithId("newMacro");
/* #ifdef VNMRJ
	     Wscrshow("resume");
#endif */
	     break;
     case 1:
	     LPRINT0(1,"loadAndExec: ...parser gives code 1, drop everything!\n");
	     ignoreEOL  = 0;		/* had a syntax error, drop it        */
	     codeTree   = NULL;
	     returnCode = 1;
	     releaseTempCache();
	     releaseWithId(tempID);
	     releaseWithId("newMacro");
	     break;
     case 2:
	     LPRINT0(1,"loadAndExec: ...parser gives code 2, back for more\n");
	     returnCode = 0;
	     break;			/* suspended, back for more           */
   }
   return(returnCode);
}

/*******************************************************************************
*
*	input/0
*
*	This function is used by the lexer to get characters.  Notice that
*	characters come from two quite different sources: terminal (sunwindow)
*	or from a macro file.  This routine picks a character and returns it.
*	When the buffer is exhausted, the difference arises: for sunwindows
*	the lexer will suspend the parser (and wait for more characters), for
*	file input, this routine just goes back for more.
*
*	NOTE: The special "suspendable" parser is required!
*
*******************************************************************************/

char         macroBuffer[1024];
char        *macroBp;
extern char  fileName[];
extern FILE *macroFile;
extern int   columnNumber;
extern int   lineNumber;

int input()
{
   LPRINT0(2,"input: starting...\n");
   if (fromFile)
   {
      LPRINT1(2,"input: ...from file \"%s\"\n",fileName);
      while (*macroBp == '\0')
      {  register char *p;
	 register int   c;

	 LPRINT0(2,"input: ...out of characters!\n");
	 p = macroBuffer;
	 while (1)
	    if ((c=getc(macroFile)) == EOF)
	       if (p == macroBuffer)
	       {
		  LPRINT0(2,"input: ...return '^D'\n");
		  return('\004');
	       }
	       else
	       {  *p = '\0';
		  LPRINT0(2,"input:    ...<done>\n");
		  break;
	       }
	    else
	    {
               if (c >= 127)
                  c -= 127;
              *p++ = c;
	       LPRINT1(2,"input:    ...'%c'\n",c);
	       if (c == '\n' || c == '\0')
	       {  *p = '\0';
		  LPRINT0(2,"input:    ...<done>\n");
		  break;
	       }
	    }
	 macroBp = macroBuffer;
      }
#ifdef DEBUG
      if (2 <= Lflag)
      {
	 if (*macroBp == '\n')
         {
	    LPRINT0(2,"input: ...return '\\n'");
         }
	 else
         {
            if (' ' <= *macroBp && *macroBp <= '~')
            {
	       LPRINT1(2,"input: ...return '%c'",*macroBp);
            }
	    else
            {
	       LPRINT1(2,"input: ...return '\\%03o'",*macroBp);
            }
         }
      }
#endif 
      if (*macroBp == '\n')
      {  oldColumnNumber = columnNumber;
	 columnNumber    = 1;
	 lineNumber     += 1;
      }
      else
	 columnNumber += 1;
      LPRINT2(2," (line %d, col %d)\n",lineNumber,columnNumber);
      return(*macroBp++);
   }
   else
   {  if (*bp == '\0')
      {  if (fromString)
	 {
	    LPRINT0(2,"input: ...end of string!\n");
	    return('\004');
	 }
	 else
	 {
	    LPRINT0(2,"input: ...suspending!\n");
	    return('\001');
	 }
      }
      else
      {
#ifdef   DEBUG
         if (2 <= Lflag)
         {
	    if (fromString)
	    {  if (*bp == '\n')
               {
		  LPRINT0(2,"input: ...return '\\n' (from string)\n");
               }
	       else
               {
                  if (' ' <= *bp && *bp <= '~')
                  {
		     LPRINT1(2,"input: ...return '%c' (from string)\n",*bp);
                  }
	          else
                  {
		     LPRINT1(2,"input: ...return '\\%03o' (from string)\n",*bp);
                  }
               }
	    }
	    else
	    {  if (*bp == '\n')
               {
		  LPRINT0(2,"input: ...return '\\n' (from terminal)\n");
               }
	       else
               {
                  if (' ' <= *bp && *bp <= '~')
                  {
		     LPRINT1(2,"input: ...return '%c' (from terminal)\n",*bp);
                  }
	          else
                  {
		     LPRINT1(2,"input: ...return '\\%03o' (from terminal)\n",*bp);
                  }
               }
	    }
         }
#endif 
	 return(*bp++);
      }
   }
}

void unput(c)				int c;
{  if (c != '\001')
   {
      if (fromFile)
      {  columnNumber -= 1;
	 if (c == '\n')
	 {  columnNumber = oldColumnNumber;
	    lineNumber  -= 1;
	 }
	 else
         {
	    columnNumber -= 1;
         }
	 *(--macroBp) = c;
      }
      else
      {
	 --bp;
      }
   }
}

int yywrap()
{  return(1);
}
