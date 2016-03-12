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
/* --------------------- dataObj.c ----------------------------- */

#ifndef ALLREADY_POSIX
 #define _POSIX_SOURCE /* defined when source is to be POSIX-compliant */
#endif
#include <vxWorks.h>
#include <stdlib.h>
#include <semLib.h>
#include <rngLib.h>
#include <memLib.h>
#include <msgQLib.h>
#include "instrWvDefines.h"
#include "commondefs.h"
#include "logMsgLib.h"
#include "rngXBlkLib.h"
#include "hashLib.h"
/* #include "timeconst.h" */
/* #include "vmeIntrp.h" */
/* #include "taskPrior.h" */
#include "hostAcqStructs.h"
#include "expDoneCodes.h"
/* #include "errorcodes.h" */
#include "ddr_symbols.h"
#include "dataObj.h"

/*
modification history
--------------------
4-07-2004,gmb  created 
*/

/*
DESCRIPTION

  This module handles the creation, initialization of the Data Object.
The other public interfaces manage the data memory allocation from the
Data and pertinate data parameters.

*/

static DATAOBJ_ID pDataId = NULL;

#ifdef INOVA_STYLE
extern int SA_Criteria; 
extern unsigned long SA_Mod; /* modulo for SA, ie which fid to stop at 'il' & ct*/
extern unsigned long SA_CTs;  /* Completed Transients for SA */

/* Exception Msges to Phandler, e.g. FOO, etc. */
extern EXCEPTION_MSGE HardErrorException;
extern EXCEPTION_MSGE GenericException;

/* extern MSG_Q_ID pUpLinkMsgQ;	/* MsgQ used between UpLinker and STM Object */

/* static FID_STAT_BLOCK ErrStatBlk;  Used for Programmatic Exception, (i.e. SA, AA) */
/* static FID_STAT_BLOCK WrnStatBlk;  Used for Programmatic Exception, Warning messages */
#endif


/**************************************************************
*
*  maxBufSize - determine the maximum system buffering that can be 
*  tolerated.  
*
*
* RETURNS:
* maxBufSIze  - if no error,
*
*/ 
static long maxBufSize(long fidSize,long totalFidBlks,unsigned long dspMemSize)
{
   long maxLimit;
   long maxFidBlksInSTM;
   long bytesPerStatBlk,  bytesPerMsgQ,  bytesPerRng; 
   long maxMallocSize, maxStructSize;

   /* - What is the maximum number of FID blocks that can  buffered */
   maxFidBlksInSTM = dspMemSize / fidSize;
   DPRINT3(-2,"maxBufSize: maxFidBlksInSTM = %ld, == %lu / %lu\n",maxFidBlksInSTM,dspMemSize,fidSize);

   /* 
      What is the maximum number of FID block stuctures 
      (i.e. FID_BLOCK_ARRAY, MSG_Qs, RINGXBLKs)
      that can be malloced with the self imposed malloc limit
      of the structures.
     Note: 1 MSG_Qs, 1 RINGs, 1 malloc buffer
   */

   maxStructSize = MAX_BUFFER_ALLOCATION / 
		   ( sizeof(FID_STAT_BLOCK) + sizeof(ITR_MSG) + sizeof(long));
  
   DPRINT1(-2,"maxBufSize:  maxStructSize: %ld\n",maxStructSize);

   /* --- who's the limiting size maxStructSize or maxFidBlksInSTM ? --- */
   maxLimit = (maxStructSize < maxFidBlksInSTM) ? maxStructSize : maxFidBlksInSTM;

   /* --- who's the limiting size maxLimit or totalFidBlks ? --- */
   return( (maxLimit < totalFidBlks) ? maxLimit : totalFidBlks );
}

