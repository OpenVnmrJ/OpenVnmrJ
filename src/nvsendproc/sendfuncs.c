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
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <netinet/in.h>
#include <errno.h>

#ifndef SENDTEST
#include "shrMLib.h"
#include "expentrystructs.h"
#include "hostAcqStructs.h"
#endif

#include "errLogLib.h"
#include "mfileObj.h"
#include "shrexpinfo.h"
#include "lc.h"
#include "PSGFileHeader.h"

#include "ndds/ndds_c.h"
#include "NDDS_Obj.h"
#include "threadfuncs.h"
#include "nddsfuncs.h"

#include "barrier.h"

extern cntlr_crew_t TheSendCrew;
extern barrier_t TheBarrier;

typedef struct _ptrNsize_ {
		     char *pCode;
	             unsigned long cSize;
	           } CODEENTRY;

typedef CODEENTRY *CODELIST;

		    
extern int chanId;	/* Channel Id */
SHR_EXP_INFO expInfo = NULL;   /* start address of shared Exp. Info Structure */

static char command[256];
static char response[256];
static char dspdlstat[256];
#ifndef SENDTEST
static SHR_MEM_ID  ShrExpInfo = NULL;
ExpEntryInfo ActiveExpInfo;
#endif

/* global info retained for acode shipping to console */
static char labelBase[81];
static long nAcodes;
static MFILE_ID mapAcodes = NULL;
static CODELIST CodeList = NULL;
static int AbortXfer = 0;	/* SIGUSR2 will set this to abort transfer, no longer used  */

static double total_timeout_val = 0.0;  /* total timeout for downloading acodes/pattern, etc. */

char *genLabel(ComboHeader *globalHeader,char *baselabel,char* label);

#ifndef SENDTEST
/***************************************************************
* sendCodes - Send an Experiment down to console
* 	      RT parameter table, Tables, Acodes
*   Args.   1. ExpInfo File
*	    2. Basename for Named Buffers (Exp1, Exp1rt,Exp1f1,Exp1t1,etc.)
*
*/
sendCodes(char *argstr)
{
    char *bufRootName;
    char *expinfofile;
    char *filename;
    int i, result, status;
    cntlr_crew_t *crew;
    SHR_EXP_INFO pExpInfo;
    off_t xferAggSize,acodeSize;
    off_t getXferFileSizes(char *filename);
    off_t determineAggregateSizes( SHR_EXP_INFO expInfo, char *cntlrName);
    double pattime, codetime, min_timeout, time_out;
    char cntlrFileName[256];


    crew = &TheSendCrew;

    /* Vnmr will not allow an experiment to start if still active thus 
       in we enter this routine and thread are not finished, then most
       likely an abort or error has left one or more thread block
       either in a barrierWait() or in a msgeGet()
     */
    /* all thread should be idle, if true, then the following routine 
       returns immediately, otherwise it waits on the conditional
       and attempts to unblock threads if needed */

    status = wait4DoneCntlrStatus(crew);

    /* Close Out any previous Experiment Files */
    resetState();
    AbortXfer = 0;

    /* 1st Arg Exp Info File */
    filename = strtok(NULL," ");
    strcpy(ActiveExpInfo.ExpId,filename);

    /* 2nd Arg. Basename (e.g. Exp1)  */
    bufRootName = strtok(NULL," ");

    DPRINT2(+1,"sendCodes: Info File: '%s', Base name: '%s'\n",
	        ActiveExpInfo.ExpId,bufRootName);

    /* Map In the Experiments Info File */
    if (mapInExp(&ActiveExpInfo) == -1)
    {
        errLogRet(ErrLogOp,debugInfo,
		"sendCodes: Exp: '%s' failed no Codes sent.\n",
		 ActiveExpInfo.ExpId);
        /* Need some type of msge back to Expproc to indicate failure */
	return(-1);
    }

    pExpInfo = ActiveExpInfo.ExpInfo;

    if (ActiveExpInfo.ExpInfo->InteractiveFlag == 1)
       ActiveExpInfo.ExpInfo->ExpState = 0;
    if (ActiveExpInfo.ExpInfo->ExpState >= EXPSTATE_TERMINATE)
    {
       DPRINT1(1,"Terminate before we got started, ExpState: %d\n",
		ActiveExpInfo.ExpInfo->ExpState);
       mapOutExp(&ActiveExpInfo);
       return(0);
    }

    barrierSetCount(&TheBarrier,crew->crew_size); /* update number of threads in the barrier membership */

    /* prepare the thread for work */
    time_out = 0.0;
    min_timeout = 8.0; /* seconds */
    for( i = 0; i < crew->crew_size; i++)
    { 
          strncpy(crew->crew[i].filepath,ActiveExpInfo.ExpId,CREW_FILEPATH_SIZE); /* "/vnmr/acqqueue/exp1NV" */
          strncpy(crew->crew[i].label,bufRootName,CREW_LABEL_SIZE);
          /* strcpy(crew->crew[i].label,ActiveExpInfo.ExpInfo->AcqBaseBufName); */
          crew->crew[i].pParam = ActiveExpInfo.ExpInfo;   /* &ShrExpInfo; */
          crew->crew[i].number = i;
          crew->crew[i].size = i;
          crew->cmd[i] = i+1;
	  nddsBufMngrInitBufs(crew->crew[i].pNddsBufMngr);


          xferAggSize = determineAggregateSizes(ActiveExpInfo.ExpInfo, crew->crew[i].cntlrId);
          DPRINT2(+4,"'%s': total Xfer size: %ld\n",crew->crew[i].cntlrId, xferAggSize);
          sprintf(cntlrFileName,"%s.%s",ActiveExpInfo.ExpInfo->PSCodefile,crew->crew[i].cntlrId);
          acodeSize = getXferFileSizes(cntlrFileName);
          DPRINT3(+4,"'%s': Acode size: %ld average Acode size: %ld\n",crew->crew[i].cntlrId, acodeSize, acodeSize/ActiveExpInfo.ExpInfo->NumAcodes);
          pattime = xferAggSize / 1000000.0;
          codetime = ((acodeSize/ActiveExpInfo.ExpInfo->NumAcodes) * 64.0) / 1000000.0;
          DPRINT3(+4,"'%s': pattime: %lf, codetime: %lf\n", crew->crew[i].cntlrId, pattime, codetime);
          time_out += (pattime * 2) + codetime;
          DPRINT1(+4,"Total Accum Time: time_out: %lf\n", time_out);
    }  
    time_out += min_timeout;
    time_out = (time_out > 60.0) ? 60.0 : time_out; 

    total_timeout_val = time_out;
    DPRINT1(+4,"Total TimeOut:  %lf\n", total_timeout_val);

    status = pthread_mutex_lock (&crew->mutex);
    if (status != 0)
          return status;
    crew->work_count = crew->crew_size;

    /* Wake all the threads to begin their work */
    status = pthread_cond_broadcast (&crew->cmdgo);
    pthread_mutex_unlock (&crew->mutex);

    /* return immediate to be able to process next message */
}
#endif

