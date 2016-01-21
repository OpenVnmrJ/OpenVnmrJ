/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*  12-01-1994  Messages sent to the console are exclusively ASCII strings  */

/*     This version reports which experiment is activing when ACQUIRING     */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <netinet/in.h>

#include "errLogLib.h"
/* #include "hostMsgChannels.h" clashes with prochandler.h so define
    UPDTPROC_CHANNEL here also see below */
#include "hostAcqStructs.h"
#include "commfuncs.h"
#include "chanLib.h"
#include "msgQLib.h"
#include "sendAsync.h"
#include "shrexpinfo.h"
#include "shrstatinfo.h"
#include "expDoneCodes.h"
#include "expQfuncs.h"
#include "procQfuncs.h"
#include "expentrystructs.h"
#include "prochandler.h"
#include "eventHandler.h"
#include "config.h"


/************************************************************************
 * Forward Declarations
 ************************************************************************/
void rmPsgFiles(SHR_EXP_INFO ExpInfo);
static int clearAuthRecordByLevel( const int level );
int isUserOk2Access(char* username);
int isExpNumOk2Access(int expnum);

/************************************************************************
 * Declarations for routines for accounting recording
 ************************************************************************/
void bill_started(SHR_EXP_INFO);
void bill_done(SHR_EXP_INFO);

/************************************************************************
 * Declarations for routines that have no include file for us to use.
 ************************************************************************/
extern int deliverMessage(char *interface, char *msg); /*   commfuncs.c */
extern int resetExpproc(void);                         /*   commfuncs.c */
extern int isExpActive(void);                          /*   commfuncs.c */
extern int connectChan(int chan_no);                   /*   commfuncs.c */
extern int setStatInQue(long numInQue);                /*   statfuncs.c */
extern int setStatExpTime(double duration);            /*   statfuncs.c */
extern int getStatOpsCmpltFlags(void);                 /*   statfuncs.c */
extern int mapInExp(ExpEntryInfo *expid);              /*    expfuncs.c */
extern int mapOutExp(ExpEntryInfo *expid);             /*    expfuncs.c */
extern int locateConfFile(char *confFile);             /*  conhandler.c */
extern int sigInfoproc(void);                          /* prochandler.c */
extern int abortSampChange(void);                      /* prochandler.c */
extern int startAutoproc(char *autodir, char *doneQ);  /* prochandler.c */
extern int startATask(int task);                       /* prochandler.c */
extern int chkTaskActive(int task);                    /* prochandler.c */
extern int activeQnoWait(int oldproc, long key,        /*  procQfuncs.c */
                         int newproc);
extern int expQshow(void);                             /*   expQfuncs.c */

extern char systemdir[];


#ifndef LINUX
extern char *strtok_r(char *, const char *, char **);
#endif

#define UPDTPROC_CHANNEL 4

#define  DELIMITER_1	' '
#define  DELIMITER_2	'\n'
#define  DELIMITER_3	','

#ifndef HOSTLEN
#define  HOSTLEN	64
#endif

#ifndef MAXPATHL
#define MAXPATHL	128
#endif

#ifndef sizeofArray
#define sizeofArray( a )  (sizeof( a ) / sizeof( (a)[ 0 ] ))
#endif

typedef struct _authRecord {
	char	userName[ HOSTLEN ];
	char	hostName[ HOSTLEN ];
	int	processID;
} authRecord;


#define NO_DOWNLOAD	0
#define DSP_DOWNLOAD	1

struct _downloadInfo {
	int	type[4];
} downloadInfo = { { NO_DOWNLOAD, NO_DOWNLOAD, NO_DOWNLOAD, NO_DOWNLOAD } };

char *dspaddrs[4] = { "804000", "884000", "904000", "984000" }; /* hex values with 0xfa stripped off */
int  dspapaddr[4] = { 0xe86, 0xea6, 0xec6, 0xee6 };

ExpEntryInfo ActiveExpInfo;

/*  interactive_pid allows us to trace an interactive acquisition.
    It remains -1 during shim display.					*/

static int interactive_pid = -1;

static int abort_in_progress = 0;
static int lastExp = 0;
static char lastUser[256];


/*  These program assist with console access authorization records  */

/*  Use this program whenever you expect a return interface.  It returns NULL
    when you have no interface, even if the message has additional parameters.	*/

static char *
checkForNoInterface( char *returnInterface )
{
	int	rlen;

	if (returnInterface == NULL)
	  return( NULL );
	rlen = strlen( returnInterface );
	if (rlen < 1)
	  return( NULL );
	if (rlen == 1 && *returnInterface == DELIMITER_1)
	  return( NULL );

	return( returnInterface );
}


/*  Is an authorization record active?  */

static int
isAuthActive( authRecord *authRec )
{
	int	ival;

	if ((int)strlen( &authRec->userName[ 0 ] ) > 0 && authRec->processID > 0) {
		ival = kill( authRec->processID, 0 );
		if (ival == 0 || errno == EPERM)
		  return( 1 );
	}

	return( 0 );
}

/*  Return 0 if identical, 1 if different  */

static int
compareAuth( const authRecord *authRec_1, const authRecord *authRec_2 )
{
	int	different, l1, l2;

/*  Two NULL records are identical.
    Any NULL record is different from any other record.  */

	if (authRec_1 == NULL && authRec_2 == NULL)
	  return( 0 );
	else if (authRec_1 == NULL || authRec_2 == NULL)
	  return( 1 );

/*  Compare user names  */

	l1 = strlen( &authRec_1->userName[ 0 ] );
	l2 = strlen( &authRec_2->userName[ 0 ] );
	if (l1 > 0 && l2 > 0) {
		different = strcmp( &authRec_1->userName[ 0 ], &authRec_2->userName[ 0 ] );
	}
	else {
		different = (l1 > 0 || l2 > 0);
	}

	if (different)
	  return( 1 );

/*  Compare host names  */

	l1 = strlen( &authRec_1->hostName[ 0 ] );
	l2 = strlen( &authRec_2->hostName[ 0 ] );
	if (l1 > 0 && l2 > 0) {
		different = strcmp( &authRec_1->hostName[ 0 ], &authRec_2->hostName[ 0 ] );
	}
	else {
		different = (l1 > 0 || l2 > 0);
	}

	if (different)
	  return( 1 );

/*  Compare process ID's  */

	if (authRec_1->processID != authRec_2->processID)
	  return( 2 );
	else
	  return( 0 );
}

static void
buildAuthRecord( authRecord *authRec, char *userName, char *hostName, int processID )
{
	int	l, limitIndex;

	memset( (char *) authRec, 0, sizeof( *authRec ) );

	if (userName != NULL) {
		l = strlen( userName );
		if (l > 0) {
			limitIndex = sizeof( authRec->userName ) - 1;
			if ( (int)strlen( userName ) > limitIndex) {
				strncpy( &authRec->userName[ 0 ], userName, limitIndex );
				authRec->userName[ limitIndex ] = '\0';
			}
			else
			  strcpy( &authRec->userName[ 0 ], userName );
		}
	}

	if (hostName != NULL) {
		l = strlen( hostName );
		if (l > 0) {
			limitIndex = sizeof( authRec->hostName ) - 1;
			if ( (int)strlen( hostName ) > limitIndex) {
				strncpy( &authRec->hostName[ 0 ], hostName, limitIndex );
				authRec->hostName[ limitIndex ] = '\0';
			}
			else
			  strcpy( &authRec->hostName[ 0 ], userName );
		}
	}

	authRec->processID = processID;
}

static void
clearAuthRecord( authRecord *authRec )
{
	memset( (char *) authRec, 0, sizeof( *authRec ) );
}


static void
parseAuthInfo( char *authString, char **userName, char **hostName, int *processID )
{

/*  authString is input.  Its characters are assumed to be writeable.
    userName, hostName, processID are output */

	char	*tmpaddr;

	*userName = authString;
	*hostName = NULL;
	*processID = 0;

	tmpaddr = strchr( authString, DELIMITER_3 );
	if (tmpaddr == NULL)
	  return;

	*tmpaddr = '\0';
	tmpaddr++;
	*hostName = tmpaddr;

	tmpaddr = strchr( tmpaddr, DELIMITER_3 );
	if (tmpaddr == NULL)
	  return;

	*tmpaddr = '\0';
	tmpaddr++;
	*processID = atoi( tmpaddr );
}

/*  End of programs to assist with console access authorization records  */

struct _vnmrAddr {
                   char address[128];
                   struct _vnmrAddr *next;
                 };

typedef struct _vnmrAddr vnmrAddr;

static vnmrAddr *addrList = NULL;

static void addVnmrAddr(const char *addr)
{
   vnmrAddr *p;
   int i = 0;

   DPRINT1( 1, "addVnmrAddr: %s\n",addr );
   p = addrList;
   while (p)
   {
      i++;
      if (p->address[0] == '\0')
      {
         strcpy(p->address,addr);
         break;
      }
      p = p->next;
   }
   if ( ! p)
   {
      vnmrAddr *newP;
      newP = (vnmrAddr *)malloc(sizeof(vnmrAddr));
      strcpy(newP->address,addr);
      if ( ! addrList )
         addrList = newP;
      else
      {
         p = addrList;
         while (p && p->next)
            p = p->next;
         p->next = newP;
      }
      newP->next = NULL;
   }
}

static void rmVnmrAddr(const char *addr)
{
   vnmrAddr *p;

   DPRINT1( 1, "rmVnmrAddr:  %s\n",addr );
   p = addrList;
   while (p)
   {
      if ( ! strcmp(p->address, addr) )
      {
         p->address[0] = '\0';
      }
      p = p->next;
   }
}

#ifdef XXX
static void showVnmrAddr()
{
   vnmrAddr *p;
   int i = 0;

   p = addrList;
   while (p)
   {
      i++;
      if (p->address[0] == '\0')
      {
         DPRINT1(1,"Address %d: avaiable\n",i);
      }
      else
      {
         DPRINT2(1,"Address %d: %s\n",i, p->address);
      }
      p = p->next;
   }
}
#endif

int autoqMsg(char *argv)
{
    char *returnInterface, *token;

#ifdef MSG_DEBUG
    DPRINT( 1, "autoqMsg called\n" );
#endif
    returnInterface = strtok( NULL, "\n" );

    if (returnInterface == NULL) {
        errLogRet(ErrLogOp,debugInfo,"ipccontst: No return interface\n" );
        return( -1 );
    }
    if ((int) strlen( returnInterface ) < 1) {
        errLogRet(ErrLogOp,debugInfo,"ipccontst: 0-length return interface\n" );
        return( -1 );
    }
    token = strtok( NULL, " " );
    if ( ! strcmp(token,"recvmsg"))
    {
       token = strtok( NULL, "\n" );
       if ( ! strcmp(token,"on"))
       {
          DPRINT1( 1, "autoqMsg add address: %s\n",returnInterface );
          rmVnmrAddr(returnInterface);
          addVnmrAddr(returnInterface);
       }
       else if ( ! strcmp(token,"off"))
       {
          DPRINT1( 1, "autoqMsg rm  address: %s\n",returnInterface );
          rmVnmrAddr(returnInterface);
       }
    }
    else if ( ! strcmp(token,"sendmsg"))
    {
       vnmrAddr *p;
       int stat;

       token = strtok( NULL, "\n" );
       DPRINT1( 1, "send msg \"'%s\"\n",token);
       p = addrList;
       while (p)
       {
          if (p->address[0] != '\0')
          {
             DPRINT1( 1, "    to %s\n",p->address );
             stat = deliverMessage( p->address, token );
             /* listener is no longer available */
             if (stat == -2)
             {
                rmVnmrAddr(p->address);
             }
          }
          p = p->next;
       }
    }
#ifdef XXX
    showVnmrAddr();
#endif
    return(0);
}



/*
 ****************************************************
   loop-back test for Vnmr: Vnmr->Expproc->Vnmr
   ipctst strToReturn,Host,port,pid

			Author Greg Brissey 10-6-94
 ****************************************************
*/
int ipctst(char *argv)
{
	char	*returnInterface;
	char	*token;

#ifdef MSG_DEBUG
	DPRINT( 1, "ipc test called in Nessie expproc\n" );
#endif
	returnInterface = strtok( NULL, "\n" );

	if (returnInterface == NULL) {
        	errLogRet(ErrLogOp,debugInfo,"ipctst: No return interface\n" );
		return( -1 );
	}
	if ((int) strlen( returnInterface ) < 1) {
        	errLogRet(ErrLogOp,debugInfo,"ipctst: 0-length return interface\n" );
		return( -1 );
	}

	while ((token = strtok( NULL, "\n" )) != NULL) {
		deliverMessage( returnInterface, token );
	}
	return (0);
}

/*
   loop-back test for Vnmr: Vnmr->Expproc->Console->Expproc->Vnmr
   ipccontst strToReturn,Host,port,pid

   Send Console cmd 'ECHO' with Expproc cmd 'PARSE' with 'ipctst .....'
   as the command for Expproc to parse.

			Author Greg Brissey 10-6-94
*/
int ipccontst(char *argv)
{
    char  Msge[512];
    char *returnInterface, *token;

#ifdef MSG_DEBUG
    DPRINT( 1, "ipc console test called in Nessie expproc\n" );
#endif
    returnInterface = strtok( NULL, "\n" );

    if (returnInterface == NULL) {
        errLogRet(ErrLogOp,debugInfo,"ipccontst: No return interface\n" );
	return( -1 );
    }
    if ((int) strlen( returnInterface ) < 1) {
        errLogRet(ErrLogOp,debugInfo,"ipccontst: 0-length return interface\n" );
	return( -1 );
    }

    if (consoleConn())
    {
        while ((token = strtok( NULL, "\n" )) != NULL) {
	    sprintf( &Msge[ 0 ], "%d%c%d%c%s%c%s%c%s",
			   ECHO, DELIMITER_2, PARSE, DELIMITER_2,
			  "ipctst", DELIMITER_1, returnInterface, DELIMITER_2, token
	    );
            writeChannel(chanId, &Msge[ 0 ], CONSOLE_MSGE_SIZE);
	}

	/*  The monitor task receives the message first in the console.  It
	    removes the ECHO token and the first DELIMITER_2.  The remainder
	    of the message is sent back to this process.  See conhandler.c.
	    It removes the PARSE token and the second DELIMITER_2.  It then
	    calls the parser with the rest of the message (that's why there
	    is the PARSE token).  Notice now the first token is "ipctst".
	    See the ipctst program, above, for what happens next.		*/
    }
    else
    {
      strcpy(Msge,"\nwrite('line3','Connection to Console not Established Yet.')\n");
      deliverMessage( returnInterface, Msge );
    }
    return (0);
}

