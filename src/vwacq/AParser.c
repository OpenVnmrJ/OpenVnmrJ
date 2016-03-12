/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* AParser.c 11.1 07/09/07 - Interpreter Modules */
/* 
 */

#define _POSIX_SOURCE /* defined when source is to be POSIX-compliant */
#include <vxWorks.h>
#include <stdio.h>
#include <string.h>
#include <msgQLib.h>
#include <semLib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "instrWvDefines.h"
#include "logMsgLib.h"
#include "acodes.h"
#include "namebufs.h"
#include "stmObj.h"
#include "fifoObj.h"
#include "tuneObj.h"
#include "AParser.h"
#include "hostAcqStructs.h"
#include "errorcodes.h"
#include "expDoneCodes.h"
#include "timeconst.h"
#include "sysflags.h"

/* getlkfid() */
#include "hardware.h"
#include "stmObj.h"
#include "adcObj.h"
/*---------------*/

extern FIFO_ID pTheFifoObject;
extern ACODE_ID pTheAcodeObject;
extern TUNE_ID pTheTuneObject;

/* for getlkfid() */
extern STMOBJ_ID pTheStmObject;
extern ADC_ID pTheAdcObject;
/*-------------------*/

extern MSG_Q_ID pMsgesToAParser;
extern MSG_Q_ID pMsgesToPHandlr;
extern MSG_Q_ID pIntrpQ;
extern DLB_ID   pDlbDynBufs;   /* Dynamic Named Buffers */
extern DLB_ID   pDlbFixBufs;   /* Fast Fix Size Named Buffers */

/* Exception Msges to Phandler, e.g. FOO, etc. */
extern EXCEPTION_MSGE GenericException;

static int FifoFd = -1;

/*********************************************************
*
* AParser.c - the ACODE Parser task 
*
**********************************************************/


/*********************************************************
 testing stubs
*********************************************************/
int 
anop(char *tt)
{
  puts("ANOP");
  puts(tt);
  return(1);
}

testsendstm(int ct, int nt, int tag2snd)
{
#ifdef TOFIX_SOMETIME
   ITR_MSG intrmsg;

	    printf("test send stm message called\n");
	    intrmsg.tag = tag2snd;
	    intrmsg.count = ct;
	    if (ct == nt)
	      intrmsg.itrType = RTZ_DATA_CMPLT;
	    else 
	      intrmsg.itrType = USER_FLAG;
	    intrmsg.msgType = INTERRUPT_OCCURRED;
	    msgQSend(pIntrpQ,(char *) &(intrmsg),sizeof(ITR_MSG),
					WAIT_FOREVER,MSG_PRI_NORMAL);
#endif
}

tstacquire(short flags, int np, int ct, int fid_num, long *stmaddr)
{
	int i;

	  printf("tacquire:   fid: %d  ct: %d   np: %d  stmaddr:0x%x\n",
					fid_num,ct,np,stmaddr);
	  *stmaddr++ = fid_num;
	  *stmaddr++ = ct;
	  *stmaddr++ = fid_num;
	  *stmaddr++ = ct;
	  for (i=0; i < (np-4)/4; i++)
	  {
	    *stmaddr++ = i*fid_num;
	    *stmaddr++ = fid_num;
	  }
	  for (i=0; i < (np-4)/4; i++)
	  {
	    *stmaddr++ = ((np-4)/4 - i) * fid_num;
	    *stmaddr++ = fid_num;
	  }
}

/*********************************************************
 End testing stubs
*********************************************************/

#ifdef TESTING
/*----------------------------------------------------------------------*/
/* fifoWriteIt								*/
/* 	Writes fifo words to nfs file.			 		*/
/*----------------------------------------------------------------------*/
fifoWriteIt(int FifoId, unsigned long *fifowords, int num)
{
   int byteswritten;
   FifoId = FifoFd;
   byteswritten = write(FifoId, (void *)fifowords, num*4);
   if (byteswritten != num*4)
	DPRINT(1,"fifoWriteIf: WRONG number of bytes written to fifofile.\n");
}

/*----------------------------------------------------------------------*/
/* createFifofile							*/
/* 	Trys to open a nfs file for writing.  The file will be 		*/
/*	/vnmrAcqqueue/<id>.Fifo						*/
/*	This routine returns a zero if successful and a -1 if		*/
/*	unsuccessful.							*/
/*	Arguments:							*/
/*		id	: experiment id char string			*/
/*----------------------------------------------------------------------*/
int createFifofile(char *id)
{
 char fifofile[128];
 int status;
   strcpy(fifofile,"/vnmrAcqqueue/");	
   strcat(fifofile,id);
   strcat(fifofile,".Fifo");
   /* First check to see if the file system is mounted and the file	*/
   /* can be opened successfully.					*/
   FifoFd = open(fifofile, O_CREAT | O_RDWR, 0666);
   if (FifoFd < 0)
   {
	/* status = nfsMount("boothost","/vnmr/acqqueue","/vnmrAcqqueue"); */
	/* if (status == 0) */
	/* { */
	/*   FifoFd = open(fifofile, O_CREAT | O_RDWR, 0666); */
  	/* } */
	DPRINT1(1,
	    "init_fifo: For fifo output file %s to be opened, filesystem\n",
							fifofile);
	DPRINT1(1,
	    "           %s has to be nfsMount<ed> on console, or previous\n",
							"/vnmrAcqqueue");
	DPRINT(1,"           file has to be closed.\n");
   }
   return(FifoFd);
}
#endif
#ifdef XXXXX
fifoStuffIt(int FifoId, unsigned long *fifowords, int num)
{
   if (num == 2) 
   {
/*
 *    printf("fifoword 0: %x (hex) fifoword 1: %x (hex)\n",fifowords[0],
 *					fifowords[1]);
 */ 
   }
   else if (num == 3)
   {
/*
 *    printf("fifoword 0: %x (hex) fifoword 1: %x (hex) fifoword 2: %x (hex)\n",
 *				fifowords[0],fifowords[1],fifowords[2]);
 */ 
   }
   else
    printf("WRONG number of fifo words specified\n");
}
#endif
/********************************************************/
/* acodeStartFifoWD -  Starts fifo watchdog timer.      */
/* RETURNS:						*/
/*  void						*/
/********************************************************/
void acodeStartFifoWD(ACODE_ID pAcodeId, FIFO_ID pFifoId)
{
    if (wdStart(pAcodeId->wdFifoStart,sysClkRateGet()/100, 
				fifoStart,(int) pFifoId) == ERROR)
    {
	DPRINT(0,"acodeStartFifoWD: Error in starting.\n");
    }
    DPRINT(1,"acodeStartFifoWD: Watchdog started.\n");
}