/*******************************************************
* getMemSize - determine size of Memory on STM board 
*
*  RETURNS
*    size in bytes;
*/
static unsigned long getMemSize()
{
     /* Now test at each 4MB address boundry to find size */
     unsigned long strtaddr;
     int i,j;
     unsigned long cnt,size;

     /* request info from DSP, not implimented */
     /* return(64*1024*1024);  /* 64 MB */
     /* use constant from ddr_symbols.h */
     return(DDR_DATA_SIZE); /* at present 64 MB 0x04000000 */
}

int getDspAppVer()
{
     return 1;
}
/*-------------------------------------------------------------
| Data Object Public Interfaces
+-------------------------------------------------------------*/
/**************************************************************
*
*  dataCreate - create the DATA Object Data Structure & Semaphore, etc..
*
*
* RETURNS:
* DATAOBJ_ID  - if no error, NULL - if mallocing or semaphore creation failed
*
*/ 
DATAOBJ_ID dataCreate(int dataChannel,char *idStr)
/* int dataChannel - STM channel = 0 - 3, for the 4 four possible stm's in system */
{
  char tmpstr[80];
  DATAOBJ_ID pDataObj;
  int tDRid, tMTid, tDEid;
  short sr;
  long memval;
  int cnt,slen;
  unsigned long maxNumOfEntries;

  unsigned long memAddr;

  if (idStr == NULL)
  {
      slen = 16;
  }
  else
  {
     slen = strlen(idStr);
  }

  /* ------- malloc space for STM Object --------- */
  if ( (pDataObj = (DATA_OBJ *) malloc( sizeof(DATA_OBJ)) ) == NULL )
  {
    errLogSysRet(LOGIT,debugInfo,"dataCreate: ");
    return(NULL);
  }

  /* zero out structure so we don't free something by mistake */
  memset(pDataObj,0,sizeof(DATA_OBJ));

  pDataObj->dspAppVersion = getDspAppVer();

  pDataObj->dspMemSize = getMemSize();

  /* --------------  setup given or default ID string ---------------- */
  pDataObj->pIdStr = (char *) malloc(slen+2);
  if (pDataObj->pIdStr == NULL)
  {
     dataDelete(pDataObj);
     return(NULL);
  }
  if (idStr != NULL)
     strcpy(pDataObj->pIdStr,idStr);
  else
     strcpy(pDataObj->pIdStr,"dataObj");

  /* -------------------------------------------------------------------*/

  pDataObj->dataState = OK;

  /* create the STM State sync semaphore */
  pDataObj->pSemStateChg = semBCreate(SEM_Q_FIFO,SEM_EMPTY);

  /* create the STM Object Mutual Exclusion semaphore */
  pDataObj->pStmMutex = semMCreate(SEM_Q_PRIORITY | SEM_INVERSION_SAFE |
                                  SEM_DELETE_SAFE);

  if ( (pDataObj->pSemStateChg == NULL) ||
       (pDataObj->pStmMutex == NULL) )
  {
    errLogSysRet(LOGIT,debugInfo,"dataCreate: Could not create semaphore ");
     dataDelete(pDataObj);
     return(NULL);
  }

   /* Make initial buffer, msgQ, freelist, may expand */
   maxNumOfEntries = MAX_BUFFER_ALLOCATION / 
		   ( sizeof(FID_STAT_BLOCK) + sizeof(ITR_MSG) + sizeof(long));
  
   pDataObj->maxFidBlkBuffered = pDataObj->maxFreeList = maxNumOfEntries;

   pDataObj->pStatBlkArray = (FID_STAT_BLOCK*) malloc(sizeof(FID_STAT_BLOCK) *
                            maxNumOfEntries);

   /* Free List of Tags, typically 0 - maxNumFidBlkToAlloc */
   pDataObj->pTagFreeList = rngXBlkCreate(maxNumOfEntries,
                               "Data Addr Free Pool ",EVENT_STM_ALLOC_BLKS,1);

  DPRINT1(-1," %lu MBytes of Memory.\n",(pDataObj->dspMemSize/1048576L));
  pDataId = pDataObj;  /* set internal static pointer */
  return( pDataObj );
}

