/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include "errLogLib.h"
#include "mfileObj.h"
#include "shrMLib.h"
#include "shrexpinfo.h"
#include "procQfuncs.h"
#include "msgQLib.h"
#include "hostMsgChannels.h"
#include "process.h"
#include "expDoneCodes.h"

ExpInfoEntry ActiveExpInfo;
ExpInfoEntry ProcExpInfo;

/*
   1. get highest conditional processing task off queue.
   2. If process of equal priority running just return to try it another day.
   3. If a lower priority is running in FG, can't do anything return to try it another day.
   4. If a lower priority is running in BG then abort it and return, we'll be back.
   5. If no processing running at this point start either FG or BG processing.
   6. If ProcId is NULL mapin ExpInfo file, if its different mapout old mapin new.
   7. If ProcId in Not NULL, if ExpInfo file different mapout one mapin new, etc.. 
   8. Call BeginProcess() this will either FG or BG the processing.

   1 - 8 tested..

   Queustions still to be answered........
   What are donecode & errorcode in respect to new system (and old) need to stay
        compatible ????
   Rules of resume ???
   Rules of which exp to do processing in needs to be rehashed out.
   Actual communication to Vnmr, Async MsgQ, Async Sockets ?
   Actual communication back to Procproc from Vnmr ?
   These will shape the actual strings passed back and forth
*/

extern int getUserUid(char *user, int *uid, int *gid);
extern int verifyInterface( char *interface );
extern int deliverMessage( char *interface, char *message );
extern int mapInExp(ExpInfoEntry *expid);
extern int mapOutExp(ExpInfoEntry *expid);
extern void UpdateStatus(ExpInfoEntry* pProcExpInfo);
   
static unsigned int FGkey = 0L;

extern char Vnmrpath[];
int Auto_Pid;
static int childpid = 0;
static char User[128];
static mode_t umask4Vnmr;
int recheckFG = 1;

void BeginProcessing(ExpInfoEntry* pProcExpInfo, int proctype, int fid, int ct,
                     int dCode, int eCode);

void initQExpInfo()
{
    memset((char*)&ActiveExpInfo,0,sizeof(ExpInfoEntry));
    memset((char*)&ProcExpInfo,0,sizeof(ExpInfoEntry));
}

