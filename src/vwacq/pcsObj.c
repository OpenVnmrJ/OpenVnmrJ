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

/*

Parallel Sort Changes

A_interp

New Acodes: 

PARALLEL_INIT
	initializes maximum parallel channels and maximum channel depth 
	that can be expected with in the experiment.

	This allows all the space that is needed to be malloc'd up front
	and not incur the time consuming malloc operation within the experiment


PARALLEL_START:

	This builds the parallel sorting Object and allocates the previous
	malloc'd resources to this Object.


PARALLEL:
	Marks which of the parallel channels the interpreter is working on


PARALLEL_END:
	Marks the end of parallel channels,
	The Sort is performed and all resources are returned
	form the sorting object.


fifoObj.c

    fifoMaskHsl
    fifoUnMaskHsl
    fifoStuffCmd

    	These functions now check to see if parallel channel operation is being parsed
		(via, if (pFifoId->pPChanObj) )

          Parallel events are placed into a ring buffer assigned to that parallel
	   channel rather than into the FIFO stuffing buffer.

        These channel ring buffers are then used as input to the sorter.



pcsObj.c


    The Sort:

    In priciple the sort is straight forward:

        1. Read in each parallel channel event

	2. Select that event which consumes the least amount of time.

	3. Issue this event and subtract the time this event takes from
	   all other channel events.

	4. Get the next event
      
        5. Goto 2, until all channels are empty then we're done.

    Unfortunately it's not that simple, due to hardware contraints.

    Constraints:

	1. Minimum time is 100 nsec or 8 ticks at 12.5 nsec resolution.

        2. Some Events time can not be subtracted from (e.g. apbus)

        3. Apbus event takes 100 nsec or 8 ticks

        4. Apbus events need a minimum separation time of 400 nsec or 32 ticks
		*actually some even longer (gradients 800nsec)

        5. Some devices require multiple apbus events to be atomic (i.e. no other
	   apbus events from other channels get mixed into the same stream)

   
   Dealing with Constraints:

      1. 
         If a delay is less than 8 ticks and there is a previous delay that is modifyable

           delay <= 4 ticks   move this delay to the previous delay 
				(i.e. add to previous delay and make this delay zero )
			      subtract these ticks from the other events

           delay > 4 ticks    remove the needed ticks to bring this delay to the
			      minimum delay (8 ticks) from the previous delay 
			      and add them to this delay 
			      subtract original ticks from the other events

     2.
	For events that can't be subtracted from (apbus) or delays that go negative
	this time is added to the carry over time for that channel.
	When a delay event is encounted this carry over time is immediately
	subtracted from the delay.

     3. After an APbus, 8 ticks are subtracted from the other events

     4.
        Issued ApBus events set an Apbus count down register (ApCntDwn) to the 
	  required separation time of 32 ticks (400nsec).
	Apbus event check the Apbus count down register, 
	  if ApCntDwn greater than Zero
	      We Force a delay of 32 ticks for this channel and add that to the carry over time
              of this channel.

        Each issued delay event is subtracted from the ApCntDwn register
    
     
     5. There are LOCK and UNLOCK events, when encountered on a channel sorting contiunes
	as usual until an Apbus or LOCK event is encounter in another channel. At
	that point no further action is taken on that channel until the original
	LOCK is given up.

        Thus this may result in skewwed timing do to this lock out effect..
	Getting worse as multiply channels attempt to lock at the same time.


*/

#ifdef VXWORKS
#include "hardware.h"
#include "acodes.h"
#include "logMsgLib.h"
#include "hostAcqStructs.h"
#include "expDoneCodes.h"
#include "errorcodes.h"
#include "instrWvDefines.h"
#else
#include <varargs.h>
#include <stdio.h>
void errLogRet(int prtOpt, char *fileNline, ...);
void errLogSysRet(int prtOpt, char *fileNline, ...);
#endif

#include "pcsObj.h"
/*
	Sorting routine
*/

#define TRUE  1
#define HS_LINE_MASK    0x07FFFFFF
#define FALSE  0
/* #define ITSaGATE  0xFF000000 */
#define READ_SRCBUF 0
#define NEXT_HOLDBUF 10
#define CHECK_TYPE 20
#define GATEON 30
#define GATEOFF 35
#define DELAY 40
#define APBUS 50
#define ALL_DELAYS 60
#define FORCED_APDELAY 65
#define APBUS_DELAYS 70
#define GRADXYZ_APBUS_DELAYS 72
#define ALL_APBUS    75
#define SUB_DELAY 80
#define FIND_MIN 90
#define MIN_ZERO 100
#define EMPTY 110
#define LOCKCHAN 120
#define UNLOCKCHAN 130
#define SETOBLGRADX 140
#define SETOBLGRADY 150
#define SETOBLGRADZ 160
#define SETOBLMATRIX 170
#define ACQSTARTLP   180
#define ACQENDLP   190
#define ACQLPCNT   200
#define ACQCTC     210
#define ACQSTART   220
#define ACQEND     230
   
#define CMD_INDEX 0
#define DATA_INDEX 1

#define UNKNOWN "Unknown "

#define AP_EDELAY  8	/* the effective delay of apbus instruction */

#define HW_DELAY_FUDGE 3L

/*======================================================================== */
#ifndef VXWORKS
/* #define HW_DELAY_FUDGE           3 */
#define APWRT           0x01000000L
#define CL_AP_BUS       APWRT
#define CL_DELAY        0x00000000L
#define CL_START_LOOP   0x04000000L
#define CL_END_LOOP	0x02000000L
#define CL_LOOP_COUNT	0x06000000L
#define CL_CTC	        0x08000000L

#define LOGIT -1
#define debugInfo 0
#endif
/*======================================================================== */

/*======================================================================== */
#ifdef VXWORKS
extern MSG_Q_ID pMsgesToPHandlr;/* MsgQ for Msges to Problem Handler */

/* Exception Msges to Phandler, e.g. FOO, etc. */
extern EXCEPTION_MSGE HardErrorException;
extern EXCEPTION_MSGE GenericException;

extern FIFO_ID pTheFifoObject;

#else	/*----------------------------------------------------------*/

static int DebugLevel=4;
#endif

/*======================================================================== */

/* format or two words are the arguments to fifoStuffCmd: 1st CntrBits, 2nd Ticks */

typedef struct __statestr         
    {
      int typeValue;
      char *typeName;
    } type_obj;

static type_obj typenames[27] = { { READ_SRCBUF,  "Read_Src" }, 
			    { NEXT_HOLDBUF, "NextHold" },
			    { CHECK_TYPE,   "Chk_Type" },
			    { GATEON,	    "GateOn  " },
			    { GATEOFF,	    "GateOff " },
			    { DELAY,        "Delay   " },
			    { APBUS,        "ApBus   " },
			    { ALL_DELAYS,   "AllDelay" },
			    { APBUS_DELAYS, "Ap&Delay" },
			    { ALL_APBUS,    "AllApbus" },
			    { SUB_DELAY,    "SubDelay" },
			    { FIND_MIN,     "Find_Min" },
			    { MIN_ZERO,     "MinZero " },
			    { EMPTY, 	    "Empty" }	,
			    { LOCKCHAN,     "LockChan" },
			    { UNLOCKCHAN,   "UnLockChan" },
			    { SETOBLGRADX,  "SetGradX" },
			    { SETOBLGRADY,  "SetGradY" },
			    { SETOBLGRADZ,  "SetGradZ" },
			    { SETOBLMATRIX, "SetMatrix" },
			    { ACQSTARTLP, "AcqStartLoop" },
			    { ACQENDLP, "AcqEndLoop" },
			    { ACQLPCNT, "AcqLoopCnt" },
			    { ACQCTC, "AcqCTC" },
			    { ACQSTART, "AcqStart" },
			    { ACQEND, "AcqEnd" }
			  };
   
static int TYPESIZE = 21;
static RINGL_ID SortBufs = NULL;
static int MaxBufs;
static int SortOutputLevel = 0;
static int EmbeddedSortLevel = 0;

static int abortSort = 0;	/* abort to sorter */
static RINGL_ID pcsObjList = NULL; /* ring buffer of pcsObjs, to keep track of Objects to enable abort
				     to delete any outstanding objects */

/* a stack construct to keep track of pcsObj being actively used
   so for an abort they can be freed if necessary
*/
static PCHANSORT_ID *pcsObjStack = NULL;
static int stackLen = 0;
static int stackPtr = -1;
static int HighLevelSort = 0;

static int HighLevel = 0;
static int HighLevelChan = 0;
static int Maxsize = 0;
static int MaxChans = 0;

/* Obliquing matrix for rotating imaging gradients in software */




/* Obliquing matrix for rotating imaging gradients in software */
static int oblgradmatrix[9] = {0,0,0,0,0,0,0,0,0};

char *getState(int state);

int pcsSetLevel(int level)
{
   int tmp = SortOutputLevel;
   SortOutputLevel = level;
   return(tmp);
}

int pcsResetMax()
{
   HighLevel = HighLevelChan = Maxsize = MaxChans = HighLevelSort = 0;
}

int pcsPrtMax()
{
   printf("MaxChans = %d, HiH2O Mark: %d, MaxSize: %d, HighWater Mark: %d, Max pcsObj Used simul: %d\n",
	MaxChans,HighLevelChan,Maxsize,HighLevel,HighLevelSort);
   return(0);
}

pcsPush(PCHANSORT_ID pcs)
{
   /* printf("push: stackPtr: %d\n",stackPtr); */
   if (stackPtr+1 < stackLen)
   {
      pcsObjStack[++stackPtr] = pcs;
      if (stackPtr > HighLevelSort) HighLevelSort = stackPtr;
   }
   else
     errLogRet(LOGIT,debugInfo,"pcsPush: Out of Stack Space:");
      
}
PCHANSORT_ID pcsPop()
{
   PCHANSORT_ID pcs;
   /* printf("pop: stackPtr: %d\n",stackPtr); */
   if (stackPtr != -1)
     pcs = pcsObjStack[stackPtr--];
   else
     pcs = NULL;
   return(pcs);
}
PCHANSORT_ID pcsPeek()   /* doesn't pop item off the stack */
{
   PCHANSORT_ID pcs;
   /* printf("peek: stackPtr: %d\n",stackPtr); */
   if (stackPtr != -1)
     pcs = pcsObjStack[stackPtr];
   else
     pcs = NULL;
   return(pcs);
}
pcsStackShow()
{
    int index = stackPtr;
    for (index = 0; index <= stackPtr; index++)
    {
     DPRINT2(-1,"stack[%d] = 0x%lx\n",index,pcsObjStack[index]);
    }
}

pcsAbortSort()
{
   abortSort = 1;
}


