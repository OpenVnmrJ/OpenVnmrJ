/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/* for SFU build, stub code for expQ functions is in table.c */
#if !defined(__INTERIX)

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
#include "shrexpinfo.h"
#include "expQfuncs.h"


/* This one entry in one of the processing queues */
/* ExpIdStr is filename of the complete Experiment info file */
/* ExpInfoStr is a quick access string for exp number and user */
typedef struct _expQentry {
		char 	ExpIdStr[EXPID_LEN];/* Name of ExpInfo File */
		char 	ExpInfoStr[EXPID_LEN];
		}  expQentry;

/* This one entry in the active processing queue */
typedef struct _activeQentry {
	 	int     Priority;
		char 	ExpIdStr[EXPID_LEN];	/* Name of ExpInfo File */
		char 	ExpInfoStr[EXPID_LEN];
		} activeQentry;
		
/* This is a Queue, processing or active */
typedef struct _procQ {
			int	numInQ;
		      } procQ;
		
/* This is a Queue, processing or active */
typedef struct _QList {
			SHR_MEM_ID shrMem;
			procQ	*procQstart;
		      } QList;
		
/* Pointers to the mmapped queues, by mmapping these queues if the process has to be
   restarted all queued information is still present.
*/

/* the list of processing queues */
static QList  ExpPendQ[NUM_EXP_OF_QUEUES];
static QList  activeQ;
void unBlockSignals(sigset_t *savemask);
int  blockSignals(sigset_t *savemask);
int  getTotalQsize();


/*************************************************************
*  initExpQ -  setup & initialize processing Queues 
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
int initExpQs(int clean)
{
  int i,qSize,retstat;
  procQ *queue;
  SHR_MEM_ID smem;
#ifdef EXPQ_DEBUG
  char *startOffset;
  expQentry *Qentries;
  char *endAddr;
  int j;
#endif

  retstat = 0;

  qSize = getTotalQsize();

   smem = shrmCreate(SHR_EXPQ_PATH,SHR_EXPQ_KEY, qSize);
  /* ifile = mOpen(PROCQ_NAME,qSize,O_RDWR | O_CREAT); */
  if (smem == NULL)
  {
     errLogSysRet(ErrLogOp,debugInfo,"initExpQs: shrmCreate failed:");
     return(0);
  }

  smem->shrmem->newByteLen = qSize;   /* Set mmap file to size of queue */
  for(i=0; i < NUM_EXP_OF_QUEUES; i++)
  {

     /* first sizeof(procQ) bytes are for the procQ structure, after it comes
        the Qentries
     */
     queue = (procQ *) smem->shrmem->offsetAddr; 	/* procQ */
     ExpPendQ[i].procQstart = queue;   /* ExpPendQ[] in priority order */
     ExpPendQ[i].shrMem = smem;   /* ExpPendQ[] in priority order */

#ifdef EXPQ_DEBUG
     startOffset = (char*) ((char*)queue - (char*)ExpPendQ[0].procQstart);
     DPRINT4(3,"%d, ProcQ Addr: 0x%lx, len %d, End Addr: 0x%lx\n",
	i+1, queue, sizeof(procQ), ((char*)queue + sizeof(procQ)) - (char*)1);
     DPRINT4(3,"%d, ProcQ Offset: 0x%lx, len %d, End Offset: 0x%lx\n",
	i+1, startOffset, sizeof(procQ), (startOffset + sizeof(procQ) - (char*)1));
#endif

     if (clean)				/* if clean flag set set in queue to zero */
        queue->numInQ = 0;
     smem->shrmem->offsetAddr += sizeof(procQ);  	/* move past procQ structure */
     smem->shrmem->offsetAddr += (sizeof(expQentry) * EXP_Q_SIZE);
#ifdef EXPQ_DEBUG
     DPRINT3(3,"    Q Entry Addr: 0x%lx, len %d, 0x%lx\n",
	queue + sizeof(procQ), (sizeof(expQentry) * EXP_Q_SIZE),
	(sizeof(expQentry) * EXP_Q_SIZE));
     
     if (DebugLevel >= 3)
     {
      Qentries = (expQentry *)((char*)queue + sizeof(procQ));
      startOffset = (char*) ((char*)Qentries - (char*)ExpPendQ[0].procQstart);
      for(j=0; j < EXP_Q_SIZE; j++)
      {
        DPRINT4(3,"    Queue Entry %d : 0x%lx, len %d, 0x%lx\n",
	 j+1,startOffset , sizeof(expQentry),sizeof(expQentry));
	 startOffset += sizeof(expQentry);
      }
      endAddr = (char*)ExpPendQ[0].procQstart;
      DPRINT2(3,"    Q Entry End: Offset: 0x%lx, End Addr: 0x%lx\n",
	 startOffset, (char*)((unsigned long)endAddr + (unsigned long)startOffset));
     }
#endif 
     
  }
  return( retstat );  
}   