/*
 * This is the main routine called by each of the pthreads
 * Each thread handles the download to a particular controller
 * e.g. master, rf1,rf2,lock,ddr, pfg, gradient
 * As such this routine must be reentrient 
 *
 * A thread only exits for controllers that have published their 'cntlrname'/downld/reply topic
 * Thread will only call this routine if there is an active subscription to this publication from
 *  a controller
 *
 * 1. Send the LC & Table file down, all controller receive these
 *
 * 2. look up controller file names, open files determine number of items in each
 *
 * 3. Obtain total free named buffers that are available for download use.
 *
 * 4. If all fits then just download them
 *
 * 4b. Try to fit all the tables, most of waveforms then send what Acodes there is space available
 *
 * 5. Done return.
 */
downLoadExpData(cntlr_t *pWrker,  void *expinfo, char *bufRootName)
{
    int freeNamedBufs;
    int result,initPSflag;
    SHR_EXP_INFO  expInfo;
    char cntlrFileName[256];
    off_t xferAggSize,acodeSize;
    off_t getXferFileSizes(char *filename);
    off_t determineAggregateSizes( SHR_EXP_INFO expInfo, char *cntlrName);
    double pattime, codetime, min_timeout, time_out;

    /* ActiveExpInfo = (ExpEntryInfo*) expinfo; */
    expInfo = (SHR_EXP_INFO) expinfo;

    /* 1. are my assigned controller files present */
      
     DPRINT1(+1,"'%s': downLoadExpData:  send Start of Xfer message to controller\n",pWrker->cntlrId);
     /* determine sizes of files to download, and calculae approx time required at the ethernet bandwidth
        to down load the files to the controller. Assumption will be made on the conservative side
        on the total aggragate badnwidth of the system divide by the total number of controllers
        being download to. Maximum badnwidth to each controller is ~ 5MN/sec due to the limitation on the
        VxWorks 405GPR network stack speed. measurement on the a SUN Blade-2000 daul CPU 950 MHZ has max 
        bandwidth of ~50 MB/sec on Gigabit ethernet, Dell 370N 3.6 GHZ Intel maximum of 35 MB/sec, will downgrade this
        to approx 20 MB/sec  so 20/num_of_controllers  */
   
     /* the files we need to check are pattern and tables acodes */
     /* sendXferStart(pWrker->PubId, numberof pat, timeout); */

     DPRINT3(+4,"'%s': acode_1_size: %ld, acode_max_size: %ld\n", pWrker->cntlrId, expInfo->acode_1_size,expInfo->acode_max_size);

#ifdef XXXX  // now down in main thread for a total of all controller file transfer 
     xferAggSize = determineAggregateSizes(expInfo, pWrker->cntlrId);
     DPRINT2(+4,"'%s': total Xfer size: %ld\n",pWrker->cntlrId, xferAggSize);
     sprintf(cntlrFileName,"%s.%s",expInfo->PSCodefile,pWrker->cntlrId);
     acodeSize = getXferFileSizes(cntlrFileName);
     DPRINT3(+4,"'%s': Acode size: %ld average Acode size: %ld\n",pWrker->cntlrId, acodeSize, acodeSize/expInfo->NumAcodes);
     pattime = xferAggSize / 1000000.0;
     codetime = ((acodeSize/expInfo->NumAcodes) * 64.0) / 1000000.0;
     DPRINT3(+4,"'%s': pattime: %lf, codetime: %lf\n", pWrker->cntlrId, pattime, codetime);
     min_timeout = 8.0; /* seconds */
     time_out = min_timeout + (pattime * 2) + codetime;
     time_out = (time_out > 60.0) ? 60.0 : time_out; 

     sendXferStartwArgs(pWrker->PubId,(int) (time_out+0.99),0);
#endif

    DPRINT2(+4,"'%s', Total TimeOut:  %lf\n", pWrker->cntlrId, total_timeout_val);
     sendXferStartwArgs(pWrker->PubId,(int) (total_timeout_val+0.99),0);
     /* sendXferStart(pWrker->PubId); */

    /* determine total number of free named buffers */
     /* getXferSize(NDDS_ID pubId, int *pipeFd); */
    /* freeNamedBufs =  getXferSize(pWrker->PubId, pWrker->SubPipeFd); */

    /* how many does the experiment need? */
    /* 1 - RT params */
    /* number of Tables ? */
    /* number of WForms ? */
    /* number of Acodes ? */
    /* allocate a fair(?) distrubution among them */
    /* if all params, tables , & wforms can be accomidate then send them all */
    /* then parse out the Acodes */

   DPRINT3(+1,"'%s': downLoadExpData: File: '%s', label: '%s'\n",pWrker->cntlrId,pWrker->filepath,pWrker->label);
   /* printShrExpInfo(expInfo); */

    DPRINT2(+1,"'%s': downLoadExpData: Send LC Params. & Tables  File: '%s'\n",pWrker->cntlrId,expInfo->AcqRTTablefile);
    if ( ((int) strlen(expInfo->AcqRTTablefile) > 0) )
    {
       initPSflag = 0;
       DPRINT1(1,"'%s': downLoadExpData: Send LC Params.\n",pWrker->cntlrId);
       result = xferFile(expInfo,initPSflag,pWrker->cntlrId,pWrker->PubId,pWrker->pNddsBufMngr /*pWrker->SubPipeFd*/,
				expInfo->AcqRTTablefile,pWrker->label,&(pWrker->numSubcriber4Pub));
       if (result < 0)
       {
          /* sendExpStat("endExp"); */
          DPRINT1(+1,"'%s': downLoadExpData:  send Completion of Xfer message to controller\n",pWrker->cntlrId);
          sendXferCmplt(pWrker->PubId);

          errLogRet(ErrLogOp,debugInfo,
		"'%s': sendCodes: LC & Pattern File unable to be  sent. ExpState: %d\n",
		 pWrker->cntlrId,expInfo->ExpState);

          return(-1);
        /* Need some type of msge back to Expproc to indicate failure */
       }
    }

    DPRINT2(+1,"'%s', downLoadExpData: Send Init PS. File: '%s'\n",pWrker->cntlrId,expInfo->InitCodefile);
    if ( ((int) strlen(expInfo->InitCodefile) > 0) )
    {
       initPSflag = 1;
       sprintf(cntlrFileName,"%s.%s",expInfo->InitCodefile,pWrker->cntlrId);
       DPRINT2(+1,"'%s': Send Init Code File: '%s'\n",pWrker->cntlrId,cntlrFileName);
       result = xferFile(expInfo,initPSflag,pWrker->cntlrId,pWrker->PubId,pWrker->pNddsBufMngr /*pWrker->SubPipeFd*/,
				cntlrFileName,pWrker->label,&(pWrker->numSubcriber4Pub));
       if (result < 0)
       {
          /* sendExpStat("endExp"); */
          DPRINT1(+1,"'%s': downLoadExpData:  send Completion of Xfer message to controller\n",pWrker->cntlrId);
          sendXferCmplt(pWrker->PubId);

          errLogRet(ErrLogOp,debugInfo,
		"'%s': sendCodes: Init File unable to be  sent. ExpState: %d\n",
		 pWrker->cntlrId,expInfo->ExpState);
          return(-1);
        /* Need some type of msge back to Expproc to indicate failure */
       }
    }

    DPRINT2(+1,"'%s', downLoadExpData: Send Pattern/WForm File: '%s'\n",pWrker->cntlrId,expInfo->WaveFormFile);
    /* if ( ((int) strlen(expInfo->WaveFormFile) > 0) ) */
    {
       initPSflag = 0;
       /* sprintf(cntlrFileName,"%s.pat.%s",expInfo->WaveFormFile,pWrker->cntlrId); */
       sprintf(cntlrFileName,"%s.pat.%s",expInfo->PSCodefile,pWrker->cntlrId);
       DPRINT2(+1,"'%s': downLoadExpData: Send Pattern/WF File: '%s'\n",pWrker->cntlrId,cntlrFileName);
       result = xferFile(expInfo,initPSflag,pWrker->cntlrId,pWrker->PubId,pWrker->pNddsBufMngr/*pWrker->SubPipeFd*/,cntlrFileName,pWrker->label,&(pWrker->numSubcriber4Pub));
       if (result == -99)
       {
          DPRINT1(+1,"'%s': downLoadExpData:  Pattern Download Aborted\n",pWrker->cntlrId);
          sendXferCmplt(pWrker->PubId);
          return(-99);
       }
       else if ((result < 0) && (result != -2))   /* no pattern is OK, error return -2 */
       {
          DPRINT1(+1,"'%s': downLoadExpData:  send Completion of Xfer message to controller\n",pWrker->cntlrId);
          sendXferCmplt(pWrker->PubId);

          errLogRet(ErrLogOp,debugInfo,
		"'%s': downLoadExpData: Pattern/WF File unable to be  sent. ExpState: %d\n",
		 pWrker->cntlrId,expInfo->ExpState);
          return(-1);
        /* Need some type of msge back to Expproc to indicate failure */
       }
    }

    DPRINT2(+1,"'%s': downLoadExpData: Send Pulse Sequence File: '%s'\n",pWrker->cntlrId,expInfo->PSCodefile);
    if ( ((int) strlen(expInfo->PSCodefile) > 0) )
    {
       initPSflag = 0;
       sprintf(cntlrFileName,"%s.%s",expInfo->PSCodefile,pWrker->cntlrId);
       DPRINT2(+1,"'%s': downLoadExpData: Send Init Code File: '%s'\n",pWrker->cntlrId,cntlrFileName);
       result = xferFile(expInfo,initPSflag,pWrker->cntlrId,pWrker->PubId,pWrker->pNddsBufMngr /*pWrker->SubPipeFd*/,
                            cntlrFileName,pWrker->label,&(pWrker->numSubcriber4Pub));
       if (result == -99)
       {
          DPRINT1(+1,"'%s': downLoadExpData:  Acode Download Aborted\n",pWrker->cntlrId);
          sendXferCmplt(pWrker->PubId);
          return(-99);
       }
       else if (result < 0)
       {
          /* sendExpStat("endExp"); */
          DPRINT1(+1,"'%s': downLoadExpData:  send Completion of Xfer message to controller\n",pWrker->cntlrId);
          sendXferCmplt(pWrker->PubId);

          errLogRet(ErrLogOp,debugInfo,
		"'%s': downLoadExpData: pulse Sequence File unable to be  sent. ExpState: %d\n",
		 pWrker->cntlrId,expInfo->ExpState);
          return(-1);
        /* Need some type of msge back to Expproc to indicate failure */
       }
    }

    DPRINT1(+1,"'%s': downLoadExpData:  send Completion of Xfer message to controller\n",pWrker->cntlrId);
    sendXferCmplt(pWrker->PubId);

    return( 0 );
}