/********************************************************/
/* acodeCancelFifoWD -  Cancels fifo watchdog timer.    */
/* RETURNS:						*/
/*  void						*/
/********************************************************/
void acodeCancelFifoWD(ACODE_ID pAcodeId)
{
    wdCancel(pAcodeId->wdFifoStart);
    DPRINT(1,"acodeCancelFifoWD: Watchdog Canceled.\n");
}

/********************************************************/
/* Remove Acode Set -  					*/
/********************************************************/
int rmAcodeSet(char *expname, int cur_acode_set)
{
 char numstring[8];
 char tmpbufname[64];
 DLBP active;
 int  status;

   numstring[0] = '\0';
   sprintf(numstring,"f%d",cur_acode_set);
   /* Get Named acode buffer */
   strcpy(tmpbufname,expname);
   strcat(tmpbufname,numstring);
   DPRINT1(1,"rmAcodeBuf: Free acode buffer: %s\n",tmpbufname);
   return(dlbFreeByName(pDlbFixBufs, tmpbufname));
}

/********************************************************/
/* Set Acode buffer as Done 				*/
/********************************************************/
int markAcodeSetDone(char *expname, int cur_acode_set)
{
 char numstring[8];
 char tmpbufname[64];
 DLBP active;

   numstring[0] = '\0';
   sprintf(numstring,"f%d",cur_acode_set);
   /* Get Named acode buffer */
   strcpy(tmpbufname,expname);
   strcat(tmpbufname,numstring);
   DPRINT1(1,"markAcodeSetDone: acode buffer: %s\n",tmpbufname);
   active = dlbFindEntry(pDlbFixBufs,tmpbufname);
   if (active != NULL)
     active->status = DONE;
   else
      return(-1);
   return(0);
}
/********************************************************/
/* Remove RTvar Set -  					*/
/********************************************************/
int rmRTvarSet(char *expname)
{
 char numstring[8];
 char tmpbufname[64];
 DLBP active;

   /* Get Named acode buffer */
   strcpy(tmpbufname,expname);
   strcat(tmpbufname,"rtv");
   DPRINT1(1,"rmRTvarSet: Free RT Parm buffer: %s\n",tmpbufname);
   return(dlbFreeByName(pDlbDynBufs, tmpbufname));
}

/********************************************************/
/* Remove WF Set -  					*/
/********************************************************/
int rmWFSet(char *expname)
{
 char numstring[8];
 char tmpbufname[64];
 DLBP active;

   /* Get Named acode buffer */
   strcpy(tmpbufname,expname);
   strcat(tmpbufname,"wf");
   DPRINT1(1,"rmWFSet: Free WF buffer: %s\n",tmpbufname);
   return(dlbFreeByName(pDlbDynBufs, tmpbufname));
}

/********************************************************/
/* Remove All Acode Sets -  					*/
/********************************************************/
int rmAcodeSets(char *expname, int start_acode_set, int num_acode_sets)
{
 char numstring[8];
 char tmpbufname[64];
 DLBP active;
 int  status,i;

   numstring[0] = '\0';
   for (i=start_acode_set; i<num_acode_sets; i++)
   {
   	/* Get Named acode buffer */
   	sprintf(numstring,"f%d",i);
   	strcpy(tmpbufname,expname);
   	strcat(tmpbufname,numstring);
	status = dlbFreeByName(pDlbFixBufs, tmpbufname);
   	DPRINT1(1,"rmAcodeSets: Free acode buffer: %s\n",tmpbufname);
   }
   return(0);
}

/********************************************************/
/* Remove Tables -  					*/
/********************************************************/
int rmTables(ACODE_ID pAcodeId, char *expname, int num_tables)
{
 char numstring[8];
 char tmpbufname[64];
 DLBP active;
 int  status, i;
   for (i=0; i<num_tables; i++)
   {
   	/* Get Named acode buffer */
   	sprintf(numstring,"t%d",i);
   	strcpy(tmpbufname,expname);
   	strcat(tmpbufname,numstring);
	status = dlbFreeByName(pDlbDynBufs, tmpbufname);
	pAcodeId->table_ptr[i] = NULL;
   	DPRINT1(1,"rmTables: Free Table: %s\n",tmpbufname);
   }
   free(pAcodeId->table_ptr);
   return(status);
}

/********************************************************/
/* Get Acode Set -  					*/
/********************************************************/
unsigned short
*getAcodeSet(char *expname, int cur_acode_set, int interactive_flag, int timeVal)
{
 char numstring[8];
 char tmpbufname[64];
 unsigned short *ac_base;		/* acode pointer */
 DLBP active;


   numstring[0] = '\0';
   sprintf(numstring,"f%d",cur_acode_set);

   /* Get Named acode buffer */
   strcpy(tmpbufname,expname);
   strcat(tmpbufname,numstring);
   DPRINT1(1,"getAcodeBuf: Find acode buffer: %s\n",tmpbufname);
   /* Pend if name buffer not there, timeout in 1 sec */
   active = dlbFindEntryP(pDlbFixBufs,tmpbufname,sysClkRateGet() * timeVal);
   if (active == NULL)
   {
	errLogRet(LOGIT,debugInfo,"getAcodeBuf: Didn't Find acode buffer: %s\n",
		   tmpbufname);
        return(NULL);
   }
   ac_base = (unsigned short *) dlbGetPntr(active); 
   
   if (*ac_base != ACODE_BUFFER)   /* is it an acode file? Remove header id?*/
   {
	errLogRet(LOGIT,debugInfo,"getAcodeBuf: Wasn't an Acode buffer: %s\n",
		tmpbufname);
	return(NULL);
   }
   active->status = PARSING;

/*  ACQI was causing problems when it aborted a FID display before the
    buffers were downloaded.  The abort would hang for a noticeable amount
    of time until those buffers were all downloaded.  This delay then
    caused other problems for ACQI.  To remedy this, we do not allow ACQI's
    FID display to stop until these buffers are downloaded, and we note
    this event by changing the acquisition state from PARSE to INTERACTIVE
    when this download completes.	February 5, 1998.		*/
    
   if (interactive_flag == ACQI_INTERACTIVE)
   {
	update_acqstate( ACQ_INTERACTIVE );
	getstatblock();                     /* send statblock up to host now */
   }
   return(ac_base);
}

