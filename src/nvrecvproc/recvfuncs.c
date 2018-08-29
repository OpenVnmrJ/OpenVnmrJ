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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pwd.h>
#include <malloc.h>
#include <netinet/in.h>
#include <utime.h>

#include <errno.h>

#include "errLogLib.h"
#include "fileObj.h"
#include "mfileObj.h"
#include "shrMLib.h"
#include "shrexpinfo.h"
#include "shrstatinfo.h"
#include "hostAcqStructs.h"
#include "expDoneCodes.h"
#include "msgQLib.h"
#include "hostMsgChannels.h"
#include "expQfuncs.h"
#include "procQfuncs.h"
#include "data.h"
#include "dspfuncs.h"
#include "crc32.h"
#ifndef RTI_NDDS_4x
#include "Data_UploadCustom3x.h"
#else /* RTI_NDDS_4x */
#include "Data_Upload.h"
#endif  /* RTI_NDDS_4x */

#ifdef THREADED
   /* |||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| */
#include "barrier.h"
#include "memorybarrier.h"
#include "rngBlkLib.h"
#include "rcvrDesc.h"
#include "recvthrdfuncs.h"
#include "flowCntrlObj.h"
extern pthread_t main_threadId;
extern cntlr_crew_t TheRecvCrew;
extern barrier_t TheBarrier;
extern RCVR_DESC ProcessThread;
extern membarrier_t TheMemBarrier;
extern FlowContrlObj *pTheFlowObj;
   /* |||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| */
#endif

extern long long systemFreeRAM;
extern long long systemTotalRAM;


extern RCVR_DESC_ID ddrSubList[64];
extern int numSubs;
#define IBUF_SIZE       (8*XFR_SIZE)

#define S_OLD_COMPLEX   0x40
#define S_OLD_ACQPAR    0x80

#define SAVE_DATA	0
#define DELETE_DATA	1
#define DATA_ERROR	-1

/*
#define XFR_SIZE        (76)
#define IBUF_SIZE       (8*XFR_SIZE)
*/

extern int chanId;	/* Channel Id */
SHR_EXP_INFO expInfo = NULL;   /* start address of shared Exp. Info Structure */

/* processing thread pipestage object */
extern PIPE_STAGE_ID pProcThreadPipeStage;

static WORKQ_ENTRY_ID pWorkQEntryCmplt = NULL;  /* for use in main thread to finish Experiment */

static SHR_MEM_ID  ShrExpInfo = NULL;  /* Shared Memory Object */

/* Vnmr Data File Header & Block Header */
static struct datafilehead fidfileheader;
static struct datablockhead fidblockheader;

/* dummy struct for now */
typedef struct  {
			long np;
			long ct;
			long bs;
			long elemid;
			long v1;
			long v2;
			long v4;
			long v5[10];
		  } lc;

#ifdef LINUX
static struct recvProcSwapbyte
{
   short s1;
   short s2;
   short s3;
   short s4;
   long  l1;
   long  l2;
   long  l3;
   long  l4;
   long  l5;
};
                                                                                
typedef union
{
   struct datablockhead *in1;
   struct recvProcSwapbyte *out;
} recvProcHeaderUnion;

typedef union
{
   float *fval;
   int   *ival;
} floatInt;                                                                               
#endif

/* char tmp[IBUF_SIZE+1]; */

static MSG_Q_ID pProcMsgQ = NULL;
static MSG_Q_ID pExpMsgQ = NULL;
static MFILE_ID ifile = NULL;
static char expInfoFile[512] = { '\0' };

static int Use_FileIO_Flag = 1;   /* Fid Data: 1 = use regular file I/O,  0 = use MMAP */
static FILE_ID ffile = NULL;

static int dspflag = 0;
static int bbytes = 0;

/* flag to indicate at least one thread has processed a harderror, thus no
 *  other thread need to do processing, i.e. multiple receivers
 */
/* int AbortFlag = 0; */
static int HardErrorFlag = 0;

void UpdateStatus(FID_STAT_BLOCK *);
void rmAcqiFiles(SHR_EXP_INFO);

int sumFloatData( float* dstadr, float* srdadr, unsigned long np);
int sumLongData( long* dstadr, long* srdadr, unsigned long np);
int sumShortData( short* dstadr, short* srdadr, unsigned long np);

/*--------------------------------------------------------------------*/
/*   eos,ct,scan; eob,bs; eof,nt,fid; eoc,il;  # - eos at modulo #    */
/*--------------------------------------------------------------------*/
#define STOP_EOS        11/* sa at end of scan */
#define STOP_EOB        12/* sa at end of bs */
#define STOP_EOF        13/* sa at end of fid */
#define STOP_EOC        14/* sa at end of interleave cycle */
/*--------------------------------------------------------------------*/

/**********************************************************
* calcDCoffset - calculates dc offsets for real and imaginary channel. 
*		calculations taken from apintfunc.c (nschk).
*
*/
static
void calcDCoffset(long *noisedata, int length, double *realdcoffset, 
			double *imagdcoffset, int dp)
{
   register int  nscnt; 
   register int	dispctr;	/* lock display data counter */
   register int val;
   register int	*dispptr;	/* lock display data pointer */
   register short *sdispptr;
   register double rsum,isum;	/* real & imaginary sum      */

	isum = rsum = 0.0;
	nscnt = (int) (length >> 1); /* check 128 points in noise check */
	if (dp == 2)
        {
	    sdispptr = (short *)noisedata;
	    for (dispctr = 0; dispctr < nscnt; dispctr++)
            {
#ifdef LINUX
                val = ntohs( *sdispptr );
                sdispptr++;
                rsum += (double) val;
                val = ntohs( *sdispptr );
                sdispptr++;
                isum += (double) val;
#else
	   	rsum += (double) *sdispptr++;
	   	isum += (double) *sdispptr++;
#endif
            }
        }
	else
        {
	    dispptr = (int *)noisedata;
	    for (dispctr = 0; dispctr < nscnt; dispctr++)
            {
#ifdef LINUX
                val = ntohl( *dispptr );
                dispptr++;
                rsum += (double) val;
                val = ntohl( *dispptr );
                dispptr++;
                isum += (double) val;
#else
	   	rsum += (double) *dispptr++;
	   	isum += (double) *dispptr++;
#endif
            }
        }
        *realdcoffset = rsum / (double) nscnt;
        *imagdcoffset = isum / (double) nscnt;
}

unsigned long maxFidsRecv;



MFILE_WRAPPER IfileWrap,BufFileWrap;

/*
 *  initialize the experiment files and worker threads
 *  called in the context of the main thread
 */
