/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>

#include "errLogLib.h"
#include "mfileObj.h"
#include "shrMLib.h"
#include "procQfuncs.h"

#ifdef XXX
/* This one entry in one of the processing queues */
typedef struct _proctype {
		int 	procType;	/* WBS, WFID, WERR, WEXP, WEXP(WAIT) */
		int	qSize;		/* Queue Size */
		int	qPri;		/* Queue Priority */
		char 	*procStr;	/* Name of processing type */
		}  procType;

static procType procProps[NUM_OF_QUEUES + 1] = {
		{ WEXP_WAIT, WEXP_WAIT_Q_SIZE, WEXP_WAIT_PRI, "Exp(Wait)" },
		{ WEXP, WEXP_Q_SIZE, WEXP_PRI, "Exp" },
		{ WERR, WERR_Q_SIZE, WERR_PRI, "Error" },
		{ WFID, WFID_Q_SIZE, WFID_PRI, "NT" },
		{ WBS,  WBS_Q_SIZE,  WBS_PRI,  "BS" },
		{ -1,  0,  0,  "Undefined" }
		     };
#endif

/* array of defined processing types and their names */
static char *procNames[NUM_OF_QUEUES+1] = {
					   "Exp(Wait)",
					   "Exp",
					   "Error",
					   "NT",
					   "BS",
					   "Undefined"
					};

/* This one entry in one of the processing queues */
typedef struct _procQentry {
		unsigned int 	fidId;		/* Fid Element, fid# 1,2,3, etc.. */
		unsigned int 	ctNum;		/* CT */
		int 		doneCode;
		int 		errorCode;
		int 		procType;	/* Wexp, Wsu, Werr, Wnt, Wbs */
		char 		ExpIdStr[EXPID_LEN];/* Name of ExpInfo File, dir is assumed */
		}  procQentry;

/* This one entry in the active processing queue */
typedef struct _activeQentry {
		char 		ExpIdStr[EXPID_LEN];	/* Name of ExpInfo File, directory is assumed */
		int 		procType;	/* Wexp, Wsu, Werr, Wnt, Wbs */
		unsigned int 	fidId;		/* Fid Element, fid# 1,2,3, etc.. */
		unsigned int 	ctNum;		/* CT */
		int 		doneCode;
		int 		errorCode;
		int		FgBgFlag;	/* Processing being done in Fg or Bg */
		int		ProcPid;	/*  FG or BG process pid */
		} activeQentry;
		
/* This is a Queue, processing or active */
typedef struct _procQ {
			int	numInQ;
		      } procQ;
		
/* This is a Queue, processing or active */
typedef struct _procList {
			SHR_MEM_ID shrMem;
			procQ	*procQstart;
		      } procList;
		
/* Pointers to the mmapped queues, by mmapping these queues if the process has to be
   restarted all queued information is still present.
*/

/* the list of processing queues */
static procList  procQlist;
static procList  activeQ;

int getTotalProcQsize();

/* There is now a single processing queue. It is a first in first out queue.
   If the last item in the queue
   is a Wnt or Wbs, that last item is replaced with the incoming item.
 */
/* Old way:Each processing type has it's own queue, with a special one.
   Werr 
   Wexp with wait (au(wait)) has a seperate Q.
   Wexp
   Wsu 
   Wnt
   Wbs

  Thus when  mulitply request come in, If there is already one 
  in the queue (Wnt,Wbs) then the request is ignored. Since
  Active processing removes them from the queue.


*/