/********************************************************/
/* Get Waveform Set -  					*/
/********************************************************/
unsigned short
*getWFSet(char *expname)
{
 char tmpbufname[64];
 unsigned short *wf_base;		/* waveform pointer */
 DLBP active;

   /**** Get Named Waveform buffer  ****/
   strcpy(tmpbufname,expname);
   strcat(tmpbufname,"wf");
   active = dlbFindEntry(pDlbDynBufs,tmpbufname);
   if (active == NULL)
   {
        DPRINT1(1,"getWFSet: Warning, didn't Find: %s\n",tmpbufname);
	return(NULL);
   }
   
   wf_base = (unsigned short *) dlbGetPntr(active);

   return(wf_base);
}

/********************************************************/
/* acodeClear -  Clears acode object and resources	*/
/*	Set up tables, Experiment info structure.	*/
/* RETURNS:						*/
/*  OK or ERROR						*/
/********************************************************/
int acodeClear(ACODE_ID pAcodeId)
{
   if (pAcodeId != NULL)
   {
	if (pAcodeId->cur_rtvar_base != NULL)
	{
	   rmRTvarSet(pAcodeId->id);
	   pAcodeId->cur_rtvar_base = NULL;
	}
	if (pAcodeId->num_tables != 0)
	{
	   rmTables(pAcodeId,pAcodeId->id,pAcodeId->num_tables);
	   pAcodeId->num_tables = 0;
	}
	if (pAcodeId->num_acode_sets != 0)
	{
	   rmAcodeSets(pAcodeId->id,pAcodeId->cur_acode_set,
					 pAcodeId->num_acode_sets);
	   pAcodeId->num_acode_sets = 0;
	}
	pAcodeId->cur_acode_base = NULL;

	/* remove any waveform buffers */
	rmWFSet(pAcodeId->id);

	if (pAcodeId->pAcodeControl != NULL)
           semGive(pAcodeId->pAcodeControl);

        /* Now Attempt to take it when, when it would block that
           is the state we want it in.
        */
#ifdef INSTRUMENT
	wvEvent(EVENT_INTRP_SUSPEND,NULL,NULL);
#endif
        while (semTake(pAcodeId->pSemParseSuspend,NO_WAIT) != ERROR);	

	if (pAcodeId->wdFifoStart != NULL)
	   wdDelete(pAcodeId->wdFifoStart);


	strcpy(pAcodeId->id,"");
	pAcodeId->interactive_flag = 0;
   }
   return(0);
}

resetParserSem(ACODE_ID pAcodeId)
{
 while (semTake(pAcodeId->pSemParseSuspend,1) != ERROR);	
}
/********************************************************/
/* Init Acode Object -  				*/
/*	Set up tables, Experiment info structure.	*/
/********************************************************/
acodeInit(ACODE_OBJ *pAcodeObj,char *expname ,int num_acode_sets, 
		int num_tables,int cur_acode_set,int interactive_flag)
{
   unsigned short *ac_cur,*ac_base,*ac_end; /* acode pointers */
   unsigned short *ac_start;
   long *rt_base, *rt_tbl;	/* pointer to realtime buffer addresses */
   short *acode;		/* acode location */
   short alength;	/* length of acode values in short words*/
   int cur_table;
   int index, tmp, not_found;
   int i,retrys;
   DLBP active;
   char tmpbufname[64];
   char numstring[8];


   numstring[0] = '\0';

   /* Copy Experiment Name */
   strcpy(pAcodeObj->id,expname); 

   /**** Setup Acodes   *****/
   pAcodeObj->num_acode_sets = num_acode_sets;
   pAcodeObj->cur_acode_set = cur_acode_set;

   /* Get new acode set 6 times pausing second between tests */
   retrys = 60;
   while (retrys > 0)
   {
     if (APSuspendVal() == TRUE)
     {
        retrys = 0;
     }
     else
     {
        pAcodeObj->cur_acode_base = pAcodeObj->cur_jump_base =
		getAcodeSet(pAcodeObj->id,pAcodeObj->cur_acode_set,interactive_flag, 1);
        if (pAcodeObj->cur_acode_base != NULL)
        {
	/* taskDelay(sysClkRateGet()*1);	/* startup delay of 1.0 seconds */
	   taskDelay(0);	/* startup delay of 1.0 seconds */
	   break;
        }
        retrys--;
        DPRINT1(0,"acodeInit:  retrys: %d\n",retrys);
        /* taskDelay(sysClkRateGet()*1); */
        taskDelay(0);
     }
   }
   if (retrys == 0)
   {
      acodeClear(pAcodeObj);
      return(SYSTEMERROR+NOACODESET);
   }

   /**** Get Named RTVar buffer  ****/
   strcpy(tmpbufname,expname);
   strcat(tmpbufname,"rtv");
   active = dlbFindEntry(pDlbDynBufs,tmpbufname);
   if (active == NULL)
   {
	errLogRet(LOGIT,debugInfo,"acodeInit: Didn't Find: %s",tmpbufname);
        acodeClear(pAcodeObj);
        return(SYSTEMERROR+NOACODERTV);
   }
   
   rt_base = (unsigned long *) dlbGetPntr(active);
   pAcodeObj->cur_rtvar_size = *rt_base++; 
   if (*rt_base != RTVAR_BUFFER)   /* is it a real time variable buffer ? */
   {
	errLogRet(LOGIT,debugInfo,"acodeInit: '%s' NOT a RT Parm Buffer",
		tmpbufname);
        acodeClear(pAcodeObj);
        return(SYSTEMERROR+NOACODERTV);
   }

   pAcodeObj->cur_rtvar_base = (unsigned int *)rt_base;
   pAcodeObj->num_tables = num_tables;

   /**** Create Interactive Info. ****/
   pAcodeObj->interactive_flag = interactive_flag;


   /**** Get Tables  ****/
   if (num_tables > 0)
   {
	pAcodeObj->table_ptr = (TBL_ID *) 
			malloc((sizeof(pAcodeObj->table_ptr)*num_tables));
	if (pAcodeObj->table_ptr == NULL)
	{
    	   errLogSysRet(LOGIT,debugInfo,
				"acodeInit: Could not Allocate TBL Space:");
    	   return(SYSTEMERROR+NOACODETBL);
	}
	cur_table = 0;
	not_found = FALSE;
	while ((cur_table < num_tables) || not_found)
	{
   	   sprintf(numstring,"t%d",cur_table);
   	   DPRINT1(1,"acodeCreate: exp suffix for table buffer: %s\n",
								numstring);

   	   strcpy(tmpbufname,expname);
   	   strcat(tmpbufname,numstring);
   	   active = dlbFindEntry(pDlbDynBufs,tmpbufname);
   	   if (active == NULL)
	   {
		not_found = TRUE;
	   }
	   else 
	   {
		pAcodeObj->table_ptr[cur_table] = (TBL_ID) dlbGetPntr(active);
	   }
	   cur_table++;
	}
	if (not_found)
	{
	   if (cur_table == 0)
	   {
        	acodeClear(pAcodeObj);
		return(SYSTEMERROR+NOACODETBL);
	   }
           else
	   {
	    	/* Fill in pointer array.  All tables may not be sent */
	    	while (cur_table < num_tables)
	    	{
	    	   pAcodeObj->table_ptr[cur_table] = NULL;
		   cur_table++;
	    	}
	   }
	}
   }

   return(0);

}
/*******************************************************
*  a TEST INTERPRETER
*  
********************************************************/