/*************************************************************
*  expQaddToTail -  add a new queue entry to the bottom of the queue
*
* add the entry given for the given queue.
*
* RETURNS:
* 0 , else -1
*
*       Author Greg Brissey 9/2/94
*/
int expQaddToTail(int priority, char* expidstr, char* expinfostr)
{
  expQentry *Qentries;
  procQ *queue;
  sigset_t    savemask;
#ifdef EXPQ_DEBUG
  char *startOffset;
#endif
 
  blockSignals(&savemask);
    
  queue = ExpPendQ[priority].procQstart;
  if (queue == NULL)
  {
    errLogRet(ErrLogOp,debugInfo,"expQaddToTail: Queue pointer is NULL\n");
    return(-1);
  }

  if (queue->numInQ == EXP_Q_SIZE)
  {
     errLogRet(ErrLogOp,debugInfo,
	"%s: Queue[%d] full, Experiment %s, not queued!\n",
	expinfostr,priority,expidstr);
     return(-1);
  }

  Qentries = (expQentry *)((char*)queue + sizeof(procQ));
  if (Qentries == NULL)
  {
    errLogRet(ErrLogOp,debugInfo,"expQaddToTail: Queue Entry pointer is NULL\n");
    return(-1);
  }
#ifdef EXPQ_DEBUG
  startOffset = (char*) ((char*)queue - (char*)ExpPendQ[0].procQstart);
  DPRINT2(3,"\n\nprocQadd: %d, ProcQ Offset: 0x%lx, \n", priority, startOffset);

  DPRINT2(3,
	"Addrs: Qentry: 0x%lx, str: 0x%lx\n",
	Qentries, Qentries[queue->numInQ].ExpIdStr);
  DPRINT2(3,"Offset: Qentry: 0x%lx, str: 0x%lx\n",
  	  ((char*) ((char*)Qentries - (char*)ExpPendQ[0].procQstart)),
  	  ((char*) ((char*)Qentries[queue->numInQ].ExpIdStr - (char*)ExpPendQ[0].procQstart)));
#endif

  shrmTake(ExpPendQ[priority].shrMem);

  strncpy(Qentries[queue->numInQ].ExpIdStr,expidstr,EXPID_LEN-1);
  strncpy(Qentries[queue->numInQ].ExpInfoStr,expinfostr,EXPID_LEN-1);
  Qentries[queue->numInQ].ExpIdStr[EXPID_LEN-1] = '\0';
  Qentries[queue->numInQ].ExpInfoStr[EXPID_LEN-1] = '\0';
  queue->numInQ++;

  shrmGive(ExpPendQ[priority].shrMem);

  /* release the signals block, note a signal maybe delieved prior to returning
     from the sigprocmask call
  */

  unBlockSignals(&savemask);

  return(OK);
}

/*************************************************************
*  expQaddToHead -  add a new queue entry to the Top of the queue
*
* add the entry given for the given queue.
*
* RETURNS:
* 0 , else -1
*
*       Author Greg Brissey 9/2/94
*/
int expQaddToHead(int priority, char* expidstr, char* expinfostr)
{
  int i;
  expQentry *Qentries;
  procQ *queue;
  sigset_t    savemask;
#ifdef EXPQ_DEBUG
  char *startOffset;
#endif
 
  blockSignals(&savemask);
    
  queue = ExpPendQ[priority].procQstart;
  if (queue == NULL)
  {
    errLogRet(ErrLogOp,debugInfo,"expQaddToTail: Queue pointer is NULL\n");
    return(-1);
  }

  if (queue->numInQ == EXP_Q_SIZE)
  {
     errLogRet(ErrLogOp,debugInfo,
	"%s: Queue[%d] full, Experiment %s, not queued!\n",
	expinfostr,priority,expidstr);
     return(-1);
  }

  Qentries = (expQentry *)((char*)queue + sizeof(procQ));
  if (Qentries == NULL)
  {
    errLogRet(ErrLogOp,debugInfo,"expQaddToTail: Queue Entry pointer is NULL\n");
    return(-1);
  }
#ifdef EXPQ_DEBUG
  startOffset = (char*) ((char*)queue - (char*)ExpPendQ[0].procQstart);
  DPRINT2(3,"\n\nprocQadd: %d, ProcQ Offset: 0x%lx, \n", priority, startOffset);

  DPRINT2(3,
	"Addrs: Qentry: 0x%lx, str: 0x%lx\n",
	Qentries, Qentries[queue->numInQ].ExpIdStr);
  DPRINT2(3,"Offset: Qentry: 0x%lx, str: 0x%lx\n",
  	  ((char*) ((char*)Qentries - (char*)ExpPendQ[0].procQstart)),
  	  ((char*) ((char*)Qentries[queue->numInQ].ExpIdStr - (char*)ExpPendQ[0].procQstart)));
#endif

  shrmTake(ExpPendQ[priority].shrMem);

  /* 1st shift entries back */
  for(i=queue->numInQ; i > 0; i--)
  {
     strncpy(Qentries[i].ExpIdStr,Qentries[i - 1].ExpIdStr,EXPID_LEN);
     strncpy(Qentries[i].ExpInfoStr,Qentries[i - 1].ExpInfoStr,EXPID_LEN);
  }
  /* Now copy in new entry at top of queue */
  strncpy(Qentries[0].ExpIdStr,expidstr,EXPID_LEN-1);
  strncpy(Qentries[0].ExpInfoStr,expinfostr,EXPID_LEN-1);
  Qentries[queue->numInQ].ExpIdStr[EXPID_LEN-1] = '\0';
  Qentries[queue->numInQ].ExpInfoStr[EXPID_LEN-1] = '\0';
  queue->numInQ++;
  shrmGive(ExpPendQ[priority].shrMem);

  /* release the signals block, note a signal maybe delieved prior to returning
     from the sigprocmask call
  */

  unBlockSignals(&savemask);

  return(OK);
}

