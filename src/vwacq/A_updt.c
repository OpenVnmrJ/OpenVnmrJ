/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* A_updt.c 11.1 07/09/07 - Interpreter Modules */
/* 
 */
#define _POSIX_SOURCE /* defined when source is to be POSIX-compliant */
#include <string.h>
#include <vxWorks.h>
#include <msgQLib.h>
#include <semLib.h>
#include <memLib.h>

#include "hostMsgChannels.h"
#include "hostAcqStructs.h"
#include "logMsgLib.h"
#include "namebufs.h"
#include  "fifoObj.h"
#include "acqcmds.h"
#include "AParser.h"
#include "fifoObj.h"
#include "fifoBufObj.h"
#include "instrWvDefines.h"
#include "errorcodes.h"
#include "expDoneCodes.h"

#ifndef NOCMD
#define  NOCMD	-1
#endif

#ifndef MAXNUMARGS
#define  MAXNUMARGS	8
#endif

/*  These DEBUG... defines let us control debugging in what is hoped 
    will be a more systematic way.  Is it suspected that the AP BUS is
    not working when ACQI tries to set a value which uses the AP BUS?
    Then define DEBUG_APBUS to be 0 and recompile this program, defining
    DEBUG too.  The values are set to 9 by default so the debug output
    will not be displayed under normal circumstances, even if DEBUG is
    defined when the program is compiled.				*/

#ifndef DEBUG_APBUS
#define  DEBUG_APBUS 9
#endif

#ifndef DEBUG_AUPDT
#define  DEBUG_AUPDT 0
#endif


static char *ErrStr = { "ERR" };
static char *OkStr = { "OK" };
static char RespondStr[20];
static int downLoadTable(int chan_no, char *buftype,char *label,int size,int number,int startn);

static int 	updtMonIsCon = 0;

extern MSG_Q_ID pMsgesToAupdt;	/* MsgQ used for Msges to AUpdt */
extern MSG_Q_ID pMsgesToPHandlr;

extern EXCEPTION_MSGE GenericException;

/* Fixed & Dynamic Named Buffers */
extern DLB_ID  pDlbDynBufs;
extern DLB_ID  pDlbFixBufs;


extern ACODE_ID pTheAcodeObject;
extern FIFO_ID	pTheFifoObject;
extern STATUS_BLOCK currentStatBlock;

/*********************************************************
 testing stubs
*********************************************************/
/********************************************************/

int getintargs(char **argstring,int *arglist, int maxargs)
{
   char *argptr; 
   int i;
   i = 0;
   while ((i < maxargs) && 
	 ((argptr=strtok_r(*argstring,",",argstring)) != (char *)NULL))
   {
	arglist[i]=atoi(argptr);
	i++;
   }
   return(i);
}

/*--------------------------------------------------------------*/
/* updtuintvals							*/
/* Parses given argument stream and updates an unsigned int	*/
/* buffer with these values.					*/
/*--------------------------------------------------------------*/
static int updtuintvals(char *argstring, unsigned int uibuffer,
				unsigned int offset, int maxargs)
{
   char *argptr;
   unsigned int *uiptr, uival;
   int i;
   i = 0;
   uiptr = (unsigned int *) uibuffer;
   uiptr = uiptr+offset;
   while ((i < maxargs) && 
	 ((argptr=strtok_r(argstring,",",&argstring)) != (char *)NULL))
   {
	uival = (unsigned int)atoi(argptr);
	*uiptr++ = uival;
	i++;
   }
   return(i);
}

/*--------------------------------------------------------------*/
/* updtushortvals						*/
/* Parses given argument stream and updates an unsigned short	*/
/* buffer with these values.					*/
/*--------------------------------------------------------------*/
static int updtushortvals(char *argstring, unsigned int usbuffer, 
					unsigned int offset, int maxargs)
{
   char *argptr; 
   unsigned short *usptr;
   int i;
   i = 0;
   usptr = (unsigned short *) usbuffer;
   usptr = usptr+offset;
   while ((i < maxargs) && 
	 ((argptr=strtok_r(argstring,",",&argstring)) != (char *)NULL))
   {
	*usptr++ = (unsigned short)atoi(argptr);
	i++;
   }
   return(i);
}