/*
 * called only if needed
 */
reAllocateResources(DATAOBJ_ID pStmId, ulong_t entries)
{
    free(pStmId->pStatBlkArray);
    pStmId->pStatBlkArray = (FID_STAT_BLOCK*) malloc(sizeof(FID_STAT_BLOCK) *
                            entries);
    if (pStmId->pStatBlkArray == NULL)
    {
       errLogSysRet(LOGIT,debugInfo,
	    "reAllocateResources: A StatBLock Array could not be re-Allocated");
      return(ERROR);
    }

    rngXBlkDelete(pStmId->pTagFreeList);   /* delete old make new */
    pStmId->pTagFreeList = rngXBlkCreate(entries,
                               "STM Data Addr Free Pool ",EVENT_STM_ALLOC_BLKS,1);
    if (pStmId->pTagFreeList == NULL)
    {
       errLogSysRet(LOGIT,debugInfo,
	    "reAllocateResources: Tag Free List could not be re-Allocated");
      return(ERROR);
    }
    pStmId->maxFidBlkBuffered = pStmId->maxFreeList = entries;
   return(0);
}


/**************************************************************
*
*  dataInitial - Initializes the Data Object, based on the experiment
*
*
*  This routine initializes the DATA Object base on the experimental
*parameters. The parameters:
*   <fidSize> is the smallest FID size (bytes) for the experiment
*   <totalFidBlks> is the total number of fid blocks to acquire. 
*  Note: <totalFidBlks> is NOT the number of FID Elements but instead
*the number of FID elements times the number of blocksizes (nt/bs). 
*E.G. arraydim=10,nt=32 & bs=8 then <totalFidBlks> = 10 * (32/8) = 40
*
*
* RETURNS:
*  OK or ERROR
*
*	Author Greg Brissey 8/5/93
*/
int dataInitial(DATAOBJ_ID pStmId, ulong_t totalFidBlks, ulong_t fidSize)
/* DATAOBJ_ID 	pStmId - data Object identifier */
/* totalFidBlks Number of FID BLocks to be acquired */
/* fidSize Size of FID in bytes */
{
   char *addr;
   long maxNumFidBlkToAlloc;
   long i;
   FID_STAT_BLOCK *pStatBlk; /* pointer to FID Entry */

   extern long maxBufSize();

   if (pStmId == NULL)
     return(ERROR);

   DPRINT2(-1,"dataInitial: nFids: %lu, size: %lu\n", totalFidBlks, fidSize);
   /*
      The msg Q That the ISR use to pass on Info is Passed as an Argument
      it is Owned by the Uplinker
	e.g. interrupt type, CT or NT, Tag value
   */

   maxNumFidBlkToAlloc = maxBufSize(fidSize, totalFidBlks,pStmId->dspMemSize);
   DPRINT4(-1,"maxBufSize: fidsize: %lu, fidblks: %lu, stm memsize: %lu, maxNumFidBlkToAlloc: %ld\n",
		fidSize,totalFidBlks,pStmId->dspMemSize,maxNumFidBlkToAlloc);

   /* if requested number is greater than already exist will need to realloc */
   if (maxNumFidBlkToAlloc > pStmId->maxFreeList)
   {
      if (reAllocateResources(pStmId,maxNumFidBlkToAlloc))
      {
         dataFreeAllRes(pStmId);
         errLogSysRet(LOGIT,debugInfo,
	    "stmInitial: A resource could not be Allocated");
         return(ERROR);
      }
   }

   /* Fill Free List with the list of Tag Values (0 to maxNumFidBlk...) */
   pStmId->maxFidBlkBuffered = maxNumFidBlkToAlloc;
   rngXBlkFlush(pStmId->pTagFreeList); 
   for (i=0; i<maxNumFidBlkToAlloc; i++)
   {
       rngXBlkPut(pStmId->pTagFreeList, &i, 1); 
   }

   /* Mark all StatBlocks as Not Allocated */
   for (i=0; i<maxNumFidBlkToAlloc; i++)
   {
       pStatBlk = &((pStmId->pStatBlkArray)[i]);
       pStatBlk->doneCode = NOT_ALLOCATED;
       pStatBlk->errorCode = 0;
       pStatBlk->fidAddr = NULL;
       pStatBlk->dataSize = fidSize;	 /* FID size in bytes */
       pStatBlk += sizeof(FID_STAT_BLOCK);
   }
   return( OK );
}