/*************************************************************
*  initProcQs -  setup & initialize processing Queues 
*
*  1. MMAP the file that will be the persistent image of the
*     processing queues.
*  2. For each Queue, initialize the procQ structure. 
*  3. If clean flag is not set do not initialize number in queue.
*
* RETURNS:
* 0 , else -1
*
*       Author Greg Brissey 9/2/94
*/
int initProcQs(int clean)
{
  int qSize,retstat;
  procQ *queue;
#ifdef PROC_DEBUG
  char *startOffset;
#endif
  SHR_MEM_ID smem;

  retstat = 0;

  qSize = getTotalProcQsize();
  /* size of queue = ((procQ + (qentry * nQentries)) * nQueues) */

   smem = shrmCreate(SHR_PROCQ_PATH,SHR_PROCQ_KEY, qSize);
  if (smem == NULL)
  {
     errLogSysRet(ErrLogOp,debugInfo,"initProcQs: shrmCreate failed:");
     return(0);
  }

  smem->shrmem->newByteLen = qSize;		/* Set mmap file to size of queue */

     /* first sizeof(procQ) bytes are for the procQ structure, after it comes
        the Qentries
     */
     queue = (procQ *) smem->shrmem->offsetAddr;		  	/* procQ */
     procQlist.procQstart = queue;
     procQlist.shrMem = smem;

#ifdef PROC_DEBUG
     startOffset = (char*) ((char*)queue - (char*)procQlist.procQstart);
     DPRINT3(3,"ProcQ Addr: 0x%lx, len %d, End Addr: 0x%lx\n",
	queue, sizeof(procQ), ((char*)queue + sizeof(procQ)) - (char*)1);
     DPRINT3(3,"ProcQ Offset: 0x%lx, len %d, End Offset: 0x%lx\n",
	 startOffset, sizeof(procQ), (startOffset + sizeof(procQ) - (char*)1));
#endif

     if (clean)				/* if clean flag set set in queue to zero */
        queue->numInQ = 0;
     smem->shrmem->offsetAddr += sizeof(procQ);  	/* move past procQ structure */
     
#ifdef DEBUG
#ifdef XXX
     if (DebugLevel >= 4)
     {
       procQentry *Qentries;
       char *endAddr;
       Qentries = (procQentry *)((char*)queue + sizeof(procQ));
       startOffset = (char*) ((char*)Qentries - (char*)procQlist.procQstart);
       for(j=0; j < qSize; j++)
       {
         fprintf(stdout,"    Queue Entry %d : 0x%lx, len %d, 0x%lx\n",
	    j+1,startOffset , sizeof(procQentry),sizeof(procQentry));
	    startOffset += sizeof(procQentry);
       }
       endAddr = (char*)procQlist.procQstart;
       fprintf(stdout,"    Q Entry End: Offset: 0x%lx, End Addr: 0x%lx\n",
	  startOffset, (char*)((unsigned long)endAddr + (unsigned long)startOffset));
     }
#endif
#endif
     
  return( retstat );  
}   

/*************************************************************
*  procQadd -  add a new queue entry to the bottom of the queue
*
* add the entry given for the given queue.
* for Wnt or Wbs queues, if a queue entry is present then do
* not add another, i.e. skip it.
*
* RETURNS:
* 0 , else -1
*
*       Author Greg Brissey 9/2/94
*/
int procQadd(int proctype, char* expidstr, int elemId, int ct, int dcode, int ecode)
{
  procQentry *Qentries;
  procQ *queue;
  sigset_t    blockmask, savemask;
#ifdef PROC_DEBUG
  char *startOffset;
#endif
 
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
 
  /* don't what to be interrupted while changing the Q */
  if (sigprocmask(SIG_BLOCK, &blockmask, &savemask) < 0) 
  {
      errLogSysRet(ErrLogOp,debugInfo,"procQadd: sigprocmask failed");
      return( -1 );
  }
    
  queue = procQlist.procQstart;
  if (queue == NULL)
  {
    errLogRet(ErrLogOp,debugInfo,"procQadd: Queue pointer is NULL\n");
    sigprocmask(SIG_SETMASK, &savemask, (sigset_t *)NULL);
    return(-1);
  }

  if (queue->numInQ == QENTRIES)
  {
     errLogRet(ErrLogOp,debugInfo,
      "Queue full, %s processing lost for fid %d, ct %d (%d %d)!\n",
       procNames[proctype],elemId,ct,dcode,ecode);
     sigprocmask(SIG_SETMASK, &savemask, (sigset_t *)NULL);
     return(-1);
  }

  Qentries = (procQentry *)((char*)queue + sizeof(procQ));
  if (Qentries == NULL)
  {
    errLogRet(ErrLogOp,debugInfo,"procQadd: Queue Entry pointer is NULL\n");
    sigprocmask(SIG_SETMASK, &savemask, (sigset_t *)NULL);
    return(-1);
  }
#ifdef PROC_DEBUG
  startOffset = (char*) ((char*)queue - (char*)procQlist.procQstart);
  DPRINT2(4,"\n\nprocQadd: %d, ProcQ Offset: 0x%lx, \n", proctype+1, startOffset);

  DPRINT5(4,
	"Addrs: Qentry: 0x%lx, procType: 0x%lx, fid: 0x%lx, ct: 0x%lx, str: 0x%lx\n",
	Qentries, &Qentries[queue->numInQ].procType, &Qentries[queue->numInQ].fidId,
	&Qentries[queue->numInQ].ctNum, Qentries[queue->numInQ].ExpIdStr);
  DPRINT3(4,
	"Addrs: Qentry: 0x%lx, doneCode: 0x%lx, errorCode: 0x%lx\n",
	Qentries, &Qentries[queue->numInQ].doneCode, &Qentries[queue->numInQ].errorCode);
  DPRINT5(4,"Offset: Qentry: 0x%lx, procType: 0x%lx, fid: 0x%lx, ct: 0x%lx, str: 0x%lx\n",
  	  ((char*) ((char*)Qentries - (char*)procQlist.procQstart)),
  	  ((char*) ((char*)&Qentries[queue->numInQ].procType - (char*)procQlist.procQstart)),
  	  ((char*) ((char*)&Qentries[queue->numInQ].fidId - (char*)procQlist.procQstart)),
  	  ((char*) ((char*)&Qentries[queue->numInQ].ctNum - (char*)procQlist.procQstart)),
  	  ((char*) ((char*)Qentries[queue->numInQ].ExpIdStr - (char*)procQlist.procQstart)));
#endif


  shrmTake(procQlist.shrMem);

   /* If the last entry in the queue is WBS or WFID
    * just replace it.
    */
   if ( (queue->numInQ) && 
        ( (Qentries[queue->numInQ - 1].procType == WBS) ||
          (Qentries[queue->numInQ - 1].procType == WFID)) )
   {
      queue->numInQ--;
   }
   Qentries[queue->numInQ].procType = proctype;
   Qentries[queue->numInQ].fidId = elemId;
   Qentries[queue->numInQ].ctNum = ct;
   Qentries[queue->numInQ].doneCode = dcode;
   Qentries[queue->numInQ].errorCode = ecode;
   strncpy(Qentries[queue->numInQ].ExpIdStr,expidstr,EXPID_LEN-1);
   Qentries[queue->numInQ].ExpIdStr[EXPID_LEN-1] = '\0';
   queue->numInQ++;

  shrmGive(procQlist.shrMem);

  /* release the signals block, note a signal maybe delieved prior to returning
     from the sigprocmask call
  */
  sigprocmask(SIG_SETMASK, &savemask, (sigset_t *)NULL);

  return(OK);
}