static int
updtapbus( ushort apreg, ushort apval )
{
	int	fifoIsEmpty, fifoIsRunning;

	fifoIsEmpty = ( ( fifoEmpty( pTheFifoObject ) == 1 ) && 
		        ( fifoBufWkEntries(pTheFifoObject->pFifoWordBufs) == 0L ) );

	fifoIsRunning = fifoRunning( pTheFifoObject );

	if (fifoIsEmpty && fifoIsRunning) {
		errLogRet( LOGIT, debugInfo, "FIFO empty but running in update APBUS\n" );
		return( -1 );
	}

	if (fifoIsEmpty)
	  writeapwordStandAlone( apreg, apval, 50 );
	else
	  writeapword( apreg, apval, 50 );

    DPRINT3( DEBUG_APBUS, "updateapbus: fifo empty: %d, apregister: 0x%x, value: 0x%x\n",
                 fifoIsEmpty, apreg, apval );

	return( 0 );
} 

/*  doUpdtNoAcodes is quite similar to doUpdtCommand,
    except that only a limited subset of commands are
    permitted.  It is not possible, for example, to
    fix A-codes when there are no A-codes present.	*/

static int doUpdtNoAcodes( char *startarg )
{
   unsigned int offset;
   int  cmd, num;
   int	paramvec[ MAXNUMARGS ];
   char	*currentarg, *paramptr;

   currentarg = startarg;
   paramptr = strtok_r(currentarg,",",&currentarg);
   cmd = atoi(paramptr);

   switch (cmd)
   {
     case FIX_APREG:
	num = getintargs(&currentarg, paramvec, 3);
	DPRINT2( 1, "FIX APREG: APBUS: 0x%x, value: %d\n", paramvec[ 0 ], paramvec[ 2 ] );
	updtapbus( paramvec[ 0 ], paramvec[ 2 ] );
	break;

     default: 
	errLogRet( LOGIT, debugInfo,
                  "Command not found or not supported unless A-codes are active.\n" );
	DPRINT1(0,
   "A_updt:  Command not found or not supported unless A-codes are active - cmd: %d\n",
    cmd );
	break;
   }
}