RINGL_ID initializeSorterBufs(int maxChans,int MaxSize)
{
    int i;
    RINGL_ID srcBuf;
    PCHANSORT_ID   pPCSId;

   abortSort = 0;
    DPRINT2(SortOutputLevel,"initializeSorterBufs: create %d buffers of %d entries long\n",maxChans,MaxSize);
#ifdef INSTRUMENT
     {
      int values[2] = { maxChans, MaxSize };
      wvEvent(EVENT_PARALLEL_INIT,&values,sizeof(values));
     }
#endif
    MaxBufs = maxChans;

    if (maxChans > MaxChans) MaxChans = maxChans;
    if (MaxSize*4 > Maxsize) Maxsize = MaxSize*4;


    stackLen = MaxBufs/2; /* the assumption here is that each sorting object will have at least 2
			      channels thus the max sorting Objects need is 1/2 of the max channels */

    /* DPRINT1(-1,"pcsObjList: 0x%lx\n",pcsObjList); */
    if (pcsObjList != NULL)
       rngLDelete(pcsObjList);

    /* DPRINT1(-1,"stackLen: %d\n",stackLen); */
    /* create a ring buffer to hold avialable pcsObjects, ie free list */
    pcsObjList = rngLCreate(stackLen,"pcsObj");
    /* DPRINT1(-1,"pcsObjList: 0x%lx\n",pcsObjList); */
    /* rngLShow(pcsObjList,1); */

    /* DPRINT1(-1,"pcsObjStack: 0x%lx\n",pcsObjStack); */
    if (pcsObjStack != NULL)
       free(pcsObjStack);

    /* create a stack to hold actively used pcsObjects */
    pcsObjStack = (PCHANSORT_ID) malloc(sizeof(PCHANSORT_ID)*stackLen);
    memset(pcsObjStack,0,(sizeof(PCHANSORT_ID)*stackLen));
    /* DPRINT1(-1,"pcsObjStack: 0x%lx\n",pcsObjStack); */

    if (SortBufs != NULL) 
         rngLDelete(SortBufs);
    SortBufs = rngLCreate(maxChans,"BufList");
    /* DPRINT1(-1,"SortBufs: 0x%lx\n",SortBufs); */
    if ((pcsObjList == NULL) || (pcsObjStack == NULL) || (SortBufs == NULL))
    {
         errLogSysRet(LOGIT,debugInfo,"initializeSorterBufs: Could not Allocate Either pcsObjectList (0x%lx), pcsStack (0x%lx). Buffer List (0x%lx):",pcsObjList,pcsObjStack,SortBufs);
#ifdef VXWORKS
         GenericException.exceptionType = HARD_ERROR;  
         GenericException.reportEvent = HDWAREERROR+PARALLELNOSPACE;   /* errorcode is returned */
         /* send error to exception handler task */
         msgQSend(pMsgesToPHandlr, (char*) &GenericException, 
		   sizeof(EXCEPTION_MSGE), NO_WAIT, MSG_PRI_NORMAL);
#endif
         return(NULL);
    }

    for(i=0; i < MaxBufs; i++)
    {
       srcBuf = rngLCreate(MaxSize*4,"SortBufs");
       rngLPut(SortBufs,&srcBuf,1);
       DPRINT1(SortOutputLevel,"Created Parallel Buffer at: 0x%lx\n", srcBuf);
       if (srcBuf == NULL)
       {
         errLogSysRet(LOGIT,debugInfo,"initializeSorterBufs: Could not Allocate Input Buffer Space:");
#ifdef VXWORKS
         GenericException.exceptionType = HARD_ERROR;  
         GenericException.reportEvent = HDWAREERROR+PARALLELNOSPACE;   /* errorcode is returned */
         /* send error to exception handler task */
         msgQSend(pMsgesToPHandlr, (char*) &GenericException, 
		   sizeof(EXCEPTION_MSGE), NO_WAIT, MSG_PRI_NORMAL);
#endif
         return(NULL);
	}
    }
    /* rngLShow(SortBufs,1); */
   

    /* allocate pcsObj up front too. */
    for(i=0; i < stackLen; i++)
    {
       pPCSId = (PCHANSORT_ID) malloc(sizeof(PCHANSORT_OBJ));  /* create structure */
       DPRINT1(SortOutputLevel,"Created pcsObj at: 0x%lx\n", pPCSId);
       if (pPCSId == NULL)
       {
         errLogSysRet(LOGIT,debugInfo,"initializeSorterBufs: Could not Allocate pcsObj Space:");
#ifdef VXWORKS
         GenericException.exceptionType = HARD_ERROR;  
         GenericException.reportEvent = HDWAREERROR+PARALLELNOSPACE;   /* errorcode is returned */
         /* send error to exception handler task */
         msgQSend(pMsgesToPHandlr, (char*) &GenericException, 
		   sizeof(EXCEPTION_MSGE), NO_WAIT, MSG_PRI_NORMAL);
#endif
         return(NULL);
       }
       rngLPut(pcsObjList,&pPCSId,1);
    }
    /* rngLShow(pcsObjList,1); */


    return(SortBufs);
}

void freeSorter()
{
    int i,nelem;
    RINGL_ID srcBuf;
    PCHANSORT_ID pPCSId;

    if ((SortBufs == NULL) || (pcsObjList == NULL) || (pcsObjStack == NULL))
	return;

#ifdef INSTRUMENT
      wvEvent(EVENT_PARALLEL_FREE,NULL,NULL);
#endif

/*
    DPRINT2(-1,"stackIndex = %d, pcsObj free List: %d\n", 
        stackPtr,rngLNElem(pcsObjList));
*/
    /* pcsStackShow(); */
/*
    DPRINT2(-1,"stackIndex = %d, pcsObj free List: %d\n", 
        stackPtr,rngLNElem(pcsObjList));
*/
    while ( (pPCSId = pcsPeek()) != NULL)
    {
	DPRINT1(SortOutputLevel,"freeSorter: release PCS Object references 0x%lx\n",pPCSId);
	pchanSortDelete(pPCSId);
    }

/*
    DPRINT2(-1,"stackIndex = %d, pcsObj free List: %d\n", 
        stackPtr,rngLNElem(pcsObjList));
*/

    while( (nelem = rngLGet( pcsObjList, &pPCSId, 1)) != 0)
    {
	DPRINT1(SortOutputLevel,"freeSorter: free PCS Object 0x%lx\n",pPCSId);
	free(pPCSId);
    }


    for(i=0; i < MaxBufs; i++)
    {
       nelem = rngLGet(SortBufs,&srcBuf,1);
       if (nelem != 0)
       {
	 rngLDelete(srcBuf);
	 DPRINT1(SortOutputLevel,"freeSorter: deleted PCS Sort Buffer 0x%lx\n",srcBuf);
       }
    }
    /* deleting the SortBufs rngbuffer here but moved to initializeSorterBufs
	to avoid what appeared to be a race condition with PPC */
    MaxBufs = 0;
}


/*
    RINGL_ID rngId = rngLCreate(10,"test");
    nelem = rngLPut(rngId,&(buffer[0]),2);
    nelem = rngLGet(rngId,&(ibuf[0]),2);
*/
PCHANSORT_ID pchanSortCreate(int numChans,char *IdStr, RINGL_ID FreeBufs,
				PCHANSORT_ID parent,FIFO_ID fifoobj)
{
  PCHANSORT_ID pPCSId;
  FIFO_ID fifoId; 
  int i,nelem;

  char tmpstr[80];

#ifdef INSTRUMENT
      wvEvent(EVENT_PARALLEL_START_CREATE,&numChans,sizeof(int));
#endif

  /*DPRINT3(1,"pFreeBufs: 0x%lx, nFreeBufs: %d, numChans: %d\n",FreeBufs,rngLNElem(FreeBufs), numChans); */
  if ( rngLNElem(FreeBufs) < numChans)
  {
    errLogRet(LOGIT,debugInfo,"pchanSortCreate: Not Enough Free Buffers :");

#ifdef VXWORKS
    GenericException.exceptionType = HARD_ERROR;  
    GenericException.reportEvent = HDWAREERROR+PARALLELNOSPACE;   /* errorcode is returned */
    /* send error to exception handler task */
    msgQSend(pMsgesToPHandlr, (char*) &GenericException, 
		   sizeof(EXCEPTION_MSGE), NO_WAIT, MSG_PRI_NORMAL);
#endif
    return(NULL);
  }

  nelem = rngLGet( pcsObjList, &pPCSId, 1);
  /* DPRINT1(-1,"pchanSortCreate: pcsId: 0x%lx\n",pPCSId); */
  /* pPCSId = (PCHANSORT_ID) malloc(sizeof(PCHANSORT_OBJ));  /* create structure */
  if (nelem == 0) 
  {
    errLogSysRet(LOGIT,debugInfo,"pchanSortCreate: Could not Get a pcsObject:");
#ifdef VXWORKS
    GenericException.exceptionType = HARD_ERROR;  
    GenericException.reportEvent = HDWAREERROR+PARALLELNOSPACE;   /* errorcode is returned */
    /* send error to exception handler task */
    msgQSend(pMsgesToPHandlr, (char*) &GenericException, 
		   sizeof(EXCEPTION_MSGE), NO_WAIT, MSG_PRI_NORMAL);
#endif
    return(NULL);
  }

  pcsPush(pPCSId);
  memset(pPCSId,0,sizeof(PCHANSORT_OBJ));

  pPCSId->pFifoId = fifoobj;
  pPCSId->pFreeBufList = FreeBufs;

  pPCSId->Parent = parent;  /* parent is none null if this is an embedded sort */
  if (parent)
  {
     pPCSId->DstBuf = parent->SrcBufs[parent->presentChanNum];  /* maybe null */
  }
  else
    pPCSId->DstBuf = NULL;

  /* create Ring Buffer size buffer nelem + 1 */
  pPCSId->numSrcs = numChans;
  for(i = 0; i < numChans; i++)
  {
     nelem = rngLGet(FreeBufs,&(pPCSId->SrcBufs[i]),1);
     /*-----------------------------------------------------------------------------------*/
     /* if nelem == 0 then we ran out of buffers to use, more parallel channels than allocated */
     if (nelem == 0) 
     {
    	errLogSysRet(LOGIT,debugInfo,"pchanSortCreate: Ran out of parallel channel buffers:");
#ifdef VXWORKS
	rngLShow(FreeBufs,0);  /* display the ring buffer */
    	GenericException.exceptionType = HARD_ERROR;  
    	GenericException.reportEvent = HDWAREERROR+PARALLELNOSPACE;   /* errorcode is returned */
    	/* send error to exception handler task */
    	msgQSend(pMsgesToPHandlr, (char*) &GenericException, 
		   sizeof(EXCEPTION_MSGE), NO_WAIT, MSG_PRI_NORMAL);
#endif
        return(NULL);
     }
     /*-----------------------------------------------------------------------------------*/
     rngLFlush(pPCSId->SrcBufs[i]);
  }
  EmbeddedSortLevel++;

  if (DebugLevel > SortOutputLevel+1)
  {
     printf("---- pchanSortCreate: %d channels to sort, parent: 0x%lx, embedded level %d (1=top level)\n",
			numChans,parent,EmbeddedSortLevel);
     printf(" Input channel buffers:\n");
     for(i=0; i < pPCSId->numSrcs; i++)
     {
	 printf("   Input Channel %d  0x%lx \n",i,pPCSId->SrcBufs[i]);
     }
     printf("\n");
     if (parent != NULL)
     {
        printf(" Parent input channel buffers:\n");
        for(i = 0; i < parent->numSrcs; i++)
        {
	   printf("--------- Channel %d -----------\n",i);
	   if (DebugLevel > SortOutputLevel+1)
	      rngLShow(parent->SrcBufs[i],2);
	   else
	      printf("Ring ID: 0x%lx, Used: %d, Free: %d\n", parent->SrcBufs[i],rngLFreeElem(parent->SrcBufs[i]),
			rngLNElem(parent->SrcBufs[i]));
        }
        printf("------------------------------------\n\n");
        printf("Destination Buffer(0x%lx) is input channel %d of parent (0x%lx)\n",
			pPCSId->DstBuf,parent->presentChanNum,parent);
        printf("------------------------------------\n\n");
     }

  }

  return( pPCSId );
}