/**************************************************************
*
*  dataFreeAllRes - Frees all resources allocated in dataInitial
*
*
* RETURNS:
*  OK or ERROR
*
*	Author Greg Brissey 4/8/04
*/
int dataFreeAllRes(DATAOBJ_ID pStmId)
/* DATAOBJ_ID 	pStmId - data Object identifier */
{
   if (pStmId == NULL)
     return(ERROR);

   DPRINT(-1,"dataFreeAllRes");
   if (pStmId->pStatBlkArray != NULL)
      free(pStmId->pStatBlkArray);

   if (pStmId->pTagFreeList != NULL)
      rngXBlkDelete(pStmId->pTagFreeList);

   pStmId->dataState = OK;
   pStmId->pStatBlkArray = NULL;
   pStmId->pTagFreeList = NULL;

  return(OK);
}
/**************************************************************
*
*  dataDelete - Deletes DATA Object and  all resources
*
*
* RETURNS:
*  OK or ERROR
*
*	Author Greg Brissey 9/9/93
*/
int dataDelete(DATAOBJ_ID pStmId)
/* DATAOBJ_ID 	pStmId - stm Object identifier */
{
   if (pStmId == NULL)
     return(ERROR);

   dataFreeAllRes(pStmId);
   if (pStmId->pSemStateChg != NULL)
       semDelete(pStmId->pSemStateChg);
   if (pStmId->pStmMutex != NULL)
       semDelete(pStmId->pStmMutex);
   free(pStmId);
}

/*----------------------------------------------------------------------*/
/* dataShwResrc							*/
/*     Show system resources used by Object (e.g. semaphores,etc.)	*/
/*	Useful to print then related back to WindView Events		*/
/*----------------------------------------------------------------------*/
VOID dataShwResrc(DATAOBJ_ID pStmId, int indent)
{
   int i;
   char spaces[40];

   for (i=0;i<indent;i++) spaces[i] = ' ';
   spaces[i]='\0';

   printf("%sData Obj: '%s', 0x%lx\n",spaces,pStmId->pIdStr,pStmId);
   printf("%s    Binary Sems: pSemStateChg -- 0x%lx\n",spaces,pStmId->pSemStateChg);
   printf("%s    Mutex:       pStmMutex  ---- 0x%lx\n",spaces,pStmId->pStmMutex);
   printf("%s    Tag Free List:\n",spaces);
   rngXBlkShwResrc(pStmId->pTagFreeList,indent+4);
   printf("\n");
}

/**************************************************************
*
*  dataTake - obtain DATA Mutex 
*
*  Obtain DATA mutex, used primarily for data transfer .
*
*  RETURNS
*   void
*/
void dataTake(DATAOBJ_ID pStmId )
{
   if (pStmId == NULL)
     return;

#ifdef INSTRUMENT
     wvEvent(EVENT_STM_MUTEX_TAKE,NULL,NULL);
#endif
   semTake(pStmId->pStmMutex,WAIT_FOREVER);
   return;
}

/**************************************************************
*
*  dataGive - give back DATA Mutex 
*
*  Give back DATA mutex, used primarily for data transfer .
*
*  RETURNS
*   void
*/
void dataGive(DATAOBJ_ID pStmId )
{
   if (pStmId == NULL)
     return;

#ifdef INSTRUMENT
     wvEvent(EVENT_STM_MUTEX_GIVE,NULL,NULL);
#endif
   semGive(pStmId->pStmMutex);
   return;
}