/*
 * Called by the last thread that finishes, which will unmap the files
 *
 */
unMapDownLoadExpData(cntlr_t *pWrker,  void *expinfo, char *bufRootName)
{
       DPRINT2(+1,"'%s': Unmapping files for '%s'.\n",pWrker->cntlrId,bufRootName);
#ifndef SENDTEST
       mapOutExp(&ActiveExpInfo);
#endif
}

off_t getXferFileSizes(char *filename)
{
    struct stat filestat;
    off_t filesize;

    if (stat(filename,&filestat) != 0)
      filesize = (off_t) 0;
    else
      filesize = filestat.st_size;
    DPRINT2(+4,"file: '%s' is %lu bytes\n",filename,filesize);
    return(filesize);
}

off_t determineAggregateSizes( SHR_EXP_INFO expInfo, char *cntlrName)
{
    char patternfile[256];
    off_t total_size;

    total_size = (off_t) 0;
    if ( ((int) strlen(expInfo->AcqRTTablefile) > 0) )
       total_size += getXferFileSizes(expInfo->AcqRTTablefile);
    DPRINT1(+4,"totalsize %lu bytes\n",total_size);
    if ( ((int) strlen(expInfo->InitCodefile) > 0) )
    {
       sprintf(patternfile,"%s.%s",expInfo->InitCodefile,cntlrName);
       total_size += getXferFileSizes(patternfile);
    }
    DPRINT1(+4,"totalsize %lu bytes\n",total_size);
    sprintf(patternfile,"%s.pat.%s",expInfo->PSCodefile,cntlrName);
    total_size += getXferFileSizes(patternfile);
    DPRINT1(+4,"totalsize %lu bytes\n",total_size);
    return (total_size);
}

int xferFile(SHR_EXP_INFO expInfo, int initPSflag, char *CntlrName,
             NDDS_ID pubId, int *subPipeFd, char *filename, char *labelName,
             int *numOfSubs)
{
   int stat;
   MFILE_ID ifile;
   ComboHeader *globalHeader;
   uint64_t size;

   if ( access(filename, R_OK) )
   {
      /* file does not exist */
     /* not a fatal error if pattern file,etc. doesn't exist */
     /* errLogSysRet(LOGOPT,debugInfo,"file %s, doesn't exist or read access denied\n",filename); */
      return(-2);
   }
   ifile = mOpen(filename,0,O_RDONLY);
   DPRINT3(+1,"'%s': xferRTFile(): mOpen: '%s', fileHdl: 0x%lx\n",CntlrName,filename,ifile);
   if (ifile == NULL)
   {
     errLogSysRet(LOGOPT,debugInfo,"could not open %s",filename);
     return(-1);
   }
   DPRINT3(+1,"'%s': xferRTFile(): mOpen: '%s', bytes: %lld\n",CntlrName,filename,ifile->byteLen);
   if (ifile->byteLen == 0)
   {
     /* DPRINT(+1,"size was zero\n"); */
     errLogRet(LOGOPT,debugInfo,"'%s': sendFile: '%s' is Empty\n",CntlrName,filename);
     mClose(ifile);
     return(-1);
   }

  globalHeader = (ComboHeader *) ifile->offsetAddr;
   switch( htonl(globalHeader->comboID_and_Number) & 0xf0000000)
   {
       case INITIALHEADER:
	    DPRINT1(+1,"'%s': Initital Header\n",CntlrName);
            stat = xferLcAndTables(ifile, CntlrName, pubId, subPipeFd, labelName, numOfSubs);
	    break;
       case ACODEHEADER:
	   DPRINT1(+1,"'%s': Acode Header\n",CntlrName);
           if (initPSflag == 1)
              stat = xferPatterns(expInfo, ifile, CntlrName, pubId, subPipeFd, labelName, numOfSubs);
           else
              stat = xferAcodes(expInfo, ifile, CntlrName, pubId, subPipeFd, labelName, numOfSubs);
	   break;
       case PATTERNHEADER:
	   DPRINT1(+1,"'%s': Pattern Header\n",CntlrName);
           stat = xferPatterns(expInfo, ifile, CntlrName, pubId, subPipeFd, labelName, numOfSubs);
	   break;

       case TABLEHEADER:
			errLogRet(LOGOPT,debugInfo,"Tables Download, not implemented\n");
                        stat = -1;
			break;
       case POSTEXPHEADER:
			errLogRet(LOGOPT,debugInfo,"POSTEXP  Download, not implemented\n");
                        stat = -1;
			break;
       default:
			errLogRet(LOGOPT,debugInfo,"Unknow Header Type\n");
                        stat = -1;
			break;
    }

    mClose(ifile);
    return(stat);
}