/*************************************************************
*  procQget -  get the next highest priority process queued 
*
*   get the next highest priority queued when processing
*   and deletes it from the queue.
*
* RETURNS:
* 0 , else -1
*
*       Author Greg Brissey 9/2/94
*/
int procQget(int* proctype, char* expidstr, int* elemId, int* ct, int* dcode, int* ecode)
{
  procQentry *Qentries;
  procQ *queue;
  int stat;
  sigset_t    blockmask, savemask;
#ifdef PROC_DEBUG
  char* startOffset;
#endif
 
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
 
  /* don't what to be interrupted while changing the Q */
  if (sigprocmask(SIG_BLOCK, &blockmask, &savemask) < 0) 
  {
      errLogSysRet(ErrLogOp,debugInfo,"procQadd: sigprocmask failed");
      return( -1 );
  }
    
  stat = -1;
     queue = procQlist.procQstart;	/* procQlist[] in priority order */
     if (queue == NULL)
     {
       errLogRet(ErrLogOp,debugInfo,"procQadd: Queue pointer is NULL\n");
       sigprocmask(SIG_SETMASK, &savemask, (sigset_t *)NULL);
       return(-1);
     }

     if (queue->numInQ == 0)	/* non in this Q */
       return(-1);

#ifdef PROC_DEBUG
     startOffset = (char*) ((char*)queue - (char*)procQlist.procQstart);
     DPRINT2(4,"\nprocQget: ProcQ Addr: 0x%lx, Offset: 0x%lx, \n", queue, startOffset);
#endif

     stat = 0;
     Qentries = (procQentry *)((char*)queue + sizeof(procQ));
     if (Qentries == NULL)
     {
       errLogRet(ErrLogOp,debugInfo,"procQadd: Queue Entry pointer is NULL\n");
       sigprocmask(SIG_SETMASK, &savemask, (sigset_t *)NULL);
       return(-1);
     }
#ifdef PROC_DEBUG
     DPRINT5(4,
	"Addrs: Qentry: 0x%lx, procType: 0x%lx, fid: 0x%lx, ct: 0x%lx, str: 0x%lx\n",
	Qentries, &Qentries[0].procType, &Qentries[0].fidId,
	&Qentries[0].ctNum, Qentries[0].ExpIdStr);
     DPRINT5(4,"Offset: Qentry: 0x%lx, procType: 0x%lx, fid: 0x%lx, ct: 0x%lx, str: 0x%lx\n",
  	  ((char*) ((char*)Qentries - (char*)procQlist.procQstart)),
  	  ((char*) ((char*)&Qentries[0].procType - (char*)procQlist.procQstart)),
  	  ((char*) ((char*)&Qentries[0].fidId - (char*)procQlist.procQstart)),
  	  ((char*) ((char*)&Qentries[0].ctNum - (char*)procQlist.procQstart)),
  	  ((char*) ((char*)Qentries[0].ExpIdStr - (char*)procQlist.procQstart)));
#endif

     *proctype = Qentries[0].procType;
     *elemId = Qentries[0].fidId;
     *ct = Qentries[0].ctNum;
     *dcode = Qentries[0].doneCode;
     *ecode = Qentries[0].errorCode;
     strncpy(expidstr,Qentries[0].ExpIdStr,EXPID_LEN);

  /* release the signals block, note a signal maybe delieved prior to returninf
     from the sigprocmask call
  */
  sigprocmask(SIG_SETMASK, &savemask, (sigset_t *)NULL);

  return(stat);
}