int chkExpQ(char *argstr)
{
  MSG_Q_ID pRcvMsgQ, pSndMsgQ;
  int sendProcMsg(int elem, int ctval, int donecode, int errorcode);
  char msge[CONSOLE_MSGE_SIZE];
  char recvmsg[128];
  char sndmsge[128];
  char msgestr[256];
  char ExpN[25];
  int totalq;
  int JpsgFlag; 
  long nAcodes, nFIDs,startFID;

  char ActiveId[256];
  int  activetype,fgbg,apid,dCode;
  extern char *proctypeName(int);

  /* Before checking besure we have a connection to console */
  DPRINT(1,"ChkQ.\n");

  if (consoleConn())
  {
     if (DebugLevel > 0)
         expQshow();
     totalq = expQentries();
     /* DPRINT1(-1,"%d Exp. Q entries\n",totalq); */
     if ((int) strlen(ActiveExpInfo.ExpId) > 1)
     {
        setStatInQue( totalq );
        return(-1);
     }

     /* check active processing Q, if processing is WEXP_WAIT then don't start next Exp. */

     DPRINT1(1,"chkExpQ: %d Processing Q entries\n",procQentries());
     DPRINT1(1,"chkExpQ: %d Active Processing Q entries\n",activeProcQentries());
     if ( activeQget(&activetype, ActiveId, &fgbg, &apid, &dCode ) != -1)
     {
        DPRINT4(1,"Processing for: '%s', pid: %d, type: %s, DoneCode: %d \n",
                 ActiveId, apid, proctypeName(activetype), dCode);
        if (activetype == WEXP_WAIT)
        {
           DPRINT(1,"chkExpQ: wait on processing, don't start any Q'd Exp.\n");
	   return(0);   /* active processing is au('wait') don't start any queued Exp. yet */
        }
     }

     if (expQget(&ActiveExpInfo.ExpPriority, ActiveExpInfo.ExpId) != -1)
     { 
	if (mapInExp(&ActiveExpInfo) != -1)
        {

	    /* Mark the Experiment as Started */
	   ActiveExpInfo.ExpInfo->ExpState = EXPSTATE_STARTED;
           bill_started(ActiveExpInfo.ExpInfo); /* mark time for bills */

	   pRcvMsgQ = openMsgQ("Recvproc");
	   pSndMsgQ = openMsgQ("Sendproc");
#ifdef XXX
           if ( (pRcvMsgQ->MsgQDbmEntry.pidActive > 0) && 
		(pSndMsgQ->MsgQDbmEntry.pidActive > 0) )
           {
#endif
	      sprintf(recvmsg,"recv %s",ActiveExpInfo.ExpId);
  	      DPRINT1(1,"Send Recvproc: '%s'\n",recvmsg);
	      sendMsgQ(pRcvMsgQ,recvmsg,strlen(recvmsg),MSGQ_NORMAL,
				WAIT_FOREVER);

	      sprintf(sndmsge,"send %s %s",ActiveExpInfo.ExpId,
				ActiveExpInfo.ExpInfo->AcqBaseBufName);
  	      DPRINT1(1,"Send Sendproc: '%s'\n",sndmsge);
	      sendMsgQ(pSndMsgQ,sndmsge,strlen(sndmsge),MSGQ_NORMAL,
				WAIT_FOREVER);

   	     /*
   	     // Used for RA, if Jpsg then nAcodes will be 1, even if nFIDs is greater than 1
   	     // If nFIDs is one then Std PSG & Jpsg are equivent in sending Acodes to console
   	     */
   	     nFIDs = ActiveExpInfo.ExpInfo->ArrayDim;	/* Total Elements of this Exp. */
	     nAcodes = ActiveExpInfo.ExpInfo->NumAcodes;
	     startFID = ActiveExpInfo.ExpInfo->Celem;
   	     JpsgFlag = ((nAcodes == 1) && (nFIDs > 1)) ? 1 : 0;

             /* JpsgFlag = ActiveExpInfo.ExpInfo->PSGident; */

   	     DPRINT4(1,"chkExpQ: # Acodes: %d, # FIDs: %d,  Jpsg flag: %d, startAcode: %d\n",
			     nAcodes,nFIDs,JpsgFlag,startFID);
   	     DPRINT1(1,"chkExpQ: ActiveExpInfo.ExpInfo->PSGident = %d (100-JPSG, 1-C PSG) \n",
			ActiveExpInfo.ExpInfo->PSGident);

   	     if (JpsgFlag == 1)
      	          startFID = 0;

   	     DPRINT1(1,"New startAcode value: %d\n",startFID);

              sprintf(&msge[0],"%d%cPARSE_ACODE %s, %ld, %ld, %ld;",
		APARSER, DELIMITER_2,
		ActiveExpInfo.ExpInfo->AcqBaseBufName,
		ActiveExpInfo.ExpInfo->NumAcodes,
		ActiveExpInfo.ExpInfo->NumTables,
		startFID);
		/* ActiveExpInfo.ExpInfo->Celem); */

  	      DPRINT1(1,"Send Msge to  Console Parser: '%s'.\n",&msge[0]);
              writeChannel(chanId,&msge[ 0 ],CONSOLE_MSGE_SIZE);

	      activeExpQadd(ActiveExpInfo.ExpPriority, ActiveExpInfo.ExpId, 
			ActiveExpInfo.ExpInfo->UserName);
              expQdelete(ActiveExpInfo.ExpPriority, ActiveExpInfo.ExpId);

              /* Update Status Block for New Active Experiment */
	      setStatExpId(ActiveExpInfo.ExpId);
	      setStatGoFlag(ActiveExpInfo.ExpInfo->GoFlag);
              setStatUserId(ActiveExpInfo.ExpInfo->UserName);
              sprintf(ExpN,"exp%d",ActiveExpInfo.ExpInfo->ExpNum);
              if (ActiveExpInfo.ExpInfo->ExpFlags & AUTOMODE_BIT)
	         setStatExpName("auto");
              else
	         setStatExpName(ExpN);
              setStatInQue(totalq - 1);
              setStatCT(ActiveExpInfo.ExpInfo->CurrentTran);
              if (ActiveExpInfo.ExpInfo->Celem > 0)
                setStatElem(ActiveExpInfo.ExpInfo->Celem);
              else
                setStatElem( 1 );
              setStatExpTime(ActiveExpInfo.ExpInfo->ExpDur);
	      /* Send Vnmr Started Experiment */
              if (ActiveExpInfo.ExpInfo->ExpFlags & AUTOMODE_BIT)
              {
                sendProcMsg(0,0,EXP_STARTED,
                    ActiveExpInfo.ExpInfo->GoFlag |
                    (ActiveExpInfo.ExpInfo->ExpFlags & RESUME_ACQ_BIT));
              }
	      else if ((int) strlen(ActiveExpInfo.ExpInfo->MachineID) > 1)
	      {
		  sprintf(msgestr,"acqstatus('%s',%d,%d)\n",
                          ExpN,EXP_STARTED, ActiveExpInfo.ExpInfo->GoFlag |
                          (ActiveExpInfo.ExpInfo->ExpFlags & RESUME_ACQ_BIT));
  	          DPRINT2(1,"Send Vnmr(%s): '%s'\n",
		  		ActiveExpInfo.ExpInfo->MachineID,
				msgestr);
		  deliverMessage( ActiveExpInfo.ExpInfo->MachineID,
				  msgestr );
	      }
              sigInfoproc();

#ifdef XXX
	   }
	   else
	   {
              if (pRcvMsgQ->MsgQDbmEntry.pidActive < 1)
	      {
      	        errLogRet(ErrLogOp,debugInfo,
	       "chkExpQ: Recvproc not running No Experiment Started for: %s.\n",
			ActiveExpInfo.ExpId);
	      }
	      if (pSndMsgQ->MsgQDbmEntry.pidActive < 1)
	      {
      	        errLogRet(ErrLogOp,debugInfo,
	       "chkExpQ: Sendproc not running No Experiment Started for: %s.\n",
			ActiveExpInfo.ExpId);
	      }
	      /* delete from Q, what action now */
           }
#endif
	   closeMsgQ(pRcvMsgQ);
	   closeMsgQ(pSndMsgQ);
        }
        else
        {
      	   errLogRet(ErrLogOp,debugInfo,
	   "chkExpQ: mapInExp failed to Map In: %s.\n",ActiveExpInfo.ExpId);
	   mapOutExp(&ActiveExpInfo);
        }
     }
     else
     {
	DPRINT(1,"chkExpQ: No Experiments in Queue.\n");
        setStatInQue(totalq);
     }
   } 
   else
   {
      errLogRet(ErrLogOp,debugInfo,
	  "chkExpQ: Console connection not established yet.\n");
   }
   return (0);
}

#define  NONE		0
#define  SHIM		1
#define  ACQUIRE	2
#define  ERROR		-1
#define  NUMBER_OF_LEVELS	2

/* message from recvproc that experiment has completed acquisition */
int ExpAcqDone(char *args)
{
  char *expId;

  expId = (char*) strtok(NULL,",");
  DPRINT2(1,"ExpAcqDone: '%s' vs active '%s'\n",expId,ActiveExpInfo.ExpId);
  if (strcmp(expId,ActiveExpInfo.ExpId) == 0)
  {
     /* save last exp and user; it may be needed by qQuery */
     lastExp = ActiveExpInfo.ExpInfo->ExpNum;
     strcpy(lastUser,ActiveExpInfo.ExpInfo->UserName);
     /* Exp. Done delete PSG files */
     rmPsgFiles(ActiveExpInfo.ExpInfo);

     if (mapOutExp(&ActiveExpInfo) == -1)
     {
       errLogRet(ErrLogOp,debugInfo,"ExpAcqDone: mapOutExp: %s failed.\n",
		   ActiveExpInfo.ExpId);
     }

     clearAuthRecordByLevel( ACQUIRE );
     setStatExpId("");
     setStatGoFlag(-1);	/* No Go,Su,etc acquiring */
     setStatUserId("");
     setStatExpName("");
  }
  else
  {
    int nq,prior;
    char expidstr[1024];
    errLogRet(ErrLogOp,debugInfo,"ExpAcqDone: Ids don't match: '%s' vs. '%s' .\n",
		expId,ActiveExpInfo.ExpId);
    nq = activeExpQentries();
    errLogRet(ErrLogOp,debugInfo,"ExpAcqDone: activeQentries: %d\n",nq);
	activeExpQget(&prior, expidstr);
    if (nq > 0)
    {
	activeExpQget(&prior, expidstr);
        errLogRet(ErrLogOp,debugInfo,"ExpAcqDone: ActiveQ Entry: pri: %d, idstr: '%s'\n",
	prior,expidstr);
    }
    clearAuthRecordByLevel( ACQUIRE );
    setStatExpId("");
    setStatGoFlag(-1);	/* No Go,Su,etc acquiring */
    setStatUserId("");
    setStatExpName("");
  }
  chkExpQ("");
  return (0);
}

/*
 * add2QHead
 * adds to the head of the Experiment Queue
 * was original in ncomm socket.c, however for Windoze cygwin/SFU usage 
 * we moved it within the Proc Domain out of PSG
 *
 *   Author greg Brissey  3/21/2006
 */
int add2QHead()
{
  char *filename;
  char *info;

  filename = (char*) strtok(NULL,",");
  info = (char*) strtok(NULL,",");
  DPRINT2(1,"add2QHead: filename: '%s', info: '%s'\n",filename,info);
  expQaddToHead(NORMALPRIO, filename, info);
  chkExpQ("");
  return (0);
}

/*
 * add2QTail
 * adds to the tail of the Experiment Queue
 * was original in ncomm socket.c, however for Windoze cygwin/SFU usage 
 * we moved it within the Proc Domain out of PSG
 *
 *   Author greg Brissey  3/21/2006
 */
int add2QTail()
{
  char *filename;
  char *info;

  filename = (char*) strtok(NULL,",");
  info = (char*) strtok(NULL,",");

  DPRINT2(1,"add2QTail: filename: '%s', info: '%s'\n",filename,info);
  expQaddToTail(NORMALPRIO, filename, info);
  chkExpQ("");
  return (0);
}

/*-----------------------------------------------------------------------
* vnmrProcAck - relays the fgcmplt message onto Procproc, this message
*  		either A. Done FG Processing or
*		       B. Can't Do It in FG, You Do It in BG
*-------------------------------------------------------------------*/
int vnmrProcAck(char *args)
{
        char     proccmd[256];
	char	*returnInterface;
	char	*msge;
	DPRINT( 1, "Vnmr Procproc Ack, send on to Procproc\n" );
	returnInterface = strtok( NULL, "\n" );
	msge = strtok( NULL, "\n" ); 
        sprintf(proccmd,"fgcmplt %s\n%s",returnInterface,msge);
	deliverMessage( "Procproc", proccmd );
	return (0);
}
/*-----------------------------------------------------------------------
* wallUsers - executes the UNIX wall -a command with given message
*				Author: Greg Brissey
*-------------------------------------------------------------------*/
int wallUsers(char *args)
{
   char *msge;
   msge = strtok(NULL,"\n");

   DPRINT1(1,"wallUsers: '%s'\n",msge);
   wallMsge(msge); 
   return (0);
}

/*
 *  reset state back to idle 
 */
void resetState()
{
  /* Experiment mmap in then release it and update status */
  DPRINT1(1,"resetState: ActiveExpInfo.ExpId - '%s'\n",ActiveExpInfo.ExpId);
  if ((int) strlen(ActiveExpInfo.ExpId) > 1)
  {
   rmPsgFiles(ActiveExpInfo.ExpInfo);
   if (mapOutExp(&ActiveExpInfo) == -1)
   {
       errLogRet(ErrLogOp,debugInfo,"resetState: mapOutExp: %s failed.\n",
		   ActiveExpInfo.ExpId);
   }
   setStatGoFlag(-1);	/* No Go,Su,etc acquiring */
   setStatExpId("");
   setStatUserId("");
   setStatExpName("");

/*  Both interactive and non-interactive acquisitions reserve the console for
    acquire.  For non-interactive, see acqhwcmd.c and the way ACCESSQUERY is
    defined in socket.c.  For interactive, we once cleared the authorization
    record in stopInteract, but now we wait and only clear it when the console
    tells the host it has completed.						*/

   clearAuthRecordByLevel( ACQUIRE );
   interactive_pid = -1;
   abort_in_progress = 0;

   /* may need to send reset command to sendproc & recvproc processes */
 }
}

/*
*  get lock signal.
*  top entry on the experiment queue is expected to be a lock FID experiment
*
*  stub program - parses its message, takes item off the queue, but then
*                 just deletes that item with no further action
*
*  for now this program is obsolete
*/

int getLockS()
{
    errLogRet(ErrLogOp,debugInfo,
		         "getLockS: W A R N I N G,   Function is obsolete.\n");
    return( -1 );
#ifdef XXXX
	if (consoleConn() == 0) {
		errLogRet(ErrLogOp,debugInfo,
		         "getLockS: Console connection not established yet.\n");
		return( -1 );
	}

	if ((int) strlen(ActiveExpInfo.ExpId) > 1) {
		errLogRet(ErrLogOp,debugInfo,
			 "getLockS: Exp: '%s' still active.\n",
			  ActiveExpInfo.ExpId);
		return(-1);
	}

	if (expQget(&ActiveExpInfo.ExpPriority, ActiveExpInfo.ExpId) == -1) {
		DPRINT(1,"getLockS: No Experiments in Queue.\n");
		return(-1);
	}

	if (mapInExp(&ActiveExpInfo) == -1) {
		DPRINT(1,"getLockS: Can't map in current experiment.\n");
		return(-1);
	}

	DPRINT1(2,"getLockS: experiment priority: %d\n", ActiveExpInfo.ExpPriority );
	DPRINT1(2,"getLockS: experiment ID: %s\n", ActiveExpInfo.ExpId );

	expQdelete(ActiveExpInfo.ExpPriority, ActiveExpInfo.ExpId);

	if (mapOutExp(&ActiveExpInfo) == -1) {
		errLogRet(ErrLogOp,debugInfo,
			 "ExpAcqDone: mapOutExp: %s failed.\n",
			  ActiveExpInfo.ExpId);
	}
	return( 0 );
#endif
}