int procQTask()
{
   char ProcId[EXPID_LEN];
   char ActiveId[EXPID_LEN];
   int proctype,activetype,fgbg,pid;
   int fid,ct;
   int dCode,eCode;
   int activeDcode;

   DPRINT(1,"Procproc: start procQtask()(chk processing Q)\n");
   if ( procQget(&proctype, ProcId, &fid, &ct, &dCode, &eCode) == -1)  /* nothin in Q ? */
     return(0);

   DPRINT2(1,"Procproc: got Qed Task, Done Code: %d Error Code: %d\n",dCode,eCode);
   if (activeQget(&activetype, ActiveId, &fgbg, &pid, &activeDcode ) == -1) /* no active processing */
   {
       if (ProcExpInfo.ExpInfo == NULL)
       {
 	      /* map in new processing Exp Info file */
    	      strncpy(ProcExpInfo.ExpId,ProcId,EXPID_LEN);
    	      ProcExpInfo.ExpId[EXPID_LEN-1] = '\0';
	      if (mapInExp(&ProcExpInfo) == -1)
	      {
                /* if expinfo file not there skip this processing,
                 * remove it from the process queue, and get the next
                 * queued item.
                 */
		ProcExpInfo.ExpInfo = NULL;
	        procQdelete(proctype, 1);
                procQTask();
	        return(0);
	      }
	      /* Both ProcExpInfo & ActiveExpInfo were NULL so they will be
		 the same */
	      /* memcpy((char*)&ActiveExpInfo,(char*)&ProcExpInfo,
			     sizeof(ExpInfoEntry)); */
              UpdateStatus(&ProcExpInfo);
       }
       else   /* is the present ProcExpInfo the right one */
       {
	   if( strcmp(ProcExpInfo.ExpId,ProcId) != 0)    /* NO, close one, open new */
	   {
	       mapOutExp(&ProcExpInfo);
 	       /* map in new processing Exp Info file */
    	       strncpy(ProcExpInfo.ExpId,ProcId,EXPID_LEN);
    	       ProcExpInfo.ExpId[EXPID_LEN-1] = '\0';
	       if (mapInExp(&ProcExpInfo) == -1)
	       {
                /* if expinfo file not there skip this processing,
                 * remove it from the process queue, and get the next
                 * queued item.
                 */
		ProcExpInfo.ExpInfo = NULL;
	        procQdelete(proctype, 1);
                procQTask();
                return(0);
	       }
               UpdateStatus(&ProcExpInfo);
	   }
       }
       BeginProcessing(&ProcExpInfo, proctype, fid, ct, dCode, eCode); 
   }
   else    /* There is an active processing going on */
   {
       if ( (proctype != WEXP_WAIT) && (proctype != WEXP) &&
            (proctype != WERR) )
       {
          /* Only check active queue at WEXP and WERR */
          return(0);
       }
       if (ActiveExpInfo.ExpInfo == NULL)
       {
          char vnhost[256];
          int vnport, vnpid;

 	  /* map in new processing Exp Info file */
    	  strncpy(ActiveExpInfo.ExpId,ActiveId,EXPID_LEN);
    	  ActiveExpInfo.ExpId[EXPID_LEN-1] = '\0';
	  if (mapInExp(&ActiveExpInfo) == -1)
	  {
             /* If the info file is not there, active element is done */
	     ActiveExpInfo.ExpInfo = NULL;
             activeQdelete(FG,pid);
             procQTask();
	     return(0);
	  }
          if (fgbg == BG)
          {
             vnpid = pid;
          }
          else
          {
             sscanf(ActiveExpInfo.ExpInfo->MachineID, "%s %d %d", vnhost,&vnport,&vnpid);
          }
          if (( kill(vnpid,0) == -1) && (errno == ESRCH))
          {
            /* if the process which was doing active processing is gone,
             * remove the active element
             */
            activeQdelete(FG,pid);
	    mapOutExp(&ActiveExpInfo);
	    ActiveExpInfo.ExpInfo = NULL;
            unlink(ActiveId);  /* delete exp info file */
            procQTask();
          }
          else if (fgbg == BG)
          {
            /* if the process which was doing active processing is present,
             * but in background, just let it continue
             */
	    mapOutExp(&ActiveExpInfo);
	    ActiveExpInfo.ExpInfo = NULL;
          }
          else if (recheckFG)
          {
            char msgestring[256];

            /* if the process which was doing active processing is present,
             * send another message for FGcomplt().  If it is still busy,
             * the message will be queued and an extra FGcomplt will be
             * delivered.  It will be ignored.
             */
            sprintf(msgestring,"acqsend('%d,0,','check')\n", (int)FGkey);
            deliverMessage( ActiveExpInfo.ExpInfo->MachineID, msgestring );
	    mapOutExp(&ActiveExpInfo);
	    ActiveExpInfo.ExpInfo = NULL;
            /*
             * avoid sending a lot of these messages. This gets reset to 1 whenever
             * a FG message is received.
             */
            recheckFG = 0;
          }
       }
   }
   return(0);
}

