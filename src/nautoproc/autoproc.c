/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <signal.h>


#include <errno.h>

#include "hostMsgChannels.h"
#include "msgQLib.h"
#include "errLogLib.h"
#include "eventHandler.h"
#include "expQfuncs.h"
#include "procQfuncs.h"

#if defined __GNUC__ && __GNUC__ >= 14
#pragma GCC diagnostic warning "-Wimplicit-function-declaration"
#endif

#define MAXPATH 256

char ProcName[MAXPATH];

MSG_Q_ID pRecvMsgQ;
MSG_Q_ID pExpMsgQ;

char MsgInbuf[PROC_MSG_SIZE];

pid_t VnmrPid = 0;

char autodir[MAXPATH];         /* automation directory */
char enterpath[MAXPATH+16];       /* path to enter file */
char samplepath[MAXPATH];      /* path to sampleinfo file */
char psgQpath[MAXPATH];        /* path to psg Queue directory */
char doneQpath[MAXPATH];        /* path to done Queue directory */

/* Used by locksys.c  routines from vnmr */
char systemdir[MAXPATH];       /* vnmr system directory */
char userdir[MAXPATH];         /* vnmr user system directory */
char curexpdir[MAXPATH];       /* current experiment path */

/* Owner uid & gid of enterQ file */
uid_t enter_uid;
gid_t enter_gid;
static void childItrp(int);
extern int Resume(char *argstr);
extern int parser(char* str);
extern void setupexcepthandler();
extern int shutdownComm(void);
extern int initCmdParser();
extern int read_info_file(char *autodir);

static void set_output()
{
   int fd;
   int tmp_euid;
   int res __attribute__((unused));
   FILE *fp __attribute__((unused));

   tmp_euid = geteuid();
   res = seteuid(getuid());   /* change the effective uid to root from vnmr1 */
   fp = freopen("/dev/null","r",stdin);
   fp = freopen("/dev/console","a",stdout);
   fp = freopen("/dev/console","a",stderr);

   for (fd=3; fd < NOFILE; fd++)
     close(fd);
   res = seteuid(tmp_euid);   /* change the effective uid back to vnmr1 */
}

/*
|
|   1st arg is automation directory.
|
|       Auto Process
|       Responsibilities:
|               Sample Management Daemon
|	Typical run scenario:
|
| Resume sent to Autoproc
|  check EnterQ, 
|   if entry present then:
|   a. Write the enterQ entry (i.e. sampleinfo) to the sampleinfo
|      file located in 'autodir/exp1/sampleinfo'
|      (go & psg expect this file here)
|   b. Fork Vnmr (with sampleinfo arg, gets the user's directory from this)
|	The Vnmr runs auto_au macro.
|      retain Vnmr's PID for future reference
|   c. delete entry from EnterQ
|
|   At this point the forked Vnmr via Autoproc will produce a psgQ entry
|     This entry will either contain a valid Exp. to run or
|     have the key word "NOGO" as the Exp. ID string indicating
|     that no go was performed.
|     The go/psg copies sampleinfo from curpar to the fid directory along with
|      the other files it copies (text, curpar)
|     PSG updates the status and data text fields within the sampleinfo file
|	and places both the Experiment Queue Entry and the sampeinfo file
|       into the psgQ.
|     When this Vnmr exits the GrimReaper will call Resume()
|	
| Check psgQ 
|   if entry present read both the Exp Queue info and
|	    the sampleinfo from the psgQ. 
|   a. Copy the sampleinfo file into the doneQ
|   b. if the Exp ID String is not "NOGO" then place 
|       the Exp. entry into the Exp Queue (to start Exp) and
|       send msge to Expproc to check it's Q
|	Experiment Starts
|   c. delete entry from psgQ
|
|  When Experiment completes (or Error) Procproc performs BG processing
|  when this completes Procproc send a message "cmplt 'type' 'data_text_field'"
|  to Autoproc.
|
|  When "cmplt" message is received by Autoproc it marks the entry within the
|  DoneQ that matches the 'data_text_field' with the passed completion string
|  (either Complete or Error)
|
|
|  For details on resume logic see Resume() in autofuncs.c
|
|					Greg Brissey  10-4-95
|
*/