/*************************************************************
*  procQentries -  return the number of entries queued 
*
*   return the number of entries queued 
*
* RETURNS:
*   number of entries in queue, else -1
*
*       Author Greg Brissey 8/2/95
*/
int procQentries()
{
  procQ *queue;

     queue = procQlist.procQstart;
     if (queue == NULL)
     {
       errLogRet(ErrLogOp,debugInfo,"procQadd: Queue pointer is NULL\n");
       return(-1);
     }
  return(queue->numInQ);
}

/*************************************************************
*  procQdelete -  Delete Queue Entry 
*
*  Delete the entry given from the given queue.
*
* RETURNS:
* 0 , else -1
*
*       Author Greg Brissey 9/2/94
*/
int procQdelete(int proctype, int entry)
{
  int i;
  procQentry *Qentries;
  procQ *queue;
  sigset_t    blockmask, savemask;

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
 
  /* don't what to be interrupted while changing the Q */
  if (sigprocmask(SIG_BLOCK, &blockmask, &savemask) < 0) 
  {
      errLogSysRet(ErrLogOp,debugInfo,"procQdelete: sigprocmask failed");
      return( -1 );
  }
    
  queue = procQlist.procQstart;
  if (queue == NULL)
  {
    errLogRet(ErrLogOp,debugInfo,"procQdelete: Queue pointer is NULL\n");
    sigprocmask(SIG_SETMASK, &savemask, (sigset_t *)NULL);
    return(-1);
  }
  if (queue->numInQ == 0)
  {
    sigprocmask(SIG_SETMASK, &savemask, (sigset_t *)NULL);
    return(OK);
  }

  if (entry > queue->numInQ)
  {
    errLogRet(ErrLogOp,debugInfo,"procQdelete: Entry doen't Exist\n");
    sigprocmask(SIG_SETMASK, &savemask, (sigset_t *)NULL);
    return(-1);
  }
  
  Qentries = (procQentry *)((char*)queue + sizeof(procQ));
  if (Qentries == NULL)
  {
    errLogRet(ErrLogOp,debugInfo,"procQdelete: Queue Entry pointer is NULL\n");
    sigprocmask(SIG_SETMASK, &savemask, (sigset_t *)NULL);
    return(-1);
  }

  shrmTake(procQlist.shrMem);

   /* delete by copying over add shifting all others down */
   for(i=(entry-1); i < queue->numInQ - 1; i++)
   {
     Qentries[i].procType = Qentries[i+1].procType;
     Qentries[i].fidId = Qentries[i+1].fidId;
     Qentries[i].ctNum = Qentries[i+1].ctNum;
     Qentries[i].doneCode = Qentries[i+1].doneCode;
     Qentries[i].errorCode = Qentries[i+1].errorCode;
     strncpy(Qentries[i].ExpIdStr,Qentries[i+1].ExpIdStr,EXPID_LEN);
   }
   /* It is possible for numInQ to be set to zero between the statements where
    * it is checked to see if it is zero and the shrmTake above.  We want to
    * make sure it does not go negative.
    */
   if (queue->numInQ > 0)
      queue->numInQ--;
   else
      queue->numInQ = 0;

  shrmGive(procQlist.shrMem);

  /* release the signals block, note a signal maybe delieved prior to returninf
     from the sigprocmask call
  */
  sigprocmask(SIG_SETMASK, &savemask, (sigset_t *)NULL);

  return(OK);
}