void pchanSortDelete(PCHANSORT_ID pcsId)
{
  int i,nelem;
  for(i = 0; i < pcsId->numSrcs; i++)
  {
     rngLFlush(pcsId->SrcBufs[i]);
     nelem = rngLPut(pcsId->pFreeBufList,&(pcsId->SrcBufs[i]),1);
  }

  EmbeddedSortLevel--;
  DPRINT1(SortOutputLevel,"pchanSortDelete: embedded level popped up to %d\n",EmbeddedSortLevel);

  pcsPop();  /* pop this active pcsObj off the stack, since it's 'deleted' */

  rngLPut(pcsObjList,&pcsId,1);  /* don't free, put it back on the free list of pcsObjs */
  /* free(pcsId); */
}

PCHANSORT_ID  pchanGetParent(PCHANSORT_ID pPCSId)
{
   return(pPCSId->Parent);
}

RINGL_ID  pchanGetActiveChanBuf(PCHANSORT_ID pPCSId)
{
   return(pPCSId->SrcBufs[pPCSId->presentChanNum]);
}

void pchanStart(PCHANSORT_ID pPCSId, int chanNumber)
{
#ifdef INSTRUMENT
      wvEvent(EVENT_PARALLEL_SELECT,&chanNumber,sizeof(int));
#endif
   pPCSId->presentChanNum = chanNumber;
   pPCSId->pInputBuf = pPCSId->SrcBufs[chanNumber];
}

pchanPut(PCHANSORT_ID pcsId, unsigned long Cntr, unsigned long Ticks)
{
   int bytes;
   rngLPut(pcsId->pInputBuf,(long*) &Cntr,1);
   bytes = rngLPut(pcsId->pInputBuf,(long*) &Ticks,1);

/*  for diagnostics, high water mark on input buffer usage
   nelem = rngLNElem(pcsId->pInputBuf);
   if ( nelem > HighLevel) HighLevel = nelem;
*/ 

   if (bytes == 0) 
   {
    	errLogSysRet(LOGIT,debugInfo,"pchanPut: Ran out of parallel input buffer space");
#ifdef VXWORKS
	rngLShow(pcsId->pInputBuf,0);  /* display the ring buffer */
    	GenericException.exceptionType = HARD_ERROR;  
    	GenericException.reportEvent = HDWAREERROR+PARALLELNOSPACE;   /* errorcode is returned */
    	/* send error to exception handler task */
    	msgQSend(pMsgesToPHandlr, (char*) &GenericException, 
		   sizeof(EXCEPTION_MSGE), NO_WAIT, MSG_PRI_NORMAL);
#endif
        return(NULL);
   }
}

/*----------------------------------------------------------------------*/
/* Oblique Gradient Functions						*/
/*----------------------------------------------------------------------*/

void clearOblGradMatrix() {
   int i;
   /* init grad angle matrix */
   for (i=0; i<9; i++) oblgradmatrix[i] = 0;
}

/*----------------------------------------------------------------------*/
/* setoblgrad								*/
/*	"setoblgrad upacks the tmpval word.  Calculates the gradient	*/
/*	value for the defined axis and sets the fifo.			*/
void setoblgrad(unsigned short axis,unsigned long tmpval)
{
   unsigned short apdelay,select,apaddr,gradselect;
   int i,value,status,naxis,rshift;
   select = tmpval >> 16;
   gradselect = select & 0xf000;
   select = select & 0x0fff;
   apaddr = tmpval & 0x0000ffff;
   
   /* set up gradient */
   switch(gradselect) {
       case SEL_WFG:
       		apdelay = STD_APBUS_DELAY;
       		rshift = OBLGRAD_BITS2SCALE - WFG_BITS;
       		break;
       case SEL_PFG1:
       		apdelay = PFG_APBUS_DELAY;
       		rshift = OBLGRAD_BITS2SCALE - PFG1_BITS;
       		break;
       case SEL_PFG2:
       		apdelay = PFG_APBUS_DELAY;
       		rshift = OBLGRAD_BITS2SCALE - PFG2_BITS;
		owriteapword(apaddr+PFG2_AMP_ADDR_REG,select|APSTDLATCH,apdelay);
		apaddr = apaddr+PFG2_AMP_VALUE_REG;
       		break;
       case SEL_TRIAX:
       		apdelay = PFG_APBUS_DELAY;
       		rshift = OBLGRAD_BITS2SCALE - TRIAX_BITS;
		owriteapword(apaddr+TRIAX_AMP_ADDR_REG,select,apdelay);
		apaddr = apaddr+TRIAX_AMP_VALUE_REG;
       		break;
       default:
    	errLogRet(LOGIT,debugInfo,
	   "setoblgrad Warning: Invalid gradient select %d.",gradselect);
	        break;       		
   }
   naxis = axis*3;
   value = oblgradmatrix[naxis] + oblgradmatrix[naxis+1] + oblgradmatrix[naxis+2];

   DPRINT2(0,"setoblgrad: naxis: %d value: %d\n",naxis,value);   
   status = oblgradient(apdelay,apaddr,GRADMAX_20>>rshift,GRADMIN_20>>rshift,
   							value>>rshift);
   if (status != 0) {
	if (status > 0) {
	   status = 0;
#ifdef VXWORKS
	   GenericException.exceptionType = WARNING_MSG;  
	   GenericException.reportEvent = WARNINGS + VGRADOVER;  
	   msgQSend(pMsgesToPHandlr, (char*) &GenericException, 
		sizeof(EXCEPTION_MSGE), NO_WAIT, MSG_PRI_NORMAL);
#endif
	} else {
	   /* APint_run = FALSE; */
	   /* status = HDWAREERROR + INVALIDACODE; */
	}
   }
}

/*----------------------------------------------------------------------*/
/* setgradmatrix							*/
/* 	Sets oblique gradient matrix value.				*/
/*      Index of value is stored in upper nibble of 32 bit word.	*/
/*	Second upper nibble defines whether value is negative or 	*/
/*	positive.  Value cannot be greater than 24 bits.		*/
/*----------------------------------------------------------------------*/
void setgradmatrix(unsigned long tmpval)
{
   int index;
   /* unpack word */
   index = (int)((tmpval >> 28) & 0x0000000f);
   tmpval = tmpval & 0x0fffffff;
   tmpval = tmpval | ((tmpval << 4) & 0xf0000000);
   DPRINT2(0,"setgradmatrix: index: %d tmpval: %d\n",index,tmpval);
   oblgradmatrix[index] = tmpval;
   
}

#ifdef VXWORKS
/*----------------------------------------------------------------------*/
/* oblgradient								*/
/*	SAME AS GRADIENT BUT WRITES TO PARALLEL OUTPUT LOCATION.	*/
/*	Gradient checks the value against its min and max values before */ 
/*	sending it to the requested apbus address.  The apbus address	*/
/*	is encoded with the details of the size of the value to be	*/
/*	sent.  Bit 12 and 13 (msb = bit 15) are defined as in apbcout.	*/
/*	Bits 14-15 are defined as: 0=16(12) bit value, 1=20 bit value,	*/
/*			2,3 are undefined.				*/
/*	Arguments:							*/
/*		apdelay	: apbusdelay					*/
/*		apaddr	: apbus address					*/
/*		maxval	: maximum value to be set.			*/
/*		minval	: minimum value to be set.			*/
/*		value	: value.					*/
/*----------------------------------------------------------------------*/
int oblgradient(unsigned short apdelay,unsigned short apaddr,int maxval,int minval,int value)
{
   int status,wordsize;
   unsigned short bytf,incf;
   status = 0;
   DPRINT4(0,"oblgradient: apaddr: 0x%x apdelay: 0x%x, maxval: %d value: %d\n",
   		apaddr,apdelay,maxval,value);
   if ( value < minval) {
    	errLogRet(LOGIT,debugInfo,
	   "oblgradient Warning: Underflow value: %d , max: %d.",value,minval);
	value = minval;
	status = 1;
   }
   if ( value > maxval) {
    	errLogRet(LOGIT,debugInfo,
	   "oblgradient Warning: Overflow value: %d , max: %d.",value,maxval);
	value = maxval;
	status = 1;
   }
   wordsize = (apaddr & GRADDATASIZ) >> 14;
   bytf = apaddr & APBYTF; 
   incf = apaddr & APINCFLG;
   apaddr = apaddr & APADDRMSK; 
   switch(wordsize) {
	case 0:			/* 16 bit */
	   {
		if (!bytf)
		    owriteapword(apaddr,(ushort)value,apdelay);
		else {
		    short apdata;
		    apdata = value;
		    owriteapword(apaddr,(apdata >> 8)|APSTDLATCH,apdelay);
	 	    if (incf) 
	 		apaddr += 1; 
		    owriteapword(apaddr,apdata|APSTDLATCH,apdelay);
		}
	   }
	   break;
	case 1:			/* 20 bit */
	   {
		if (!bytf)
		{
		    status= -1;
    		    errLogRet(LOGIT,debugInfo,
			"oblgradient Error: Nonexistent 20 bit apword setting.");
		}
		else {
		    short apdata;
		    apdata = value;
		    owriteapword(apaddr,(apdata >> 12)|APSTDLATCH,apdelay);
	 	    if (incf) 
	 		apaddr += 1; 
		    owriteapword(apaddr,(apdata >> 4)|APSTDLATCH,apdelay);
	 	    if (incf) 
	 		apaddr += 1; 
		    owriteapword(apaddr,(apdata << 4)|APSTDLATCH,apdelay);
		}
	   }
	   break;
	default:
	   status = -1;
    	   errLogRet(LOGIT,debugInfo,
		"oblgradient Error: Unknown gradient data size setting.");
	   break;
   }
   return(status);
}

/*----------------------------------------------------------------------*/
/*	SAME AS WRITEAPWORD BUT WRITES TO PARALLEL OUTPUT LOCATION.	*/
/*----------------------------------------------------------------------*/
void owriteapword(ushort apaddr,ushort apval, ushort apdelay)
{
 ushort *shortfifoword;
 ulong_t fifoword;
 
 DPRINT2( 3, "owriteapword: 0x%x, 0x%x\n", apaddr, apval );
 shortfifoword = (short *) &fifoword;
 shortfifoword[0] = APWRITE | apaddr;
 shortfifoword[1] = apval;
 fifoStuffOCmd(pTheFifoObject,CL_AP_BUS,fifoword);
 fifoStuffOCmd(pTheFifoObject,CL_DELAY,apdelay);
}
#endif

