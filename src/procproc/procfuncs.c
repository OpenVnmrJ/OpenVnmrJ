/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pwd.h>

#include <errno.h>

#include "errLogLib.h"
#include "mfileObj.h"
#include "shrMLib.h"
#include "shrexpinfo.h"
#include "shrstatinfo.h"
#include "procQfuncs.h"
#include "process.h"
#include "data.h"



#define S_OLD_COMPLEX   0x40
#define S_OLD_ACQPAR    0x80

extern ExpInfoEntry ActiveExpInfo;
extern ExpInfoEntry ProcExpInfo;
extern char ProcName[256];

extern int mapInExp(ExpInfoEntry *expid);
extern int mapOutExp(ExpInfoEntry *expid);
extern void KillProcess(int pid);
extern void expStatusRelease();
extern int  shutdownComm(void);


SHR_EXP_INFO expInfo = NULL;   /* start address of shared Exp. Info Structure */

static SHR_MEM_ID  ShrExpInfo = NULL;  /* Shared Memory Object */

/**************************************************************
*
*  expId -  set expId for following conditional process queueing
*  requests  for this Exp.
*
*  1. mapin Exp Info Dbm 
*
* RETURNS:
* 0 , else -1
*
*       Author Greg Brissey 9/6/94
*/
int expId(char *argstr)
{
    char* filename;

    filename = strtok(NULL," ");

    DPRINT1(1,"expId: Experiment  Info File: '%s'\n",filename);

  if (ActiveExpInfo.ExpInfo == NULL)
  {
    strncpy(ActiveExpInfo.ExpId,filename,EXPID_LEN);
    ActiveExpInfo.ExpId[EXPID_LEN-1] = '\0';
    if ( mapInExp(&ActiveExpInfo) == -1)
      {
         errLogRet(LOGOPT,debugInfo,
            "expId: Exp Info Not Present, Map in request Ignored.");
         return(-1);
      }
  }
  else
  {
     if( strcmp(ActiveExpInfo.ExpId,filename) != 0)    /* NO, close one, open new */
     {
	mapOutExp(&ActiveExpInfo);
	/* map in new processing Exp Info file */
        strncpy(ActiveExpInfo.ExpId,filename,EXPID_LEN);
        ActiveExpInfo.ExpId[EXPID_LEN-1] = '\0';
        mapInExp(&ActiveExpInfo);
     }
  }
  return(0);
}

/**************************************************************
*
*  chkQueue -  Another Process has put an Entry into the Q.
*
* Another process has placed a new entry into the processing
* Queue.
*
* Acutal for now we do nothing here, the interrupt for the message
*  is enough to get things rolling. In the future on could do
*  some task here.
*
* RETURNS:
* 0 , else -1
*
*       Author Greg Brissey 9/9/94
*/
int chkQueue(char *argstr)
{
    return(0);
}


/**************************************************************
*
*  wExp -  Queue When Experiment Complete Processing
*
*  1. 
*  2. 
*  If any Error Occurs during transfer, Send Msg to Expproc.
*  Expproc will take the proper action.
*
* RETURNS:
* 0 , else -1
*
*       Author Greg Brissey 8/30/94
*/
int wExp(char *argstr)
{
#ifdef TODO
   char *value;
   unsigned int  fid,ct;
   value = strtok(NULL," ");
   fid = atol(value);
   value = strtok(NULL," ");
   ct = atol(value);
   DPRINT(1,"wExp: When Exp Processing\n");
   if (ActiveExpInfo.ExpInfo == NULL)
   {
      errLogRet(ErrLogOp,debugInfo, "wExp: Exp. DataBase not Opened.\n");
      return(-1);
   }
   else
   {
      if ( ActiveExpInfo.ExpInfo->ProcMask & WHEN_EXP_PROC )
      {
         if (ActiveExpInfo.ExpInfo->ProcWait > 0)   /* au(wait) or just au */
         {
            DPRINT3(1,"wExp(wait): Queue Exp Processing on Exp: '%s', FID: %d, CT: %d\n",
		ActiveExpInfo.ExpId, fid, ct);
            procQadd(WEXP_WAIT, ActiveExpInfo.ExpId, fid, ct);
         }
         else
         {
            DPRINT3(1,"wExp: Queue Exp Processing on Exp: '%s', FID: %d, CT: %d\n",
		ActiveExpInfo.ExpId, fid, ct);
            procQadd(WEXP, ActiveExpInfo.ExpId, fid, ct);
         }
      }
   }
#endif
   return(0);
}