int rebootConsole(char *args)
{
	char	*returnInterface, *userName;
	char	 msge[CONSOLE_MSGE_SIZE];

	returnInterface = strtok( NULL, "\n" );
	userName = strtok( NULL, "\n" );

	DPRINT2(1,"User %s wants to reboot the console using %s to acknowledge\n",
                   userName, returnInterface);

/*  At this time everyone gets to do it, without restriction or checks  */

	sprintf( &msge[ 0 ], "%d%c", ABORTALLACQS, DELIMITER_2 );
	writeChannel(chanId, &msge[ 0 ], CONSOLE_MSGE_SIZE);

/*  If the spectrometer is idle, Expproc would detect the console-reboot by the
    absense of HeartBeat.  See commfuncs.c.  If an acquisition was in progress
    though, this HeartBeat is itself absent and Expproc had no way of detecting
    that the console had rebooted.  Se we go ahead and reset Expproc (and the
    other proc's) now.    May 1997						*/

	resetExpproc();
	return (0);
}

/* abort with no processing  */
int acqHalt2(char *args)
{
   char *returnInterface, *userName, *expName;
   char	 msge[CONSOLE_MSGE_SIZE];
   int   reply_ok;

   returnInterface = strtok( NULL, "\n" );
   userName = strtok( NULL, " \n" );
   expName = strtok( NULL, " \n" );

   DPRINT3(1, "User %s wants to halt experiment %s using %s to acknowledge\n",
	 userName, expName, returnInterface);

   reply_ok = ( (returnInterface != NULL) &&
                 ( (int) strlen(returnInterface) > 1) );
   if (isExpActive() == 0) 
   {
      if (reply_ok)
      {
	sprintf(msge, "write('error','No Experiment Active.')\n");
	deliverMessage( returnInterface, msge );
      }
      return 0;
   }

   if (isUserOk2Access(userName) == 0) 
   {
      if (reply_ok)
      {
        sprintf(msge,
                "write('error','Halt denied, %s already has Exp. present.')\n",
				userName);
	deliverMessage( returnInterface, msge );
      }
      return 0;
   }

   if (abort_in_progress)
   {
      if (reply_ok)
      {
        sprintf(msge, "write('error','Halt Acquisition already in progress')\n");
	deliverMessage( returnInterface, msge );
      }
      return 0;
   }

   /* Setting ProcMask = 0 turns off processing */
   ActiveExpInfo.ExpInfo->ProcMask = 0;

   abort_in_progress = 131071;
   memset( &msge[ 0 ], 0, sizeof( msge ) );
   sprintf( &msge[ 0 ], "%d%c", HALTACQ, DELIMITER_2 );
   writeChannel(chanId, &msge[ 0 ], CONSOLE_MSGE_SIZE);

   /* we set the status to idle here even before the halt is complete,
      this is to allow go to queue another au/go right after a halt
   */
   setStatGoFlag(-1);	/* No Go,Su,etc acquiring */
   return (0);
}

/* abort but provides Wexp processing  */
int acqHalt(char *args)
{
   char *returnInterface, *userName, *expName;
   char	 msge[CONSOLE_MSGE_SIZE];
   int   reply_ok;

   returnInterface = strtok( NULL, "\n" );
   userName = strtok( NULL, " \n" );
   expName = strtok( NULL, " \n" );

   DPRINT3(1, "User %s wants to halt experiment %s using %s to acknowledge\n",
	 userName, expName, returnInterface);

   reply_ok = ( (returnInterface != NULL) &&
                 ( (int) strlen(returnInterface) > 1) );
   if (isExpActive() == 0) 
   {
      if (reply_ok)
      {
	sprintf(msge, "write('error','No Experiment Active.')\n");
	deliverMessage( returnInterface, msge );
      }
      return 0;
   }

   if (isUserOk2Access(userName) == 0) 
   {
      if (reply_ok)
      {
        sprintf(msge,
                "write('error','Halt denied, %s already has Exp. present.')\n",
				userName);
	deliverMessage( returnInterface, msge );
      }
      return 0;
   }

   if (abort_in_progress)
   {
      if (reply_ok)
      {
        sprintf(msge, "write('error','Halt Acquisition already in progress')\n");
	deliverMessage( returnInterface, msge );
      }
      return 0;
   }

   abort_in_progress = 131071;
   memset( &msge[ 0 ], 0, sizeof( msge ) );
   sprintf( &msge[ 0 ], "%d%c", HALTACQ, DELIMITER_2 );
   writeChannel(chanId, &msge[ 0 ], CONSOLE_MSGE_SIZE);

   /* we set the status to idle here even before the halt is complete,
      this is to allow go to queue another au/go right after a halt
   */
   setStatGoFlag(-1);	/* No Go,Su,etc acquiring */
   return (0);
}

/* abort Werror processing  */
int acqAbort(char *args)
{
   char	*returnInterface, *userName, *expName;
   char	 msge[CONSOLE_MSGE_SIZE];
   int   reply_ok;


   returnInterface = strtok( NULL, "\n" );
   userName = strtok( NULL, " \n" );
   expName = strtok( NULL, " \n" );

   DPRINT3(1,"User %s wants to abort experiment %s using %s to acknowledge\n",
      	userName, expName, returnInterface);

   reply_ok = ( (returnInterface != NULL) &&
                 ( (int) strlen(returnInterface) > 1) );
   if (isExpActive() == 0) 
   {
      sprintf(msge,"(umask 0; cat /dev/null > %s/acqqueue/psg_abort)\n",systemdir);
      system(msge);
      if (reply_ok)
      {
	sprintf(msge, "write('error','No Experiment Active.')\n");
	deliverMessage( returnInterface, msge );
      }
      return 0;
   }

   if (interactive_pid >= 0) {
      if (reply_ok)
      {
	sprintf(msge, "write('error','No Abort Acq when system is in interactive mode.')\n");
	deliverMessage( returnInterface, msge );
      }
      return 0;
   }

   if (isUserOk2Access(userName) == 0) 
   {
      if (reply_ok)
      {
        sprintf(msge,
                "write('error','Abort denied, %s already has Exp. present.')\n",
				userName);
	deliverMessage( returnInterface, msge );
      }
      return 0;
   }

   /* if (abort_in_progress) */
   if (ActiveExpInfo.ExpInfo->ExpState == EXPSTATE_ABORTED)
   {
      ActiveExpInfo.ExpInfo->ExpState = EXPSTATE_ABORTED;
      if (reply_ok)
      {
        sprintf(msge, "write('error','Abort Acquisition already in progress')\n");
	deliverMessage( returnInterface, msge );
      }
      return 0;
   }

   abort_in_progress = 131071;
   memset( &msge[ 0 ], 0, sizeof( msge ) );
   sprintf( &msge[ 0 ], "%d%c", ABORTACQ, DELIMITER_2 );
   writeChannel(chanId, &msge[ 0 ], CONSOLE_MSGE_SIZE);
   abortSampChange();  /* send abort to roboproc if running */

   /* we set the status to idle here even before the abort is complete,
      this is to allow go to queue another au/go right after an abort.
      This solves the problem were an aa followed by an au within a macro
	(react) resulted in the au not being allowed causing the react
	macro to fail sometimes...
   */
   setStatGoFlag(-1);	/* No Go,Su,etc acquiring */
   return (0);
}

int acqDebug(char *args)
{
   char	*returnInterface, *tmpstr;
   char	 msge[CONSOLE_MSGE_SIZE];
   char	 procmsge[128];
   int dlevel;
   MSG_Q_ID pMsgQ; 

   returnInterface = strtok( NULL, "\n" );
   tmpstr = strtok( NULL, "\n" );
   dlevel = atoi(tmpstr);	/* debug level */

   /* debug level 1-9 for Proc Family, 10-19 console */
   if (dlevel > 9)
   {
     dlevel -= 10;
     memset( &msge[ 0 ], 0, sizeof( msge ) );
     sprintf( &msge[ 0 ], "%d%c%d", ACQDEBUG, DELIMITER_2, dlevel);
     writeChannel(chanId, &msge[ 0 ], CONSOLE_MSGE_SIZE);
   }
   else
   {
     sprintf(procmsge,"debug %d",dlevel);
     pMsgQ = openMsgQ("Atproc");
     if ( pMsgQ != NULL)
     {
  	DPRINT1(1,"Send Atproc: '%s'\n",procmsge);
	sendMsgQ(pMsgQ,procmsge,strlen(procmsge),MSGQ_NORMAL,
				WAIT_FOREVER);
        closeMsgQ(pMsgQ);
     }
     pMsgQ = openMsgQ("Autoproc");
     if ( pMsgQ != NULL)
     {
  	DPRINT1(1,"Send Autoproc: '%s'\n",procmsge);
	sendMsgQ(pMsgQ,procmsge,strlen(procmsge),MSGQ_NORMAL,
				WAIT_FOREVER);
        closeMsgQ(pMsgQ);
     }
     pMsgQ = openMsgQ("Roboproc");
     if ( pMsgQ != NULL)
     {
  	DPRINT1(1,"Send Roboproc: '%s'\n",procmsge);
	sendMsgQ(pMsgQ,procmsge,strlen(procmsge),MSGQ_NORMAL,
				WAIT_FOREVER);
        closeMsgQ(pMsgQ);
     }
     pMsgQ = openMsgQ("Recvproc");
     if ( pMsgQ != NULL)
     {
  	DPRINT1(1,"Send Recvproc: '%s'\n",procmsge);
	sendMsgQ(pMsgQ,procmsge,strlen(procmsge),MSGQ_NORMAL,
				WAIT_FOREVER);
        closeMsgQ(pMsgQ);
     }
     pMsgQ = openMsgQ("Sendproc");
     if ( pMsgQ != NULL)
     {
  	DPRINT1(1,"Send Sendproc: '%s'\n",procmsge);
	sendMsgQ(pMsgQ,procmsge,strlen(procmsge),MSGQ_NORMAL,
				WAIT_FOREVER);
        closeMsgQ(pMsgQ);
     }
     pMsgQ = openMsgQ("Procproc");
     if ( pMsgQ != NULL)
     {
  	DPRINT1(1,"Send Procproc: '%s'\n",procmsge);
	sendMsgQ(pMsgQ,procmsge,strlen(procmsge),MSGQ_NORMAL,
				WAIT_FOREVER);
        closeMsgQ(pMsgQ);
     }
     pMsgQ = openMsgQ("Expproc");
     if ( pMsgQ != NULL)
     {
  	DPRINT1(1,"Send Expproc: '%s'\n",procmsge);
	sendMsgQ(pMsgQ,procmsge,strlen(procmsge),MSGQ_NORMAL,
				WAIT_FOREVER);
        closeMsgQ(pMsgQ);
     }
   }
   return (0);
}
/*---------------------------------------------------------------------
* SA 
*   eos,ct,scan; eob,bs; eof,nt,fid; eoc,il;  # - eos at modulo #
+--------------------------------------------------------------------*/
#define STOP_EOS        11/* sa at end of scan */
#define STOP_EOB        12/* sa at end of bs */
#define STOP_EOF        13/* sa at end of fid */
#define STOP_EOC        14/* sa at end of interleave cycle */
/*--------------------------------------------------------------------*/
int acqStop(char *args)
{
   char	*returnInterface, *userName, *expName, *SA_Type, *SA_mod;
   char	 msge[CONSOLE_MSGE_SIZE];
   int saType,expnum,acqstopped;
   int totalq;
   unsigned long saMod,saMode;
   ExpEntryInfo SaExpInfo;

   returnInterface = strtok( NULL, "\n" );
   SA_Type = strtok( NULL, " \n" );
   SA_mod = strtok( NULL, " \n" );
   userName = strtok( NULL, " \n" );
   expName = strtok( NULL, " \n" );

   saType = atoi(SA_Type);
   saMod = atol(SA_mod);
   expnum = atoi(&expName[3]);  /* in form of exp1, exp2,exp3, etc.... */
   /* At present valid SA types: BS, FID, IL */
   DPRINT5(1,
    "User %s wants to Stop experiment %s \n 'Type: %d, Mod:%lu' using %s to acknowledge\n",
         userName, expName, saType, saMod, returnInterface);

   acqstopped = 0;
   if (isExpActive() != 0) 
   {
      DPRINT2(1,"acqStop: check active exp. User: '%s', exp: %d\n",userName,expnum);
      if ( (isUserOk2Access(userName) != 0)  && (isExpNumOk2Access(expnum) != 0))
      {

   	/* convert to type of SA for inova style console SA type */
   	switch(saType)
   	{
     	   case STOP_EOS:        /* sa at end of scan */
		saMode = CT_CMPLT;
		break;
     	   case STOP_EOB:        /* sa at end of bs */
		saMode = BS_CMPLT;
		break;
     	   case STOP_EOF:        /* sa at end of fid */
		saMode = EXP_FID_CMPLT;
		break;
     	   case STOP_EOC:        /* sa at end of interleave cycle */
		saMode = IL_CMPLT;
   		/* saMod =  ActiveExpInfo.ExpInfo->ArrayDim; */
		break;
     	   default:
		saMode = CT_CMPLT;
		saMod = 1L;
		break;
   	}
    
   	memset( &msge[ 0 ], 0, sizeof( msge ) );
   	sprintf( &msge[ 0 ], "%d%c%d %lu", STOP_ACQ, DELIMITER_2,saMode,saMod );
   	writeChannel(chanId, &msge[ 0 ], CONSOLE_MSGE_SIZE);
	acqstopped = 1;		/* stop exp in console */
        DPRINT1(1,"acqStop: SA Active Exp., Console Cmd: '%s'\n",msge);
      }
   }

   /* if there are Q entries and we haven't stop the active exp. then check the Q */
   if ( (expQentries() > 0) && (acqstopped != 1) )
   {
     DPRINT2(1,"acqStop: check Exp. Q.,  User: '%s', exp: %d\n",userName,expnum);
     if ( expQsearch(userName,&expName[3],&SaExpInfo.ExpPriority, SaExpInfo.ExpId)  )
     {
      DPRINT4(1,"acqStop: Removing from expQ user: '%s', exp: %s, priority: %d, ExpId: '%s'\n",
		userName,&expName[3],SaExpInfo.ExpPriority, SaExpInfo.ExpId);
      expQdelete(SaExpInfo.ExpPriority, SaExpInfo.ExpId);
      if (mapInExp(&SaExpInfo) != -1)
      {
          /* delete PSG files */
          rmPsgFiles(SaExpInfo.ExpInfo);

          if (mapOutExp(&SaExpInfo) == -1)
          {
            errLogRet(ErrLogOp,debugInfo,"ExpAcqDone: mapOutExp: %s failed.\n",
		   SaExpInfo.ExpId);
          }
      }
      else
      {
          errLogRet(ErrLogOp,debugInfo,"acqStop: mapInExp: %s failed.\n", SaExpInfo.ExpId);
      }

      totalq = expQentries();
      setStatInQue( totalq );
      sigInfoproc();
     }
   }
   return 0;
}