/*************************************************************
*  procQclean -  reset all process Q to no entries 
*
*
* RETURNS:
* 0 , else -1
*
*       Author Greg Brissey 9/2/94
*/
int procQclean(void)
{
  procQ *queue;
  sigset_t    blockmask, savemask;

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
 
  /* don't what to be interrupted while changing the Q */
  if (sigprocmask(SIG_BLOCK, &blockmask, &savemask) < 0) 
  {
      errLogSysRet(ErrLogOp,debugInfo,"procQdelete: sigprocmask failed");
      return( -1 );
  }
    
  shrmTake(procQlist.shrMem);   /* OK, all the Queues are using the same shrMem */
     queue = procQlist.procQstart;
     queue->numInQ = 0;

  shrmGive(procQlist.shrMem);

  /* release the signals block, note a signal maybe delieved prior to returninf
     from the sigprocmask call
  */
  sigprocmask(SIG_SETMASK, &savemask, (sigset_t *)NULL);

  return(0);
}

/*************************************************************
*  procQRelease -  release share memory of queues 
*
* RETURNS:
* void
*
*       Author Greg Brissey 9/12/94
*/
void procQRelease()
{
  shrmRelease(procQlist.shrMem);
}
/*************************************************************
*  procQshow -  print the contents of all processing Qs
*
* RETURNS:
* 0 , else -1
*
*       Author Greg Brissey 9/2/94
*/
void procQshow(void )
{
  int j;
  procQentry *Qentries;
  procQ *queue;

     queue = procQlist.procQstart;
     Qentries = (procQentry *)((char*)queue + sizeof(procQ));
     fprintf(stdout,"\nQueue,  %d in Q, 1st Entry Addr: 0x%p\n",
	queue->numInQ,Qentries);
     for( j=0; j < queue->numInQ; j++)
     {
	fprintf(stdout,"     (%d): ExpId: '%s', FID: %d, CT: %d Done: %d Error: %d\n",
		j+1,Qentries[j].ExpIdStr, Qentries[j].fidId, Qentries[j].ctNum,
		Qentries[j].doneCode, Qentries[j].errorCode);
     }
}

/*************************************************************
*  initActiveQ -  setup & initialize processing Queues 
*
*  1. MMAP the file that will be the persistent image of the
*     processing queues.
*  2. Initialize the procQ structure. 
*  3. If clean flag is not set do not initialize number in queue.
*
* RETURNS:
* 0 , else -1
*
*       Author Greg Brissey 9/2/94
*/
int initActiveQ(int clean)
{
  int qSize;
  procQ *queue;
  SHR_MEM_ID smem;


  /* size of queue = ((procQ + (qentry * nQentries)) * nQueues) */
  qSize = ((sizeof(procQ) + (sizeof(activeQentry) * NUM_IN_ACT_Q)));

  smem = shrmCreate(SHR_ACTIVEQ_PATH,SHR_ACTIVEQ_KEY, qSize);
  if (smem == NULL)
  {
     /* if mOpen fails on queues no sense in running */
     errLogSysRet(ErrLogOp,debugInfo,"initActiveQ: shrmCreate failed:");
     return(0);
  }

  smem->shrmem->newByteLen = qSize;			/* Set mmap file to size of queue */
  queue = (procQ *) smem->shrmem->offsetAddr;	  	/* procQ */
  activeQ.procQstart = queue;   /* procQlist[] in priority order */
  activeQ.shrMem = smem;   /* procQlist[] in priority order */
  if (clean)					/* if clean flag set set in queue to zero */
     queue->numInQ = 0;
  smem->shrmem->offsetAddr += sizeof(procQ);  	/* move past procQ structure */
  return(0);
}

/*************************************************************
*  activeQadd -  add a new queue entry to the bottom of the queue
*
*
* RETURNS:
* 0 , else -1
*
*       Author Greg Brissey 9/2/94
*/
int activeQadd(char* expidstr, int proctype, int elemId, int ct, int FgBg, int procpid,
           int dcode, int ecode)
{
  procQ *queue;
  activeQentry *Qentries;
  sigset_t    blockmask, savemask;
 
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
 
  /* don't what to be interrupted while changing the Q */
  if (sigprocmask(SIG_BLOCK, &blockmask, &savemask) < 0) 
  {
      errLogSysRet(ErrLogOp,debugInfo,"procQadd: sigprocmask failed");
      return( -1 );
  }
    
  queue = activeQ.procQstart;
  if (queue == NULL)
  {
    errLogRet(ErrLogOp,debugInfo,"activeQadd: Queue pointer is NULL\n");
    sigprocmask(SIG_SETMASK, &savemask, (sigset_t *)NULL);
    return(-1);
  }

  Qentries = (activeQentry *)((char*)queue + sizeof(procQ));
  if (Qentries == NULL)
  {
    errLogRet(ErrLogOp,debugInfo,"activeQadd: Queue Entry pointer is NULL\n");
    sigprocmask(SIG_SETMASK, &savemask, (sigset_t *)NULL);
    return(-1);
  }
  shrmTake(activeQ.shrMem);
   Qentries[queue->numInQ].procType = proctype;
   Qentries[queue->numInQ].fidId = elemId;
   Qentries[queue->numInQ].ctNum = ct;
   Qentries[queue->numInQ].doneCode = dcode;
   Qentries[queue->numInQ].errorCode = ecode;
   Qentries[queue->numInQ].FgBgFlag = FgBg;
   Qentries[queue->numInQ].ProcPid = procpid;
   strncpy(Qentries[queue->numInQ].ExpIdStr,expidstr,EXPID_LEN-1);
   queue->numInQ++;
  shrmGive(activeQ.shrMem);

  /* release the signals block, note a signal maybe delieved prior to returninf
     from the sigprocmask call
  */
  sigprocmask(SIG_SETMASK, &savemask, (sigset_t *)NULL);

  return(queue->numInQ - 1);
}