/*======================================================================== */
/*  functions for standalone compilation */
#ifndef VXWORKS
/* #define HS_LINE_MASK    0x07FFFFFF */
#define CW_DATA_FIELD_MASK 0x00FFFFFF
#define DATA_FIELD_SHFT_IN_HSW  5
#define DATA_FIELD_SHFT_IN_LSW  27
#define MAX_HW_LOOPS    16777215

void fifoOMaskHsl(FIFO_ID pFifoId,int whichone, unsigned long bitsToTurnOn)
/* pFifoId - fifo Object identifier */
/* whichone - Standard or Extended */
/* bitsToTurnOn - Bits to OR with HSlines */
{
    if (pFifoId == NULL)
      return;

    if (whichone == STD_HS_LINES)
    {
       pFifoId->HSLines |= (bitsToTurnOn & HS_LINE_MASK); /* Std HS Lines */
    }
    else
    {   
       pFifoId->HSLinesExt |= bitsToTurnOn;     /* Extended High Speed Lines */
    }
    return;
}

/**************************************************************
*
*  fifoUnMaskHsl - AND the value and HSlines together
*  Clears current high speed line bits given by the bits indicated
*  with the argument "BitsToClear".
*  Arguments:
*   which_hs_lines  : 0=standard, 1=optional
*   bits_to_clear   : hs line bits to clear
*
* RETURNS:
*     void
*/
void fifoOUnMaskHsl(FIFO_ID pFifoId,int whichone, unsigned long BitsToClear)
/* pFifoId - fifo Object identifier */
/* whichone - Standard or Extended */
/* BitsToClear - value to AND with HSlines */
{
    if (pFifoId == NULL)
      return;

    if (whichone == STD_HS_LINES)
    {
       pFifoId->HSLines &= (~BitsToClear & HS_LINE_MASK); /* Std HS Lines */
    }
    else
    {   
       pFifoId->HSLinesExt &= ~BitsToClear;     /* Extended High Speed Lines */
    }
    return;
}       


void fifoStuffOCmd(FIFO_ID pFifoId, unsigned long cntrlBits, unsigned long DataField)
{
   unsigned long fifowords[3];

    if (pFifoId == NULL)
      return;

     /* if delay then correct for 1st 100nsec is 5 ticks not 8 ticks */ 
     if ( cntrlBits == CL_DELAY )   
      DataField -= HW_DELAY_FUDGE; 

    /* word = cntrlBits | (0x07FFFFFF & (datafield >> 5);  hw */
    fifowords[0] = cntrlBits | (CW_DATA_FIELD_MASK &
                           (DataField >> DATA_FIELD_SHFT_IN_HSW));
 
    /* word = (0xff & DataField) << 27;   lw */
    fifowords[1] = ((0xff & DataField) << DATA_FIELD_SHFT_IN_LSW) |
                        (pFifoId->HSLines & HS_LINE_MASK);

   printf("      Cntrl: 0x%lx, data: 0x%lx (%lu), HSLines: 0x%lx  fwords: 0x%lx, 0x%lx\n",
		cntrlBits, DataField, DataField, pFifoId->HSLines, fifowords[0], fifowords[1]);
}


void errLogRet(int prtOpt, char *fileNline, ...)
/* int prtOpt - Unix Version compatibility, No function */
/* char* fileNline - String containing file names & line number */
{
   printf("errLogRet: \n");
 
   return;
}

void errLogSysRet(int prtOpt, char *fileNline, ...)
/* int prtOpt - Unix Version compatibility, No function */
/* char* fileNline - String containing file names & line number */
{
   printf("errLogRet: \n");
   return;
}

#define    X_AXIS               0
#define    Y_AXIS               1
#define    Z_AXIS               2

int oblgradient(unsigned short apdelay,unsigned short apaddr,
		int maxval,int minval,int value)
{
   return(0);
}

void owriteapword(unsigned short apaddr,unsigned short apval, unsigned short apdelay)
{
}

/*
PCHAN_SETOBLMATRIX 0xFB000000L 
PCHAN_SETOBLGRADZ 0xFA000000L 
PCHAN_SETOBLGRADY 0xF9000000L 
PCHAN_SETOBLGRADX 0xF8000000L 
*/



#endif

/*======================================================================== */

newDelay(PCHANSORT_ID pcsId)
{
   newGDelay(pcsId,pcsId->wordLatch[(pcsId->presentMinChan)*2], 
		pcsId->wordLatch[((pcsId->presentMinChan)*2)+1]);
}

newGDelay(PCHANSORT_ID pcsId,long cmd,long ticks)
{
    long tmpHSlines;

    if (pcsId->DstBuf != NULL)
    {
       /* rngLPut(pcsId->DstBuf,&(pcsId->wordLatch[(pcsId->presentMinChan)*2]),2); */
       rngLPut(pcsId->DstBuf,&cmd,1);
       rngLPut(pcsId->DstBuf,&ticks,1);
       /* rngLShow(pcsId->DstBuf,2); */
    }
    else
    {
     /* if M1Delay present put into buffer prior to over writing */
     if (pcsId->prevDelay[1] != 0)
     {
	   tmpHSlines = pcsId->pFifoId->HSLines;
	   pcsId->pFifoId->HSLines = pcsId->prevHSLines;
   	   if (DebugLevel > SortOutputLevel+1)
   	   {
              if (pcsId->prevDelay[0] == 0)
              {
                printf("Fifo: %4ld %7.4lf us TI, Delay %4ld, %7.4lf, HSLines: 0x%lx\n",
		        pcsId->FifoTI,pcsId->FifoTI*0.0125,
			pcsId->prevDelay[1],pcsId->prevDelay[1]*0.0125,
			pcsId->pFifoId->HSLines);
	        pcsId->FifoTI += pcsId->prevDelay[1];
              }
              else
              {
                 printf("Fifo: %4ld %7.4lf us TI, ApBus %4ld, %7.4lf, Data: 0x%lx (%lu), HSLines: 0x%lx\n",
		        pcsId->FifoTI,pcsId->FifoTI*0.0125,8,8*0.0125,
			pcsId->prevDelay[1],pcsId->prevDelay[1],pcsId->pFifoId->HSLines);
	        pcsId->FifoTI += 8;
              }

	   }
           fifoStuffOCmd(pcsId->pFifoId, pcsId->prevDelay[0],pcsId->prevDelay[1]);
           pcsId->pFifoId->HSLines = tmpHSlines;
     }
     pcsId->prevDelay[0] = cmd; /* pcsId->wordLatch[(pcsId->presentMinChan)*2]; */
     pcsId->prevDelay[1] = ticks; /* pcsId->wordLatch[((pcsId->presentMinChan)*2)+1]; */
     pcsId->prevHSLines = pcsId->pFifoId->HSLines;
   }
}

forcePrevDelay(PCHANSORT_ID pcsId)
{
    long tmpHSlines;

    if (pcsId->DstBuf == NULL)
    {
     /* if M1Delay present put into buffer prior to over writing */
     if (pcsId->prevDelay[1] != 0)
     {
	   tmpHSlines = pcsId->pFifoId->HSLines;
	   pcsId->pFifoId->HSLines = pcsId->prevHSLines;
   	   if (DebugLevel > SortOutputLevel+1)
   	   {
              if (pcsId->prevDelay[0] == 0)
              {
                printf("Fifo: %4ld %7.4lf us TI, Delay %4ld, %7.4lf, HSLines: 0x%lx\n",
		        pcsId->FifoTI,pcsId->FifoTI*0.0125,
			pcsId->prevDelay[1],pcsId->prevDelay[1]*0.0125,
			pcsId->pFifoId->HSLines);
	        pcsId->FifoTI += pcsId->prevDelay[1];
              }
              else
              {
                 printf("Fifo: %4ld %7.4lf us TI, ApBus %4ld, %7.4lf, Data: 0x%lx (%lu), HSLines: 0x%lx\n",
		        pcsId->FifoTI,pcsId->FifoTI*0.0125,8,8*0.0125,
			pcsId->prevDelay[1],pcsId->prevDelay[1],pcsId->pFifoId->HSLines);
	        pcsId->FifoTI += 8;
              }

	   }
           fifoStuffOCmd(pcsId->pFifoId, pcsId->prevDelay[0],pcsId->prevDelay[1]);
           pcsId->pFifoId->HSLines = tmpHSlines;
     }
     pcsId->prevDelay[0] = 0L;
     pcsId->prevDelay[1] = 0L;
     pcsId->prevHSLines = 0L;
   }
}

