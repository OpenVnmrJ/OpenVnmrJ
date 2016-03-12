/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* downLinker.c 11.1 07/09/07 - DownLinker Task - Receives Data from Host */
/* 
 */

/*
modification history
--------------------
8-10-94,ph  created 
10-17-94, gmb  refined and put into SCCS
*/

/*
DESCRIPTION

  This Task  handles the Acodes & Xcode DownLoad to the Console computer.
  It Waits on a channel which is written into by sendproc on the Host. 
  Its actions depending on the commands it receives.

*/


#define _POSIX_SOURCE /* defined when source is to be POSIX-compliant */
#include <vxWorks.h>
#include <stdioLib.h>
#include <semLib.h>
#include <memLib.h>
#include <msgQLib.h>
#include "hostAcqStructs.h"
#include "hostMsgChannels.h"
#include "expDoneCodes.h"
#include "errorcodes.h"
#include "logMsgLib.h"
#include "namebufs.h"
#include "sysflags.h"
#include "instrWvDefines.h"

extern MSG_Q_ID pMsgesToAParser;/* MsgQ used for Msges to Acode Parser */
extern MSG_Q_ID pMsgesToXParser;/* MsgQ used for Msges to Xcode Parser */
extern MSG_Q_ID pMsgesToAupdt;	/* MsgQ used for Msges to AUpdt */
extern MSG_Q_ID pMsgesToPHandlr;

extern EXCEPTION_MSGE GenericException;

/* Fixed & Dynamic Named Buffers */
extern DLB_ID  pDlbDynBufs;
extern DLB_ID  pDlbFixBufs;

/* 
** downlink format 
** 1. keystring = "downLink",":",
** 2. Buffering_Type," ",
** 3. Label or BaseLable," ",
** 3. Size(dyn) or MaxSize(fixed) in bytes
** 4. Number of Buffers being sent (fixed)
** 5. Starting Number (fixed) used to gen. postfix (E.G. BaseLabel+f%d -> Exp1f1)
*/

static int  dwnLnkrIsCon = 0;
static char *ErrStr = { "ERR" };
static char *OkStr = { "OK" };
static char RespondStr[20];
static int  DownLkrPrior;

#define DOWNLINKER_SUSPEND 1
#define DOWNLINKER_DUMPDATA 2
static int  DownLinkerAbort = 0;
static int  DownLinkerActive = 0;

int
receiveVmeDownload( int chanId, char *vmeaddr, int dwnldsize )
{
   int	dwnldrem, xfercount, xfersize;

   dwnldrem = dwnldsize;
   while (dwnldrem > 0) 
   {
      if (dwnldrem > XFR_SIZE)
         xfersize = XFR_SIZE;
      else
         xfersize = dwnldrem;

      /* xfercount = ReadChannel( chanId, vmeaddr, xfersize ); */
      /* if connection broken will send msge to phandler & suspend task */
      xfercount = phandlerReadChannel( chanId, vmeaddr, xfersize );

      if (xfercount < 1)
         return( xfercount );

      dwnldrem = dwnldrem - xfercount;
      vmeaddr += xfercount;
   }

   return( dwnldsize );
}

int 
parseDlInstr(char *instr,char *cmd,char *bufType, char *label,int *size,int *number,int *startn)
{
   int k;
   k = sscanf(instr,"%s %s %s %d %d %d",cmd,bufType,label,size,number,startn);
   if (k < 2)
     return(-1); 
   return(1);
}

/* called went communications is lost with Host */
restartDwnLnk()
{
   int tid;
   DPRINT(0,"restartDwnLnk:");
   if ( dwnLnkrIsCon == 1)
   {
      markBusy(DOWNLINKER_FLAGBIT);
      DPRINT(0,"taskRestart of tDownLink:");
      dwnLnkrIsCon = 0;
      if ((tid = taskNameToId("tDownLink")) != ERROR)
        taskRestart(tid);
   }
}