static int doUpdtcommand( ACODE_ID pAcodeId, char *startarg )
{
   int	cmd, cmdindex, iter, numparams, numparsed, paramindex;
   int	num;
   unsigned int offset;
   int	paramvec[ MAXNUMARGS ];
   char	*currentarg, *paramptr;

   currentarg = startarg;
   paramptr = strtok_r(currentarg,",",&currentarg);
   cmd = atoi(paramptr);

   DPRINT1(DEBUG_AUPDT,"cmd #: %d\n",cmd);
   switch (cmd)
   {
     case FIX_APREG:

#ifdef INSTRUMENT
     wvEvent(EVENT_AUPDATE_FIXAPREG,NULL,NULL);
#endif

	num = getintargs(&currentarg, paramvec, 3);
	DPRINT2( 1, "FIX APREG: APBUS: 0x%x, value: %d\n", paramvec[ 0 ], paramvec[ 2 ] );
	updtapbus( paramvec[ 0 ], paramvec[ 2 ] );
	break;

     case FIX_ACODE: 

#ifdef INSTRUMENT
     wvEvent(EVENT_AUPDATE_FIXACODE,NULL,NULL);
#endif

	num = getintargs(&currentarg, paramvec, 1);
	if ((num == 1) && (pAcodeId->cur_acode_base != NULL))
	{
	   offset = (unsigned int) paramvec[0];
	   updtushortvals(currentarg,
		(unsigned int)(pAcodeId->cur_acode_base),offset, 1);
	}
	break;

     case FIX_ACODES: 

#ifdef INSTRUMENT
     wvEvent(EVENT_AUPDATE_FIXACODES,NULL,NULL);
#endif

	num = getintargs(&currentarg, paramvec, 2);
	if ((num == 2) && (pAcodeId->cur_acode_base != NULL))
	{
	   offset = (unsigned int) paramvec[0];
	   num = paramvec[1];
	   updtushortvals(currentarg,
		(unsigned int)(pAcodeId->cur_acode_base),offset, num);
	}
	break;

     case FIX_RTVARS: 

#ifdef INSTRUMENT
     wvEvent(EVENT_AUPDATE_FIXRTVARS,NULL,NULL);
#endif

	num = getintargs(&currentarg, paramvec, 2);
	if ((num == 2) && (pAcodeId->cur_rtvar_base != NULL))
	{
	   long *rt_tbl;	/* pointer to first rtvalue */
	   offset = (unsigned int) paramvec[0];
	   num = paramvec[1];
	   rt_tbl = (long *) (pAcodeId->cur_rtvar_base + 1);
	   updtuintvals(currentarg,
		(unsigned int) (rt_tbl),offset, num);
	}
	break;

     case CHG_TABLE: 

#ifdef INSTRUMENT
     wvEvent(EVENT_AUPDATE_CHGTABLE,NULL,NULL);
#endif

	num = getintargs(&currentarg, paramvec, 1);
	DPRINT1(DEBUG_AUPDT,"CHG_TABLE: args=%d\n",num);
	if (num == 1)
	{
	   DLBP active;
	   int tablenum,i;
	   char updatename[64],curname[64];
	   char numstring[8];
	   tablenum = paramvec[0];
           DPRINT2(DEBUG_AUPDT,"table ref: %d, AcodePtr: 0x%lx\n",tablenum,pAcodeId->table_ptr[tablenum]);
	   if (pAcodeId->table_ptr[tablenum] != NULL)
	   {
		DLBP active;
   	   	strcpy(curname,pAcodeId->id);
		sprintf(numstring,"t%d",paramvec[0]);
   	   	strcat(curname,numstring);
   	   	strcpy(updatename,curname);
   	   	strcat(updatename,"updt");
		DPRINT2(DEBUG_AUPDT,"replace: '%s' with '%s' table\n",curname,updatename);
		/* dlbPrintBuffers(pDlbDynBufs);  */
                /*
                 for (i=0; i < pAcodeId->num_tables; i++)
                 {
			DPRINT2(DEBUG_AUPDT,"table[%d] = 0x%lx\n",i,pAcodeId->table_ptr[i]);
		 }
                 */
	   	if ((active = dlbRename(pDlbDynBufs,curname,updatename)) != NULL)
		{
		   DPRINT1(DEBUG_AUPDT,"new table ref: 0x%lx\n",dlbGetPntr(active));
		   pAcodeId->table_ptr[tablenum] = (TBL_ID) dlbGetPntr(active);
		}
		/* dlbPrintBuffers(pDlbDynBufs); */
		/*
                 printf("\nTable Info -- \n");
                 for (i=0; i < pAcodeId->num_tables; i++)
                 {
			DPRINT2(DEBUG_AUPDT,"table[%d] = 0x%lx\n",i,pAcodeId->table_ptr[i]);
		 }
		*/

	   }
	}
	break;

     case CHG_RTVAR: 

#ifdef INSTRUMENT
     wvEvent(EVENT_AUPDATE_CHGRTVAR,NULL,NULL);
#endif

	if (pAcodeId->cur_rtvar_base != NULL)
	{
	   DLBP active;
	   char updatename[64],curname[64];
   	   strcpy(curname,pAcodeId->id);
   	   strcat(curname,"rtv");
   	   strcpy(updatename,curname);
   	   strcat(curname,"updt");
	   if ((active = dlbRename(pDlbDynBufs,curname,updatename)) != NULL)
	   {
		long *rt_base;
		/* rt_base = (unsigned int *) dlbGetPntr(active); */
		rt_base = (long *) dlbGetPntr(active);
		pAcodeId->cur_rtvar_size = *rt_base++; 
		pAcodeId->cur_rtvar_base = (unsigned int *) rt_base;
	   }
	}
	break;

     case CHG_ACODE: 

#ifdef INSTRUMENT
     wvEvent(EVENT_AUPDATE_CHGACODE,NULL,NULL);
#endif

	num = getintargs(&currentarg, paramvec, 1);
	if (num == 1)
	{
	   DLBP active;
	   int acodenum;
	   char updatename[64],curname[64];
	   char numstring[8];
	   acodenum = paramvec[0];
	   if ((pAcodeId->cur_acode_set == acodenum) && 
					(pAcodeId->cur_acode_base != NULL))
	   {
   	   	strcpy(curname,pAcodeId->id);
		sprintf(numstring,"f%d",acodenum);
   	   	strcat(curname,numstring);
   	   	strcpy(updatename,curname);
   	   	strcat(curname,"updt");
	   	if ((active = dlbRename(pDlbFixBufs,curname,updatename)) != NULL)
		{
		   pAcodeId->cur_acode_base = 
					(unsigned short *) dlbGetPntr(active);
		}
	   }
	}
	break;


     default: 
	errLogRet( LOGIT, debugInfo, "Command not found.\n" );
	DPRINT1(0,"A_updt:  Command not found - cmd: %d\n",cmd );
   }
	return( numparsed );
}