/**************************************************************
*
*  wNt -  Queue When FID Complete Processing
*
*  1. 
*  2. 
*  If any Error Occurs during transfer, Send Msg to Expproc.
*  Expproc will take the proper action.
*
* RETURNS:
* 0 , else -1
*
*       Author Greg Brissey 8/30/94
*/
int wNt(char *argstr)
{
#ifdef TODO
   char *value;
   int stat;
   unsigned int  fid,ct;
   value = strtok(NULL," ");
   fid = atol(value);
   value = strtok(NULL," ");
   ct = atol(value);
   DPRINT(1,"wNt: When NT (FID) Processing\n");
   if (ActiveExpInfo.ExpInfo == NULL)
   {
      errLogRet(ErrLogOp,debugInfo, "wNt: Exp. DataBase not Opened.\n");
      return(-1);
   }
   else
   {
      if ( ActiveExpInfo.ExpInfo->ProcMask & ( WHEN_GA_PROC | WHEN_NT_PROC) )
      {
         stat = procQadd(WFID, ActiveExpInfo.ExpId, fid, ct);
         if (stat != SKIPPED)
         {
           DPRINT3(1,"wNt: Queue FID Processing on Exp: '%s', FID: %d, CT: %d\n",
		ActiveExpInfo.ExpId, fid, ct);
  	 }
         else
	 {
	   DPRINT(1,"wNt: Queueing Skipped\n");
	 }
      }
   }
#endif
   return(0);
}

/**************************************************************
*
*  wBs -  Queue When BlockSize Complete Processing
*
*  1. 
*  2. 
*  If any Error Occurs during transfer, Send Msg to Expproc.
*  Expproc will take the proper action.
*
* RETURNS:
* 0 , else -1
*
*       Author Greg Brissey 8/30/94
*/
int wBs(char *argstr)
{
#ifdef TODO
   char *value;
   int stat;
   unsigned int  fid,ct;
   value = strtok(NULL," ");
   fid = atol(value);
   value = strtok(NULL," ");
   ct = atol(value);
   DPRINT(1,"wBs: When BlockSize Processing\n");
   if (ActiveExpInfo.ExpInfo == NULL)
   {
      errLogRet(ErrLogOp,debugInfo, "wBs: Exp. DataBase not Opened.\n");
      return(-1);
   }
   else
   {
      if ( ActiveExpInfo.ExpInfo->ProcMask & WHEN_BS_PROC )
      {
         stat = procQadd(WBS, ActiveExpInfo.ExpId, fid, ct);
         if (stat != SKIPPED)
         {
           DPRINT3(1,"wBs: Queue BlockSize Processing on Exp: '%s', FID: %d, CT: %d\n",
		ActiveExpInfo.ExpId, fid, ct);
  	 }
         else
	 {
	   DPRINT(1,"Wbs: Queueing Skipped\n");
	 }
      }
   }
#endif
   return(0);
}


/**************************************************************
*
*  wError -  Queue When Error Processing
*
*  If any Error Occurs during transfer, Send Msg to Expproc.
*  Expproc will take the proper action.
*
* RETURNS:
* 0 , else -1
*
*       Author Greg Brissey 8/30/94
*/
int wError(char *argstr)
{
#ifdef TODO
   char *value;
   unsigned int  fid,ct;
   value = strtok(NULL," ");
   fid = atol(value);
   value = strtok(NULL," ");
   ct = atol(value);
   DPRINT(1,"wError: When Error Processing\n");
   if (ActiveExpInfo.ExpInfo == NULL)
   {
      errLogRet(ErrLogOp,debugInfo, "wError: Exp. DataBase not Opened.\n");
      return(-1);
   }
   else
   {
      if ( ActiveExpInfo.ExpInfo->ProcMask & WHEN_ERR_PROC )
      {
         DPRINT3(1,"wError: Queue Error Processing on Exp: '%s', FID: %d, CT: %d\n",
		ActiveExpInfo.ExpId, fid, ct);
         procQadd(WERR, ActiveExpInfo.ExpId, fid, ct);
      }
   }
#endif
   return(0);
}

