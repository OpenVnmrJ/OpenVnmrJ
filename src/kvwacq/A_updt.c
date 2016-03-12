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
#define _POSIX_SOURCE /* defined when source is to be POSIX-compliant */
#include <string.h>
#include <vxWorks.h>
#include <msgQLib.h>
#include <semLib.h>

#include "hostAcqStructs.h"
#include "logMsgLib.h"
#include "namebufs.h"
#include "fifoObj.h"
#include "hardware.h"
#include "acqcmds.h"
#include "AParser.h"
#include "fifoObj.h"
#include "fifoBufObj.h"
#include "instrWvDefines.h"

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
#define  DEBUG_AUPDT 9
#endif

extern MSG_Q_ID pMsgesToAupdt;

extern ACODE_ID pTheAcodeObject;
extern FIFO_ID	pTheFifoObject;
extern STATUS_BLOCK currentStatBlock;
extern DLB_ID   pDlbDynBufs;   /* Dynamic Named Buffers */
extern DLB_ID   pDlbFixBufs;   /* Fast Fix Size Named Buffers */


/*********************************************************
 testing stubs
*********************************************************/
/********************************************************/

static DLBP updtNamedBuf(char *updtname,char *name)
{
   /* free current buffer and rename update buffer	*/
   return(dlbRename(pDlbDynBufs,updtname,name));
}


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
updtLockphase( ushort apval )
{
	int	fifoIsEmpty, fifoIsRunning;
	int	tmp1,tmp2;

	fifoIsEmpty = ( ( fifoEmpty( pTheFifoObject ) == 1 ) && 
		        ( fifoBufWkEntries(pTheFifoObject->pFifoWordBufs) == 0L ) );
	fifoIsRunning = fifoRunning( pTheFifoObject );

	if (fifoIsEmpty && fifoIsRunning) {
		errLogRet( LOGIT, debugInfo, "FIFO empty but running in update APBUS\n" );
		return( -1 );
	}

	if (fifoIsEmpty)
           init_fifo( NULL, 0, 0 );

        fifoStuffCmd(pTheFifoObject,CL_AP_BUS_SLCT, 0x0A30); /* lk xmtr */
        fifoStuffCmd(pTheFifoObject,CL_AP_BUS_WRT,  0x0A17); /* PIRB2 24-31*/
        tmp1 = (int)(- (currentStatBlock.stb.AcqLockPhase * 32.0 / 45.0))&0xFF;
        fifoStuffCmd(pTheFifoObject,CL_AP_BUS_INCWR,0x0A00|tmp1);/* zeroing */
        fifoStuffCmd(pTheFifoObject,CL_AP_BUS_SLCT, 0x0A30); /* lk xmtr */
        fifoStuffCmd(pTheFifoObject,CL_AP_BUS_WRT,  0x0A18); /* SMC2 */
        fifoStuffCmd(pTheFifoObject,CL_AP_BUS_INCWR,0x0A08);
        fifoStuffCmd(pTheFifoObject,CL_AP_BUS_SLCT, 0x0A30); /* lk xmtr */
        fifoStuffCmd(pTheFifoObject,CL_AP_BUS_WRT,  0x0A1E); /* HOP CLK2 */
        fifoStuffCmd(pTheFifoObject,CL_AP_BUS_INCWR,0x0A00); /* strobe*/
        fifoStuffCmd(pTheFifoObject,CL_AP_BUS_SLCT, 0x0A30); /* lk xmtr */
        fifoStuffCmd(pTheFifoObject,CL_AP_BUS_WRT,  0x0A17); /* PIRB2 24-31*/
        tmp2 = (int)(apval * 32.0 / 45.0)&0xFF;
        fifoStuffCmd(pTheFifoObject,CL_AP_BUS_INCWR,0x0A00|tmp2);
        fifoStuffCmd(pTheFifoObject,CL_AP_BUS_SLCT, 0x0A30); /* lk xmtr */
        fifoStuffCmd(pTheFifoObject,CL_AP_BUS_WRT,  0x0A1E); /* HOP CLK2*/
        fifoStuffCmd(pTheFifoObject,CL_AP_BUS_INCWR,0x0A00); /* strobe */

	if (fifoIsEmpty)
        {  fifoStuffCmd(pTheFifoObject,HALTOP,0);
           fifoStart( pTheFifoObject );
           fifoWait4Stop( pTheFifoObject );
        }

    DPRINT3( DEBUG_APBUS, "updtLockphase: fifo empty: %d, value: %d [0x%x]\n",
                 fifoIsEmpty, apval, apval );
	currentStatBlock.stb.AcqLockPhase = apval;

	return( 0 );
}