downlink(MSG_Q_ID pMsgsToXPMGR,DLB_ID DynPool,DLB_ID FixPool)
{
   int	bytes;
   int 	chan_no;
   ulong_t count,block;
   int totsize;
   int size,number,startn;
   char *tail;
   char *dataAddr;
   char hbuf[250],label[64],cmd[64],buftype[64],resp[64];
   char okb[DLRESPB],suxb[DLRESPB];
   DLBP hit;
   static int downLoad(int chan_no, char *buftype,char *label,int size,int number,int startn);


   char *token,**pLast;

   DownLinkerAbort = 0;
   DownLinkerActive = 0;
   chan_no = SENDPROC_CHANNEL;

   DPRINT(1,"Downlink: Establish Connection to Host.\n");
   EstablishChannelCon(chan_no);
   DPRINT(1,"Downlink: Connection Established.\n");

   DPRINT(1,"Downlink:Server LOOP Ready & Waiting.\n");
   strcpy(okb,"OK");
   strcpy(suxb,"SUX");
   dwnLnkrIsCon = 1;
   bytes = flushChannel(chan_no);
   DPRINT2(0,"downlink: %d bytes flushed from channel %d\n",bytes,chan_no);
   FOREVER
   {
       /* we used to reset DownLinkerAbort to zero here, however if the abort came during
	 a dynamic download the  flag got reset and the Fix download was never aborted properly
	 thus the DownLinkerAbort is now reset to Zero in phandler just after systemReady returns.
         DownLinkerAbort = 0;
       */
#ifdef INSTRUMENT
           wvEvent(EVENT_DOWNLINKER_READY,NULL,NULL);
#endif
      markReady(DOWNLINKER_FLAGBIT);
      DPRINT1(0,"%d bytes in channel prior cmd read\n",bytesInChannel(chan_no));
      /* if connection broken will send msge to phandler & suspend task */
      bytes = phandlerReadChannel(chan_no, hbuf, DLCMD_SIZE);
      /* bytes = readChannel( chan_no, hbuf, DLCMD_SIZE); */

      DPRINT1(1,"\nDownlink: Got Cmd, %d bytes\n",bytes);
#ifdef INSTRUMENT
           wvEvent(EVENT_DOWNLINKER_BUSY,NULL,NULL);
#endif
      markBusy(DOWNLINKER_FLAGBIT);

      if (parseDlInstr(hbuf,cmd,buftype,label,&size,&number,&startn) < 1)
      {
	 errLogRet(LOGIT,debugInfo,"DownLink: Bad Instruction");
         bytes = flushChannel(chan_no);
         DPRINT2(0,"downlink: %d bytes flushed from channel %d\n",bytes,chan_no);
	 /* rWriteChannel(chan_no,ErrStr,DLRESPB); */
	 continue;
      }
      tail = hbuf + strspn(hbuf, " ") + strlen(cmd);
      tail += strspn(tail, " ");

      if (strncmp(cmd,"downLoad",strlen("downLoad")) == 0)
      {
#ifdef INSTRUMENT
           wvEvent(EVENT_DOWNLINKER_CMD_DWNLD,NULL,NULL);
#endif
         downLoad(chan_no,buftype,label,size,number,startn);
	 DPRINT(1,"downLoad(): Returned\n");
      }
      else if (strncmp(cmd,"startExp",strlen("startExp")) == 0)
      {
          sprintf(RespondStr,"OK");
	  phandlerWriteChannel(chan_no,RespondStr,DLRESPB);
          DownLinkerActive = 1;
      DPRINT(0,"DownLinkerActive set to 1 from Start Exp\n");
      }
      else if (strncmp(cmd,"endExp",strlen("endExp")) == 0)
      {
          sprintf(RespondStr,"OK");
	  phandlerWriteChannel(chan_no,RespondStr,DLRESPB);
          DownLinkerActive = 0;
      DPRINT(0,"DownLinkerActive set to 0 from End Exp\n");
          if (DownLinkerAbort == DOWNLINKER_SUSPEND)  /* aborted */
          {
           DPRINT(0,"endExp: Suspend DwnLink, Aborted\n");
	   taskSuspend(0);
          }
      }
      else if (strncmp(cmd,"XParseCmd",strlen("XParseCmd")) == 0)
      {
#ifdef INSTRUMENT
           wvEvent(EVENT_DOWNLINKER_CMD_XPARSE,NULL,NULL);
#endif
	if (pMsgesToXParser != NULL)
          msgQSend(pMsgesToXParser,tail,strlen(tail)+1,NO_WAIT,MSG_PRI_NORMAL);
        else
         errLogRet(LOGIT,debugInfo,"downlink: XParser MsgQ not initialized.\n");
      }
      else if (strncmp(cmd,"AParseCmd",strlen("AParseCmd")) == 0)
      {
#ifdef INSTRUMENT
           wvEvent(EVENT_DOWNLINKER_CMD_APARSE,NULL,NULL);
#endif
	if (pMsgesToAParser != NULL)
          msgQSend(pMsgesToAParser,tail,strlen(tail)+1,NO_WAIT,MSG_PRI_NORMAL);
        else
         errLogRet(LOGIT,debugInfo,"downlink: AParser MsgQ not initialized.\n");
      }
      else if (strncmp(cmd,"AUpdtCmd",strlen("AUpdtCmd")) == 0)
      {
#ifdef INSTRUMENT
           wvEvent(EVENT_DOWNLINKER_CMD_AUPDT,NULL,NULL);
#endif
	if (pMsgesToAupdt != NULL){
	    msgQSend(pMsgesToAupdt, tail,strlen(tail)+1,NO_WAIT,MSG_PRI_NORMAL);
	    strcpy(RespondStr, "OK");
        }else{
	    errLogRet(LOGIT,debugInfo,"downlink: Aupdt MsgQ not initialized\n");
	    strcpy(RespondStr, "NQ"); /* No Queue */
	}
        /* if connection broken will send msge to phandler & suspend task */
	phandlerWriteChannel(chan_no,RespondStr,DLRESPB);
      }
   }
   closeChannel( chan_no );
}