/*----------------------------------------------------------------------*/
/* A_updt								*/
/* This routine will parse the received commands.			*/
/* - While the acodeObj is NULL parsed commands will be thrown away.	*/
/* - When the acodeObj is not NULL, the parser semaphore will be taken	*/
/*   and will not be given up until the message has been parsed.	*/
/*   Note: This allows a number of commands to be sent that are 	*/
/*   dependant on one another.						*/
/*----------------------------------------------------------------------*/
void A_updt(char *cmdstring)
{
   char *cmdptr,*argptr,*refaddr;
   int i,iter,retrys,cmd,num,parse_count, total_count,size;
   int cmdargs[8];
   refaddr = 0L;

   DPRINT1( DEBUG_AUPDT, "A_updt got %s\n", cmdstring );

   /* Parse Command -  		*/
   /* "argptr" will point to the character after the token and should	*/
   /* be used as the starting point in the next strtok_r call		*/
   cmdptr=strtok_r(cmdstring,";",&argptr);
   if (cmdptr == NULL)
   {
	errLogRet( LOGIT, debugInfo, "Command not parsed.\n" );
	DPRINT1(0,"A_updt:  Command not parsed - cmdstring: %s\n",cmdstring );
	return;
   }
   if (argptr == NULL)
   {
	errLogRet( LOGIT, debugInfo, "No command for A_updt.\n" );
	DPRINT1(0,"A_updt: No command for A_updt - cmdstring: %s\n",cmdstring );
	return;
   }

   /* test if cmd is being done by reference address */
   if (strcmp(cmdptr,"CmdByAddr") == 0)
   {
	/* CmdByAddr 0x%lx %d\n */
	cmdptr=strtok_r(argptr," ",&argptr);
 	refaddr = (char*) strtol(cmdptr,NULL,0);
	cmdptr=strtok_r(argptr,"\n",&argptr);
        size = atoi(cmdptr);
        DPRINT2(DEBUG_AUPDT,"Cmd via Address Reference, Addr: 0x%lx, size: %d\n",
		refaddr,size);
        cmdptr=strtok_r(refaddr,";",&argptr);
   }


   total_count = atoi(cmdptr);
   DPRINT1(DEBUG_AUPDT,"total count: %d\n",total_count);

   /* Take semaphore before updating acode objects, do all the 	 */
   /* commands in a message.  This will allow for dependancies.  */
   if (pTheAcodeObject != NULL)
     semTake(pTheAcodeObject->pAcodeControl,WAIT_FOREVER);

   i = 0;
   while ((i<total_count) && (argptr != NULL))
   {
	cmdptr=strtok_r(argptr,";",&argptr);
        DPRINT1(DEBUG_AUPDT,"cmd: '%s'\n",cmdptr);
	if (cmdptr != NULL)
	{
	   /* May  want to check for error and break out of loop	*/
	   /* in case the first command fails and the rest of the	*/
	   /* commands are dependant on it.				*/

           if (pTheAcodeObject != NULL){
	     doUpdtcommand( pTheAcodeObject, cmdptr );
           }else{
	     doUpdtNoAcodes( cmdptr );
	   }
	}
	i++;
   }

   if (pTheAcodeObject != NULL)
     semGive(pTheAcodeObject->pAcodeControl);

   if (refaddr != 0L)
     free(refaddr);

}