int recvData(char *argstr)
{

  int uid,gid;
  int datastat,stat;
  char fidpath[512];
  char expCmpCmd[256];
  char *pInfoFile;
  unsigned long totalDataSize, tempbufsize,nbufs;
  void resetState();
  RINGBLK_ID pFreeBufIndex = NULL;
  MFILE_ID bufferIfile = NULL;
  PSTMF  sumFunc = NULL;
  MFILE_ID_WRAPPER pIfileWrap,pBufFileWrap;
  SHARED_DATA_ID pSharedData;
  long long maxBufferMemoryUsage;

  int status,i,index;
  cntlr_crew_t *crew;
  membarrier_t *pmembar;
  int ddrThrdIndexPos[64],ddrIndex,ddrNum;
  char *pToken, *pDDRName;
  char recvrs[512];
  long long calcMaxMemory4BuffersMB(int numActiveDDrs);

  crew = &TheRecvCrew;
  pmembar = &TheMemBarrier;


  DPRINT(+1,"\n|||||||||||||||||||||||||||||||||||||||||||||||||||\n\n >>>>>>>>> recvData Invoked.\n\n");

  if ( ! consoleConn() )
  {
     errLogRet(ErrLogOp,debugInfo,
	"recvData: Channel not connected to console yet, Transfer request Ignored.");
     return(-1);
  }


  /* =========================================================================== */
  /* =========================================================================== */
   /* |||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| */
        
  /* at some point these need to be protect via a mutex to be thread safe... */
  pProcMsgQ = openMsgQ("Procproc");
  pExpMsgQ = openMsgQ("Expproc");

/*
  if (pProcMsgQ == NULL)
      return(-1);
*/
     
 /* AbortFlag = 0; */
 clearDiscardIssues();

  pInfoFile = strtok(NULL," ");
  // pInfoFile = "/vnmr/acqqueue/exp1.greg.405830";   4 ddrs
  // pInfoFile = "/vnmr/acqqueue/exp1.greg.998183";   // 1 ddr, np=4096
  strcpy(expInfoFile,pInfoFile);

  DPRINT1(+1,"\n--------------------\n\n ----- Preparing for: '%s' data.\n\n",pInfoFile);
  /*
      1st mapin the Exp Info Dbm 
  */
  if ( mapIn(expInfoFile) == -1)   /* this routine, sets ShrExpInfo & expInfo */
  {
     errLogRet(ErrLogOp,debugInfo,
	"recvData: Exp Info File %s Not Present, Transfer request Ignored.", expInfoFile);
     return(-1);
  }
  // DPRINT5(+1,"Expecting: %ld FIDs, NT = %ld, GOFlag = %d, NP = %ld, Data FileSize: %lld\n",
  DPRINT5(+1,"Expecting: %u FIDs, NT = %u, GOFlag = %d, NP = %u, Data FileSize: %lld\n",
	  expInfo->ArrayDim,expInfo->NumTrans,expInfo->GoFlag,
	  expInfo->NumDataPts,expInfo->DataSize);

  if (expInfo->ExpFlags & VPMODE_BIT)
  {
     char str[1024];
     char addr[1024];
     char node[128];
     procQadd(WERR, ShrExpInfo->MemPath, 0, 0, EXP_STARTED, expInfo->GoFlag |
           expInfo->ExpFlags & RESUME_ACQ_BIT);
     sendMsgQ(pProcMsgQ,"chkQ",strlen("chkQ"),MSGQ_NORMAL,NO_WAIT);
     sscanf(expInfo->VpMsgID,"%[^;]; %s",addr,node);
     sprintf(str,"ipccontst %s\n vpMsg('%s',%d)\n",
                   addr, node, EXP_STARTED);
     sendMsgQ(pExpMsgQ,str,strlen(str),MSGQ_NORMAL,NO_WAIT);
  }
  if(expInfo->NFmod>expInfo->NumFids)
      expInfo->NFmod=expInfo->NumFids;
  if(expInfo->NFmod<1)
      expInfo->NFmod=1;
  
  DPRINT3(+1,"               NF:%u NFMOD:%d IL:%d\n",
      expInfo->NumFids,expInfo->NFmod,expInfo->IlFlag);

  /*
      2nd open up the Data file to the proper size and changed the
      file owner & group to the Experiment submitter.
  */
  DPRINT1(+1,"recvData:  uplink data into: '%s'\n",expInfo->DataFile);
  DPRINT1(+1,"recvData;  expInfo->GoFlag = %d\n",expInfo->GoFlag);
  if (! expInfo->GoFlag)
  {
     sprintf(fidpath,"%s/fid",expInfo->DataFile);
     DPRINT1(1,"getExpData: uplink data into: '%s'\n",fidpath);

     /* Forget Malloc and Read, we'll use the POWER of MMAP */
     if ( !(expInfo->ExpFlags & RESUME_ACQ_BIT))
     {
       /* open new data file */
       if (Use_FileIO_Flag == 0)
       {
          ifile = mOpen(fidpath,expInfo->DataSize,O_RDWR | O_CREAT | O_TRUNC);
          ffile = NULL;
       }
       else
       {
          ffile = fFileOpen(fidpath,expInfo->DataSize,O_RDWR | O_CREAT | O_TRUNC);
          ifile = NULL;
          DPRINT1(1,"ffile: 0x%lx\n",(unsigned long) ffile);
          /* DPRINT2(-5,"fOpen(): fd = %d, filepath: '%s' \n", ffile->fd,ffile->filePath); */
       }
     }
     else
     { 
       /* open existing data file SA/RA */
       if (Use_FileIO_Flag == 0)
       {
          ifile = mOpen(fidpath,expInfo->DataSize,O_RDWR | O_CREAT);
       }
       else
       {
          ffile = fFileOpen(fidpath,expInfo->DataSize,O_RDWR | O_CREAT);
       }
     }

     /* if (ifile == NULL) */
     if ( ((Use_FileIO_Flag == 0) && (ifile == NULL)) || ((Use_FileIO_Flag == 1) && (ffile == NULL)) )
     {
       errLogSysRet(ErrLogOp,debugInfo,"recvData: could not open %s",fidpath);
       resetState();
       return(-1);
     }
     if (Use_FileIO_Flag == 0)
     {
       DPRINT1(+1,"recvData;  ifile: 0x%lx\n",(unsigned long) ifile);
       DPRINT1(+1,">>>>> mmap adder for datafile: 0x%lx\n",(unsigned long) ifile->mapStrtAddr);
     }
     else
     {
        DPRINT1(+1,"recvData; fileObj  ffile: 0x%lx\n",(unsigned long) ffile);
     }

     /* get the UID & GID of exp. owner */
     if ( getUserUid(expInfo->UserName,&uid,&gid) == 0) 
     {
        int old_euid = geteuid();

        seteuid( getuid() );
  	chown(fidpath, uid, gid);
        seteuid( old_euid );
     }
     else
     {
	errLogRet(ErrLogOp,debugInfo,
	   "recvData: Unable to change '%s' uid for User: '%s'\n",
		fidpath,expInfo->UserName);
     }

     /* set the access permission as directed by PSG */
     chmod( fidpath, 0666 & ~(expInfo->UserUmask) );


     /* if Interleave (IL) or RA then will need a buffer file to put the reieved data
      * since it will have to be added to the fid data file
      */
     DPRINT2(2,"-------> recvData: IlFlag:  %d, RAFlag: %d\n",expInfo->IlFlag,expInfo->RAFlag);
     DPRINT1(2,"-------> recvData: DataPtSize:  %d\n",expInfo->DataPtSize);
     if ( (expInfo->IlFlag != 0) || (expInfo->RAFlag != 0) )
     {
       if (expInfo->DataPtSize == 4)
       {
          sumFunc = (PSTMF) sumFloatData;    /* sumLongData */
       } 
       else
       {
          sumFunc = (PSTMF) sumShortData;
       }
       DPRINT3(2,"sumFunc: 0x%lx, sumFloat: 0x%lx, sumShort: 0x%lx\n", sumFunc, sumFloatData, sumShortData);
     }


     /* Check for In-Line DSP */
     /* never an option in Nirvana */
     dspflag = 0;

     /* ----------------------------------------------------------- */

     DPRINT(+1,"recvData: check RESUME bit\n");
     if ( !(expInfo->ExpFlags & RESUME_ACQ_BIT)) 
     {
       DPRINT(1,"recvData: RESUME bit NOT set, InitialFileHeaders() called\n");
       InitialFileHeaders(); /* set the default values for the file & block headers */

       if (Use_FileIO_Flag == 0)
       {
           /* write fileheader to datafile (via mmap) */
           memcpy((void*) ifile->offsetAddr,
		    (void*) &fidfileheader,sizeof(fidfileheader));
           ifile->offsetAddr += sizeof(fidfileheader);  /* move my file pointers */
           ifile->newByteLen += sizeof(fidfileheader);
       }
       else
       {
           fFileWrite(ffile,(char*) &fidfileheader, sizeof(fidfileheader), (off_t) 0LL);
       }
     }
     else
     {
        /* Hey if file is less than fileheader + blockheader size then forget it */
         if (ifile->byteLen < (sizeof(fidfileheader) + sizeof(fidblockheader)))
         {
            errLogRet(ErrLogOp,debugInfo,
	      "recvData: Existing File Not Large enough for RA usage: %lu bytes\n",
		ifile->byteLen);
            resetState();
            return(-1);
         }

         if (Use_FileIO_Flag == 0)
         {
            /* Set New size to it's existing size, Sure wouldn't what it to get smaller! */
            ifile->newByteLen = ifile->byteLen;

            /* read in file header */
            memcpy((void*) &fidfileheader, (void*) ifile->offsetAddr,sizeof(fidfileheader));

	    /* Update Relevant Info */
            fidfileheader.nblocks   = expInfo->ArrayDim; /* n fids*/

            /* write fileheader to datafile (via mmap) */
            memcpy((void*) ifile->offsetAddr,
		   (void*) &fidfileheader,sizeof(fidfileheader));

	    /* move file pointer */
            ifile->offsetAddr += sizeof(fidfileheader);  

            /* read in block header */
            memcpy((void*) &fidblockheader, 
		   (void*) ifile->offsetAddr,sizeof(fidblockheader));
            ifile->offsetAddr += sizeof(fidblockheader);  /* move my file pointers */
         }
         else
         {
             /* read in file header */
             fFileRead(ffile, (char*) &fidfileheader, sizeof(fidfileheader), (long long) 0LL);

	     /* Update Relevant Info */
             fidfileheader.nblocks   = expInfo->ArrayDim; /* n fids*/

             /* write fileheader to datafile (via mmap) */
             fFileWrite(ffile, (char*) &fidfileheader, sizeof(fidfileheader), (long long) 0LL);

             /* read in block header */
             fFileRead(ffile, (char*) &fidblockheader, sizeof(fidblockheader), (long long) sizeof(fidfileheader));
         }
     }

  }
  else
  {
     ifile = NULL;
     ffile = NULL;
  }

   maxFidsRecv = 0L;
   HardErrorFlag = 0;

    /* we must determine which DDR are active for this Experiment and set there cmd
     *  appropriately
    */
    DPRINT1(+2," --->> RcvrMapping: '%s'\n",expInfo->RvcrMapping);

    /* strcpy(recvrs,"ddr1"); */
    strcpy(recvrs,expInfo->RvcrMapping);
    /* scan through recv string, 'ddr1,ddr2,ddr3','1,2,3'
     * the ordinal # represents the receiver and it's position
     * within the string determines order in FID file 
     */
    ddrNum = 0;
    pToken = recvrs;
    while ( (pDDRName = (char*) strtok_r(pToken," ,",&pToken)) != NULL)
    {
       /* DPRINT1(-1,"recData: token: '%s'\n",pDDRName); */
       /* ddrThrdIndexPos[ddrNum++] = findCntlr(&TheRecvCrew, pDDRName ); */
       ddrThrdIndexPos[ddrNum++] = findCntlr(pDDRName );
       DPRINT2(+1,"recData: token: '%s', thread Index: %d\n",pDDRName,ddrThrdIndexPos[ddrNum-1]);
    }

    /* Warning & Note, not all receivers (DDRs) maybe used in an experiment so
     *  the barrier must be modifable in this respect 
     */

    DPRINT1(+1,"recData: Set Barrier to Number of Active DDRs: %d\n",ddrNum);
    barrierSetCount(&TheBarrier,ddrNum); /* set number of active ddrs in the barrier membership */

    ProcessThread.pParam = expInfo; /* for processing thread pipestage */

   /* |||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| */
    /* prepare the thread for work */
   /* |||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| */
    
    pIfileWrap = &IfileWrap;
    pBufFileWrap = &BufFileWrap;

    pSharedData = (SHARED_DATA_ID) lockSharedData(pmembar);
    if (pSharedData == NULL)
       errLogSysQuit(LOGOPT,debugInfo,"recvData: Could not lock memory barrier mutex");

       /* pSharedData->pMapBufFile = bufferIfile; always buffering now */
       pSharedData->pMapFidFile = ifile;
       pSharedData->pFidFile = ffile;
       pSharedData->AbortFlag = 0;
       pSharedData->discardIssues = 0;

    unlockSharedData(pmembar);

    /* all recvFid() thread non-active */
    resetCntlrStatus();
    
    pBufFileWrap->pMapFile = NULL;
    pIfileWrap->pMapFile = ifile;
    pIfileWrap->pFile = ffile;

    resetFlowCntrl(pTheFlowObj);

    /* first free all the workQ data bufs, thus when realloc the memory usage doesn't climb */
    for( i = 0; i < numSubs; i++)
    {
         workQFreeDataBufs(ddrSubList[i]->pWorkQObj);
    }

    /* calc Total RAM to be allowed to used based on Ram of system and number of Active DDRs */
    maxBufferMemoryUsage = calcMaxMemory4BuffersMB(ddrNum);
    DPRINT1(+1,"-- Calc'd maxBufferMemoryUsage: %llu\n",maxBufferMemoryUsage);
     
#ifdef TESTING_ROUTINES
*    /* long long requiredMemory(WORKQ_ID pWorkQ, int numActiveDDRs, int qSize, long fidSizeBytes, long nf) */
*    workQrequiredMemory(ddrSubList[0]->pWorkQObj, ddrNum, 1, expInfo->FidSize,expInfo->NumFids/expInfo->NFmod);
*    workQrequiredMemory(ddrSubList[0]->pWorkQObj, ddrNum, 3, expInfo->FidSize,expInfo->NumFids/expInfo->NFmod);
*    workQrequiredMemory(ddrSubList[0]->pWorkQObj, ddrNum, 6, expInfo->FidSize,expInfo->NumFids/expInfo->NFmod);
*    workQrequiredMemory(ddrSubList[0]->pWorkQObj, ddrNum, 20, expInfo->FidSize,expInfo->NumFids/expInfo->NFmod);
*    workQrequiredMemory(ddrSubList[0]->pWorkQObj, ddrNum, 40, expInfo->FidSize,expInfo->NumFids/expInfo->NFmod);
*
*
*    /* int maxQueueSize(WORKQ_ID pWorkQ, int numActiveDDRs, long long memSize, long fidSizeBytes, long nf) */
*    maxWorkQSize(ddrSubList[0]->pWorkQObj, ddrNum,  systemFreeRAM, expInfo->FidSize,expInfo->NumFids/expInfo->NFmod);
*    maxWorkQSize(ddrSubList[0]->pWorkQObj, ddrNum,  systemFreeRAM/2LL, expInfo->FidSize,expInfo->NumFids/expInfo->NFmod);
*    maxWorkQSize(ddrSubList[0]->pWorkQObj, ddrNum,  systemFreeRAM/3LL, expInfo->FidSize,expInfo->NumFids/expInfo->NFmod);
*    maxWorkQSize(ddrSubList[0]->pWorkQObj, ddrNum,  systemFreeRAM/4LL, expInfo->FidSize,expInfo->NumFids/expInfo->NFmod);
#endif   /* TESTING_ROUTINES */

    for( i = 0; i < numSubs; i++)
    {
       extern int getCntlrNum(char *id);
       int result;
       Data_Upload *issue;
       WORKQ_ENTRY_ID wrkQEntry;
       int ddrPosition;
       int transferLimit;

       ddrPosition = -1;  /* -1 == not used */

         for (ddrIndex=0; ddrIndex < ddrNum; ddrIndex++)
         {
           if ( i == ddrThrdIndexPos[ddrIndex] )
              {
                 ddrPosition = ddrIndex;
                 break;
              }
          }
         ddrSubList[i]->pParam = expInfo;   /* &ShrExpInfo; */
         ddrSubList[i]->activeWrkQEntry = NULL; 
         ddrSubList[i]->prevElemId = 0L; 
         setMaxWorkQMemoryUsage(ddrSubList[i]->pWorkQObj, maxBufferMemoryUsage);
         workQDataBufsInit(ddrSubList[i]->pWorkQObj, pIfileWrap,expInfo->FidSize,expInfo->NumFids/expInfo->NFmod, ddrPosition,ddrNum,sumFunc);
         transferLimit = numAvailWorkQs(ddrSubList[i]->pWorkQObj) - 1;
         if (ddrPosition != -1)  /* only put DDR that are being used into flow controle object */
             initFlowCntrl(pTheFlowObj, getCntlrNum(ddrSubList[i]->cntlrId), ddrSubList[i]->cntlrId, ddrSubList[i]->PubId, transferLimit, transferLimit/2);

         initBlockHeader(ddrSubList[i]->pFidBlockHeader,expInfo);
         wrkQEntry = workQGet(ddrSubList[i]->pWorkQObj);
         ddrSubList[i]->activeWrkQEntry = wrkQEntry; 
         issue = ddrSubList[i]->SubId->instance;
#ifndef RTI_NDDS_4x
         issue->data.val = (char*) wrkQEntry->pFidStatBlk;    /* point NDDS to fid stat block 1st */
#else /* RTI_NDDS_4x */
         /* must confirm this step gmb 8/13/07 */
         DDS_OctetSeq_set_length(&issue->data,0); 
#endif  /* RTI_NDDS_4x */
         DPRINT3(+1,"'%s': %d - Preparing '%s' WorkDesc for Experiment\n",ddrSubList[i]->cntlrId,i,ddrSubList[i]->cntlrId);
         if (ddrSubList[i]->p4Diagnostics == NULL)
         {
             ddrSubList[i]->p4Diagnostics = (DIAG_DATA_ID) malloc(sizeof(DIAG_DATA));
             gettimeofday(&(ddrSubList[i]->p4Diagnostics->tp),NULL);
         }
         ddrSubList[i]->p4Diagnostics->pipeHighWaterMark = 0;
         ddrSubList[i]->p4Diagnostics->workQLowWaterMark = ddrSubList[i]->pWorkQObj->numWorkQs;
         DPRINT2(3,"'%s': recvData: Send C_RECVPROC_READY to '%s'\n",ddrSubList[i]->cntlrId, ddrSubList[i]->cntlrId);
         DPRINT3(3,"'%s': recvData: ddrPos: %d, transferLimit: %d\n",ddrSubList[i]->cntlrId,ddrPosition,transferLimit);
         result = send2DDR(ddrSubList[i]->PubId, C_RECVPROC_READY, ddrPosition, transferLimit, 0);
         if (result == -1)
         {
           errLogRet(LOGOPT,debugInfo,"send2AllDDRs: send2DDR to '%s' failed\n", ddrSubList[i]->PubId->topicName);
         }
    }

   /* |||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| */
   DPRINT(+1,"\n >>>>>>> recvData returned. \n\n|||||||||||||||||||||||||||||||||||||||||||||||||||\n\n");
   return(0);
}



   /* |||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| */
   /* |||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| */
   /* |||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| */



/*
 * Finish up Experiment and Send Expproc expdone message
 * Invoked by the last thread to complete all FID Transfers
 * however executed in the context of the main thread, to help avoid race conditions
 *  as of 3/27/2006, was called by different thread  GMB
 *
 */