/*------------------------------------------------------------------------
|
|       BeginProcessing()/1
|       Fork & Exec a child for BackGround vnmr processing
|       Send commands to Vnmr for ForeGround processing
|
|                               Author Greg Brissey 9/7/94
|
+-----------------------------------------------------------------------*/
void BeginProcessing(ExpInfoEntry* pProcExpInfo, int proctype, int fid, int ct,
                     int dCode, int eCode)
{
    char cmdstring[1024];
    char procmsge[1024];
    char msgestring[1024];
    int process2do;
    MSG_Q_ID pAutoMsgQ;

    /* we no longer pass the actual command string to Vnmr but just it's
       type, and Vnmr will read it's procpar to get the command string 
       This means for Wexp, Wnt & Wexp will need tobe concatenated
    */
    process2do =  pProcExpInfo->ExpInfo->ProcMask;
    DPRINT3(1,"BeginProcessing(): type = %d dcode: %d ecode: %d \n",
               proctype,dCode,eCode);
    switch(proctype)
    {
       case WEXP_WAIT:
       case WEXP:
	    if (pProcExpInfo->ExpInfo->GoFlag != EXEC_GO)
            {
	       process2do &= WHEN_SU_PROC;
               eCode = pProcExpInfo->ExpInfo->GoFlag;  /* type of su done */
            }
	    else
            {
	       process2do &= WHEN_GA_PROC | WHEN_EXP_PROC | WHEN_NT_PROC;
            }
	break;
       case WERR:
               if (dCode == EXP_STARTED)
	          process2do = 0;
               else
	          process2do &= WHEN_ERR_PROC;
	break;
       case WFID:
	       process2do &= (WHEN_NT_PROC | WHEN_GA_PROC);
	break;
       case WBS:
	       process2do &= WHEN_BS_PROC;
	break;
       default:
	       process2do = 0;	/* don't do any */
	break;
    }

    if (++FGkey > 1024)
	FGkey = 1L;
    DPRINT2(1,"BeginProcessing(): flags: automode= %d vpmode= %d\n",
               pProcExpInfo->ExpInfo->ExpFlags & AUTOMODE_BIT,pProcExpInfo->ExpInfo->ExpFlags & VPMODE_BIT);
/*
    sprintf(msgestring,"acqcp('exp%d','%s',%d,%d,%d,%d,%lu,%lu,%lu)\n",
			pProcExpInfo->ExpInfo->ExpNum,
			pProcExpInfo->ExpId,0,0,process2do,(int)FGkey, fid, ct);
 */
    if ( (pProcExpInfo->ExpInfo->ExpFlags & VPMODE_BIT) &&
         (pProcExpInfo->ExpInfo->ExpFlags & AUTOMODE_BIT) )
       sprintf(msgestring,"acqsend(5,'%d',0,%d,'vpa',%d,%d,%d,%d,'%s')\n",
                        (int)FGkey, process2do,dCode,eCode, fid, ct,
                        pProcExpInfo->ExpInfo->DataFile);
    else if (pProcExpInfo->ExpInfo->ExpFlags & VPMODE_BIT)
       sprintf(msgestring,"acqsend(5,'%d',0,%d,'vp',%d,%d,%d,%d,'%s')\n",
                        (int)FGkey, process2do,dCode,eCode, fid, ct,
                        pProcExpInfo->ExpInfo->DataFile);
    else
       sprintf(msgestring,"acqsend(5,'%d',%d,%d,'exp%d',%d,%d,%d,%d)\n",
                        (int)FGkey,pProcExpInfo->ExpInfo->ExpNum,
			process2do,pProcExpInfo->ExpInfo->ExpNum,dCode,eCode, fid, ct);
    /* ---  If FG Vnmr is in Process Exp#  & Wexp Not Processing in FG, --- */
    /*       And not in automation mode                             */
    /* ---  Then do FG processing else do it in BG                      --- */
    /*  FG is Never done for automation Experiments!  */

    if ( (strlen(pProcExpInfo->ExpInfo->MachineID) > (size_t) 1) &&
	  (verifyInterface( pProcExpInfo->ExpInfo->MachineID )) &&
         ( !(pProcExpInfo->ExpInfo->ExpFlags & AUTOMODE_BIT) ||
            ( ((pProcExpInfo->ExpInfo->ExpFlags & VPMODE_BIT) && (pProcExpInfo->ExpInfo->ExpFlags & AUTOMODE_BIT)) ) ) )
    {
 
         /* acqcp('exp#','ExpInfo',donecode,errorcode,WprocType,fid,ct) */

         DPRINT(1,"Send Vnmr(MachineId): acqsend(5,FGkey,Exp#,Process2Do,'exp#',dcode,ecode,fid,ct)\n");
         DPRINT2(1,"Send Vnmr(%s): '%s'\n",
         		pProcExpInfo->ExpInfo->MachineID,
         		msgestring);
         deliverMessage( pProcExpInfo->ExpInfo->MachineID, msgestring );

         DPRINT(1,"BeginProcessing: FG calling activeQadd\n");
	 activeQadd(pProcExpInfo->ExpId, proctype, fid, ct, FG, (int) FGkey, dCode, eCode );
	 procQdelete(proctype, 1);

    }
    else	/* ----- process in BackGround ----- */
    {

        char vnmrmode[20];
        char expnum[16];
        char fidpath[256+4];
        char userpath[256+4];
        char host[256+4];
        char vnmr_acqid[16];
        int ret;
        int uid;
        int gid;
        FILE *fp __attribute__((unused));

        /* Be Absolutely sure there is a Vnmr out there to RUN */
        if ( access(Vnmrpath,R_OK | X_OK) != 0)
        {
           errLogRet(ErrLogOp,debugInfo,
                 "WARNING: BG '%s' Processing for '%s' was NOT performed, \n",
                   procmsge, pProcExpInfo->ExpId);
           errLogRet(ErrLogOp,debugInfo,
                 "         '%s' not found or can not run.\n",
                   Vnmrpath);
           return;   /* don't do any more */
        }

        /*
	    Common Arguments for NORMAL or AUTOMATION Vnmr Arguments
        */
	/* below we first copy the string "acqsend('5',..." to 
	   cmdstring (why? I don't know, msgestring isn't used anymore)
	   and then set the 8th character to '6'. So the cmdstring then
	   reads as "acqsend('6',...". By the way, in acqproc the digits
	   5 and 6 are defines as FGREPLY and FGNOREPLY respectively.
	   For those that need to maintain software this may be of help.
	*/
        strcpy(cmdstring,msgestring);
        cmdstring[8] = '6';
        sprintf(userpath,"-u%s",pProcExpInfo->ExpInfo->UsrDirFile);
        sprintf(host,"-h%s",pProcExpInfo->ExpInfo->MachineID);
        sprintf(vnmr_acqid,"-i%d",pProcExpInfo->ExpInfo->ExpNum);
	sprintf(fidpath,"-d%s",pProcExpInfo->ExpInfo->DataFile);

        /* Are we in Automation Mode ? */
        if ( !(pProcExpInfo->ExpInfo->ExpFlags & AUTOMODE_BIT) )
        {
          /* back mode is used because the experiment will need to be
           * locked by the background Vnmr. The acq mode expects the
           * experiment to already be locked.
           */
          sprintf(vnmrmode,"-mback");
          sprintf(expnum,"-n%d",pProcExpInfo->ExpInfo->ExpNum);
        }
        else     /* Automation Mode */
        {
	    /* Use Exp2 & Exp3 in automation mode */
           if (proctype == WEXP)
              sprintf(expnum,"-n2");
           else
              sprintf(expnum,"-n3");

           sprintf(vnmrmode,"-mauto");

           // len = strlen(fidpath);
            /* Test Auto_Pid ? */
           /* if acquisition in automode and go neither a go(nowait)  */
           /*  or Werror issue a resume to autoproc prior to processing */
           if ( (pProcExpInfo->ExpInfo->ProcWait == AU_NOWAIT) && 
		(proctype != WERR) )
           {
	     DPRINT(1,"BeginProcessing: Prior to vfork - Send Resume\n");
    	     pAutoMsgQ = openMsgQ("Autoproc");
	     if (pAutoMsgQ != NULL)
	     {
	       sendMsgQ(pAutoMsgQ, "resume", strlen("resume"), MSGQ_NORMAL,
 			WAIT_FOREVER); /* NO_WAIT */
	       closeMsgQ(pAutoMsgQ);
	     }
           }

        }
 
 
        /* save User is a static location for forked child */
	strcpy(User,pProcExpInfo->ExpInfo->UserName);
	umask4Vnmr = pProcExpInfo->ExpInfo->UserUmask;
        childpid = fork();  /* fork a child */
        DPRINT1(1,"Start BG process ----  childpid = %d\n",childpid);
        /* -----  Parent vs Child ---- */
        if (childpid == 0)
        {
	    sigset_t signalmask;

	    /* set signal mask for child ti zip */
	    sigemptyset( &signalmask );
    	    sigprocmask( SIG_SETMASK, &signalmask, NULL );

            /* change the user and group ID of the child so that VNMR
               will run with those ID's */
	    getUserUid(User, &uid, &gid);
 
            ret = seteuid(getuid());
            fp = freopen("/dev/null","r",stdin);
            fp = freopen("/dev/console","a",stdout);
            fp = freopen("/dev/console","a",stderr);
            if ( setgid(gid) )
            {
                  DPRINT(1,"BGprocess:  cannot set group ID\n");
            }
 
            if ( setuid(uid) )
            {
                  DPRINT(1,"BGprocess:  cannot set user ID\n");
            }

            /* set umask to be that requested by PSG */
            umask( umask4Vnmr );
              
            ret=execl(Vnmrpath,"Vnmr",vnmrmode,host,vnmr_acqid,expnum,userpath,fidpath,cmdstring,NULL);
            if (ret == -1)
            {
               errLogRet(ErrLogOp,debugInfo,
                 "WARNING: BG '%d' for Exp%d was NOT performed, \n",
                   proctype,pProcExpInfo->ExpInfo->ExpNum);
               errLogRet(ErrLogOp,debugInfo, "         execl on '%s' failed.\n",
                   Vnmrpath);
               return;   /* don't do any more */
            }
        }
        else     /* if (childpid != 0)  Child */
        {
            DPRINT(1,"BeginProcessing: BG calling activeQadd()\n");
	    activeQadd(pProcExpInfo->ExpId, proctype, fid, ct, BG, childpid, dCode, eCode);
	    procQdelete(proctype, 1);
        }
    }
}

