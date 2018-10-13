/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*-----------------------------------------------------------------------------
|	handler.c
|
|	processChar(c)			char c;
|	sendReadyToMaster( )
|
|	This module contains the code to process characters that come
|	in from the Sun keyboard.  Its subroutines are only utilized
|	when VNMR is run on the Sun console in the master / Vnmr
|	configuration.
|
|	Remember this is the Child (Vnmr)
|
+----------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vnmrsys.h"
#include "buttons.h"
#include "tools.h"
#include "wjunk.h"

#ifdef  DEBUG
extern int      Tflag;
#define TPRINT0(lvl,str) \
	if (Tflag >= lvl) fprintf(stderr,str)
#define TPRINT1(lvl,str, arg1) \
	if (Tflag >= lvl) fprintf(stderr,str,arg1)
#define TPRINT2(lvl,str, arg1, arg2) \
	if (Tflag >= lvl) fprintf(stderr,str,arg1,arg2)
#define TPRINT3(lvl,str, arg1, arg2, arg3) \
	if (Tflag >= lvl) fprintf(stderr,str,arg1,arg2,arg3)
#else 
#define TPRINT0(lvl,str) 
#define TPRINT1(lvl,str, arg2) 
#define TPRINT2(lvl,str, arg1, arg2) 
#define TPRINT3(lvl,str, arg1, arg2, arg3) 
#endif 
#define  MAX_NUM_BUTS 40

extern char	     vnMode[];
extern int           curSor;
extern int           working;
extern int           Wturnoff_buttonsCalled;
#ifdef VNMRJ
extern int           df_repeatscan;
#endif 

#define NEWLINE         10
#define BACKSPACE       '\010'  /***/
#define CONTROL_C       '\003'  /***/
#define CONTROL_D       '\004'  /***/
#define CONTROL_U       '\025'  /***/
#define DELETE          '\177'  /***/
#define ESC		'\033'
#define NEWLINE         10
#define CRRETURN        13

static void doFunction(int c);

/*
   Parameters for Command history. Two files are available: a global file and a local file.
   Typically, the global file would keep the complete history. The local file might to used
   to capture inputs to build a macro.
   The doCmdHistory parameter is true if either of the two history files are active.
   The doThisCmdHistory is a parameter to decide if a particular command should be put in the
   history files. Before every command execution, doThisCmdHistory is initialized to the value
   of doCmdHistory. Specific commands, such as jMove and jFunc, can prevent the command
   from being put into the history file by setting doThisCmdHistory = 0. Macros can prevent
   themselves from being put into the history file by calling cmdHistory('skip')
 */

   
static int doCmdHistory = 0;
int        doThisCmdHistory;
static void saveCmdHistory(char *buffer);
FILE *globalCmdHistory = NULL;
FILE *localCmdHistory = NULL;
char localCmdHistoryPath[MAXPATH];

#ifdef VNMRJ

extern int okExec(char *);
extern void newLogCmd();
extern void setExecLevel(int n);
extern void saveLogCmd(char *s, int n);
extern int sendTripleEscToMaster(char code, char *string_to_send );
extern void executeFunction(int num);
extern void nmr_exit(char *modeptr);
extern void p11_updateParamChanged();
extern void p11_writeCmdHistory(char* str);
extern int loadAndExec(char *buffer);
extern void skipErrClear();
extern void restore_original_cursor();
extern void set_hourglass_cursor();
extern void set_batch_function(int on);
extern void setCancel(int doit, char *str);

static int get_hourglass(char *buffer)
{
  int len, ret;
  char ch0;
  if (df_repeatscan == 1)
      return 0;
  ch0 = buffer[0];
  ret = 1;
  if (ch0 == 'j')
  {
      if (strncmp(buffer,"jFunc",5)==0)
         ret = 0;
      else 
      {
         if (strncmp(buffer,"jEven",5)==0)
            ret = 0;
         else if (strncmp(buffer,"jMove",5)==0)
            ret = 0;
      }
      return ret;
  }
  if (ch0 == 'a')
  {
      if ((strncmp(buffer,"acqsend",7)==0) || (strncmp(buffer,"acqstat",7)==0))
         ret = 0;
      else if (strncmp(buffer,"aip",3)==0)
         ret = 0;
      return ret;
  }
  if (ch0 == 'i')
  {
      if (strncmp(buffer,"isim",4)==0) // isimagebrowser
         ret = 0;
      return ret;
  }
  len = strlen(buffer);
  if (len < 6)
      return ret;
  if (strncmp(buffer,"readlk",6)==0)
      ret = 0;
  else if (strncmp(buffer,"vnmrjc",6)==0) // vnmrjcmd
      ret = 0;
 
  return ret;
}
#endif 