/**************************************************************
*
*  dataGetState - Obtains the current DATA Status
*
*  This routines Obtains the status of the DATA via 3 different modes.
*
*   NO_WAIT - return the present value immediately.
*   WAIT_FOREVER - waits till the STM Status has changed 
*			and and returns this new value.
*   TIME_OUT - waits till the STM Status has changed or 
*		    the number of <secounds> has elasped 
*		    (timed out) before returning.
*
*  NOTE: The Task that calls this routine with 
*	 STM_WAIT_FOREVER or STM_TIME_OUT will block !!
*     
*
*
* RETURNS:
* STM state - if no error, TIME_OUT - if in STM_TIME_OUT mode call timed out
*	      or ERROR if called with invalid mode;
*
*/ 
int dataGetState(DATAOBJ_ID pStmId, int mode, int secounds)
{
   int state;

   if (pStmId == NULL)
     return(ERROR);

   switch(mode)
   {
     case NO_WAIT:
          state = pStmId->dataState;
	  break;

     case WAIT_FOREVER: /* block if state has not changed */
	  semTake(pStmId->pSemStateChg, WAIT_FOREVER);  
          state = pStmId->dataState;
	  break;

     case TIME_OUT:     /* block if state has not changed, until timeout */
          if ( semTake(pStmId->pSemStateChg, (sysClkRateGet() * secounds) ) != OK )
	     state = TIME_OUT;
          else 
             state = pStmId->dataState;
          break;

     default:
	  state = ERROR;
	  break;
   }
   return(state);
}

/**************************************************************
*
*  dataAllocAcqBlk - Obtains the next free DATA memory block (May Block)
*
*  This routines Obtains the next free block of resources for
*data acquisition. A Stat Block and Data Space are reserved and keyed
*to the Tag value returned. No mallocing is occurring.
*  The <fid_element> is that FID that this acquisition data 
*block belongs.  Since there will be more than one block per FID 
*if blocksize (bs) is not zero.
*  The <strtCt> is the logical starting ct for this data block.  
*Although each data block starts at zero completed transients the 
*logical start transient may not be zero. When bs is not zero this 
* will be the case. For an experiment where bs=8 nt=32, each FID 
* has 4 data blocks to acquire (nt/bs), thus the logical
*starting transient for the data blocks are: 
*  1st - 0, 2nd - 9, 3rd - 17, 4th - 25.
*This allows the host to blocksize average properly.
*  The <fidSize> is the FID Size in Bytes, this value need not be
*a constant. (i.e. arrayed np is allowed)
*  <Tag> is the Key that is used to identify these resource to the
*stmObj in future calls. This Tag is typically programmed to be return
* via a STM interrupt to identify the data.
*  <vmeAddr> this is the VME Address to place the Data
*  <apAddr> this is the AP bus Address of this STM 
*
*  RETURNS
*    FID_STAT_BLOCK*, or Blocks if no free Tag value is available
*/
FID_STAT_BLOCK* dataAllocAcqBlk(DATAOBJ_ID pStmId, ulong_t fid_element, ulong_t np, ulong_t strtCt, ulong_t endCt, ulong_t nt, ulong_t fidSize, long *Tag, ulong_t* vmeAddr)
/* pStmId 	- stm Object Id */
/* fid_element - FID that this acquisition data block belongs */
/* np	       - Number of data points of data */
/* strtCt      - The logical starting transient of this data block */
/* endCt      -  The end CT of this data block */
/* fidSize     - The Size of the Data Block (FID) in bytes */
/* Tag	       - Tag value returned */
/* vmeAddr     - VME Address of Data */
{
  int nfreetags;
  long newTag;
  long *p2FreeTags;
  FID_STAT_BLOCK* pPrevStatBlk;
  FID_STAT_BLOCK* pStatBlk;
  FID_STAT_BLOCK* pStatArray;

   if (pStmId == NULL)
     return(NULL);

#ifdef INSTRUMENT
     wvEvent(EVENT_STM_ALLOC_BLK,NULL,NULL);
#endif
     /* get an Unused Tag Value, if none available then BLOCK */
     rngXBlkGet(pStmId->pTagFreeList, Tag, 1);
     DPRINT1(-1,"dataAllocAcqBlk: Tag Value: %ld\n",*Tag);

     /* Get Pointer to FID_STAT_BLOCK referenced via Tag Value */
     pStatBlk = &((pStmId->pStatBlkArray)[*Tag]);

     /* Get Pointer to previous allocated FID_STAT_BLOCK referenced via Tag Value */
     if (*Tag != 0)
     {
       pPrevStatBlk = &((pStmId->pStatBlkArray)[((*Tag) - 1)]);
     }

      /* we don't calc Memory Addr, since the DSP keep track of this itself and returns
         the address to us when the data block is complete
      */
    pStatBlk->fidAddr = (long*) 0;

     pStatBlk->elemId = fid_element;	/* FID # */
     pStatBlk->startct = strtCt;	
     pStatBlk->ct = endCt;	
     pStatBlk->nt = nt;	
     pStatBlk->np = np;	
     pStatBlk->dataSize = fidSize;	 /* FID size in bytes */
     pStatBlk->doneCode = NO_DATA;

/*
     DPRINT6(-1,"dataAllocAcqBlk() - fid_element: %ld, strtCt: %ld, endCt: %lu, nt: %lu, np: %lu, fidSize: %lu\n",
	fid_element, strtCt, endCt, nt, np, fidSize);
     DPRINT6(-1,"dataAllocAcqBlk() - fid_element: %ld, strtCt: %ld, endCt: %lu, nt: %lu, np: %lu, fidSize: %lu\n",
	pStatBlk->elemId,pStatBlk->startct, pStatBlk->ct,pStatBlk->nt,pStatBlk->np,pStatBlk->dataSize);

     DPRINT4(-1,"dataAllocAcqBlk: Tag Value: %ld, StatBlock Addr: 0x%lx, Size: %ld (0x%lx)\n",
	*Tag,pStatBlk,pStatBlk->dataSize,pStatBlk->dataSize);
*/


   return( pStatBlk );
}