int APstart(char *cmdstring, int interactive_flag)
{
   char *cptr1,*cptr2;
   int num_acode_sets, cur_acode_set, num_tables, index, tmp, status;
   int i,retrys;
   DLBP active;
   TBL_ID tbl_ptr;
   int tbl_index;
   char expname[64];


   /* Parse Command -  Get exp name 	*/
   if ( (cptr1 = strtok_r(cmdstring,",",&cptr2)) != NULL)
   {
	strcpy(expname,cptr1);
   } else {
	errLogRet( LOGIT, debugInfo, "Exp name not in command string.\n" );
	return(SYSTEMERROR+INVALIDPCMD);
   }

   /* Set default argument values */
   num_acode_sets = 1;
   num_tables = 0;
   cur_acode_set = 0;

   /* Parse Command -  Get num of acode sets 	*/
   cptr1=strtok_r(cptr2,",",&cptr2);
   if (cptr1 != NULL)
   {
   	num_acode_sets = atoi(cptr1);
	/* Parse Command -  Get num of tables 	*/
   	cptr1=strtok_r(cptr2,",",&cptr2);
	if (cptr1 != NULL) 
	{
	   num_tables = atoi(cptr1);
	   /* Parse Command -  Get current acode set */
   	   cptr1=strtok_r(cptr2,",",&cptr2);
	   if (cptr1 != NULL) 
	   	cur_acode_set = atoi(cptr1);
	}
   }

   DPRINT3(1,"APint arg buffer: %s  Num Acodes: %d Num Tables: %d\n",
		expname,num_acode_sets,num_tables);


   /***** Initialize Exp  *****/

   if (pTheTuneObject)
     semTake( pTheTuneObject->pSemAccessFIFO, WAIT_FOREVER );

/*  No current STM tag at the start of the experiment  */

   pTheStmObject->currentTag = -1;

/*  see comment dated February 5, 1998, in getAcodeSet
    for explanation of why the next line was commented out.	*/

   /*update_acqstate( (interactive_flag == ACQI_INTERACTIVE) ? ACQ_INTERACTIVE : ACQ_PARSE );*/
   update_acqstate( ACQ_PARSE );
   getstatblock();	/* send statblock up to host now */
   status = acodeInit(pTheAcodeObject,expname,num_acode_sets,num_tables,
					cur_acode_set,interactive_flag);
   if (status != 0)
   {
	errLogRet(LOGIT,debugInfo,"APstart: Exp: %s not initialized!",
								expname);
	return(status);
   }


   /*****  Start Parser   *****/
   fifoClrStart4Exp(pTheFifoObject);
   status = APint(pTheAcodeObject);


   /*****  Cleanup   *****/
   acodeClear(pTheAcodeObject);

   fifoCloseLog(pTheFifoObject);

   return(status);
}

int APbatch(char *cmdstring)
{
   return( APstart(cmdstring, 0) );
}

int APinteractive(char *cmdstring)
{
   return( APstart(cmdstring, ACQI_INTERACTIVE) );
}

#define AACTIONS (4)
/*****************************************************************/
/* probably should just do APint and not adispatch */
/*****************************************************************/
struct { 
   char keystring[16];
   int  (*func)();
}  AParser_Tab[AACTIONS] = 
{ "PARSE_ACODE", APbatch, "INITX", anop, "ACQI_PARSE", APinteractive,
  "A_CLEAN", anop};

adispatch(char *tmp)
{
   int k,match;
   char estr[200];
   char *key;
   k = 0;
   match = 0;
   do
   {
     key = AParser_Tab[k].keystring;
     if (strncmp(tmp,key,strlen(key)) == 0)
     {
       k = (*AParser_Tab[k].func)(tmp+strlen(key)+1);
       return(k);
     }
     k++;
   }
   while (k < AACTIONS);
   puts("adispatch FAIL");
}

void
APparser()
{
   char msgb[102],tmp[100];
   char *pt1,*pt2;
   int ival;
   /* safety net */
   msgb[101]='\0';
   msgb[100]='\0';
   memset((void *) tmp,'\0',100);
   /* check data structure integrity */

   FOREVER
   {
#ifdef INSTRUMENT
	wvEvent(EVENT_PARSE_READY,NULL,NULL);
#endif
     markReady(APARSER_FLAGBIT);

     ival = msgQReceive(pMsgesToAParser,msgb,100,WAIT_FOREVER);

#ifdef INSTRUMENT
	wvEvent(EVENT_PARSE_BUSY,NULL,NULL);
#endif
     markBusy(APARSER_FLAGBIT);
     if (ival == ERROR)
     {
	printf("PARSER Q ERROR\n");
	errLogSysRet(LOGIT,debugInfo,"PARSER Q ERROR");
	return;
     }
     else
     {
       /* we could get multiple instructions in one message */
       pt1 = msgb;
       while ((pt2  = strchr(pt1,';')) != NULL)
       {
	  strncpy(tmp,pt1,(pt2-pt1));
	  *(tmp + (pt2-pt1)) = '\0';
	  *(tmp + (pt2-pt1+1)) = '\0';
	  if ((ival=adispatch(tmp)) != 0)
	  {
              if ( phandlerBusy() == 0 )  /* if phandler active then don't bother to report any Interpreter Errors */
	      {
       		GenericException.exceptionType = HARD_ERROR;  
       		GenericException.reportEvent = ival;   /* errorcode is returned */
    		/* send error to exception handler task */
    		msgQSend(pMsgesToPHandlr, (char*) &GenericException, 
		   sizeof(EXCEPTION_MSGE), NO_WAIT, MSG_PRI_NORMAL);
	      }
	      else
              {
		DPRINT1(0,"Phandler Busy:  error code %d not reported.\n",ival);
	      }
	  }
	  pt1 = pt2+1;
       }
     }
   }
}


startParser(int priority, int taskoptions, int stacksize)
{
   if (taskNameToId("tParser") == ERROR)
    taskSpawn("tParser",priority,taskoptions,stacksize,APparser,
		  pMsgesToAParser,2,3,4,5,6,7,8,9,10);

}

killParser()
{
   int tid;

   markBusy(APARSER_FLAGBIT);
   if ((tid = taskNameToId("tParser")) != ERROR)
	taskDelete(tid);
}