/* int recvFidsCmplt(PIPE_STAGE_ID pPipeStage,WORKQ_ENTRY_ID pWorkQEntry) */
/* int recvFidsCmplt(WORKQ_ENTRY_ID pWorkQEntry) */
int recvFidsCmplt(void)
{
  int datastat,stat,i;
  char expCmpCmd[256];
  FID_STAT_BLOCK *pStatBlk;
  char *CntlrName;
  cntlr_crew_t *crew;
  RCVR_DESC_ID pRcvrDesc;
  WORKQINVARIENT_ID pWrkqInvar;
  SHR_EXP_INFO pExpInfo = NULL;   /* start address of shared Exp. Info Structure */
  SHARED_DATA_ID pSharedData;
  membarrier_t *pmembar;
  WORKQ_ENTRY_ID pWorkQEntry;
  char fidpath[512];


  pWorkQEntry = pWorkQEntryCmplt;
  if (pWorkQEntry == NULL)
  {
      errLogRet(ErrLogOp,debugInfo, 
      "recvFidsCmplt: pWorkQEntry s NULL for finishing Experiment, can not finish Experiment, fatal error\n");
       return(-1);
  }
  pStatBlk = pWorkQEntry->pFidStatBlk;
  pRcvrDesc = (RCVR_DESC_ID) pWorkQEntry->pInvar->pRcvrDesc;

  pmembar = &TheMemBarrier;

  CntlrName = pRcvrDesc->cntlrId;
  pExpInfo = (SHR_EXP_INFO) pRcvrDesc->pParam;
  /* pWorkQEntry = pRcvrDesc->activeWrkQEntry; */

  DPRINT1(+1,"'%s': recvFidsCmplt()\n",CntlrName);

  /* processDoneCode(pExpInfo,pStatBlk); */

  /*** LOCK Shared DATA *************************************** */
  pSharedData = (SHARED_DATA_ID) lockSharedData(pmembar);
  if (pSharedData == NULL)
       errLogSysQuit(LOGOPT,debugInfo,"recvData: Could not lock memory barrier mutex");
  /* vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv*/

     /* if (pWorkQEntry->pInvar->fiddatafile->pMapFile != NULL) */
     if ( ((Use_FileIO_Flag == 0) && (pWorkQEntry->pInvar->fiddatafile->pMapFile != NULL)) )
     {
   
        DPRINT3(+1,"'%s': recvFidsCmplt: ifile: opened file size: %llu, new file size: %llu\n", 
           CntlrName,pWorkQEntry->pInvar->fiddatafile->pMapFile->byteLen, 
           pWorkQEntry->pInvar->fiddatafile->pMapFile->newByteLen);
        mClose(pWorkQEntry->pInvar->fiddatafile->pMapFile);
        pSharedData->pMapFidFile = NULL;
        pWorkQEntry->pInvar->fiddatafile->pMapFile = NULL;
        sprintf(fidpath,"%s/fid",pExpInfo->DataFile);
        utime(fidpath,NULL);  /* update timestamp of fid to present time, bug# 3783 */
     }
     else if ((Use_FileIO_Flag == 1) && (pWorkQEntry->pInvar->fiddatafile->pFile != NULL))
     {
        fFileClose(pWorkQEntry->pInvar->fiddatafile->pFile);
        pSharedData->pFidFile = NULL;
        pWorkQEntry->pInvar->fiddatafile->pFile = NULL;
        sprintf(fidpath,"%s/fid",pExpInfo->DataFile);
        utime(fidpath,NULL);  /* update timestamp of fid to present time, bug# 3783 */
     }

 /* ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^*/
 unlockSharedData(pmembar);  /* true some of the above are NOT included in the shared data struct 
                               but as a hack, these are checked by other threads, and
			       this is only done at the exp end. Thus blocking the all the threads
                               for this long will be OK. (I hope)   GMB */

 /***************************************************************************************/

  DPRINT(+1,"recvFidsCmplt: Exp Completion Received,  Send C_RECVPROC_DONE to DDR\n");
  send2AllDDRs(C_RECVPROC_DONE, 0, 0, 0); /* tell DDR(s) recvproc done for now */

  stat = wait4Send2Cmplt(30 /* sec */, 0 /* qlevel */);
  if ( stat == -1 )
     errLogRet(ErrLogOp,debugInfo,"wait4Send2Cmplt: timed out\n");

#define INSTRUMENT
#ifdef INSTRUMENT
    DPRINT(+1,"vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv\n");
    for( i = 0; i < numSubs; i++)
    {
       float used,free,total,bufused,NDDSbkup,recvFidbkup;

       used = (float) rngBlkNElem (ddrSubList[i]->pInputQ);
       free = (float) rngBlkFreeElem (ddrSubList[i]->pInputQ);
       total = used + free;
       bufused = (float) (ddrSubList[i]->pWorkQObj->numWorkQs - ddrSubList[i]->p4Diagnostics->workQLowWaterMark);
       NDDSbkup = ((bufused / ddrSubList[i]->pWorkQObj->numWorkQs) * 100.0);
       recvFidbkup = ((ddrSubList[i]->p4Diagnostics->pipeHighWaterMark / total) * 100.0);
       if ( NDDSbkup >= 90.0 )
       {
         /* workQFreeDataBufs(ddrSubList[i]->pWorkQObj); */
         DPRINT4(+4,"'%s': NDDS Thread(): BufferQ LowWaterMark: %4d,  %4.1f%% backup, MaxEntries: %4d\n",
               ddrSubList[i]->cntlrId,
               ddrSubList[i]->p4Diagnostics->workQLowWaterMark,
               NDDSbkup, ddrSubList[i]->pWorkQObj->numWorkQs);
       }
       if ( recvFidbkup >= 90.0 )
       {
         DPRINT4(+4,"'%s':    recvFid():  InputQ HighWaterMark: %4d,  %4.1f%% backup, MaxEntries: %4d\n",
               ddrSubList[i]->cntlrId, ddrSubList[i]->p4Diagnostics->pipeHighWaterMark,
               recvFidbkup, (int) total);
       }
    }
    DPRINT(+1,"^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n");
    if (DebugLevel > 0)
       ReportFlow(pTheFlowObj);
    DPRINT(+1,"^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n");
#endif

  processDoneCode(pExpInfo,pStatBlk);
  if (expInfo->ExpFlags & VPMODE_BIT)
  {
     char str[1024];
     char addr[1024];
     char node[128];
     sscanf(expInfo->VpMsgID,"%[^;]; %s",addr,node);
     sprintf(str,"ipccontst %s\n vpMsg('%s',%d)\n",
                   addr, node, pStatBlk->doneCode & 0xFFFF);
     sendMsgQ(pExpMsgQ,str,strlen(str),MSGQ_NORMAL,NO_WAIT);
  }

  /* return the workQEntry to the free list */
  workQFree(pWorkQEntry->pWorkQObj, pWorkQEntry);

  setStatExpName(""); 
  setStatGoFlag(-1); 

  closeMsgQ(pProcMsgQ);
  pProcMsgQ = NULL;

  /* Tell Expproc that for Recvproc is done  With Experiment Acquiring Data */
  /* Expproc must get this message along with the SYSTEM_READY from master to start another Exp */
  sprintf(expCmpCmd,"expdone %s",ShrExpInfo->MemPath);

  activeExpQdelete(expInfoFile);   /* expInfoFile =  filename string */

  if ( mapOut(expInfoFile) == -1)
  {
     errLogRet(ErrLogOp,debugInfo,"getExpData: mapOut failed\n");
     unlockSharedData(pmembar);
     return(-1);
  }
  expInfoFile[0] = '\0';

  DPRINT1(+1,"recvFidsCmplt: Send Expproc expdone msg: '%s'\n",expCmpCmd);
  if ((stat = sendMsgQ(pExpMsgQ,expCmpCmd,strlen(expCmpCmd),MSGQ_NORMAL,
				WAIT_FOREVER)) != 0)
  {
     errLogRet(ErrLogOp,debugInfo, 
      "recvFidsCmplt: Expproc is not running. Exp. Done not received: '%s'\n",
            ShrExpInfo->MemPath);
  }

  closeMsgQ(pExpMsgQ);
  pExpMsgQ = NULL;
  return(0);
}   


int ExpCmplted(WORKQ_ENTRY_ID pWorkQEntry)
{
  int result;
  void *dumaddr;
  dumaddr = lockSharedData(&TheMemBarrier);
  if (dumaddr == NULL)
       errLogSysQuit(LOGOPT,debugInfo,"ExpCmplted: Could not lock memory barrier mutex");

    if ( (pExpMsgQ == NULL) ||  (expInfo == NULL)) 
       /* (pWorkQEntry->pInvar->fiddatafile->pMapFile == NULL) ) true if su */
       result = 1;
    else
       result = 0;

   unlockSharedData(&TheMemBarrier);
   return result;
}

   /* |||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| */
   /* |||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| */

/*   each receiver has a separate thread inwhich recvFid() is executed
 *   If there are 4 receivers then there are 4 threads each executing
 *   revcFid() for that receivers data.
 *   Fids are queue into the appropriate threads queue by the 
 *   NDDS thread that receives all Data.
 * Each recvFid() is bounded by a barrierwait() such that each thread
 * is sync on a per elemId/FId boundry . This is done since only one thread
 * needs to invoke processing for that FID (e.g. wfid,wexp) to stay consistent
 * withthe previous multi-receivers processes stratagy (Inova)
 */ 