static int
updtapbus( ushort apreg, ushort apval )
{
	int	fifoIsEmpty, fifoIsRunning;

#ifdef FIFOBUFOBJ
	fifoIsEmpty = ( ( fifoEmpty( pTheFifoObject ) == 1 ) && 
		        ( fifoBufWkEntries(pTheFifoObject->pFifoWordBufs) == 0L ) );
#else
	fifoIsEmpty = fifoEmpty( pTheFifoObject );
#endif
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

     case LKPHASE:
        num = getintargs(&currentarg, paramvec, 1);
        updtLockphase ( paramvec[ 0 ] );
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

     case LKPHASE:
        num = getintargs(&currentarg, paramvec, 1);
        updtLockphase ( paramvec[ 0 ] );
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
	   rt_tbl = pAcodeId->cur_rtvar_base + 1;
	   updtuintvals(currentarg,
		(unsigned int) (rt_tbl),offset, num);
	}
	break;

     case CHG_TABLE: 

#ifdef INSTRUMENT
     wvEvent(EVENT_AUPDATE_CHGTABLE,NULL,NULL);
#endif

	num = getintargs(&currentarg, paramvec, 1);
	if (num == 1)
	{
	   DLBP active;
	   int tablenum;
	   char updatename[64],curname[64];
	   char numstring[8];
	   tablenum = paramvec[0];
	   if (pAcodeId->table_ptr[tablenum] != NULL)
	   {
		DLBP active;
   	   	strcpy(curname,pAcodeId->id);
		sprintf(numstring,"t%d",paramvec[0]);
   	   	strcat(curname,numstring);
   	   	strcpy(updatename,curname);
   	   	strcat(curname,"updt");
	   	if ((active = updtNamedBuf(updatename,curname)) != NULL)
		{
		   pAcodeId->table_ptr[tablenum] = (TBL_ID) dlbGetPntr(active);
		}
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
	   if ((active = updtNamedBuf(updatename,curname)) != NULL)
	   {
		long *rt_base;
		rt_base = (unsigned int *) dlbGetPntr(active);
		pAcodeId->cur_rtvar_size = *rt_base++; 
		pAcodeId->cur_rtvar_base = rt_base;
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
	   	if ((active = updtNamedBuf(updatename,curname)) != NULL)
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
   char *cmdptr,*argptr;
   int i,iter,retrys,cmd,num,parse_count, total_count;
   int cmdargs[8];

   DPRINT1( 0, "A_updt got %s\n", cmdstring );
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

   total_count = atoi(cmdptr);

   /* Take semaphore before updating acode objects, do all the 	 */
   /* commands in a message.  This will allow for dependancies.  */
   if (pTheAcodeObject != NULL)
     semTake(pTheAcodeObject->pAcodeControl,WAIT_FOREVER);

   i = 0;
   while ((i<total_count) && (argptr != NULL))
   {
	cmdptr=strtok_r(argptr,";",&argptr);
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

}


void
Aupdt()
{
   char	xmsge[ CONSOLE_MSGE_SIZE ];
   int	ival;

   FOREVER {
	ival = msgQReceive(
	   pMsgesToAupdt, &xmsge[ 0 ], CONSOLE_MSGE_SIZE, WAIT_FOREVER);
		DPRINT1( 0, "Xparse:  msgQReceive returned %d\n", ival );
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
}

killAupdt()
{
   int tid;
   if ((tid = taskNameToId("tAupdt")) != ERROR)
      taskDelete(tid);
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