/*************************************************************
*  expQget -  get the next highest priority process queued 
*
*   get the next highest priority queued when processing
*   and deletes it from the queue.
*
* RETURNS:
* 0 , else -1
*
*       Author Greg Brissey 9/2/94
*/
int expQget(int* priority, char* expidstr)
{
  expQentry *Qentries;
  procQ *queue;
  int i,stat;
  sigset_t    savemask;
#ifdef EXPQ_DEBUG
  char* startOffset;
#endif

  blockSignals(&savemask);
 
  stat = -1;
  /* search in priority order */
  for(i=0; i < NUM_EXP_OF_QUEUES; i++) 
  {
     queue = ExpPendQ[i].procQstart;	/* ExpPendQ[] in priority order */
     if (queue == NULL)
     {
       errLogRet(ErrLogOp,debugInfo,"expQget: Queue pointer is NULL\n");
       return(-1);
     }

     if (queue->numInQ == 0)	/* non in this Q, goto next */
       continue;

#ifdef EXPQ_DEBUG
     startOffset = (char*) ((char*)queue - (char*)ExpPendQ[0].procQstart);
     DPRINT3(3,"\nexpQget: priority %d, ProcQ Addr: 0x%lx, Offset: 0x%lx, \n", 
		i+1, queue, startOffset);
#endif

     stat = 0;
     Qentries = (expQentry *)((char*)queue + sizeof(procQ));
     if (Qentries == NULL)
     {
       errLogRet(ErrLogOp,debugInfo,"expQget: Queue Entry pointer is NULL\n");
       return(-1);
     }
#ifdef EXPQ_DEBUG
     DPRINT2(3,
	"Addrs: Qentry: 0x%lx, str: 0x%lx\n", Qentries, Qentries[0].ExpIdStr);
     DPRINT2(3,"Offset: Qentry: 0x%lx, IdStr: 0x%lx\n",
  	  ((char*) ((char*)Qentries - (char*)ExpPendQ[0].procQstart)),
  	  ((char*) ((char*)Qentries[0].ExpIdStr - (char*)ExpPendQ[0].procQstart)));
#endif

     *priority = i;
     strncpy(expidstr,Qentries[0].ExpIdStr,EXPID_LEN);
     break;
  }

  /* release the signals block, note a signal maybe delieved prior to returninf
     from the sigprocmask call
  */
  unBlockSignals(&savemask);

  return(stat);
}