/**************************************************************
*
*  qError -  Queue When Error Processing
*
*  Some errors (Warnings and soft-error) are only reported
*  to Expproc. Expproc sends them here for action
*
* RETURNS:
* 0 , else -1
*
*/
int qError(char *argstr)
{
   char *value;
   unsigned int  fid,ct;
   int   dcode, ecode;
   char *expid;

   expid = strtok(NULL," ");
   value = strtok(NULL," ");
   fid = atol(value);
   value = strtok(NULL," ");
   ct = atol(value);
   value = strtok(NULL," ");
   dcode = atol(value);
   value = strtok(NULL," ");
   ecode = atol(value);
   DPRINT(1,"qError: When Error Processing\n");
   DPRINT5(1,"Queue Error for Exp: '%s', FID: %d, CT: %d Donecode= %d Errorcode= %d\n",
		expid, fid, ct, dcode, ecode);
   procQadd(WERR, expid, fid, ct, dcode, ecode);

#ifdef XXX
  /* ----------  ActiveExpInfo.ExpInfo is always NULL at this point  6/10/95 GMB ------------------ */
   if (ActiveExpInfo.ExpInfo == NULL)
   {
      errLogRet(ErrLogOp,debugInfo, "qError: Exp. DataBase not Opened.\n");
      return(-1);
   }
   else
   {
      DPRINT5(1,"Queue Error for Exp: '%s', FID: %d, CT: %d Donecode= %d Errorcode= %d\n",
		expid, fid, ct, dcode, ecode);
      procQadd(WERR, expid, fid, ct, dcode, ecode);
   }
#endif
   return(0);
}

int abortCodes(char *str)
{
   DPRINT(1,"abortCodes: \n");
   return(0);
}

int qStatus()
{
   fprintf(stdout,"\nPending Process Queue\n");
   fprintf(stdout,"-----------------------\n");
   procQshow();
   fprintf(stdout,"\nActive Process Queue\n");
   fprintf(stdout,"-----------------------\n");
   activeQshow();
   return(0);
}

/* Routine call by exception handlers to exit gracefully */
void ShutDownProc()
{
   if (ProcExpInfo.ExpInfo != NULL)
   {
      mapOutExp(&ProcExpInfo);
   }
   shutdownComm();
   procQRelease();
   activeQRelease();
   expStatusRelease();
}

int resetProcproc(char *str)
{
   int activetype,fgbg,pid,dcode;
   char ActiveId[EXPID_LEN];

   DPRINT(1,"Reset: Clear Qs and Kill any BG processing.\n");

   procQclean();
   if (activeQget(&activetype, ActiveId, &fgbg, &pid, &dcode ) != -1) /* no active processing */
   {
      if (fgbg != FG)
      {
         KillProcess(pid);
      }
      activeQclean();
   }

   if (ProcExpInfo.ExpInfo != NULL)
   {
      mapOutExp(&ProcExpInfo);
   }
   return(0);
}

int terminate(char *str)
{
   DPRINT(1,"terminate: \n");
   if (ProcExpInfo.ExpInfo != NULL)
   {
      mapOutExp(&ProcExpInfo);
   }
   shutdownComm();
   procQRelease();
   activeQRelease();
   expStatusRelease();
   exit(0);
}

int debugLevel(char *str)
{
    extern int DebugLevel;
    char *value;
    int  val;
    value = strtok(NULL," ");
    val = atoi(value);
    DPRINT1(0,"debugLevel: New DebugLevel: %d\n",val);
    DebugLevel = val;
    return(0);
}