xferLcAndTables(MFILE_ID ifile, char *CntlrName, NDDS_ID pubId, int *subPipeFd, char *labelName, int *numOfSubs)
{
   char cmdstr[256];
   char label[30];
   int lcsize,bytes,stat,tblnum,tblsize;
   uint64_t size,sizeleft;
   ComboHeader *globalHeader;
  /*  Format:Cmd BufType Label(base) Size(max) number start_number  */

  stat = 0;
  sizeleft = size = ifile->byteLen;
  DPRINT2(+1,"'%s': xferLcAndTables: file size: %lld\n",CntlrName,size);

  globalHeader = (ComboHeader *) ifile->offsetAddr;
  /* send the combo header down to console too */
  /* ifile->offsetAddr += sizeof(ComboHeader); */

  /* printCombo(globalHeader); */

  if ( htonl(globalHeader->sizeInBytes) != (sizeof(struct _lc)+sizeof(autodata)))
  {
    errLogRet(LOGOPT,debugInfo,"'%s': xferLcAndTables: '%s' header size mismatch\n",CntlrName,ifile->filePath);
    return(-1);
  }

  lcsize = (sizeof(ComboHeader)+sizeof(struct _lc)+sizeof(autodata));

  /* sprintf(label,"%slc",labelName); */
  genLabel(globalHeader,labelName,label);
  DPRINT4(+1,"'%s': xferLcAndTables: LC Addr 0x%lx, lcsize: %d,labelKey: '%s'\n",CntlrName,ifile->offsetAddr, lcsize,label);


  /* DPRINT(+1,"call writeToConsole()\n"); */
  bytes = writeToConsole(CntlrName, pubId, label, ifile->offsetAddr, lcsize, 1, 1);

#ifdef USE_XFER_ACKS
*  stat = getXferdAck(pubId,subPipeFd);  /* Lets forget about getting acks for now,  GMB 11-03-2004 */
*
*  /* DPRINT(+1,"returned from getXferdAck()\n"); */
*  DPRINT2(+1,"'%s': xferLcAndTables: Ack Received, status: %d\n",CntlrName,stat);
*  if (stat != 0)
*  {
*     DPRINT2(+1,"'%s': xferLcAndTables: Transfer Error, status: %d\n",CntlrName,stat);
*  }
#endif

  /* determine if there are tables to download */
  sizeleft -= lcsize;
  ifile->offsetAddr += lcsize;
  tblsize = 0;
  while (sizeleft >= sizeof(ComboHeader))
  {
     DPRINT3(+1,"'%s': xferTables: size: %lld, sizeleft: %lld\n",CntlrName,ifile->byteLen,sizeleft);
     ifile->offsetAddr += tblsize;
     globalHeader = (ComboHeader *) ifile->offsetAddr;
     /* printCombo(globalHeader); */

     /* sizeleft -= sizeof(ComboHeader); */
     /* send the conbo header down to console too. */
     /* ifile->offsetAddr += sizeof(ComboHeader); */

     tblnum = htonl(globalHeader->comboID_and_Number) & 0x0ffff; /* pattern number */
     tblsize = htonl(globalHeader->sizeInBytes) + sizeof(ComboHeader);

     /* sprintf(label,"%st%d",labelName,patnum); */
     genLabel(globalHeader,labelName,label);

     DPRINT4(+1,"'%s': tblnum: %d, tblsize: %d, label: '%s'\n", CntlrName,tblnum,tblsize,label);

     bytes = writeToConsole(CntlrName, pubId, label, ifile->offsetAddr, tblsize, 1, 1 );
#ifdef USE_XFER_ACKS
*     stat = getXferdAck(pubId,subPipeFd); /* Lets forget about getting acks for now,  GMB 11-03-2004 */
*     DPRINT2(+1,"'%s': xferTables: Ack Received, status: %d\n",CntlrName,stat);
*     if (stat != 0)
*     {
*        DPRINT2(+1,"'%s': xferTables: Transfer Error, status: %d\n",CntlrName,stat);
*     }
#endif
     sizeleft -= tblsize;
  }
  return(stat);
}

getPatternInfo(MFILE_ID ifile, char *CntlrName, unsigned long *numPats, unsigned long *maxsize, unsigned long *totalsize)
{
   int lcsize,bytes,stat,comboHeaderSize;
   uint64_t sizeleft;
   int patsize,patnum;
   ComboHeader *globalHeader;

   *numPats = *maxsize = *totalsize = 0L;

   stat = 0;

   sizeleft = ifile->byteLen;

   comboHeaderSize = sizeof(ComboHeader);
   patsize = 0;
   while (sizeleft >= comboHeaderSize)
   {
     DPRINT4(+3,"'%s': getPatternInfo(): size: %lld, sizeleft: %lld, combosize: %d\n",
                 CntlrName,ifile->byteLen,sizeleft,comboHeaderSize);
     ifile->offsetAddr += patsize;
     globalHeader = (ComboHeader *) ifile->offsetAddr;
     /* printCombo(globalHeader); */
     patnum = htonl(globalHeader->comboID_and_Number) & 0x0ffff; /* pattern number */
     patsize = htonl(globalHeader->sizeInBytes) + comboHeaderSize;
     (*numPats)++;
     if (patsize > *maxsize)  
        *maxsize = patsize;
     *totalsize = *totalsize + patsize;
     sizeleft -= patsize;
   }
   ifile->offsetAddr = ifile->mapStrtAddr;  /* back to beginning of mmap file */

   return(*numPats);
}