/*************************************************************
*  expQsearch -  search the Exp Q for the User name & Exp number
*
*   search for an Exp Q entry matching the given User name and Exp
*   number returning the priority and Exp Id String of the match
*   This search relies on the fact that the Info String has the
*   form "exp# username" e.g.  "3 vnmr1"  additional tokens maybe
*   in the string but are ignored only the 1st two must be there
*
* RETURNS:
*  1 - Match Found, 0 - No Match, -1 - Error
*
*       Author Greg Brissey 10/24/96
*/
int expQsearch(char *userName,char* expnum,int *priority,char *expidstr)
{
  expQentry *Qentries;
  procQ *queue;
  int i,stat,qindx;
  sigset_t    savemask;
  char  infostr[EXPID_LEN];
#ifdef EXPQ_DEBUG
  char* startOffset;
#endif


  blockSignals(&savemask);
 
  stat = -1;
  *priority = -1;
  strcpy(expidstr,"");

  /* search in priority order */
  for(i=0; i < NUM_EXP_OF_QUEUES; i++) 
  {
     queue = ExpPendQ[i].procQstart;	/* ExpPendQ[] in priority order */
     if (queue == NULL)
     {
       errLogRet(ErrLogOp,debugInfo,"expQget: Queue pointer is NULL\n");
       return(-1);
     }

     if (queue->numInQ == 0)	/* none in this Q, goto next */
       continue;

#ifdef EXPQ_DEBUG
     startOffset = (char*) ((char*)queue - (char*)ExpPendQ[0].procQstart);
     DPRINT3(3,"\nexpQget: priority %d, ProcQ Addr: 0x%lx, Offset: 0x%lx, \n", 
		i+1, queue, startOffset);
#endif

     stat = 0;
     Qentries = (expQentry *)((char*)queue + sizeof(procQ));
     if (Qentries == NULL)
     {
       errLogRet(ErrLogOp,debugInfo,"expQget: Queue Entry pointer is NULL\n");
       return(-1);
     }
#ifdef EXPQ_DEBUG
     DPRINT2(3,
	"Addrs: Qentry: 0x%lx, str: 0x%lx\n", Qentries, Qentries[0].ExpIdStr);
     DPRINT2(3,"Offset: Qentry: 0x%lx, IdStr: 0x%lx\n",
  	  ((char*) ((char*)Qentries - (char*)ExpPendQ[0].procQstart)),
  	  ((char*) ((char*)Qentries[0].ExpIdStr - (char*)ExpPendQ[0].procQstart)));

     DPRINT2(3,"expQsearch: priority: %d, entries: %d\n",i,queue->numInQ);
#endif
     for( qindx = 0; qindx < queue->numInQ; qindx++)
     {
        char *expN, *user;

	DPRINT2(3,"Id: '%s', Info: '%s'\n",Qentries[qindx].ExpIdStr,Qentries[qindx].ExpInfoStr);
	strncpy(infostr,Qentries[qindx].ExpInfoStr,EXPID_LEN-1);
        expN = strtok(infostr," ");
        if (expN == NULL)
	{
           errLogRet(ErrLogOp,debugInfo,
		"expQsearch: Error, No Exp. # in Exp Q entry: %d, pri: %d, Id: '%s', InfoStr: '%s'.\n",
			qindx, i, Qentries[qindx].ExpIdStr, Qentries[qindx].ExpInfoStr);
	   return(stat);
	}
        user = strtok(NULL," ");
        if (user == NULL)
        {
           errLogRet(ErrLogOp,debugInfo,
		"expQsearch: Error, No User Name in Exp Q entry: %d, pri: %d, Id: '%s', InfoStr: '%s'.\n",
			qindx, i, Qentries[qindx].ExpIdStr, Qentries[qindx].ExpInfoStr);
	   return(stat);
	}
	DPRINT4(3,"ExpN: '%s' =? '%s', User: '%s' =? '%s'\n",expnum,expN,userName,user);
	if ( ( ! strcmp(expnum,expN)) && ( ! strcmp(userName,user)))
        {
           stat = 1;
          *priority = i;
          strncpy(expidstr,Qentries[qindx].ExpIdStr,EXPID_LEN);
  	  unBlockSignals(&savemask);
  	  return(stat);
        }
     }
  }

  /* release the signals block, note a signal maybe delieved prior to returning
     from the sigprocmask call
  */
  unBlockSignals(&savemask);

  return(stat);
}

/*************************************************************
*  expQIdsearch -  search the Exp Q for the ID str
*
*   search for an Exp Q entry matching the given ID str
*   returning the priority of the match
*
* RETURNS:
*  1 - Match Found, 0 - No Match, -1 - Error
*
*       Author Greg Brissey 10/24/96
*/
int expQIdsearch(char *expidstr, int *priority)
{
  expQentry *Qentries;
  procQ *queue;
  int i,stat,qindx;
  sigset_t    savemask;
#ifdef EXPQ_DEBUG
  char* startOffset;
#endif


  blockSignals(&savemask);
 
  stat = -1;
  *priority = -1;

  /* search in priority order */
  for(i=0; i < NUM_EXP_OF_QUEUES; i++) 
  {
     queue = ExpPendQ[i].procQstart;	/* ExpPendQ[] in priority order */
     if (queue == NULL)
     {
       errLogRet(ErrLogOp,debugInfo,"expQget: Queue pointer is NULL\n");
       return(-1);
     }

     if (queue->numInQ == 0)	/* none in this Q, goto next */
       continue;

#ifdef EXPQ_DEBUG
     startOffset = (char*) ((char*)queue - (char*)ExpPendQ[0].procQstart);
     DPRINT3(3,"\nexpQget: priority %d, ProcQ Addr: 0x%lx, Offset: 0x%lx, \n", 
		i+1, queue, startOffset);
#endif

     stat = 0;
     Qentries = (expQentry *)((char*)queue + sizeof(procQ));
     if (Qentries == NULL)
     {
       errLogRet(ErrLogOp,debugInfo,"expQget: Queue Entry pointer is NULL\n");
       return(-1);
     }
#ifdef EXPQ_DEBUG
     DPRINT2(3,
	"Addrs: Qentry: 0x%lx, str: 0x%lx\n", Qentries, Qentries[0].ExpIdStr);
     DPRINT2(3,"Offset: Qentry: 0x%lx, IdStr: 0x%lx\n",
  	  ((char*) ((char*)Qentries - (char*)ExpPendQ[0].procQstart)),
  	  ((char*) ((char*)Qentries[0].ExpIdStr - (char*)ExpPendQ[0].procQstart)));

     DPRINT2(3,"expQsearch: priority: %d, entries: %d\n",i,queue->numInQ);
#endif
     for( qindx = 0; qindx < queue->numInQ; qindx++)
     {
	DPRINT2(3,"Id: '%s', Info: '%s'\n",Qentries[qindx].ExpIdStr,Qentries[qindx].ExpInfoStr);
	if ( ! strcmp(Qentries[qindx].ExpIdStr,expidstr))
        {
           stat = 1;
          *priority = i;
  	  unBlockSignals(&savemask);
  	  return(stat);
        }
     }
  }

  /* release the signals block, note a signal maybe delieved prior to returning
     from the sigprocmask call
  */
  unBlockSignals(&savemask);

  return(stat);
}