static DLBP pDwnLdBuf = NULL;

int clearCurrentNameBuffer()
{
   if (pDwnLdBuf == NULL)
      return(-1);
   pDwnLdBuf->status = READY;
    return(0);
}

static int
downLoad(int chan_no, char *buftype,char *label,int size,int number,int startn)
{
  int done,n,bytes,retrys;
  char newLabel[128];
  long tableSize;
  /* DLBP pDwnLdBuf;   made static so that a call can be made to reset status buffer */

  DPRINT5(1,"Buf: '%s', Label: '%s', size: %d, num: %d, start#: %d\n",
		buftype,label, size, number, startn);


  /* -----------  D Y N A M I C   D O W N L O A D  --------------- */
  if (strncmp(buftype,"Dynamic",strlen("Dynamic")) == 0)
  {
      /* get a Dynamic Type Named Buffer, number is always assumed tobe One */
#ifdef INSTRUMENT
           wvEvent(EVENT_DOWNLINKER_GETBUF,NULL,NULL);
#endif
    pDwnLdBuf = dlbMakeEntry(pDlbDynBufs,label,size);
    if (pDwnLdBuf == NULL)
    {
      if (acqerrno == S_namebufs_NAMED_BUFFER_ALREADY_INUSE )
      {
         /* for testing just delete the one already there */
         errLogRet(LOGIT,debugInfo,
          "downLoad:Label '%s' Sill in Use, Erasing same name buffer\n",
			label);
         pDwnLdBuf = dlbFindEntry(pDlbDynBufs,label);
         dlbFree(pDwnLdBuf);
         pDwnLdBuf = dlbMakeEntry(pDlbDynBufs,label,size);
         if (pDwnLdBuf == NULL)
         {
           /* tell sendproc TOO BIG & max size */
           sprintf(RespondStr,"2B %ld",memPartFindMax(memSysPartId));
           /* if connection broken will send msge to phandler & suspend task */
           phandlerWriteChannel(chan_no,RespondStr,DLRESPB);
           /* rWriteChannel(chan_no,RespondStr,DLRESPB); */

           errLogRet(LOGIT,debugInfo,
	     "download: Dynamic: Buffer request: %d bytes too large\n",size);

	   GenericException.exceptionType = ALLOC_ERROR;
	   GenericException.reportEvent = SYSTEMERROR + DYNBUFNOSPACE;
	   /* send ALLOC_ERROR (WARNING & ABORT) to exception handler task, 
		it knows what to do */
	    msgQSend(pMsgesToPHandlr, (char*) &GenericException, 
				sizeof(EXCEPTION_MSGE), 
					NO_WAIT, MSG_PRI_URGENT);
            return(-1);
         }
      }
      else
      {
         /* tell sendproc malloc failed */
         sprintf(RespondStr,"2B %ld",memPartFindMax(memSysPartId));
         /* if connection broken will send msge to phandler & suspend task */
         phandlerWriteChannel(chan_no,RespondStr,DLRESPB);
         /* rWriteChannel(chan_no,RespondStr,DLRESPB); */

         errLogRet(LOGIT,debugInfo,
	     "download: Dynamic: Buffer request: %d bytes too large\n",size);

	 GenericException.exceptionType = ALLOC_ERROR;
	 GenericException.reportEvent = SYSTEMERROR + DYNBUFNOSPACE;
	 /* send ALLOC_ERROR (WARNING & ABORT) to exception handler task, 
		it knows what to do */
	 msgQSend(pMsgesToPHandlr, (char*) &GenericException, 
				sizeof(EXCEPTION_MSGE), 
					NO_WAIT, MSG_PRI_URGENT);

         return(-1);
      }
    }
      /* pass OK and total number of buffers available */
      sprintf(RespondStr,"OK %d",dlbFreeBufs(pDlbDynBufs));

      /* if connection broken will send msge to phandler & suspend task */
#ifdef INSTRUMENT
           wvEvent(EVENT_DOWNLINKER_HOST_ACK,NULL,NULL);
#endif
      phandlerWriteChannel(chan_no,RespondStr,DLRESPB);
      /* rWriteChannel(chan_no,RespondStr,DLRESPB); */

      readIntoBuf(chan_no, (char*)pDwnLdBuf->data_array,size);
      DPRINT5(1,"Recv: '%s',1- 0x%lx 2- 0x%lx, 3- 0x%lx 4- 0x%lx\n",
		label, pDwnLdBuf->data_array[0],
		pDwnLdBuf->data_array[1],
		pDwnLdBuf->data_array[2],
		pDwnLdBuf->data_array[3] );
      pDwnLdBuf->status = READY;
#ifdef INSTRUMENT
           wvEvent(EVENT_DOWNLINKER_XFERCMPLT,NULL,NULL);
#endif
  }   
  else if (strncmp(buftype,"Tables",strlen("Tables")) == 0)
  {
      /* get a Dynamic Type Named Buffer */
#ifdef INSTRUMENT
           wvEvent(EVENT_DOWNLINKER_GETBUF,NULL,NULL);
#endif
     sprintf(RespondStr,"OK %d",dlbFreeBufs(pDlbDynBufs));
     /* if connection broken will send msge to phandler & suspend task */
     phandlerWriteChannel(chan_no,RespondStr,DLRESPB);

     done = 0;
     n = startn;
 /* downLoad(int chan_no, char *buftype,char *label,int size,int number,int startn) */
     while (!done)
     {
	/* If aborted then suspend downLinker, phandler will resume this task to download */
        bytes = phandlerReadChannel( chan_no, &tableSize, sizeof(long));
        if (tableSize == 0)
        {
           done = 1;
           DPRINT1(0,"Tables Aborted, Recv %d tables\n",n);
           return(-1);
        }
	sprintf(newLabel,"%st%d",label,n);
        DPRINT2(0,"getting table: %s, size: %ld\n",newLabel,tableSize);
        pDwnLdBuf = dlbMakeEntry(pDlbDynBufs,newLabel,tableSize);
        if (pDwnLdBuf == NULL)
        {
          if (acqerrno == S_namebufs_NAMED_BUFFER_ALREADY_INUSE )
          {
             /* for testing just delete the one already there */
             errLogRet(LOGIT,debugInfo,
              "downLoad:Label '%s' Sill in Use, Erasing same name buffer\n",
			    label);
             pDwnLdBuf = dlbFindEntry(pDlbDynBufs,newLabel);
             dlbFree(pDwnLdBuf);
             pDwnLdBuf = dlbMakeEntry(pDlbDynBufs,newLabel,tableSize);
             if (pDwnLdBuf == NULL)
             {
               /* tell sendproc TOO BIG & max size */
               sprintf(RespondStr,"2B %ld",memPartFindMax(memSysPartId));
               /* if connection broken will send msge to phandler & suspend task */
               phandlerWriteChannel(chan_no,RespondStr,DLRESPB);
    
               errLogRet(LOGIT,debugInfo,
	         "download: Dynamic: Buffer request: %d bytes too large\n",size);

	       GenericException.exceptionType = ALLOC_ERROR;
	       GenericException.reportEvent = SYSTEMERROR + DYNBUFNOSPACE;
	       /* send ALLOC_ERROR (WARNING & ABORT) to exception handler task, 
		    it knows what to do */
	        msgQSend(pMsgesToPHandlr, (char*) &GenericException, 
				sizeof(EXCEPTION_MSGE), 
					NO_WAIT, MSG_PRI_URGENT);
                return(-1);
             }
          }
          else
          {
             /* tell sendproc malloc failed */
             sprintf(RespondStr,"2B %ld",memPartFindMax(memSysPartId));
             /* if connection broken will send msge to phandler & suspend task */
             phandlerWriteChannel(chan_no,RespondStr,DLRESPB);

             errLogRet(LOGIT,debugInfo,
	         "download: Dynamic: Buffer request: %d bytes too large\n",size);

	     GenericException.exceptionType = ALLOC_ERROR;
	     GenericException.reportEvent = SYSTEMERROR + DYNBUFNOSPACE;
	     /* send ALLOC_ERROR (WARNING & ABORT) to exception handler task, 
		    it knows what to do */
	     msgQSend(pMsgesToPHandlr, (char*) &GenericException, 
				sizeof(EXCEPTION_MSGE), 
					NO_WAIT, MSG_PRI_URGENT);
             return(-1);
          }
       }

      readIntoBuf(chan_no, (char*)pDwnLdBuf->data_array,tableSize);
      DPRINT5(1,"Recv: '%s',1- 0x%lx 2- 0x%lx, 3- 0x%lx 4- 0x%lx\n",
		newLabel, pDwnLdBuf->data_array[0],
		pDwnLdBuf->data_array[1],
		pDwnLdBuf->data_array[2],
		pDwnLdBuf->data_array[3] );
      pDwnLdBuf->status = READY;

      n = n + 1;
#ifdef INSTRUMENT
           wvEvent(EVENT_DOWNLINKER_XFERCMPLT,NULL,NULL);
#endif

      if (n >= number) done = 1;

    } /* end of while */
    DPRINT1(0,"Tables Cmplt, Recv %d tables\n",n);
     sprintf(RespondStr,"OK %d",dlbFreeBufs(pDlbFixBufs));
     /* if connection broken will send msge to phandler & suspend task */
     phandlerWriteChannel(chan_no,RespondStr,DLRESPB);

   }
   /* ----------------   F I X E D   D O W N L O A D   ------------------------- */
  else if (strncmp(buftype,"Fixed",strlen("Fixed")) == 0)
  {
     int oldnbufs,newnbufs;
     /* make this test so that il='y' won't attempt to reconfigFix() thereby 
	freeing all the the buffers
     */
     /*
     DPRINT2(0,"size: %d, dataBufSize: %ld\n",size , pDlbFixBufs->dataBufSize);
     DPRINT2(0,"number: %d, dataBufNum: %ld\n",number , pDlbFixBufs->dataBufNum);
    */
#ifdef INSTRUMENT
           wvEvent(EVENT_DOWNLINKER_FIXED,NULL,NULL);
#endif
     if ( (size > pDlbFixBufs->dataBufSize) || ( number > pDlbFixBufs->dataBufNum) )
     {
        oldnbufs = dlbFreeBufs(pDlbFixBufs);

        newnbufs = dlbReuse(pDlbFixBufs , number, size);

        DPRINT4(1,
          "dwnld: FIX: prev nbufs: %d, new nbufs: %d, bufsize: %lu, free bufs: %d\n",
	    oldnbufs,newnbufs,pDlbFixBufs->dataBufSize,dlbFreeBufs(pDlbFixBufs));
       /* test for size too big to handle error */
       if ( newnbufs <= 0)
       {
         /* tell sendproc TOO BIG & max size */
         sprintf(RespondStr,"2B %ld",dlbMaxSingleBuf(pDlbFixBufs));

         /* if connection broken will send msge to phandler & suspend task */
         phandlerWriteChannel(chan_no,RespondStr,DLRESPB);
         /* rWriteChannel(chan_no,RespondStr,DLRESPB); */

         errLogRet(LOGIT,debugInfo,
	     "download: Fixed: Buffer request: %d bytes too large\n",size);

	 GenericException.exceptionType = ALLOC_ERROR;
	 GenericException.reportEvent = SYSTEMERROR + FIXBUFNOSPACE;
	 /* send ALLOC_ERROR (WARNING & ABORT) to exception handler task, 
		it knows what to do */
	  msgQSend(pMsgesToPHandlr, (char*) &GenericException, 
				sizeof(EXCEPTION_MSGE), 
					NO_WAIT, MSG_PRI_URGENT);
         return(-1);
       }
     }

#ifdef INSTRUMENT
           wvEvent(EVENT_DOWNLINKER_HOST_ACK,NULL,NULL);
#endif
     sprintf(RespondStr,"OK %d",dlbFreeBufs(pDlbFixBufs));
     /* if connection broken will send msge to phandler & suspend task */
     phandlerWriteChannel(chan_no,RespondStr,DLRESPB);
     /* rWriteChannel(chan_no,RespondStr,DLRESPB); */
     done = 0;
     n = startn;
     while (!done)
     {
	/* If aborted then suspend downLinker, phandler will resume this task to download
	   remainder data until XFER_ABORTED data block is received  from the host */
        if (DownLinkerAbort == DOWNLINKER_SUSPEND)  /* aborted */
        {
           DPRINT(1,"downLoad: Suspend DwnLink, Aborted\n");
	   dlbFree(pDwnLdBuf);
	   taskSuspend(0);
	   DownLinkerAbort = DOWNLINKER_DUMPDATA;
	   /* temporary buffer to dump remaining acodes into */
	   sprintf(newLabel,"%sdump",label);
#ifdef INSTRUMENT
           wvEvent(EVENT_DOWNLINKER_GETBUF,NULL,NULL);
#endif
           pDwnLdBuf = dlbMakeEntryP(pDlbFixBufs,newLabel,size, WAIT_FOREVER);/*already mark as LOADING*/
        }

        if (DownLinkerAbort != DOWNLINKER_DUMPDATA)  /* if not abort get a new buffer name  */
        {
	  sprintf(newLabel,"%sf%d",label,n);
#ifdef INSTRUMENT
           wvEvent(EVENT_DOWNLINKER_GETBUF,NULL,NULL);
#endif
          pDwnLdBuf = dlbMakeEntryP(pDlbFixBufs,newLabel,size, WAIT_FOREVER);/*already mark as LOADING*/
        }
        else if (DownLinkerAbort == DOWNLINKER_DUMPDATA)
        {
#ifdef INSTRUMENT
           wvEvent(EVENT_DOWNLINKER_DUMP,NULL,NULL);
#endif
        }
        if (pDwnLdBuf == NULL)
        {
         /* for testing just delete the one already there */
         errLogRet(LOGIT,debugInfo,
          "downLoad:Label '%s' Already in Use, Erasing same name buffer\n",
			label);
         pDwnLdBuf = dlbFindEntry(pDlbFixBufs,label);   
         dlbFree(pDwnLdBuf);
         pDwnLdBuf = dlbMakeEntry(pDlbFixBufs,label,size);
         if (pDwnLdBuf == NULL)
         {
            errLogRet(LOGIT,debugInfo,"Didn't Obtain Buffer");
         }
        }
        /* DPRINT2(0,"downLoad: Acode: %d, %d bytes in Channel prior to reading\n",n,bytesInChannel(chan_no)); */
        bytes = readIntoBuf(chan_no, (char*)pDwnLdBuf->data_array,size);
        /* DPRINT2(0,"downLoad: Acode: %d, %d bytes in Channel after to reading\n",n,bytesInChannel(chan_no)); */
        DPRINT6(0,"Fix Recv: '%s',%d -bytes, 1- 0x%lx 2- 0x%lx, 3- 0x%lx, %d bytes in Channel\n",
		newLabel, bytes, pDwnLdBuf->data_array[0],
		pDwnLdBuf->data_array[1], pDwnLdBuf->data_array[2],
		bytesInChannel(chan_no) );
       
        if ((bytes < 0) && (DownLinkerAbort != DOWNLINKER_DUMPDATA))
        {
             pDwnLdBuf->status = READY;
             dlbFree(pDwnLdBuf);
             errLogSysRet(LOGIT,debugInfo,"Reading into Fixed Buffer, Socket Failure, suspending downLinker\n");
       	     GenericException.exceptionType = LOST_CONN;  
       	     GenericException.reportEvent = SCSIERROR + READERROR;
    	     /* send error to exception handler task */
    	      msgQSend(pMsgesToPHandlr, (char*) &GenericException, 
		   	    sizeof(EXCEPTION_MSGE), NO_WAIT, MSG_PRI_NORMAL);
             taskSuspend(0);
        }

        if (pDwnLdBuf->data_array[0] == XFER_STATUS_BLK )
        {
	  if (pDwnLdBuf->data_array[1] == XFER_COMPLETE) /* 6543210 */
	  {
#ifdef INSTRUMENT
           wvEvent(EVENT_DOWNLINKER_XFER_STATCMPLT,NULL,NULL);
#endif
	     DPRINT(0,"Data Transfer of items completed.\n");
             DPRINT1(0,"Free XFER_STATUS_BLK buffer: %s\n",newLabel);
             pDwnLdBuf->status = READY;
	     dlbFree(pDwnLdBuf);
	     done = 1;
             DownLinkerActive = 0;
	  }
          else  if (pDwnLdBuf->data_array[1] == XFER_ABORTED)  /* 9876543 */
          {
#ifdef INSTRUMENT
           wvEvent(EVENT_DOWNLINKER_XFER_STATABORT,NULL,NULL);
#endif
	     DPRINT(0,"Data Transfer Aborted.\n");
	     done = 1;
	     DPRINT(0,"Free All Buffers.\n");
             pDwnLdBuf->status = READY;
	     dlbFreeAll(pDlbFixBufs);   /* clear all buffers */
             DownLinkerActive = 0;
	  }
          else  if (pDwnLdBuf->data_array[1] == XFER_RESETCNT)  /* 43211234 */
          {
#ifdef INSTRUMENT
           wvEvent(EVENT_DOWNLINKER_XFER_STATRESETCNT,NULL,NULL);
#endif
	     DPRINT(0,"Reset buffer naming count.\n");
             n = -1;   /* -1 here it will be incremneted to zero before used. */
             pDwnLdBuf->status = READY;
             if (DownLinkerAbort != DOWNLINKER_DUMPDATA)
	       dlbFree(pDwnLdBuf);
	  }
        }
        pDwnLdBuf->status = READY;
#ifdef INSTRUMENT
           wvEvent(EVENT_DOWNLINKER_XFERCMPLT,NULL,NULL);
#endif
        n = (++n) % number;
     }
  }
  else if (strncmp(buftype,"VME",strlen("VME")) == 0)
  {
     char *vmeaddr, *probeaddr;
     int dwnldrem;
     int ok2proceed, quickval;

#ifdef INSTRUMENT
           wvEvent(EVENT_DOWNLINKER_VME,NULL,NULL);
#endif
     quickval = 0;
     ok2proceed = 131071;
     vmeaddr = (char *) htol( label );
     probeaddr = vmeaddr;

     /* the beginnings of multiple dsp download, but it's more complicated than
        first thought, darn...  since the dsp code is read directly from the socket and
        straight into the dsp's memory. No buffering is done. So I either have to have
        sendproc send it the right number of time or just try something down here.

        ADC_ID pAdc = adcGetAdcObjByIndex(0);
        if (vmeaddr == (pAdc->adcBaseAddr + DSP_COEF_OFFSET))
             dspDownLd = 1;
     */

     if (vxMemProbe( probeaddr, VX_WRITE, sizeof( quickval ), &quickval ) == ERROR)
     {
        ok2proceed = 0;
     }
     else
     {
        probeaddr = probeaddr = vmeaddr + size - sizeof( quickval );
        if (vxMemProbe( probeaddr, VX_WRITE, sizeof( quickval ), &quickval ) == ERROR)
        {
           ok2proceed = 0;
        }
     }
     if (ok2proceed)
     {
#ifdef INSTRUMENT
           wvEvent(EVENT_DOWNLINKER_HOST_ACK,NULL,NULL);
#endif
        phandlerWriteChannel(chan_no,OkStr,DLRESPB);
        /* rWriteChannel(chan_no,OkStr,DLRESPB); */
        bytes = receiveVmeDownload(chan_no,vmeaddr,size);
     }
     else
     {
        phandlerWriteChannel(chan_no,ErrStr,DLRESPB);
        /* rWriteChannel(chan_no,ErrStr,DLRESPB); */
     }
#ifdef INSTRUMENT
           wvEvent(EVENT_DOWNLINKER_XFERCMPLT,NULL,NULL);
#endif
  }
  else
  {
     errLogRet(LOGIT,debugInfo,
          "downLoad:Buffer Type: '%s' unknown\n",buftype);
     phandlerWriteChannel(chan_no,ErrStr,DLRESPB);
     /* rWriteChannel(chan_no,ErrStr,DLRESPB); */
     return(-1);
  }
  return(0);
}