xferPatterns(SHR_EXP_INFO ExpInfo, MFILE_ID ifile, char *CntlrName, NDDS_ID pubId, int *subPipeFd, char *labelName, int *numOfSubs)
{
   char cmdstr[256];
   char label[30];
   int lcsize,bytes,stat,comboHeaderSize;
   uint64_t sizeleft;
   int patsize,patnum;
   ComboHeader *globalHeader;
   unsigned long numPats, maxsize, totalsize;

  /*  Format:Cmd BufType Label(base) Size(max) number start_number  */

  /* for lack of a better way, run through the entire file to find the actual number of 
   *  patterns and the maximum pattern size, and total aggragrate size  we do the following call
   *   GMB 11/15/2005 
   */

  /*  not used yet, future enhancement for downloading large number of patterns 
   *
   * getPatternInfo(ifile, CntlrName, &numPats, &maxsize, &totalsize);
   * DPRINT4(4,"%s': Patterns: number: %lu, MaxSize: %lu, Total Size: %lu (bytes)\n",
   *		CntlrName, numPats, maxsize, totalsize);
   */
		
  stat = 0;

  sizeleft = ifile->byteLen;

  comboHeaderSize = sizeof(ComboHeader);
  /*DPRINT6(+1,"int: %d, ACODEHEADER: %d, TABLEHEADER: %d, PATTERNHEADER: %d, COMBOHEADER: %d, combo: %d\n",
	sizeof(int), sizeof(AcodeHeader), sizeof(TableHeader), sizeof(PatternHeader),
	sizeof(ComboHeader), comboHeaderSize); */

  patsize = 0;
  while (sizeleft >= comboHeaderSize)
  {
     DPRINT4(+1,"'%s': xferPatterns(): size: %lld, sizeleft: %lld, combosize: %d\n",CntlrName,ifile->byteLen,sizeleft,comboHeaderSize);
     ifile->offsetAddr += patsize;
     globalHeader = (ComboHeader *) ifile->offsetAddr;
     /* printCombo(globalHeader); */

     /* sizeleft -= sizeof(ComboHeader); */
     /* send the conbo header down to console too. */
     /* ifile->offsetAddr += sizeof(ComboHeader); */

     patnum = htonl(globalHeader->comboID_and_Number) & 0x0ffff; /* pattern number */
     patsize = htonl(globalHeader->sizeInBytes) + comboHeaderSize;

     if ( (ExpInfo->ExpState >= EXPSTATE_TERMINATE)  )
     {
   	 DPRINT3(+1,"'%s': On Pattern %d, Xfer Aborted: bytes: %d\n",CntlrName,patnum,patsize);
	 return(-99);
     }

     /* sprintf(label,"%sp%d",labelName,patnum); */
     genLabel(globalHeader,labelName,label);

     /* DPRINT5(+1,"patnum: %d, patsize: %d, label: '%s', patelemsize: %d, patnumelemts: %d\n",
		patnum,patsize,label,globalHeader->PAT.pattern_element_size,globalHeader->PAT.numberElements); */
     DPRINT4(+1,"'%s': xferPatterns(): patnum: %d, patsize: %d, label: '%s'\n", CntlrName,patnum,patsize,label);

     bytes = writeToConsole(CntlrName, pubId, label, ifile->offsetAddr, patsize, 1, 1 );
#ifdef USE_XFER_ACKS
*     stat = getXferdAck(pubId,subPipeFd); /* Lets forget about getting acks for now,  GMB 11-03-2004 */
*     DPRINT2(+1,"'%s': xferPatterns(): Ack Received, status: %d\n",CntlrName,stat);
*     if (stat != 0)
*     {
*        DPRINT2(+1,"'%s': xferPatterns(): Transfer Error, status: %d\n",CntlrName,stat);
*     }
#endif
     sizeleft -= patsize;
   }  

  return(stat);
}