int expQgetinfo(int index, char* expinfostr)
{
  expQentry *Qentries;
  procQ *queue;
  int i,stat;
  sigset_t    savemask;
  int qindex = 0;
#ifdef EXPQ_DEBUG
  char* startOffset;
#endif

  blockSignals(&savemask);
 
  stat = -1;
  /* search in priority order */
  for(i=0; i < NUM_EXP_OF_QUEUES; i++) 
  {
     queue = ExpPendQ[i].procQstart;	/* ExpPendQ[] in priority order */
     if (queue == NULL)
     {
       errLogRet(ErrLogOp,debugInfo,"expQget: Queue pointer is NULL\n");
       return(-1);
     }

     if (queue->numInQ == 0)	/* non in this Q, goto next */
       continue;

#ifdef EXPQ_DEBUG
     startOffset = (char*) ((char*)queue - (char*)ExpPendQ[0].procQstart);
     DPRINT3(3,"\nexpQget: priority %d, ProcQ Addr: 0x%lx, Offset: 0x%lx, \n", 
		i+1, queue, startOffset);
#endif

     stat = 0;
     Qentries = (expQentry *)((char*)queue + sizeof(procQ));
     if (Qentries == NULL)
     {
       errLogRet(ErrLogOp,debugInfo,"expQget: Queue Entry pointer is NULL\n");
       return(-1);
     }
#ifdef EXPQ_DEBUG
     DPRINT2(3,
	"Addrs: Qentry: 0x%lx, str: 0x%lx\n", Qentries, Qentries[0].ExpIdStr);
     DPRINT2(3,"Offset: Qentry: 0x%lx, IdStr: 0x%lx\n",
  	  ((char*) ((char*)Qentries - (char*)ExpPendQ[0].procQstart)),
  	  ((char*) ((char*)Qentries[0].ExpIdStr - (char*)ExpPendQ[0].procQstart)));
#endif

     if ( (qindex + queue->numInQ) >= index)
     {
        strncpy(expinfostr,Qentries[index - qindex - 1].ExpInfoStr,EXPID_LEN);
        break;
     }
     else
     {
        qindex += queue->numInQ;
     }
  }

  /* release the signals block, note a signal maybe delieved prior to returninf
     from the sigprocmask call
  */
  unBlockSignals(&savemask);

  return(stat);
}