/*---------------------------------------------------------------------
* acqDequeue 
+--------------------------------------------------------------------*/
int acqDequeue(char *args)
{
   char *returnInterface, *file, *file2;
   int totalq;
   ExpEntryInfo SaExpInfo;
   int res = 0;

   returnInterface = strtok( NULL, "\n" );
   file = strtok( NULL, " \n" ); /* toss user name, etc */
   file = strtok( NULL, " \n" );
   file2 = strtok( NULL, " \n" );

   DPRINT3(1,
    "Request to dequeue %s (%s) using %s to acknowledge\n",
         file, file2, returnInterface);

   /* if there are Q entries and we haven't stop the active exp. then check the Q */
   if ( expQentries() > 0 )
   {
     sprintf(SaExpInfo.ExpId,"%s/acqqueue/%s",file2,file);
     DPRINT1(1,"acqDequeue: check Exp. Q.,  ID: '%s'\n",SaExpInfo.ExpId);
     if ( expQIdsearch(SaExpInfo.ExpId, &SaExpInfo.ExpPriority) == 1  )
     {
      DPRINT2(1,"acqDequeue: Removing from expQ  priority: %d, ExpId: '%s'\n",
                SaExpInfo.ExpPriority, SaExpInfo.ExpId);
      expQdelete(SaExpInfo.ExpPriority, SaExpInfo.ExpId);
      if (mapInExp(&SaExpInfo) != -1)
      {
          char tmpStr[512];

          strcpy(tmpStr,SaExpInfo.ExpId);
          /* delete PSG files */
          rmPsgFiles(SaExpInfo.ExpInfo);

          if (mapOutExp(&SaExpInfo) == -1)
          {
            errLogRet(ErrLogOp,debugInfo,"ExpAcqDone: mapOutExp: %s failed.\n",
                   SaExpInfo.ExpId);
          }
          DPRINT1(1,"acqDequeue: unlink  '%s'\n", tmpStr);
          unlink(tmpStr);
          sprintf(tmpStr,"rm -rf %s/acqqueue/acq/%s",file2,file);
          DPRINT1(1,"acqDequeue: exec  '%s'\n", tmpStr);
          system(tmpStr);
          res = 1;
      }
      else
      {
          errLogRet(ErrLogOp,debugInfo,"acqStop: mapInExp: %s failed.\n", SaExpInfo.ExpId);
      }

      totalq = expQentries();
      setStatInQue( totalq );
      sigInfoproc();
     }
   }
   deliverMessage( returnInterface, res ? "1" : "0" );
   return 0;
}

void acqQ(char *args)
{
}

/* change Wexp,etc. values */
int parmChg(char *args)
{
   char	*returnInterface, *userName, *token;
   int mask, on_off;

   returnInterface = strtok( NULL, "\n" );
   userName = strtok( NULL, "," );
   token = strtok( NULL, "," );
   mask = atoi( token );
   token = strtok( NULL, "," );
   on_off = atoi( token );

   DPRINT3(1,
    "User %s wants to Change processing mask %d %s\n",
         userName, mask, (on_off) ? "on" : "off");

   if (isExpActive() == 0) 
   {
      return 0;
   }

   if (isUserOk2Access(userName) == 0) 
   {
      errLogRet(ErrLogOp,debugInfo,"parmChg: user '%s' denied access\n", userName );
      return 0;
   }
   if (on_off)
      ActiveExpInfo.ExpInfo->ProcMask |=  mask;
   else
      ActiveExpInfo.ExpInfo->ProcMask &=  ~mask;
   return (0);
}

/* Go into automation mode */
/* 1st arg, return addr, 2nd automation doneQ path */
int autoMode(char *args)
{
   char *returnInterface;
   char *token;
   char autodir[256], autoDoneQ[256];

   /* AutoMode = AUTO_PENDING; */
   returnInterface = strtok( NULL, "\n" );
   token = strtok( NULL, "," );
   strncpy(autodir,token,255);
   autodir[255] = '\0';
   DPRINT2(1,"autoMode: return Addr: '%s', autodir: '%s'\n",returnInterface,
		autodir);
   sprintf(autoDoneQ,"%s/DoneQ",autodir);
   /* Used to send a resume when starting Autoproc. Now, the resume is down explicitly by Autoproc */
   if ( ! startAutoproc(autodir,autoDoneQ))  /* if not started then start it */
   {
      char ActiveId[256];
      int  activetype,fgbg,apid,dCode;
      extern char *proctypeName(int);

      deliverMessage( "Autoproc", "listen" );
      if ( activeQget(&activetype, ActiveId, &fgbg, &apid, &dCode ) != -1)
      {
/*
 *      Used to return only for EXP_WAIT processing. However, since autora are
 *      asynchronous, other processing, such as BS,
 *      could be occurring for an experiment that was started with an au('wait').
 *      Do not want to send a resume in this case.

        extern char *proctypeName(int);

        DPRINT4(1,"Checking for resume: '%s', pid: %d, type: %s, DoneCode: %d \n",
                 ActiveId, apid, proctypeName(activetype), dCode);
        if (activetype == WEXP_WAIT)
        {
           DPRINT(1,"autora: wait on processing, don't send resume.\n");
           return(0);
        }
 */
        DPRINT(1,"autora: wait on processing, don't send resume.\n");
	return(0);
      }
      deliverMessage( "Autoproc", "resume" );
   }
   return (0);
}

/* Go into non-automation mode */
int normalMode(char *args)
{
   char *returnInterface;
   char *token;

   DPRINT(1,"normalMode: telling  Autoproc to ignore resumes\n");
   if ( chkTaskActive(AUTOPROC) )
   {
      returnInterface = strtok( NULL, "\n" );
      token = strtok( NULL, "," );
      if ( ! strcmp(token,"1"))
         deliverMessage( "Autoproc", "ignore 1" );
      else
         deliverMessage( "Autoproc", "ignore 0" );
   }
   return (0);
/*
   DPRINT(1,"normalMode: killing Autoproc\n");
    killATask(AUTOPROC);
*/
}

int autoResume(char *args)
{
  MSG_Q_ID pMsgQ;
  char ActiveId[256];
  int  activetype,fgbg,apid,dCode;
  DPRINT(1,"autoResume: \n");

  if ( activeQget(&activetype, ActiveId, &fgbg, &apid, &dCode ) != -1)
  {
     if (activetype == WEXP_WAIT)
     {
         activeQnoWait(WEXP_WAIT, apid, WEXP);
     }
  }
  pMsgQ = openMsgQ("Autoproc");
  if ( pMsgQ != NULL)
  {
     DPRINT(1,"Send Resume.\n");
     sendMsgQ(pMsgQ,"resume",strlen("resume"),MSGQ_NORMAL,
		WAIT_FOREVER);
     closeMsgQ(pMsgQ);
  }
  return 0;
}

/* Return to non-automation mode */
int autoSuppend(char *args)
{
   DPRINT(1,"autoSuppend: telling  Autoproc to ignore resumes\n");
   deliverMessage( "Autoproc", "ignore" );
/*
   DPRINT(1,"autoSuppend: killing Autoproc\n");
   killATask(AUTOPROC);
*/
   return 0;
}

/* Request for autoproc to terminate */
/* 1st arg, return addr, 2nd automation doneQ path */
int autoOk2Die(char *args)
{
   MSG_Q_ID pMsgQ;
   char *token,*who;
   pid_t pid;
   extern char *markProcDead(int index);

   /* AutoMode = AUTO_PENDING; */
   /*
   token = strtok( NULL, "," );
   pid = (pid_t) atoi(token);
   DPRINT1(0,"autoOk2Die: PID: '%d'\n",pid);
   */
   who = markProcDead(AUTOPROC);
   DPRINT1(1,"Marked: '%s' as Terminated \n",who);
   pMsgQ = openMsgQ("Autoproc");
   if ( pMsgQ != NULL)
   {
     DPRINT(1,"Send Autoproc Ok2Die.\n");
     sendMsgQ(pMsgQ,"Ok2Die",strlen("Ok2Die"),MSGQ_NORMAL,
		WAIT_FOREVER);
     closeMsgQ(pMsgQ);
  }
   return 0;
}

int acqRobotCmdAck(char *args)
{
   char	 msge[CONSOLE_MSGE_SIZE];
   char *cmdReturnStatus;

   cmdReturnStatus = strtok( NULL, " \n" );
   memset( &msge[ 0 ], 0, sizeof( msge ) );
   sprintf( &msge[ 0 ], "%d%c%s", ROBO_CMD_ACK, DELIMITER_2,cmdReturnStatus );
   DPRINT1(1,"acqRobotCmdAck: '%s'\n",msge);
   writeChannel(chanId, &msge[ 0 ], CONSOLE_MSGE_SIZE);
   return 0;
}

/* readhdw */
void acqHardwareRead(char *args)
{
}


/* return experiment status */
int qQuery(char *args)
{
	char	*returnInterface;
        char    Msge[512];
        char    tmpstr[256];
        int     index;
        int     numQ;
        int     len;

	DPRINT( 1, "queue Query called in Nessie expproc\n" );
	returnInterface = strtok( NULL, "\n" );

	if (returnInterface == NULL) {
        	errLogRet(ErrLogOp,debugInfo,"qQuery: No return interface\n" );
		return( -1 );
	}
	if ((int) strlen( returnInterface ) < 1) {
        	errLogRet(ErrLogOp,debugInfo,"qQuery: 0-length return interface\n" );
		return( -1 );
	}
	DPRINT1( 1, "return address is %s\n",returnInterface );
        if ((int) strlen(ActiveExpInfo.ExpId) > 1)
        {
    /* Report back what kind of experiment is progress as well (GoFlag).  */
           if (ActiveExpInfo.ExpInfo->ExpFlags & AUTOMODE_BIT)
              sprintf(Msge,"%d auto %d\n", ActiveExpInfo.ExpInfo->ExpNum,
                  ActiveExpInfo.ExpInfo->GoFlag);
           else
              sprintf(Msge,"%d %s %d\n", ActiveExpInfo.ExpInfo->ExpNum,
                  ActiveExpInfo.ExpInfo->UserName,
                  ActiveExpInfo.ExpInfo->GoFlag);
           len = strlen(Msge);
           numQ =  expQentries();
	   DPRINT1( 1, "active exp. num in Q is %d\n",numQ );
           index = 0;
           while (index < numQ)
           {
              index++;
              if (!expQgetinfo(index, tmpstr))
              {
                 len += strlen(tmpstr) + 1;
                 if (len < 512)
                    strcat(Msge,tmpstr);
                    strcat(Msge,"\n");
              }
           }
        }
        else
        {
           char ActiveId[256];
           int  activetype,fgbg,apid,dCode;
           int automode = 0;

           if ( chkTaskActive(AUTOPROC) )
           {
              strcpy(Msge,"1 auto -1\n");
              automode = 1;
           }
           else
           {
              strcpy(Msge,"0 nobody -1\n");
           }
           if ( activeQget(&activetype, ActiveId, &fgbg, &apid, &dCode ) != -1)
           {
              if (activetype == WEXP_WAIT)
              {
                 DPRINT(1,"qQuery: Active processing with wait option\n");
                 if (automode)
                    strcpy(Msge,"3 auto -1\n");
                 else
                    sprintf(Msge,"%d %s -1\n",lastExp,lastUser);
              }
           }
           len = strlen(Msge);
           numQ =  expQentries();
	   DPRINT1( 1, "active exp. num in Q is %d\n",numQ );
           index = 0;
           while (index < numQ)
           {
              index++;
              if (!expQgetinfo(index, tmpstr))
              {
                 len += strlen(tmpstr) + 1;
                 if (len < 512)
                    strcat(Msge,tmpstr);
                    strcat(Msge,"\n");
              }
           }
        }
	DPRINT1( 1, "exp Q is %s\n",Msge );
	deliverMessage( returnInterface, Msge );
	return 0;
}

/* send acqproc access permission status back */
void accessQuery(char *args)
{
}

/* request Vnmr reconnect */
void reconRequest(char *args)
{
}

int
setStatBlockIntv(char *args)
{
	char	*returnInterface, *token;
	char	 consoleMsg[ CONSOLE_MSGE_SIZE ];
	int	 interval;

#ifdef MSG_DEBUG
	DPRINT( 1, "set stat block rate called in NDC expproc\n" );
#endif
	returnInterface = strtok( NULL, "\n" );
	token = strtok( NULL, "\n" );
	if (token == NULL) {
        	/*errLogRet(ErrLogOp,debugInfo,"setStatBlocIntv: no interval specified.\n");*/
		interval = MAX_STATUS_INTERVAL;
	}
	else
	  interval = atoi( token );

	if (interval < MIN_STATUS_INTERVAL) {
        	errLogRet(ErrLogOp,debugInfo,
			"setStatBlocIntv: interval of %d too short, using %d\n",
			 interval, MIN_STATUS_INTERVAL
		);
		interval = MIN_STATUS_INTERVAL;
	}
	else if (interval > MAX_STATUS_INTERVAL) {
        	errLogRet(ErrLogOp,debugInfo,
			"setStatBlocIntv: interval of %d too long, using %d\n",
			 interval, MAX_STATUS_INTERVAL
		);
		interval = MAX_STATUS_INTERVAL;
	}

	sprintf( &consoleMsg[ 0 ], "%d%c%d", STATINTERV, DELIMITER_2, interval );
	writeChannel( chanId, &consoleMsg[ 0 ], sizeof( consoleMsg ) );
	return 0;
}

int
getStatBlock(char *args)
{
	char	*returnInterface;
	char	 consoleMsg[ CONSOLE_MSGE_SIZE ];

#ifdef MSG_DEBUG
	DPRINT( 1, "get stat block called in NDC expproc\n" );
#endif
	returnInterface = strtok( NULL, "\n" );

	if (consoleConn() == 0) {
		deliverMessage( returnInterface, "DOWN" );
		return 0;
	}

	sprintf( &consoleMsg[ 0 ], "%d%c", GETSTATBLOCK, DELIMITER_2 );
	writeChannel( chanId, &consoleMsg[ 0 ], sizeof( consoleMsg ) );

	deliverMessage( returnInterface, "OK" );
	return 0;
}


/*  These programs assist with the console access authorization scheme.  

    You may:
        locate a record by level (SHIM or ACQUIRE)
        locate a record by authorization record
        clear a record by level (SHIM or ACQUIRE)
        clear a record by index
        verify access at a particular level.
									*/


static struct _consoleAccess {
	authRecord	authRec;
	int		level;
	int		count;
} interactiveAccess[ NUMBER_OF_LEVELS ] = {
	{ 0, NONE, 0 },
	{ 0, NONE, 0 },
};

