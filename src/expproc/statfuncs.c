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
#include <sys/time.h>
#include <time.h>
#include <fcntl.h>
#include <ctype.h>

#include "errLogLib.h"
#include "hostAcqStructs.h"
#include "mfileObj.h"
#include "shrMLib.h"
#include "shrstatinfo.h"

static EXP_STATUS_INFO ExpStatus;
static SHR_MEM_ID ExpStatusObj;

char *getTimeStr(time_t time, char *timestr,int maxsize);
char *getTimeDif(time_t time, char *timestr);
void initConsoleStatus();

#define EXP_STATUS_NAME "/vnmr/acqqueue/ExpStatus"

/*************************************************************
*  Some programs to work with Time Stamps
*
*    struct timeval {
*       long	tv_sec;
*       long	tv_usec;
*    };
*
*    typedef struct timeval TIMESTAMP;
*/

int
isTimeStampNull( TIMESTAMP *ts )
{
	return (ts->tv_sec == 0 && ts->tv_usec == 0);
}

int
cmpTimeStamp( TIMESTAMP *ts1, TIMESTAMP *ts2 )
{
	int	retval;

	if (ts1->tv_sec != ts2->tv_sec) {
		if (ts1->tv_sec > ts2->tv_sec)
		  retval = 1;
		else
		  retval = -1;
	}
	else if (ts1->tv_usec > ts2->tv_usec)
	  retval = 1;
	else if (ts1->tv_usec < ts2->tv_usec)
	  retval = -1;
	else
	  retval = 0;

	return( retval );
}

/*************************************************************
*  initExpStatus -  setup & initialize Experiment Status
*
*  1. MMAP the file that will be the persistent image of the
*     status.
*  2. If clean flag is not set do not initialize structure.
*
* RETURNS:
* 0 , else -1
*
*       Author Greg Brissey 9/2/94
*/
int initExpStatus(int clean)   /* zero out Exp Status */
{

  if (clean)
  {
     struct stat buf;
     /* If the file already exists and is the right size, don't zero it out */
     if ( ! stat(EXP_STATUS_NAME, &buf) && (buf.st_size == sizeof(EXP_STATUS_STRUCT)) )
        clean = 0;
  }

  ExpStatusObj = shrmCreate(EXP_STATUS_NAME,SHR_EXP_STATUS_KEY, 
			      sizeof(EXP_STATUS_STRUCT));
  if (ExpStatusObj == NULL)
  {
     errLogSysRet(ErrLogOp,debugInfo,"initExpQs: shrmCreate failed:");
     return( -1 );
  }

  /* Set mmap file to proper size */
  ExpStatusObj->shrmem->newByteLen = sizeof(EXP_STATUS_STRUCT); 
  ExpStatus = (EXP_STATUS_INFO) shrmAddr(ExpStatusObj);

  if (clean)
    initConsoleStatus();

  return(0);
}

/*************************************************************
*  expStatusRelease -  release share memory exp status 
*
* RETURNS:
* void
*
*       Author Greg Brissey 12/12/94
*/
void expStatusRelease()
{
  if (ExpStatusObj != NULL)
    shrmRelease(ExpStatusObj);
  ExpStatusObj = NULL;
}

void initConsoleStatus()
{
   shrmTake(ExpStatusObj);
   memset((char*)ExpStatus,0,sizeof(EXP_STATUS_STRUCT));
   ExpStatus->csb.AcqLockLevel = 0;
   ExpStatus->csb.AcqSpinSet = -1;
   ExpStatus->csb.AcqSpinAct = -1;
   ExpStatus->csb.AcqSpinSpeedLimit = -1;
   ExpStatus->csb.AcqSpinSpan = -1;
   ExpStatus->csb.AcqSpinAdj = -1;
   ExpStatus->csb.AcqSpinMax = -1;
   ExpStatus->csb.AcqSpinActSp = -1;
   ExpStatus->csb.AcqSpinProfile = -1;
   ExpStatus->csb.AcqVTSet = 30000;
   ExpStatus->csb.AcqVTAct = 30000;
   ExpStatus->csb.AcqTickCountError = -1;
   ExpStatus->csb.AcqChannelBitsConfig1 = 0;
   ExpStatus->csb.AcqChannelBitsConfig2 = 0;
   ExpStatus->csb.AcqChannelBitsActive1 = 0;
   ExpStatus->csb.AcqChannelBitsActive2 = 0;
   ExpStatus->csb.Acqstate = ACQ_INACTIVE;
   ExpStatus->csb.AcqShimSet = -1;
   ExpStatus->csb.consoleID = -1;
   strcpy(ExpStatus->csb.probeId1,"");
   strcpy(ExpStatus->csb.gradCoilId,"");
   gettimeofday( &(ExpStatus->TimeStamp), NULL);
   shrmGive(ExpStatusObj);
}