setDwnLkAbort()
{
    DownLinkerAbort = 1;
}

clrDwnLkAbort()
{
    DownLinkerAbort = 0;
    DownLinkerActive = 0;
}

int downLinkerIsActive()
{
    return(DownLinkerActive);
}

readIntoBuf(int chan_no, char *bufAdr, int size)
{
  unsigned int count;
  unsigned int block;
  int bytes;

  count = size;
  block = DLXFR_SIZE;
  while (count > 0)
  {
    if (count < block)
      block = count;

    /* if connection broken will send msge to phandler & suspend task */
    bytes = phandlerReadChannel( chan_no, bufAdr, block);
    /* bytes = ReadChannel( chan_no, bufAdr, block); */
    if (bytes < 0)
    {
       errLogSysRet(LOGIT,debugInfo,"readIntoBuf: read of data failed\n");
       return(bytes);
    }
    bufAdr += block;
    count -= block;
  }
  return(size-count);
}

reconfigFix(number,size)
{
    dlbReuse(pDlbFixBufs , number, size);
}

chngDwnLkrPrior(int priority)
{
   taskPrioritySet(priority);
}

restoreDwnLkrPrior()
{
   taskPrioritySet(DownLkrPrior);
}

startDwnLnk(int taskPriority,int taskOptions,int stackSize)
{
   DownLkrPrior = taskPriority;
   if (taskNameToId("tDownLink") == ERROR)
     taskSpawn("tDownLink",taskPriority,taskOptions,stackSize,downlink,
	        NULL,pDlbDynBufs,pDlbFixBufs,4,5,6,7,8,9,10);
}

killDwnLnk()
{
   int tid;
   markBusy(DOWNLINKER_FLAGBIT);
   if ((tid = taskNameToId("tDownLink")) != ERROR)
      taskDelete(tid);
   dwnLnkrIsCon = 0;
}
resumeDownLink()
{
   int tid;
   if ((tid = taskNameToId("tDownLink")) != ERROR)
      taskResume(tid);
}