void
Aupdt()
{
   char	xmsge[ CONSOLE_MSGE_SIZE ];
   int	ival;

   FOREVER {
	ival = msgQReceive(
	  	pMsgesToAupdt, &xmsge[ 0 ], CONSOLE_MSGE_SIZE, WAIT_FOREVER);
	DPRINT1( DEBUG_AUPDT, "Xparse:  msgQReceive returned %d\n", ival );
	if (ival == ERROR)
	{
	   DPRINT(1, "X PARSER Q ERROR\n");
	   return;
	}
	else
	{
	   A_updt(xmsge);
	}
   }
}



AupdrMon()
{
   int	bytes;
   int 	chan_no;
   ulong_t count,block;
   int totsize;
   int size,number,startn;
   int numberTables,numberAcodes;
   char *tail;
   char *dataAddr;
   char hbuf[250],label[64],cmd[64],buftype[64],resp[64];
   char okb[DLRESPB],suxb[DLRESPB];
   DLBP hit;
   int index;
   char tablename[64];
   char xmsge[ CONSOLE_MSGE_SIZE ];
   int updtcmdsize, tblcmdsize;
   int tablesize, tblref;


   char *token,**pLast;

   chan_no = UPDTPROC_CHANNEL;

   DPRINT(1,"AupdrMon: Establish Connection to Host.\n");
   EstablishChannelCon(chan_no);
   DPRINT(1,"AupdrMon: Connection Established.\n");

   updtMonIsCon = 1;

   DPRINT(1,"AupdrMon:Server LOOP Ready & Waiting.\n");
   strcpy(okb,"OK");
   strcpy(suxb,"SUX");
   bytes = flushChannel(chan_no);
   DPRINT2(0,"AupdrMon: %d bytes flushed from channel %d\n",bytes,chan_no);
   FOREVER
   {
      DPRINT1(1,"%d bytes in channel prior cmd read\n",bytesInChannel(chan_no));
      /* if connection broken will send msge to phandler & suspend task */
      bytes = phandlerReadChannel(chan_no, &updtcmdsize, sizeof(int));
      /* DPRINT2(DEBUG_AUPDT,"AupdtMon: bytes: %d, cmdsize: %d\n",bytes,updtcmdsize); */
      bytes = phandlerReadChannel(chan_no, hbuf, updtcmdsize);

      /* DPRINT1(DEBUG_AUPDT,"\nAupdtMon: Got Cmd, %d bytes\n",bytes); */
      hbuf[updtcmdsize] = 0;
      DPRINT1(DEBUG_AUPDT,"\nAupdtMon: Got Cmd, '%s' \n",hbuf);
      if (parseAupdtInstr(hbuf,cmd,buftype,label,&numberTables,&numberAcodes) < 1)
      {
	 errLogRet(LOGIT,debugInfo,"AupdrMon: Bad Instruction");
         bytes = flushChannel(chan_no);
         DPRINT2(0,"AupdrMon: %d bytes flushed from channel %d\n",bytes,chan_no);
	 /* rWriteChannel(chan_no,ErrStr,DLRESPB); */
	 continue;
      }
      DPRINT5(DEBUG_AUPDT,"AupdtMon: cmd: '%s', buf: '%s', tbl: '%s', ntables: %d, acodes: %d\n",
	cmd,buftype,label,numberTables,numberAcodes);

      /* if no tables then don't get them */
      if (numberTables > 0)
      {
        for (index=0; index < numberTables; index++)
        {
          bytes = phandlerReadChannel(chan_no, &tblcmdsize, sizeof(int));
	  /* DPRINT2(DEBUG_AUPDT,"AupdtMon: bytes: %d, cmdsize: %d\n",bytes,tblcmdsize); */
          bytes = phandlerReadChannel(chan_no, hbuf, tblcmdsize);
          /* DPRINT1(DEBUG_AUPDT,"\nAupdtMon: Got tblcmd, %d bytes\n",bytes); */
	  parseTableInfo(hbuf,tablename,&tablesize,&tblref);
	  DPRINT3(DEBUG_AUPDT,"AupdtMon: tablename: '%s', tblsize: %d, tblref: %d\n",
		tablename,tablesize,tblref);
          downLoadTable(chan_no, buftype,tablename,tablesize,1,tblref);
        }
      }
      bytes = phandlerReadChannel(chan_no, &updtcmdsize, sizeof(int));
      DPRINT2(DEBUG_AUPDT,"AupdtMon: bytes: %d, updtcmdsize: %d\n",bytes,updtcmdsize);
      if (updtcmdsize > CONSOLE_MSGE_SIZE)
      {
         char *cmd;
         cmd = (char *) malloc(updtcmdsize);  /* A_updt will free it */
	 /* DPRINT1(DEBUG_AUPDT,"Malloc Space at 0x%lx\n",cmd); */
         if (cmd != NULL)
	 {
	   bytes = phandlerReadChannel(chan_no, cmd, updtcmdsize);
           sprintf(xmsge,"CmdByAddr;0x%lx %d\n",cmd,updtcmdsize);
	   DPRINT1(DEBUG_AUPDT,"malloc'd xmsge: '%s'\n",xmsge);
           /* send the CmdByAddr Cmd */
     	   msgQSend(pMsgesToAupdt, xmsge, strlen(xmsge)+1, NO_WAIT, MSG_PRI_NORMAL);
         }
         else
          DPRINT(DEBUG_AUPDT,"malloc Failed\n");
      }
      else
      {
	 bytes = phandlerReadChannel(chan_no, xmsge, updtcmdsize);
         DPRINT1(DEBUG_AUPDT,"updtcmd: '%s'\n",xmsge);
     	 msgQSend(pMsgesToAupdt, xmsge, updtcmdsize, NO_WAIT, MSG_PRI_NORMAL);
      }

   }
   closeChannel( chan_no );
}