int mapInExp(ExpInfoEntry *expid)
{
    DPRINT1(1,"mapInExp: map Shared Memory Segment: '%s'\n",expid->ExpId);

    expid->ShrExpInfo = shrmCreate(expid->ExpId,SHR_EXP_INFO_KEY,(unsigned long)sizeof(SHR_EXP_STRUCT)); 
    if (expid->ShrExpInfo == NULL)
    {
//       errLogSysRet(LOGOPT,debugInfo,"mapInExp: shrmCreate() failed:");
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
    DPRINT1(2,"'%s': shrmShow():\n",ProcName);
    if (DebugLevel >= 2)
      shrmShow(expid->ShrExpInfo);
#endif

    expid->ExpInfo = (SHR_EXP_INFO) shrmAddr(expid->ShrExpInfo);

    return(0);
}

int mapOutExp(ExpInfoEntry *expid)
{
    DPRINT1(1,"mapOutExp: unmap Shared Memory Segment: '%s'\n",expid->ExpId);

    if (expid->ShrExpInfo != NULL)
      shrmRelease(expid->ShrExpInfo); 

    memset((char*)expid,0,sizeof(ExpInfoEntry));  /* clear struct */

    return(0);
}

int mapIn(char *str)
{
    char* filename;

    filename = strtok(NULL," ");

    DPRINT1(1,"mapIn: map Shared Memory Segment: '%s'\n",filename);

    ShrExpInfo = shrmCreate(filename,1,(unsigned long)sizeof(SHR_EXP_STRUCT)); 
    if (ShrExpInfo == NULL)
    {
       errLogSysRet(LOGOPT,debugInfo,"mapIn: shrmCreate() failed:");
       return(-1);
    }
    if (ShrExpInfo->shrmem->byteLen < sizeof(SHR_EXP_STRUCT))
    {
        /* hey, this file is not a shared Exp Info file */
       shrmRelease(ShrExpInfo);         /* release shared Memory */
       unlink(filename);        /* remove filename that shared Mem created */
       ShrExpInfo = NULL;
       expInfo = NULL;
       return(-1);
    }

#ifdef DEBUG
    DPRINT1(2,"'%s': shrmShow():\n",ProcName);
    if (DebugLevel >= 2)
      shrmShow(ShrExpInfo);
#endif

    expInfo = (SHR_EXP_INFO) shrmAddr(ShrExpInfo);

    return(0);
}

int mapOut(char *str)
{
/*
    char* filename;

    filename = strtok(NULL," ");

    DPRINT1(1,"mapOut: unmap Shared Memory Segment: '%s'\n",filename);
*/

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
*  UpdateStatus - updates Status 
*
*
* RETURNS:
* 0 , else -1
*
*       Author Greg Brissey 8/12/94
*/
void UpdateStatus(ExpInfoEntry* pProcExpInfo)
{
   char ExpN[EXPSTAT_STR_SIZE];
   DPRINT(2,"Update Proccessing Status: \n");
    /* Update Status Block for New Active Experiment */
   setStatProcExpId(pProcExpInfo->ExpId);
   setStatProcUserId(pProcExpInfo->ExpInfo->UserName);
   sprintf(ExpN,"exp%d",pProcExpInfo->ExpInfo->ExpNum);
   setStatProcExpName(ExpN);
}

/*--------------------------------------------------------------------
| getUserUid()
|       get the user's  uid & gid outof the passwd file
+-------------------------------------------------------------------*/
int getUserUid(char *user, int *uid, int *gid)
{
    struct passwd *pswdptr;

    if ( (pswdptr = getpwnam(user)) == ((struct passwd *) -1) )
    {
        *uid = *gid = -1;
        return(-1);
    }   
    *uid = pswdptr->pw_uid;
    *gid = pswdptr->pw_gid;
    DPRINT3(1,"getUserUid: user: '%s', uid = %d, gid = %d\n", user,*uid,*gid);
    return(0);
}