int sendproc2BG(ExpInfoEntry* pProcExpInfo, int proctype, char *cmdstring)
{

        char vnmrmode[20];
        char expnum[16];
        char fidpath[256+4];
        char userpath[256+4];
        char host[256+4];
        char vnmr_acqid[16];
        int ret;
        int uid;
        int gid;
        FILE *fp __attribute__((unused));

        /* Be Absolutely sure there is a Vnmr out there to RUN */
        if ( access(Vnmrpath,R_OK | X_OK) != 0)
        {
           errLogRet(ErrLogOp,debugInfo,
                 "WARNING: BG Processing for '%s' was NOT performed, \n",
                    pProcExpInfo->ExpId);
           errLogRet(ErrLogOp,debugInfo,
                 "         '%s' not found or can not run.\n",
                   Vnmrpath);
           return(0);   /* don't do any more */
        }

        sprintf(userpath,"-u%s",pProcExpInfo->ExpInfo->UsrDirFile);

        sprintf(host,"-h%s",pProcExpInfo->ExpInfo->MachineID);
 
        sprintf(vnmr_acqid,"-i%d",pProcExpInfo->ExpInfo->ExpNum);

	sprintf(fidpath,"-d%s",pProcExpInfo->ExpInfo->DataFile);
        sprintf(vnmrmode,"-macq");    /* normal acq BG processing */
        sprintf(expnum,"-n%d",pProcExpInfo->ExpInfo->ExpNum);

        /* save User is a static location for forked child */
	strcpy(User,pProcExpInfo->ExpInfo->UserName);
	umask4Vnmr = pProcExpInfo->ExpInfo->UserUmask;
        DPRINT1(1,"Send BG  Vnmr: '%s'\n", cmdstring);
        childpid = fork();  /* fork a child */
        DPRINT1(1,"Start BG process ---- %d\n",childpid);
        /* -----  Parent vs Child ---- */
        if (childpid == 0)
        {
            /* change the user and group ID of the child so that VNMR
               will run with those ID's */
	    getUserUid(User, &uid, &gid);
 
            ret = seteuid(getuid());
            fp = freopen("/dev/null","r",stdin);
            fp = freopen("/dev/console","a",stdout);
            fp = freopen("/dev/console","a",stderr);
            if ( setgid(gid) )
            {
                  DPRINT(1,"BGprocess:  cannot set group ID\n");
            }
 
            if ( setuid(uid) )
            {
                  DPRINT(1,"BGprocess:  cannot set user ID\n");
            }

            /* set umask to be that requested by PSG */
            umask( umask4Vnmr );

            ret=execl(Vnmrpath,"Vnmr",vnmrmode,host,vnmr_acqid,expnum,userpath,fidpath,cmdstring,NULL);
            if (ret == -1)
            {
               errLogRet(ErrLogOp,debugInfo,
                 "WARNING: BG %d for Exp%d was NOT performed, \n",
                   proctype,pProcExpInfo->ExpInfo->ExpNum);
               errLogRet(ErrLogOp,debugInfo, "         execl on '%s' failed.\n",
                   Vnmrpath);
               return(0);   /* don't do any more */
            }
        }
        else     /* if (childpid != 0)  Child */
        {
           return(childpid);
        }
   return(0);
}