int parseTableInfo(char *instr, char *tblname, int *size, int *tblref)
{
   int k;
   k = sscanf(instr,"%s %d %d",tblname,size,tblref);
   if (k < 2)
     return(-1); 
   return(1);
}

/* command -  only one right now updttblrtvar
   bufType - dynamic 
   label - root name of NamedBuffer
   number of tables
   updtcmd 
*/
int 
parseAupdtInstr(char *instr,char *cmd,char *bufType, char *label,int *ntables, int *nacodes)
{
   int k;
   char ntbls[80],nacds[80];
   k = sscanf(instr,"%s %s %s %d %d",cmd,bufType,label,ntables,nacodes);
   /* DPRINT2(DEBUG_AUPDT,"parseAupdtInstr: cmd: '%s', conversions: %d\n",instr,k); */
   if (k < 2)
     return(-1); 
   return(1);
}


startAupdt(int priority, int taskoptions, int stacksize)
{
   if (pMsgesToAupdt == NULL)
     pMsgesToAupdt = msgQCreate(300,100,MSG_Q_PRIORITY);
   if (pMsgesToAupdt == NULL)
   {
      errLogSysRet(LOGIT,debugInfo,
     	  "could not create X Parser MsgQ, ");
      return;
   }
   
   if (taskNameToId("tAupdt") == ERROR)
    taskSpawn("tAupdt",priority,0,stacksize,Aupdt,pMsgesToAupdt,
						2,3,4,5,6,7,8,9,10);
   if (taskNameToId("tUpdtMon") == ERROR)
     taskSpawn("tUpdtMon",priority-1,0,stacksize,AupdrMon,
	        NULL,pDlbDynBufs,pDlbFixBufs,4,5,6,7,8,9,10);
}

 
killAupdt()
{
   int tid;
   if ((tid = taskNameToId("tAupdt")) != ERROR)
      taskDelete(tid);
}