/* format or two words are the arguments to fifoStuffCmd: 1st CntrBits, 2nd Ticks */
void pchanSort(PCHANSORT_ID pcsId)
{
   int i,state;
   int pChanIndex;
   int wordType;
   int words;
   int allSrcEmpty;
   int ReEstablishMinDelay;
   int type;
   long delay;
   int delaycnt, apcnt, othercnt, gradcnt;
   int lockedChan = -1;
   int oblGradAxisSetFlag[3] = { 0, 0, 0 };
   int oblGradAxisDuration[3] = { 0, 0, 0 };
   int presentMinType;

   /* force out of present buffer into FIFO */
   /*  Used in conjuction with diagnostic output from fifoBufStuffer */
   /*
   if (pcsId->DstBuf == NULL)  
      fifoBufForceRdy(pcsId->pFifoId->pFifoWordBufs);  
   */

#ifdef INSTRUMENT
      wvEvent(EVENT_PARALLEL_SORT,NULL,NULL);
#endif
   pcsId->presentMinChan = -1;

   /* be sure destination buffer is empty before we start */
   /*
   if (pcsId->DstBuf != 0) 
      rngLFlush(pcsId->DstBuf);
*/

   if (DebugLevel > SortOutputLevel)
   {
     printf("\n --- Sorting Embedded Level %d -----\n",EmbeddedSortLevel);
     if (DebugLevel > SortOutputLevel+1)
     {
        printf("Contents of chans: \n");
        for(i = 0; i < pcsId->numSrcs; i++)
        {
	   printf("--------- Channel %d -----------\n",i);
	   rngLShow(pcsId->SrcBufs[i],2);
        }
        printf("------------------------------------\n\n");
        printf("Destination Buffer = 0x%lx (0=fifo)\n",pcsId->DstBuf);
        if (pcsId->DstBuf != 0) 
	   rngLShow(pcsId->DstBuf,2);
        printf("------------------------------------\n\n\n");
     }
   }

   allSrcEmpty = FALSE;

   /* Initalize holding buffers */
   for(i = 0; i < pcsId->numSrcs; i++)
   {
      if (abortSort)
	 return;

      pcsId->ChanTimeAccum[i] = 0L;
      state = readSrc(pcsId,i);
      /* words = rngLGet(pcsId->SrcBufs[i],&(pcsId->wordLatch[i*2]),2); */
      /* DPRINT3(1,"init: %d - Cntrl 0x%lx, data: 0x%lx\n",
	  i,pcsId->wordLatch[i*2],pcsId->wordLatch[(i*2)+1]); */
      if (state == EMPTY)
      {
	/* should have no emptys on start, OK maybe not make message on debuglevel 1/20/2000 */
        DPRINT1(SortOutputLevel+1,"initial hold buffer fill, PChanBuf %d was EMPTY ...........\n",i);
    	/*errLogRet(LOGIT,debugInfo,
		"initial hold buffer fill, PChanBuf %d was EMPTY ...........\n",i); */
        pcsId->pchanEmpty[i] = TRUE;
        pcsId->numPChanEmpty++;
        if (pcsId->numPChanEmpty >= pcsId->numSrcs)
	  allSrcEmpty = TRUE;	
      }
   }

   pcsId->presentPChan = 0;
   state = CHECK_TYPE;

   pcsId->TimeIndex = 0L;	/* Time Index  (in ticks) */

   if (DebugLevel > SortOutputLevel+1)
       showStates(pcsId,state);

   while(!allSrcEmpty)
   {
      if (abortSort)
	 return;

      switch(state)
      {

    case READ_SRCBUF: /* read in entry from present sort channel */
	 state = readSrc(pcsId,pcsId->presentPChan);
	 break;

    case NEXT_HOLDBUF:  /* go to next sort channel buffer, i.e. incr pcsId->presentPChan */
	 nextHoldBuf(pcsId);
	 state = determineType(pcsId->wordLatch[(pcsId->presentPChan)*2]);
	 break;

    case CHECK_TYPE:  /* determine entry type */
             state = determineType(pcsId->wordLatch[(pcsId->presentPChan)*2]);
             break;

    case GATEON:     /* hsline gate on instruction */
             /* pcsId->pFifoId->HSLines */
	     /* if embedded pchan then put gate instructioninto
		next sort input buffer */
	     if (pcsId->DstBuf != NULL)  
	     {
		rngLPut(pcsId->DstBuf,&(pcsId->wordLatch[(pcsId->presentPChan)*2]),2);
	     }
	     else
             {
	        fifoOMaskHsl(pcsId->pFifoId,STD_HS_LINES, 
			pcsId->wordLatch[((pcsId->presentPChan)*2)+DATA_INDEX]);
	     }
	     /*DPRINT2(-1,"GATEON: Chan: %d, HSLines: 0x%lx\n",
		pcsId->presentPChan,pcsId->pFifoId->HSLines); */
   	     if (DebugLevel > SortOutputLevel+1)
	     {
		 long delta;
   	         /* showStates(pcsId,state); */
		 delta = pcsId->TimeIndex - pcsId->ChanTimeIndex[pcsId->presentPChan];
                 printf("%4ld TI, '%8s': 0x%08lx, HSL: 0x%08lx, \n                     TI: Spec'd: %8.4lf, Now: %8.4lf (us), delta: %8.4lf us\n",
		pcsId->TimeIndex,
		getState(state), 
		pcsId->wordLatch[((pcsId->presentPChan)*2)+DATA_INDEX],
                pcsId->pFifoId->HSLines,
		(pcsId->ChanTimeIndex[pcsId->presentPChan]*0.0125),
		(pcsId->TimeIndex*0.0125),delta*0.0125);
	     }
	     state = READ_SRCBUF;
	     break;

    case GATEOFF:  /* hsline gate off instruction */
             /* pcsId->pFifoId->HSLines */
	     /* if embedded pchan then put gate instructioninto
		next sort input buffer */
	     if (pcsId->DstBuf != NULL)  
	     {
		rngLPut(pcsId->DstBuf,&(pcsId->wordLatch[(pcsId->presentPChan)*2]),2);
	     }
	     else
             {
	        fifoOUnMaskHsl(pcsId->pFifoId,STD_HS_LINES, 
			pcsId->wordLatch[((pcsId->presentPChan)*2)+DATA_INDEX]);
	     }
	    /* DPRINT2(-1,"GATEOFF: Chan: %d, HSLines: 0x%lx\n",
		pcsId->presentPChan,pcsId->pFifoId->HSLines); */
   	     if (DebugLevel > SortOutputLevel+1)
	     {
		 long delta;
   	         /* showStates(pcsId,state); */
		 delta = pcsId->TimeIndex - pcsId->ChanTimeIndex[pcsId->presentPChan];
                 printf("%4ld TI, '%8s': 0x%08lx, HSL: 0x%08lx, \n                   TI: Spec'd: %8.4lf, Now: %8.4lf (us), delta: %8.4lf us\n",
		pcsId->TimeIndex,
		getState(state), 
		pcsId->wordLatch[((pcsId->presentPChan)*2)+DATA_INDEX],
                pcsId->pFifoId->HSLines,
		(pcsId->ChanTimeIndex[pcsId->presentPChan]*0.0125),
		(pcsId->TimeIndex*0.0125),delta*0.0125);
	     }
	     state = READ_SRCBUF;
	     break;


    case SETOBLGRADX:  /* The min delay is determine and the gradient is */
    case SETOBLGRADY:  /* set if not already (via GRADXYZ_APBUS_DELAYS) */
    case SETOBLGRADZ:  /* */
	    state = FIND_MIN;
	    break;

    case SETOBLMATRIX:
             /* pcsId->pFifoId->HSLines */
	     /* if embedded pchan then put setmatrix instruction into
		next sort input buffer */
	     DPRINT2(SortOutputLevel+1,"SETOBLMATRIX: Chan: %d, Data: 0x%lx\n",
		pcsId->presentPChan,
			pcsId->wordLatch[((pcsId->presentPChan)*2)+1]);
	     if (pcsId->DstBuf != NULL)  
	     {
		rngLPut(pcsId->DstBuf,&(pcsId->wordLatch[(pcsId->presentPChan)*2]),2);
	     }
	     else
             {
		 setgradmatrix(pcsId->wordLatch[((pcsId->presentPChan)*2)+DATA_INDEX]);
                 oblGradAxisSetFlag[0] = oblGradAxisSetFlag[1] = oblGradAxisSetFlag[2] = 0;
                 oblGradAxisDuration[0] = oblGradAxisDuration[1] = oblGradAxisDuration[2] = 0;

	     }
   	     if (DebugLevel > SortOutputLevel+1)
	     {
		/*      Definition of oblgradmatrix: 9 longs                  */
		/*              0 = ro(x)  1 = pe(x)  2 = ss(x)               */
		/*              3 = ro(y)  4 = pe(y)  5 = ss(y)               */
		/*              6 = ro(z)  7 = pe(z)  8 = ss(z)               */
#ifndef VXWORKS
		printf("ro(x):\t0x%08lx,\tpe(x):\t0x%08lx,\tss(x):\t0x%08lx\n",
			oblgradmatrix[0],oblgradmatrix[1],oblgradmatrix[2]);
		printf("ro(y):\t0x%08lx,\tpe(y):\t0x%08lx,\tss(y):\t0x%08lx\n",
			oblgradmatrix[3],oblgradmatrix[4],oblgradmatrix[5]);
		printf("ro(z):\t0x%08lx,\tpe(z):\t0x%08lx,\tss(z):\t0x%08lx\n",
			oblgradmatrix[6],oblgradmatrix[7],oblgradmatrix[8]);
#endif
	     }
	     state = READ_SRCBUF;
	     break;



   case DELAY:
   	     /* showStates(pcsId,state); */
	    /* negative time carryover then remove from this new delay */
	    /*printf("Delay: Chan: %d, TimeIndex: %8.4lf us, %ld ticks\n",
		    pcsId->presentPChan,pcsId->ChanTimeIndex[pcsId->presentPChan]*0.0125,pcsId->ChanTimeIndex[pcsId->presentPChan]); */
	    if (pcsId->carryOver[pcsId->presentPChan] != 0)
            {
	        DPRINT3(SortOutputLevel+1,"DELAY: Chan: %d, Ticks = %ld, CarryOver = %ld, ",
			pcsId->presentPChan, 
		        pcsId->wordLatch[((pcsId->presentPChan)*2)+1],
			pcsId->carryOver[pcsId->presentPChan]);
		pcsId->wordLatch[((pcsId->presentPChan)*2)+1] += 
			         pcsId->carryOver[pcsId->presentPChan];
		if ( pcsId->wordLatch[((pcsId->presentPChan)*2)+1] < 0 ) 
		{
		   pcsId->carryOver[(pcsId->presentPChan)] =  pcsId->wordLatch[((pcsId->presentPChan)*2)+1];
		   pcsId->wordLatch[((pcsId->presentPChan)*2)+1] = 0L;
	        }
    		else
		   pcsId->carryOver[pcsId->presentPChan] = 0;

	        DPRINT1(SortOutputLevel+1," Corrected Delay: %ld\n", 
		        pcsId->wordLatch[((pcsId->presentPChan)*2)+DATA_INDEX]);
            }
	    state = FIND_MIN;
	    break;

   case APBUS: 		/* APDelay fixed at 100nsec or 8 ticks */
   	    /* showStates(pcsId,state); */
	    /*printf("Apbus: Chan: %d, TimeIndex: %8.4lf us, %ld ticks\n",
		    pcsId->presentPChan,pcsId->ChanTimeIndex[pcsId->presentPChan]*0.0125,pcsId->ChanTimeIndex[pcsId->presentPChan]); */
	    state = FIND_MIN;
	    break;

      case ALL_DELAYS :
   	     if (DebugLevel > SortOutputLevel+2)
   	         showStates(pcsId,state);
   	     if (DebugLevel > SortOutputLevel+1)
	     {
		 long delta;
		 delta = pcsId->TimeIndex - pcsId->ChanTimeIndex[pcsId->presentMinChan];
                 printf("%4ld TI, '%8s': %ld,%7.4lf us, HSL: 0x%08lx, \n                     TI: Spec'd: %8.4lf, Now: %8.4lf, delta: %8.4lf\n",
		pcsId->TimeIndex,
		getState(ALL_DELAYS), 
		pcsId->wordLatch[((pcsId->presentMinChan)*2)+DATA_INDEX],
		pcsId->wordLatch[((pcsId->presentMinChan)*2)+DATA_INDEX]*0.0125,
                pcsId->pFifoId->HSLines,
		(pcsId->ChanTimeIndex[pcsId->presentMinChan]*0.0125),
		(pcsId->TimeIndex*0.0125),delta*0.0125);
	     }
	     if ( (pcsId->wordLatch[((pcsId->presentMinChan)*2)+1] < 8) &&
		  (pcsId->DstBuf == NULL) )
	     {
		   int needed = 8 - pcsId->wordLatch[((pcsId->presentMinChan)*2)+1];
   	           /* showStates(pcsId,state); */
                   /* no previous delay to work on then just punt! */
		   if (pcsId->prevDelay[1] != 0)
		   {
		      if (needed >= 4) 
		      {
			/* zero this delay and add time to previous delay */
			DPRINT2(0,"          Zero this Delay and add %ld to Previous Delay of %ld\n",
				   pcsId->wordLatch[((pcsId->presentMinChan)*2)+1],
				   pcsId->prevDelay[1]);
			pcsId->prevDelay[1] += 
				    pcsId->wordLatch[((pcsId->presentMinChan)*2)+1];
	               pcsId->TimeIndex += pcsId->wordLatch[((pcsId->presentMinChan)*2)+1];
		       pcsId->wordLatch[((pcsId->presentMinChan)*2)+1] = 0;
		       pcsId->presentPChan = pcsId->presentMinChan;
		       state = SUB_DELAY;
                      }
		      else   /* needed <=4 */
		      {
		       /* round this delay to 100nsec and subtract needed from previous delay */
			DPRINT2(0,"          Round up this Delay by %ld to 100nsec and subtract it from previous Delay of %ld\n",
				needed,pcsId->prevDelay[1]);
		       pcsId->prevDelay[1] -= needed;
	               pcsId->TimeIndex += pcsId->wordLatch[((pcsId->presentMinChan)*2)+1];
		       pcsId->wordLatch[((pcsId->presentMinChan)*2)+1] += needed;
                       newDelay(pcsId);
		       state = SUB_DELAY;
		      }
		   }
		   else
		   {
		       pcsId->presentMinDelay += needed; /* apbus 100 nsec */
		       pcsId->wordLatch[((pcsId->presentMinChan)*2)+1] += needed;
	               pcsId->TimeIndex += pcsId->wordLatch[((pcsId->presentMinChan)*2)+1];
		        DPRINT1(0,"    ----> W A R N I N G: added %d ticks to achieve min delay <---\n",
			needed);	
                       newDelay(pcsId);
		       state = SUB_DELAY;
		   }
	      }
	      else
	      {
                  newDelay(pcsId);
		  state = SUB_DELAY;
	          pcsId->TimeIndex += pcsId->wordLatch[((pcsId->presentMinChan)*2)+1];
		  /*
	          fifoStuffOCmd(pcsId->pFifoId,
			pcsId->wordLatch[(pcsId->presentMinChan)*2],
			pcsId->wordLatch[((pcsId->presentMinChan)*2)+1]);
		  */
	      }
	      pcsId->ApBusCntDwn -= pcsId->presentMinDelay;
	      break;

     case LOCKCHAN:
	     if (pcsId->DstBuf != NULL)
             {
                rngLPut(pcsId->DstBuf,&(pcsId->wordLatch[(pcsId->presentPChan)*2]),2);
	        state = READ_SRCBUF;
             }
             else
             {
	        if (lockedChan == -1)
                {
                    lockedChan = pcsId->presentPChan;
	            state = READ_SRCBUF;
      	        }
	        else
                {
	            state = FIND_MIN;
      	        }
             }
	     break;

     case UNLOCKCHAN:
	     if (pcsId->DstBuf != NULL)
             {
                rngLPut(pcsId->DstBuf,&(pcsId->wordLatch[(pcsId->presentPChan)*2]),2);
	        state = READ_SRCBUF;
             }
             else
             {
	        if ( lockedChan == pcsId->presentPChan )
	           lockedChan = -1;
                else
		   printf("Obtained unlock from wrong channel %d, should be %d\n",
			   pcsId->presentPChan,lockedChan);
	     }
	     state = READ_SRCBUF;
	  break;

     case FORCED_APDELAY:
		
		/* Force a min delay of 32 ticks */
		pcsId->carryOver[pcsId->presentMinChan] -= 32;
                pcsId->presentMinDelay = 32;
                newGDelay(pcsId,0L,32L);
	        pcsId->TimeIndex += 32L;
		subtractDelay(pcsId);
                pcsId->ApBusCntDwn = 0L;
		state = FIND_MIN;
	   break;

     case APBUS_DELAYS:
	    /* pcsId->presentMinChan = pcsId->presentPChan; */
	    /* pcsId->presentMinDelay = AP_EDELAY; */
	    /* if to soon for another AP then hold off and force a delay */

/* been test on one case and appears to work at this point, but not yet standard
   in the compile.
*/
#ifdef REFINE_FORCED_DELAY
	    if (pcsId->ApBusCntDwn > 0)
            {
	   	if ( determineType(pcsId->wordLatch[pcsId->presentMinChan*2])
		     != DELAY )
		{
		   /* Force a min delay of 32 ticks */
                   state = FORCED_APDELAY;
		   break;
		}
		else
		{
		   /* if min is a delay, then add enough to this delay to allow 
		      apbus instruction instead of forcing 32 ticks */
                   int needed, added;
		   needed = pcsId->ApBusCntDwn +   /* ticks needed to allow apbus */
			pcsId->wordLatch[(pcsId->presentMinChan*2)+1];
		   needed = (needed < 8) ? 8 :  needed;
		   /* ticks added to min delay to achieve "needed" ticks */
		   added = needed - 
			pcsId->wordLatch[(pcsId->presentMinChan*2)+1];
		   pcsId->carryOver[pcsId->presentMinChan] -= added;
		   pcsId->wordLatch[(pcsId->presentMinChan*2)+1] = 0L;
                   newGDelay(pcsId,0L,needed);
                   pcsId->presentMinDelay = needed;
	           pcsId->TimeIndex += needed;
		   subtractDelay(pcsId);
                   pcsId->ApBusCntDwn = 0L;
		   state = FIND_MIN;
		   break;
		}
            }
#else
            /* normal Method */
	    if (pcsId->ApBusCntDwn > 0)
            {
		/* Force a min delay of 32 ticks */
                state = FORCED_APDELAY;
		break;
            }
#endif
	    else
            {
   	        if (DebugLevel > SortOutputLevel+1)
	        {
		   long delta;
		   delta = pcsId->TimeIndex - pcsId->ChanTimeIndex[pcsId->presentMinChan];
                   printf("%4ld TI, '%8s': 0x%08lx, HSL: 0x%08lx, \n                     TI: Spec'd: %8.4lf, Now: %8.4lf, delta: %8.4lf\n",
		      pcsId->TimeIndex,
		      getState(state), 
		      pcsId->wordLatch[((pcsId->presentMinChan)*2)+DATA_INDEX],
                      pcsId->pFifoId->HSLines,
		      (pcsId->ChanTimeIndex[pcsId->presentMinChan]*0.0125),
		      (pcsId->TimeIndex*0.0125),delta*0.0125);
                }
   	            /* showStates(pcsId,state); */

		/* put any pending delay into the pipe before the apbus command */
	        forcePrevDelay(pcsId);

		/* put apbus command into the pipe */
		if (pcsId->DstBuf != NULL)
		{
		   rngLPut(pcsId->DstBuf,&(pcsId->wordLatch[(pcsId->presentMinChan)*2]),2);
		   /* rngLShow(pcsId->DstBuf,2); */
		}
		else
		{
   	           if (DebugLevel > SortOutputLevel+1)
   	           {
                      printf("Fifo: %4ld %7.4lf us TI, ApBus %4ld, %7.4lf, Data: 0x%lx (%lu), HSLines: 0x%lx\n",
		        pcsId->FifoTI,pcsId->FifoTI*0.0125,8,8*0.0125,
			pcsId->wordLatch[((pcsId->presentMinChan)*2)+1],
			pcsId->wordLatch[((pcsId->presentMinChan)*2)+1],
			pcsId->pFifoId->HSLines);
	                pcsId->FifoTI += 8;
                   }
		   fifoStuffOCmd(pcsId->pFifoId,
			pcsId->wordLatch[(pcsId->presentMinChan)*2],
			pcsId->wordLatch[((pcsId->presentMinChan)*2)+1]);
		}

		/* wait 400nsec before next ap */
	        pcsId->ApBusCntDwn = 32; /* 8 + 8 + 8 + 8;  */

	        pcsId->TimeIndex += pcsId->presentMinDelay;
		state = SUB_DELAY;
            }
            break;



     case GRADXYZ_APBUS_DELAYS:
           /* normal Method */
            if (pcsId->ApBusCntDwn > 0)
            {
                /* Force a min delay of 32 ticks */
                state = FORCED_APDELAY;
                break;
            }
	    else
            {
	        unsigned short axis;
   	        if (DebugLevel > SortOutputLevel+1)
	        {
		   long delta;
		   delta = pcsId->TimeIndex - pcsId->ChanTimeIndex[pcsId->presentMinChan];
                   printf("%4ld TI, '%8s': 0x%08lx, HSL: 0x%08lx, \n                     TI: Spec'd: %8.4lf, Now: %8.4lf, delta: %8.4lf\n",
		      pcsId->TimeIndex,
		      getState(state), 
		      pcsId->wordLatch[((pcsId->presentMinChan)*2)+DATA_INDEX],
                      pcsId->pFifoId->HSLines,
		      (pcsId->ChanTimeIndex[pcsId->presentMinChan]*0.0125),
		      (pcsId->TimeIndex*0.0125),delta*0.0125);
                }
   	            /* showStates(pcsId,state); */

                type = determineType(pcsId->wordLatch[pcsId->presentMinChan*2]);
                switch(type)
                {
                   case SETOBLGRADX:    axis = X_AXIS;  break;
                   case SETOBLGRADY:    axis = Y_AXIS;  break;
                   case SETOBLGRADZ:    axis = Z_AXIS;  break;
                }

                if (oblGradAxisSetFlag[axis] == 0)
 		{

		/* put any pending delay into the pipe before the apbus command */
	        forcePrevDelay(pcsId);

		/* put gradient apbus command into the pipe */
		if (pcsId->DstBuf != NULL)
		{
		   rngLPut(pcsId->DstBuf,&(pcsId->wordLatch[(pcsId->presentMinChan)*2]),2);
		}
		else
		{
   	           if (DebugLevel > SortOutputLevel+1)
   	           {
		       printf("Axis: %d (0-X,1-Y,2-Z), valueL 0x%lx\n",
				axis, pcsId->wordLatch[((pcsId->presentMinChan)*2)+1]);
                   }
		   setoblgrad(axis,pcsId->wordLatch[((pcsId->presentMinChan)*2)+1]);
                   oblGradAxisSetFlag[axis] = 1;
                   oblGradAxisDuration[axis] = pcsId->presentMinDelay;
		}
	        pcsId->TimeIndex += pcsId->presentMinDelay;
		state = SUB_DELAY;

		}
		else
 		{
		  pcsId->presentPChan = pcsId->presentMinChan;
		  pcsId->carryOver[pcsId->presentMinChan] += pcsId->presentMinDelay;
		  state = READ_SRCBUF;
		}
             }
	    break;

    case ACQSTART:
	     if (pcsId->DstBuf != NULL)  
	     {
		rngLPut(pcsId->DstBuf,&(pcsId->wordLatch[(pcsId->presentPChan)*2]),2);
	     }
	     else
             {
		pcsId->AcqLpCnt = 1;
		pcsId->AcqAccumTime = 0;
		/*
		   Acquisition at present is locked, thus this channel is forced
		   to be the Min Channel
		*/
		pcsId->presentMinChan = pcsId->presentPChan;
		/* force any delays and HSlines setting prior to acquire words */
	        forcePrevDelay(pcsId);
             }
             state = READ_SRCBUF;
	     break;

    case ACQLPCNT:
	     if (pcsId->DstBuf != NULL)  
	     {
		rngLPut(pcsId->DstBuf,&(pcsId->wordLatch[(pcsId->presentPChan)*2]),2);
	     }
	     else
             {
		/* hardware loop count is one less than count, e.g. value 799 is a loop of 800 */
		pcsId->AcqLpCnt = 
		    pcsId->wordLatch[((pcsId->presentPChan)*2)+DATA_INDEX] + 1;
	        forcePrevDelay(pcsId);
		newGDelay(pcsId,pcsId->wordLatch[(pcsId->presentPChan)*2],
                          pcsId->wordLatch[((pcsId->presentPChan)*2)+1]);
             }
	     state = READ_SRCBUF;
	     break;

    case ACQSTARTLP:
	     if (pcsId->DstBuf != NULL)  
	     {
		rngLPut(pcsId->DstBuf,&(pcsId->wordLatch[(pcsId->presentPChan)*2]),2);
	     }
	     else
             {
		pcsId->AcqAccumTime +=
                    ( (pcsId->wordLatch[((pcsId->presentPChan)*2)+DATA_INDEX] + HW_DELAY_FUDGE) * pcsId->AcqLpCnt);
	        forcePrevDelay(pcsId);
		newGDelay(pcsId,pcsId->wordLatch[(pcsId->presentPChan)*2],
                          pcsId->wordLatch[((pcsId->presentPChan)*2)+1]);
             }
	     state = READ_SRCBUF;
	     break;

    case ACQCTC:
	     if (pcsId->DstBuf != NULL)  
	     {
		rngLPut(pcsId->DstBuf,&(pcsId->wordLatch[(pcsId->presentPChan)*2]),2);
	     }
	     else
             {
		pcsId->AcqAccumTime +=
                    ( (pcsId->wordLatch[((pcsId->presentPChan)*2)+DATA_INDEX] + HW_DELAY_FUDGE) * pcsId->AcqLpCnt);
	        forcePrevDelay(pcsId);
		newGDelay(pcsId,pcsId->wordLatch[(pcsId->presentPChan)*2],
                          pcsId->wordLatch[((pcsId->presentPChan)*2)+1]);
             }
	     state = READ_SRCBUF;
	     break;

    case ACQENDLP:
	     if (pcsId->DstBuf != NULL)  
	     {
		rngLPut(pcsId->DstBuf,&(pcsId->wordLatch[(pcsId->presentPChan)*2]),2);
	     }
	     else
             {
		pcsId->AcqAccumTime +=
                    ( (pcsId->wordLatch[((pcsId->presentPChan)*2)+DATA_INDEX] + HW_DELAY_FUDGE) * pcsId->AcqLpCnt);
		pcsId->AcqLpCnt = 1;
	        forcePrevDelay(pcsId);
		newGDelay(pcsId,pcsId->wordLatch[(pcsId->presentPChan)*2],
                          pcsId->wordLatch[((pcsId->presentPChan)*2)+1]);
             }
	     state = READ_SRCBUF;
	     break;

    case ACQEND:
	     if (pcsId->DstBuf != NULL)  
	     {
		rngLPut(pcsId->DstBuf,&(pcsId->wordLatch[(pcsId->presentPChan)*2]),2);
                state = READ_SRCBUF;
	     }
	     else
             {
		/* force last CTC out prior to any new HSLines or other instructions */
	        forcePrevDelay(pcsId);

		/* take the total 'at' time and subtract it from other channels */
		/* 1st take care of any carryOver on this channel */
	        if (pcsId->carryOver[pcsId->presentPChan] != 0)
                {
	          DPRINT3(SortOutputLevel+1,"ACQEND: Chan: %d, Ticks = %ld, CarryOver = %ld, ",
			pcsId->presentPChan, pcsId->AcqAccumTime,
			pcsId->carryOver[pcsId->presentPChan]);
		  pcsId->AcqAccumTime += 
			         pcsId->carryOver[pcsId->presentPChan];
		  if ( pcsId->AcqAccumTime < 0 ) 
		  {
		     pcsId->carryOver[(pcsId->presentPChan)] =  pcsId->AcqAccumTime;
		     pcsId->AcqAccumTime = 0L;
	          }
    		  else
		     pcsId->carryOver[pcsId->presentPChan] = 0;

	          DPRINT1(SortOutputLevel+1," Corrected Delay: %ld\n", 
		          pcsId->AcqAccumTime);
                }
	        pcsId->presentMinDelay = pcsId->AcqAccumTime;
		pcsId->presentMinChan = pcsId->presentPChan;
	        state = SUB_DELAY;
             }
	     break;


     case SUB_DELAY: /* subtract delay from remaining delays . 
			if AP then subtract delay from carryover 
			if a delay goes negative then add to carryover */
	     for (i=0; i < pcsId->numSrcs; i++)
	     {
		/* if min chan then skip subtraction, skip needed for apbus case */
		/* if chan is empty skip subtraction */
	        if ( (i == pcsId->presentMinChan) || (pcsId->pchanEmpty[i] == TRUE) )
		   continue;

		type = determineType(pcsId->wordLatch[i*2]);
		switch(type)
		{
		   /* since the data word is not a delay for these types the time is
		      removed from the carryOver, when the carryOver is added to the
		      next available delay the time will be accounted for.
  		   */
		   case APBUS:  
		   case LOCKCHAN:
    		   case SETOBLGRADX:
    		   case SETOBLGRADY:
    		   case SETOBLGRADZ:
    		   case ACQSTART:   /* actually these 6 types should never have to be */
    		   case ACQLPCNT:   /* subtracted from since these should be locked */
    		   case ACQSTARTLP: /* and completed prior to any subtraction occurring */
    		   case ACQCTC:
    		   case ACQENDLP:
    		   case ACQEND:
			pcsId->carryOver[i] -= pcsId->presentMinDelay;
			break;

		   /* subtract the event delay for the delay data word directly */
		   case DELAY:
		        pcsId->wordLatch[(i*2)+1] -= pcsId->presentMinDelay;
		        if ( pcsId->wordLatch[(i*2)+1] < 0 ) 
			{
			   pcsId->carryOver[i] +=  pcsId->wordLatch[(i*2)+1];
			   pcsId->wordLatch[(i*2)+1] = 0L;
	                }
			break;

		   default:
			DPRINT1(-1,"SUB_DELAY: Unknown Type: %d\n",type);
		        break;
		}
             }
	     pcsId->presentPChan = pcsId->presentMinChan;
   	     if (DebugLevel > SortOutputLevel+3)
   	         showStates(pcsId,state);
	     state = READ_SRCBUF;
	     break;

      /* search for min delay  (returns ALLDELAYS, APBUS&DELAYS, ZEROMIN */
      case FIND_MIN:  

	   delaycnt = apcnt = othercnt = gradcnt = 0;
           ReEstablishMinDelay = TRUE;
           presentMinType = -1;
	   for(i=0; i < pcsId->numSrcs; i++)
           {
               /* if chan is Empty then skip it */
	      if ( (pcsId->pchanEmpty[i] == TRUE) )
		   continue;

	      type = determineType(pcsId->wordLatch[i*2]);
                switch(type)
                {
		   /* APBUS by definition is the minimum event, even if there are delays less than it. */
                   case APBUS:
			apcnt++;

#ifdef REFINE_FORCED_DELAY
			if ( ( AP_EDELAY < pcsId->presentMinDelay) 
			     || (ReEstablishMinDelay == 1)
			     || ((lockedChan != -1) && (lockedChan == i)) ) /* locked chan ap is always the one */
#else

			/* Normal */
			if ( ( AP_EDELAY < pcsId->presentMinDelay) 
                             || (apcnt == 1)  /* no matter if a delay < AP_EDELAY, force APBUS to be the min */
			     || (ReEstablishMinDelay == 1)
			     || ((lockedChan != -1) && (lockedChan == i)) ) /* locked chan ap is always the one */
#endif

			{
			   pcsId->presentMinDelay = AP_EDELAY;
			   pcsId->presentMinChan = i;
			   presentMinType = type;
			   ReEstablishMinDelay = 0;
                        }
                        break;
 
                   case DELAY:
			/* (apcnt == 0), if apbus has been found then no longer check for delays to be
			   less than apbus, (actually this may occur, but apbus are the minumum event 
			   by definition */
			delaycnt++;
#ifdef REFINE_FORCED_DELAY
			if ( (( pcsId->wordLatch[(i*2)+1] < pcsId->presentMinDelay)
			     || (ReEstablishMinDelay == 1)) ) 
#else
			/* Normal */
			if ( (( pcsId->wordLatch[(i*2)+1] < pcsId->presentMinDelay)
			     || (ReEstablishMinDelay == 1)) && (apcnt == 0) ) 
#endif
			{
			   pcsId->presentMinDelay = pcsId->wordLatch[(i*2)+1];
			   pcsId->presentMinChan = i;
			   presentMinType = type;
			   ReEstablishMinDelay = 0;
                        }
                        break;

    		   case SETOBLGRADX:
    		   case SETOBLGRADY:
    		   case SETOBLGRADZ:
			{
   			  int tickduration = pcsId->wordLatch[(i*2)] & 0xFFFFF;
			  gradcnt++;
			
			  if ( (( tickduration < pcsId->presentMinDelay)
			       || (ReEstablishMinDelay == 1)) && (apcnt == 0)
			       || ((lockedChan != -1) && (lockedChan == i)) ) /* locked chan*/
			  {
			     pcsId->presentMinDelay = tickduration;
			     pcsId->presentMinChan = i;
			     presentMinType = type;
			     ReEstablishMinDelay = 0;
			  }
			}
			break;
		    
		   case GATEON:
		   case GATEOFF:
    		   case SETOBLMATRIX:
			othercnt++;
			break;

		   case LOCKCHAN:
			if (lockedChan == -1)
 			   othercnt++;
			else
			   delaycnt++;
			break;
 
                   default:
		       othercnt++;
                        DPRINT1(-1,"FIND_MIN: Unknown Type: %d\n",type);
                        break;
                }
	   }
   	   /* showStates(pcsId,state); */
	   /* Note: Order of Test is Important, don't change! */
           if (othercnt)
	      state = NEXT_HOLDBUF;
           else if (pcsId->presentMinDelay == 0)
           {
	      /* state = MIN_ZERO; */
	      pcsId->presentPChan = pcsId->presentMinChan;
	      state = READ_SRCBUF;
	   }
	   else
	   {
		switch(presentMinType)
		{
		   case DELAY: 
			state = ALL_DELAYS; 
			break;
		   case APBUS: 
	      		state = APBUS_DELAYS;
			break;
    		   case SETOBLGRADX:
    		   case SETOBLGRADY:
    		   case SETOBLGRADZ:
		   	state = GRADXYZ_APBUS_DELAYS;
			break;
		   default: 
			state = READ_SRCBUF; 
			break;
		}
           }
           break;


    case MIN_ZERO:
   	   /* showStates(pcsId,state); */
	   pcsId->presentPChan = pcsId->presentMinChan;
	   state = READ_SRCBUF;
	   break;

    case EMPTY: /* Mark pChan as EMpty */
   	     if (DebugLevel > SortOutputLevel+2)
   	         showStates(pcsId,state);
             if (pcsId->pchanEmpty[pcsId->presentPChan] != TRUE)
             {
                pcsId->pchanEmpty[pcsId->presentPChan] = TRUE;
                /* pcsId->wordLatch[pcsId->presentPChan*2] = 0xFFF00000L; */
	        pcsId->numPChanEmpty++;
                if (pcsId->numPChanEmpty >= pcsId->numSrcs)
		  allSrcEmpty = TRUE;	
	     } 
	     state = NEXT_HOLDBUF;
	     break;
   default:
   	     showStates(pcsId,state);
	     DPRINT1(-1,"Sort default case, state = %d, unknown\n",state);
   			/* Aaaaah !! */
	    break;
   }
  }
  /* force HSline to the last state for the last delay */
  forcePrevDelay(pcsId);

#ifdef INSTRUMENT
      wvEvent(EVENT_PARALLEL_CMPLT,NULL,NULL);
#endif
  if (DebugLevel > SortOutputLevel+1)
  {
     printf("------------------------------------------------------------------\n");
     printf("Destination Buffer(0x%lx,0=fifo) Content:\n",pcsId->DstBuf);
	if (pcsId->DstBuf != 0) rngLShow(pcsId->DstBuf,2);
     printf("------------------------------------------------------------------\n");
     printf("\n");
     for(i = 0; i < pcsId->numSrcs; i++)
     {
        printf("Total Duration: Chan: %d, TimeIndex: %8.4lf us, %ld ticks\n",
	    i,pcsId->ChanTimeIndex[i]*0.0125,pcsId->ChanTimeIndex[i]);
     }
     printf("Actually Duration After Sort: %8.4lf us, %ld ticks\n",
	pcsId->TimeIndex*0.0125,pcsId->TimeIndex);
     printf("Fifo Duration After Sort: %8.4lf us, %ld ticks\n",
	pcsId->FifoTI*0.0125,pcsId->FifoTI);
   }
   /* force out of present buffer into FIFO */
   /*  Used in conjuction with diagnostic output from fifoBufStuffer */
   /*
   if (pcsId->DstBuf == NULL)  
      fifoBufForceRdy(pcsId->pFifoId->pFifoWordBufs);  
   */
    DPRINT(SortOutputLevel,"======================> Sort Completed <======================= \n");

  return;

}