int dataAllocWillBlock(DATAOBJ_ID pStmId)
/* pStmId 	- stm Object Id */
{
   return(rngXBlkIsEmpty(pStmId->pTagFreeList));
}

/* incase the parser is blocked try to get a data buffer, 
 *  this routine place the tag zero into the rng buffer
 *  which unblock the parser.
 *  For an abort which this should only be used, this is safe 
*/
int dataForceAllocUnBlock(DATAOBJ_ID pStmId)
{
   int tag = 0;
   rngXBlkPut(pStmId->pTagFreeList,(long*) &tag,1);
   DPRINT(-1,"dataForceAllocUnBlock: Force block Alloc to unblock\n");
   return( OK );
}



int dataPeekAtNextTag(DATAOBJ_ID pStmId, int *nxtTag)
{
   int fromP,stat;
   stat = RNG_LONG_PEEK(pStmId->pTagFreeList, nxtTag, fromP);
   if (stat == 0)  /* buffer empty, so guess at next value */
   {
       if ( (*nxtTag + 1) == pStmId->maxFreeList ) /* set back to zero */
         *nxtTag = 0;
       else
         *nxtTag = *nxtTag +1;  /* increment for next */
   }
   return stat;
}

int dataDataBufsAllFree(DATAOBJ_ID pStmId)
/* pStmId 	- stm Object Id */
{
   int used,ready;
   used = rngXBlkNElem(pStmId->pTagFreeList);
   ready = (used == pStmId->maxFidBlkBuffered) ? 1 : 0;
   DPRINT3(2,"stmBufsAllFree: used: %d, Exp total: %d, ready: %d\n",
	used,pStmId->maxFidBlkBuffered ,ready);
   return(ready);
}