void sendReadyToMaster()
{
    if (!Wissun()) return;
    setCancel(1,"");
    sendTripleEscToMaster( 'R', "0");
}
/*------------------------------------------------------------------------------
|
|	processChar
|
|	This routine processes each character that comes in from the
|	sun keyboard via the pipe between master and Vnmr.
|
+-----------------------------------------------------------------------------*/

void processChar(char c)
{  extern int  interuption;
   static char buffer[2048];
   static int  bp = 0;
   static int  escape = 0;
   static int  saw_escape = 0;
   static int  escape_num = 0;
#ifdef VNMRJ
   static int  show_hourglass = 0;
#endif 

   TPRINT2(1,"processChar: got %c (\\0%o)\n",
              (' ' <= c && c <= '~') ? c : ' ',c);
   if (escape && c != NEWLINE && c != CRRETURN)
   {

/*  Escape sequences must include a carriage return, so the Master
    recognizes the Child is busy, and will queue input from the
    Acqproc until the Child completes its current command.

    Thus the operation implied by the Escape Sequence is postponed
    until the Child receives a Carriage Return or New Line character.	*/

      saw_escape = 131071;
      escape_num = 10*escape_num + c-'0';
   }
   else
      switch (c)
      { case NEWLINE:
	case CRRETURN:    
			TPRINT0(1,"processChar: ...carriage return\n");
			if (saw_escape)
			{
			   saw_escape = 0;
                           escape = 0;
			   set_hourglass_cursor();
#ifdef VNMRJ
                           set_batch_function(1); // turn on graphics batch
                           setExecLevel(0);
                           newLogCmd("buttonFunc", 10);
#endif 
			   doFunction( escape_num+'0' );
#ifdef VNMRJ
                           set_batch_function(0); // turn off graphics batch
                           saveLogCmd("buttonFunc", 10);
#endif 
			   restore_original_cursor();
                           bp = 0;
			   sendReadyToMaster();
			   return;
			}
			else if (escape) {
				Werrprintf( "error in input from keyboard" );
				escape = 0;
			        saw_escape = 0;
                                bp = 0;
				sendReadyToMaster();
				return;
			}
			buffer[bp++] = '\n';
			buffer[bp++] = '\0';
			TPRINT0(2,"processChar:    saving environment\n");
			working = 1;

#ifdef VNMRJ
			if ((show_hourglass = get_hourglass(buffer)) != 0)
			   set_hourglass_cursor();
                        set_batch_function(1);
#else 
			show_hourglass = 1;
			set_hourglass_cursor();
#endif 
                        if (c == CRRETURN)
                           skipErrClear();

			/* save command history */
#ifdef VNMRJ
                        setExecLevel(0);
                        if (bp < 512) {
			    p11_writeCmdHistory(buffer); 
                            newLogCmd(buffer, bp);
                        }
		// if(okExec(buffer)) {	
                        doThisCmdHistory = doCmdHistory;
			loadAndExec(buffer);
                        if (doThisCmdHistory)
                        {
                           saveCmdHistory(buffer);
                        }
/*
if ( strncmp(buffer,"jMove",5) && strncmp(buffer,"jFunc",5) )
 */

                        set_batch_function(0);
                        if (bp < 512) {
                            saveLogCmd(buffer, bp - 2);
                        }
			
			p11_updateParamChanged();
		// }
#else 
			loadAndExec(buffer);

#endif 

                        if (show_hourglass != 0)
			    restore_original_cursor();
			Buttons_off();

			working     = 0;
			if (interuption)
			{
			   TPRINT0(2,"processChar:    Back at square one!\n");
			   interuption = 0;
			}
			else
			{
			   TPRINT0(2,"processChar:    Back again (normally)\n");
			}
			bp = 0;
			sendReadyToMaster();
			break;
	case ESC:       
			TPRINT0(1,"processChar: ...escape\n");
			escape = 1;
			escape_num = 0;
			break;
	case BACKSPACE:
	case DELETE:    
			TPRINT0(1,"processChar: ...delete\n");
			if (bp > 0)
			{  bp--;
			   putchar(BACKSPACE);
			   putchar(' ');
			   putchar(BACKSPACE);
			}
			break;
	case CONTROL_U: 
			TPRINT0(1,"processChar: ...^U\n");
			bp = 0;
			break;
	case EOF:
	case CONTROL_D: 
			TPRINT0(1,"processChar: ...end of file\n");
			nmr_exit(vnMode);
			break;
	default:	
			TPRINT1(2,"terminal_mail_loop: got %c\n",c);
			buffer[bp++] = c;
                        if (bp > 2044) {
			    buffer[bp++] = '\n';
			    buffer[bp++] = '\0';
		  	    Werrprintf( "%s", buffer);
		  	    Werrprintf( "command was too long, abort!\n" );
                            bp = 0;
                        }
			break;
      }
}