char *getStatCmpltTime(char *timestr,int maxsize)
{
   return(getTimeStr(ExpStatus->CompletionTime,timestr,maxsize));
}
int setStatCmpltTime(time_t cmplttime)
{
   ExpStatus->CompletionTime = cmplttime;
   shrmTake(ExpStatusObj);
   gettimeofday( &(ExpStatus->TimeStamp), NULL);
   shrmGive(ExpStatusObj);
   return(0);
}

long getStatRemainingTime()
{
   return(ExpStatus->RemainingTime);
}
char *getStatRemTime(char *timestr)
{
   return(getTimeDif(ExpStatus->RemainingTime,timestr));
}
int setStatRemTime(int diftime)
{
   ExpStatus->RemainingTime = diftime;
   shrmTake(ExpStatusObj);
   gettimeofday( &(ExpStatus->TimeStamp), NULL);
   shrmGive(ExpStatusObj);
   return(0);
}

char *getStatDataTime(char *timestr,int maxsize)
{
   return(getTimeStr(ExpStatus->DataTime,timestr,maxsize));
}
int setStatDataTime()
{
   ExpStatus->DataTime = time(NULL);
   shrmTake(ExpStatusObj);
   gettimeofday( &(ExpStatus->TimeStamp), NULL);
   shrmGive(ExpStatusObj);
   return(0);
}

long getStatExpTime()
{
   return(ExpStatus->ExpTime);
}
int setStatExpTime(double duration)
{
   ExpStatus->ExpTime = (int) (duration + 0.01);
   shrmTake(ExpStatusObj);
   gettimeofday( &(ExpStatus->TimeStamp), NULL);
   ExpStatus->StartTime = ExpStatus->TimeStamp.tv_sec;
   shrmGive(ExpStatusObj);
   return(0);
}

long getStatInQue()
{
   return(ExpStatus->ExpInQue);
}
int setStatInQue(long numInQue)
{
   ExpStatus->ExpInQue = numInQue;
   shrmTake(ExpStatusObj);
   gettimeofday( &(ExpStatus->TimeStamp), NULL);
   shrmGive(ExpStatusObj);
   return(0);
}

int getStatGoFlag()
{
   return(ExpStatus->GoFlag);
}
int setStatGoFlag(int goflag)
{
   ExpStatus->GoFlag = goflag;
   shrmTake(ExpStatusObj);
   gettimeofday( &(ExpStatus->TimeStamp), NULL);
   shrmGive(ExpStatusObj);
   return(0);
}


unsigned long getStatCT()
{
   return(ExpStatus->CT);
}
int setStatCT(unsigned long ct)
{
   ExpStatus->CT = ct;
   shrmTake(ExpStatusObj);
   gettimeofday( &(ExpStatus->TimeStamp), NULL);
   shrmGive(ExpStatusObj);
   return(0);
}

unsigned long getStatElem()
{
   return(ExpStatus->FidElem);
}
int setStatElem(unsigned long elem)
{
   ExpStatus->FidElem = elem;
   shrmTake(ExpStatusObj);
   gettimeofday( &(ExpStatus->TimeStamp), NULL);
   shrmGive(ExpStatusObj);
   return(0);
}

int getSystemVerId()
{
   return(ExpStatus->SystemVerId);
}
int setSystemVerId(int version)
{
   ExpStatus->SystemVerId = version;
   shrmTake(ExpStatusObj);
   gettimeofday( &(ExpStatus->TimeStamp), NULL);
   shrmGive(ExpStatusObj);
   return(0);
}