cleanParser()
{
  int tid;
  if (pMsgesToAParser != NULL)
  {
    msgQDelete(pMsgesToAParser);
    pMsgesToAParser = NULL;
  }
  tid = taskNameToId("tParser");
  if (tid != ERROR)
    taskDelete(tid);
}

cleanMsgesToAParser()
{
  char xmsge[ CONSOLE_MSGE_SIZE ];
  int numleft;
  if (pMsgesToAParser != NULL)
  {
    numleft = msgQNumMsgs(pMsgesToAParser);
    if (numleft)
    {
       while (msgQReceive(pMsgesToAParser, &xmsge[ 0 ], CONSOLE_MSGE_SIZE, 
						NO_WAIT) > 0)
       {
	   DPRINT(1,"cleanMsgesToAParser: Msg Discarded.\n");
       }
    }
  }
}

acodeResetUpdtSem(ACODE_ID pAcodeId)
{
   if (pAcodeId != NULL)
   {

	if (pAcodeId->pSemParseUpdt != NULL)
	{
	   /* give semi thus unblocking stuffing */
	   semGive(pAcodeId->pSemParseUpdt);

	   /* Now Attempt to take it when, when it would block that 	*/
	   /*   is the state we want it in.				*/
	   while (semTake(pAcodeId->pSemParseUpdt,NO_WAIT) != ERROR);
	}

   }
}

void giveParseSem()
{
   if ( (pTheAcodeObject != NULL) && (pTheAcodeObject->pSemParseSuspend != NULL) )
      semGive(pTheAcodeObject->pSemParseSuspend);
}

/*--------------------------------------------------------------*/
/* AParserAA							*/
/* 	Abort sequence for Aparser.				*/
/*--------------------------------------------------------------*/
AParserAA()
{
   int tid;
   if ((tid = taskNameToId("tParser")) != ERROR)
   {
	/* taskSuspend(tid); */
        markBusy(APARSER_FLAGBIT);
	acodeClear(pTheAcodeObject);
	acodeResetUpdtSem(pTheAcodeObject);
	cleanMsgesToAParser();

#ifdef INSTRUMENT
	wvEvent(EVENT_PARSE_RESTART,NULL,NULL);
#endif

	taskRestart(tid);
   }
}

/********************************************************/
/* Create Acode Object -  				*/
/*	 Zero info structure.				*/
/********************************************************/
ACODE_OBJ *acodeCreate(char *expname)
{
   ACODE_OBJ *pAcodeObj;


   /**** SETUP EXP Buffer ****/
   pAcodeObj = (ACODE_OBJ *) malloc(sizeof(ACODE_OBJ));
   if (pAcodeObj == NULL)
   {
    	errLogSysRet(LOGIT,debugInfo,"acodeCreate: Could not Allocate Space:");
    	return(NULL);
   }
   /* zero out structure so we don't free something by mistake */
   memset(pAcodeObj,0,sizeof(ACODE_OBJ));


   strcpy(pAcodeObj->id,expname); 


   /**** Create Interactive Acode Control Semaphore. ****/
   /* This was originally a mutex semaphore but was changed to	*/
   /* binary semaphore to provide more flexibility.		*/
   pAcodeObj->pAcodeControl = NULL;
   pAcodeObj->pAcodeControl = semBCreate(SEM_Q_PRIORITY, SEM_FULL);

   /**** Create Interactive Updt Semaphore. ****/
   pAcodeObj->pSemParseUpdt = NULL;
   pAcodeObj->pSemParseUpdt = semBCreate(SEM_Q_FIFO,SEM_EMPTY);

   /**** Create Sync Operations needing FIFO (AUTOLOCK, AUTOSHIM) ****/
   pAcodeObj->pSemParseSuspend = NULL;
/*
  Note: that putSampFromMagnet() & getSampFromMagnet() called from shandler 
        also take pSemParseSuspend, to wait for the roboproc to report during
        the GETSAMP & LOADSAMP acodes. These Acodes also take the pSemParseSuspend 
        in the standard shandler approach.
        The semaphore was changed from FIFO to PRIORITY type so that the 
        shandler (of higher priority) is released
        then the parser (lower priority) rather than the other way around.   3/11/96
*/
   /* pAcodeObj->pSemParseSuspend = semBCreate(SEM_Q_FIFO,SEM_EMPTY); */
   pAcodeObj->pSemParseSuspend = semBCreate(SEM_Q_PRIORITY,SEM_EMPTY);

   /**** Create Fifo Start WatchDog Timer ****/
   pAcodeObj->wdFifoStart = wdCreate();

   return(pAcodeObj);
}

/********************************************************/
/* acodeDelete -  Deletes acode object and resources	*/
/*	Set up tables, Experiment info structure.	*/
/* RETURNS:						*/
/*  	void						*/
/********************************************************/
void acodeDelete(ACODE_ID pAcodeId)
{
   if (pAcodeId != NULL)
   {
	if (pAcodeId->cur_rtvar_base != NULL)
	{
	   rmRTvarSet(pAcodeId->id);
	   pAcodeId->cur_rtvar_base = NULL;
	}
	if (pAcodeId->num_tables != 0)
	{
	   rmTables(pAcodeId,pAcodeId->id,pAcodeId->num_tables);
	   pAcodeId->num_tables = 0;
	}
	if (pAcodeId->cur_acode_base != NULL)
	{
	   rmAcodeSet(pAcodeId->id, pAcodeId->cur_acode_set);
	   pAcodeId->cur_acode_base = NULL;
	}
	if (pAcodeId->pAcodeControl != NULL)
           semDelete(pAcodeId->pAcodeControl);
	if (pAcodeId->pSemParseUpdt != NULL)
           semDelete(pAcodeId->pSemParseUpdt);
	if (pAcodeId->pSemParseSuspend != NULL)
           semDelete(pAcodeId->pSemParseSuspend);

	if (pAcodeId->wdFifoStart != NULL)
           wdDelete(pAcodeId->wdFifoStart);

	free(pAcodeId);
	pTheAcodeObject = NULL;
   }
}