readSrc(PCHANSORT_ID pcsId,int channel)
{
   int words,state;
   if (pcsId->pchanEmpty[channel] == FALSE)
   {
     /*DPRINT1(-1,"READ_SRCBUF: presentChan = %d \n",pcsId->presentPChan);*/
      words = rngLGet(pcsId->SrcBufs[channel], 
                     &(pcsId->wordLatch[channel*2]),2);
      if (words == 0)
      {
          pcsId->wordLatch[channel*2] = 0xFFF00000L;
	  state = EMPTY;
      }
      else
	  state = determineType(pcsId->wordLatch[channel*2]);

        pcsId->ChanTimeIndex[channel] = pcsId->ChanTimeAccum[channel];
        if (state == DELAY)
	{
	    pcsId->ChanTimeAccum[channel] += pcsId->wordLatch[(channel*2)+1];
	}
        else if (state == APBUS)
	{
	    pcsId->ChanTimeAccum[channel] += AP_EDELAY;
	}
      if (DebugLevel > SortOutputLevel+1)
      {
	 printf("\n     Rd Chan %d ",channel);
         showHoldBuf(pcsId,channel); printf("\n");
      }
         /* showStates(pcsId,state); */
   }
   else
	state = NEXT_HOLDBUF;
   return(state);
}