xferAcodes(SHR_EXP_INFO ExpInfo, MFILE_ID ifile, char *CntlrName, NDDS_ID pubId, int *subPipeFd, char *labelName, int *numOfSubs)
{
   char label[30];
   int k,bytes,status;
   int codesize,codenum;
   long acqBufsFree;
   int ntimes, nTimes2sendAcodes, startAcode;
   int abortSent;
   long nAcodes, nFIDs;	/* number of Fids or ArrayDim */
   int JpsgFlag; 
   uint64_t fileSize;
   uint64_t sizeleft;
   char *code_ptr;
   ComboHeader *globalHeader;
   int ackInterval;
   int barrierStatus;
   NDDSBUFMNGR_ID pNddsBufMngr;

   abortSent = status = 0;
   ackInterval = 1;

   pNddsBufMngr = (NDDSBUFMNGR_ID) subPipeFd;
  /*  Format:Cmd BufType Label(base) Size(max) number start_number  */

   /* number of acodes in file */
   nAcodes = ExpInfo->NumAcodes;
   nFIDs = ExpInfo->ArrayDim;	/* Total Elements of this Exp. */
   startAcode = ExpInfo->CurrentElem;
   /* maxSize = ExpInfo->acode_max_size; */
   DPRINT5(+1,"'%s': xferAcodes: Num Acodes: %d, FIDs: %d, Jpsg flag: %d, startAcode: %d\n",
	CntlrName,nAcodes,nFIDs,JpsgFlag,startAcode);

   DPRINT5(4,"'%s': xferAcodes: Num Acodes: %d, FIDs: %d, startAcode: %d, maxSize: %d\n",
	CntlrName,nAcodes,nFIDs,startAcode,ExpInfo->acode_max_size);

  /* Experiment aborted ? */
  if ( ExpInfo->ExpState >= EXPSTATE_TERMINATE)
  {
    return(-1);
  }

   /*
   // Used for RA, if Jpsg then nAcodes will be 1, even if nFIDs is greater than 1
   // If nFIDs is one then Std PSG & Jpsg are equivent in sending Acodes to console
   */
   /* JpsgFlag = ((nAcodes == 1) && (nFIDs > 1)) ? 1 : 0; */
   JpsgFlag = (ExpInfo->PSGident == 100) ? 1 : 0;
   DPRINT5(+1,"'%s': xferAcodes: Acodes: %d, FIDs: %d, Jpsg flag: %d, startAcode: %d\n",
	CntlrName,nAcodes,nFIDs,JpsgFlag,startAcode);

   if (JpsgFlag == 1)
      startAcode = 0;

   DPRINT2(1,"'%s': xferAcodes: startAcode value: %d\n",CntlrName,startAcode);

   /* init_compressor(); */

   codesize = 0;

   fileSize = ifile->byteLen;

   /* nAcodes + 1 (extra one for the stat block sent down for abort or complete */
   /* sprintf(cmdstr,"downLoad Fixed %s %d %d %d",bufRootName,maxSize, nAcodes+1,startAcode); */

   /* acqBufsFree =  getXferSize(pubId, subPipeFd); */
   acqBufsFree =  getXferSize(pubId, pNddsBufMngr);
   DPRINT2(1,"'%s': xferAcodes: Initial freeNamedBufs: %d\n",CntlrName,acqBufsFree);
   if (acqBufsFree == -99)
   {
   	DPRINT(+1,"'%s': getXferSize() abort returned, end transfer\n");
	abortSent = 1;
     	status = -99;
	return(-99);
   }

   /* ackInterval = (nAcodes > acqBufsFree) ? acqBufsFree : nAcodes; */
   ackInterval = 50;

   /* -------- Interleaving -----------------------*/
   /* Interleaving & can't fit all acodes down in console */
   /* For Interleave, we take the simple approach whereby we just
        send all the acodes down mulitple times.
	Number of time equals NT / BS, e.g. nt=10, bs=5 10/5 = 2 times
   */
   if ( (ExpInfo->IlFlag != 0) && (nAcodes > 1) )
   {
	nTimes2sendAcodes = 
	 (ExpInfo->NumTrans - ExpInfo->CurrentTran) / ExpInfo->NumInBS;

	if (((ExpInfo->NumTrans - ExpInfo->CurrentTran) % ExpInfo->NumInBS) > 0)
	    nTimes2sendAcodes++;
		
        DPRINT3(2,"'%s': xferAcodes: nAcodes: %lu < acqBufsFree: %lu\n",CntlrName,nAcodes,acqBufsFree);
        /* Interleaving & can fit all acodes down in console, only send once then */
        if ( nAcodes <= acqBufsFree )
        { 
	    nTimes2sendAcodes = 1;
        }
        DPRINT2(+1,"'%s': xferAcodes: Interleave, Times to send Acode set: %d\n",CntlrName,nTimes2sendAcodes);
   }
   else
   {
	nTimes2sendAcodes = 1;
        DPRINT2(+1,"'%s': xferAcodes: Non-Interleave, Times to send Acode set: %d\n",CntlrName,nTimes2sendAcodes);
   }
     
   /* For Non-Interleaved Exp nTimes2sendAcodes = 1,
      For Interleave Exp we will send done the set of Acodes mulitple times
   */
   for ( ntimes = 0; ntimes < nTimes2sendAcodes; ntimes++)
   {

       DPRINT2(1,"'%s': xferAcodes: Set %d of Acodes being sent\n",CntlrName,ntimes+1);
        /* for each new set after the 1st, we must tell downLoader that the Acodes
	   are comming.
        */
        /* k = ExpInfo->FidsCmplt; */
        codesize = 0;
   	DPRINT2(+1,"'%s': xferAcodes: Sending %d Fids\n",CntlrName,nAcodes);
        bytes = 0;

        DPRINT3(+1,"'%s': xferAcodes: offset: 0x%lx, StrtAddr: 0x%lx \n",CntlrName,ifile->offsetAddr,ifile->mapStrtAddr);
        /* Back to beginning of File */
        ifile->offsetAddr = ifile->mapStrtAddr;

        for (k = 0; k < nAcodes; k++)
        {
   	   DPRINT2(+1,"'%s': xferAcodes: Sending Acode: %d\n",CntlrName,k);
    	   if ( (ExpInfo->ExpState >= EXPSTATE_TERMINATE) && (k > 0) )
           {
	      DPRINT1(+1,"'%s': xferAcodes: Acode Transfer -- ABORTED --\n",CntlrName);
              barrierWaitAbort(&TheBarrier);
              
              /* do we even need to inform downloader that transfer was aborted ?? */
              /* XferAborted(pubId, subPipeFd); */
              
   	      DPRINT3(+1,"'%s': On Fid %d, Xfer Aborted: bytes: %d\n",CntlrName,k,bytes);
	      abortSent = 1;
     	      status = -99;
	      return(-99);
	   }

           if ( acqBufsFree <= 0)
           {
                DPRINT1(1,"'%s': Request new numFree\n",CntlrName);
                /* acqBufsFree =  getXferSize(pubId, subPipeFd);  /* if it's zero will hang */
                acqBufsFree =  getXferSize(pubId, pNddsBufMngr);
                if (acqBufsFree == -99)
                {
   	           DPRINT(+1,"'%s': getXferSize() abort returned, end transfer\n");
	           abortSent = 1;
     	           status = -99;
	           return(-99);
                }
                DPRINT2(1,"'%s': Request Received, numFree: %d\n",CntlrName,acqBufsFree);
           }

           globalHeader = (ComboHeader *) ifile->offsetAddr;
           /* send combo header to console too. */
           /* ifile->offsetAddr += sizeof(ComboHeader); */
           code_ptr = (char*) ifile->offsetAddr;
           codenum = htonl(globalHeader->comboID_and_Number) & 0x0ffff; /* Acode number */
           codesize = htonl(globalHeader->sizeInBytes) + sizeof(ComboHeader);


	   if (k >= startAcode)
	   {
              int barflag;

              genLabel(globalHeader,labelName,label);

   	      /* DPRINT6(+1,"'%s': xferAcodes: fid: '%s', adr: 0x%lx, size %d, sn: %d, ackInterval: %d\n",CntlrName,label,code_ptr,codesize,k+1,ackInterval); */
   	      DPRINT5(1,"'%s': xferAcodes: fid: '%s', adr: 0x%lx, size %d, sn: %d\n",CntlrName,label,code_ptr,codesize,k+1);
              status = writeToConsole(CntlrName, pubId, label, ifile->offsetAddr, codesize, k+1, ackInterval );

              /* All threads stay to gether */
              barflag = (((k - startAcode)+1) % 32);
              DPRINT4(+3,"'%s': (k=%d - startAcode=%d)+1  % 32 = barflag = %d\n",CntlrName,k,startAcode,barflag);
              DPRINT2(+1,"'%s': Wait at barrier if Zero: barflag = %d\n",CntlrName,barflag);
              if ( (((k - startAcode)+1) % 32) == 0)
              {
   	         DPRINT2(1,"'%s': Acode: %ld, Wait on Barrier\n",CntlrName,k);
                 barrierStatus = barrierWait( &TheBarrier );
                 if (barrierStatus == -99)
                 {
                     DPRINT1(+1,"'%s': Returned from Aborted Barrier Wait\n",CntlrName);
     	             status = -99;
		     return(-99);
                 }
   	         DPRINT2(1,"'%s': Returned from Barrier, status: %d\n",CntlrName,barrierStatus);
              }

#ifdef USE_XFER_ACKS   /* no longer used, caused hanging of threads */
*              if (((k+1) % ackInterval) == 0)
*              {   
*                 DPRINT1(+1,"'%s': xferAcodes: Wait for AckInterval status.\n",CntlrName);
*                 status = getXferdAck(pubId,subPipeFd);
*                 DPRINT2(+1,"'%s': xferAcodes: Ack Received, status: %d\n",CntlrName,status);
*                 if (status != 0)
*                 {
*                   DPRINT2(+1,"'%s': xferAcodes: Transfer Error, status: %d\n",CntlrName, status);
*                 }
*              }   
#endif
	      acqBufsFree--;
              /* ExpInfo->LastCodeSent = k; */
	   }
           ifile->offsetAddr += codesize;
        }

	/* Interleaving: When done with the last acode set start with	*/
	/* 		 the first acode set until nTimes2sendAcodes.	*/
        if (ExpInfo->IlFlag != 0)
	  startAcode = 0;
     }

     DPRINT2(2,"'%s':  abortSent: %d \n",CntlrName,abortSent);
     if ( abortSent == 0 ) 
     {
        DPRINT(2,"Acode Transfer Complete\n");
	/*
        statusBlk[0] = htonl(XFER_STATUS_BLK);
        statusBlk[1] = htonl(XFER_COMPLETE);
        */

         /* do we need this ? */
        /* XferCmplt(pubId,subPipeFd); */
   	DPRINT2(2,"On Fid %d, Xfer Complete: bytes: %d\n",k,bytes);
     }
  /* Not interleaved and all codes sent down then we are done,
     otherwise maybe not so don't close files
  */
  return(status);
}

/*  
 *
 *   Format:Cmd BufType Label(base) Size(max) number start_number 
 * RT parms - "downLoad,Dynamic,Exp1,64,1,0"
 * Tables   - "downLoad,Dynamic,Exp1,1024,1,0"
 * Tables   - "downLoad,Dynamic,Exp1,1024,1,1"
 * Acodes   - "downLoad,Fixed,Exp1,512,10,0"
 * Acodes   - "downLoad,Fixed,Exp1,512,10,10"
 */

/***************************************************************
* sendTables - Send a named buffer (Global Table) to the console
*
*   Args.   1)	  Name of file containing table(s).
*	    2)	  Order in file of 1st table to download (starting from 0).
*	    3)	  Name to give 1st table (e.g. "exp1t0updt").
*	    4)	  Order of 2nd table to download.
*	    5)	  Name to give second table.
*	    ...
*	    2n)	  Order in file of nth table to send.
*	    2n+1) Name to give nth table
*/
int
sendTables(char *argstr)
  /* NOTE:
   * The "argstr" argument has been processed with strtok()--resulting in
   * a null being substituted for the first delimiter.
   * We ignore the "argstr" parameter, and continue
   * processing with strtok(NULL, ...).  Of course, this will fail
   * miserably if the parser is changed so that it no longer uses strtok(),
   * or if another call to strtok() occurs between these two.
   */
{
    char *args;
    char *filename;
    int rtn;
    char *sn;
    char *tblname;

    if ((filename = strtok(NULL," ")) == NULL){
        errLogRet(LOGOPT,debugInfo,
		"sendTables: No table file specified\n");
	return -1;
    }

    /* Send all the tables specified in the arg list */
    while ((sn=strtok(NULL, " ")) && (tblname=strtok(NULL," "))){
	if (rtn=sendTable(filename, atoi(sn), tblname)){
	    /* If error, send no more tables */
	    break;
	}
    }

    return rtn;
}