void showAuthRecord(void)
{
   int i,active;
   printf("Authorization Records: \n");
   for( i=0; i < NUMBER_OF_LEVELS; i++)
   {
     printf("\nRecord: %d, Count: %d, Level: %s, User: '%s', Host: '%s', PID: %d\n",
	i,interactiveAccess[i].count,
      ((interactiveAccess[i].level == SHIM) ? "Shim" : "Acquire"),
	interactiveAccess[i].authRec.userName,
	interactiveAccess[i].authRec.hostName,
	interactiveAccess[i].authRec.processID);
    active = isAuthActive( &interactiveAccess[i].authRec );
    printf("This Process ID is %s\n",(active != 0) ? "Active" : "Non-Active");
   }
}

static int
locateAuthRecordByLevel( const int level )
{
	int	iter, retval;

	retval = -1;
	for (iter = 0; iter < sizeofArray( interactiveAccess ); iter++)
	  if (level == interactiveAccess[ iter ].level) {
	  	retval = iter;
		break;
	  }

	return( retval );
}

static int
locateAuthRecordByRecord( const authRecord *authRec )
{
	int	iter, retval;

	retval = -1;
	for (iter = 0; iter < sizeofArray( interactiveAccess ); iter++)
	  if (compareAuth( authRec, &interactiveAccess[ iter ].authRec ) == 0) {
	  	retval = iter;
		break;
	  }

	return( retval );
}

static int
clearAuthRecordByLevel( const int level )
{
	int	index;

	index = locateAuthRecordByLevel( level );
	if (index < 0) {
		return( -1 );
	}

	DPRINT2( 1, "clearing authorization record by level at level %s with count %d\n",
		    ((interactiveAccess[index].level == SHIM) ? "Shim" : "Acquire"),
		      interactiveAccess[index].count );
		
	clearAuthRecord( &interactiveAccess[ index ].authRec );
	interactiveAccess[ index ].count--;
	if (interactiveAccess[ index ].count < 1)
	  interactiveAccess[ index ].level = NONE;

	return( 0 );
}

static int
clearAuthRecordByIndex( const int index )
{
	if (index >= sizeofArray( interactiveAccess ) || index < 0)
	  return( -1 );

	DPRINT2( 1, "clearing authorization record by index at level %s with count %d\n",
		    ((interactiveAccess[index].level == SHIM) ? "Shim" : "Acquire"),
		      interactiveAccess[index].count );
		
	clearAuthRecord( &interactiveAccess[ index ].authRec );
	interactiveAccess[ index ].count--;
	if (interactiveAccess[ index ].count < 1)
	  interactiveAccess[ index ].level = NONE;

	return( 0 );
}

/*  Return -1 if error.  Any error return represents a flaw in the
    overall logic of the authorization scheme and should not happen.	*/

static int
storeAuthRecord( const authRecord *requesting, const int level )
{
	int	index, iter;

/*  First check if the level to be stored matches the level of any of the current
    record.   If so, then check the authorization.  If they too match, then we are
    trying to store the same record again.  That is OK.  If they do not match,
    that is bad.  We can't store this new record.

    July 1997:  If the stored record matches the current record, increment the
    access count.  Each request for access must be matched by a release of that
    same access.  The release of ACQUIRE access occurs in resetState().            */

	for (iter = 0; iter < sizeofArray( interactiveAccess ); iter++)
	  if (interactiveAccess[ iter ].level == level) {

	DPRINT2( 1, "found authorization record while storing at level %s with count %d\n",
		    ((interactiveAccess[iter].level == SHIM) ? "Shim" : "Acquire"),
		      interactiveAccess[iter].count );
		
	  	if (compareAuth(
				 requesting,
				&interactiveAccess[ iter ].authRec
		) == 0) {
			interactiveAccess[ iter ].count++;
			return( 0 );
		}
		else
		  return( -1 );
	  }

/*  Now search for an empty record.  If none found, we cannot store this new record.  */

	index = -1;
	for (iter = 0; iter < sizeofArray( interactiveAccess ); iter++)
	  if (interactiveAccess[ iter ].level == NONE) {
		index = iter;
		break;
	  }

	if (index < 0)
	  return( -1 );

	memcpy( (char*) &interactiveAccess[ index ].authRec,
	        (char*) requesting,
	        sizeof( *requesting ) );
	interactiveAccess[ index ].level = level;
	interactiveAccess[ index ].count = 1;

	DPRINT2( 1, "stored authorization record at level %s with count %d\n",
		    ((interactiveAccess[index].level == SHIM) ? "Shim" : "Acquire"),
		      interactiveAccess[index].count );
		
	return( 0 );
}


/*  WARNING:  To provide greater user-friendliness, strings are compared
              only upto the length of the template.  Thus "acq" and "acqi"
              are effectively the same to this program.				*/
 
static int
parseLevel( char *levelString )
{
	int	retval;

	if (strncmp( levelString, "shim", strlen( "shim" ) ) == 0)
	  retval = SHIM;
	else if (strncmp( levelString, "acq", strlen( "acq" ) ) == 0)
	  retval = ACQUIRE;
	else if (strncmp( levelString, "none", strlen( "none" ) ) == 0)
	  retval = NONE;
	else
	  retval = ERROR;

	return( retval );
}

/*  Returns 1 if OK,  0 if not  */

static int
verifyInteractAccess( const authRecord *requesting, int level )
{
	int	index, retval;

	retval = 1;

/*  Consult the access scheme used by sa, aa, etc., to keep Eve
    from messing with Bob's sample.  Here it keeps Eve from
    shimming on Bob's sample.  Remember that Eve can't run lock
    display or FID display while Bob is collecting data because
    in that case Bob will have reserved the console for acquiring
    prior to starting the experiment.  See go.c, vnmr, and
    ACCESSQUERY in socket.c.						*/

	if (requesting != NULL)
	  if (isUserOk2Access( (char *)(&(requesting->userName[ 0 ])) ) == 0)
	    return( 0 );

	index = locateAuthRecordByLevel( level );
	if (index < 0)			/* If no one accessing at this level */
	  return( 1 );					     /* then it's OK */

	if (isAuthActive( &interactiveAccess[ index ].authRec )) 
        {
          int res;
	  res = compareAuth( &interactiveAccess[ index ].authRec, requesting );
	  retval = (res == 0);
          if ((retval == 0) && (res == 2))
             retval = 1;
	}

/*  interactive process no longer active -  erase the record.  */

	else 
        {
	   if (interactiveAccess[ index ].authRec.processID == interactive_pid)
	     interactive_pid = -1;
	   clearAuthRecord( &interactiveAccess[ index ].authRec );
	   interactiveAccess[ index ].level = NONE;
	   interactiveAccess[ index ].count = 0;
	}

	return( retval );
}

const static int	accessLevel[ NUMBER_OF_LEVELS ] = { SHIM, ACQUIRE };

/*  Use this program to determine if someone has interactive access to the console.  */

int
isThereInteractiveAccess()
{
	int	iter;

	for (iter = 0; iter < NUMBER_OF_LEVELS; iter++)
	 if (verifyInteractAccess( NULL, accessLevel[ iter ]) == 0)
	  return( 1 );

	return( 0 );
}

/*  The transparent command uses this program  */

static int
verifyNoAccessConflict( const authRecord *requesting )
{
	int	iter;

	for (iter = 0; iter < NUMBER_OF_LEVELS; iter++)
	 if (verifyInteractAccess( requesting, accessLevel[ iter ]) == 0)
	  return( 0 );

	return( 1 );
}

/*  end of programs to assist with the console access authorization scheme.  */


#if 0
int
disconnectInteractive()
{
	int	index, iter, ipid;

	for (iter = 0; iter < NUMBER_OF_LEVELS; iter++) {
		index = locateAuthRecordByLevel( accessLevel[ iter ] );
		if (index < 0)
		  continue;
		ipid = interactiveAccess[ index ].authRec.processID;
		DPRINT1( 0, "would tell process %d to disconnect\n", ipid );
	}

}
#endif

static int
hasSetupOrGoCompleted()
{
	int	opsCmpltFlags, opsCmpltMask;

	opsCmpltMask = (SETUP_CMPLT_FLAG | EXP_CMPLT_FLAG);
	opsCmpltFlags = getStatOpsCmpltFlags();
	if ((opsCmpltFlags & opsCmpltMask) == 0)
	  return( 0 );
	else
	  return( 1 );
}

/*  This command lets the VNMR command expactive find out if an
    automation run is in progress.  Other replies are included
    for future development.  It is quite similar, but not identical
    to the AcqState field of the console status block.  A key
    difference is this command examines the state of the system
    from the perspective of Expproc, not the console.

    Possible replies:
        INTERACTIVE
        ACQUIRING <experiment>
        RESERVED
        auto
        SHIMMING
        IDLE

    April 22, 1997  */

int queryStatus(char *args)
{
	char	*returnInterface, *userName;
	char	 expprocReply[ CONSOLE_MSGE_SIZE ];

	returnInterface = strtok( NULL, "\n" );
	userName = strtok( NULL, "\n" );

	DPRINT2(1,"query status: user %s, return interface %s\n",
                   userName, returnInterface);

        if ( chkTaskActive(AUTOPROC) )
	  strcpy( &expprocReply[ 0 ], "auto" );
	else if (interactive_pid >= 0)
	  deliverMessage( returnInterface, "INTERACTIVE" );
	else if ((int) strlen(ActiveExpInfo.ExpId) > 1)
	  sprintf( &expprocReply[ 0 ], "ACQUIRING %s", ActiveExpInfo.ExpId );
	else if (locateAuthRecordByLevel( ACQUIRE ) >= 0)
	  sprintf( &expprocReply[ 0 ], "RESERVED" );
	else if (locateAuthRecordByLevel( SHIM ) >= 0)
	  strcpy( &expprocReply[ 0 ], "SHIMMING" );
	else 
	  strcpy( &expprocReply[ 0 ], "IDLE" );

	deliverMessage( returnInterface, &expprocReply[ 0 ] );

	return( 0 );
}

/*  The reserve console program expects to reply to the requesting process.
    Messages and their meaning are listed here:

    OK - requesting process has access to the console
    BUSY - Special reply for ACQUIRE level of access.  The requesting process has
           access but according to the Expproc an acquisition started previously
           is still active.  This might happen if ACQI switches from FID display
           to lock display.  The FID experiment gets aborted (see stopInteract,
           this file) but this takes a finite amount of time.  We insist on ACQI
           waiting until this abort completes before we let it start the lock
           display.  See LKshow in lockdisplay.c.  A similar situation applies
           for FIDshow in fiddisplay.c.  You should not get this reply when
           requesting access for a non-interactive experiment; if you do, there
           is a Race condition between aborting the previous experiment and
           starting this experiment.
    NO - requesting process does NOT have access
    INTERACTIVE - requesting process does NOT have access, and the Expproc
                  (helpfully) points out it is ACQI that has taken over the
                  console.							*/


int
reserveConsole(char *args)
{
	int	 	 granted, level, newPid, statAcqState;
	char		*returnInterface, *authInfo, *userName, *hostName, *modeInteractive;
	char		 expprocReply[ CONSOLE_MSGE_SIZE ];
	authRecord	 requester;

	DPRINT( 1, "reserve console called in NDC expproc\n" );
	returnInterface = strtok( NULL, "\n" );
	returnInterface = checkForNoInterface( returnInterface );
	authInfo = strtok( NULL, "\n" );
	modeInteractive = strtok( NULL, "\n" );

	if (authInfo == NULL) {
		errLogRet( ErrLogOp, debugInfo,
			  "reserve console: no authorization information\n" );
		deliverMessage( returnInterface, "NO" );
		return( -1 );
	}

	if (modeInteractive == NULL) {
		errLogRet( ErrLogOp, debugInfo, "reserve console: no mode\n" );
		deliverMessage( returnInterface, "NO" );
		return( -1 );
	}

/*  Check for a series of special situations here ...  */

	if (consoleConn() == 0) {
		errLogRet( ErrLogOp, debugInfo, "reserve console: console is down\n" );
		deliverMessage( returnInterface, "DOWN" );
		return( -1 );
	}

	statAcqState = getStatAcqState();
	if ( (statAcqState == ACQ_TUNING) && !chkTaskActive(AUTOPROC) )
        {
		errLogRet( ErrLogOp, debugInfo, "reserve console: console in tune mode\n" );
		deliverMessage( returnInterface, "TUNING" );
		return( -1 );
	}

/*  Certainly ACQ_TUNING is not the only AcqState that suggests the console is
    not available to be reserved.  At this time though, we catch other states
    in other ways, for example the console has been reserved by someone else.	*/

	parseAuthInfo( authInfo, &userName, &hostName, &newPid );

	if (newPid == 0) {
		errLogRet( ErrLogOp, debugInfo,
			  "reserve console: no process ID\n" );
		deliverMessage( returnInterface, "NO" );
		return( -1 );
	}

	level = parseLevel( modeInteractive );
	if (level == ERROR) {
		errLogRet( ErrLogOp, debugInfo,
	   "reserve console: unrecognized level of access '%s'\n", modeInteractive
		);
		deliverMessage( returnInterface, "NO" );
	}

	buildAuthRecord( &requester, userName, hostName, newPid );

/*  level == NONE ==> process is releasing its access.
    Therefore granted becomes true if the requesting process has an access record;
    that is, a record matching the current requester can be located in the database.  */

	if (level == NONE)
	  granted = (locateAuthRecordByRecord( &requester ) > 0);
	else
	  granted = verifyInteractAccess( &requester, level );

	if (granted) {
		if (level == NONE) {
			int	index;

			while ((index = locateAuthRecordByRecord( &requester )) >= 0)
			  clearAuthRecordByIndex( index );
			strcpy( expprocReply, "OK" );
		}

/*  A series of tests and checks are required to prevent conflicts.  In all cases
    the authorization record is stored.  If none if these tests or checks pan out,
    the reply will be OK. 

    1)  If ACQUIRE access is requested and ACQI has access to the console,
        the reply is BUSY.  See the description at the start of reserveConsole.

    2)  If SHIM access is reqested and an acquisition is proceeding, then
        reply ACQUIRING.  Append the experiment identification to the reply.

    3)  If SHIM access is requested and an acquisition is planned, reply
        ACQUIRING.  An acquisition is planned if the console is reserved for
        ACQUIRE but no experiment is active yet.  This helps prevent a race
        between go and ACQI.  Previously if ACQI connected after the console
        had been reserved (by go) but before Expproc had actually started the
        acquisition, then ACQI would receive OK and (in principle) access to
        FID display and lock display was available.

    4)  If the automation daemon (Autoproc) is active, reply auto.		*/

		else {
			if ( (level == ACQUIRE) && (interactive_pid >= 0) )
				strcpy( expprocReply, "BUSY" );
			else if (level == SHIM && (int) strlen(ActiveExpInfo.ExpId) > 1)
				sprintf( &expprocReply[ 0 ],
					 "ACQUIRING %s", ActiveExpInfo.ExpId );
			else if (level == SHIM && locateAuthRecordByLevel( ACQUIRE ) >= 0)
				sprintf( &expprocReply[ 0 ], "ACQUIRING" );
                        else if ( chkTaskActive(AUTOPROC) )
				strcpy (expprocReply, "auto" );
			else if (hasSetupOrGoCompleted())
				strcpy( expprocReply, "OK" );
			else
				strcpy( expprocReply, "OK2" );

		/*  Do not store the authorization record if the reply will be
		    BUSY.  The requesting process (ACQI or some other interactive
		    program) already has access to the console.  Following test
		    is the logical negation of the 1st special test, at the start
		    of this program block.    April 23, 1997  */

			if ( (level != ACQUIRE) || (interactive_pid < 0) )
			  storeAuthRecord( &requester, level );
		}
		deliverMessage( returnInterface, expprocReply );
	}
	else {
		DPRINT1(1,
	   "reserve console: access level '%s' denied\n", modeInteractive
		);
		if (interactive_pid >= 0)
		  deliverMessage( returnInterface, "INTERACTIVE" );
                else if ( chkTaskActive(AUTOPROC) )
		  deliverMessage( returnInterface, "auto" );
                else
		  deliverMessage( returnInterface, "NO" );
	}

	return( 0 );
}