nextHoldBuf(PCHANSORT_ID pcsId)
{
   /* increment present Pchan index; */
   pcsId->presentPChan++;

   if (pcsId->presentPChan >= pcsId->numSrcs)
      pcsId->presentPChan = 0;
}

subtractDelay(PCHANSORT_ID pcsId)
{
   int i,type;
	     for (i=0; i < pcsId->numSrcs; i++)
	     {
		/* if min chan then skip subtraction, skip needed for apbus case */
		/* if chan is empty skip subtraction */
	        if ( (i == pcsId->presentMinChan) || (pcsId->pchanEmpty[i] == TRUE) )
		   continue;

		type = determineType(pcsId->wordLatch[i*2]);
		switch(type)
		{
		   case APBUS:
    		   case SETOBLGRADX:
    		   case SETOBLGRADY:
    		   case SETOBLGRADZ:
    		   case ACQSTART:   /* actually these 6 types should never have to be */
    		   case ACQLPCNT:   /* subtracted from since these should be locked */
    		   case ACQSTARTLP: /* and completed prior to any subtraction occurring */
    		   case ACQCTC:
    		   case ACQENDLP:
    		   case ACQEND:
			pcsId->carryOver[i] -= pcsId->presentMinDelay;
			pcsId->carryOver[i] -= pcsId->presentMinDelay;
			break;

		   case DELAY:
		        pcsId->wordLatch[(i*2)+1] -= pcsId->presentMinDelay;
		        if ( pcsId->wordLatch[(i*2)+1] < 0 ) 
			{
			   pcsId->carryOver[i] +=  pcsId->wordLatch[(i*2)+1];
			   pcsId->wordLatch[(i*2)+1] = 0L;
	                }
			break;

		   default:
			DPRINT1(-1,"SUB_DELAY: Unknown Type: %d\n",type);
		        break;
		}
             }
}