int recvFid(WORKQ_ENTRY_ID pWorkQEntry)
{
    tcrc calcCRC;
    FID_STAT_BLOCK *pStatBlk;
    short status;
    long fidNum,fidSizeBytes;
    char *dataPtr, *fidblkhdrSpot, *dataSrcPtr;
    char *CntlrName;
    int doProcFlag;
    RCVR_DESC_ID pRcvrDesc;
    WORKQINVARIENT_ID pWrkqInvar;
    SHR_EXP_INFO pExpInfo = NULL;   /* start address of shared Exp. Info Structure */
    struct datablockhead *pBlockHeader = NULL;
    SHARED_DATA_ID pSharedData;
    int barrierStatus;    /* barrier can return 0, -1, or -99 for aborted barrierwait */
    extern int getCntlrNum(char *id);

    pRcvrDesc = (RCVR_DESC_ID) pWorkQEntry->pInvar->pRcvrDesc;

    CntlrName = pRcvrDesc->cntlrId;
    pExpInfo = (SHR_EXP_INFO) pRcvrDesc->pParam;
    pBlockHeader = (struct datablockhead *) pRcvrDesc->pFidBlockHeader;

    pSharedData = (SHARED_DATA_ID) lockSharedData(&TheMemBarrier);
    if (pSharedData == NULL)
       errLogSysQuit(LOGOPT,debugInfo,"recvData: Could not lock memory barrier mutex");

    if (pSharedData->AbortFlag == 1)
    {
        DPRINT(+1,"\n\n++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
         rngBlkFlush(pRcvrDesc->pInputQ);
        DPRINT1(+1,"'%s': recvFid:  Error StatBlock AbortFlag set., flush inputQ  and return.\n",
                                CntlrName);
        DPRINT(+1,"++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n\n");
         unlockSharedData(&TheMemBarrier);
         return -1;
    }

    unlockSharedData(&TheMemBarrier);



#define WRNSTATBLK 2
#define FIDSTATBLK 4
#define SU_STOPSTATBLK 8

   /* if an error or warning then go on to processing the statBlk */
   /* ---------------------------------------------------------------------------------- */
    if (pWorkQEntry->statBlkType == WRNSTATBLK)
    {
        DPRINT(+1,"\n\n++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
        DPRINT1(+1,"'%s': recvFid:  Warning StatBlock, skip this routine and pass it on\n",
                                CntlrName);
        DPRINT(+1,"++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n\n");
        return(0);
    }
    else if (pWorkQEntry->statBlkType == SU_STOPSTATBLK)
    {
        DPRINT(+1,"\n\n++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
        DPRINT1(+1,"'%s': recvFid:  SETUP CMPLT StatBlock, wait on barrier \n",
                                CntlrName);
        DPRINT(+1,"++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n\n");
        barrierStatus = barrierWait( &TheBarrier );
        DPRINT2(+1,"'%s': barrierWat returned: %d\n",CntlrName,barrierStatus);
        if (barrierStatus  == -1)
        {
        /* only the last thread does processing */
        /* we are the one to invoke processing */
        /* And only for the last trace for this FID do we invoke processing */
           DPRINT1(+1,"'%s': recvFid: Return from Barrrier, SEND SETUP CMPLT \n",CntlrName);
           DPRINT(+1,"++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n\n");
          return(1);
        }
        else if (barrierStatus == -99)   /* aborted */
        {
            rngBlkFlush(pRcvrDesc->pInputQ);
            DPRINT1(+1,"'%s': recvFid: Returned from Aborted Barrier\n",CntlrName);
            return -1;
        }
        
        DPRINT1(+1,"'%s': recvFid: Returned from Barrier\n",CntlrName);
        /* return(0); */
        DPRINT(+1,"++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n\n");
        return(-1);
    }
   /* ---------------------------------------------------------------------------------- */


    pStatBlk = pWorkQEntry->pFidStatBlk;
 
    /* -------------- Check FIdStatBlock from Console CRC ----------------------------- */
    DPRINT(+1,"\n\n++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
    /* for linux systems this will not work, since the statblock has been byte swapped the
       calc CRC will not be the same as calc by the console on the un byte swap statblock
     */
#ifndef LINUX
    calcCRC = addbfcrc((char*)pWorkQEntry->pFidStatBlk,sizeof(FID_STAT_BLOCK));
    DPRINT4(+1,"'%s': recvFid:  FidStatBlock: 0x%lx, given CRC: 0x%lx, calc CRC: 0x%lx\n",
      CntlrName,pWorkQEntry->pFidStatBlk, pWorkQEntry->statBlkCRC, calcCRC);
 
    if ( pWorkQEntry->statBlkCRC != calcCRC)
    {
        errLogRet(ErrLogOp,debugInfo, "\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
        errLogRet(ErrLogOp,debugInfo,
                  "recvFid: StatBlk CRC Checksums do NOT match: Given: 0x%lx != Calc: 0x%lx\n",
                     pWorkQEntry->statBlkCRC,calcCRC);
        errLogRet(ErrLogOp,debugInfo, "\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
    }
#else
    DPRINT2(+1,"'%s': recvFid:  FidStatBlock: 0x%lx\n",CntlrName,pWorkQEntry->pFidStatBlk);
#endif
   /* ---------------------------------------------------------------------------------- */

       
    DPRINT7(1,"'%s': recvFid:  elemId: %ld, DataSize: %ld, CT: %ld, NP: %ld, DoneCode: 0x%x, %d\n",
                CntlrName,pStatBlk->elemId, pStatBlk->dataSize, pStatBlk->ct,
                pStatBlk->np, pStatBlk->doneCode,(pStatBlk->doneCode & 0xFFFF));
 
    if (pStatBlk->doneCode == EXP_FIDZERO_CMPLT)
    {
         DPRINT1(+1,"'%s': recvFid:  doneCode: EXP_FIDZERO_CMPLT, switch to EXP_FID_CMPLT\n",CntlrName);
         pStatBlk->doneCode = EXP_FID_CMPLT;
    }

    if (pWorkQEntry->dataCRC)   /* if zero then don't bother with CRC check */
    	calcCRC = addbfcrc((char*)pWorkQEntry->pFidData,pStatBlk->dataSize);
    else
        calcCRC = pWorkQEntry->dataCRC;

    /* set pointer to FID data on Disk  (MMAP'd) */
    if (Use_FileIO_Flag == 0)
    {
      dataPtr = getWorkQFidPtr(pWorkQEntry->pWorkQObj,pWorkQEntry);
      DPRINT4(1,"'%s': dataPtr: 0x%lx, buffer: 0x%lx, size bytes: %lu\n", CntlrName, dataPtr, pWorkQEntry->pFidData, pStatBlk->dataSize);
    }
    else
    {
       /* no need for File I/O */
       DPRINT4(1,"'%s': buffer: 0x%lx, size bytes: %lu\n", CntlrName, dataPtr, pWorkQEntry->pFidData, pStatBlk->dataSize);
    }

    DPRINT3(1,"'%s': recvFid:  elemId: %ld, trueElemId: %lu \n", CntlrName,pStatBlk->elemId, pWorkQEntry->trueElemId);

    DPRINT4(+1,"'%s': recvFid:  FidAddr: 0x%lx, given CRC: 0x%lx, calc CRC: 0x%lx\n",
      CntlrName,dataPtr, pWorkQEntry->dataCRC, calcCRC);

    /* check the console to Recvproc transfer CRC */
    if ( pWorkQEntry->dataCRC != calcCRC)
    {
        /* errLogRet(ErrLogOp,debugInfo, "\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n"); */
        errLogRet(ErrLogOp,debugInfo,
                  "recvFid: FID CRC Checksums do NOT match: Given: 0x%lx != Calc: 0x%lx\n",pWorkQEntry->dataCRC,calcCRC);
        /* errLogRet(ErrLogOp,debugInfo, "\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n"); */
    }


   /* |||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| */


   // if ((pStatBlk->doneCode != EXP_FIDZERO_CMPLT) && (pStatBlk->dataSize != 0))
   // {

    /* if pStmFunc == NULL, then the DDRs are doing the summing, just copy the data verbatem */
    if (pWorkQEntry->pInvar->pStmFunc == NULL)
    {
        /* prtMFileMap(pSharedData->pMapFidFile); coring if use 4 MB as MAXMAP in mfileObj.c */
        /* DPRINT4(-5,"revFid() cpyFID -  src: 0x%lx, dest: 0x%lx, bytes: %lu, last addr: 0x%lx\n", 
         * pWorkQEntry->pFidData,dataPtr,pStatBlk->dataSize, dataPtr + pStatBlk->dataSize); */
          if (Use_FileIO_Flag == 0)
          {
              memcpy(dataPtr,pWorkQEntry->pFidData,pStatBlk->dataSize);
          }
          else
          {
              writeWorkQFid2File(pWorkQEntry->pWorkQObj,pWorkQEntry,pStatBlk->dataSize);
          }
    }
    else /* if (pWorkQEntry->pInvar->pStmFunc != NULL) */
    {
       int Ra_Ct_Zero, First_Bs, Il_First_Bs;

       /* if RA & CT == 0 then we want to memcpy not add */
       Ra_Ct_Zero = ((pExpInfo->RAFlag) && (pExpInfo->CurrentTran == 0));

       /* If BS==0 then memcpy (there are no BS) otherwise 
        * if (ct/bs) <= 1 then memcpy (i.e. 1st BS) */

       DPRINT3(+2,"'%s': NumInBS: %ld, ct: %ld\n",CntlrName, pExpInfo->NumInBS,pStatBlk->ct);
       First_Bs =  (pExpInfo->NumInBS >= 1) ? (((long)pStatBlk->ct <= pExpInfo->NumInBS)) : 1;

       /* if Interleave and elements 1st BS then memcpy not add */

       Il_First_Bs = (expInfo->IlFlag != 0) && (pExpInfo->RAFlag == 0) && First_Bs;

       DPRINT4(+1,"'%s': Ra_Ct_Zero: %d, First_Bs: %d, Il_First_Bs: %d\n",
		CntlrName,Ra_Ct_Zero,First_Bs,Il_First_Bs);

      if ( Ra_Ct_Zero || Il_First_Bs )
      {
        /* 1st Block size don't add just copy into file */
        DPRINT4(+2,"'%s': IL BS 1: memcpy(0x%lx,0x%lx,%lu)\n",CntlrName,dataPtr, pWorkQEntry->pFidData, pStatBlk->dataSize);

        if (Use_FileIO_Flag == 0)
           memcpy(dataPtr,pWorkQEntry->pFidData,pStatBlk->dataSize);
        else
           writeWorkQFid2File(pWorkQEntry->pWorkQObj,pWorkQEntry,pStatBlk->dataSize);

      }
      else
      {
          int overflow,bytes;
          // using stat block np was NOT right for imaging with nfmod  e.g. np=256 with nfmod=128 np really equals 32768, not 256
          // unsigned long np = pStatBlk->np * expInfo->NFmod;  another pssible way of calc np
          // opted for the nfmod unaware model, datalength / bytes in data point, e.g.  131072 / 4 = 32768 for np
          unsigned long np = pStatBlk->dataSize / expInfo->DataPtSize;
          if (Use_FileIO_Flag == 0)
          {
             DPRINT4(+2,"'%s': IL Summing: pStmFunc(0x%lx,0x%lx,%lu)\n",CntlrName, dataPtr, pWorkQEntry->pFidData, pStatBlk->np);
       	     // overflow = (pWorkQEntry->pInvar->pStmFunc)((void*) dataPtr, (void*) pWorkQEntry->pFidData, pStatBlk->np );
       	     overflow = (pWorkQEntry->pInvar->pStmFunc)((void*) dataPtr, (void*) pWorkQEntry->pFidData, np );
          }
          else
          {
             DPRINT2(+2,"'%s': IL Summing: sumWorkQFidData; workQEntry: 0x%lx\n",CntlrName, pWorkQEntry);
             // using stat block np was right for imaging with nfmod set. e.g. np=256 with nfmod=128 np really equals 32768
             // overflow =  sumWorkQFidData(pWorkQEntry->pWorkQObj,pWorkQEntry,pStatBlk->dataSize,pStatBlk->np);
             overflow =  sumWorkQFidData(pWorkQEntry->pWorkQObj,pWorkQEntry,pStatBlk->dataSize,np);
          }
          /* since it's either float or short the only error could be for short addition */
          if (overflow)
          {
       	      errLogRet(ErrLogOp,debugInfo,
			"sumShortData: OverFlow Detected, Data Corrupted");
          }
      }

   }

  // } // end if EXP_FIDZERO_CMPLT 

   /* |||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| */


   /* |||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| */

   /* Update fid block header for this FID */
   /* fidblockheader.scale = fidstatblk.scale; */
   /* fidblockheader.index = htons(pStatBlk->elemId); */
   /* fidblockheader.index = htons(pWorkQEntry->trueElemId); */

   /*  Only update the fid block header for the last of cf fids
    * otherwise processing can happen prior to all 'echos' cf have been
    * place into the fid file.  bug# 782
    * this test works since cf fids always come in 1-nf sequence order
    */
   DPRINT3(+1,"'%s': revFid:  pWorkQEntry->cf+1= %d == pWorkQEntry->pInvar->NumFids=%d \n",
          CntlrName,pWorkQEntry->cf+1,pWorkQEntry->pInvar->NumFids);
   if (pWorkQEntry->cf+1 == pWorkQEntry->pInvar->NumFids)
   {

      pBlockHeader->index = htons(pWorkQEntry->trueElemId);

      /* If acquisition is clearing data at blocksize, correct the ct */
      if (pExpInfo->ExpFlags & CLR_AT_BS_BIT) 
      {
         pBlockHeader->ctcount = htonl(pExpInfo->NumInBS); 
      } 
      else 
      {
         pBlockHeader->ctcount = htonl(pStatBlk->ct);
      }
      pBlockHeader->lvl = (float) 0.0; /* dcLevel[iRcvr].r; */
      pBlockHeader->tlt = (float) 0.0; /* dcLevel[iRcvr].i; */
      if ( (pExpInfo->DspOversamp > 1) && (pStatBlk->ct == 1) )
      { 
        pBlockHeader->lvl = 0.0;  /* make sure FID offsets are zero after dsp */
        pBlockHeader->tlt = 0.0;  /* make sure FID offsets are zero after dsp */
      } 


#ifdef LINUX
      {
       floatInt un;
       un.fval = &(pBlockHeader->lvl);
       *un.ival = htonl( *un.ival );
       un.fval = &(pBlockHeader->tlt);
       *un.ival = htonl( *un.ival );
      }
#endif
      if  (pExpInfo->DspGainBits){
          pBlockHeader->scale = htons( pExpInfo->DspGainBits );
      }

      /* always convert to Big Endian, if we are a BIG Endian machine this does nothing */
      /* DATABLOCKHEADER_CONVERT_HTON(&fidblockheader); */

      /* Copy fid block header to Disk */
      /* we copy this even for RA since the fid data may not be complete
       * (i.e. not all fids taken) before SA. Therefore the fidblockhead may
       * not be present for this FID.
      */
      if (Use_FileIO_Flag == 0)
      {
         fidblkhdrSpot = getWorkQFidBlkHdrPtr(pWorkQEntry->pWorkQObj,pWorkQEntry);
         /* prtMFileMap(pSharedData->pMapFidFile); */
         /* DPRINT5(+1,"'%s': revFid:  status: 0x%hx, %hd, fidblkhdrSpot Addr: 0x%lx, endAddr: 0x%lx \n",
          * CntlrName,pBlockHeader->status,pBlockHeader->status,fidblkhdrSpot, fidblkhdrSpot + sizeof(struct datablockhead)); */
         memcpy((void*)fidblkhdrSpot,(void*)pBlockHeader, sizeof(struct datablockhead));
      }
      else
      { 
           writeWorkQFidHeader2File(pWorkQEntry->pWorkQObj,pWorkQEntry,(void*)pBlockHeader,sizeof(struct datablockhead));
      }
   }

   pStatBlk->elemId = pWorkQEntry->trueElemId;
   if (pStatBlk->elemId > maxFidsRecv)
       maxFidsRecv = pStatBlk->elemId;
   DPRINT2(+1,"'%s': revFid:  Update Max received FIDs (trueElemId) to: %lu\n",CntlrName,maxFidsRecv);

   DPRINT4(+1,"'%s': recvFid:  elemId: %d, blockAddr: 0x%lx, DataAddr: 0x%lx\n",
      CntlrName,pWorkQEntry->pFidStatBlk->elemId, fidblkhdrSpot, dataPtr);
   /* |||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| */
   /* |||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| */
 
    /* increment number of fid or nfmod transfers recieved from this DDR */
    IncrementTransferLoc(pTheFlowObj, getCntlrNum(CntlrName));

    /* if barrierWait() returns -1,  this thread is the last to complete 
     * therefore this thread can be the one to do something special */
    DPRINT1(+1,"'%s': Wait on Barrier\n",CntlrName);
    doProcFlag = -1;
    barrierStatus = barrierWait( &TheBarrier );
    if (barrierStatus == -1)
    {
        /* only the last thread does processing */
        /* we are the one to invoke processing */
        /* And only for the last trace for this FID do we invoke processing */

        /* Since all active Receivers have completed (via barrier wait) prior to processing */
        /* we need to set the ElemId to the max elemid in the active Receiver Elemid group */
        /* GMB 11/03/2009 */
        DPRINT3(+1,"'%s': recvFid: Return from Barrrier, CF: %ld ,  NF/NFmod: %ld \n",CntlrName,pWorkQEntry->cf+1,pWorkQEntry->pInvar->NumFids);
        if (pWorkQEntry->cf+1 == pWorkQEntry->pInvar->NumFids)
        {
            long div,mod,mult,mxElemId;
            /* if (pStatBlk->elemId <  maxFidsRecv) */
            //  if ( (pStatBlk->elemId <  maxFidsRecv) &&  ((expInfo->IlFlag == 1) && last interleave cycle)
            // last interleave cycle;  CT == NT  
            // if ( (pStatBlk->elemId <  maxFidsRecv) && (expInfo->IlFlag != 1))
            DPRINT5(+1,"'%s': recvFid:  elemID: %ld ,  maxFidsRecv: %ld, CT: %ld, NT: %ld \n",
                 CntlrName,pStatBlk->elemId, maxFidsRecv, pStatBlk->ct, pExpInfo->NumTrans);
            div = pStatBlk->elemId / pWorkQEntry->pInvar->numActiveRcvrs;
            mod = pStatBlk->elemId % pWorkQEntry->pInvar->numActiveRcvrs;
            mult = div + ((mod > 0) ? 1 : 0);
            mxElemId = pWorkQEntry->pInvar->numActiveRcvrs * mult;
            // DPRINT7(-51,"elemId(%ld) / ActiveRcvrs(%ld) = %ld, elemId %% ActiveRcvrs = %ld,  ActiveRvcrs(%ld) * mult(%ld) = %ld (mxElemId)\n",
            //       pStatBlk->elemId, pWorkQEntry->pInvar->numActiveRcvrs, div, mod, pWorkQEntry->pInvar->numActiveRcvrs, mult, mxElemId);
            DPRINT4(+1,"'%s': recvFid: Change elemId: %lu, to max ElemId: %ld, base on number of ActiveRcvrs: %ld, for proper processing \n",
                    CntlrName,pStatBlk->elemId, mxElemId, pWorkQEntry->pInvar->numActiveRcvrs); 
            pStatBlk->elemId = mxElemId;

            doProcFlag = 1;
        }
        else
        {
            workQFree(pWorkQEntry->pWorkQObj, pWorkQEntry);
            doProcFlag = -1;
        }
        /* back from barrier wait, test location of transfers, send continue msg to DDRs if appriopriate */
        AllAtIncrementMark(pTheFlowObj, getCntlrNum(CntlrName));
    }
    else if (barrierStatus == -99)   /* aborted */
    {
         workQFree(pWorkQEntry->pWorkQObj, pWorkQEntry);
         rngBlkFlush(pRcvrDesc->pInputQ);
         DPRINT1(+1,"'%s': recvFid: Returned from Aborted Barrier\n",CntlrName);
         doProcFlag = -1;
    }
    else
    {
        workQFree(pWorkQEntry->pWorkQObj, pWorkQEntry);
         doProcFlag = -1;
        DPRINT1(+1,"'%s': recvFid: Returned from Barrier\n",CntlrName);
    }
     
    DPRINT(+1,"++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n\n");
 
    return(doProcFlag);
}


   /* |||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| */
   /* |||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| */
   /* |||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| */
   /* |||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| */
   /* |||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| */

int processFid(WORKQ_ENTRY_ID pWorkQEntry)
{        
    tcrc crcChkSum, calcCRC;
    FID_STAT_BLOCK *pStatBlk;
     int status,stat;
    long fidNum,fidSizeBytes;
    char *dataPtr,*fidblkhdrSpot;
    int return_status;
    char *CntlrName;
    cntlr_crew_t *crew;
    RCVR_DESC_ID pRcvrDesc;
    WORKQINVARIENT_ID pWrkqInvar;
    SHR_EXP_INFO pExpInfo = NULL;   /* start address of shared Exp. Info Structure */
    SHARED_DATA_ID pSharedData;
 
    pRcvrDesc = (RCVR_DESC_ID) pWorkQEntry->pInvar->pRcvrDesc;

    CntlrName = pRcvrDesc->cntlrId;
    pExpInfo = (SHR_EXP_INFO) pRcvrDesc->pParam;

    pStatBlk = pWorkQEntry->pFidStatBlk;
    DPRINT(+1,"\n\n++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
    DPRINT2(+1,"'%s': processFid:  FidStatBlock Type: %d\n",CntlrName,pWorkQEntry->statBlkType);

    DPRINT6(+1,"'%s': StatBlk Type: %d, (ERRSTATBLK=%d, WRNSTATBLK=%d, FIDSTATBLK=%d, SU_STOPSTATBLK=%d) \n",CntlrName,
		pWorkQEntry->statBlkType,ERRSTATBLK,WRNSTATBLK,FIDSTATBLK,SU_STOPSTATBLK);
    DPRINT7(+1,"'%s': >>>>>>> Fid StatBlk: Fid: %ld, DataSize: %ld, CT: %ld, NP: %ld, DoneCode: 0x%x, %d\n",
                CntlrName,pStatBlk->elemId, pStatBlk->dataSize, pStatBlk->ct,
                pStatBlk->np, pStatBlk->doneCode,(pStatBlk->doneCode & 0xFFFF));

    DPRINT3(+1,"'%s': >>>>>>> CF: %ld ,  NF/NFmod: %ld \n",CntlrName,pWorkQEntry->cf+1,pWorkQEntry->pInvar->NumFids);
    if ( ExpCmplted(pWorkQEntry) == 1)
    {
       DPRINT(+1,"Experiment Already completed, Ignore Processing\n");
       DPRINT(+1,"++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n\n");
       return -1;
    }

    if (pWorkQEntry->statBlkType != ERRSTATBLK)
    {
       /* if (pStatBlk->elemId > maxFidsRecv)
	   maxFidsRecv = pStatBlk->elemId; */

      	/* --- Update Current CT & Current Element ----- */
      	pExpInfo->CurrentTran = pStatBlk->ct;	/* Current Transient */
      	pExpInfo->Celem = pStatBlk->elemId;       /* Current Element */

      	/* set the number of completed FIDs */
      	/* Note using pExpInfo->NumTrans to determine if FIDs are completed
	   is probably not going to work in the long run, since people
	   will want to array NT, which means each FID would have its own
	   criteria for completion....
      	*/
      	if ( ((pStatBlk->doneCode & 0xFFFF)  == EXP_FID_CMPLT) && (pWorkQEntry->cf+1 == pWorkQEntry->pInvar->NumFids) )
      	{
           DPRINT2(+1,"'%s': Set pExpInfo->FidsCmplt = pStatBlk->elemId =  %ld \n",CntlrName,pStatBlk->elemId);
           pExpInfo->FidsCmplt = pStatBlk->elemId;
      	}

      	/* Now the FID Xfr is complete, time to update status struct &
	   check on processing */

      	/* Update Status so that others can be aware of whats happening */
      	UpdateStatus(pStatBlk);

     }

     /* return_status = processDoneCode(pExpInfo,pStatBlk); /* does not do FID Cmplt processing */

     /* return the workQEntry to the free list */
    /* workQFree(pWorkQEntry->pWorkQObj, pWorkQEntry); */
    DPRINT2(+1,"'%s': processFid:  processDoneCode stat: %d\n",CntlrName,return_status);
    DPRINT(+1,"++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n\n");
   switch( (pStatBlk->doneCode & 0xFFFF) )
   {
	   case BS_CMPLT:
                if (pExpInfo->NumInBS != 0)
	            processBS(pStatBlk);
                return_status = -1;
	        break;
	   case WARNING_MSG:		/* warnings */
	        processWarning(pStatBlk);
                return_status = -1;
	        break;
	   case EXP_FID_CMPLT:
                // FFFFFFF
                // pWorkQEntry->pInvar->numActiveRcvrs
                if ( pStatBlk->elemId != pExpInfo->ArrayDim)
                {
		   procesWNT(pStatBlk);
                   return_status = -1;
		}
                else
                   return_status = 0;
		break;
	  default:
                return_status = 0;
		break;
    }
 
    /* if ( (return_status == BS_CMPLT) || (return_status == WARNING_MSG) || (return_status == 0) ) */
    if (return_status == -1)
    {
      /* --- Update Current CT & Current Element ----- */
      pExpInfo->CurrentTran = pStatBlk->ct;	/* Current Transient */
      pExpInfo->Celem = pStatBlk->elemId;       /* Current Element */
      /* return the workQEntry to the free list */
      workQFree(pWorkQEntry->pWorkQObj, pWorkQEntry);
      return(-1);   /* not the end of experiment yet */
    }
    else   /* this is a terminating DoneCode whatever it might be */
    {
       int done;
      /* --- Update Current CT & Current Element ----- */
      pExpInfo->CurrentTran = pStatBlk->ct;	/* Current Transient */
      pExpInfo->Celem = pStatBlk->elemId;       /* Current Element */
     if ( ((pStatBlk->doneCode & 0xFFFF)  == EXP_FID_CMPLT) && (pWorkQEntry->cf+1 == pWorkQEntry->pInvar->NumFids) )

       if (! pExpInfo->GoFlag)
       {
            struct datafilehead *pdatafilehead;
            fidSizeBytes = pWorkQEntry->pFidStatBlk->dataSize + sizeof(struct datablockhead);
           /*  dataPtr = getWorkQFidPtr(pWorkQEntry->pWorkQObj,pWorkQEntry); */
           /* vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv */
           /* if files are > 2 GB, then the file will have multiple mmaps, thus to be sure
            * we are at the very beginning of the file, we must seek to the beginning
            * ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ */
           if (pWorkQEntry->pInvar->fiddatafile->pMapFile != NULL)
           {
              mMutexLock(pWorkQEntry->pInvar->fiddatafile->pMapFile);
                status =  mFidSeek(pWorkQEntry->pInvar->fiddatafile->pMapFile, 1L, sizeof(struct datafilehead), fidSizeBytes);
                pdatafilehead = (struct datafilehead *) pWorkQEntry->pInvar->fiddatafile->pMapFile->mapStrtAddr;
              mMutexUnlock(pWorkQEntry->pInvar->fiddatafile->pMapFile);
              DPRINT2(+1,"'%s': processFid:  Update Fid File's received FIDs to: %lu\n",CntlrName,maxFidsRecv);
	      pdatafilehead->nblocks = htonl(maxFidsRecv);
           }
           else
           {
                struct datafilehead DataFileHead;
                fFileRead(pWorkQEntry->pInvar->fiddatafile->pFile, (char*) &DataFileHead, sizeof(struct datafilehead), (off_t) 0LL);
                DataFileHead.nblocks = htonl(maxFidsRecv);
                fFileWrite(pWorkQEntry->pInvar->fiddatafile->pFile, (char*) &DataFileHead, sizeof(struct datafilehead), (off_t) 0LL);
           }
       }
       done = (return_status != EXP_FID_CMPLT);
       DPRINT4(+1,"'%s': processFid:  return_status(%d) != EXP_FID_CMPLT(%d) = %d\n",
		CntlrName,return_status,EXP_FID_CMPLT,done);
       done = (return_status == EXP_FID_CMPLT) && (pWorkQEntry->cf+1 == pWorkQEntry->pInvar->NumFids);
       DPRINT6(+1,"'%s': processFid:  return_status(%d) == EXP_FID_CMPLT(%d) && (cf+1 (%d) == nf (%d)) = %d\n",
		CntlrName,return_status,EXP_FID_CMPLT,pWorkQEntry->cf+1, pWorkQEntry->pInvar->NumFids,done);
       /* if ( (return_status != EXP_FID_CMPLT) || ((return_status == EXP_FID_CMPLT) && 
            (pWorkQEntry->cf+1 == pWorkQEntry->pInvar->NumFids)) ) */
       if ( ((pStatBlk->doneCode & 0xFFFF) != EXP_FID_CMPLT) || (((pStatBlk->doneCode & 0xFFFF) == EXP_FID_CMPLT) && 
            (pWorkQEntry->cf+1 == pWorkQEntry->pInvar->NumFids)) )
       {
          if ( ((pStatBlk->doneCode & 0xFFFF) == EXP_HALTED) ||
               ((pStatBlk->doneCode & 0xFFFF) == EXP_ABORTED) ||
               ((pStatBlk->doneCode & 0xFFFF) == HARD_ERROR) )
          {
               /* if halting the release any recvFid() thread from the barrier wait */
              /* barrierWaitAbort(&TheBarrier); */
              rngBlkFlush(ProcessThread.pInputQ);
              DPRINT1(+1,"'%s': processFid:  Exp Halted, Flush processing Q\n",
		CntlrName);
          }

           /* return the workQEntry to the free list */
           /* workQFree(pWorkQEntry->pWorkQObj, pWorkQEntry); wait this must be done in main thread now */
          DPRINT(+1,"calling wait4DoneCntlrStatus\n");
          wait4DoneCntlrStatus();
          DPRINT(+1,"returned from wait4DoneCntlrStatus\n");
          /* recvFidsCmplt(pWorkQEntry); */
          /* recvFidsCmplt() now called in the context of main thread, via SIGUSR2, 3/27/2006 GMB */
          pWorkQEntryCmplt = pWorkQEntry;  /* set WorkQ for recvFidsCmplt() invoked from main thread */
          pthread_kill(main_threadId ,SIGUSR2);  /* signal main thread to call recvFidsCmplt() */
          return(-1);    /* exp finished */
       }
       else
       {
          /* return the workQEntry to the free list */
          workQFree(pWorkQEntry->pWorkQObj, pWorkQEntry);
          return(-1);    /* not all trace  present yet for NF  */
       }
    }
}


   /* |||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| */
   /* |||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| */
   /* |||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| */
   /* |||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| */



#ifdef LINUX
static void byteSwap(void *data, register int num, int isShort)
{
   /* byte-swap data to Linux format */
   register int cnt;
   if (isShort)
   {
      register short *sPtr;
      sPtr = (short *) data;
      for (cnt = 0; cnt < num; cnt++)
      {
         *sPtr = ntohs( *sPtr );
         sPtr++;
      }
   }
   else
   {
      register int *iPtr;
      iPtr = (int *) data;
      for (cnt = 0; cnt < num; cnt++)
      {
         *iPtr = ntohl( *iPtr );
         iPtr++;
      }
   }
}
#endif

/***********************************************************
*
* sumFloatData
*   Sums the Interleaved BS Fid with the mmap data file fid
*   dstadr is the mmap data fid addr, srdadr is the BS Fid
*   np is the number of data points to be added
*/
int sumFloatData( float* dstadr, float* srdadr, unsigned long np)
{
   unsigned long i;
   // DPRINT3(1,"sumFloatData - dstadr: 0x%lx, srcadr: 0x%lx,  np: %ld\n",dstadr,srdadr,np);

#ifdef LINUX
   union _swap_union_ { float f; uint32_t ui; } dstval, srcval, sumval;
   // struct _floatswap_ sf;
#else
   float dstval,srcval,sumval;
#endif

#ifndef LINUX

   for (i=0;i<np;i++,dstadr++)
   {
#ifdef XXXX_DEBUG
     if (i<20)
     {
        DPRINT3(1,"sum: %f = %f + %f\n",*dstadr + *srdadr,*dstadr,*srdadr);
     }
#endif 

     *dstadr += *srdadr++;   /* sum that data */

   }

#else  /* LINUX i.e. Little Endian */

   for (i=0;i<np;i++,dstadr++,srdadr++)
   {
      dstval.f = *dstadr;
      srcval.f = *srdadr;

#ifdef XXX
*     if (i<20)
*     {
*        DPRINT2(+14,"dstbi: 0x%lx, srcbi: 0x%lx,\n", 
*		      dstval.ui, srcval.ui);
*     }
#endif
      
      /* convert the big endian floats to little endian via union */
      dstval.ui = ntohl( dstval.ui );
      srcval.ui = ntohl( srcval.ui );

      sumval.f = dstval.f + srcval.f;

#ifdef XXX_DEBUG
     if (i<20)
     {
        /* DPRINT2(-14,"dstli: 0x%lx, srcli: 0x%lx\n", 
		      dstval.ui, srcval.ui);  */
        DPRINT3(1,"sum: %f = %f + %f\n",sumval.f, dstval.f, srcval.f);
     }
#endif

      sumval.ui = htonl( sumval.ui );
        
     *dstadr = sumval.f;
   }
#endif  /* Linux */

   return(0);
}

/***********************************************************
*
* sumLongData
*   Sums the Interleaved BS Fid with the mmap data file fid
*   dstadr is the mmap data fid addr, srdadr is the BS Fid
*   np is the number of data points to be added
*/
int sumLongData( long* dstadr, long* srdadr, unsigned long np)
{
   unsigned long i;
   long maxval = 0x7FFFFFFF - 0x100000; /* 2^31 - 2^20, imminent overflow */
   int ovrflow = 0;

   for (i=0;i<np;i++,dstadr++)
   {
#ifdef XXX_DEBUG
     if (i<20)
     {
        DPRINT3(1,"sum: %ld = %ld + %ld\n",*dstadr + *srdadr,*dstadr,*srdadr);
     }
#endif
     if ( (*dstadr += *srdadr++) > maxval)
	ovrflow = 1;
     if (*dstadr < -maxval)
	ovrflow = 1;
   }
   return(ovrflow);
}

/***********************************************************
*
* sumShortData
*   Sums the Interleaved BS Fid with the mmap data file fid
*   dstadr is the mmap data fid addr, srdadr is the BS Fid
*   np is the number of data points to be added
*/
int sumShortData( short* dstadr, short* srdadr, unsigned long np)
{
   unsigned long i;
   short dstval, srcval;
   long sumval;
   long maxval =  0x7FFF;  /* 2^15 */
   int ovrflow = 0;

   // DPRINT3(1,"sumShortData - dstadr: 0x%lx, srcadr: 0x%lx,  np: %ld\n",dstadr,srdadr,np);

#ifndef LINUX
   for (i=0;i<np;i++,dstadr++)
   {
     if ( (sumval = (long) *dstadr + (long) *srdadr++) > maxval)
	ovrflow = 1;
     if (sumval < -maxval)
	ovrflow = 1;

#ifdef XXX_DEBUG
     if (i<20)
     {
        DPRINT4(-11,"sum: %ld = %hd + %hd\n, ovrflow: %d",
		sumval,*dstadr,*(srdadr-1),ovrflow);
     }
#endif
        
     *dstadr = (short) sumval;
   }

#else  /* LINUX i.e. Little Endian */

   for (i=0;i<np;i++,dstadr++,srdadr++)
   {
      dstval = ntohs( *dstadr );
      srcval = ntohs( *srdadr );
     if ( (sumval = (long) dstval + (long) srcval) > maxval)
	     ovrflow = 1;
     if (sumval < -maxval)
	     ovrflow = 1;

#ifdef XXX_DEBUG
     if (i<20)
     {
        DPRINT4(-11,"sum: %ld = %hd + %hd\n, ovrflow: %d",
		sumval,dstval,srcval,ovrflow);
     }
#endif
        
     *dstadr = (short) htons(sumval);
   }
#endif  /* Linux */

   return(ovrflow);
}

int abortCodes(char *str)
{
   DPRINT(1,"abortCodes: \n");
   return(0);
}

void resetState()
{
  /* close msgQs if open */
  if (pProcMsgQ != NULL)
  {
	closeMsgQ(pProcMsgQ);
	pProcMsgQ = NULL;
  }
  if (pExpMsgQ != NULL)
  {
	closeMsgQ(pExpMsgQ);
	pExpMsgQ = NULL;
  }
  /* close mmap data file if opened. */
  if (ifile != NULL)
  {
    mClose(ifile); 
    ifile = NULL;
  }

  /* map out shared expingo info file  */
  if ( (int) strlen(expInfoFile) > 1)
  {
     mapOut(expInfoFile);
     expInfoFile[0] = '\0';
  }
}

int consoleConn()
{
    return 1;
}
void terminate(char *str)
{
   DPRINT(1,"terminate: shutdown comlink, Release processing Q\n");
   shutdownComm();
   procQRelease();
   expStatusRelease();
   exit(0);
}

void ShutDownProc()
{
   shutdownComm();
   procQRelease();
   expStatusRelease();
}

int debugLevel(char *str)
{
    extern int DebugLevel;
    char *value;
    int  val;
    value = strtok(NULL," ");
    val = atoi(value);
    DebugLevel = val;
    DPRINT1(1,"debugLevel: New DebugLevel: %d\n",val);
    return(0);
}

int mapIn(char *filename)
{
    DPRINT1(1,"mapIn: mapin  '%s'\n",filename);

    ShrExpInfo = shrmCreate(filename,SHR_EXP_INFO_RW_KEY,(unsigned long)sizeof(SHR_EXP_STRUCT)); 
    if (ShrExpInfo == NULL)
    {
       errLogSysRet(ErrLogOp,debugInfo,"mapIn: shrmCreate() failed:");
       return(-1);
    }

    if (ShrExpInfo->shrmem->byteLen < sizeof(SHR_EXP_STRUCT))
    {
	/* hey, this file is not a shared Exp Info file */
       shrmRelease(ShrExpInfo);		/* release shared Memory */
       unlink(filename);	/* remove filename that shared Mem created */
       ShrExpInfo = NULL;
       expInfo = NULL;
       return(-1);
    }

#ifdef DEBUG
    if (DebugLevel >= 2)
      shrmShow(ShrExpInfo);
#endif

    expInfo = (SHR_EXP_INFO) shrmAddr(ShrExpInfo);

    return(0);
}

int mapOut(char *str)
{
    if (ShrExpInfo != NULL)
    {
       shrmRelease(ShrExpInfo);
       ShrExpInfo = NULL;
       expInfo = NULL;
    }
    return(0);
}

char *createFidBlockHeader()
{
  struct datablockhead *pFidBlockHeader;
  pFidBlockHeader = (struct datablockhead *) malloc( sizeof(struct datablockhead) );
  return (char*) pFidBlockHeader;
}

int initBlockHeader(struct datablockhead *pFidBlockHeader, SHR_EXP_INFO pExpInfo )
{
        /* --------------------  FID Header  ---------------------------- */
        pFidBlockHeader->scale = (short) 0;
        pFidBlockHeader->status = S_DATA | S_OLD_COMPLEX | S_DDR;/* init status to fid*/
        pFidBlockHeader->index = (short) 0;
        pFidBlockHeader->mode = (short) 0;
        pFidBlockHeader->ctcount = (long) 0;
        pFidBlockHeader->lpval = (float) 0.0;
        pFidBlockHeader->rpval = (float) 0.0;
        pFidBlockHeader->lvl = (float) 0.0;
        pFidBlockHeader->tlt = (float) 0.0;

        /* for the DDR  dp='y'-> float else it's 16-bit int, (those imagers) */
        if ( pExpInfo->DataPtSize == 4)  /* dp='y' (4) */
        {
           pFidBlockHeader->status |= S_FLOAT;
        }

#ifdef LINUX
       {
          recvProcHeaderUnion hU;
                                                                                
          hU.in1 = pFidBlockHeader;
          hU.out->s1 = htons(hU.out->s1);
          hU.out->s2 = htons(hU.out->s2);
          hU.out->s3 = htons(hU.out->s3);
          hU.out->s4 = htons(hU.out->s4);
          hU.out->l1 = htonl(hU.out->l1);
          hU.out->l2 = htonl(hU.out->l2);
          hU.out->l3 = htonl(hU.out->l3);
          hU.out->l4 = htonl(hU.out->l4);
          hU.out->l5 = htonl(hU.out->l5);
       }
#endif

    return(0);

}

/**************************************************************
}
/**************************************************************
*
*  InitialFileHeaders - initial the static file & block headers 
*
* This routine initializes the file header and block header
* strcutures.
*
* RETURNS:
* 0 , else -1
*
*       Author Greg Brissey 8/12/94
*/
int InitialFileHeaders()
{

/*
    int             decfactor = 1,
                    dscoeff = 1;
*/
 
    /* datasize = expptr->DataPtSiz;  data pt bytes
     * fidsize  = expptr->FidSiz;     fid in bytes
     * np = expptr->N_Pts;
     * dpflag = datasize;
     */

    /* decfactor = *( (int *)getDSPinfo(DSP_DECFACTOR) ); */


        /* fidfileheader.nblocks   = expInfo->ArrayDim / expInfo->NumFids; /* n fids, testing */
        fidfileheader.nblocks   = expInfo->ArrayDim; /* n fids*/
        fidfileheader.ntraces   = expInfo->NumFids; /* NF */;
        /* fidfileheader.np  	= ( dspflag ? np / decfactor : np ); */
        fidfileheader.np  	= expInfo->NumDataPts;
        fidfileheader.ebytes    = expInfo->DataPtSize;     /* data pt bytes */
        fidfileheader.tbytes    = expInfo->DataPtSize * expInfo->NumDataPts; /* trace in bytes */
                                /*blk in byte*/
        bbytes = (fidfileheader.tbytes * fidfileheader.ntraces)
                                        + sizeof(fidblockheader);
        fidfileheader.bbytes = bbytes;
 
        fidfileheader.nbheaders = 1;
        fidfileheader.status    = S_DATA | S_OLD_COMPLEX | S_DDR;/* init status FID */
        fidfileheader.vers_id   = 0;
#ifdef LINUX
        fidfileheader.nblocks   = htonl(fidfileheader.nblocks);
        fidfileheader.ntraces   = htonl(fidfileheader.ntraces);
        fidfileheader.np        = htonl(fidfileheader.np);
        fidfileheader.ebytes    = htonl(fidfileheader.ebytes);
        fidfileheader.tbytes    = htonl(fidfileheader.tbytes);
        fidfileheader.bbytes    = htonl(fidfileheader.bbytes);
        fidfileheader.nbheaders = htonl(fidfileheader.nbheaders);
        fidfileheader.vers_id   = htons(fidfileheader.vers_id);
#endif

        /* --------------------  FID Header  ---------------------------- */
        fidblockheader.scale = (short) 0;
        fidblockheader.status = S_DATA | S_OLD_COMPLEX | S_DDR;/* init status to fid*/
        fidblockheader.index = (short) 0;
        fidblockheader.mode = (short) 0;
        fidblockheader.ctcount = (long) 0;
        fidblockheader.lpval = (float) 0.0;
        fidblockheader.rpval = (float) 0.0;
        fidblockheader.lvl = (float) 0.0;
        fidblockheader.tlt = (float) 0.0;

        /* for the DDR  dp='y'-> float else it's 16-bit int, (those imagers) */
        
        if ( expInfo->DataPtSize == 4)  /* dp='y' (4) */
        {
           fidfileheader.status |= S_FLOAT;
           fidblockheader.status |= S_FLOAT;
        }

#ifdef INOVA
        if ( expInfo->DataPtSize == 4)  /* dp='y' (4) */
        {                                
           if (dspflag) 		/* values set to S_FLOAT not S_32 */
           {
              fidfileheader.status |= S_FLOAT;
              fidblockheader.status |= S_FLOAT;
           }
           else
           {
              fidfileheader.status |= S_32;
              fidblockheader.status |= S_32;
           }
        }
#endif

      /* fidfileheader.status    = htons(fidfileheader.status); */
      /* DATABLOCKHEADER_CONVERT_HTON(&fidblockheader); */

#ifdef LINUX
       {
          recvProcHeaderUnion hU;
                                                                                
          fidfileheader.status    = htons(fidfileheader.status);
          hU.in1 = &fidblockheader;
          hU.out->s1 = htons(hU.out->s1);
          hU.out->s2 = htons(hU.out->s2);
          hU.out->s3 = htons(hU.out->s3);
          hU.out->s4 = htons(hU.out->s4);
          hU.out->l1 = htonl(hU.out->l1);
          hU.out->l2 = htonl(hU.out->l2);
          hU.out->l3 = htonl(hU.out->l3);
          hU.out->l4 = htonl(hU.out->l4);
          hU.out->l5 = htonl(hU.out->l5);
       }
#endif

    return(0);

}

/**************************************************************
*
*  UpdateStatus - Updates appropriate members of Status Struct
*
* This routine updates the Status apprropriate for recvproc
*
* expanded to update the FID element number, taking into account
* requirements for an interleaved experiment.
*
* RETURNS:
* nothing
*
*       Author Greg Brissey 10/28/94
*/
void UpdateStatus(FID_STAT_BLOCK *pFidstatblk)
{
      int arraydim, bs, ctcnt, elemid, ilflag;

      DPRINT(2,"UpdateStatus: \n");
      setStatDataTime();

/*  Avoid program failures if for some bizarre reason expInfo is NULL.  */

      if (expInfo == NULL) {
          arraydim = 1;
          bs = 1;
          ilflag = 0;
      }
      else {
          arraydim = expInfo->ArrayDim;
          bs = expInfo->NumInBS;
          ilflag = expInfo->IlFlag;
      }

/*  Only update the element ID if it might have changed  */

      if ((pFidstatblk->doneCode & 0xFFFF) == EXP_FID_CMPLT ||
         ((pFidstatblk->doneCode & 0xFFFF) == BS_CMPLT && ilflag)) {
          elemid = pFidstatblk->elemId + 1;
          if (elemid > arraydim) {
              if ((pFidstatblk->doneCode & 0xFFFF) == BS_CMPLT)
                  elemid = 1;
                else
                  elemid = 0;		/* the experiment is over */
          }

/*  Follow the logic.  The done code is either EXP_FID_CMPLT or BS_CMPLT, and
    if it is BS_CMPLT then interleaving is in effect.  In each case the current
    element has completed (temporarily for BS_CMPLT of course) and the console
    has moved on to the next element.  Therefore the status display also needs
    to move on to the next element.  If the new element ID is larger than the
    number of elements and the done code is EXP_FID_CMPLT, then the experiment
    has completed.

    In no other case does the element ID change.				*/

          setStatElem(elemid);
          if ((pFidstatblk->doneCode & 0xFFFF) == BS_CMPLT) {
              if (elemid > 1)
                ctcnt = pFidstatblk->ct - bs;
              else
                ctcnt = pFidstatblk->ct;	/* a new interleave cycle has started */
                                             /* the ct count for the new cycle starts */
          }                                    /* at the value where the old one ends */
          else if (ilflag && elemid > 0)
            ctcnt = pFidstatblk->ct - bs;  /* the final interleave cycle is completing */
          else
            ctcnt = 0;         /* ct count is always 0 when the experiment completes, */
      }                                                         /* interleaved or not */
      else
        ctcnt = pFidstatblk->ct;

      setStatCT(ctcnt);
}

/**************************************************************
*
*  Process - decides if Data Process is required
*
* This routine decides if Data Process is required. 
* Then sends Procproc a message to perform the
* the appropriate processing.
*
* RETURNS:
* 0 , else -1
*
*       Author Greg Brissey 8/12/94
*/
#define ERROR 0xffffffff

processBS(FID_STAT_BLOCK *pFidStatBlk)
{
    int stat,qstat;

     /* if hard error already process no need for addition thread to do processing */
     if ( HardErrorFlag == 1)
     {
        DPRINT(1,"processBS: HardErrorFlag set, just return, no processing\n");
        return 0; 
     }
     /* test done in calling routine */
     /* if (pExpInfo->NumInBS != 0) */
     /* { */

        DPRINT(1,"processBS:  Processing\n");
        /* msgQ full, just come back, we can skip Wbs processing */
        qstat = procQadd(WBS, ShrExpInfo->MemPath, pFidStatBlk->elemId, pFidStatBlk->ct,
                             pFidStatBlk->doneCode, pFidStatBlk->ct / expInfo->NumInBS);
        if (qstat != SKIPPED)
        {
            DPRINT2(1,"processBS: WBS: Queue FID: %ld, CT: %ld\n",
		        pFidStatBlk->elemId, pFidStatBlk->ct);
	    if ((stat = sendMsgQ(pProcMsgQ,"chkQ",strlen("chkQ"),MSGQ_NORMAL,NO_WAIT)) != 0)
	    {
       	         errLogRet(ErrLogOp,debugInfo, 
	               "Process: Procproc is not running. Wbs Process not done for FID %lu, CT: %lu\n",
		           pFidStatBlk->elemId,pFidStatBlk->ct);
                 return(-1);
	     }
  	}
     /* } */
     return 0;
}

processWarning(FID_STAT_BLOCK *pFidStatBlk)
{
    int stat,qstat;

     /* if hard error already process no need for addition thread to do processing */
     if ( HardErrorFlag == 1)
     {
        DPRINT(1,"processDoneCode: HardErrorFlag set, just return, no processing\n");
        return 0; 
     }
     DPRINT(1,"processWarning: WARNING_MSG Werr Processing\n");
     /* wait because we guarantee this processing tobe done*/
     procQadd(WERR, ShrExpInfo->MemPath, pFidStatBlk->elemId, 
              /*  pFidStatBlk->ct */ 1L, pFidStatBlk->doneCode, pFidStatBlk->errorCode);
     if ((stat = sendMsgQ(pProcMsgQ,"chkQ",strlen("chkQ"),
	          MSGQ_NORMAL,WAIT_FOREVER)) != 0)
     {
          errLogRet(ErrLogOp,debugInfo, 
   	       "Process: Procproc is not running. Error Process not done for FID %lu, CT: %lu\n",
	  pFidStatBlk->elemId,pFidStatBlk->ct);
     }
     return 0;
}

procesWNT(FID_STAT_BLOCK *pFidStatBlk)
{
    int stat,qstat;

     /* if hard error already process no need for addition thread to do processing */
     if ( HardErrorFlag == 1)
     {
        DPRINT(1,"processDoneCode: HardErrorFlag set, just return, no processing\n");
        return 0; 
     }

     DPRINT(+1,"Wnt Processing\n");
     /* msgQ full, just come back, we can skip Wnt processing */
     qstat = procQadd(WFID, ShrExpInfo->MemPath, pFidStatBlk->elemId, pFidStatBlk->ct,
                            pFidStatBlk->doneCode, pFidStatBlk->errorCode);
     if (qstat != SKIPPED)
     {
         DPRINT2(+1,"wNT: Queue FID: %ld, CT: %ld\n",
		            pFidStatBlk->elemId, pFidStatBlk->ct);
	 if ((stat = sendMsgQ(pProcMsgQ,"chkQ",strlen("chkQ"),MSGQ_NORMAL,NO_WAIT)) != 0)
	 {
       	     errLogRet(ErrLogOp,debugInfo, 
	            "Process: Procproc is not running. Wnt Process not done for FID %lu, CT: %lu\n",
		               pFidStatBlk->elemId,pFidStatBlk->ct);
             return(-1);
	 }
     }
    return(0);
}

int processDoneCode(SHR_EXP_INFO pExpInfo,FID_STAT_BLOCK *pFidStatBlk)
{
    int return_status,stat,qstat;

     /* if hard error already process no need for addition thread to do processing */
     if ( HardErrorFlag == 1)
     {
        DPRINT(1,"processDoneCode: HardErrorFlag set, just return, no processing\n");
        return 0; 
     }

	 switch( (pFidStatBlk->doneCode & 0xFFFF) )
         {

#ifdef XXXX
	   case WARNING_MSG:		/* warnings */
	        DPRINT(1,"processDoneCode: WARNING_MSG Werr Processing\n");
	        /* wait because we guarantee this processing tobe done*/
	        procQadd(WERR, ShrExpInfo->MemPath, pFidStatBlk->elemId, 
			     /*  pFidStatBlk->ct */ 1L, pFidStatBlk->doneCode, pFidStatBlk->errorCode);
	        if ((stat = sendMsgQ(pProcMsgQ,"chkQ",strlen("chkQ"),
			          MSGQ_NORMAL,WAIT_FOREVER)) != 0)
	        {
	          errLogRet(ErrLogOp,debugInfo, 
	   	       "Process: Procproc is not running. Error Process not done for FID %lu, CT: %lu\n",
		  pFidStatBlk->elemId,pFidStatBlk->ct);
	        }
                return_status = WARNING_MSG; 	/* don't stop just a warning */
	        DPRINT(1,"WARNING_MSG: continue\n");
                break;
#endif

	   case STOP_CMPLT:		/* result of SA, no process */
	       {
		  int SA_Type;
                DPRINT(1,"processDoneCode: STOP_CMPLT\n");
		/* A Halt is equivilent to a AA but Wexp proc not Werr */
	        /* full, wait because we guarantee this processing tobe done */
                /* use pExpInfo->Celem & pExpInfo->CurrentTran, since they reflect last
		   data transfer prior to this stop message
  	 	*/
		SA_Type = STOP_EOF;
 		switch(pFidStatBlk->errorCode)
                {
		  case EXP_FID_CMPLT: SA_Type = STOP_EOF; break;
		  case BS_CMPLT:      SA_Type = STOP_EOB; break;
		  case CT_CMPLT:      SA_Type = STOP_EOS; break;
		  case IL_CMPLT:      SA_Type = STOP_EOC; break;
                }
                if (pExpInfo->ProcWait > 0)   /* au(wait) or just au */
                {
                   DPRINT2(1,
		     "processDoneCode: wExp(wait): Queue FID: %ld, CT: %ld\n",
		          pFidStatBlk->elemId, pFidStatBlk->ct);
	           procQadd(WEXP_WAIT, ShrExpInfo->MemPath, pExpInfo->Celem, 
		      pExpInfo->CurrentTran, pFidStatBlk->doneCode, SA_Type);
                }
                else
                {
                   DPRINT2(1,"processDoneCode wExp: Queue FID: %ld, CT: %ld\n",
		       pFidStatBlk->elemId, pFidStatBlk->ct);
	           procQadd(WEXP, ShrExpInfo->MemPath, pExpInfo->Celem, 
		      pExpInfo->CurrentTran, pFidStatBlk->doneCode, SA_Type);
                }
	        if ((stat = sendMsgQ(pProcMsgQ,"chkQ",strlen("chkQ"),
				MSGQ_NORMAL,WAIT_FOREVER)) != 0)
	        {
       	           errLogRet(ErrLogOp,debugInfo, 
	           "processDoneCode: Procproc is not running. Wexp Process not done for FID %lu, CT: %lu\n",
		       pFidStatBlk->elemId,pFidStatBlk->ct);
	        }
                return_status = EXP_STOPPED;
		/* return(EXP_STOPPED); */
               }
		break;

	   case EXP_HALTED:		/* result of Halt, do Wexp processing */
               DPRINT(1,"processDoneCode: EXP_HALTED\n");
              {
		/* A Halt is equivilent to a AA but Wexp proc not Werr */
                /* use pExpInfo->Celem & pExpInfo->CurrentTran, since they reflect last
		   data transfer prior to this stop message
  	 	*/
		pFidStatBlk->doneCode = EXP_ABORTED;
                /* Set this now, since go may check if acquisition is active */
                setStatExpName("");
                setStatGoFlag(-1);
	        /* full, wait because we guarantee this processing tobe done */
                if (pExpInfo->ProcWait > 0)   /* au(wait) or just au */
                {
                   DPRINT2(1,
		     "processDoneCode: wExp(wait): Queue FID: %ld, CT: %ld\n",
		          pFidStatBlk->elemId, pFidStatBlk->ct);
	           procQadd(WEXP_WAIT, ShrExpInfo->MemPath, pExpInfo->Celem, 
		      pExpInfo->CurrentTran, pFidStatBlk->doneCode, pFidStatBlk->errorCode);
                }
                else
                {
                   DPRINT2(1,"processDoneCode wExp: Queue FID: %ld, CT: %ld\n",
		       pFidStatBlk->elemId, pFidStatBlk->ct);
	           procQadd(WEXP, ShrExpInfo->MemPath, pExpInfo->Celem, 
		      pExpInfo->CurrentTran, pFidStatBlk->doneCode, pFidStatBlk->errorCode);
                }
	        if ((stat = sendMsgQ(pProcMsgQ,"chkQ",strlen("chkQ"),
				MSGQ_NORMAL,WAIT_FOREVER)) != 0)
	        {
       	           errLogRet(ErrLogOp,debugInfo, 
	           "processDoneCode: Procproc is not running. Wexp Process not done for FID %lu, CT: %lu\n",
		       pFidStatBlk->elemId,pFidStatBlk->ct);
	        }
	      }
              return_status = EXP_HALTED;
	      /* return(EXP_HALTED); */
	      break;

	   case EXP_ABORTED:
               DPRINT(1,"processDoneCode: EXP_ABORTED\n");
       		{
		   DPRINT(1,"processDoneCode: Werr Processing\n");
                   /* Set this now, since go may check if acquisition is active */
                   setStatExpName("");
                   setStatGoFlag(-1);
	           /* wait because we guarantee this processing tobe done*/
		   procQadd(WERR, ShrExpInfo->MemPath, pExpInfo->Celem, 
				pExpInfo->CurrentTran, pFidStatBlk->doneCode, pFidStatBlk->errorCode);
		   if ((stat = sendMsgQ(pProcMsgQ,"chkQ",strlen("chkQ"),
				     MSGQ_NORMAL,WAIT_FOREVER)) != 0)
		   {
		     errLogRet(ErrLogOp,debugInfo, 
	    	     "Process: Procproc is not running. Error Process not done for FID %lu, CT: %lu\n",
			pFidStatBlk->elemId,pFidStatBlk->ct);
		   }
      		}
                return_status = EXP_HALTED;
		/* return(EXP_ABORTED);	/* result of ABORT Experiment */
		break;

	   case HARD_ERROR:
               DPRINT1(1,"processDoneCode: HARD_ERROR, errorcode: %d\n",pFidStatBlk->errorCode);
       		{
                   /* set harderror flag so no other thread will process */
                   HardErrorFlag = 1;

		   DPRINT(1,"processDoneCode: Werr Processing\n");
                   /* Set this now, since go may check if acquisition is active */
                   setStatExpName("");
                   setStatGoFlag(-1);
	           /* wait because we guarantee this processing tobe done*/
		   procQadd(WERR, ShrExpInfo->MemPath, pExpInfo->Celem, 
				pExpInfo->CurrentTran, pFidStatBlk->doneCode, pFidStatBlk->errorCode);
		   if ((stat = sendMsgQ(pProcMsgQ,"chkQ",strlen("chkQ"),
				     MSGQ_NORMAL,WAIT_FOREVER)) != 0)
		   {
		     errLogRet(ErrLogOp,debugInfo, 
	    	     "Process: Procproc is not running. Error Process not done for FID %lu, CT: %lu\n",
			pFidStatBlk->elemId,pFidStatBlk->ct);
		   }
      		}
                return_status = EXP_HALTED;
		/* return(EXP_HALTED);	/* result of HARD_ERROR. RPNZ */
		break;

	   case SETUP_CMPLT:
                DPRINT(1,"processDoneCode: SETUP_CMPLT\n");
       		{
	           procQadd(WEXP, ShrExpInfo->MemPath, pExpInfo->Celem, 
		      pExpInfo->CurrentTran, pFidStatBlk->doneCode, pFidStatBlk->errorCode);
		   if ((stat = sendMsgQ(pProcMsgQ,"chkQ",strlen("chkQ"),
				     MSGQ_NORMAL,WAIT_FOREVER)) != 0)
		   {
		     errLogRet(ErrLogOp,debugInfo, 
	    	     "Process: Procproc not running. Wexp Process not done for SU\n");
		   }
      		}

                return_status = SETUP_CMPLT;
		/* return(EXP_CMPLT); */
		break;

	   case EXP_FID_CMPLT:
                {
                   long fidNum;
                   /* Wexp test */
                   /* fidNum = ((pFidStatBlk->elemId - 1) / expInfo->NumFids) + 1; */
                   /* pFidStatBlk->elemId has ben changed to the true elemId */
                   /* maybe should use  maxFidsRecv instead of pFidStatBlk->elemId */
                   if ( pFidStatBlk->elemId == expInfo->ArrayDim)
                   /* if ( fidNum == expInfo->ArrayDim) */
                   {
                     /* Set this now, since go may check to see is acquisition is active */
                     setStatExpName("");
                     setStatGoFlag(-1);
	             /* msgQ full, wait because we guarantee this processing to be done */
                     if (expInfo->ProcWait > 0)   /* au(wait) or just au */
                     {
                        DPRINT2(1,"wExp(wait): Queue FID: %ld, CT: %ld\n",
		                pFidStatBlk->elemId, pFidStatBlk->ct);
	                procQadd(WEXP_WAIT, ShrExpInfo->MemPath, pFidStatBlk->elemId,
		                 pFidStatBlk->ct, EXP_COMPLETE, pFidStatBlk->errorCode);
                     }
                     else
                     {
                        DPRINT2(1,"wExp: Queue FID: %ld, CT: %ld\n",
		                pFidStatBlk->elemId, pFidStatBlk->ct);
	                procQadd(WEXP, ShrExpInfo->MemPath, pFidStatBlk->elemId,
		                 pFidStatBlk->ct, EXP_COMPLETE, pFidStatBlk->errorCode);
                     }
	             if ((stat = sendMsgQ(pProcMsgQ,"chkQ",strlen("chkQ"),MSGQ_NORMAL,NO_WAIT)) != 0)
	             {
       	                errLogRet(ErrLogOp,debugInfo, 
	                "Process: Procproc is not running. Wexp Process not done for FID %lu, CT: %lu\n",
		            pFidStatBlk->elemId,pFidStatBlk->ct);
                        return(-1);
	             }
                     return_status = EXP_FID_CMPLT;
                     /* return(0); */
                  }
                  else   /* Wnt */
                  {

	             DPRINT(-5,"SHould Never happen !!! Wnt Processing\n");
#ifdef XXXXX
	             DPRINT(1,"Wnt Processing\n");
	             /* msgQ full, just come back, we can skip Wnt processing */
	             qstat = procQadd(WFID, ShrExpInfo->MemPath, pFidStatBlk->elemId, pFidStatBlk->ct,
                                      pFidStatBlk->doneCode, pFidStatBlk->errorCode);
                     if (qstat != SKIPPED)
                     {
                        DPRINT2(1,"wNT: Queue FID: %ld, CT: %ld\n",
		            pFidStatBlk->elemId, pFidStatBlk->ct);
	                if ((stat = sendMsgQ(pProcMsgQ,"chkQ",strlen("chkQ"),MSGQ_NORMAL,NO_WAIT)) != 0)
	                {
       	                   errLogRet(ErrLogOp,debugInfo, 
	                   "Process: Procproc is not running. Wnt Process not done for FID %lu, CT: %lu\n",
		               pFidStatBlk->elemId,pFidStatBlk->ct);
                           return(-1);
	                }
  	             }
                   return_status = 0;   /* NT_CMPLT */
                   /* return(0); */
#endif
                  }
                }
                break;

#ifdef XXXXXX
	   case BS_CMPLT:
                if (expInfo->NumInBS != 0)
                {

	           DPRINT(1,"Wbs Processing\n");
	           /* msgQ full, just come back, we can skip Wbs processing */
	           qstat = procQadd(WBS, ShrExpInfo->MemPath, pFidStatBlk->elemId, pFidStatBlk->ct,
                                    pFidStatBlk->doneCode, pFidStatBlk->ct / expInfo->NumInBS);
                   if (qstat != SKIPPED)
                   {
                      DPRINT2(1,"wBS: Queue FID: %ld, CT: %ld\n",
		        pFidStatBlk->elemId, pFidStatBlk->ct);
	              if ((stat = sendMsgQ(pProcMsgQ,"chkQ",strlen("chkQ"),MSGQ_NORMAL,NO_WAIT)) != 0)
	              {
       	               errLogRet(ErrLogOp,debugInfo, 
	               "Process: Procproc is not running. Wbs Process not done for FID %lu, CT: %lu\n",
		           pFidStatBlk->elemId,pFidStatBlk->ct);
                       return(-1);
	              }
  	           }
                 return_status = BS_CMPLT;
		}
		break;
#endif

	  default:
                DPRINT1(-5,"default: DoneCode: %d  not handled here....\n",(pFidStatBlk->doneCode & 0xFFFF));
                return_status = EXP_STOPPED;
		/* return(EXP_STOPPED); */
		break;
         }

      	/* --- Update Current CT & Current Element ----- */
      	/* pExpInfo->CurrentTran = pFidStatBlk->ct;	/* Current Transient */
      	/* pExpInfo->Celem = pFidStatBlk->elemId;       /* Current Element */

        return(return_status);

}

/*--------------------------------------------------------------------
| getUserUid()
|       get the user's  uid & gid outof the passwd file
+-------------------------------------------------------------------*/
int getUserUid(char *user, int *uid, int *gid)
{
    struct passwd *pswdptr;

    if ( (pswdptr = getpwnam(user)) == ((struct passwd *) NULL) )
    {
        *uid = *gid = -1;
        DPRINT1(1,"getUserUid: user: '%s' not found\n", user);
        return(-1);
    }   
    *uid = pswdptr->pw_uid;
    *gid = pswdptr->pw_gid;
    DPRINT3(1,"getUserUid: user: '%s', uid = %d, gid = %d\n", user,*uid,*gid);
    return(0);
}


/*##################################################################*/

/*  These programs deal with Interactive data.  The goal is to
    ultimately put them in a separate file.			*/


static char interactInfoFile[512] = { '\0' };


int startInteract()
{
	char	*pInfoFile;
	int	 uid,gid;

	if ( ! consoleConn() ) {
		errLogRet(ErrLogOp,debugInfo,
	   "start interact: Channel not connected to console yet, start request ignored.");
		return(-1);
	}

	pExpMsgQ = openMsgQ("Expproc");  /* pExpMsgQ is a static defined at the top */
	if (pExpMsgQ == NULL) {
		errLogRet(ErrLogOp,debugInfo,
	   "start interact: Channel not connected to console yet, start request ignored.");
		return(-1);
	}

	pInfoFile = strtok(NULL," ");
	strcpy(interactInfoFile,pInfoFile);

    DPRINT1( 1, "start interactive called in recvproc with info file %s\n", interactInfoFile );

	if (mapIn( interactInfoFile ) == -1) {
		errLogRet(ErrLogOp,debugInfo,
	   "start interact: Exp Info Not Present, start request Ignored.");
		return(-1);
	}

  /*DPRINT3(1,
    "Expecting: NP: %ld, Data File Size: %ld, put into file %s\n",
     expInfo->NumDataPts,expInfo->DataSize, expInfo->DataFile);*/

	ifile = mOpen(expInfo->DataFile,expInfo->DataSize,O_RDWR | O_CREAT );
	if (ifile == NULL) {
		errLogSysRet(ErrLogOp,debugInfo,
	   "start interact: could not open %s",expInfo->DataFile);
		resetState();
		return(-1);
	}

   DPRINT1(1,"offset into mmap file is 0x%x\n", ifile->offsetAddr );

   /* get the UID & GID of exp. owner */
   if (	getUserUid(expInfo->UserName,&uid,&gid) == 0) 
   {
        int old_euid = geteuid();

        seteuid( getuid() );
	chown(expInfo->DataFile, uid, gid);
        seteuid( old_euid );
   }
   else
   {
	errLogRet(ErrLogOp,debugInfo,
	   "startInteract: Unable to change '%s' uid for User: '%s'\n",
		expInfo->DataFile,expInfo->UserName);
   }
   DPRINT( 1, "calling recvInteract\n" );
   recvInteract();
   DPRINT( 1, "start interactive completes in recvproc\n" );
   stopInteract();
   return(0);
}

int stopInteract()
{
		DPRINT( 1, "stop interactive starts in recvproc\n" );
	if ( ! consoleConn() ) {
		errLogRet(ErrLogOp,debugInfo,
	   "stop interact: Channel not connected to console yet, stop request Ignored.");
		return(-1);
	}

	if (mapOut( interactInfoFile ) == -1) {
		errLogRet(ErrLogOp,debugInfo,
	   "stop interact: Exp Info Not Present, stop request Ignored.");
		return(-1);
	}

	mClose(ifile);
	ifile = NULL;

	rmAcqiFiles( expInfo );
	unlink( interactInfoFile );

	closeMsgQ(pExpMsgQ);
	pExpMsgQ = NULL;

	DPRINT( 1, "stop interactive completes in recvproc\n" );
        return( 0 );
}

/**********************************************************
* rmAcqiFiles - removes ACQI files 
*
*/
void rmAcqiFiles( SHR_EXP_INFO ExpInfo )
{
	if (ExpInfo == NULL)
	  return;

	if ((int) strlen( ExpInfo->DataFile ) > 1)
	  unlink( ExpInfo->DataFile );
}

int recvInteract()
{
     errLogRet(ErrLogOp,debugInfo,"recvInteract: NOT Implemented for NIRVANA \n" );
      return(-1);
}


/* calc Total RAM to be allowed to used based on Ram of system and number of Active DDRs */
long long calcMaxMemory4BuffersMB(int numActiveDDrs)
{
    long long alottedMemory;
    long long desiredMemory;
    long long allowedMemory;
     
    /* calc Total RAM to be allowed to used based on Ram of system and number of Active DDRs */
    /*
     * a rough estimate which is fairly implerical, is about 32 MB per DDRs
     * take this 32 MB times the number of DDRs for a total optimum size
     * then limit it based on total RAM in system
     *
     * this breaks down with imaging exp, with large nf and nfmod=1
     * i.e a single FID can be greater than 32 MB
     */ 
     /* desiredMemory = 32LL * 1048576LL * (long long) numActiveDDrs; */

     allowedMemory = alottedMemory = systemTotalRAM / 4LL;  /* was 8th now 4th of total memory */
     
     /*
     * if (desiredMemory <= alottedMemory)
     *    allowedMemory = desiredMemory;
     * else
     *    allowedMemory = alottedMemory;
     */    
     /* systemFreeRAM; */
     
      return( allowedMemory );
}