/*************************************************************
*  activeQget -  get the processing presently active 
*
*   get the next highest priority queued when processing
*   and deletes it from the queue.
*
* RETURNS:
* 0 , else -1
*
*       Author Greg Brissey 9/2/94
*/
int activeQget(int* proctype, char* expidstr, int* fgbg, int* pid, int* dcode)
{
  procQ *queue;
  activeQentry *Qentries;
  sigset_t    blockmask, savemask;
 
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
 
  /* don't what to be interrupted while changing the Q */
  if (sigprocmask(SIG_BLOCK, &blockmask, &savemask) < 0) 
  {
      errLogSysRet(ErrLogOp,debugInfo,"activeQget: sigprocmask failed");
      return( -1 );
  }

  queue = activeQ.procQstart;
  if (queue == NULL)
  {
    errLogRet(ErrLogOp,debugInfo,"activeQget: Queue pointer is NULL\n");
    sigprocmask(SIG_SETMASK, &savemask, (sigset_t *)NULL);
    return(-1);
  }

  if (queue->numInQ == 0)	/* non in this Q, goto next */
  {
    sigprocmask(SIG_SETMASK, &savemask, (sigset_t *)NULL);
    return(-1);
  }

  Qentries = (activeQentry *)((char*)queue + sizeof(procQ));
  if (Qentries == NULL)
  {
    errLogRet(ErrLogOp,debugInfo,"activeQget: Queue Entry pointer is NULL\n");
    sigprocmask(SIG_SETMASK, &savemask, (sigset_t *)NULL);
    return(-1);
  }

  shrmTake(activeQ.shrMem);

   *proctype = Qentries[0].procType;
   *fgbg = Qentries[0].FgBgFlag;
   *pid = Qentries[0].ProcPid;
   *dcode = Qentries[0].doneCode;
   strncpy(expidstr, Qentries[0].ExpIdStr,EXPID_LEN);

  shrmGive(activeQ.shrMem);

  /* release the signals block, note a signal maybe delieved prior to returninf
     from the sigprocmask call
  */
  sigprocmask(SIG_SETMASK, &savemask, (sigset_t *)NULL);

  return(0);

}

/*************************************************************
*  activeProcQentries -  return the number of entries queued 
*
*   return the number of entries queued 
*
* RETURNS:
*   number of entries in queue, else -1
*
*       Author Greg Brissey 8/2/95
*/
int activeProcQentries()
{
  procQ *queue;

  queue = activeQ.procQstart;
  if (queue == NULL)
  {
    errLogRet(ErrLogOp,debugInfo,"activeQadd: Queue pointer is NULL\n");
    return(-1);
  }

  return(queue->numInQ);
}