determineType(unsigned long cntrl)
{
   int type;
   type = -1;
   /* DPRINT1(-1,"determineType: type = 0x%lx\n",cntrl); */
   cntrl &= 0xFFF00000L;
   /* DPRINT1(-1,"determineType: type = 0x%lx\n",cntrl); */
   switch(cntrl)
   {
	case CL_DELAY:      type = DELAY; 	break;
	case 0xFF000000L:   type = GATEON; 	break;
	case 0xFE000000L:   type = GATEOFF; 	break;
	case 0xFD000000L:   type = LOCKCHAN; 	break;
	case 0xFC000000L:   type = UNLOCKCHAN; 	break;
	case 0xFFF00000L:   type = EMPTY; 	break;
	case CL_AP_BUS:     type = APBUS; 	break;
	case CL_START_LOOP: 
	case CL_START_LOOP | CL_CTC: 
			    type = ACQSTARTLP;   break;
	case CL_END_LOOP | CL_CTC:   
			    type = ACQENDLP;     break;

	case CL_LOOP_COUNT: type = ACQLPCNT;     break;
	case CL_CTC:        type = ACQCTC;     break;
	case 0xF7000000L:   type = ACQSTART;	break;
	case 0xF6000000L:   type = ACQEND;	break;
	case 0xF8000000L:   type = SETOBLGRADX; 	break;
	case 0xF9000000L:   type = SETOBLGRADY; 	break;
	case 0xFA000000L:   type = SETOBLGRADZ; 	break;
	case 0xFB000000L:   type = SETOBLMATRIX; 	break;
	default:
		errLogRet(LOGIT,debugInfo,"Invalid Type in Parallel: 0x%lx\n",cntrl);
#ifdef VXWORKS
       		GenericException.exceptionType = HARD_ERROR;  
       		GenericException.reportEvent = HDWAREERROR+INVALPARALLELTYPE;   /* errorcode is returned */
    		/* send error to exception handler task */
    		msgQSend(pMsgesToPHandlr, (char*) &GenericException, 
		   sizeof(EXCEPTION_MSGE), NO_WAIT, MSG_PRI_NORMAL);
#endif
     		type = EMPTY;
		break;

   }
   return(type);
}

char *getState(int state)
{
  int i;
  char *name = NULL;
  for(i=0; i < TYPESIZE; i++)
  {
     if (state == typenames[i].typeValue)
     {
	name = typenames[i].typeName;
     }
  }
  if (name == NULL)
     name = "Unknown ";
  return(name);
}

showStates(PCHANSORT_ID pcsId,int instate)
{
  int i,type;
  /*printf("TimeIndex: %4ld (%8.4lf usec), State: '%8s' ",pcsId->TimeIndex,
		(pcsId->TimeIndex*0.0125),getState(instate));
  */
  printf("%4ld TI, '%8s': ",pcsId->TimeIndex,getState(instate));
  showHoldBuf(pcsId,pcsId->presentMinChan);
  printf("HSL: 0x%08lx, MDelay: %5ld,  MChan: %2d\n",
	pcsId->pFifoId->HSLines,pcsId->presentMinDelay,pcsId->presentMinChan);
}

showHoldBuf(PCHANSORT_ID pcsId,int channel)
{
  int i,type;
  for(i=0; i < pcsId->numSrcs; i++)
  {  
    char *star;
    star = (channel == i) ? "*" : "";
    type = determineType(pcsId->wordLatch[i*2]);
    switch(type)
    {
	case DELAY:
		   printf("Delay %5ld%s, ",pcsId->wordLatch[(i*2)+1],star);
		   break;
	case APBUS:
		   printf("Apbus 5%s, ",star);
		   break;
        case GATEON:
		   printf("GateOn 0x%08lx%s, ",pcsId->wordLatch[(i*2)+1],star);
		   break;
        case GATEOFF:
		   printf("GateOff 0x%08lx%s, ",pcsId->wordLatch[(i*2)+1],star);
		   break;
	case LOCKCHAN:
		   printf("LkChan %2d%s, ",i,star);
		   break;

	case UNLOCKCHAN:
		   printf(" UnLkChan %2d%s, ",i,star);
		   break;
        case SETOBLGRADX:
		   printf("SetGradX %5ld,0x%08lx%s, ",(pcsId->wordLatch[(i*2)] & 0x000FFFFF),
				pcsId->wordLatch[(i*2)+1],star);
		   break;
        case SETOBLGRADY:
		   printf("SetGradY %5ld,0x%08lx%s, ",(pcsId->wordLatch[(i*2)] & 0x000FFFFF),
				pcsId->wordLatch[(i*2)+1],star);
		   break;
        case SETOBLGRADZ:
		   printf("SetGradZ %5ld,0x%08lx%s, ",(pcsId->wordLatch[(i*2)] & 0x000FFFFF),
				pcsId->wordLatch[(i*2)+1],star);
		   break;

	case SETOBLMATRIX:
		   printf("SetOblMatrix 0x%08lx%s, ",pcsId->wordLatch[(i*2)+1],star);
		   break;

	case ACQSTARTLP:
		   printf("AcqStartLp 0x%08lx%s, ",pcsId->wordLatch[(i*2)+1],star);
		   break;

	case ACQENDLP:
		   printf("AcqEndLp 0x%08lx%s, ",pcsId->wordLatch[(i*2)+1],star);
		   break;

	case ACQLPCNT:
		   printf("AcqLpCnt 0x%08lx%s, ",pcsId->wordLatch[(i*2)+1],star);
		   break;

	case ACQCTC:
		   printf("AcqCTC 0x%08lx%s, ",pcsId->wordLatch[(i*2)+1],star);
		   break;

	case ACQSTART:
		   printf("AcqStart 0x%08lx%s, ",pcsId->wordLatch[(i*2)+1],star);
		   break;

	case ACQEND:
		   printf("AcqEnd 0x%08lx%s, ",pcsId->wordLatch[(i*2)+1],star);
		   break;

        case EMPTY:
		   printf(" Empty %s\t",star);
		   break;
        default:
		   printf("Unkwn 0x%lx%s\t",pcsId->wordLatch[(i*2)],star); 
                   break;

    }
    printf("CO: %5d, ",pcsId->carryOver[i]);
  }
}

void pchanShowChans(PCHANSORT_ID pcsId)
{
   int i;

   if (pcsId == NULL)
	return;

   printf("Contents of chans: \n");
   for(i = 0; i < pcsId->numSrcs; i++)
   {
	printf("--------- Channel %d -----------\n",i);
	rngLShow(pcsId->SrcBufs[i],1);
   }
   printf("------------------------------------\n\n\n");
}