FID_STAT_BLOCK *dataGetStatBlk(DATAOBJ_ID pStmId,long dataTag)
{
   FID_STAT_BLOCK *pStatBlk;
   DPRINT1(1,"dataGetStatBlk: tag: %d\n",dataTag);
   if ( (dataTag >= 0) && (dataTag < pStmId->maxFidBlkBuffered) )
   {
       /* Get Pointer to FID_STAT_BLOCK referenced via Tag Value */
       pStatBlk = &((pStmId->pStatBlkArray)[dataTag]);
   }
   else
   {
       errLogSysRet(LOGIT,debugInfo,"dataGetStatBlk: Illegal tag value: %ld",dataTag);
       pStatBlk = NULL;
   }
   return(pStatBlk);
}

/**************************************************************
*
*  dataFreeFidBlk - Frees all STM resources associated with the FID Block
*
*  This routines Frees the STM resources associated with a FID Block
* reference thought its index. This includes:
*  1. Marking the Stat Block as NOT_ALLOCATED
*  2. Place the Tag value back into the Tag Free List
*
* NOTE: A Task calling this routine could block if the FID 
*	Block Free List is full.
*
*  RETURNS
*    OK  or ERROR
*/
int dataFreeFidBlk(DATAOBJ_ID pStmId,long tag)
/* pStmId - stm Object Id */ 
/* tag - index to the FID Block, etc.. */
{
  FID_STAT_BLOCK *pStatBlk;

  if (pStmId == NULL)
     return(ERROR);

  if (tag < 0)
     return(-1);
  pStatBlk = &((pStmId->pStatBlkArray)[tag]);
  pStatBlk->doneCode = NOT_ALLOCATED;
  pStatBlk->errorCode = 0;
  rngXBlkPut(pStmId->pTagFreeList,(long*) &tag,1);
  DPRINT4(1,"stmFreeFidBlk: Tag: %d, FID Addr: 0x%lx, size: %ld (0x%lx)\n",
	tag,pStatBlk->fidAddr,pStatBlk->dataSize,pStatBlk->dataSize);
  return( OK );
}

/**************************************************************
*
*  dataTag2DataAddr - Gets Data Address specified by the Tag 
*
*
* RETURNS:
*  16-bit STM Tag AP Register Value
*/
long *dataTag2DataAddr(DATAOBJ_ID pStmId,short tag)
/* pStmId - fifo Object identifier */
/* tag  - Tag index */
{
   FID_STAT_BLOCK *pTmpStatBlk;
   long *statblkAddr;

   if ((pStmId == NULL))
     return((long*)-1);

   /* Get Pointer to FID_STAT_BLOCK referenced via Tag Value */
   if ( (tag >= 0) && (tag < pStmId->maxFidBlkBuffered) )
   {
       pTmpStatBlk = &((pStmId->pStatBlkArray)[tag]);
       statblkAddr = pTmpStatBlk->fidAddr;
   }
   else 
   {
       errLogSysRet(LOGIT,debugInfo,"dataGetStatBlk: Illegal tag value: %ld",tag);
       statblkAddr = NULL;
   }
      
   return( statblkAddr );
}

/**************************************************************
*
*  dataTag2StatBlk - Gets FID Stat Block address corresponding to Tag
*
*
* RETURNS:
*  address of Stat Block corresponding to the Tag argument
*/
FID_STAT_BLOCK *dataTag2StatBlk( DATAOBJ_ID pStmId, short tag )
/* pStmId - fifo Object identifier */
/* tag  - Tag index */
{
   FID_STAT_BLOCK *pTmpStatBlk;

   if (pStmId == NULL) 
     return((FID_STAT_BLOCK *) NULL);

   if ( (tag >= 0) && (tag < pStmId->maxFidBlkBuffered) )
   {
       pTmpStatBlk = &((pStmId->pStatBlkArray)[tag]);
   }
   else 
   {
       errLogSysRet(LOGIT,debugInfo,"dataGetStatBlk: Illegal tag value: %ld",tag);
       pTmpStatBlk  = NULL;
   }
      
   return( pTmpStatBlk );
}