/*************************************************************
*  activeQdelete -  delete entry base on FG/BG and Key 
*
* RETURNS:
* 0 , else -1
*
*       Author Greg Brissey 9/2/94
*/
int activeQdelete(int fgbg, int key)
{
  int i,entry;
  activeQentry *Qentries;
  procQ *queue;
  sigset_t    blockmask, savemask;

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
 
  /* don't what to be interrupted while changing the Q */
  if (sigprocmask(SIG_BLOCK, &blockmask, &savemask) < 0) 
  {
      errLogSysRet(ErrLogOp,debugInfo,"procQdelete: sigprocmask failed");
      return( -1 );
  }
    
  queue = activeQ.procQstart;
  if (queue == NULL)
  {
    errLogRet(ErrLogOp,debugInfo,"activeQdelete: Queue pointer is NULL\n");
    sigprocmask(SIG_SETMASK, &savemask, (sigset_t *)NULL);
    return(-1);
  }
  if (queue->numInQ == 0)
  {
    sigprocmask(SIG_SETMASK, &savemask, (sigset_t *)NULL);
    return(OK);
  }

  Qentries = (activeQentry *)((char*)queue + sizeof(procQ));
  if (Qentries == NULL)
  {
     errLogRet(ErrLogOp,debugInfo,"activeQdelete: Queue Entry pointer is NULL\n");
     sigprocmask(SIG_SETMASK, &savemask, (sigset_t *)NULL);
     return(-1);
  }

  
  /* search for FG/BG and key */
  entry = -1;
  for(i=0; i < queue->numInQ; i++)
  {
     if( (fgbg == Qentries[i].FgBgFlag) && (key == Qentries[i].ProcPid) )
     {
        entry = i;
	break;
     }
  }
  if (entry == -1)
  {
     sigprocmask(SIG_SETMASK, &savemask, (sigset_t *)NULL);
     return(-1);
  }

  /* delete by copying over and shifting all others down */
  shrmTake(activeQ.shrMem);
   for(i=entry; i < queue->numInQ - 1; i++)
   {
    Qentries[i].procType = Qentries[i+1].procType;
    Qentries[i].fidId = Qentries[i+1].fidId;
    Qentries[i].ctNum = Qentries[i+1].ctNum;
    Qentries[i].doneCode = Qentries[i+1].doneCode;
    Qentries[i].errorCode = Qentries[i+1].errorCode;
    Qentries[i].FgBgFlag = Qentries[i+1].FgBgFlag;
    Qentries[i].ProcPid = Qentries[i+1].ProcPid;
    strncpy(Qentries[i].ExpIdStr,Qentries[i+1].ExpIdStr,EXPID_LEN);
   }
   queue->numInQ--;
  shrmGive(activeQ.shrMem);

  /* release the signals block, note a signal maybe delieved prior to returninf
     from the sigprocmask call
  */
  sigprocmask(SIG_SETMASK, &savemask, (sigset_t *)NULL);

  return(OK);
}

/*************************************************************
*  activeQtoBG -  reset from FG to BG
*
* RETURNS:
* 0 , else -1
*
*       Author Greg Brissey 9/2/94
*/
int activeQtoBG(int oldfgbg, int key, int newfgbg, int procpid)
{
  int i,entry;
  activeQentry *Qentries;
  procQ *queue;
  sigset_t    blockmask, savemask;

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
 
  /* don't what to be interrupted while changing the Q */
  if (sigprocmask(SIG_BLOCK, &blockmask, &savemask) < 0) 
  {
      errLogSysRet(ErrLogOp,debugInfo,"procQdelete: sigprocmask failed");
      return( -1 );
  }
    
  queue = activeQ.procQstart;
  if (queue == NULL)
  {
    errLogRet(ErrLogOp,debugInfo,"activeQdelete: Queue pointer is NULL\n");
    sigprocmask(SIG_SETMASK, &savemask, (sigset_t *)NULL);
    return(-1);
  }
  if (queue->numInQ == 0)
  {
    sigprocmask(SIG_SETMASK, &savemask, (sigset_t *)NULL);
    return(OK);
  }

  Qentries = (activeQentry *)((char*)queue + sizeof(procQ));
  if (Qentries == NULL)
  {
     errLogRet(ErrLogOp,debugInfo,"activeQdelete: Queue Entry pointer is NULL\n");
     sigprocmask(SIG_SETMASK, &savemask, (sigset_t *)NULL);
     return(-1);
  }

  
  /* search for FG/BG and key */
  entry = -1;
  for(i=0; i < queue->numInQ; i++)
  {
     if( (oldfgbg == Qentries[i].FgBgFlag) && (key == Qentries[i].ProcPid) )
     {
        entry = i;
	break;
     }
  }
  if (entry == -1)
  {
     errLogRet(ErrLogOp,debugInfo,"activeQdelete: Entry doen't Exist for key %d\n",
	key);
     sigprocmask(SIG_SETMASK, &savemask, (sigset_t *)NULL);
     return(-1);
  }

  /* reset FgBgFlag and pid */
  shrmTake(activeQ.shrMem);
    Qentries[i].FgBgFlag = newfgbg;
    Qentries[i].ProcPid = procpid;
  shrmGive(activeQ.shrMem);

  /* release the signals block, note a signal maybe delieved prior to returninf
     from the sigprocmask call
  */
  sigprocmask(SIG_SETMASK, &savemask, (sigset_t *)NULL);

  return(OK);
}