/*************************************************************
* expQentries - return the number of entries in the expQ
*
*  Return the number of entries within the Experiment
*  Queue.
*
* RETURNS:
*  Number in Queue 
*
*       Author Greg Brissey 9/2/94
*/
int  expQentries()
{
  procQ *queue;
  int i,nEntries;
  sigset_t    savemask;

  blockSignals(&savemask);

  nEntries = 0;
  /* search in priority order */
  for(i=0; i < NUM_EXP_OF_QUEUES; i++) 
  {
     queue = ExpPendQ[i].procQstart;	/* ExpPendQ[] in priority order */
     if (queue == NULL)
     {
       errLogRet(ErrLogOp,debugInfo,"expQentries: Queue pointer is NULL\n");
       return(-1);
     }

     nEntries += queue->numInQ;
  }

  /* release the signals block, note a signal maybe delieved prior to returninf
     from the sigprocmask call
  */
  unBlockSignals(&savemask);

  return(nEntries);
}
/*************************************************************
*  expQdelete -  Delete Queue Entry 
*
*  Delete the entry given from the given queue.
*
* RETURNS:
* 0 , else -1
*
*       Author Greg Brissey 9/2/94
*/
int expQdelete(int priority, char *idstr)
{
  int i;
  int entry;
  expQentry *Qentries;
  procQ *queue;
  sigset_t    savemask;

  blockSignals(&savemask);
 
  queue = ExpPendQ[priority].procQstart;
  if (queue == NULL)
  {
    errLogRet(ErrLogOp,debugInfo,"expQdelete: Queue pointer is NULL\n");
    return(-1);
  }
  if (queue->numInQ == 0)
      return(OK);

  Qentries = (expQentry *)((char*)queue + sizeof(procQ));
  entry = -1;
  for(i=0; i < queue->numInQ; i++)
  {
     if(strncmp(Qentries[i].ExpIdStr,idstr,EXPID_LEN) == 0)
     {
	entry = i;
	break;
     }
  }

  if (entry < 0 )
  {
    errLogRet(ErrLogOp,debugInfo,"expQdelete: Entry doen't Exist\n");
    return(-1);
  }
  
  shrmTake(ExpPendQ[priority].shrMem);
  /* delete by copying over and shifting all others down */
  /* DPRINT2(-1,"expQdelete():  entry: %d, of %d to delete.\n",entry+1,queue->numInQ); */
  for(i=entry; i < queue->numInQ - 1; i++)
  {
     /* DPRINT4(-1," copy entry %d - '%s' over %d - '%s'\n",i+1,Qentries[i+1].ExpIdStr,i,Qentries[i].ExpIdStr); */
     strncpy(Qentries[i].ExpIdStr,Qentries[i+1].ExpIdStr,EXPID_LEN);
     /* DPRINT4(-1," copy entry %d - '%s' over %d - '%s'\n",i+1,Qentries[i+1].ExpInfoStr,i,Qentries[i].ExpInfoStr); */
     strncpy(Qentries[i].ExpInfoStr,Qentries[i+1].ExpInfoStr,EXPID_LEN);
  }
  queue->numInQ--;
  shrmGive(ExpPendQ[priority].shrMem);

  /* release the signals block, note a signal maybe delieved prior to returning
     from the sigprocmask call
  */

  unBlockSignals(&savemask);

  return(OK);
}

/*************************************************************
*  expQclean -  reset all process Q to no entries 
*
*
* RETURNS:
* 0 , else -1
*
*       Author Greg Brissey 9/2/94
*/
void expQclean(void)
{
  int i;
  procQ *queue;
  sigset_t    savemask;

  blockSignals(&savemask);

  shrmTake(ExpPendQ[0].shrMem);   /* OK, all the Queues are using the same shrMem */
  for(i=0; i < NUM_EXP_OF_QUEUES; i++)
  {
     queue = ExpPendQ[i].procQstart;
     queue->numInQ = 0;
  }
  shrmGive(ExpPendQ[0].shrMem);

  /* release the signals block, note a signal maybe delieved prior to returninf
     from the sigprocmask call
  */

  unBlockSignals(&savemask);

  return;
}

/*************************************************************
*  expQRelease -  release share memory of queues 
*
* RETURNS:
* void
*
*       Author Greg Brissey 9/12/94
*/
void expQRelease()
{
  shrmRelease(ExpPendQ[0].shrMem);
}


/*************************************************************
*  expQshow -  print the contents of all processing Qs
*
* RETURNS:
* 0 , else -1
*
*       Author Greg Brissey 9/2/94
*/
int expQshow(void )
{
  int i,j;
  expQentry *Qentries __attribute__((unused));
  procQ *queue;
  int totalq = 0;

  for(i=0; i < NUM_EXP_OF_QUEUES; i++)
  {
     queue = ExpPendQ[i].procQstart;
     Qentries = (expQentry *)((char*)queue + sizeof(procQ));
     /* printf("\n Exp Queue, Priority: %d, In Queue: %d, 1st Entry Addr: 0x%lx\n",
	i, queue->numInQ,Qentries); */
     DPRINT2(-1,"\n Exp Queue, Priority: %d, In Queue: %d\n",
	i, queue->numInQ);
     totalq += queue->numInQ;
     for( j=0; j < queue->numInQ; j++)
     {
	/* printf("     (%d): ExpId:   '%s'\n",j+1,Qentries[j].ExpIdStr); */
	/* printf("     (%d): ExpInfo: '%s'\n",j+1,Qentries[j].ExpInfoStr); */
	DPRINT2(-1,"     (%d): ExpId:   '%s'\n",j+1,Qentries[j].ExpIdStr);
	DPRINT2(-1,"     (%d): ExpInfo: '%s'\n",j+1,Qentries[j].ExpInfoStr);
     }
  }
  return( totalq );
}