/********************************************************************
* stmShow - display the status information on the STM Object
*
*  This routine display the status information of the STM Object
*
*
*  RETURN
*   VOID
*
*/
VOID stmShow(DATAOBJ_ID pStmId, int level)
/* DATAOBJ_ID pStmId - STM Object ID */
/* int level 	   - level of information */
{
   FID_STAT_BLOCK *pStatBlk; /* array of FID Entries */
   int i;

   if (pStmId == NULL)
     return;

   printf("\n -------------------------------------------------------------\n\n");
   printf("STM Object (0x%lx): %s\n",
           pStmId,pStmId->pIdStr);
 /*   printf("Board Type: '%s'\n",getBoardType(pStmId->stmBrdVersion)); */
   /* printf("SCCS ID: '%s'\n",pStmId->pSID); */

   /* stmPrtStatus(pStmId); */

   /* stmPrtRegs(pStmId); */

   printf("\nTag Free List:\n");
   rngXBlkShow(pStmId->pTagFreeList, 0);

   printf("\nSTM State Semaphore: \n");
   printSemInfo(pStmId->pSemStateChg,"STM State Semaphore",1);
   /* semShow(pStmId->pSemStateChg,level); */

   if (level > 0)
   {
     int foundone = 0;
     printf("\nData Mutex Semaphore: \n");
     semShow(pStmId->pStmMutex,level);

     printf("\nFID Blocks allocated for Acquisition (out of %d possible): \n",pStmId->maxFidBlkBuffered);
     for( i=0; i < pStmId->maxFidBlkBuffered; i++)
     {
       pStatBlk = &((pStmId->pStatBlkArray)[i]);

       if(pStatBlk->doneCode != NOT_ALLOCATED)
       {
         foundone = 1;
         printf("Tag: %d, Elem: %ld, FidAddr: 0x%lx, State: %d \n",
	      i, pStatBlk->elemId,pStatBlk->fidAddr,pStatBlk->doneCode);
         printf("Start CT: %ld, NP: %ld, CT: %ld, FidSize: %ld \n",
	   pStatBlk->startct, pStatBlk->np, pStatBlk->ct, pStatBlk->dataSize);
       }
     }
     if (!foundone)
	printf(" No Blocks allocated.\n");
   }
}


#ifdef TESTING_MODULE
stmPrintStmDataArray(DATAOBJ_ID pStmId)
/* DATAOBJ_ID pStmId - STM Object ID */
{
   FID_STAT_BLOCK *pStatBlk; /* array of FID Entries */
   int i;
   int foundone = 0;

   if (pStmId == NULL)
     return;

   printf("\n -------------------------------------------------------------\n\n");
   printf("STM Object (0x%lx): %s [Board Addr: 0x%lx]\n",pStmId,pStmId->pStmIdStr,pStmId->stmBaseAddr);
   printf("SCCS ID: '%s'\n",pStmId->pSID);

     printf("\nFID Blocks allocated for Acquisition (out of %d possible): \n",pStmId->maxFidBlkBuffered);
     for( i=0; i < pStmId->maxFidBlkBuffered; i++)
     {
       pStatBlk = &((pStmId->pStatBlkArray)[i]);

       if(pStatBlk->doneCode != NOT_ALLOCATED)
       {
         foundone = 1;
         printf("Tag: %d, Elem: %ld, FidAddr: 0x%lx, State: %d \n",
	      i, pStatBlk->elemId,pStatBlk->fidAddr,pStatBlk->doneCode);
         printf("Start CT: %ld, NP: %ld, CT: %ld, FidSize: %ld \n",
	   pStatBlk->startct, pStatBlk->np, pStatBlk->ct, pStatBlk->dataSize);
       }
     }
     if (!foundone)
	printf(" No Blocks allocated.\n");
}

#endif
