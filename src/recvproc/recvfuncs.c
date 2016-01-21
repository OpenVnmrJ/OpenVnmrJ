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
#include <netinet/in.h>
#include <time.h>

#include <errno.h>

#include "errLogLib.h"
#include "mfileObj.h"
#include "shrMLib.h"
#include "shrexpinfo.h"
#include "shrstatinfo.h"
#include "chanLib.h"
#include "hostAcqStructs.h"
#include "expDoneCodes.h"
#include "msgQLib.h"
#include "hostMsgChannels.h"
#include "expQfuncs.h"
#include "procQfuncs.h"
#include "data.h"
#include "dspfuncs.h"


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

static SHR_MEM_ID  ShrExpInfo = NULL;  /* Shared Memory Object */

/* Vnmr Data File Header & Block Header */
static struct datafilehead fidfileheader;
static struct datablockhead fidblockheader;
static struct datafilehead acqfileheader;
static struct datablockhead acqblockheader;

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
struct recvProcSwapbyte
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

static int dspflag = 0;
static int bbytes = 0;

void UpdateStatus(FID_STAT_BLOCK *);
void rmAcqiFiles(SHR_EXP_INFO);

static void sleepMilliSeconds(int msecs)
{
   struct timespec req;
   req.tv_sec = msecs / 1000;
   req.tv_nsec = (msecs % 1000) * 1000000;
#ifdef __INTERIX
         usleep(req.tv_nsec/1000);
#else
         nanosleep( &req, NULL);
#endif
}

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