/*************************************************************
*  initActiveExpQ -  setup & initialize processing Queues 
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
int initActiveExpQ(int clean)
{
  int qSize;
  procQ *queue;
  SHR_MEM_ID smem;


  /* size of queue = ((procQ + (qentry * nQentries)) * nQueues) */
  qSize = ((sizeof(procQ) + (sizeof(activeQentry) * NUM_IN_ACT_Q)));

  smem = shrmCreate(SHR_ACTIVE_EXPQ_PATH,SHR_ACTIVE_EXPQ_KEY, qSize);
  if (smem == NULL)
  {
     /* if mOpen fails on queues no sense in running */
     errLogSysRet(ErrLogOp,debugInfo,"initActiveQ: shrmCreate failed:");
     return(0);
  }

  smem->shrmem->newByteLen = qSize;			/* Set mmap file to size of queue */
  queue = (procQ *) smem->shrmem->offsetAddr;	  	/* procQ */
  activeQ.procQstart = queue;   /* ExpPendQ[] in priority order */
  activeQ.shrMem = smem;   /* ExpPendQ[] in priority order */
  if (clean)					/* if clean flag set set in queue to zero */
     queue->numInQ = 0;
  smem->shrmem->offsetAddr += sizeof(procQ);  	/* move past procQ structure */
  return(0);
}

/*************************************************************
*  activeExpQadd -  add a new queue entry to the bottom of the queue
*
*
* RETURNS:
* 0 , else -1
*
*       Author Greg Brissey 9/2/94
*/
int activeExpQadd(int priority, char* expidstr, char* expinfostr)
{
  procQ *queue;
  activeQentry *Qentries;
  sigset_t    savemask;
 
  blockSignals(&savemask);
    
  queue = activeQ.procQstart;
  if (queue == NULL)
  {
    errLogRet(ErrLogOp,debugInfo,"activeExpQadd: Queue pointer is NULL\n");
    return(-1);
  }

  Qentries = (activeQentry *)((char*)queue + sizeof(procQ));
  if (Qentries == NULL)
  {
    errLogRet(ErrLogOp,debugInfo,"activeExpQadd: Queue Entry pointer is NULL\n");
    return(-1);
  }
  shrmTake(activeQ.shrMem);
  Qentries[queue->numInQ].Priority = priority;
  strncpy(Qentries[queue->numInQ].ExpIdStr,expidstr,EXPID_LEN-1);
  strncpy(Qentries[queue->numInQ].ExpInfoStr,expinfostr,EXPID_LEN-1);
  queue->numInQ++;
  shrmGive(activeQ.shrMem);

  /* release the signals block, note a signal maybe delieved prior to returning
     from the sigprocmask call
  */
  unBlockSignals(&savemask);

  return(queue->numInQ - 1);
}

/*************************************************************
*  activeExpQget -  get the processing presently active 
*
*   get the next highest priority queued when processing
*   and deletes it from the queue.
*
* RETURNS:
* 0 , else -1
*
*       Author Greg Brissey 9/2/94
*/
int activeExpQget(int* priority, char* expidstr)
{
  procQ *queue;
  activeQentry *Qentries;
  sigset_t    savemask;
 
  blockSignals(&savemask);

  queue = activeQ.procQstart;
  if (queue == NULL)
  {
    errLogRet(ErrLogOp,debugInfo,"activeExpQadd: Queue pointer is NULL\n");
    return(-1);
  }

  if (queue->numInQ == 0)	/* non in this Q, goto next */
    return(-1);

  Qentries = (activeQentry *)((char*)queue + sizeof(procQ));
  if (Qentries == NULL)
  {
    errLogRet(ErrLogOp,debugInfo,"activeExpQget: Queue Entry pointer is NULL\n");
    return(-1);
  }

  shrmTake(activeQ.shrMem);

  *priority = Qentries[0].Priority;
  strncpy(expidstr, Qentries[0].ExpIdStr,EXPID_LEN);

  shrmGive(activeQ.shrMem);

  /* release the signals block, note a signal maybe delieved prior to returninf
     from the sigprocmask call
  */
  unBlockSignals(&savemask);

  return(0);

}

/*************************************************************
* activeExpQentries - return the number of entries in the activeQ
*
*  Return the number of entries within the Experiment Active
*  Queue.
*
* RETURNS:
*  Number in Queue 
*
*       Author Greg Brissey 9/2/94
*/
int  activeExpQentries()
{
  procQ *queue;
  sigset_t    savemask;
 
  int nEntries = 0;

  blockSignals(&savemask);

  queue = activeQ.procQstart;
  if (queue == NULL)
  {
    errLogRet(ErrLogOp,debugInfo,"activeExpQadd: Queue pointer is NULL\n");
    return(-1);
  }

  nEntries = queue->numInQ;

  /* release the signals block, note a signal maybe delieved prior to returninf
     from the sigprocmask call
  */
  unBlockSignals(&savemask);

  return(nEntries);
}