int getInterpVerId()
{
   return(ExpStatus->InterpVerId);
}
int setInterpVerId(int version)
{
   ExpStatus->InterpVerId = version;
   shrmTake(ExpStatusObj);
   gettimeofday( &(ExpStatus->TimeStamp), NULL);
   shrmGive(ExpStatusObj);
   return(0);
}

/*************************************************************/

int getStatAcqState()
{
   return((int)ExpStatus->csb.Acqstate);
}
int setStatAcqState(int status)
{
   ExpStatus->csb.Acqstate = (short) status;
   shrmTake(ExpStatusObj);
   gettimeofday( &(ExpStatus->TimeStamp), NULL);
   shrmGive(ExpStatusObj);
   return(0);
}

long
getStatAcqFidCnt()
{
	return( ExpStatus->csb.AcqFidCnt );
}
int setStatAcqFidCnt( long fidCnt )
{
	ExpStatus->csb.AcqFidCnt = fidCnt;
	shrmTake(ExpStatusObj);
	gettimeofday( &(ExpStatus->TimeStamp), NULL);
	shrmGive(ExpStatusObj);
   return(0);
}

long
getStatAcqCtCnt()
{
	return( ExpStatus->csb.AcqCtCnt );
}
int setStatAcqCtCnt( long ctCnt )
{
	ExpStatus->csb.AcqCtCnt = ctCnt;
	shrmTake(ExpStatusObj);
	gettimeofday( &(ExpStatus->TimeStamp), NULL);
	shrmGive(ExpStatusObj);
   return(0);
}

int getStatLkLevel()
{
   return((int)ExpStatus->csb.AcqLockLevel);
}
int setStatLkLevel(int lklev)
{
   ExpStatus->csb.AcqLockLevel = (short) lklev;
   shrmTake(ExpStatusObj);
   gettimeofday( &(ExpStatus->TimeStamp), NULL);
   shrmGive(ExpStatusObj);
   return(0);
}

int getStatLkGain()
{
   return((int)ExpStatus->csb.AcqLockGain);
}
int setStatLkGain(int lkval)
{
   ExpStatus->csb.AcqLockGain = (short) lkval;
   shrmTake(ExpStatusObj);
   gettimeofday( &(ExpStatus->TimeStamp), NULL);
   return(0);
}

int getStatLkPower()
{
   return((int)ExpStatus->csb.AcqLockPower);
}
int setStatLkPower(int lkval)
{
   ExpStatus->csb.AcqLockPower = (short) lkval;
   shrmTake(ExpStatusObj);
   gettimeofday( &(ExpStatus->TimeStamp), NULL);
   shrmGive(ExpStatusObj);
   return(0);
}

int getStatLkPhase()
{
   return((int)ExpStatus->csb.AcqLockPhase);
}

int getStatLkMode()
{
   return( (int) ((ExpStatus->csb.AcqLSDVbits >> 2) & 0x03) );
}

int setStatLkPhase(int lkval)
{
   ExpStatus->csb.AcqLockPhase = (short) lkval;
   shrmTake(ExpStatusObj);
   gettimeofday( &(ExpStatus->TimeStamp), NULL);
   shrmGive(ExpStatusObj);
   return(0);
}

static int getGradTuneIndex(char *axis, char *type)
{
    int index;

    *axis = tolower(*axis);
    *type = tolower(*type);
    index = 3 * (*axis - 'x');
    if (*type == 'p') {
	index += 0;		/* OK, so it doesn't do much */
    } else if (*type == 'i') {
	index += 1;
    } else if (*type == 's') {
	index += 2;		/* "Spare" for now */
    }
    if (index >= 0 && index < 9) {
	return index;
    } else {
	return -1;
    }
}

int getStatGradError()
{
    return((int)ExpStatus->csb.gpaError);
}

int getStatGradTune(char *axis, char *type)
{
    int index;

    index = getGradTuneIndex(axis, type);
    if (index >= 0) {
	return (int)ExpStatus->csb.gpaTuning[index];
    } else {
	return -1;
    }
}