int releaseConsole(char *args)
{
	int	 	 accepted, index, level, thisPid;
	char		*modeInteractive, *returnInterface, *authInfo, *userName, *hostName;
	authRecord	 requester;

#ifdef MSG_DEBUG
	DPRINT( 1, "release console called in NDC expproc\n" );
#endif
	returnInterface = strtok( NULL, "\n" );
	returnInterface = checkForNoInterface( returnInterface );
	authInfo = strtok( NULL, "\n" );
	modeInteractive = strtok( NULL, "\n" );

	if (authInfo == NULL) {
		errLogRet( ErrLogOp, debugInfo,
			  "release console: no authorization information\n" );
		deliverMessage( returnInterface, "NO" );
		return( -1 );
	}

	parseAuthInfo( authInfo, &userName, &hostName, &thisPid );

	if (thisPid == 0) {
		errLogRet( ErrLogOp, debugInfo,
			  "release console: no process ID\n" );
		deliverMessage( returnInterface, "NO" );
		return( -1 );
	}

	if (modeInteractive == NULL)
	  level = NONE;
	else
	  level = parseLevel( modeInteractive );

	accepted = 1;
	buildAuthRecord( &requester, userName, hostName, thisPid );

	if (level == NONE) {
		while ((index = locateAuthRecordByRecord( &requester )) >= 0)
		  clearAuthRecordByIndex( index );
	}
	else {
		index = locateAuthRecordByLevel( level );
		if (index >= 0) {
			accepted = (compareAuth( &interactiveAccess[ index ].authRec, &requester ) == 0);
			if (accepted)
			  clearAuthRecordByIndex( index );
		}
	}

	if (accepted) {
		deliverMessage( returnInterface, "OK" );
	}
	else {
		deliverMessage( returnInterface, "NO" );
	}

	return( 0 );
}


/* sethw */
/* place after the Interactive Access declaration, so it can consult it  */

int acqHwSet(char *args)
{
	char		*returnInterface;
	char		*userName, *hostName, *authInfo;
	char		*token;
	char		 console_msg[ CONSOLE_MSGE_SIZE ];
	char		 toke[ CONSOLE_MSGE_SIZE ];
        int              masToke = 0;
        int              numTokes, tok1, tok2, tok3, tok4;
	char		 delimiter_2[ 2 ];
	int		 console_len, granted, thisPid, tlen;
	authRecord	 requester;
        struct timespec timer;


	returnInterface = strtok( NULL, "\n" );
	returnInterface = checkForNoInterface( returnInterface );
	authInfo = strtok( NULL, "\n" );

#ifdef DEBUG
        if ( returnInterface != NULL) {
		DPRINT1(1, "acqHwSet: returnInterface - '%s' \n", returnInterface );
	}
	else
	  DPRINT(1, "acqHwSet: no return interface\n" );
#endif
	DPRINT1(1, "acqHwSet: User %s wants to set hardware values\n", authInfo );

	if (consoleConn() == 0) {
		deliverMessage( returnInterface, "DOWN" );
		return 0;
	}

/*  This program does not check for an active experiment.  At this time it is
    desired to be able to set the hardware during an acquisition, either with
    Vnmr or with ACQI.  Now if some process has reserved the console, either
    for shimming, or for (interactive) acquisition, then only that process will
    be granted access to the console.  Granting access however is done
    independent of whether an acquisition is in progress, at least here.	*/

	parseAuthInfo( authInfo, &userName, &hostName, &thisPid );

/*  Although we do not require a PID, if a process has claimed interactive access
    to the console and another attempts to set hardware without providing a process
    ID, the compareAuth program (called from verifyInteractAccess) will decline to
    grant access to the requesting process since the two process IDs will be
    different (zero vs. non-zero).						*/

	buildAuthRecord( &requester, userName, hostName, thisPid );

/*  The Roboproc is a special username which only the Roboproc is expected to
    use.  It needs to effect set hardware commands (e. g. change sample), but
    simultaneous ACQI connected access is also required.  The latter causes
    the console to be reserved for shimming, locking out other set hardware
    activity without this work-a-round.

    Added Expproc as a 2nd special username, 02/1997				*/

        if ( strcmp(userName,"Roboproc") != 0 && strcmp(userName,"Expproc") != 0 )
        {
	  granted = verifyInteractAccess( &requester, SHIM );
	}
	else
	{
	  DPRINT1(1,"acqHwSet: User %s wants it He's got it.\n",userName);
	  granted = 1;
	}

	if (granted == 0) {
		deliverMessage( returnInterface, "interactive" );
		return 0;
	}

	memset( (char *) &console_msg[ 0 ], 0, sizeof( console_msg ) );
	delimiter_2[ 0 ] = DELIMITER_2;
	delimiter_2[ 1 ] = '\0';
	sprintf( &console_msg[ 0 ], "%d%c", XPARSER, DELIMITER_2 );
	console_len = strlen( &console_msg[ 0 ] );

        numTokes = 0;
        strcpy(toke,"");
	while ((token = strtok( NULL, "\n" )) != NULL) {
		tlen = strlen( token ) + strlen( &delimiter_2[ 0 ] );
		console_len += tlen;
                if (numTokes == 0)
                {
                   strcpy(toke,token);
                   numTokes++;
                }
		strcat( &console_msg[ 0 ], token );
		strcat( &console_msg[ 0 ], &delimiter_2[ 0 ] );
	}
        numTokes = atoi(toke);
        masToke = 0;
        if (numTokes == 2)
        {
           sscanf(toke,"2,%d,%d,", &tok1, &tok2);
	   DPRINT2(2,"acqHwSet: tok1: %d tok2: %d\n",tok1, tok2);
           masToke = 1;
           if (tok1 == 49)
              strcpy(console_msg, "userCmd start");
           else if (tok1 == 48)
              strcpy(console_msg, "userCmd stop");
           else if (tok1 == 38)
              sprintf(console_msg, "userCmd bearspan %d", tok2);
           else if (tok1 == 39)
              sprintf(console_msg, "userCmd bearadj %d", tok2);
           else if (tok1 == 40)
              sprintf(console_msg, "userCmd bearmax %d", tok2);
           else if (tok1 == 41)
              sprintf(console_msg, "userCmd asp %d", tok2);
           else if (tok1 == 47)
           {
              if (tok2 == 10)
                 strcpy(console_msg, "roboready");
              else
                 sprintf(console_msg, "userCmd profile %d", tok2);
           }
           else
              masToke = 0;
        }
        else if (numTokes == 4)
        {
           sscanf(toke,"4,%d,%d,%d,%d,", &tok1, &tok2, &tok3, &tok4);
	   DPRINT2(2,"acqHwSet: tok1: %d tok2: %d\n",tok1, tok2);
	   DPRINT2(2,"acqHwSet: tok3: %d tok4: %d\n",tok3, tok4);
           if ( (tok1 == 35) && (tok3 == 9) )
           {
              char testFile[256];
              sprintf(testFile,"%s/masport",systemdir);
              if (! access(testFile, R_OK) )
              {
                 sprintf(console_msg, "userCmd speed %d", tok4);
                 masToke = 1;
              }
           }
        }

        if (masToke)
	   masCmd(console_msg);
        else
	   writeChannel(chanId, &console_msg[ 0 ], CONSOLE_MSGE_SIZE);
/* There appears to be a race in Vnmrbg. After this message is sent
 * to Expproc, Vnmrbg does a "select" on the receiving port to get
 * the below message. If the "started" message arrives before the
 * "select" is called, Vnmrbg hangs up waiting for it.
 * The following sleep seems to avoid the problem.
 */
        timer.tv_sec=0;
        timer.tv_nsec = 15000000;   /* 15 msec */
        nanosleep(&timer, NULL);
	deliverMessage( returnInterface, "started" );
	return 0;
}

/*  Based on acqHwSet, but allows access to the A-code update facility in the console.  */


int call_aupdt(char *args)
{
        char            *returnInterface;
        char            *userName, *hostName, *authInfo;
        char            *token;
        char             console_msg[ CONSOLE_MSGE_SIZE ];
        char             delimiter_2[ 2 ];
        int              console_len, granted, thisPid, tlen;
        authRecord       requester;

        returnInterface = strtok( NULL, "\n" );
        returnInterface = checkForNoInterface( returnInterface );
        authInfo = strtok( NULL, "\n" );

        DPRINT1(1, "user %s wants to set hardware values\n", authInfo );

        if (consoleConn() == 0) {
                deliverMessage( returnInterface, "DOWN" );
                return 0;
        }

/*  ACQUIRE access is currently required.  Most AUPDT stuff requires an
    acquisition be in progress.  Only ACQI fiddisplay uses this facility now.   */

        parseAuthInfo( authInfo, &userName, &hostName, &thisPid );
        buildAuthRecord( &requester, userName, hostName, thisPid );
        granted = verifyInteractAccess( &requester, ACQUIRE );

        if (granted == 0) {
                deliverMessage( returnInterface, "interactive" );
                return 0;
        }
 
        memset( (char *) &console_msg[ 0 ], 0, sizeof( console_msg ) );
        delimiter_2[ 0 ] = DELIMITER_2;
        delimiter_2[ 1 ] = '\0';
        sprintf( &console_msg[ 0 ], "%d%c", AUPDT, DELIMITER_2 );
        console_len = strlen( &console_msg[ 0 ] );
 
        while ((token = strtok( NULL, "\n" )) != NULL) {
                tlen = strlen( token ) + strlen( &delimiter_2[ 0 ] );
                console_len += tlen;
                strcat( &console_msg[ 0 ], token );
                strcat( &console_msg[ 0 ], &delimiter_2[ 0 ] );
        }
 
        writeChannel(chanId, &console_msg[ 0 ], CONSOLE_MSGE_SIZE);
        deliverMessage( returnInterface, "started" );
	return 0;
}
 

/*  Based on call_aupdt, but allows seperate socket for data stream (tables) */

int call_jupdt(char *args)
{
   char		*returnInterface;
   char		*userName, *hostName, *authInfo;
   char		*token, *strpos;
   int		granted, thisPid;
   int 		nTables, TotalTableSize;
   int 		*ptr,cnt,i;
   char 	*chrptr;
   char         *cmd;
   char         *updtcmd;
   int 		cmdlen,updtcmdsize,bytes;
   authRecord	requester;


   strpos = args;

   cnt = 6;
   while( ((cnt > 0) && (chanIdSync == -1)) )
   {
      chanIdSync = connectChan(UPDTPROC_CHANNEL);
      cnt--;
   }
   if (chanIdSync == -1)
   {
	errLogRet( ErrLogOp, debugInfo,
	        "Failed to establish update connection to Console, msge not sent\n");
	return 0;
   }
   /* DPRINT1(-1,"chanIdSync = %d\n",chanIdSync); */

   returnInterface = strtok_r( args, "\n", &strpos );
      /* DPRINT2(-1,"strtok: '%s', strpos: 0x%lx\n",returnInterface,strpos); */
   returnInterface = checkForNoInterface( returnInterface );
   authInfo = strtok_r( (char *)NULL, "\n", &strpos );
      /* DPRINT2(-1,"strtok: '%s', strpos: 0x%lx\n",authInfo,strpos); */

   DPRINT1(1, "user %s wants to set hardware values\n", authInfo );
   token = strtok_r((char *)NULL,"\n",&strpos);
     /* DPRINT2(-1,"strtok: updtCmd: '%s', strpos: 0x%lx\n",token,strpos); */
   cmd = token;
   cmdlen = strlen(cmd);
   DPRINT2(1,"updtCmd: '%s', size: %d\n",cmd,cmdlen);
        
   token = strtok_r((char *)NULL," ",&strpos);
   nTables = atoi(token);
   token = strtok_r((char *)NULL,"\n",&strpos);
   TotalTableSize = atoi(token);
   DPRINT2(1,"Ntables: %d, total Bytes: %d\n",nTables,TotalTableSize);

   token = strtok_r((char *)NULL,"\n",&strpos);
   /* DPRINT2(-1,"strtok: '%s', strpos: 0x%lx\n",token,strpos); */
   /* memcpy(updtcmd,token,strlen(token)+1); */
   updtcmd = token;
   updtcmdsize = strlen(updtcmd)+1;
   DPRINT2(1,"updtCmd: '%s', size: %d\n",updtcmd,strlen(updtcmd));

#ifdef DIAGNOSTIC_OUTPUT
   cnt = TotalTableSize / 4;
   /* chrptr =  calloc(cnt, sizeof(int)); */
   chrptr =  malloc(TotalTableSize);
   memcpy(chrptr,strpos,TotalTableSize);
   /* ptr = (int *) strpos; */
   ptr = (int*) chrptr;
   DPRINT1(-1,"ptr: 0x%lx\n",ptr);
   cnt = TotalTableSize;
   for(i=0;i<cnt;i++)
   {
/*
     DPRINT4(-1,"Table Value[%d](0x%x) = %d, 0x%x\n",i,ptr,*ptr,*ptr);
*/
     DPRINT5(-1,"Table Value[%d](0x%lx) = %d, 0x%x, %c \n",i,chrptr,*chrptr,*chrptr,*chrptr);
     DPRINT(-1,"\n");
     chrptr++;
   }
   /* free(chrptr); */
   free(ptr);
#endif

   if (consoleConn() == 0) 
   {
	deliverMessage( returnInterface, "DOWN" );
	return 0;
   }


        
   /*  ACQUIRE access is currently required.  Most AUPDT stuff requires an
       acquisition be in progress.  */

   parseAuthInfo( authInfo, &userName, &hostName, &thisPid );
   buildAuthRecord( &requester, userName, hostName, thisPid );
   granted = verifyInteractAccess( &requester, ACQUIRE );

   if (granted == 0) {
	deliverMessage( returnInterface, "interactive" );
	return 0;
   }

    /* Now get the rtvar and rttable cmds
      1. tables must sent down prorir to update, use seperate socket & channel
         to send these down.

      2. Update tables pointers & then rtvars
    */

     blockAllEvents();
     DPRINT1(1,"Send: %s\n",cmd);
     bytes = writeChannel(chanIdSync,(char *)(&cmdlen),sizeof(int));
     DPRINT1(-1,"bytes written: %d\n",bytes);

     bytes = writeChannel(chanIdSync,cmd,cmdlen);
     DPRINT1(-1,"bytes written: %d\n",bytes);
     if ( nTables > 0)
     {
        DPRINT(1,"Send: Tables\n");

        bytes = writeChannel(chanIdSync,strpos,TotalTableSize);
     }

     bytes = writeChannel(chanIdSync,(char *)(&updtcmdsize),sizeof(int));
     DPRINT1(1,"Send: %s\n",updtcmd);

     bytes = writeChannel(chanIdSync,updtcmd,updtcmdsize);
     unblockAllEvents();
     return 0;
}