/*----------------------------------------------------------------------*/
/* acodeShwResrc							*/
/*     Show system resources used by Object (e.g. semaphores,etc.)	*/
/*	Useful to print then related back to WindView Events		*/
/*----------------------------------------------------------------------*/
VOID acodeShwResrc(register ACODE_ID pAcodeId, int indent )
{
   int i;
   char spaces[40];

   for (i=0;i<indent;i++) spaces[i] = ' ';
   spaces[i]='\0';

   printf("%sAcode Obj: '%s', 0x%lx\n",spaces,pAcodeId->id,pAcodeId);
   printf("%s Binary Sems: pAcodeControl ----- 0x%lx\n",spaces,pAcodeId->pAcodeControl);
   printf("%s              pSemParseUpdt  ---- 0x%lx\n",spaces,pAcodeId->pSemParseUpdt);
   printf("%s              pSemParseSuspend  - 0x%lx\n",spaces,pAcodeId->pSemParseSuspend);
   printf("%s W-Dog:       wdFifoStart  ------ 0x%lx\n",spaces,pAcodeId->wdFifoStart);
}


/*----------------------------------------------------------------------*/
/* acodeShow								*/
/*	Displays information on the acodes.				*/
/*----------------------------------------------------------------------*/
acodeShow(ACODE_ID pAcodeId, int level)
{
 char tmpbufname[64],numstring[8];
 int i;
 DLBP active;


   if (pAcodeId == NULL)
   {
   	printf("Acode structure does not exist!!!\n");
	return;
   }

   printf("Acode/exp Name: %s\n\n",pAcodeId->id);
   printf("	Interactive Flag:  %d\n", pAcodeId->interactive_flag);
   printf("	Num of Acode Sets: %d\n", pAcodeId->num_acode_sets);
   printf("	Current Acode Set: %d\n", pAcodeId->cur_acode_set);
   printf("	Number of Tables:  %d\n", pAcodeId->num_tables);

   printf("Acode Update Semaphore: \n");
   printSemInfo(pAcodeId->pSemParseUpdt,"Acode Update Semaphore",level);
   /* semShow(pAcodeId->pSemParseUpdt,level); */

   printf("Acode Control Semaphore: \n");
   printSemInfo(pAcodeId->pAcodeControl,"Acode Control Semaphore",level);
   /* semShow(pAcodeId->pAcodeControl,level); */

   printf("Parser Suspension Semaphore: \n");
   printSemInfo(pAcodeId->pSemParseSuspend,"Acode Suspension Semaphore",level);
   /* semShow(pAcodeId->pSemParseSuspend,level); */

   printf("Fifo Start Watchdog Timer: \n");
   wdShow(pAcodeId->wdFifoStart);

   /* Display info on Current Acode Buffer */
   strcpy(tmpbufname,pAcodeId->id);
   sprintf(numstring,"f%d",pAcodeId->cur_acode_set);
   strcat(tmpbufname,numstring);
   printf("\nAcode Info -- Buffername: %s\n",tmpbufname);
   active = dlbFindEntry(pDlbFixBufs,tmpbufname);
   if (active == NULL)
	printf("No Current Acode Buffer\n");
   else
	dlbPrintEntry(active);


   /* Display info on Real Time Variables */
   strcpy(tmpbufname,pAcodeId->id);
   strcat(tmpbufname,"rtv");
   printf("\nRTvar Info -- Buffername: %s\n",tmpbufname);
   active = dlbFindEntry(pDlbDynBufs,tmpbufname);
   if (active == NULL)
	printf("Could not Find Real Time Variable Buffer for Acode Id\n");
   else
   {
	unsigned int *rtptr;
	dlbPrintEntry(active);
	rtptr = pAcodeId->cur_rtvar_base;
	printf("Buffer Id = 0x%x (hex) %d (dec)\n",*rtptr,*rtptr);
	rtptr++;
	printf("\n Entry    decimal value    hex value \n");
	for (i=0; i< pAcodeId->cur_rtvar_size-1; i++)
	{
	  printf(" %6d     %10d      0x%8x\n",i,*rtptr,*rtptr);
	  rtptr++;
	}  
   }

   /* Display info on Tables */
   if (pAcodeId->num_tables > 0)
   {
	printf("\nTable Info -- \n");
	for (i=0; i < pAcodeId->num_tables; i++)
	{
	   strcpy(tmpbufname,pAcodeId->id);
	   sprintf(numstring,"t%d",i);
	   strcat(tmpbufname,numstring);
	   printf("\nTable Buffername: %s\n",tmpbufname);
	   active = dlbFindEntry(pDlbFixBufs,tmpbufname);
	   if (active == NULL)
		printf("Could not Find Named Table Buffer\n");
	   else
		dlbPrintEntry(active);
	   if (pAcodeId->table_ptr[i] == NULL)
		printf("Null Table Ptr\n");
	   else
	   {
		printf("  Num Entries: %d\n",
					pAcodeId->table_ptr[i]->num_entries);
		printf("  Size Entry:  %d\n",
					pAcodeId->table_ptr[i]->size_entry);
		printf("  Mod Factor:  %d\n",
					pAcodeId->table_ptr[i]->mod_factor);
		printf("  Start Addr:  0x%x\n",&pAcodeId->table_ptr[i]->entry);
	   }

	}
   }

}

/*  These programs setup a canned sequence of A_codes to get the Lock signal  */

/* see acodes.h for these defines */
/* #define  LOCK_CHAN	2 */
/* #define  TEST_CHAN	0 */

typedef struct {
	int	value;
	short	offset;
} rtpar;


static int	stopoffset;

int
getstopoffset()
{
	int	retval;

	retval = stopoffset;
	retval++;
	return( retval );
}

static
putcode_long(unsigned int lcode,unsigned short *acptr)
{
	unsigned short *scode;

	scode = (unsigned short *) &lcode;
	*(acptr)++ = scode[0];
	*(acptr)++ = scode[1];
}

static
dump_acodes( unsigned short *acptr, unsigned int acount )
{
	int	iter;

	for (iter = 0; iter < acount; iter++) {
		printf( "%2d:  %d\n", iter, *acptr );
		acptr++;
	}
}