int setStatGradTune(char *axis, char *type, int value)
{
    int index;

    index = getGradTuneIndex(axis, type);
    if (index >= 0) {
	ExpStatus->csb.gpaTuning[index] = (short)value;
	shrmTake(ExpStatusObj);
	gettimeofday( &(ExpStatus->TimeStamp), NULL);
	shrmGive(ExpStatusObj);
	return 0;
    }
    return -1;
}

int getStatAcqSample()
{
   return((int)ExpStatus->csb.AcqSample);
}
int setStatAcqSample(int sample )
{
   ExpStatus->csb.AcqSample = sample;
   shrmTake(ExpStatusObj);
   gettimeofday( &(ExpStatus->TimeStamp), NULL);
   shrmGive(ExpStatusObj);
   return(0);
}

int getStatRecvGain()
{
   return((int)ExpStatus->csb.AcqRecvGain);
}
int setStatRecvGain(int gain)
{
   ExpStatus->csb.AcqRecvGain = (short) gain;
   shrmTake(ExpStatusObj);
   gettimeofday( &(ExpStatus->TimeStamp), NULL);
   shrmGive(ExpStatusObj);
   return(0);
}

int getStatSpinAct()
{
   return((int)ExpStatus->csb.AcqSpinAct);
}

int getStatSpinSet()
{
   return((int)ExpStatus->csb.AcqSpinSet);
}

int setStatSpinAct(int spin_val)
{
   ExpStatus->csb.AcqSpinAct = (short) spin_val;
   shrmTake(ExpStatusObj);
   gettimeofday( &(ExpStatus->TimeStamp), NULL);
   shrmGive(ExpStatusObj);
   return(0);
}

int getStatShimValue(int dacnum)
{
   return((int)ExpStatus->csb.AcqShimValues[dacnum]);
}
int setStatShimValue(int dacnum, int value)
{
   ExpStatus->csb.AcqShimValues[dacnum] = (short) value;
   shrmTake(ExpStatusObj);
   gettimeofday( &(ExpStatus->TimeStamp), NULL);
   shrmGive(ExpStatusObj);
   return(0);
}

int getStatShimSet()
{
   return((int)ExpStatus->csb.AcqShimSet);
}

int getStatLSDV()
{
   return( (int) ExpStatus->csb.AcqLSDVbits );
}

int getStatVTAct()
{
   return( (int) ExpStatus->csb.AcqVTAct );
}

int getStatVTSet()
{
   return( (int) ExpStatus->csb.AcqVTSet );
}

int getStatVTC()
{
   return( (int) ExpStatus->csb.AcqVTC );
}

int getStatAcqTickCountError()
{
   return( (int) ExpStatus->csb.AcqTickCountError );
}

int getStatAcqChannelBitsConfig1()
{
   return( (int) ExpStatus->csb.AcqChannelBitsConfig1 );
}

int getStatAcqChannelBitsConfig2()
{
   return( (int) ExpStatus->csb.AcqChannelBitsConfig2 );
}

int getStatAcqChannelBitsActive1()
{
   return( (int) ExpStatus->csb.AcqChannelBitsActive1 );
}

int getStatAcqChannelBitsActive2()
{
   return( (int) ExpStatus->csb.AcqChannelBitsActive2 );
}

int getStatPneuBearing()
{
   return( (int) ExpStatus->csb.AcqPneuBearing );
}

int getStatPneuVtAir()
{
   return( (int) ExpStatus->csb.AcqPneuVtAir );
}

int getStatPneuVTAirLimits()
{
   return( (int) ExpStatus->csb.AcqPneuVTAirLimits );
}

int getStatPneuSpinner()
{
   return( (int) ExpStatus->csb.AcqPneuSpinner );
}

int getStatConsoleID()
{
   return( (int) ExpStatus->csb.consoleID );
}

long getStatLockFreqAP()
{
   return( ExpStatus->csb.AcqLockFreqAP );
}

int getStatLockFreq1()
{
   return( ExpStatus->csb.AcqLockFreq1 );
}