/*  This routine DOES NOT have to notify the parent that the
    child is ready for further input from AcqProc.		*/

#ifdef GRAPHON
static void processCursor(char *s)
{  char        *ptr;
   char        *xp;
   char        *yp;
   extern int (*mouseButton)();
   extern int (*mouseMove)();
   int          butNum;
   int          x,y;
   static int   butNumPushed;

   TPRINT1(2,"processCursor: got \"%s\"\n",s);
   xp = ptr = &s[1];
   while (*ptr != ';' && *ptr)
      ptr++;
   *ptr++ = '\0';
   yp     = ptr;
   while (*ptr != ';' && *ptr)
      ptr++;
   if (!*ptr)
   {  butNum = 0;
   }
   else
   {  *ptr++= '\0';
      butNumPushed = butNum = *ptr - '0';
      switch (butNumPushed)
      { case 4:  butNumPushed = 0;
		 break;
	case 2:  butNumPushed = 1;
		 break;
	case 1:  butNumPushed = 2;
		 break;
	default: 
		 TPRINT1(2,"processCursor: bad button pushed %d\n",butNumPushed);
      }
   }
   x = atoi(xp);
   y = atoi(yp);
   TPRINT3(1,"processCursor: x=%d y=%d butNum = %d\n",x,y,butNum);
   if (mouseButton) /* if button routine defined */
      (*mouseButton)(butNumPushed,butNum ? 0 : 1, x,y);
   if (mouseMove)
      (*mouseMove)(x,y);
}
#endif

static void doFunction(int c)
{
#ifdef GRAPHON
   char curSeq[14];
   int curSeqptr;
#endif
   int num;

   TPRINT1(1,"doFunction: Function number %c\n",c);

/* if cursor is on, check for graphon cursor seq */

#ifdef GRAPHON
   if (curSor && c == '9' && Wisgraphon())
   {  curSeqptr = 0;
      while ((c = getchar()) != '\n' && c != 'v')
      {   curSeq[curSeqptr++] = c;
      }
      curSeq[curSeqptr] = '\0';
      processCursor(curSeq);
      return;
   }
#endif
   if (c < '1' || ('0' + MAX_NUM_BUTS) < c)
   {  Werrprintf("'%c'  No such function key",c);
      return;
   }
   num = c - '1';
   executeFunction(num); /* this routine located in terminal.c */
}	


/* Functions related to Command History */

static void saveCmdHistory(char *buffer)
{
   if (globalCmdHistory)
   {
      fprintf(globalCmdHistory,"%s",buffer);
      fflush(globalCmdHistory);
   }
   if (localCmdHistory)
   {
      fprintf(localCmdHistory,"%s",buffer);
      fflush(localCmdHistory);
   }
}