static
makeLockAcodes( char *name )
{

/*  Note:  watch the size of rtvars and acodes - they must be
           large enough to hold all the data stored there ...	*/

	char	rtname[ 40 ], acname[ 40 ];
	short	acodes[ 256 ], *mark_branch, *pAcode;
	long	rtvars[ 32 ], *rtbase;
	int	branch_offset; 
	int	rtsize, acsize;
	rtpar	bs, cbs, ct, dp, dwell, hkdelay, il, nf, np, nb, nt, ss, ctss;
	rtpar	stop, zero, dtmcntrl, adccntrl, maxsum, activercvrs;
	DLBP	buf_entry;

	bs.value = 0;
	cbs.value = 0;
	ct.value = 0;
	ctss.value = 0;
	dp.value = 4;
	np.value = 512;
	nb.value = 3;			/* changed from 2 to 3, 06/01/1995 */
	nf.value = 1;
	nt.value = 1;
	ss.value = 0;
	/*dwell.value = 10000;	   dwell time is 0.0001 sec, expressed in units of 10 nsec */
	dwell.value = CNT312_5USEC;	/* 312.5 us */
	il.value = 0;
	hkdelay.value = CNT100_MSEC; /* FIFO ticks:  100 ms */

	stop.value = 0;
	zero.value = 0;
	dtmcntrl.value = STM_AP_ENABLE_ADC1 | STM_AP_ENABLE_STM;
	adccntrl.value = ADC_AP_ENABLE_CTC | 
			(LOCK_CHAN << ADC_AP_CHANSELECT_POS);
	maxsum.value = 0x7fffffff;
	activercvrs.value = 1;

	sprintf( &rtname[ 0 ], "%srtv", name );

	rtbase = &rtvars[ 2 ];
	rtvars[ 1 ] = RTVAR_BUFFER;
	rtvars[ 2 ] = np.value;
	np.offset = &rtvars[ 2 ] - rtbase;
	rtvars[ 3 ] = dp.value;
	dp.offset = &rtvars[ 3 ] - rtbase;
	rtvars[ 4 ] = nb.value;
	nb.offset = &rtvars[ 4 ] - rtbase;
	rtvars[ 5 ] = nf.value;
	nf.offset = &rtvars[ 5 ] - rtbase;
	rtvars[ 6 ] = bs.value;
	bs.offset = &rtvars[ 6 ] - rtbase;
	rtvars[ 7 ] = cbs.value;
	cbs.offset = &rtvars[ 7 ] - rtbase;
	rtvars[ 8 ] = ct.value;
	ct.offset = &rtvars[ 8 ] - rtbase;
	rtvars[ 9 ] = nt.value;
	nt.offset = &rtvars[ 9 ] - rtbase;
	rtvars[ 10 ] = ss.value;
	ss.offset = &rtvars[ 10 ] - rtbase;
	rtvars[ 11 ] = dwell.value;
	dwell.offset = &rtvars[ 11 ] - rtbase;
	rtvars[ 12 ] = il.value;
	il.offset = &rtvars[ 12 ] - rtbase;
	rtvars[ 13 ] = hkdelay.value;
	hkdelay.offset = &rtvars[ 13 ] - rtbase;
	rtvars[ 14 ] = stop.value;
	stop.offset = &rtvars[ 14 ] - rtbase;
	rtvars[ 15 ] = zero.value;
	zero.offset = &rtvars[ 15 ] - rtbase;
	rtvars[ 16 ] = dtmcntrl.value;
	dtmcntrl.offset = &rtvars[ 16 ] - rtbase;
	rtvars[ 17 ] = ctss.value;
	ctss.offset = &rtvars[ 17 ] - rtbase;
	rtvars[ 18 ] = adccntrl.value;
	adccntrl.offset = &rtvars[ 18 ] - rtbase;
	rtvars[ 19 ] = maxsum.value;
	maxsum.offset = &rtvars[ 19 ] - rtbase;
	rtvars[ 20 ] = activercvrs.value;
	activercvrs.offset = &rtvars[ 20 ] - rtbase;
	rtsize = 21;
	rtvars[ 0 ] = rtsize;

	stopoffset = stop.offset;

	pAcode = &acodes[ 0 ];
	*pAcode++ = ACODE_BUFFER;
	*pAcode++ = INIT_FIFO;
	*pAcode++ = 2;
	*pAcode++ = 0;
	*pAcode++ = 0;
	*pAcode++ = INIT_STM;
	*pAcode++ = 5;
	*pAcode++ = dp.offset;
	*pAcode++ = np.offset;
	*pAcode++ = nb.offset;
	*pAcode++ = 0;		/* use standard STM, not HS(5MHz) STM/ADC */
	*pAcode++ = activercvrs.offset;
	*pAcode++ = INIT_ADC;
	*pAcode++ = 3;
	*pAcode++ = 0;
	*pAcode++ = adccntrl.offset;
	*pAcode++ = activercvrs.offset;

/*  The next sequence of A-codes set the lock frequency and the control
    word.  For now, the A-codes are to be computed by PSG using the lock
    controller object and executed as part of the su/go command.  These
    A-codes would be executed only once for each time the lock display
    is started.								*/

#if 0
	*pAcode++ = APBCOUT;
	*pAcode++ = 6;
	*pAcode++ = AP_MIN_DELAY_CNT;		/* delay of about 500 ns */
	*pAcode++ = 5;
	*pAcode++ = 0x0b54 | 0x1000 | 0x2000;
	tmp = lkfreqval;
	w1 =  (0x0ff & tmp) << 8;
	tmp >>= 8;
	*pAcode++ = w1 | (tmp & 0x0ff);
	tmp >>= 8;
	w1 =  (0x0ff & tmp) << 8;
	tmp >>= 8;
	*pAcode++ = w1 | (tmp & 0x0ff);
	*pAcode++ = 0;
	*pAcode++ = APBCOUT;
	*pAcode++ = 4;
	*pAcode++ = AP_MIN_DELAY_CNT;
	*pAcode++ = 1;
	*pAcode++ = 0x0b57 | 0x1000;
	*pAcode++ = 0x0;
#endif

/*  Set the lock systems duty cycle to 20 Hz, helpful for obtaining a lock signal.  */

	*pAcode++ = APBCOUT;
	*pAcode++ = 4;
	*pAcode++ = AP_MIN_DELAY_CNT;		/* delay of about 500 ns */
	*pAcode++ = 1;
	*pAcode++ = 0x0b51 | 0x1000;
	*pAcode++ = 0x1a << 8;		/* value of 0x1a required to set the duty cycle */

/*  Start the FIFO up now, before beginning the lock display loop, so stuff
    can be set on the AP bus without requiring a start-on-sync.			*/

	*pAcode++ = FIFOHALT;
	*pAcode++ = 0;
	*pAcode++ = FIFOSTART;
	*pAcode++ = 0;
	*pAcode++ = FIFOWAIT4STOP;
	*pAcode++ = 0;

/*  End of stuff done only once for each time the lock display is started.  */

	mark_branch = pAcode;

	*pAcode++ = APBCOUT;
	*pAcode++ = 4;
	*pAcode++ = AP_MIN_DELAY_CNT;		/* delay of about 500 ns */
	*pAcode++ = 1;
	*pAcode++ = 0x0b51 | 0x1000;
	*pAcode++ = 0x1a << 8;		/* value of 0x1a required to set the duty cycle */

	*pAcode++ = RT2OP;
	*pAcode++ = 3;
	*pAcode++ = SET;
	*pAcode++ = zero.offset;
	*pAcode++ = ct.offset;

	*pAcode++ = INITSCAN;
	*pAcode++ = 9;
	*pAcode++ = np.offset;
	*pAcode++ = ss.offset;
	*pAcode++ = ss.offset;		/* supposed to be ssct, but it's always 0 */
	*pAcode++ = nt.offset;
	*pAcode++ = ct.offset;
	*pAcode++ = bs.offset;
	*pAcode++ = cbs.offset;
	*pAcode++ = maxsum.offset;
	*pAcode++ = activercvrs.offset;

	*pAcode++ = APBCOUT;
	*pAcode++ = 4;
	*pAcode++ = AP_MIN_DELAY_CNT;		/* delay of about 500 ns */
	*pAcode++ = 1;
	*pAcode++ = 0x0e86;
	*pAcode++ = 0;		/* DSP pass through mode */

	*pAcode++ = NEXTSCAN;
	*pAcode++ = 8;
	*pAcode++ = np.offset;
	*pAcode++ = dp.offset;
	*pAcode++ = nt.offset;
	*pAcode++ = ct.offset;
	*pAcode++ = bs.offset;
	*pAcode++ = dtmcntrl.offset;
	*pAcode++ = nf.offset;
	*pAcode++ = activercvrs.offset;

/*	*pAcode++ = LOCKPHASE_R;
	*pAcode++ = 1;
	*pAcode++ = lockphase.offset;
	*pAcode++ = LOCKPOWER_R;
	*pAcode++ = 1;
	*pAcode++ = lockpower.offset;
	*pAcode++ = LOCKGAIN_R;
	*pAcode++ = 1;
	*pAcode++ = lockgain.offset;
	*pAcode++ = EVENT1_DELAY;
	*pAcode++ = 2;
	*pAcode++ = 512;
	*pAcode++ = 0;			*/

/*  Next delay was worked out experimentally to give a good synchronization
    between the lock transmitter gating and the resulting signal.		*/

	*pAcode++ = EVENT1_DELAY;
	*pAcode++ = 2;
	putcode_long( CNTLKACQUIRE, pAcode);		/* ca 42.5 ms */
	pAcode += 2;
	*pAcode++ = ENABLEOVRLDERR;
	*pAcode++ = 5;
	*pAcode++ = ss.offset;		/* supposed to be ssct, but it's always 0 */
	*pAcode++ = ct.offset;
	*pAcode++ = dtmcntrl.value;
	*pAcode++ = adccntrl.offset;
	*pAcode++ = activercvrs.offset;
	*pAcode++ = TACQUIRE;
	*pAcode++ = 3;
	*pAcode++ = 0;			/* flags for TACQUIRE */
	*pAcode++ = np.offset;
	*pAcode++ = dwell.offset;
	*pAcode++ = DISABLEOVRLDERR;
	*pAcode++ = 3;
	*pAcode++ = dtmcntrl.value;
	*pAcode++ = adccntrl.offset;
	*pAcode++ = activercvrs.offset;
	*pAcode++ = ENDOFSCAN;
	*pAcode++ = 7;
	*pAcode++ = ss.offset;
	*pAcode++ = ss.offset;		/* supposed to be ssct, but it's always 0 */
	*pAcode++ = nt.offset;
	*pAcode++ = ct.offset;
	*pAcode++ = bs.offset;
	*pAcode++ = cbs.offset;
	*pAcode++ = activercvrs.offset;
/*	*pAcode++ = FIFOSTARTSYNC; */
/* 	*pAcode++ = 0;	*/
	*pAcode++ = LOCK_UPDT;
	*pAcode++ = 2;
	*pAcode++ = il.offset;
	*pAcode++ = hkdelay.offset;
	*pAcode++ = FIFOWAIT4STOP_2;
	*pAcode++ = 0;
	*pAcode++ = BRA_EQ;
	*pAcode++ = 4;
	*pAcode++ = zero.offset;
	*pAcode++ = stop.offset;
	branch_offset = (mark_branch - pAcode);
	putcode_long( branch_offset, pAcode);
	pAcode += 2;
	*pAcode++ = FIFOHARDRESET;
	*pAcode++ = 0;

/*  Set the lock systems duty cycle back to 2 KHz  */
/*  determined 03/29/95 that 0x0a sets the correct duty cycle  */

	*pAcode++ = APBCOUT;
	*pAcode++ = 4;
	*pAcode++ = AP_MIN_DELAY_CNT;		/* delay of about 500 ns */
	*pAcode++ = 1;
	*pAcode++ = 0x0b51 | 0x1000;
	*pAcode++ = 0x0a << 8;

	*pAcode++ = FIFOHALT;
	*pAcode++ = 0;
	*pAcode++ = FIFOSTART;
	*pAcode++ = 0;
	*pAcode++ = FIFOWAIT4STOP;
	*pAcode++ = 0;
	*pAcode++ = END_PARSE;
	*pAcode++ = 0;
	*pAcode++ = 0;
	*pAcode++ = 0;

	acsize = (pAcode - &acodes[ 0 ]);
	sprintf( &acname[ 0 ], "%sf0", name );

	buf_entry = dlbFindEntry( pDlbFixBufs, &acname[ 0 ] );
	if (buf_entry != NULL) {
		printf( "another entry of name %s, deleting it\n", &acname[ 0 ] );
		dlbFree( buf_entry );
	}
	buf_entry = dlbMakeEntry( pDlbFixBufs, &acname[ 0 ], 
						acsize * sizeof(short));
	if (buf_entry == NULL) {
		printf( "Can't make a named buffer entry '%s'\n", &acname[ 0 ] );
		return( -1 );
	}
	memcpy( buf_entry->data_array, &acodes[ 0 ], acsize * sizeof(short) );
	/*dump_acodes( (short *) buf_entry->data_array, acsize );*/
        buf_entry->status = READY;

	buf_entry = dlbFindEntry( pDlbDynBufs, &rtname[ 0 ] );
	if (buf_entry != NULL) {
		printf( "another entry of name %s, deleting it\n", &rtname[ 0 ] );
		dlbFree( buf_entry );
	}
	buf_entry = dlbMakeEntry( pDlbDynBufs, &rtname[ 0 ],
						 rtsize * sizeof(long) );
	if (buf_entry == NULL) {
		printf( "Can't make a named buffer entry '%s'\n", &rtname[ 0 ] );
		return( -1 );
	}
	memcpy( buf_entry->data_array, &rtvars[ 0 ], rtsize * sizeof( long ) );
        buf_entry->status = READY;

	return( 0 );
}

setup_for_lock()
{
	int	ival;

	ival = makeLockAcodes( "lock" );
	if (ival != 0) {
    		errLogSysRet(LOGIT,debugInfo,"Warning: Could not Allocate Lock A-codes:");
	}
}