#ifdef XXXNOTUSED
void waitOrWerrResume(char *expIdStr, int proctype)
{
   MSG_Q_ID pAutoMsgQ;
   int sendResume;

  DPRINT2(1,"waitOrWerrResume: Proc ID: '%s', Active ID: '%s'\n",
	ProcExpInfo.ExpId,expIdStr);
  if ( strcmp(expIdStr,ProcExpInfo.ExpId) == 0 )
  {
     DPRINT3(1,"waitOrWerrResume: ProcWait: %d(0=nowait), proctype: %d(%d=WERR)\n",
	ProcExpInfo.ExpInfo->ProcWait,proctype,proctype);
     if ( (ProcExpInfo.ExpInfo->ProcWait == AU_NOWAIT) && 
          (proctype != WERR) )
     {
	sendResume = 0;
     }
     else
     {
	sendResume = 1;
     }
  }
  else
  {
     strncpy(ActiveExpInfo.ExpId,expIdStr,EXPID_LEN);
     ActiveExpInfo.ExpId[EXPID_LEN-1] = '\0';
     DPRINT1(1,"waitOrWerrResume: Mapin ActiveExpInfo: '%s'\n",expIdStr);
     if (mapInExp(&ActiveExpInfo) == -1)
     {
         errLogRet(ErrLogOp,debugInfo,
             "waitOrWerrResume: Could not MapIn: '%s' \n",expIdStr);
	 sendResume = 0;
     }
     else
     {
       DPRINT3(1,"waitOrWerrResume: ActiveWait: %d(0=nowait), proctype: %d(%d=WERR)\n",
	ActiveExpInfo.ExpInfo->ProcWait,proctype,proctype);
        if ( (ActiveExpInfo.ExpInfo->ProcWait == AU_NOWAIT) && 
             (proctype != WERR) )
        {
	   sendResume = 0;
        }
        else
        {
	   sendResume = 1;
        }
	mapOutExp(&ActiveExpInfo);
     }
     if (sendResume)
     {
	DPRINT(1,"waitOrWerrResume: Send Resume\n");
    	pAutoMsgQ = openMsgQ("Autoproc");
	if (pAutoMsgQ != NULL)
	{
	   sendMsgQ(pAutoMsgQ, "resume", strlen("resume"), MSGQ_NORMAL,
 			WAIT_FOREVER); /* NO_WAIT */
	   closeMsgQ(pAutoMsgQ);
	}
     }
  }
}
#endif