int getStatLockFreq2()
{
   return( ExpStatus->csb.AcqLockFreq2 );
}

static int getstatstr(char *retstr, char *arg, int maxlen)
{
  if (maxlen >= EXPSTAT_STR_SIZE)
  {
    shrmTake(ExpStatusObj);
     strcpy(retstr,arg);
    shrmGive(ExpStatusObj);
  }
  else
  {
    shrmTake(ExpStatusObj);
     strncpy(retstr,arg,maxlen);
     retstr[maxlen-1] = '\0';
    shrmGive(ExpStatusObj);
  }
  return(0);
}
static int setstatstr(char *arg, char *given)
{
  int len;
  len = strlen(given);

  shrmTake(ExpStatusObj);
  if (len > EXPSTAT_STR_SIZE)
  {
     strncpy(arg,given,EXPSTAT_STR_SIZE);
     arg[EXPSTAT_STR_SIZE-1] = '\0';
  }
  else
  {
     strcpy(arg,given);
  }
  gettimeofday( &(ExpStatus->TimeStamp), NULL);
  shrmGive(ExpStatusObj);
  return(0);
}

int getStatUserId(char *usrname,int maxsize)
{
  return(getstatstr(usrname,ExpStatus->UserID,maxsize));
}
int setStatUserId(char *usrname)
{
  return(setstatstr(ExpStatus->UserID,usrname));
}

int getStatExpName(char *expname,int maxsize)
{
  return(getstatstr(expname,ExpStatus->ExpID,maxsize));
}
int setStatExpName(char *expname)
{
  return(setstatstr(ExpStatus->ExpID,expname));
}

int getStatExpId(char *expidstr,int maxsize)
{
  return(getstatstr(expidstr,ExpStatus->ExpIdStr,maxsize));
}
int setStatExpId(char *expidstr)
{
  return(setstatstr(ExpStatus->ExpIdStr,expidstr));
}

int getStatProcUserId(char *usrname,int maxsize)
{
  return(getstatstr(usrname,ExpStatus->ProcUserID,maxsize));
}
int setStatProcUserId(char *usrname)
{
  return(setstatstr(ExpStatus->ProcUserID,usrname));
}

int getStatProcExpName(char *expname,int maxsize)
{
  return(getstatstr(expname,ExpStatus->ProcExpID,maxsize));
}
int setStatProcExpName(char *expname)
{
  return(setstatstr(ExpStatus->ProcExpID,expname));
}

int getStatProcExpId(char *expidstr,int maxsize)
{
  return(getstatstr(expidstr,ExpStatus->ProcExpIdStr,maxsize));
}
int setStatProcExpId(char *expidstr)
{
  return(setstatstr(ExpStatus->ProcExpIdStr,expidstr));
}

int
getStatOpsCompl()
{
  return( (int) ExpStatus->csb.AcqOpsComplCnt );
}

int
getStatOpsCmpltFlags()
{
  return( (int) ExpStatus->csb.AcqOpsComplFlags );
}

int
getStatNpErr()
{
   return((int)ExpStatus->csb.AcqNpErr);
}

int
getStatRcvrNpErr()
{
   return((int)ExpStatus->csb.AcqRcvrNpErr);
}

void
getStatTimeStamp(struct timeval *time)
{
   shrmTake(ExpStatusObj);
   memcpy((char*) time,(char*) &(ExpStatus->TimeStamp),sizeof(struct timeval));
   shrmGive(ExpStatusObj);
  return;
}

int
isStatTimeStampNew(struct timeval *time)
{
  int stat;

  stat = 0;
  shrmTake(ExpStatusObj);
  if (ExpStatus->TimeStamp.tv_sec > time->tv_sec)
     stat = 1;
  else if (ExpStatus->TimeStamp.tv_usec > time->tv_usec)
     stat = 1;

  shrmGive(ExpStatusObj);
  return(stat);
}

time_t getTime()
{
    return(time(NULL));
}