/**************************************************************
*
*  recvData -  get ready for Exp data
*		Shared Status Struct is already mapped in
*		 we assume Expproc has update who,what
*	         
*  1. mapin Exp Info Dbm 
*  2. Open the Data File.
*  3. Initialize the Data File & Block Headers.
*  4. Upload Exp. Data
*  4a. Update Exp Status (Data Written Time, FID# & CT)
*  5. Close Data on Exp. Completion
*  6. mapout Exp Info Dbm
*  7. Return 
*  If any Error Occurs during transfer, Send Msg to Expproc.
*  Expproc will take the proper action.
*
* RETURNS:
* 0 , else -1
*
*       Author Greg Brissey 8/12/94
*/
int recvData(char *argstr)
{

  int uid,gid;
  int datastat,stat;
  char fidpath[512];
  char expCmpCmd[256];
  char *pInfoFile;
  void resetState();

  if ( ! consoleConn() )
  {
     errLogRet(ErrLogOp,debugInfo,
	"recvData: Channel not connected to console yet, Transfer request Ignored.");
     return(-1);
  }

  pProcMsgQ = openMsgQ("Procproc");
  pExpMsgQ = openMsgQ("Expproc");
/*
  if (pProcMsgQ == NULL)
      return(-1);
*/
     
  pInfoFile = strtok(NULL," ");
  strcpy(expInfoFile,pInfoFile);

  DPRINT1(1,"\n--------------------\nPreparing for: '%s' data.\n",pInfoFile);
  /*
      1st mapin the Exp Info Dbm 
  */
  if ( mapIn(expInfoFile) == -1)
  {
     errLogRet(ErrLogOp,debugInfo,
	"recvData: Exp Info File %s Not Present, Transfer request Ignored.", expInfoFile);
     return(-1);
  }
  DPRINT5(1,"Expecting: %ld FIDs, %ld NT, %d GoFlag, %ld NP, %ld - Data File Size\n",
	expInfo->ArrayDim,expInfo->NumTrans,expInfo->GoFlag,
	expInfo->NumDataPts,expInfo->DataSize);

  /*
      2nd open up the Data file to the proper size and changed the
      file owner & group to the Experiment submitter.
  */
  /* DPRINT1(1,"getExpData: uplink data into: '%s'\n",expInfo->DataFile); */
  /* DPRINT1(1,"recvData;  expInfo->GoFlag = %d\n",expInfo->GoFlag); */
  if (! expInfo->GoFlag)
  {
     sprintf(fidpath,"%s/fid",expInfo->DataFile);
     DPRINT1(1,"getExpData: uplink data into: '%s'\n",fidpath);

     /* Forget Malloc and Read, we'll use the POWER of MMAP */
     if ( !(expInfo->ExpFlags & RESUME_ACQ_BIT))
     {
       /* open new data file */
       ifile = mOpen(fidpath,expInfo->DataSize,O_RDWR | O_CREAT | O_TRUNC);
     }
     else
     { 
       /* open existing data file SA/RA */
       ifile = mOpen(fidpath,expInfo->DataSize,O_RDWR | O_CREAT);
     }

     if (ifile == NULL)
     {
       errLogSysRet(ErrLogOp,debugInfo,"recvData: could not open %s",fidpath);
       resetState();
       return(-1);
     }

     /* get the UID & GID of exp. owner */
     if (	getUserUid(expInfo->UserName,&uid,&gid) == 0) 
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

     /* Check for In-Line DSP */
     if (expInfo->DspOversamp > 1)
     {
	dspflag = 1;
     }
     else
     {
	dspflag = 0;
     }

     /* ----------------------------------------------------------- */

     if ( !(expInfo->ExpFlags & RESUME_ACQ_BIT)) 
     {
       DPRINT(1,"recvData: RESUME bit NOT set, InitialFileHeaders() called\n");
       InitialFileHeaders(); /* set the default values for the file & block headers */

       /* write fileheader to datafile (via mmap) */
       memcpy((void*) ifile->offsetAddr,
		(void*) &fidfileheader,sizeof(fidfileheader));
       ifile->offsetAddr += sizeof(fidfileheader);  /* move my file pointers */
       ifile->newByteLen += sizeof(fidfileheader);
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

         /* Set New size to it's existing size, Sure wouldn't what it to get smaller! */
         ifile->newByteLen = ifile->byteLen;

         /* read in file header */
         memcpy((void*) &fidfileheader, (void*) ifile->offsetAddr,sizeof(fidfileheader));

	 /* Update Relevant Info */
         fidfileheader.nblocks   = htonl(expInfo->ArrayDim); /* n fids*/

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

     /* returns EXP_CMPLT, EXP_HALTED, EXP_STOPPED, EXP_ABORTED */
     datastat = uploadData(ifile,expInfo);
     DPRINT1(2,"uploadData return: %d\n",datastat);

     mClose(ifile);
     ifile = NULL;
  }
  else
  {
     ifile = NULL;
     datastat = uploadData(ifile,expInfo);
  }

  closeMsgQ(pProcMsgQ);
  pProcMsgQ = NULL;

  /* if EXP_HALTED, EXP_STOPPED, or EXP_ABORTED, HARD_ERROR
	Expproc will be told via Expproc channel
     so no need to tell Expproc from Here */

  /* if EXP_CMPLT, then acquisition completed tell Expproc */
  if (datastat == EXP_CMPLT)
  {
    /* Tell Expproc that Experiment is completed Acquiring Data */
    sprintf(expCmpCmd,"expdone %s",ShrExpInfo->MemPath);
    if ((stat = sendMsgQ(pExpMsgQ,expCmpCmd,strlen(expCmpCmd),MSGQ_NORMAL,
				WAIT_FOREVER)) != 0)
    {
      errLogRet(ErrLogOp,debugInfo, 
      "getExpData: Expproc is not running. Exp. Done not received: '%s'\n",
            ShrExpInfo->MemPath);
    }
  }

  closeMsgQ(pExpMsgQ);
  pExpMsgQ = NULL;

  activeExpQdelete(expInfoFile);

  if ( mapOut(expInfoFile) == -1)
  {
     errLogRet(ErrLogOp,debugInfo,"getExpData: mapOut failed\n");
     return(-1);
  }
  expInfoFile[0] = '\0';

  return(0);
}   

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

/*--------------------------------------------------------------------*/
/*   eos,ct,scan; eob,bs; eof,nt,fid; eoc,il;  # - eos at modulo #    */
/*--------------------------------------------------------------------*/
#define STOP_EOS        11/* sa at end of scan */
#define STOP_EOB        12/* sa at end of bs */
#define STOP_EOF        13/* sa at end of fid */
#define STOP_EOC        14/* sa at end of interleave cycle */
/*--------------------------------------------------------------------*/

/**************************************************************
*
*  uploadData - Perform actual task of getting data 
*
*  1. Wait on the channel for incoming Data
*  2. 1st transfer of FID is a info block. (Status, np, strtct, ct, elemid, etc)
*  3. Series of transfers at some buffering size.
*  4. Fid transfer complete, determine if any processing is required
*  5. Send Msgq to Procproc for any proccessing to be done
*  6. Return 
*  If any Error Occurs during transfer, Send Msg to Expproc.
*  Expproc will take the proper action.
*
* RETURNS:
* 0 , else -1
*
*       Author Greg Brissey 8/12/94
*/
int uploadData(MFILE_ID datafile, SHR_EXP_INFO expInfo)
{
   char *fidblkhdrSpot;
   char *dataPtr;
   char *blksizeData;
   char *discardData;
   unsigned long xfrSize,bytes2xfr;
   unsigned long maxFidsRecv;
   unsigned long fidSize,correctedNP,correctedFidSize;
   int done,bytes,sync,stat;
   FID_STAT_BLOCK fidstatblk;
   char acqMsg[ACQ_UPLINK_XFR_MSG_SIZE+5];
   int enddata_flag;
   long noisedata[256];
   int iRcvr;
   int nRcvrs;
   int uplink_flag;
   int noisesize = 0;
   struct {
       double r;
       double i;
   } dcLevel[MAX_STM_OBJECTS];
/*   long *noisedata; */

   DPRINT1(1,"uploadData: datafile: 0x%lx \n",datafile);
   DPRINT2(1,"uploadData: expInfo->IlFlag = %d RAFlag = %d\n",
				expInfo->IlFlag,expInfo->RAFlag);

   for (iRcvr=0; iRcvr<MAX_STM_OBJECTS; iRcvr++){
       dcLevel[iRcvr].r = dcLevel[iRcvr].i = 0;
   }

   /* determine fid size which will differ between  inline DSP & normal operation */
   if (expInfo->DspOversamp > 1)
   {
       DPRINT(1,"initDSP: --------------------------------\n");
       initDSP(expInfo); /* initial dsp structs and read in coeff */

       fidSize = ((expInfo->NumDataPts * expInfo->DspOversamp) + (expInfo->DspOsCoef - 1)) * 
		   expInfo->DataPtSize;

       noisesize = ((256 * expInfo->DspOversamp) + (expInfo->DspOsCoef - 1)) * 
		   expInfo->DataPtSize;

       /* noisedata = (long *) malloc(((noisesize*sizeof(long))) + 10); */
       DPRINT2(1,"FID Size: %lu bytes, Noise Size: %ld bytes\n",fidSize,noisesize);
   }
   else
   {
       fidSize = expInfo->FidSize;
       /* noisedata = (long *) malloc((256*sizeof(long))); */
   }
   DPRINT3(1,"uploadData: OverSample: %d, local FID buffer size: %d, Actual FID Size: %d\n",
	expInfo->DspOversamp,fidSize,expInfo->FidSize);
   DPRINT1(1,"uploadData: number of FIDS: %d\n", expInfo->NumFids );

   /* interleave Exp or ra we must do the summing with buffer, inline DSP need buffer */
   /* to hold oversampled data */
   if ( (expInfo->IlFlag != 0) || (expInfo->RAFlag != 0) || 
	(expInfo->DspOversamp > 1) )
   {
       /* Just incase np is less than noise np of 256 */
       if (fidSize > noisesize)
          blksizeData = (char*) malloc((fidSize) + 10);
       else
	  blksizeData = (char*) malloc((noisesize) + 10);

       DPRINT2(1,"uploadData: malloc for Il-BS, RA or DSP: %lu bytes, addr: 0x%lx\n",
		  fidSize,blksizeData);
       if (blksizeData == NULL)
       {
  	/* don't know how to recover, so terminate process */
      	mClose(datafile);
      	errLogQuit(ErrLogOp,debugInfo,
			"uploadData: Failed to get blksizeData\n");
       }
   }
   else
   {
       blksizeData = NULL;
   }

   /* malloc xfersize buffer to discard data for ct < 0.  SA 	*/
   /* during steady state.					*/
   discardData = (char*) malloc(XFR_SIZE);
   if (discardData == NULL)
   {
      /* don't know how to recover, so terminate process */
      mClose(datafile);
      errLogQuit(ErrLogOp,debugInfo,"uploadData: Failed to get discardData\n");
   }

   done = enddata_flag = 0;
   maxFidsRecv = 0L;
   nRcvrs = 1;			/* Default number of rcvrs */
   iRcvr = -1;
   while( !done )
   {
      /* keep reading channel till uplink msg is obtain then proceed */
      /* This action is an attempt to resync channels incase there are */
      /* problems on the channel, garbled, wrong # bytes, etc.  */
      uplink_flag = sync = 0;
      /* DPRINT(-1,"At The Beginning\n"); */
      while( !sync )
      {
         DPRINT(1,"uploadData: Waiting for Acq UPLINK Msg\n");
         blockAllEvents();
         bytes = readChannel(chanId,acqMsg,ACQ_UPLINK_XFR_MSG_SIZE);
         unblockAllEvents();
         acqMsg[ACQ_UPLINK_XFR_MSG_SIZE] = '\0';
         DPRINT1(1,"uploadData: Acq UPLINK Msg: '%s'\n",acqMsg);
	 if (strncmp(acqMsg,ACQ_UPLINK_MSG,strlen(ACQ_UPLINK_MSG)) == 0)
         {
	    uplink_flag = sync = 1;
         }
	 else
         {
	    /* test if for some reason we are to stop data transfer */
	    if (strncmp(acqMsg,ACQ_UPLINK_ENDDATA_MSG,strlen(ACQ_UPLINK_MSG)) == 0)
    	    {
		enddata_flag = 1;
	        sync = 1;
	    }
	 }
      }
      blockAllEvents();
      bytes = readChannel(chanId,(char*)&fidstatblk,sizeof(fidstatblk));
      unblockAllEvents();
      if (bytes < sizeof(fidstatblk))
      {
	/* don't know how to recover, so terminate process */
	mClose(datafile);
	errLogQuit(ErrLogOp,debugInfo,"uploadData: Failed to get fidstatblk\n");
      }
#ifdef LINUX
      FSB_CONVERT( fidstatblk );
#endif

      if (uplink_flag) {
	  /* Increment rcvr with each UPLINK msg (but see IF block below) */
	  iRcvr++;
      }
      if (fidstatblk.ct > 0 && fidstatblk.elemId > 0) {
	   /* Not a noise FID; cycle rcvr number and fix elemId number */
	  if (iRcvr >= nRcvrs) { iRcvr = 0; }
	  fidstatblk.elemId = ((fidstatblk.elemId - 1) * nRcvrs + iRcvr + 1);
      }

      DPRINT5(1,">>>>>>> FIDstat Read  Fid StatBlk: Fid: %ld, DataSize: %ld, CT: %ld, NP: %ld,\n                                      State: %d\n",
		fidstatblk.elemId, fidstatblk.dataSize, fidstatblk.ct,
		fidstatblk.np, fidstatblk.doneCode);

      DPRINT3(1,"Statblock: elemId=%d, iRcvr=%d, nRcvrs=%d\n",
	      fidstatblk.elemId, iRcvr, nRcvrs);
      /*
          Correct FidSize & NP return from console to reflect decimated sizes
	  Further done we'll equate fidstatblk.np & fidstatblk.dataSize to them
      */
      /* DPRINT(-1,">>>>>>>  AT  'AA' \n"); */
      if (expInfo->DspOversamp > 1)
      {
	   correctedNP = (fidstatblk.np - (expInfo->DspOsCoef - 1))/expInfo->DspOversamp;
	   correctedFidSize = correctedNP * expInfo->DataPtSize;
           DPRINT4(1,"Fid StatBlk: OverSample: Factor %d, Coef %d;  Corrected:  DataSize %lu, NP %lu\n",
		expInfo->DspOversamp, expInfo->DspOsCoef, correctedFidSize, correctedNP);
      }
      /* DPRINT(-1,">>>>>>>  AT  'A' \n"); */

      if ( fidstatblk.doneCode == WARNING_MSG )
      {
	   DPRINT(1,"uploadData: WARNING_MSG Werr Processing\n");
	   /* wait because we guarantee this processing tobe done*/
	   procQadd(WERR, ShrExpInfo->MemPath, fidstatblk.elemId, 
			/*  fidstatblk.ct */ 1L, fidstatblk.doneCode, fidstatblk.errorCode);
	   if ((stat = sendMsgQ(pProcMsgQ,"chkQ",strlen("chkQ"),
			     MSGQ_NORMAL,WAIT_FOREVER)) != 0)
	   {
	     errLogRet(ErrLogOp,debugInfo, 
	   	     "Process: Procproc is not running. Error Process not done for FID %lu, CT: %lu\n",
		fidstatblk.elemId,fidstatblk.ct);
	   }
	   enddata_flag = 0;	/* don't stop just a warning */
	   DPRINT(1,"WARNING_MSG: continue\n");
           continue;
      }

      /* DPRINT(-1,">>>>>>>  AT  'B' \n"); */


      /*********************************************************************
       * If data transfer stopped permaturely use fidstatblk to find out why
       * then do the right thing.
       */
      if (enddata_flag)
      {
      /* DPRINT(-1,">>>>>>>  AT  'C' \n"); */
	DPRINT1(1,"---------  Enddata Flag SET ------ DoneCode: %d\n",fidstatblk.doneCode);
         /* 1st update file header to proper number of FIDs present in data */
         if (! expInfo->GoFlag)
         {
            struct datafilehead *pdatafilehead;
	    if ( mFidSeek(datafile, 1L, sizeof(struct datafilehead), bbytes ) == -1)
	    {
    
    	       /* don't know how to recover, so terminate process */
	       mClose(datafile);
	       errLogQuit(ErrLogOp,debugInfo,"uploadData: mFidSeek failed.\n");
	    }
            pdatafilehead = (struct datafilehead *) datafile->mapStrtAddr;
	    pdatafilehead->nblocks = htonl(maxFidsRecv);
         }

	 /* Always free discard buffer */
	 free(discardData);
         discardData = NULL;

         /* incase we are Interleaving, RA, or inline DSP  need to free this buffer */
         if (blksizeData != NULL)
         {
	    DPRINT1(1,"Free Il BS: 0x%lx\n",blksizeData);
            free(blksizeData);
            blksizeData = NULL;
         }

	 switch(fidstatblk.doneCode)
         {
	   case STOP_CMPLT:		/* result of SA, no process */
	       {
		  int SA_Type;
                DPRINT(1,"uploadData: STOP_CMPLT\n");
		/* A Halt is equivilent to a AA but Wexp proc not Werr */
	        /* full, wait because we guarantee this processing tobe done */
                /* use expInfo->Celem & expInfo->CurrentTran, since they reflect last
		   data transfer prior to this stop message
  	 	*/
		SA_Type = STOP_EOF;
 		switch(fidstatblk.errorCode)
                {
		  case EXP_FID_CMPLT:
			SA_Type = STOP_EOF;
			break;
		  case BS_CMPLT:
			SA_Type = STOP_EOB;
			break;
		  case CT_CMPLT:
			SA_Type = STOP_EOS;
			break;
		  case IL_CMPLT:
			SA_Type = STOP_EOC;
			break;
                }
                if (expInfo->ProcWait > 0)   /* au(wait) or just au */
                {
                   DPRINT2(1,
		     "uploadData: wExp(wait): Queue FID: %ld, CT: %ld\n",
		          fidstatblk.elemId, fidstatblk.ct);
	           procQadd(WEXP_WAIT, ShrExpInfo->MemPath, expInfo->Celem, 
		      expInfo->CurrentTran, fidstatblk.doneCode, SA_Type);
                }
                else
                {
                   DPRINT2(1,"uploadData wExp: Queue FID: %ld, CT: %ld\n",
		       fidstatblk.elemId, fidstatblk.ct);
	           procQadd(WEXP, ShrExpInfo->MemPath, expInfo->Celem, 
		      expInfo->CurrentTran, fidstatblk.doneCode, SA_Type);
                }
	        if ((stat = sendMsgQ(pProcMsgQ,"chkQ",strlen("chkQ"),
				MSGQ_NORMAL,WAIT_FOREVER)) != 0)
	        {
       	           errLogRet(ErrLogOp,debugInfo, 
	           "uploadData: Procproc is not running. Wexp Process not done for FID %lu, CT: %lu\n",
		       fidstatblk.elemId,fidstatblk.ct);
	        }
		return(EXP_STOPPED);
               }
		break;

	   case EXP_HALTED:		/* result of Halt, do Wexp processing */
               DPRINT(1,"uploadData: EXP_HALTED\n");
              {
		/* A Halt is equivilent to a AA but Wexp proc not Werr */
                /* use expInfo->Celem & expInfo->CurrentTran, since they reflect last
		   data transfer prior to this stop message
  	 	*/
		fidstatblk.doneCode = EXP_ABORTED;
	        /* full, wait because we guarantee this processing tobe done */
                if (expInfo->ProcWait > 0)   /* au(wait) or just au */
                {
                   DPRINT2(1,
		     "uploadData: wExp(wait): Queue FID: %ld, CT: %ld\n",
		          fidstatblk.elemId, fidstatblk.ct);
	           procQadd(WEXP_WAIT, ShrExpInfo->MemPath, expInfo->Celem, 
		      expInfo->CurrentTran, fidstatblk.doneCode, fidstatblk.errorCode);
                }
                else
                {
                   DPRINT2(1,"uploadData wExp: Queue FID: %ld, CT: %ld\n",
		       fidstatblk.elemId, fidstatblk.ct);
	           procQadd(WEXP, ShrExpInfo->MemPath, expInfo->Celem, 
		      expInfo->CurrentTran, fidstatblk.doneCode, fidstatblk.errorCode);
                }
	        if ((stat = sendMsgQ(pProcMsgQ,"chkQ",strlen("chkQ"),
				MSGQ_NORMAL,WAIT_FOREVER)) != 0)
	        {
       	           errLogRet(ErrLogOp,debugInfo, 
	           "uploadData: Procproc is not running. Wexp Process not done for FID %lu, CT: %lu\n",
		       fidstatblk.elemId,fidstatblk.ct);
	        }
	      }
	      return(EXP_HALTED);
	      break;

	   case EXP_ABORTED:
               DPRINT(1,"uploadData: EXP_ABORTED\n");
       		{
		   DPRINT(1,"uploadData: Werr Processing\n");
	           /* wait because we guarantee this processing tobe done*/
		   procQadd(WERR, ShrExpInfo->MemPath, expInfo->Celem, 
				expInfo->CurrentTran, fidstatblk.doneCode, fidstatblk.errorCode);
		   if ((stat = sendMsgQ(pProcMsgQ,"chkQ",strlen("chkQ"),
				     MSGQ_NORMAL,WAIT_FOREVER)) != 0)
		   {
		     errLogRet(ErrLogOp,debugInfo, 
	    	     "Process: Procproc is not running. Error Process not done for FID %lu, CT: %lu\n",
			fidstatblk.elemId,fidstatblk.ct);
		   }
      		}
		return(EXP_ABORTED);	/* result of ABORT Experiment */
		break;

	   case HARD_ERROR:
               DPRINT(1,"uploadData: HARD_ERROR\n");
       		{
		   DPRINT(1,"uploadData: Werr Processing\n");
	           /* wait because we guarantee this processing tobe done*/
		   procQadd(WERR, ShrExpInfo->MemPath, expInfo->Celem, 
				expInfo->CurrentTran, fidstatblk.doneCode, fidstatblk.errorCode);
		   if ((stat = sendMsgQ(pProcMsgQ,"chkQ",strlen("chkQ"),
				     MSGQ_NORMAL,WAIT_FOREVER)) != 0)
		   {
		     errLogRet(ErrLogOp,debugInfo, 
	    	     "Process: Procproc is not running. Error Process not done for FID %lu, CT: %lu\n",
			fidstatblk.elemId,fidstatblk.ct);
		   }
      		}
		return(EXP_HALTED);	/* result of HARD_ERROR. RPNZ */
		break;

	   case SETUP_CMPLT:
               DPRINT(1,"uploadData: SETUP_CMPLT\n");
       		{
	           procQadd(WEXP, ShrExpInfo->MemPath, expInfo->Celem, 
		      expInfo->CurrentTran, fidstatblk.doneCode, fidstatblk.errorCode);
		   if ((stat = sendMsgQ(pProcMsgQ,"chkQ",strlen("chkQ"),
				     MSGQ_NORMAL,WAIT_FOREVER)) != 0)
		   {
		     errLogRet(ErrLogOp,debugInfo, 
	    	     "Process: Procproc not running. Wexp Process not done for SU\n");
		   }
      		}
		return(EXP_CMPLT);
		break;

	  default:
		return(EXP_STOPPED);
		break;
         }
      }
      /* DPRINT(-1,">>>>>>>  AT  'D' \n"); */
      if (datafile == NULL)
         return(EXP_CMPLT);
      /* DPRINT(-1,">>>>>>>  AT  'E' \n"); */

      if ( (fidstatblk.doneCode == WARNING_MSG) )
      {
          /* DPRINT(-1,"uploadData: It was an Warning skip getting the FID\n"); */
          continue;
      }

      /* if ct <= 0 it is a steady state fid and should be discarded */
      /* else if fid elemid == 0 then this is the NOISE fid */
      if ((long)fidstatblk.ct <= 0)	/* ss FID */
      {
	dataPtr = (char *)discardData;	
      }
      else if (fidstatblk.elemId == 0)	/* noise element */
      {
	/* added test for JPSG since noise is oversampled for dsp='i'*/
        if ((fidstatblk.np > 256) && (expInfo->DspOversamp > 1))
	  dataPtr = (char *)blksizeData; /* if np != 256 then noise has been oversampled */	
	else
	  dataPtr = (char *)noisedata;	
      }
      else 
      {
	if ( mFidSeek(datafile, fidstatblk.elemId, sizeof(struct datafilehead),
                      bbytes ) == -1)
	{

	   /* don't know how to recover, so terminate process */
	   mClose(datafile);
	   errLogQuit(ErrLogOp,debugInfo,"uploadData: mFidSeek failed.\n");
	}
	fidblkhdrSpot = datafile->offsetAddr;	/* save block header position */
        DPRINT2(2,"Header & Data Offset: 0x%lx, 0x%lx\n",
		   fidblkhdrSpot, datafile->offsetAddr + sizeof(fidblockheader));

 	/* Check for interleave Exp or first fid in ra. These we must do 	*/
	/* summing on the host. If DSP (inline) then need the buffer for the    */
        /* oversampled data  	 						*/
	if ( (expInfo->IlFlag != 0) || (expInfo->DspOversamp > 1) ||
	     ((expInfo->RAFlag != 0) && (fidstatblk.elemId == 
						(expInfo->CurrentElem+1))) )
	{
           dataPtr = blksizeData;
	}
	else
	{
           /* set to fid data position */;
           dataPtr = datafile->offsetAddr + sizeof(fidblockheader); 
	}
      }

      bytes2xfr = fidstatblk.dataSize;
      DPRINT(2,">>>>    Data Xfr:\n");
      while( bytes2xfr )
      {
          if (bytes2xfr < XFR_SIZE)
	     xfrSize = bytes2xfr;
          else
	     xfrSize = XFR_SIZE;

          DPRINT4(2,"Address: 0x%lx, Xfr Size: 0x%lx (%lu)  Left: %lu\n",
		dataPtr,xfrSize,xfrSize,bytes2xfr);
          blockAllEvents();
	  bytes = readChannel(chanId,dataPtr,xfrSize);
          unblockAllEvents();
          if (bytes < xfrSize)
          {
	    /* don't know how to recover, so terminate process */
	    mClose(datafile);
	    errLogQuit(ErrLogOp,debugInfo,"uploadData: Failed to get xfrSize.\n");
          }

	  if ((long)fidstatblk.ct > 0)   /* data will be discarded if ct <= 0 */
	    dataPtr += xfrSize;
	  bytes2xfr -= xfrSize;
      }

      if ((long)fidstatblk.ct <= 0)
      {
	DPRINT(1,"Do Nothing data is to be discarded\n");
	continue;
      }
      else if ( fidstatblk.elemId == 0)     /* Calculate noise values */
      {
	long *noisePtr;
	DPRINT(1,"A Noise FID do dclevel Calc\n");
	nRcvrs = iRcvr + 1;	/* Count up nbr of rcvrs being used */

	/* added test for JPSG since noise is oversampled for dsp='i'*/
        if ((fidstatblk.np > 256) && (expInfo->DspOversamp > 1))
	  noisePtr = (long *)blksizeData; /* if np != 256 then noise has been oversampled */	
	else
	  noisePtr = noisedata;	

	/* calcDCoffset(noisedata,fidstatblk.dataSize/expInfo->DataPtSize, */
	calcDCoffset(noisePtr,fidstatblk.dataSize/expInfo->DataPtSize,
		     &dcLevel[iRcvr].r,
		     &dcLevel[iRcvr].i,
		     expInfo->DataPtSize);
	DPRINT3(1,"dcLevel[%d] = (%g, %g)\n",
		iRcvr, dcLevel[iRcvr].r, dcLevel[iRcvr].i);
      }
      else
      {
        if (expInfo->DspOversamp > 1)
	{
	   dsp_dcset((float)dcLevel[iRcvr].r, (float)dcLevel[iRcvr].i); 
	   fidstatblk.np = correctedNP;
	   fidstatblk.dataSize = correctedFidSize;
	   DPRINT2(1,"Change fidstatblk np & dataSize to decimated values: np %lu, datasize: %lu\n",
		fidstatblk.np,fidstatblk.dataSize);
	}

        DPRINT1(1,"expInfo->IlFlag: %d\n",expInfo->IlFlag); 
        DPRINT2(1,"expInfo->RAFlag: %d  startelem: %d\n",expInfo->RAFlag,
						expInfo->CurrentElem); 
      	/* If interleave, or first fid of ra, or inline dsp; then we do the summing 	*/
	/* on the host.							*/
	if ( (expInfo->IlFlag != 0) ||
	     ((expInfo->RAFlag != 0) && (fidstatblk.elemId == 
						(expInfo->CurrentElem+1))) )
      	{
	   int stat,Ra_Ct_Zero,First_Bs,Il_First_Bs,Dsp_First_Bs;

           /* INLINE DSP */
           if (expInfo->DspOversamp > 1)
           {
	      DPRINT(1,"Il or RA dspExec Filter  -------------------\n");
#ifdef LINUX
              byteSwap(blksizeData, fidSize / expInfo->DataPtSize, (expInfo->DataPtSize == 2) );
#endif
              dspExec(blksizeData, blksizeData, expInfo->NumDataPts, fidSize, fidstatblk.ct);
           }

            /* if RA & CT == 0 then we want to memcpy not add */
            Ra_Ct_Zero = ((expInfo->RAFlag) && (expInfo->CurrentTran == 0));

            /* If BS==0 then memcpy (there are no BS) otherwise 
		if (ct/bs) <= 1 then memcpy (i.e. 1st BS) */
	    First_Bs =  (expInfo->NumInBS >= 1) ? (((long)fidstatblk.ct / expInfo->NumInBS) <= 1 ) : 1;


            /* if Interleave and elements 1st BS then memcpy not add */
            Il_First_Bs = (expInfo->RAFlag == 0) && First_Bs;

            /* if inline DSP and 1st BS then memcpy not add */
            Dsp_First_Bs = dspflag && First_Bs;

            DPRINT4(1,"Ra_Ct_Zero: %d, First_Bs: %d, Il_First_Bs: %d, Dsp_First_Bs: %d\n",
		Ra_Ct_Zero,First_Bs,Il_First_Bs,Dsp_First_Bs);

	   /* Check for 1st fid of interleaving to write without 	*/
	   /* summing to the datafile. Note: bs >= 1 for IlFlag.	*/
/*
           if ( ((expInfo->RAFlag) && (expInfo->CurrentTran == 0)) ||
	        ((expInfo->RAFlag == 0) && 
		 (((long)fidstatblk.ct / expInfo->NumInBS) <= 1 )) ||
                 ((dspflag)  && (((long)fidstatblk.ct / expInfo->NumInBS) <= 1 )) )
*/
	   /* if ( Ra_Ct_Zero || Il_First_Bs || Dsp_First_Bs) */
	   if ( Ra_Ct_Zero || Il_First_Bs )
           {
             /* 1st Block size don't add just copy into file */
	     DPRINT3(1,"IL BS 1: memcpy(0x%lx,0x%lx,%lu)\n",(datafile->offsetAddr + sizeof(fidblockheader)),
				blksizeData, fidstatblk.dataSize);

#ifdef LINUX
             DPRINT4(2,"DSP NumDataPts=%d DataPtSize=%d datasize= %d size/num=%d\n",expInfo->NumDataPts, 
                         expInfo->DataPtSize,  fidstatblk.dataSize,  fidstatblk.dataSize/ expInfo->NumDataPts);
             if (expInfo->DspOversamp > 1)
                byteSwap(datafile->offsetAddr + sizeof(fidblockheader),
                    expInfo->NumDataPts, (expInfo->DataPtSize == 2) );
#endif
	     memcpy((void*)(datafile->offsetAddr + sizeof(fidblockheader)),
			(void*)blksizeData, fidstatblk.dataSize); 

#ifdef DEBUG
             {   /* diagnostic output */
        	int i;
		long *lptr;
		short *sptr;
	        if (expInfo->DataPtSize == 4)
		{	
             	   for(i=0,lptr=(long*)blksizeData;i<20;i++)
             	   {
              	     DPRINT2(1,"data[%d]: %ld\n",i,*lptr++);
             	   }
		}
		else
		{	
             	   for(i=0,sptr=(short*)blksizeData;i<20;i++)
             	   {
              	     DPRINT2(1,"data[%d]: %hd\n",i,*sptr++);
             	   }
		}
	     }
#endif
           }
	   else
           {
      	     if (expInfo->DataPtSize == 4)    /* 16/32 bit data to sum ? */
      	     {
               if (expInfo->DspOversamp > 1)  /* inline dsp results in floats */
               {
      	         DPRINT3(1,"sumFloatData(0x%lx, 0x%lx, %lu)\n",
		    (datafile->offsetAddr + sizeof(fidblockheader)), blksizeData,
		     fidstatblk.np );
#ifdef LINUX
                 byteSwap((long*)(datafile->offsetAddr + sizeof(fidblockheader)), fidstatblk.np, 0);
                 /* Incoming data was already byte-swapped for DSP operation */
#endif
       	         stat = sumFloatData( (long*)(datafile->offsetAddr + 
		        sizeof(fidblockheader)),(long*)blksizeData, fidstatblk.np );
#ifdef LINUX
                 byteSwap((long*)(datafile->offsetAddr + sizeof(fidblockheader)), fidstatblk.np, 0);
#endif
	       }
	       else
               {
      	         DPRINT3(1,"sumLongData(0x%lx, 0x%lx, %lu)\n",
		    (datafile->offsetAddr + sizeof(fidblockheader)), blksizeData,
		    fidstatblk.np );
#ifdef LINUX
                 byteSwap((long*)(datafile->offsetAddr + sizeof(fidblockheader)), fidstatblk.np, 0);
                 byteSwap((long*)blksizeData, fidstatblk.np, 0);
#endif
       	         stat = sumLongData( (long*)(datafile->offsetAddr + 
		        sizeof(fidblockheader)),(long*)blksizeData, fidstatblk.np);
#ifdef LINUX
                 byteSwap((long*)(datafile->offsetAddr + sizeof(fidblockheader)), fidstatblk.np, 0);
#endif
                 if (stat)
       	            errLogRet(ErrLogOp,debugInfo,
			  "sumLongData: Imminent OverFlow Detected");
	       }
                 
       	     }
	     else
             {
		/* for oversamping use exp info NP, otherwise use np that came back from
		   console (i.e. might array np */
               DPRINT3(1,"sumShortData(0x%lx, 0x%lx, %lu)\n",
		    (datafile->offsetAddr + sizeof(fidblockheader)), blksizeData,
		     fidstatblk.np );
#ifdef LINUX
               byteSwap((short*)(datafile->offsetAddr + sizeof(fidblockheader)), fidstatblk.np, 1);
               byteSwap((long*)blksizeData, fidstatblk.np, 1);
#endif
               stat = sumShortData( (short*)(datafile->offsetAddr + 
		    sizeof(fidblockheader)),(short*)blksizeData,  fidstatblk.np  );
#ifdef LINUX
               byteSwap((short*)(datafile->offsetAddr + sizeof(fidblockheader)), fidstatblk.np, 1);
#endif
               if (stat)
       	          errLogRet(ErrLogOp,debugInfo,
			"sumShortData: OverFlow Detected, Data Corrupted");
             }
           }
      	}
        else
        {
           /* INLINE DSP */
           if (expInfo->DspOversamp > 1)
           {
	      DPRINT(1,"dspExec Filter ----------------------------\n");
#ifdef LINUX
             byteSwap(blksizeData, fidSize / expInfo->DataPtSize, (expInfo->DataPtSize == 2) );
#endif
              dspExec(blksizeData, (datafile->offsetAddr + sizeof(fidblockheader)),
			expInfo->NumDataPts, fidSize, fidstatblk.ct);
#ifdef LINUX
             byteSwap(datafile->offsetAddr + sizeof(fidblockheader),
                 expInfo->NumDataPts, (expInfo->DataPtSize == 2) );
#endif
           }
        }


      	/* keep track of largest FID element received to update datafilehead in 
	   case of SA or some other early stopping point prior to Exp completion
      	*/ 
      	if (fidstatblk.elemId > maxFidsRecv)
	   maxFidsRecv = fidstatblk.elemId;

      	/* Update fid block header for this FID */
      	/* fidblockheader.scale = fidstatblk.scale; */
      	fidblockheader.index = htons(fidstatblk.elemId);

	/* If acquisition is clearing data at blocksize, correct the ct */
	if (expInfo->ExpFlags & CLR_AT_BS_BIT) {
	    fidblockheader.ctcount = htonl(expInfo->NumInBS);
	} else {
	    fidblockheader.ctcount = htonl(fidstatblk.ct);
	}
      	fidblockheader.lvl = (float) dcLevel[iRcvr].r;
      	fidblockheader.tlt = (float) dcLevel[iRcvr].i;
        if ( (expInfo->DspOversamp > 1) && (fidstatblk.ct == 1) )
        { 
             fidblockheader.lvl = 0.0;       /* make sure FID offsets are zero after dsp */
             fidblockheader.tlt = 0.0;       /* make sure FID offsets are zero after dsp */
        } 
#ifdef LINUX
        {
           floatInt un;
           un.fval = &fidblockheader.lvl;
           *un.ival = htonl( *un.ival );
           un.fval = &fidblockheader.tlt;
           *un.ival = htonl( *un.ival );
        }
#endif

        if  (expInfo->DspGainBits)
      	    fidblockheader.scale = htons( -expInfo->DspGainBits );

      	/* Copy fid block header to Disk */
     	/* we copy this even for RA since the fid data may not be complete
	  (i.e. not all fids taken) before SA. Therefore the fidblockhead may
	  not be present for this FID.
      	*/
      	memcpy((void*)fidblkhdrSpot,(void*)&fidblockheader,
						sizeof(fidblockheader));

      	/* --- Update Current CT & Current Element ----- */
      	expInfo->CurrentTran = fidstatblk.ct;	/* Current Transient */
      	expInfo->Celem = fidstatblk.elemId;       /* Current Element */

      	/* set the number of completed FIDs */
      	/* Note" using expInfo->NumTrans to determine if FIDs are completed
	   is probably not going to work in the long run, since people
	   will want to array NT, which means each FID would have its own
	   criteria for completion....
      	*/
      	if ( (fidstatblk.doneCode == EXP_FID_CMPLT) )
      	{
           expInfo->FidsCmplt = fidstatblk.elemId;
      	}

      	/* Now the FID Xfr is complete, time to update status struct &
	   check on processing */

      	/* Update Status so that others can be aware of whats happening */
      	UpdateStatus(&fidstatblk);
      	Process(&fidstatblk);
      } /* end std processing (non-noise data) */

      DPRINT4(1,"eleId %ld == NFids %ld, statCT %ld == NumTrans %ld\n",
		fidstatblk.elemId,expInfo->ArrayDim,fidstatblk.ct,
							expInfo->NumTrans);
      /* Note" using expInfo->NumTrans to determine if FIDs are completed
	 is probably not going to work in the long run, since people
	 will want to array NT, which means each FID would have its own
	 criteria for completion....
      */
      if ( (fidstatblk.elemId == expInfo->ArrayDim) &&
	   (fidstatblk.doneCode == EXP_FID_CMPLT) )
      {
	  /* check for abort at last possible moment! */
          /* if abort don't set done, need to go back and read the statblock
             that indicates an abort , otherwise the next exp will think it was aborted!
          */
  	  if (expInfo->ExpState != EXPSTATE_ABORTED)
	  {
	    DPRINT(1,"Exp. Complete.\n");
   	    done = 1;
          }
      }
   }

   if (discardData != NULL)
      free(discardData);
   /* incase we are Interleaving ,RA, Inline DSP, we need to free this buffer */
   if (blksizeData != NULL)
      free(blksizeData);

   return(EXP_CMPLT);
}

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

   for (i=0;i<np;i++,dstadr++)
   {
#ifdef DEBUG
     if (i<20)
     {
        DPRINT3(1,"sum: %f = %f + %f\n",*dstadr + *srdadr,*dstadr,*srdadr);
     }
#endif
     *dstadr += *srdadr++;
   }
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
#ifdef DEBUG
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
   long sumval;
   long maxval =  0x7FFF;  /* 2^15 */
   int ovrflow = 0;

   for (i=0;i<np;i++,dstadr++)
   {
     if ( (sumval = (long) *dstadr + (long) *srdadr++) > maxval)
	ovrflow = 1;
     if (sumval < -maxval)
	ovrflow = 1;

#ifdef DEBUG
     if (i<20)
     {
        DPRINT4(1,"sum: %ld = %hd + %hd\n, ovrflow: %d",
		sumval,*dstadr,*(srdadr-1),ovrflow);
     }
#endif
        
     *dstadr = (short) sumval;
   }
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
        fidfileheader.status    = S_DATA | S_OLD_COMPLEX;/* init status FID */
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
        fidblockheader.status = S_DATA | S_OLD_COMPLEX;/* init status to fid*/
        fidblockheader.index = (short) 0;
        fidblockheader.mode = (short) 0;
        fidblockheader.ctcount = (long) 0;
        fidblockheader.lpval = (float) 0.0;
        fidblockheader.rpval = (float) 0.0;
        fidblockheader.lvl = (float) 0.0;
        fidblockheader.tlt = (float) 0.0;
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
    /* -----------------------  ACQ File Header  -------------------------- */
    acqfileheader.nblocks =  expInfo->ArrayDim; /* n fids */
    acqfileheader.ntraces = 1L;
    acqfileheader.np = (long) sizeof(lc) / 2;  /* total size */
    acqfileheader.ebytes = 2;   /* variable size in bytes */
    acqfileheader.tbytes = sizeof(lc); /* Low Core parameters in bytes */
    acqfileheader.bbytes = sizeof(lc) + sizeof(acqblockheader);/* blockbytes */
    acqfileheader.nbheaders = 1;
    acqfileheader.status = S_DATA | S_OLD_ACQPAR;/* init status to abort code */
    acqfileheader.vers_id = 0;
    /* ------------------------  ACQ Header  ---------------------------- */
    acqblockheader.scale = (short) 0;
    acqblockheader.status = S_DATA | S_OLD_ACQPAR;
    acqblockheader.index = (short) 0;
    acqblockheader.mode = (short) 0;
    acqblockheader.ctcount = (long) 0;
    acqblockheader.lpval = (float) 0.0;
    acqblockheader.rpval = (float) 0.0;
    acqblockheader.lvl = (float) 0.0;
    acqblockheader.tlt = (float) 0.0;

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

      if (pFidstatblk->doneCode == EXP_FID_CMPLT ||
         (pFidstatblk->doneCode == BS_CMPLT && ilflag)) {
          elemid = pFidstatblk->elemId + 1;
          if (elemid > arraydim) {
              if (pFidstatblk->doneCode == BS_CMPLT)
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
          if (pFidstatblk->doneCode == BS_CMPLT) {
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
int Process(FID_STAT_BLOCK *pFidstatblk)
{
    int stat,qstat;

    /*
	check for Wbs, Wnt, Wexp, Werr
    */
    if (pFidstatblk->doneCode == ERROR)
    {
       if ( expInfo->ProcMask & WHEN_ERR_PROC )
       {
	 DPRINT(1,"Werr Processing\n");
	 /* msgQ full, wait because we guarantee this processing to be done */
	 procQadd(WERR, ShrExpInfo->MemPath, pFidstatblk->elemId, pFidstatblk->ct,
                  pFidstatblk->doneCode, pFidstatblk->errorCode);
	 if ((stat = sendMsgQ(pProcMsgQ,"chkQ",strlen("chkQ"),MSGQ_NORMAL,NO_WAIT)) != 0)
	 {
       	    errLogRet(ErrLogOp,debugInfo, 
	    "Process: Procproc is not running. Error Process not done for FID %lu, CT: %lu\n",
		pFidstatblk->elemId,pFidstatblk->ct);
            return(-1);
	 }
        }
       return(0);
    }

    /* Wexp test */
    if ( (pFidstatblk->elemId == expInfo->ArrayDim) &&
	   (pFidstatblk->doneCode == EXP_FID_CMPLT) )
           /* (pFidstatblk->ct == expInfo->NumTrans) ) */
    {
         int count = 0;
         /* Set this now, since go may check to see is acquisition is active */
         setStatExpName("");
         setStatGoFlag(-1);     /* No Go,Su,etc acquiring */
         /* wait for final statblock since processing will read
          * shim and lock parameters from it.
          */
         while ((count < 10) && (getStatAcqState() != ACQ_IDLE))
         {
            count++;
            sleepMilliSeconds(100);
         }
	 /* msgQ full, wait because we guarantee this processing to be done */
         if (expInfo->ProcWait > 0)   /* au(wait) or just au */
         {
            DPRINT2(1,"wExp(wait): Queue FID: %ld, CT: %ld\n",
		    pFidstatblk->elemId, pFidstatblk->ct);
	    procQadd(WEXP_WAIT, ShrExpInfo->MemPath, pFidstatblk->elemId,
		     pFidstatblk->ct, EXP_COMPLETE, pFidstatblk->errorCode);
         }
         else
         {
            DPRINT2(1,"wExp: Queue FID: %ld, CT: %ld\n",
		    pFidstatblk->elemId, pFidstatblk->ct);
	    procQadd(WEXP, ShrExpInfo->MemPath, pFidstatblk->elemId,
		     pFidstatblk->ct, EXP_COMPLETE, pFidstatblk->errorCode);
         }
	 if ((stat = sendMsgQ(pProcMsgQ,"chkQ",strlen("chkQ"),MSGQ_NORMAL,NO_WAIT)) != 0)
	 {
       	    errLogRet(ErrLogOp,debugInfo, 
	    "Process: Procproc is not running. Wexp Process not done for FID %lu, CT: %lu\n",
		pFidstatblk->elemId,pFidstatblk->ct);
            return(-1);
	 }
       return(0);
    }

    /* Wnt test */
    /* if ( (pFidstatblk->ct == expInfo->NumTrans) ) */
    if ( (pFidstatblk->doneCode == EXP_FID_CMPLT) )
    {
	 DPRINT(1,"Wnt Processing\n");
	 /* msgQ full, just come back, we can skip Wnt processing */
	 qstat = procQadd(WFID, ShrExpInfo->MemPath, pFidstatblk->elemId, pFidstatblk->ct,
                          pFidstatblk->doneCode, pFidstatblk->errorCode);
         if (qstat != SKIPPED)
         {
            DPRINT2(1,"wNT: Queue FID: %ld, CT: %ld\n",
		pFidstatblk->elemId, pFidstatblk->ct);
	    if ((stat = sendMsgQ(pProcMsgQ,"chkQ",strlen("chkQ"),MSGQ_NORMAL,NO_WAIT)) != 0)
	    {
       	       errLogRet(ErrLogOp,debugInfo, 
	       "Process: Procproc is not running. Wnt Process not done for FID %lu, CT: %lu\n",
		   pFidstatblk->elemId,pFidstatblk->ct);
               return(-1);
	    }
  	 }
       return(0);
    }

    /* Wbs test */
    if (expInfo->NumInBS != 0)
    {
      if ( (pFidstatblk->doneCode == BS_CMPLT) )
      {
	   DPRINT(1,"Wbs Processing\n");
	   /* msgQ full, just come back, we can skip Wbs processing */
	   qstat = procQadd(WBS, ShrExpInfo->MemPath, pFidstatblk->elemId, pFidstatblk->ct,
                            pFidstatblk->doneCode, pFidstatblk->ct / expInfo->NumInBS);
           if (qstat != SKIPPED)
           {
              DPRINT2(1,"wBS: Queue FID: %ld, CT: %ld\n",
		pFidstatblk->elemId, pFidstatblk->ct);
	      if ((stat = sendMsgQ(pProcMsgQ,"chkQ",strlen("chkQ"),MSGQ_NORMAL,NO_WAIT)) != 0)
	      {
       	       errLogRet(ErrLogOp,debugInfo, 
	       "Process: Procproc is not running. Wbs Process not done for FID %lu, CT: %lu\n",
		   pFidstatblk->elemId,pFidstatblk->ct);
               return(-1);
	      }
  	   }
         return(0);
      }
    }
   return(0);
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
   char *dataPtr;
   unsigned long xfrSize,bytes2xfr,size2alloc;
   unsigned long ptsRecv;
   int64_t lastFid;
   int bytesPerPt,done,bytes,sync;
   FID_STAT_BLOCK fidstatblk;
   char acqMsg[ACQ_UPLINK_XFR_MSG_SIZE+5];
   int enddata_flag;

/*  These identifiers were added to insure transfer into the memory shared with
    ACQI is done one long-word at a time.  The socket driver (see readChannel ->
    readSocket) appears to do this transfer one short word at a time.  If a data
    point is changing its sign and ACQI gets one short word before and one short
    word afterwards, the resulting long word will have a value around +/- 65536
    (2**16) and introduce a Spike into the displayed spectrum.  March 1996.	*/

   char *dataRecvBuf;
   int *isrca, *idsta, iter;

   /* sit in this while loop for the entire Experiment */

   done = enddata_flag = 0;
   lastFid = -1;
   ptsRecv = 0;
   bytesPerPt = 0;

/*  See comment dated March 1996, above (dataRecvBuf, size2alloc was added January 1997)  */

   size2alloc = expInfo->NumDataPts * expInfo->DataPtSize * expInfo->NumFids;
   dataRecvBuf = (char *) malloc( size2alloc );

/*  The value for NumFids should always be valid, even if nf
    is not defined.  Look at initacqparms.c, SCCS category PSG.  */

   DPRINT1( 1, "in recvInteract, allocated %d chars\n", size2alloc );
   DPRINT1( 1, "in recvInteract: number of FIDS: %d\n", expInfo->NumFids );
   if (dataRecvBuf == NULL) {
      errLogSysRet(ErrLogOp,debugInfo,"recvInteract: cannot get space to receive data\n" );
      return(-1);
   }

   bytes = flushChannel( chanId );
   DPRINT1(1,"recvInteract: flush %d bytes from channel\n",bytes);
   while( !done )
   {
      /* keep reading channel till uplink msg is obtain then proceed */
      /* This action is an attempt to resync channels incase there are */
      /* problems on the channel, garbled, wrong # bytes, etc.  */
      sync = 0;
      while( !sync )
      {
         DPRINT(1,"recvInteract: Waiting for Acq ACQ_UPLINK_INTERA_MSG Msg\n");
         blockAllEvents();
         bytes = readChannel(chanId,acqMsg,ACQ_UPLINK_XFR_MSG_SIZE);
         unblockAllEvents();
         acqMsg[ACQ_UPLINK_XFR_MSG_SIZE] = '\0';
         DPRINT1(1,"recvInteract: Acq UPLINK Msg: '%s'\n",acqMsg);

	 if (strncmp(acqMsg,ACQ_UPLINK_INTERA_MSG,sizeof(ACQ_UPLINK_INTERA_MSG)) == 0)
         {
	    sync = 1;
         }
	 else
         {
	    /* test if for some reason we are to stop data transfer */
	    if (
	        strncmp(acqMsg,ACQ_UPLINK_ENDDATA_MSG,strlen(ACQ_UPLINK_ENDDATA_MSG))
		  == 0)
    	    {
		enddata_flag = 1;
	        sync = 1;
	    }
	    else if (
	       strncmp( acqMsg, ACQ_UPLINK_NODATA_MSG,strlen(ACQ_UPLINK_NODATA_MSG) ) 
			== 0) 
            {
		sync = 0;
	    }

	 }
      }

      blockAllEvents();
      bytes = readChannel(chanId,(char*)&fidstatblk,sizeof(fidstatblk));
      unblockAllEvents();
      if (bytes < sizeof(fidstatblk))
      {
	/* don't know how to recover, so terminate process */
	errLogSysRet(ErrLogOp,debugInfo,"recvInteract: Failed to get fidstatblk\n");
	free( dataRecvBuf );
	return(-1);
      }
#ifdef LINUX
      FSB_CONVERT( fidstatblk );
#endif
      DPRINT5(1,"Fid StatBlk: Fid: %ld, DataSize: %ld, CT: %ld, NP: %ld,\n                                      State: %d\n",
		fidstatblk.elemId, fidstatblk.dataSize, fidstatblk.ct,
		fidstatblk.np, fidstatblk.doneCode);
      if (fidstatblk.elemId != lastFid)
      {
	lastFid = fidstatblk.elemId;
	ptsRecv = 0;
	if (fidstatblk.np > 0)
	  bytesPerPt = fidstatblk.dataSize / fidstatblk.np;
      }

      dataPtr = ifile->offsetAddr + sizeof(TIMESTAMP); 
      memcpy((void*) dataPtr, (void*) &fidstatblk,sizeof(fidstatblk));

      /*********************************************************************
       * If data transfer stopped permaturely use fidstatblk to find out why
       * then do the right thing.
       *
       * Special Notes:
       *    The time stamp is updated only after the interactive data has
       *    been received.  If for some reason the program returns prematurely
       *    you probably will want to update the time stamp before returning.
       *    See the EXP_ABORTED case, below.
       *
       *    Except for EXP_ABORTED, processing continues, even if an error
       *    has occurred.  recvInteract relies on the data size being 0 to
       *    avoid a serious jam.
       */

      if (enddata_flag)
      {
	 switch(fidstatblk.doneCode)
         {
	   case STOP_CMPLT:		/* result of SA, no process */
                DPRINT(1,"recvInteract: STOP_CMPLT\n");
/* 		return(EXP_STOPPED); */
		break;

	   case EXP_HALTED:		/* result of Halt, do Wexp processing */
               DPRINT(1,"recvInteract: EXP_HALTED\n");
	      /* return(EXP_HALTED); */
	      break;

	   case EXP_ABORTED:
               DPRINT(1,"recvInteract: EXP_ABORTED\n");
		gettimeofday((struct timeval *) ifile->offsetAddr, NULL );
		free( dataRecvBuf );
		return(EXP_ABORTED);	/* result of ABORT Experiment */
		break;

	   case HARD_ERROR:
               DPRINT(1,"recvInteract: HARD_ERROR\n");
		/* return(EXP_HALTED);	/* result of HARD_ERROR. RPNZ */
		break;

	  default:
	/* 	return(EXP_STOPPED); */
		break;
         }
      }


      /* set to fid data position */;
      /* We no longer let the socket driver (readChannel -> readSocket) transfer
         directly into the shared memory.  See comment dated March 1996, above	*/
      /*dataPtr = ifile->offsetAddr + sizeof(TIMESTAMP) + sizeof( fidstatblk ); 
      dataPtr += ptsRecv * bytesPerPt;*/

      dataPtr = dataRecvBuf;			/* put the data here, initially */
      bytes2xfr = fidstatblk.dataSize;
      DPRINT(2,"Data Xfr:\n");
      while( bytes2xfr )			/* bytes2xfr is 0 if an error occurred */
      {
          if (bytes2xfr < XFR_SIZE)
	     xfrSize = bytes2xfr;
          else
	     xfrSize = XFR_SIZE;

         blockAllEvents();
	  bytes = readChannel(chanId,dataPtr,xfrSize);
         unblockAllEvents();
          if (bytes < xfrSize)
          {
	    /* don't know how to recover, so terminate process */
	    /*mClose(ifile);*/
	    errLogRet(ErrLogOp,debugInfo,"uploadData: Failed to get xfrSize.\n");
	    free( dataRecvBuf );
	    return( -1 );
          }

	  dataPtr += xfrSize;
	  bytes2xfr -= xfrSize;
      }

/*  Now transfer the new console data into the memory shared with ACQI.
    Be aware the value of dataRecvBuf never changes.  We used dataPtr
    to move through the receive buffer, above, and now use it to access
    the shared memory.

    PSG sets dp to "y", so np is always a count in long-words.		*/

      if ((fidstatblk.np > 0) && 
		((ptsRecv + fidstatblk.np) <= expInfo->NumDataPts))
      {
          dataPtr = ifile->offsetAddr + sizeof(TIMESTAMP) + sizeof( fidstatblk ); 
          dataPtr += ptsRecv * bytesPerPt;
          isrca = (int *) dataRecvBuf;
          idsta = (int *) dataPtr;
          for (iter = 0; iter < fidstatblk.np; iter++)
          {
	    *idsta++ = ntohl( *isrca );
	    isrca++;
          }
      }

      ptsRecv += fidstatblk.np;
      if (ptsRecv >= expInfo->NumDataPts)
        ptsRecv = 0;

/*  Now we are ready to mark this as new data...  */

      gettimeofday((struct timeval *) ifile->offsetAddr, NULL );
   }

/*  This never happens ...  an interactive experiment never completes ...  for now  */

   free( dataRecvBuf );  /* Prevent memory leak if it ever does happen */
   return(EXP_CMPLT);
}