/*************************************************************
*  activeExpQdelete -  delete entry base on FG/BG and Key 
*
* RETURNS:
* 0 , else -1
*
*       Author Greg Brissey 9/2/94
*/
int activeExpQdelete(char *Expidstr)
{
  int i,entry;
  activeQentry *Qentries;
  procQ *queue;
  sigset_t    savemask;

  blockSignals(&savemask);
    
  queue = activeQ.procQstart;
  if (queue == NULL)
  {
    errLogRet(ErrLogOp,debugInfo,"activeExpQdelete: Queue pointer is NULL\n");
    return(-1);
  }
  if (queue->numInQ == 0)
      return(OK);

  Qentries = (activeQentry *)((char*)queue + sizeof(procQ));
  if (Qentries == NULL)
  {
     errLogRet(ErrLogOp,debugInfo,"activeExpQdelete: Queue Entry pointer is NULL\n");
     return(-1);
  }

  
  /* search for FG/BG and key */
  entry = -1;
  for(i=0; i < queue->numInQ; i++)
  {
     if( strcmp(Expidstr,Qentries[i].ExpIdStr) == 0 )
     {
        entry = i;
	break;
     }
  }
  if (entry == -1)
  {
     errLogRet(ErrLogOp,debugInfo,"activeExpQdelete: Entry doen't Exist for '%s'\n",
	Expidstr);
     return(-1);
  }

  /* delete by copying over and shifting all others down */
  shrmTake(activeQ.shrMem);
  for(i=entry; i < queue->numInQ - 1; i++)
  {
    Qentries[i].Priority = Qentries[i+1].Priority;
    strncpy(Qentries[i].ExpIdStr,Qentries[i+1].ExpIdStr,EXPID_LEN);
    strncpy(Qentries[i].ExpInfoStr,Qentries[i+1].ExpInfoStr,EXPID_LEN);
  }
  queue->numInQ--;
  shrmGive(activeQ.shrMem);

  /* release the signals block, note a signal maybe delieved prior to returning
     from the sigprocmask call
  */
  unBlockSignals(&savemask);

  return(OK);
}

/*************************************************************
*  activeExpQclean -  reset number in Q to zero
*
* RETURNS:
* 0 , else -1
*
*       Author Greg Brissey 9/2/94
*/
int activeExpQclean(void)
{
  procQ *queue;
  sigset_t    savemask;

  blockSignals(&savemask);

  queue = activeQ.procQstart;
  shrmTake(activeQ.shrMem);
  queue->numInQ = 0;
  shrmGive(activeQ.shrMem);

  /* release the signals block, note a signal maybe delieved prior to returning
     from the sigprocmask call
  */
  unBlockSignals(&savemask);

  return(0);
}

/*************************************************************
*  activeExpQRelease -  release share memory of active queue 
*
* RETURNS:
* void
*
*       Author Greg Brissey 9/12/94
*/
void activeExpQRelease()
{
  shrmRelease(activeQ.shrMem);
}
    
/*************************************************************
*  activeExpQshow -  prints contents of active queue
*
* RETURNS:
* 0 , else -1
*
*       Author Greg Brissey 9/2/94
*/
int activeExpQshow(void)
{
  int j;
  activeQentry *Qentries __attribute__((unused));
  procQ *queue;

  queue = activeQ.procQstart;
  Qentries = (activeQentry *)((char*)queue + sizeof(procQ));
  DPRINT1(-1,"\nActive Queue: In Queue: %d\n",
	queue->numInQ);
  for( j=0; j < queue->numInQ; j++)
  {
    DPRINT3(-1,"     (%d): ExpId: '%s', Priority: %d\n",
		j+1,Qentries[j].ExpIdStr, Qentries[j].Priority);
    DPRINT3(-1,"     (%d): ExpInfo: '%s', Priority: %d\n",
		j+1,Qentries[j].ExpInfoStr, Qentries[j].Priority);
  }
  return( queue->numInQ );
}

/*************************************************************
* getTotalQsize - return the Queue size in bytes 
*
*  Return the Experiment Queue size in bytes 
*
* RETURNS:
*  size in bytes of Exp Queue
*
*/
int getTotalQsize()
{
   int total,i;
   total = 0;
   for(i=0; i < NUM_EXP_OF_QUEUES; i++)
   {
      total += (sizeof(procQ) + (sizeof(expQentry) * EXP_Q_SIZE));
   }
   return(total);
}

int blockSignals(sigset_t *savemask)
{
  sigset_t    blockmask;

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
  if (sigprocmask(SIG_BLOCK, &blockmask, savemask) < 0) 
  {
      errLogSysRet(ErrLogOp,debugInfo,"blockSignals: sigprocmask failed");
      return( -1 );
  }
  return(0);
}
    
void unBlockSignals(sigset_t *savemask)
{
  sigprocmask(SIG_SETMASK, savemask, (sigset_t *)NULL);
}

#endif /* __INTERIX or MACOS */