char *getTimeStr(time_t time, char *timestr,int maxsize)
{
   struct tm *tmtime;

   /* -------- POSIX, ANSI way of getting time of day */
   if (time > 0)
   {
      tmtime = localtime(&time);
      strftime(timestr,maxsize,"%H:%M:%S",tmtime);
   }
   else
   {
     strcpy(timestr,"");
   }
   return(timestr);
}

char *getTimeDif(time_t time, char *timestr)
{
  int hrs,mins,sec;

  hrs = (int) (time / 3600L);
  mins = (int) ( (time % 3600L) / 60L );
  sec = (int) (time % 60L);
  sprintf(timestr,"%02d:%02d:%02d",hrs,mins,sec);
  return(timestr);
}

/* how it was done, probably won't keep these particular states */
void setStatState(int state)
{
    char message[80];

    switch(state)
    {
/*
       case ACQ_IDLE:
                        strcpy(message,"Idle");
                        break;
        case ACQ_ACQUIRE:
                        strcpy(message,"Acquiring");
                        break;
        case ACQ_VTWAIT:
                        strcpy(message,"VT Regulation");
                        break;
        case ACQ_SPINWAIT:
                        strcpy(message,"Spin Regulation");
                        break;
        case ACQ_AGAIN:
                        strcpy(message,"Auto Set Gain");
                        break;
        case ACQ_ALOCK:
                        strcpy(message,"Auto Locking");
                        break;
        case ACQ_AFINDRES:
                        strcpy(message,"Lock: Find Res.");
                        break;
        case ACQ_APOWER:
                        strcpy(message,"Lock: Adj. Power");
                        break;
        case ACQ_APHASE:
                        strcpy(message,"Lock: Adj. Phase");
                        break;
        case ACQ_SHIMMING:
                        strcpy(message,"Shimming");
                        break;
        case ACQ_SMPCHANGE:
                        strcpy(message,"Changing Sample");
                        break;
        case ACQ_RETRIEVSMP:
                        strcpy(message,"Retrieving Sample");
                        break;
        case ACQ_LOADSMP:
                        strcpy(message,"Loading Sample");
                        break;
        case ACQ_INTERACTIVE:
                        strcpy(message,"Interactive");
                        break;
        case ACQ_TUNING:
                        strcpy(message,"Tuning");
                        break;
  */
        default:
               /* strcpy(message, (acq_ok) ? "Active" : "Inactive"); */
                        strcpy(message, "Inactive");
                        break;
    }                    
    shrmTake(ExpStatusObj);
    strcpy(ExpStatus->ReportAcqState,message);
    shrmGive(ExpStatusObj);
}

int
receiveConsoleStatusBlock( char *pcsb )
{
    int changed;

 /* 'changed' should be zero if the statblock has not changed from previous ones */

    changed = memcmp( (char *) &ExpStatus->csb, pcsb, sizeof( ExpStatus->csb ) );
    memcpy( (char *) &ExpStatus->csb, pcsb, sizeof( ExpStatus->csb ) );
    gettimeofday( &(ExpStatus->TimeStamp), NULL);
    return(changed);
}

int
writeConsolseStatusBlock( char *filename )
{
    int fp, wval;

    fp = open( filename, O_CREAT | O_TRUNC| O_RDWR, 0666 );
    if (fp < 0) {
      return( -1 );
    }
    wval = write( fp, (char *) &ExpStatus->csb, sizeof( ExpStatus->csb ));
    close( fp );
    if (wval != sizeof( ExpStatus->csb ))
      return( -1 );
    return( 0 );
}

int
compareShimsConsoleStatusBlock( CONSOLE_STATUS *pcsb )
{
    int changed;

    changed = memcmp(
		 (char *)&ExpStatus->csb.AcqShimValues[ 0 ],
		 (char *)&pcsb->AcqShimValues[ 0 ],
		  sizeof( short ) * MAX_SHIMS_CONFIGURED );
    return( changed );
}

/* DPRINT and DPRINT1 are defined in errLogLib.h but with a different number
   of args.  Redefine here for lockit and unlockit */
#ifdef TESTING
#undef DPRINT
#define DPRINT(str)      fprintf(stderr,str)
#undef DPRINT1
#define DPRINT1(str,arg) fprintf(stderr,str,arg)
#else
#undef DPRINT
#define DPRINT(str)
#undef DPRINT1
#define DPRINT1(str,arg)
#endif