/*  cmdHistory

    cmdHistory<:$global,$local>   - display or return current history files
    cmdHistory('on')              - start new global command history file
    cmdHistory('append')          - append to existing global history file
    cmdHistory('off')             - stop the global history file
    cmdHistory('on',filename)     - start new local command history file
    cmdHistory('append',filename) - append to existing local history file
    cmdHistory('off',filename)    - stop the local history file
    cmdHistory('skip')            - do not put current macro into command history

    cmdHistory will save commands executed from the command line or as a result
    using interface items such as buttons or menus. Two separate command histories
    can be kept. A macro can avoid being added to the history files by calling cmdHistory
    with the 'skip' argument. The cmdHistory files are not kept for any background
    operations.

    Issues: name of the global history file. Should it only be active from VP 1. If not,
    need separate names. How to handle multiple operators.

 */

int cmdHistory(int argc, char *argv[], int retc, char *retv[])
{
   char path[MAXPATH];

   if (argc == 1)
   {
      if (globalCmdHistory)
         sprintf(path,"%s/cmdHistory",userdir);
      if (retc)
      {
         retv[0] = (globalCmdHistory) ? newString("") : newString(path);
         if (retc > 1)
            retv[1] = (localCmdHistory) ? newString("") : newString(localCmdHistoryPath);
      }
      else
         Winfoprintf("Global history is %s. Local history is %s",
            (globalCmdHistory) ? path : "off",
            (localCmdHistory) ? localCmdHistoryPath : "off");
      doThisCmdHistory = 0;
      RETURN;
   }
   if (Bnmr)
      RETURN;
   if (argc == 2) /* This handles global cmdHistory and the 'skip' case */
   {
      if ( ! strcmp(argv[1],"on") ) /* write to global cmdHistory */
      {
         if (globalCmdHistory == NULL)
         {
            sprintf(path,"%s/cmdHistory",userdir);
            globalCmdHistory = fopen(path,"w");
         }
         doCmdHistory = 1;
         doThisCmdHistory = 0;
      }
      else if ( ! strcmp(argv[1],"append") ) /* append to global cmdHistory */
      {
         if (globalCmdHistory == NULL)
         {
            sprintf(path,"%s/cmdHistory",userdir);
            globalCmdHistory = fopen(path,"a");
         }
         doCmdHistory = 1;
         doThisCmdHistory = 0;
      }
      else if ( ! strcmp(argv[1],"off") )
      {
         if (globalCmdHistory)
         {
            fclose(globalCmdHistory);
            globalCmdHistory = NULL;
         }
         if ( ! localCmdHistory)
            doCmdHistory = 0;
         doThisCmdHistory = 0;
      }
      else if ( ! strcmp(argv[1],"skip") )  /* allow macros to avoid being added to the history */
      {
         doThisCmdHistory = 0;
      }
      else
      {
         Wscrprintf("%s usage error: %s unknown argument",argv[0], argv[1]);
      }
   }
   else if (argc == 3) /* This handles local cmdHistory */
   {
      if ( ! strcmp(argv[1],"on") )
      {
         if ( localCmdHistory )
            fclose(localCmdHistory);
         localCmdHistory = fopen(argv[2],"w");
         if ( localCmdHistory )
         {
            doCmdHistory = 1;
            doThisCmdHistory = 0;
            strcpy(localCmdHistoryPath,argv[2]);
         }
         else
         {
            Werrprintf("%s failed to open command history file %s",argv[0], argv[2]);
         }
      }
      else if ( ! strcmp(argv[1],"append") )
      {
         if ( localCmdHistory )
            fclose(localCmdHistory);
         localCmdHistory = fopen(argv[2],"a");
         if ( localCmdHistory )
         {
            doCmdHistory = 1;
            doThisCmdHistory = 0;
            strcpy(localCmdHistoryPath,argv[2]);
         }
         else
         {
            Werrprintf("%s failed to open command history file %s",argv[0], argv[2]);
         }
      }
      else if ( ! strcmp(argv[1],"off") )
      {
         if (localCmdHistory)
            fclose(localCmdHistory);
         localCmdHistory = NULL;
         if ( ! globalCmdHistory)
            doCmdHistory = 0;
         doThisCmdHistory = 0;
      }
      else
      {
         Wscrprintf("%s usage error: %s unknown argument",argv[0], argv[1]);
      }
   }
   else
   {
      Wscrprintf("%s usage error: too many arguments",argv[0]);
   }
   RETURN;
}