/* called went communications is lost with Host */
/* restart both the uplinker and the interactive uplinker */

restartUpdtMon()
{
   int tid;
   DPRINT(0,"restartUpdtMon:");
   if ( updtMonIsCon == 1)
   {
      DPRINT(0,"taskRestart of tUpdtMon:");
      updtMonIsCon = 0;
      if ((tid = taskNameToId("tUpdtMon")) != ERROR)
        taskRestart(tid);
   }
}

killUpdtMon()
{
   int tid;
   if ((tid = taskNameToId("tUpdtMon")) != ERROR)
	taskDelete(tid);
   updtMonIsCon = 0;
}

cleanAupdt()
{
  int tid;
  if (pMsgesToAupdt != NULL)
  {
    msgQDelete(pMsgesToAupdt);
    pMsgesToAupdt = NULL;
  }
  tid = taskNameToId("tAupdt");
  if (tid != ERROR)
    taskDelete(tid);
}

static int
cleanMsgesToAupdt()
{
  char xmsge[ CONSOLE_MSGE_SIZE ];
  int numleft;
  if (pMsgesToAupdt != NULL)
  {
    numleft = msgQNumMsgs(pMsgesToAupdt);
    if (numleft > 0)
    {
       while (msgQReceive(pMsgesToAupdt, &xmsge[ 0 ], CONSOLE_MSGE_SIZE, 
						NO_WAIT) > 0)
       {
	   DPRINT(1,"cleanMsgesToAupdt: Msg Discarded.\n");
       }
    }
  }
  return( numleft );
}

/*--------------------------------------------------------------*/
/* AupdtAA							*/
/* 	Abort sequence for Aupdt.				*/
/*								*/
/*  Extended, February 1998					*/
/*	It was found that if the Problem Handler (phandler.c)	*/
/*	"stole" a message for the A_updt program (i.e.		*/
/*	cleanMsgesToAupdt found and read 1 or more messages),	*/
/*	then the task would not restart until it was sent a	*/
/*	message.  This caused the Problem Handler to hang in	*/
/*	AupdtAA until this message was received, which might	*/
/*	not ever happen.  It was also found that the restart	*/
/*	was immediate if a stub message was sent to A_updt	*/
/*	in this situatuion.  Thus cleanMsgesToAupdt now returns */
/*	the count of messages it found pending, and if one or	*/
/*	more were present, the Problem Handler (AupdtAA) sends	*/
/*	a stub message to Aupdt.	February 5, 1998	*/
/*--------------------------------------------------------------*/
AupdtAA()
{
   char	msg4Aupdt[ 32 ];
   int msgesCount;
   int tid;

   if ((tid = taskNameToId("tAupdt")) != ERROR)
   {
	/* taskSuspend(tid); */
	msgesCount = cleanMsgesToAupdt();
	if ((pTheAcodeObject != NULL) && 
				(pTheAcodeObject->pAcodeControl != NULL))
	{
		semGive(pTheAcodeObject->pAcodeControl);
	}
	if (msgesCount > 0)
	{
	   sprintf( &msg4Aupdt[ 0 ], "0;" );
	   msgQSend(pMsgesToAupdt, &msg4Aupdt[ 0 ], 3, NO_WAIT, MSG_PRI_NORMAL);
        }
	taskRestart(tid);
   }
}