int main(int argc, char *argv[])
{
   char *tmpptr;
   struct stat statbuf;
   sigset_t    blockmask;
   void TheGrimReaper(void*);
   void processMsge(void*);
   void set_output();
   int res __attribute__((unused));
 
   strncpy(ProcName,argv[0],256);
   ProcName[255] = '\0';

  /* --- mask to block SIGALRM, SIGIO and SIGCHLD interrupts --- */
  sigemptyset( &blockmask );
  sigaddset( &blockmask, SIGALRM );
  sigaddset( &blockmask, SIGIO );
  sigaddset( &blockmask, SIGCHLD );
  sigaddset( &blockmask, SIGQUIT );
  sigaddset( &blockmask, SIGPIPE );
  sigaddset( &blockmask, SIGALRM );
  sigaddset( &blockmask, SIGTERM );
  sigaddset( &blockmask, SIGUSR1 );
  sigaddset( &blockmask, SIGUSR2 );
 
  DebugLevel = 0;

  umask(000); /* clear file creation mode mask,so that open has control */

  strcpy(autodir,argv[1]);
  set_output();

   DPRINT1(1,"Automation Directory: '%s'\n",autodir);
   fprintf(stdout,"Start of Automation\n");

   /* Lets use the syslog facility provide by the OS. */
   logSysInit("Autoproc",AUTOLOG_FACILITY);

   /* initialize exception handlers */
   setupexcepthandler();
 
   /* initialize command parser */
   initCmdParser();

   /* Map in Expproc Experiment Queues */
   initExpQs(0);   /* Experiment waiting to be run */
   initActiveExpQ(0);  /* Experiment that is presently acquiring */

   /* Map in Processing Queues */
   initProcQs(0);	/* pending processing */
   initActiveQ(0);	/* active processing */

    /* initialize environment parameter vnmrsystem value */
    tmpptr = getenv("vnmrsystem");            /* vnmrsystem */
    if (tmpptr != NULL)
    {
        strcpy(systemdir,tmpptr);      /* copy value into global */
    }
    else
    {
        strcpy(systemdir,"/vnmr");     /* use /vnmr as default value */
    }
    sprintf(enterpath,"%s/acqbin/Autolog",systemdir);
    if ( ! access(enterpath,F_OK) )
    {
        DebugLevel = 1;
    }
    strcpy(enterpath,autodir);
    strcat(enterpath,"/enterQ");
    strcpy(samplepath,autodir);
    strcat(samplepath,"/exp1/sampleinfo");
    strcpy(psgQpath,autodir);
    strcat(psgQpath,"/psgQ");
    strcpy(doneQpath,autodir);
    strcat(doneQpath,"/doneQ");
 
    DPRINT3(1,"autodir:'%s', enter:'%s', psgQ:'%s'\n",
        autodir,enterpath,psgQpath);
 
    /* do any remapping of enterQ key words */
    read_info_file(autodir);  

    /* Obtain the owner of the Automation directory. Forked Vnmr(s)
       will be run under this owner uid & gid.
    */
    if (stat(autodir, &statbuf) < 0)
    { 
      perror("autoproc: stat() can't stat enterQ: ");
      /* OK, default to root */
      enter_uid = (pid_t) 0;
      enter_gid = (gid_t) 0;
    }
    else
    {
      enter_uid = statbuf.st_uid;
      enter_gid = statbuf.st_gid;
    }
 
   /* Initialize the Event Handlers Queue */
   setupForAsync(20, &blockmask);
   blockAllEvents();

   /* here we register both the signal handler to be called
       and the non-interrupt function to handle the I/O */
   registerAsyncHandlers(
			  SIGCHLD,	/* BG processing Signal */
			  childItrp,	/* this puts the event on the eventQ */
			  TheGrimReaper
			 );

   /* setup key database, and Message Queue, facilities */
   pRecvMsgQ = createMsgQ("Autoproc", (int) AUTOPROC_MSGQ_KEY, (int) AUTO_MSG_SIZE);
   if (pRecvMsgQ == NULL)
      exit(1);

   pExpMsgQ = openMsgQ("Expproc");
   if (pExpMsgQ == NULL)
      exit(1);

   setMsgQAsync(pRecvMsgQ,processMsge);
   unblockAllEvents();

   /* switch to root here so that autoproc will have permission to lock and unlock the 
      various Qs it uses
   */
   res = seteuid(getuid());
   DPRINT4(1,"uid: %d, euid: %d, gid: %d, egid: %d\n",getuid(),geteuid(),getgid(),getegid());

   /*
    * Used to get this first resume from Expproc, but there is a race between Expproc
    * sending the resume and Autoproc's createMsgQ() which may interpret that resume
    * as an old message and delete it
    */
   Resume("");
   asyncMainLoop();

   closeMsgQ(pExpMsgQ);
   shutdownComm();

   return(EXIT_SUCCESS);
}