/* handle BG processing complete, actions */
void bgProcComplt(char *expIdStr, int proctype, int dCode, int pid) 
{
  MSG_Q_ID pAutoMsgQ;
  ExpInfoEntry *Info;
  DPRINT2(1,"bgProcComplt: Exp ID: '%s', BG Proc ID: '%s'\n",
	ProcExpInfo.ExpId,expIdStr);
  /* set info point to proper Exp Info File */
  if ( strcmp(expIdStr,ProcExpInfo.ExpId) == 0 )
  {
     Info = &ProcExpInfo;
  }
  else  /* not the present one so map it in */
  {
     strncpy(ActiveExpInfo.ExpId,expIdStr,EXPID_LEN);
     ActiveExpInfo.ExpId[EXPID_LEN-1] = '\0';
     DPRINT1(1,"bgProcComplt: Mapin ActiveExpInfo: '%s'\n",expIdStr);
     if (mapInExp(&ActiveExpInfo) == -1)
     {
         errLogRet(ErrLogOp,debugInfo,
             "bgProcComplt: Could not MapIn: '%s' \n",expIdStr);
	 Info = NULL;
     }
     else
     {
	 Info = &ActiveExpInfo;
     }
  }
  /* Hey, if automation Exp, then send cmplt & resume to Autoproc, if appropriate */
  if ( Info->ExpInfo->ExpFlags & AUTOMODE_BIT )
  {
     /* if last of processing then send cmplt & resume (if Werr or Wait) to Autoproc */
     if ( ((proctype == WERR) && ((dCode != WARNING_MSG) && (dCode != EXP_STARTED) ) ) ||
          (proctype == WEXP) || 
          (proctype == WEXP_WAIT) )
     {
        pAutoMsgQ = openMsgQ("Autoproc");
        if (pAutoMsgQ != NULL)
        {
          char msgestring[EXPINFO_STR_SIZE + 32];
          /* Inform Autoproc of completion so it can update the doneQ */
          if (proctype == WERR)
  	  {
            sprintf(msgestring,"cmplt 1 %s",Info->ExpInfo->DataFile);
            /* The -4 after strlen(msgestring) is to remove the ".fid" from DataFile */
            sendMsgQ(pAutoMsgQ, msgestring, strlen(msgestring)-4, MSGQ_NORMAL,
 			WAIT_FOREVER); /* NO_WAIT */
	  }
	  else
  	  {
            sprintf(msgestring,"cmplt 0 %s",Info->ExpInfo->DataFile);
            /* The -4 after strlen(msgestring) is to remove the ".fid" from DataFile */
            sendMsgQ(pAutoMsgQ, msgestring, strlen(msgestring)-4, MSGQ_NORMAL,
 			WAIT_FOREVER); /* NO_WAIT */
	  }

          /* Send resume here if Werr or au(wait)  */
          /* if ( (Info->ExpInfo->ProcWait == 1) || 
           *   (proctype == WERR) )
           */
          /* We used to send a resume only if Werr or au(wait). Now we
           * do it all the time
           */
          {
	     sendMsgQ(pAutoMsgQ, "resume", strlen("resume"), MSGQ_NORMAL,
 			     WAIT_FOREVER); /* NO_WAIT */
          }
          closeMsgQ(pAutoMsgQ);
        }
	else
        {
           errLogRet(ErrLogOp,debugInfo,
                 "bgProcComplt: Could open Autoproc MsgQ\n");
        }
     }
  }

  /* if last of processing then delete shared exp info file */
  if ( ((proctype == WERR) && ((dCode != WARNING_MSG) && (dCode != EXP_STARTED) ) ) ||
        (proctype == WEXP) || 
        (proctype == WEXP_WAIT) )
  {
     mapOutExp(Info);	/* map out exp info file */
     unlink(expIdStr);  /* delete exp info file */
  }
  if (activeQdelete(BG,pid) == -1)  /* remove enter from activeQ */
  {
    errLogRet(ErrLogOp,debugInfo,
	"bgProcComplt: activeQdelete of %d failed\n",pid);
  }
}

/*------------------------------------------------------------------------
|
|       KillProcess()/1
|        kill active BG process
|
|                               Author Greg Brissey 9/7/94
+-----------------------------------------------------------------------*/
void KillProcess(int pid)
{
/*    logprint(STDERR,expid,ProcKillMsge,"ID = %d\n",expid); */
    kill(pid,SIGQUIT); /* kill active lower priority process */
}