static int
downLoadTable(int chan_no, char *buftype,char *label,int size,int number,int startn)
{
  int done,n,bytes,retrys;
  int idx;
  char newLabel[128];
  DLBP pDwnLdBuf;
  DPRINT5(DEBUG_AUPDT,"Buf: '%s', Label: '%s', size: %d, num: %d, start#: %d\n",
		buftype,label, size, number, startn);


  /* -----------  D Y N A M I C   D O W N L O A D  --------------- */
  if (strncmp(buftype,"Dynamic",strlen("Dynamic")) == 0)
  {
      /* get a Dynamic Type Named Buffer, number is always assumed tobe One */
/*
#ifdef INSTRUMENT
           wvEvent(EVENT_DOWNLINKER_GETBUF,NULL,NULL);
#endif
*/
    pDwnLdBuf = dlbMakeEntry(pDlbDynBufs,label,size);
    if (pDwnLdBuf == NULL)
    {
      if (acqerrno == S_namebufs_NAMED_BUFFER_ALREADY_INUSE )
      {
         /* for testing just delete the one already there */
         errLogRet(LOGIT,debugInfo,
          "downLoad:Label '%s' Still in Use, Erasing same name buffer\n",
			label);
         pDwnLdBuf = dlbFindEntry(pDlbDynBufs,label);
         dlbFree(pDwnLdBuf);
         pDwnLdBuf = dlbMakeEntry(pDlbDynBufs,label,size);
         if (pDwnLdBuf == NULL)
         {
           /* tell sendproc TOO BIG & max size */
           sprintf(RespondStr,"2B %ld",memPartFindMax(memSysPartId));
           /* if connection broken will send msge to phandler & suspend task */
           /*phandlerWriteChannel(chan_no,RespondStr,DLRESPB); */

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
         /* phandlerWriteChannel(chan_no,RespondStr,DLRESPB); */
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
      /* sprintf(RespondStr,"OK %d",dlbFreeBufs(pDlbDynBufs)); */

      /* if connection broken will send msge to phandler & suspend task */

      bytes = phandlerReadChannel( chan_no, (char*) pDwnLdBuf->data_array, size);
      DPRINT6(DEBUG_AUPDT,"Recv: %d bytes, '%s',1- 0x%lx 2- 0x%lx, 3- 0x%lx 4- 0x%lx\n",
		bytes,label, pDwnLdBuf->data_array[0],
		pDwnLdBuf->data_array[1],
		pDwnLdBuf->data_array[2],
		pDwnLdBuf->data_array[3] );
      pDwnLdBuf->status = READY;
      for(idx=0; idx < size/4; idx++)
        DPRINT3(DEBUG_AUPDT,"value[%d]: %d (0x%x)\n",idx,pDwnLdBuf->data_array[idx],pDwnLdBuf->data_array[idx]);
	DPRINT1(DEBUG_AUPDT,"Table DLB Addr: 0x%lx\n",pDwnLdBuf);
        dlbPrintEntry(pDwnLdBuf);	
#ifdef INSTRUMENT
           wvEvent(EVENT_DOWNLINKER_XFERCMPLT,NULL,NULL);
#endif
  }   
   /* ----------------   F I X E D   D O W N L O A D   ------------------------- */
  /* else if (strncmp(buftype,"Fixed",strlen("Fixed")) == 0) */
}