/***************************************************************
* sendDelTables - Send and delete a named buffer (Global Table) to the console
*
*   Args.   1)	  Name of file containing table(s).
*	    2)	  Order in file of 1st table to download (starting from 0).
*	    3)	  Name to give 1st table (e.g. "exp1t0updt").
*	    4)	  Order of 2nd table to download.
*	    5)	  Name to give second table.
*	    ...
*	    2n)	  Order in file of nth table to send.
*	    2n+1) Name to give nth table
*/
int
sendDelTables(char *argstr)
  /* NOTE:
   * The "argstr" argument has been processed with strtok()--resulting in
   * a null being substituted for the first delimiter.
   * We ignore the "argstr" parameter, and continue
   * processing with strtok(NULL, ...).  Of course, this will fail
   * miserably if the parser is changed so that it no longer uses strtok(),
   * or if another call to strtok() occurs between these two.
   */
{
    char *args;
    char *filename;
    int rtn;
    char *sn;
    char *tblname;

    if ((filename = strtok(NULL," ")) == NULL){
        errLogRet(LOGOPT,debugInfo,
		"sendTables: No table file specified\n");
	return -1;
    }

    /* Send all the tables specified in the arg list */
    while ((sn=strtok(NULL, " ")) && (tblname=strtok(NULL," "))){
	if (rtn=sendTable(filename, atoi(sn), tblname)){
	    /* If error, send no more tables */
	    break;
	}
    }
    unlink(filename);
    return rtn;
}

sendTable(char *fname,		/* Name of table file */
	  int tab_nbr,		/* Position in file of table to download */
	  char *tname)		/* Name to give the downloaded table */
{
}

/****************************************************/
/****************************************************/
/****************************************************/
/****************************************************/

#define NCBFX	  200	/* more than psg's 32 */
struct _compr_
{
   unsigned char *ucpntr;
   int size;
} ref_table[NCBFX];

static void
init_compressor()
{
  int i;
  for (i=0; i < NCBFX; i++)
  {
    ref_table[i].ucpntr = NULL;
    ref_table[i].size   = 0;
  }
}

int decompress(unsigned char *dest, unsigned char *src,
               unsigned char *ref, int num)
{
   int xcnt;
   int k;
   xcnt=0;
   while (num-- > 0)
   {
     /* all is keyed off 0 in stream */
     if (*src != 0)
     {
       *dest++ =   *ref++ ^ *src++;  /* uncompressed case */
       xcnt++;
     }
     else 
     {
       src++;  /* 2 count field */
       k = *src++; /* src is next */
       num--;
       while (k-- > 0)
       {
         *dest++ = *ref++;
         xcnt++;
       }
     }
   }
   return(xcnt);
}
      
install_ref(int size,int bufn,unsigned char *ptr)
{
   /* test is programming error test */
   if (ref_table[bufn].size != 0) 
   {
      if (ref_table[bufn].size != size)
	fprintf(stderr,"size mismatch %d in table %d in this buf\n",
	   ref_table[bufn].size,size);
      return;  /*  CAREFUL */
   }
   ref_table[bufn].ucpntr = ptr;
   ref_table[bufn].size = size;
}

unsigned char *get_ref(int num)
{
   if ((num < 0) || (num >= NCBFX))
   {
     fprintf(stderr,"no such reference %d\n",num);
     return(NULL);
   }
   return(ref_table[num].ucpntr);
}

#define NO_CODE    0
#define REF_CODE  64
#define CMP_CODE 128

/*---------------------------------------------------------------------
+--------------------------------------------------------------------*/
unsigned char *
send_code(unsigned char *addr, unsigned char *tmpptr, int codesize, int buf_num)
{
    int bytes;
    int index_x;
    unsigned char *codes;
    long codeoffset;
    int action;
    int full_size;
    unsigned char *pp1;

    /***************************************************/
    /*  sizes 					       */
    /***************************************************/

    action= 0x0c0 & buf_num;
    full_size = ((unsigned long) (0xffffff00 & buf_num)) >> 8;
    buf_num  &= 0x03f; 
    codes = addr;
    DPRINT3(1,"send_code(): action= %d full_size= %d buffer= %d\n",
                    action,full_size,buf_num);
    /*******************************************
    ** decipher action 
    ** 
    *******************************************/
    switch (action) 
    {
	case REF_CODE: install_ref(full_size,buf_num,addr); 
        case NO_CODE:   break;
	case CMP_CODE: 
	 pp1 = get_ref(buf_num);
	 if (pp1 == NULL)
	 {
	   /* see if you can look it up!! */
	    fprintf(stderr,"readexpcode(): no reference acode\n");
	   return(0);
	 }
	 index_x=decompress(tmpptr,addr,pp1,codesize);

	 if (index_x != full_size) 
	 {
	    fprintf(stderr,"readexpcode(): decompress %d ,expected  %d \n",
                    index_x,full_size);
	    perror("READEXPCODE decompress mismatch\n");
	    exit(-1);
	 }
	 codes = tmpptr;
	 break;
     }
    return(codes);
}
static void
get_code_info(unsigned char *addr, unsigned char *out)
{
   int i;

   for (i=0; i < 2 * sizeof(long); i++)
      *out++ = *addr++;
}

/****************************************************/
/****************************************************/
/****************************************************/
/****************************************************/

#ifndef SENDTEST
abortCodes(char *str)
{
   DPRINT(1,"abortCodes: \n");
   return(0);
}

terminate(char *str)
{
   extern int shutdownMsgQ();
   DPRINT(1,"terminate: \n");
   /* shutdownComm();  in commfuncs.c which is not used by nirvana */
   exit(0);
}

ShutDownProc()
{
   shutdownComm();  /* shutdown ndds subscriptions & publications */
   resetState();
}

resetState()
{
  labelBase[0] = (char) NULL;
  nAcodes = 0;
  if (mapAcodes != NULL)
  {
     mClose(mapAcodes);
  }
  if ( CodeList != NULL)
  {
     free(CodeList);
  }
  if ((int) strlen(ActiveExpInfo.ExpId) > 1)
  {
    mapOutExp(&ActiveExpInfo);
  }
}

debugLevel(char *str)
{
    extern int DebugLevel;
    char *value;
    int  val;
    value = strtok(NULL," ");
    val = atoi(value);
    DPRINT1(1,"debugLevel: New DebugLevel: %d\n",val);
    DebugLevel = val;
    return(0);
}