static int
completeStartLock()
{
	char	 msge[CONSOLE_MSGE_SIZE], recvmsg[128];

	sprintf(recvmsg,"startI %s",ActiveExpInfo.ExpId);
  	DPRINT1(1,"Send Recvproc: '%s'\n",recvmsg);
	deliverMessage( "Recvproc", recvmsg );

	memset( &msge[ 0 ], 0, sizeof( msge ) );
	sprintf( &msge[ 0 ], "%d%c", STARTLOCK, DELIMITER_2 );
	writeChannel(chanId,&msge[ 0 ],CONSOLE_MSGE_SIZE);
	return 0;
}

static int
completeStartFID()
{
	char	msge[CONSOLE_MSGE_SIZE], recvmsg[128], sndmsge[128];

	DPRINT( 1, "complete start FID called\n" );

/* Send Recvproc its message first  */

	sprintf(recvmsg,"startI %s",ActiveExpInfo.ExpId);
  	DPRINT1(1,"Send Recvproc: '%s'\n",recvmsg);
	deliverMessage( "Recvproc", recvmsg );

	sprintf( &sndmsge[ 0 ], "send %s %s",
		  ActiveExpInfo.ExpId, ActiveExpInfo.ExpInfo->AcqBaseBufName);
  	DPRINT1(1,"Send Sendproc: '%s'\n", &sndmsge[ 0 ] );
	deliverMessage( "Sendproc", sndmsge );

	memset( &msge[ 0 ], 0, sizeof( msge ) );
	sprintf(&msge[0],"%d%cACQI_PARSE,%s,%ld,%ld,%ld;",
		STARTINTERACT, DELIMITER_2,
		ActiveExpInfo.ExpInfo->AcqBaseBufName,
		ActiveExpInfo.ExpInfo->NumAcodes,
		ActiveExpInfo.ExpInfo->NumTables,
		ActiveExpInfo.ExpInfo->Celem);
  	DPRINT1(1,"Send console: '%s'\n", &msge[ 0 ] );
	writeChannel(chanId,&msge[ 0 ],CONSOLE_MSGE_SIZE);
	return 0;
}

/*  startInteract was changed to examine the authorization record database
    and to only store the record for the requesting process if no record
    was present prior to the call to startInteract.  The interactive
    application is expected to reserve the console for acquiring before
    starting the interactive acquisition.  However Expproc did not enforce
    this requirement.  Since Expproc will eventually count the number of
    times a process obtains access to the console, it needs to be more
    careful about storing an authorization record.   April 24, 1997.      */

int
startInteract()
{
	int	 	 granted, newPid, notRegistered, startingLock;
	char		*returnInterface, *authInfo, *userName, *hostName;
	MFILE_ID	 ifile;
	authRecord	 requester;

#ifdef MSG_DEBUG
	DPRINT( 1, "start interactive called in NDC expproc\n" );
#endif
	returnInterface = strtok( NULL, "\n" );
	returnInterface = checkForNoInterface( returnInterface );
	authInfo = strtok( NULL, "\n" );

	if (authInfo == NULL) {
		errLogRet( ErrLogOp, debugInfo,
			  "start interactive: no authorization information\n" );
		deliverMessage( returnInterface, "NO" );
		return( -1 );
	}

	if (consoleConn() == 0) {
		deliverMessage( returnInterface, "DOWN" );
		return 0;
	}

	notRegistered = (locateAuthRecordByLevel( ACQUIRE ) < 0);
	parseAuthInfo( authInfo, &userName, &hostName, &newPid );

/*  Insist on a Process ID... but not host name  */

	if (newPid == 0) {
		errLogRet( ErrLogOp, debugInfo,
			  "start interactive: no process ID\n" );
		deliverMessage( returnInterface, "NO" );
		return( -1 );
	}

	buildAuthRecord( &requester, userName, hostName, newPid );
	granted = verifyInteractAccess( &requester, ACQUIRE );

/*  You are not permitted to start an interactive acquisition
    if any kind of acquisition is in progress.			*/

	if (granted) {
		granted = (int) strlen( ActiveExpInfo.ExpId ) < 1;
	}

/*  Note that if this ever becomes a multi-threaded application, you will
    need to restrict access to the authorization record at this point in
    the program.  We do not store the authorization information until the
    end of this procedure.  See the reference to storeAuthRecord.	*/

	if (granted == 0) {
		ExpEntryInfo	DummyExpInfo;

		errLogRet( ErrLogOp, debugInfo,
			  "start interactive: access denied\n" );

	/*  If access is not granted, the Expproc needs to remove the entry
            ACQI put on the experiment queue.  Other failure returns prior
	    to this represent program failures which we do not expect to
            occur in nominal operation.  This situation occurs whenever ACQI
            tries to connect for lock or FID display when something else is
            going on, so we do need to remove the ACQI entry.  07/05/1995  */

		if (expQget(&DummyExpInfo.ExpPriority, DummyExpInfo.ExpId) != -1)
		  expQdelete(DummyExpInfo.ExpPriority, DummyExpInfo.ExpId);

		deliverMessage( returnInterface, "NO" );
		return( 0 );
	}

	if (expQget(&ActiveExpInfo.ExpPriority, ActiveExpInfo.ExpId) == -1) {
		DPRINT(1,"start interactive: No experiment in queue.\n");
		deliverMessage( returnInterface, "NO" );
		return(-1);
	}

	if (mapInExp(&ActiveExpInfo) == -1) {
		DPRINT(1,"start interactive: Can't map in current experiment.\n");
		deliverMessage( returnInterface, "NO" );
		return(-1);
	}

#if 0
		printf( "startInteractive: data point size: %d, number of points: %d\n",
			 ActiveExpInfo.ExpInfo->DataPtSize,
			 ActiveExpInfo.ExpInfo->NumDataPts
		);
		printf( "length of Acq Base Buffer Name: %d, length of Code File: %d\n",
			 strlen( ActiveExpInfo.ExpInfo->AcqBaseBufName ),
			 strlen( ActiveExpInfo.ExpInfo->Codefile )
		);
		printf( "length of data file: %d, length of user name: %d\n",
			 strlen( ActiveExpInfo.ExpInfo->DataFile ),
			 strlen( ActiveExpInfo.ExpInfo->UserName )
		);
		printf( "GO flag: %d\n", ActiveExpInfo.ExpInfo->GoFlag );
#endif

/*  Maintain consistancy with user names, 09/27/1995  */

	strcpy(ActiveExpInfo.ExpInfo->UserName, userName);

/*  You can only do ACQI_LOCK or EXEC_GO as an interactive experiment.
    (EXEC_LOCK is a different experiment from interactive lock; do not
    confuse the two.)							*/

	if (ActiveExpInfo.ExpInfo->GoFlag == ACQI_LOCK) {
		startingLock = 131071;
	}
	else if (ActiveExpInfo.ExpInfo->GoFlag == EXEC_GO) {
		startingLock = 0;
	}
	else {
		errLogRet( ErrLogOp, debugInfo,
			 "start interactive: go flag of %d not available\n",
			  ActiveExpInfo.ExpInfo->GoFlag
		);
		deliverMessage( returnInterface, "NO" );
		return( -1 );
	}

/*  Perform checks to prevent program failures...  */

	if ((int) strlen( ActiveExpInfo.ExpInfo->DataFile ) < 1) {
		errLogRet( ErrLogOp, debugInfo,
			  "start interactive: no datafile name\n" );
		deliverMessage( returnInterface, "NO" );
		return( -1 );
	}
	if (ActiveExpInfo.ExpInfo->DataSize < 1) {
		errLogRet( ErrLogOp, debugInfo,
			  "start interactive: data size is 0 or smaller\n" );
		deliverMessage( returnInterface, "NO" );
		return( -1 );
	}

#if 0
		printf( "Will open %s with a size of %d\n",
			 ActiveExpInfo.ExpInfo->DataFile,
			 ActiveExpInfo.ExpInfo->DataSize
		);
#endif

	ifile = mOpen(
		ActiveExpInfo.ExpInfo->DataFile,
		ActiveExpInfo.ExpInfo->DataSize,
		O_RDWR | O_CREAT | O_TRUNC
	);
	ifile->newByteLen = ActiveExpInfo.ExpInfo->DataSize,
	mClose( ifile );

	if (startingLock)
	  completeStartLock();
	else
	  completeStartFID();

	expQdelete(ActiveExpInfo.ExpPriority, ActiveExpInfo.ExpId);

	if (notRegistered)
	  storeAuthRecord( &requester, ACQUIRE );
	interactive_pid = requester.processID;

	deliverMessage( returnInterface, "OK" );

	return( 0 );
}

/*  getInteract provides no response -  it would
    take too much time for ACQI to wait for it.		*/

int
getInteract()
{
	int	 	 granted, newPid;
	char		*returnInterface, *authInfo, *userName, *hostName;
	char	 	 msge[CONSOLE_MSGE_SIZE], recvmsg[128];
	authRecord	 requester;

#ifdef MSG_DEBUG
	DPRINT( 1, "get interactive called in NDC expproc\n" );
#endif
	returnInterface = strtok( NULL, "\n" );
	returnInterface = checkForNoInterface( returnInterface );
	authInfo = strtok( NULL, "\n" );

	if (authInfo == NULL) {
		errLogRet( ErrLogOp, debugInfo,
			  "get interactive: no authorization information\n" );
		return( -1 );
	}

	parseAuthInfo( authInfo, &userName, &hostName, &newPid );

	if (newPid == 0) {
		errLogRet( ErrLogOp, debugInfo,
			  "get interactive: no process ID\n" );
		return( -1 );
	}

	buildAuthRecord( &requester, userName, hostName, newPid );
	granted = verifyInteractAccess( &requester, ACQUIRE );

/*  It is expected the requesting process has access ...  */

	if (granted == 0) {
		errLogRet( ErrLogOp, debugInfo,
			  "get interactive: requesting process was not granted access\n" );
		return( 0 );
	}

	sprintf(recvmsg,"recvI");
#ifdef MSG_DEBUG
  	DPRINT1(1,"Send Recvproc: '%s'\n",recvmsg);
#endif
/*
	deliverMessage( "Recvproc", recvmsg );
*/

	memset( &msge[ 0 ], 0, sizeof( msge ) );
	sprintf( &msge[ 0 ], "%d%c", GETINTERACT, DELIMITER_2 );
	writeChannel(chanId,&msge[ 0 ],CONSOLE_MSGE_SIZE);
	return 0;
}

int
stopInteract()
{
	int	 	 granted, newPid;
	char		*returnInterface, *authInfo, *userName, *hostName;
	char	 	 msge[CONSOLE_MSGE_SIZE];
	authRecord	 requester;

#ifdef MSG_DEBUG
	DPRINT( 1, "stop interactive called in NDC expproc\n" );
#endif
	returnInterface = strtok( NULL, "\n" );
	returnInterface = checkForNoInterface( returnInterface );
	authInfo = strtok( NULL, "\n" );

	if (authInfo == NULL) {
		errLogRet( ErrLogOp, debugInfo,
			  "stop interactive: no authorization information\n" );
		deliverMessage( returnInterface, "NO" );
		return( -1 );
	}

	parseAuthInfo( authInfo, &userName, &hostName, &newPid );

	if (newPid == 0) {
		errLogRet( ErrLogOp, debugInfo,
			  "stop interactive: no process ID\n" );
		deliverMessage( returnInterface, "NO" );
		return( -1 );
	}

	buildAuthRecord( &requester, userName, hostName, newPid );
	granted = verifyInteractAccess( &requester, ACQUIRE );

	if (granted == 0) {
		deliverMessage( returnInterface, "NO" );
		return( 0 );
	}

	abort_in_progress = 131071;
	memset( &msge[ 0 ], 0, sizeof( msge ) );
	sprintf( &msge[ 0 ], "%d%c", STOPINTERACT, DELIMITER_2 );
	writeChannel(chanId,&msge[ 0 ],CONSOLE_MSGE_SIZE);

/*  "stopI" message to Recvproc deleted -  the Recvproc finds out from
    the console that the interactive acquisition is over.  Sending it a
    stopI message would only serve to confuse it, possibly causing it
    to screw up the next (non-interactive) experiment.  07/12/1995	*/

        /*DPRINT(1,"stopInteract: deliver 'stopI' to Recvproc\n");
	deliverMessage( "Recvproc", "stopI" );*/

/*  Let the resetState map out the experiment, clear the
    authorization record and set the interactive PID to -1.
    It keeps another interactive acquisition frm starting
    (or trying to start) until the console is ready.		*/

	/*if (mapOutExp(&ActiveExpInfo) == -1) {
		errLogRet(ErrLogOp,debugInfo,
			 "stop interactive: mapOutExp: %s failed.\n",
			  ActiveExpInfo.ExpId);
	}

	clearAuthRecordByLevel( ACQUIRE );
	interactive_pid = -1;*/

	deliverMessage( returnInterface, "OK" );

	return( 0 );
}

/*********************************************************
* queryAcquisition
*
*    preliminary version !!
*/
int
queryAcquisition()
{
	char	*returnInterface, *authInfo, *userName, *hostName;
	int	 newPid;

	printf( "query acquisition called in NDC expproc\n" );

	returnInterface = strtok( NULL, "\n" );
	returnInterface = checkForNoInterface( returnInterface );
	authInfo = strtok( NULL, "\n" );

	if (authInfo == NULL) {
		errLogRet( ErrLogOp, debugInfo,
			  "start interactive: no authorization information\n" );
		deliverMessage( returnInterface, "NO" );
		return( -1 );
	}

	if (consoleConn() == 0) {
		deliverMessage( returnInterface, "DOWN" );
		return 0;
	}

	parseAuthInfo( authInfo, &userName, &hostName, &newPid );
	if (isUserOk2Access( &userName[ 0 ] ) == 0) {
		deliverMessage( returnInterface, "NO" );
		return 0;
	}

	if (isExpActive())
	   printf( "experiment information is in %s\n", ActiveExpInfo.ExpId );
	else
	   printf( "no acquisition current right now\n" );
	deliverMessage( returnInterface, "NOT YET" );
	return 0;
}

/*********************************************************
* isExpActive - return 1 if an experiment is active
*
*/
int isExpActive()
{
   return(((int) strlen(ActiveExpInfo.ExpId) > 1));
}

/*********************************************************
* isUserOk2Access - return 1 if give user may have access
*		to the console.
*
*/
int isUserOk2Access(char* username)
{
   int stat = 1;
   if ((int) strlen(ActiveExpInfo.ExpId) > 1) 
   {
     if (ActiveExpInfo.ExpInfo != NULL)
     {
	if (strcmp(username,ActiveExpInfo.ExpInfo->UserName) != 0)
	{
          stat = 0;
        }
     }
   }
   return(stat);
}