/**************************************************************
*
*  processMsge - Routine envoked to read message Q and call parser
*
*   This Function is the Non-interrupt function called to handle the
*   msgeQ interrupt as register in setupMsgSig() via registerAsyncHandlers()
*   (proccomm.c)
*  
* RETURNS:
* void 
*
*       Author Greg Brissey 9/6/94
*/
void processMsge(void *notin)
{
  int rtn;

  /* Keep reading the Msg Q until no further Messages */
  do {
	   /* read msgQ don't block if none there */
           rtn = recvMsgQ(pRecvMsgQ, MsgInbuf, RECV_MSG_SIZE, NO_WAIT);
	   /* if we got a message then go ahead and parse it */
	   if (rtn > 0)
	   {
	      DPRINT2(1,"received %d bytes, MsgInbuf len %zd bytes\n",rtn,strlen(MsgInbuf));
	      parser(MsgInbuf);
	      MsgInbuf[0] = '\0';
	   }
      }
      while(rtn != -1);	/* if no message continue on */

   return;
}

/**************************************************************
*
*  childItrp - Routine envoked on receiving the SIGCHLD
*
*  This catches the SIGCHLD Signal add place the 'event' on the
*  event Q. Then in non-interrupt mode the register function will
*  be called to handle the actual I/O.
*  
* RETURNS:
* void 
*
*       Author Greg Brissey 9/6/94
*/
static void
childItrp(int signal)
{
    /* Place the SIGCHLD interrupt & int onto the eventQ, the non-
       interrupt function (processMsge) will be called with msgId as an argument */
    processNonInterrupt( SIGCHLD, (void*) 0 );
    return;
}

/*-------------------------------------------------------------------------
|
|   TheGrimReaper()
|   Get the Status of the died children so that it may rest in peace
|   (i.e., get status of exited BG process so it doesn't become a Zombie)
|   Then check for more conditional processing.
|
|                               Author Greg Brissey 9/8/94
+--------------------------------------------------------------------------*/

void
TheGrimReaper(void* arg)
{
    int coredump;
    int pid;
    int status;
    int termsig;
    int kidstatus;

    DPRINT(1,"|||||||||||||||||||  SIGCHLD   ||||||||||||||||||||||||\n");
    DPRINT(1,"TheGrimReaper(): At Work !!!\n");

    /* --- GrimReaper get all exited or signal children before leaving --- */
    /*     Note:  1st argument to waitpid is -1 to specify any child process.  */
 
    while ((pid = waitpid( -1, &kidstatus, WNOHANG | WUNTRACED )) > 0)
    {
        if ( WIFSTOPPED(kidstatus) ) /* Is this an exiting or stopped Process */
           continue;             /* If a STOPPED Process go to next waitpid() */
 
        /* if non-zero if normal termination of child */
        if (WIFEXITED( kidstatus ) != 0)
          status = WEXITSTATUS( kidstatus );
        else
          status = 0;

        /* child terminated due to an uncaught signal */
        if (WIFSIGNALED( kidstatus ) != 0)
        {
          termsig = WTERMSIG( kidstatus );
          coredump = WCOREDUMP( kidstatus );
        }
        else
        {
          coredump = termsig = 0;
        }

        DPRINT4(1,"TheGrimReaper: Child Pid: %d, Status: %d, Core Dumped: %d, Termsig: %d\n",
            	pid,status,coredump,termsig);
        if (pid == VnmrPid)
        {
           DPRINT(1,"Child is VNMR, Update doneQ, call Resume to check psgQ\n");
	   VnmrPid = 0;		/* clear pid */
	   Resume("");		/* call Resume to allow checking of psgQ */
        }
   }
   return;
}