/*************************************************************
*  activeQnoWait -  reset from WEXP_WAITT to WEXP
*
* RETURNS:
* 0 , else -1
*
*/
int activeQnoWait(int oldproc, int key, int newproc)
{
  int i,entry;
  activeQentry *Qentries;
  procQ *queue;
  sigset_t    blockmask, savemask;

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
 
  /* don't what to be interrupted while changing the Q */
  if (sigprocmask(SIG_BLOCK, &blockmask, &savemask) < 0) 
  {
      errLogSysRet(ErrLogOp,debugInfo,"procQdelete: sigprocmask failed");
      return( -1 );
  }
    
  queue = activeQ.procQstart;
  if (queue == NULL)
  {
    errLogRet(ErrLogOp,debugInfo,"activeQnoWait: Queue pointer is NULL\n");
    sigprocmask(SIG_SETMASK, &savemask, (sigset_t *)NULL);
    return(-1);
  }
  if (queue->numInQ == 0)
  {
    sigprocmask(SIG_SETMASK, &savemask, (sigset_t *)NULL);
    return(OK);
  }

  Qentries = (activeQentry *)((char*)queue + sizeof(procQ));
  if (Qentries == NULL)
  {
     errLogRet(ErrLogOp,debugInfo,"activeQnoWait: Queue Entry pointer is NULL\n");
     sigprocmask(SIG_SETMASK, &savemask, (sigset_t *)NULL);
     return(-1);
  }

  
  /* search for Proc and key */
  entry = -1;
  for(i=0; i < queue->numInQ; i++)
  {
     if( (oldproc == Qentries[i].procType) && (key == Qentries[i].ProcPid) )
     {
        entry = i;
	break;
     }
  }
  if (entry == -1)
  {
     sigprocmask(SIG_SETMASK, &savemask, (sigset_t *)NULL);
     return(OK);
  }

  /* reset ProcFlag */
  shrmTake(activeQ.shrMem);
    Qentries[i].procType = newproc;
  shrmGive(activeQ.shrMem);

  /* release the signals block, note a signal maybe delieved prior to returninf
     from the sigprocmask call
  */
  sigprocmask(SIG_SETMASK, &savemask, (sigset_t *)NULL);

  return(OK);
}

/*************************************************************
*  activeQclean -  reset number in Q to zero
*
* RETURNS:
* 0 , else -1
*
*       Author Greg Brissey 9/2/94
*/
int activeQclean(void)
{
  procQ *queue;
  sigset_t    blockmask, savemask;

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
 
  /* don't what to be interrupted while changing the Q */
  if (sigprocmask(SIG_BLOCK, &blockmask, &savemask) < 0) 
  {
      errLogSysRet(ErrLogOp,debugInfo,"procQdelete: sigprocmask failed");
      return( -1 );
  }
  queue = activeQ.procQstart;

  shrmTake(activeQ.shrMem);
   queue->numInQ = 0;
  shrmGive(activeQ.shrMem);

  /* release the signals block, note a signal maybe delieved prior to returninf
     from the sigprocmask call
  */
  sigprocmask(SIG_SETMASK, &savemask, (sigset_t *)NULL);

  return(0);
}

/*************************************************************
*  activeQRelease -  release share memory of active queue 
*
* RETURNS:
* void
*
*       Author Greg Brissey 9/12/94
*/
void activeQRelease()
{
  shrmRelease(activeQ.shrMem);
}
    
/*************************************************************
*  activeQshow -  prints contents of active queue
*
* RETURNS:
* 0 , else -1
*
*       Author Greg Brissey 9/2/94
*/
void activeQshow(void)
{
  int j;
  activeQentry *Qentries;
  procQ *queue;

  queue = activeQ.procQstart;
  Qentries = (activeQentry *)((char*)queue + sizeof(procQ));
  fprintf(stdout,"\nActive Queue: %d in Q, 1st Entry Addr: 0x%p\n",
	queue->numInQ,Qentries);
  for( j=0; j < queue->numInQ; j++)
  {
    fprintf(stdout,"     (%d): ExpId: '%s', FID: %d, CT: %d, Done: %d, Error: %d, FGproc: %d, PID: %d\n",
		j+1,Qentries[j].ExpIdStr, Qentries[j].fidId, Qentries[j].ctNum,
		Qentries[j].doneCode, Qentries[j].errorCode,
		Qentries[j].FgBgFlag, Qentries[j].ProcPid);
  }
}

/*************************************************************
*  proctypeName -  returns char* to name of process type
*
* RETURNS:
* char * , else NULL
*
*       Author Greg Brissey 9/2/94
*/
char *proctypeName(int type)
{
   if ((type >= NUM_OF_QUEUES) || (type < 0))
     type = NUM_OF_QUEUES;

   return(procNames[type]);
}

int getTotalProcQsize()
{
   int total;
   total = (sizeof(procQ) + (sizeof(procQentry) * QENTRIES));
   return(total);
}