/*********************************************************
* isExpNumOk2Access - return 1 if give exp. number  may have access
*		to the console.
*
*/
int isExpNumOk2Access(int expnum)
{
   int stat = 1;
   if ((int) strlen(ActiveExpInfo.ExpId) > 1) 
   {
     if (ActiveExpInfo.ExpInfo != NULL)
     {
	if (expnum != ActiveExpInfo.ExpInfo->ExpNum)
	{
          stat = 0;
        }
     }
   }
   return(stat);
}

/**********************************************************
* rmPsgFiles - removes the PSG files of the Experiment
*  Code, RTParms, Tables, RF Pattern, Grad Files
*
*/
void rmPsgFiles(SHR_EXP_INFO ExpInfo)
{
   DPRINT3(1,"\n   rmPsgFiles: codefile: '%s', rtp: '%s', table: '%s'\n",
	ExpInfo->Codefile,ExpInfo->RTParmFile,ExpInfo->TableFile);
   DPRINT1(1,"\n               waveform: '%s'\n",
	ExpInfo->WaveFormFile);
   if (ExpInfo->InteractiveFlag == 0)
   {
     if ((int) strlen(ExpInfo->Codefile) > 1)
          unlink(ExpInfo->Codefile);
     if ((int) strlen(ExpInfo->RTParmFile) > 1)
          unlink(ExpInfo->RTParmFile);
     if ((int) strlen(ExpInfo->TableFile) > 1)
          unlink(ExpInfo->TableFile);
     if ((int) strlen(ExpInfo->WaveFormFile) > 1)
          unlink(ExpInfo->WaveFormFile);
     if ((int) strlen(ExpInfo->GradFile) > 1)
          unlink(ExpInfo->GradFile);
     bill_done(ExpInfo);   /* mark if /vnmr/adm/accounting/enabled exists */
   }
}

int transparent( char *args )
{
	char		*msge, *returnInterface, *authInfo, *userName, *hostName;
	char	 	 consoleCmd[CONSOLE_MSGE_SIZE];
	int		 granted, thisPid;
	authRecord	 requester;

	returnInterface = strtok( NULL, "\n" );
	returnInterface = checkForNoInterface( returnInterface );
	authInfo = strtok( NULL, "\n" );
	msge = strtok(NULL,"");

	if (authInfo == NULL) {
		errLogRet( ErrLogOp, debugInfo,
			  "transparent: no authorization information\n" );
		return( -1 );
	}

	parseAuthInfo( authInfo, &userName, &hostName, &thisPid );
	buildAuthRecord( &requester, userName, hostName, thisPid );
	granted = verifyNoAccessConflict( &requester );
	if (granted == 0) {
		errLogRet( ErrLogOp, debugInfo,
			  "transparent command denied access to the console\n" );
		return( -1 );
	}

	memset( &consoleCmd[ 0 ], 0, sizeof( consoleCmd ) );

#if 0
	if (msge == NULL) {
		DPRINT( 0, "transparent command found nothing to send\n" );
		return( -1 );
	}
	else {
		DPRINT1( 0, "transparent: '%s'\n", msge );
	}
#endif

	strcpy( &consoleCmd[ 0 ], msge );
	writeChannel(chanId,&consoleCmd[ 0 ],CONSOLE_MSGE_SIZE);
	return( 0 );
}

/*  Unlike most commands, download DSP does NOT use its return interface
    and does NOT require any authorization information.			*/

#define  VMEADDRLEN	(8+2+1)

#define  FIX_APREG	40
#define  DSP_APREG	0x0e86
#define  DSP_APVAL	0x8000

static int whichDsp(char *vmeaddr)
{
   int i;
   char *stripvme,*strtmp2;
   /* 1st strip high order digit from address, since 68k & ppc will differ */
   /* e.g. 0xfa804000 --> 804000 */
   strtmp2 = strchr(vmeaddr,'x');
   stripvme = &strtmp2[3];
   
   for (i=0; i < 4; i++)
   {
      if (strcmp(stripvme,dspaddrs[i]) == 0)
	 break;
   }
   if (i > 3) i = 0;
   return i;
}

int downloadDSP( char *args )
{
	char		*memaddr, *returnInterface, *vmeAddrAscii, *dspFile;
	char		 filePath[ MAXPATHL ];
	char		 sendProcCmd[ MAXPATHL + 20 ];
	MFILE_ID	 md;
        int		 index;

	returnInterface = strtok( NULL, "\n" );
	returnInterface = checkForNoInterface( returnInterface );
	dspFile = strtok( NULL, "\n" );
	if (dspFile != NULL)
	  vmeAddrAscii = strtok( NULL, "\n" );
	else {
		vmeAddrAscii = NULL;
		dspFile = "dsp.ram";
	}

	if (vmeAddrAscii == NULL) {
		struct _hw_config	*confaddr;

		locateConfFile( &filePath[ 0 ] );

		md = mOpen( &filePath[ 0 ], 0, O_RDONLY );
		if (md == NULL) {
			errLogRet( ErrLogOp, debugInfo,
		   "can't access console configuration file %s\n", &filePath[ 0 ]
			);
			return( -1 );
		}
		confaddr = (struct _hw_config *) md->mapStrtAddr;
		vmeAddrAscii = malloc( VMEADDRLEN );
		if (vmeAddrAscii == NULL) {
			errLogRet( ErrLogOp, debugInfo,
		   "can't allocate space in DSP download\n"
			);
			mClose( md );
			return( -1 );
		}
		sprintf( vmeAddrAscii, "0x%lx", htonl((long) confaddr->dspDownLoadAddr) );
		mClose( md );
	}


/*  Next test is for absolute vs. relative path name for the DSP download file.  */

	if (*dspFile != '/') {
		strcpy( &filePath[ 0 ], getenv("vnmrsystem") );
		strcat( &filePath[ 0 ], "/acq/" );
		strcat( &filePath[ 0 ], dspFile );
	}
	else
	  strcpy( &filePath[ 0 ], dspFile );

        index = whichDsp(vmeAddrAscii);
	downloadInfo.type[index] = DSP_DOWNLOAD;

#ifdef DEBUG_DSP_DOWNLOAD
	errLogRet( ErrLogOp, debugInfo, "downloadDSP: download DSP:\n" );
	errLogRet( ErrLogOp, debugInfo, "downloadDSP: use file %s\n", &filePath[ 0 ] );
	errLogRet( ErrLogOp, debugInfo, "downloadDSP: use VME addr %s\n", vmeAddrAscii );
	errLogRet( ErrLogOp, debugInfo, "downloadDSP: DSP[%d] type = %d\n", index,downloadInfo.type[index] );
#endif

      /* sizeof sendProcCmd is 20 chars larger than filePath to prevent memory overrun */

	sprintf( &sendProcCmd[ 0 ], "vme %s %s", &filePath[ 0 ], vmeAddrAscii );
	deliverMessage( "Sendproc", &sendProcCmd[ 0 ] );

	return( 0 );
}

int dwnldComplete( char *args )
{
	char	*returnInterface, *dwnldStatus, *dwnldVmeAddress;
	char	 consoleCmd[ CONSOLE_MSGE_SIZE ];
	char	wallstr[256];
        int DSP_apreg,index;

        index = 0;
        DSP_apreg = dspapaddr[index];
	returnInterface = strtok( NULL, "\n" );
	returnInterface = checkForNoInterface( returnInterface );

#ifdef DEBUG_DSP_DOWNLOAD
	if (returnInterface == NULL)
	  errLogRet( ErrLogOp, debugInfo, "download complete with no return interface\n" );
	else
	  errLogRet( ErrLogOp, debugInfo, "download complete with return interface %s\n", returnInterface );
#endif

	dwnldStatus = strtok( NULL, "\n" );
	if (dwnldStatus == NULL) {
		errLogRet( ErrLogOp, debugInfo,
			  "download complete received without any status\n"
		);
		return( -1 );
	}

#ifdef DEBUG_DSP_DOWNLOAD
	errLogRet( ErrLogOp, debugInfo, "dwnldComplete: download complete with status %s\n", dwnldStatus );
#endif

	dwnldVmeAddress = strtok( NULL, "\n" );
        if ( dwnldVmeAddress != NULL)
        {
          index = whichDsp(dwnldVmeAddress);
          DSP_apreg = dspapaddr[index];
	  /* errLogRet( ErrLogOp, debugInfo, 
		"dwnldComplete: VME: '%s', ApAddr: 0x%x, index = %d\n",dwnldVmeAddress,index ); */
        }

	if (strncmp( "Completed", dwnldStatus, strlen( "Completed" )) == 0) 
        {
	   /* errLogRet( ErrLogOp, debugInfo, "Dsp %d SW Download '%s', VME: '%s', ApAddr: 0x%x\n",
			index+1, dwnldStatus, dwnldVmeAddress,DSP_apreg ); */
	   if (downloadInfo.type[index] == DSP_DOWNLOAD) 
           {
	      sprintf( &consoleCmd[ 0 ], "%d%c1;%d,%d,1,%d", 
		         AUPDT, DELIMITER_2, FIX_APREG, DSP_apreg, DSP_APVAL
		     );
	      /*errLogRet( ErrLogOp, debugInfo, "dwnldComplete: cmd: '%s'\n",consoleCmd ); */
	      writeChannel(chanId, &consoleCmd[ 0 ], CONSOLE_MSGE_SIZE);
	   }
	 /*
	  sprintf(wallstr,
                 "echo 'Dsp %d SW Download '%s', VME: '%s', ApAddr: 0x%x' | /usr/sbin/wall -a",
                                index+1, dwnldStatus, dwnldVmeAddress,DSP_apreg );
           system(wallstr);
	 */
	}
        else
        {
	  errLogRet( ErrLogOp, debugInfo, "Error: Dsp %d SW Download '%s', VME: '%s', ApAddr: 0x%x\n",
			index+1, dwnldStatus, dwnldVmeAddress,DSP_apreg );
	  sprintf(wallstr,
                 "echo 'Error: Dsp %d SW Download '%s', VME: '%s', ApAddr: 0x%x' | /usr/sbin/wall -a",
                                index+1, dwnldStatus, dwnldVmeAddress,DSP_apreg );
          system(wallstr);

        }
	downloadInfo.type[index] = NO_DOWNLOAD;

	return( 0 );
}

int getExpInfoFile( char *args )
{
	char	*returnInterface;

	returnInterface = strtok( NULL, "\n" );
	returnInterface = checkForNoInterface( returnInterface );

	if (isExpActive()) {
   		deliverMessage( returnInterface, ActiveExpInfo.ExpId );
	}
	else {
   		deliverMessage( returnInterface, " " );
	}

	return( 0 );
}

void sendRoboCommand(char *cmd)
{
   int stat, maxtries;
   /* If just started Roboproc, send roboReady until Roboproc
    * says it got it. Roboproc just throws it away, but it ensures
    * the messageQ is initialized.
    */
   maxtries = 25;
   while( 1 )
   {
      stat = deliverMessage( "Roboproc", cmd);
      if (stat != -1)
         break;
      else
         maxtries--;
      usleep(100000); /* sleep 100 msec */

      DPRINT(1,"sendRoboCommand: Trying Again to deliverMessage\n");

      if (maxtries == 0)
      {
         DPRINT(0, "EXPPROC UNABLE TO TALK TO ROBOPROC MSGQ\n");
         DPRINT(0, "INCORRECT SAMPLE CHANGER OPERATION MAY RESULT\n");
         break;
      }
   }
}

int roboClear(char *args)
{
   if ( ! chkTaskActive(ROBOPROC) )
   {
      DPRINT(1, "Roboproc not active, just return\n");
      return ( 0 );
   }
   DPRINT(1, "Expproc sending \'roboclear\' to Roboproc\n");

   sendRoboCommand("roboclear");
   return ( 0 );
}

int robotMessage(char *args)
{
   char *returnInterface;
   char *authInfo;
   char *token;
   char delimiter_2[ 2 ];
   char msg[256];
   int msg_len, tlen;
   int started = startATask(ROBOPROC);  /* if not started then start it */

   returnInterface = strtok( NULL, "\n" );
   authInfo = strtok( NULL, "\n" );
   msg[0] = '\0';
   msg_len = 0;
   delimiter_2[ 0 ] = DELIMITER_2;
   delimiter_2[ 1 ] = '\0';
	/* sprintf( &msg[ 0 ], "%d%c", XPARSER, DELIMITER_2 );
	 * msg_len = strlen( &msg[ 0 ] );
         */

   while ((token = strtok( NULL, "\n" )) != NULL) {
   	tlen = strlen( token ) + strlen( &delimiter_2[ 0 ] );
   	msg_len += tlen;
   	strcat( &msg[ 0 ], token );
   	strcat( &msg[ 0 ], &delimiter_2[ 0 ] );
   }

   DPRINT1(1, "EXPPROC RECEIVED ROBOT MESSAGE %s\n", msg);

   if (started)
   {
      sendRoboCommand("roboready");
   }
   sendRoboCommand(msg);
   deliverMessage( returnInterface, "started" );
   return ( 0 );
}

int queueSample(char *args)
{
   char *parstr1, *parstr2, *parstr3, *parstr4, parm[32];
   int started = startATask(ROBOPROC);  /* if not started then start it */

   parstr1 = strtok( NULL, " " );
   parstr2 = strtok( NULL, " " );
   parstr3 = strtok( NULL, " " );
   parstr4 = strtok( NULL, "\n" );

   DPRINT4(1, "EXPPROC RECEIVED QUEUESAMPLE %s %s %s %s MESSAGE\n",
           parstr1, parstr2, parstr3, parstr4);

   sprintf(parm, "queuesmp %s %s %s %s\n", parstr1, parstr2,
           parstr3, parstr4);

   DPRINT1(1, "EXPPROC SENDING \'%s\' TO ROBOPROC\n", parm);
   if (started)
   {
      /* If just started Roboproc, send command until Roboproc
       * says it is ready.
       */
      sendRoboCommand("roboready");
   }

   sendRoboCommand(parm);
   return ( 0 );
}

int atCmd()
{
   int started = startATask(ATPROC);  /* if not started then start it */

   if (started == 1) /* Initial start does not need a message */
     return(0);
   else if ( ! started)  /* Already running, so send a message */
   {
      deliverMessage( "Atproc", "atcmd");
      DPRINT(1,"EXPPROC SENDING \'atcmd\' TO ATPROC\n");
   }
   return ( 0 );
}


int relayCmd(char *args)
{
   char	*tmpstr;
   MSG_Q_ID pMsgQ; 
   char procMember[128];

   tmpstr = strtok( NULL, " " );
   strcpy(procMember,(tmpstr+1));
   tmpstr = strtok( NULL, "\n" );
   pMsgQ = openMsgQ(procMember);
   if ( pMsgQ != NULL)
   {
      DPRINT2(1,"Send %s: '%s'\n", procMember, tmpstr);
      sendMsgQ(pMsgQ,tmpstr,strlen(tmpstr),MSGQ_NORMAL,
				WAIT_FOREVER);
      closeMsgQ(pMsgQ);
   }
   return(0);
}

int masCmd(char *msg)
{
   int started = startATask(MASPROC);  /* if not started then start it */
   if (started)
   {
      sendRoboCommand("roboready");
   }
   sendRoboCommand(msg);
   return(0);
}