#ifndef MAXPATH
#define MAXPATH 256
#endif

/*
 * file locking is a two step process.
 * 1. Make an ID file that can be associated with a process. The file name
 *    is idPath. This idPath filename and a timeout value are written into
 *    the ID file.
 * 2. Link the lock file to this ID file. The link step is atomic and will fail
 *    in the lock file already exists. We confirm the lock by making sure the
 *    ID file has 2 hard links.
 * Note that locking may wait until the lock expires or the lock was removed.
 * The unlock action does not test who owns the lock. It just unlocks it.
 */

/* unlockit always succeeds */
void unlockit(const char *lockPath, const char *idPath)
{
   char lockid[MAXPATH];
   time_t locktime;
   int    ret __attribute__((unused));

   if ( ! access(lockPath,R_OK | W_OK) )
   {
      FILE *fd;
      fd = fopen(lockPath, "r");
      if (fd != NULL)
      {
         ret = fscanf(fd,"%s %ld\n", lockid, &locktime);
         fclose(fd);
         unlink(lockid);
      }
      unlink(lockPath);
   }
   if (idPath != NULL)
      unlink(idPath);
}

/* returns 1 on success; 0 on failure */
int lockit(const char *lockPath, const char *idPath, time_t timeout)
{
   char lockid[MAXPATH];
   struct stat buf;
   FILE *fd;
   int cnt;
   time_t expire = -1;
   time_t locktime; 
   struct timeval tv;
   struct timespec req;

   gettimeofday( &tv, NULL);
   timeout += tv.tv_sec;

   if ( ! access(idPath,R_OK | W_OK) )
   {
      /* if id file already exists. See if it is linked to lock file.
       * If it is, we are done.
       */
      if ( ! access(lockPath,R_OK | W_OK) )
      {
         /* Both files exist. Check link count */
         stat(idPath, &buf);
         if (buf.st_nlink == 2) /* lock file is in place */
            return(1);
      }
   }
   else
   {
      mode_t oldmask;
      /* make id file */
      oldmask = umask(0);
      fd = fopen(idPath, "w");
      umask(oldmask);
      if (fd != NULL)
      {
         fprintf(fd,"%s %ld\n", idPath, timeout);
         fclose(fd);
      }
      else
      {
        return(0);
      }
   }
   cnt = 0;
   while (cnt < 60)  /* 30 secs */
   {
      int res;

      if ( (res = link(idPath,lockPath)) == 0)
      {
         /* file locked */
         return(1);
      }
      else
      {
         /* sometimes link() reports failure even if it worked.
          * check link count on id file
          */
         stat(idPath, &buf);
         if (buf.st_nlink == 2) /* lock file is in place */
            return(1);
         
         /* check for expiration time on lockfile */
         DPRINT("check expire time\n");
         if (expire == -1)
         {
            expire = 0;
            fd = fopen(lockPath, "r");
         DPRINT1("open %s for expire time\n", lockPath);
            if (fd != NULL)
            {
               int ret __attribute__((unused));
               ret = fscanf(fd,"%s %ld\n", lockid, &locktime);
               fclose(fd);
               expire = locktime;
         DPRINT1("got expire time %ld\n", expire);
            }
         }
      }
      if (expire > 0)
      {
         gettimeofday( &tv, NULL);
         if (expire < tv.tv_sec)
         {
            DPRINT("lock time expired\n");
            unlink(lockPath);
            unlink(lockid);
            continue;
         }
         DPRINT1("remaining time is %ld\n",expire - tv.tv_sec);
#ifdef __INTERIX
         usleep(999999);
#else
         req.tv_sec = 1;
         req.tv_nsec = 0;
         nanosleep( &req, NULL);
#endif
         continue;
      }
      cnt++;
#ifdef __INTERIX
      usleep(500000);
#else
      req.tv_sec = 0;
      req.tv_nsec = 500000000; /* .5 secs */
      nanosleep( &req, NULL);
#endif
   }
   return(0);
}