mapInExp(ExpEntryInfo *expid)
{
    DPRINT1(2,"mapInExp: map Shared Memory Segment: '%s'\n",expid->ExpId);

    expid->ShrExpInfo = shrmCreate(expid->ExpId,SHR_EXP_INFO_KEY,(unsigned long)sizeof(SHR_EXP_STRUCT)); 
    if (expid->ShrExpInfo == NULL)
    {
       errLogSysRet(LOGOPT,debugInfo,"mapInExp: shrmCreate() failed:");
       return(-1);
    }
    if (expid->ShrExpInfo->shrmem->byteLen < sizeof(SHR_EXP_STRUCT))
    {
        /* hey, this file is not a shared Exp Info file */
       shrmRelease(expid->ShrExpInfo);         /* release shared Memory */
       unlink(expid->ExpId);        /* remove filename that shared Mem created */
       expid->ShrExpInfo = NULL;
       return(-1);
    }


#ifdef DEBUG
    if (DebugLevel >= 3)
       shrmShow(expid->ShrExpInfo);
#endif

    expid->ExpInfo = (SHR_EXP_INFO) shrmAddr(expid->ShrExpInfo);

    /* Should open the shared Exp status shared structure here */

    return(0);
}

mapOutExp(ExpEntryInfo *expid)
{
    DPRINT1(2,"mapOutExp: unmap Shared Memory Segment: '%s'\n",expid->ExpId);

    if (expid->ShrExpInfo != NULL)
      shrmRelease(expid->ShrExpInfo); 
    
    if (expid->ShrExpStatus != NULL)
      shrmRelease(expid->ShrExpStatus); 

    memset((char*) expid,0,sizeof(ExpEntryInfo));  /* clear struct */

    return(0);
}

mapIn(char *str)
{
    char* filename;

    filename = strtok(NULL," ");

    DPRINT1(1,"mapIn: map Shared Memory Segment: '%s'\n",filename);

    ShrExpInfo = shrmCreate(filename,SHR_EXP_INFO_KEY,(unsigned long)sizeof(SHR_EXP_STRUCT)); 
    if (ShrExpInfo == NULL)
    {
       errLogSysRet(LOGOPT,debugInfo,"mapIn: shrmCreate() failed:");
       return(-1);
    }

#ifdef DEBUG
    if (DebugLevel >= 3)
      shrmShow(ShrExpInfo);
#endif

    expInfo = (SHR_EXP_INFO) shrmAddr(ShrExpInfo);

    return(0);
}

mapOut(char *str)
{
    char* filename;

    filename = strtok(NULL," ");

    DPRINT1(1,"mapOut: unmap Shared Memory Segment: '%s'\n",filename);

    shrmRelease(ShrExpInfo);
    ShrExpInfo = NULL;
    expInfo = NULL;
    return(0);
}


/* dummy the SIGUSR2 routines, now use shrexpinfo.h ExpState to determine aborts, etc. */

blockAbortTransfer() { return(0); }

unblockAbortTransfer() { return(0); }

setupAbortXfer() { return(0); }


sendVME(char *argstr )
{
    errLogRet(LOGOPT,debugInfo,"sendVME: not consistent with Nirvana\n");
    /* deliverMessage( "Expproc", "dwnldComplete  \nBad Parameters\n0x00eeeeee\n"); */
        return( -1 );
}

#endif

printCombo(ComboHeader *globalHeader)
{
    DPRINT2(+1,"ComboID_Number: 0x%lx, size: %d\n", htonl(globalHeader->comboID_and_Number), htonl(globalHeader->sizeInBytes) );
    switch( htonl(globalHeader->comboID_and_Number) & 0xf0000000)
    {
       case INITIALHEADER:
			DPRINT(+1,"Initital Header\n");
			break;
       case ACODEHEADER:
			DPRINT(+1,"Acode Header\n");
			break;
       case PATTERNHEADER:
			DPRINT(+1,"Pattern Header\n");
			break;
       case TABLEHEADER:
			DPRINT(+1,"Table Header\n");
			break;
       case POSTEXPHEADER:
			DPRINT(+1,"PostExp Header\n");
			break;
       default:
			DPRINT(+1,"Unknown Header\n");
			break;
    }
   return; 
}

char *genLabel(ComboHeader *globalHeader,char *baselabel,char* label)
{
    int number;
    char *retLabel = label;

    number = htonl(globalHeader->comboID_and_Number) & 0x0fffffff;
    switch( htonl(globalHeader->comboID_and_Number) & 0xf0000000)
    {
       case INITIALHEADER:
  		        sprintf(label,"%slc%d",baselabel,number);
			/* DPRINT(+1,"Initital Header\n"); */
			break;
       case ACODEHEADER:
  		        sprintf(label,"%sf%d",baselabel,number);
			/* DPRINT(+1,"Acode Header\n"); */
			break;
       case PATTERNHEADER:
  		        sprintf(label,"%sp%d",baselabel,number);
			/* DPRINT(+1,"Pattern Header\n"); */
			break;
       case TABLEHEADER:
  		        sprintf(label,"%st%d",baselabel,number);
			/* DPRINT(+1,"Table Header\n"); */
			break;
       case POSTEXPHEADER:
  		        sprintf(label,"%spost%d",baselabel,number);
			/* DPRINT(+1,"PostExp Header\n"); */
			break;
       default:
                        retLabel = NULL;
			DPRINT(+1,"Unknown Header\n");
			break;
    }
   return retLabel; 
}

#ifdef SENDTEST
printShrExpInfo(SHR_EXP_INFO expInfo)
{
   printf("ExpDuration: %lf\n", expInfo->ExpDur);
   printf("DataSize: %lld\n", expInfo->DataSize);
   printf("ArrayDim: %ld\n", expInfo->ArrayDim);
   printf("FidSize: %ld\n", expInfo->FidSize);
   printf("NumAcodes: %ld\n", expInfo->NumAcodes);
   printf("NumTables: %ld\n", expInfo->NumTables);
   printf("NumFids: %ld\n", expInfo->NumFids);
   printf("NumTrans: %ld\n", expInfo->NumTrans);
   printf("NumInBS: %ld\n", expInfo->NumInBS);
   printf("NumDataPts: %ld\n", expInfo->NumDataPts);
   printf("ExpNum: %ld\n", expInfo->ExpNum);
   printf("ExpFlags: %ld\n", expInfo->ExpFlags);
   printf("IlFlag: %ld\n", expInfo->IlFlag);
   printf("GoFlag: %ld\n", expInfo->GoFlag);
   printf("InetPid: %ld\n", expInfo->InetPid);
   printf("InetPort: %ld\n", expInfo->InetPort);
   printf("InetAddr: %ld\n", expInfo->InetAddr);
   printf("MachineID: '%s'\n", expInfo->MachineID);
   printf("InitCodefile: '%s'\n", expInfo->InitCodefile);
   printf("PreCodefile: '%s'\n", expInfo->PreCodefile);
   printf("PSCodefile: '%s'\n", expInfo->PSCodefile);
   printf("PostCodefile: '%s'\n", expInfo->PostCodefile);
   printf("AcqRTTablefile: '%s'\n", expInfo->AcqRTTablefile);
   printf("RTParmFile: '%s'\n", expInfo->RTParmFile);
   printf("WaveFormFile: '%s'\n", expInfo->WaveFormFile);
   printf("DataFile: '%s'\n", expInfo->DataFile);
   printf("CurExpFile: '%s'\n", expInfo->CurExpFile);
   printf("UsrDirFile: '%s'\n", expInfo->UsrDirFile);
   printf("SysDirFile: '%s'\n", expInfo->SysDirFile);
   printf("UserName: '%s'\n", expInfo->UserName);
   printf("AcqBaseBufName: '%s'\n", expInfo->AcqBaseBufName);
}
#endif
