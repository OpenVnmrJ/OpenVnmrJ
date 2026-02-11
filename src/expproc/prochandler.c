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
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include "errLogLib.h"
#include "prochandler.h"

#define MAXPATHL 128

/* Used by locksys.c  routines from vnmr */
char systemdir[MAXPATHL];       /* vnmr system directory */
 
static pid_t child;

/*
*  Initialized data base for all process that msgQs
*  that this process may wish to talk to.
*  the record define can be found in hostMsgChannels.h
*  and their order is important.
*/
static TASK_RECORD taskList[PROC_RECORD_SIZE] = {
                EXP_RECORD,
                RECV_RECORD,
                SEND_RECORD,
                PROC_RECORD,
                INFO_RECORD,
                ROBO_RECORD,
                AUTO_RECORD,
                AT_RECORD,
                MAS_RECORD,
                SPUL_RECORD,
                NULL_RECORD,
                NULL_RECORD
           };

int chkAccess(char* filename);
void killEmDead(int first2Die,int last2Die);

pid_t startTask(char *taskname, char *filepath)
{
    int ret __attribute__((unused));
    char testpath[512];
    int setmask;
    sigset_t		omask, qmask;

    setmask = (strcmp(taskname,taskList[INFOPROC].procName) == 0);
    strcpy(testpath,systemdir);
    strcat(testpath,filepath);
    if ( access(testpath,R_OK | X_OK) != 0)
    {
        return(-1);   /* don't do any more */
    }
    child = fork();
    if (child != 0)     
    {
	    return(child);
    }
    else
    {
        if (setmask)
        {
	   sigemptyset( &qmask );
	   sigaddset( &qmask, SIGUSR2 );
	   sigprocmask( SIG_BLOCK, &qmask, &omask );
        }
        ret = execl(testpath, taskname, NULL);
        errLogSysQuit(ErrLogOp,debugInfo,"Task: '%s' execl failed..", taskname);
    }
    return(0);
}

int startAutoproc(char *autodir, char *autoDoneQ)  /* if not started then start it */
{
   char testpath[512];
   int ret __attribute__((unused));

   DPRINT1(1,"startATask: Test Access for: %s\n", taskList[AUTOPROC].procPath);
   if (taskList[AUTOPROC].taskPid != -1)
     return(0);
   if (chkAccess(taskList[AUTOPROC].procPath) == -1)
   {
      errLogSysRet(ErrLogOp,debugInfo,"Can find '%s', Correct then restart.",
      taskList[AUTOPROC].procName);
      return(-1);
   }

   DPRINT1(1,"startAutoproc - Start: %s\n", taskList[AUTOPROC].procName);
    strcpy(testpath,systemdir);
    strcat(testpath,taskList[AUTOPROC].procPath);
    child = fork();
    if (child != 0)     
    {
       taskList[AUTOPROC].taskPid = child;
       return(1);
    }
    else
    {
        ret = execl(testpath, taskList[AUTOPROC].procName, autodir,autoDoneQ,NULL);
        errLogSysQuit(ErrLogOp,debugInfo,"Task: '%s' execl failed..", 
		taskList[AUTOPROC].procName);
    }
    return(0);
}

static void startTasks()
{
   int i,alive;
   pid_t pid;
   char *tmpptr;

   /* initialize environment parameter vnmrsystem value */
   tmpptr = getenv("vnmrsystem");            /* vnmrsystem */
   if (tmpptr != (char *) 0)
   {
       strcpy(systemdir,tmpptr);      /* copy value into global */
   }
   else
   {   
       strcpy(systemdir,"/vnmr");     /* use /vnmr as default value */
   }

   taskList[EXPPROC].taskPid = getpid();
   /* for( i = RECVPROC; i <= SPULPROC; i++) */
   for( i = RECVPROC; i < PROC_RECORD_SIZE; i++)   /* just for early testing for now */
   {
      if (taskList[i].autostart == 1)
      {
         DPRINT1(1,"Test Access for: %s\n", taskList[i].procPath);
         if (chkAccess(taskList[i].procPath) == -1)
         {
            if (i == MASPROC)  /* Ignore Masproc if it is missing */
               taskList[i].autostart = 0;
            else
	       errLogSysQuit(ErrLogOp,debugInfo,
                 "Can find '%s', Correct then restart.", taskList[i].procName);
         }
      }
   }
   for( i = RECVPROC; i < PROC_RECORD_SIZE; i++) 
   {
      if (taskList[i].autostart == 1)
      {
       DPRINT1(1,"startTasks - Start: %s\n", taskList[i].procName);
       if (taskList[i].taskPid != -1)
       {
	   alive = kill(taskList[i].taskPid,0); /* are you out there */
       }
       else
	   alive = -1;

       /* don't start them if they are already running */
       if (alive != 0)
       {
         if ((pid = startTask(taskList[i]. procName, taskList[i].procPath)) == -1)
         {
	    errLogSysQuit(ErrLogOp,debugInfo,"Terminating!, Couldn't exec: '%s'",
		taskList[i].procPath);
         }
         taskList[i].taskPid = pid;
       }
      }
   }
}

void initBaseTasks()
{
   startTasks();
}

int startATask(int task)
{
   pid_t pid;

   DPRINT1(1,"startATask: Test Access for: %s\n", taskList[task].procPath);
   if (taskList[task].taskPid == -1)
   {
     if (chkAccess(taskList[task].procPath) == -1)
     {
	 errLogSysRet(ErrLogOp,debugInfo,"Can find '%s', Correct then restart.",
	     taskList[task]. procName);
	 return(-1);
     }
     DPRINT1(1,"startATask - Start: %s\n", taskList[task].procName);
     if ((pid = startTask(taskList[task]. procName, taskList[task].procPath)) == -1)
     {
	 errLogSysRet(ErrLogOp,debugInfo,"Terminating!, Couldn't exec: '%s'",
		taskList[task].procPath);
         return(-1);
     }
     taskList[task].taskPid = pid;
     return(1);
   }
   else
     return(0);
}

void restartTasks()
{
   startTasks();
}

void killATask(int task)
{
    killEmDead(task,task);
}

/* this is used when connection with host lost */
void killTasks()
{
   killEmDead(RECVPROC,PROCPROC);
}

/* this is used when Expproc is to terminate */
void killEmAll()
{
   killEmDead(RECVPROC,SPULPROC);
}

char *registerDeath(pid_t pid)
{
   int i;
   for( i = EXPPROC; i <= SPULPROC; i++) 
   {
       DPRINT2(1,"registerDeath:  died: %d, who: %d\n",
		  pid,taskList[i].taskPid);
       /* Autoproc death is handled in msgehandler.c by a request to die to avoid race 
	  conditions with the SIGCHLD signal */
       if ( (taskList[i].taskPid == pid) && ( i != AUTOPROC ) )
       {
	 taskList[i].taskPid = (pid_t) -1;
	 DPRINT2(1,"Process: '%s' pid: %d, Died.\n",taskList[i].procName,pid);
	 return(taskList[i].procName);
       }
   }
   return(NULL);
}

char *markProcDead(int index)
{
   taskList[index].taskPid = (pid_t) -1;
   return(taskList[index].procName);
}

void killEmDead(int first2Die,int last2Die)
{
   int i;
   struct timeval tv;
   for( i = first2Die; i <= last2Die; i++) 
   {
       DPRINT2(1,"SIGTERM kill: %s pid: %d\n",
		  taskList[i].procName,taskList[i].taskPid);
       if ( taskList[i].taskPid != (pid_t) -1 )
       {
         kill(taskList[i].taskPid,SIGTERM);
       }
   }
   /* wait for a little while. can not sleep() in here. Apparently. kill(pid,0) does not work
    * until the process is reaped.
    */
   tv.tv_sec = 2;
   tv.tv_usec = 0;
   select(0, NULL, NULL, NULL, &tv);
   /* let's make sure, if SIGTERM was not good enough we'll use something stronger SIGKILL */
   for( i = first2Die; i <= last2Die; i++) 
   {
       if (taskList[i].taskPid != (pid_t) -1)
       {
           kill(taskList[i].taskPid,SIGKILL);
  	   taskList[i].taskPid = (pid_t) -1;
       }
   }
}

void abortSendprocXfer()
{
   if (taskList[SENDPROC].taskPid != -1)
   {
      /* abort sendproc transfer */
      if (kill(taskList[SENDPROC].taskPid,SIGUSR2) == 0)
      {
    	DPRINT(1,"kill of Transfer suceeded.\n");
      }
      else
        errLogSysRet(ErrLogOp,debugInfo,"Aborting Sendproc (%d) Transfer Failed, ",
                taskList[SENDPROC].taskPid);
   }
}

void abortSampChange()
{
   if (taskList[ROBOPROC].taskPid != -1)
   {
      /* abort sendproc transfer */
      if (kill(taskList[ROBOPROC].taskPid,SIGUSR2) == 0)
      {
    	DPRINT(1,"kill of Sample Changer Suceeded.\n");
      }
      else
        errLogSysRet(ErrLogOp,debugInfo,"Aborting Sample Changer (%d) Failed, ",
                taskList[ROBOPROC].taskPid);
   }
}


void sigInfoproc()
{
   if (taskList[INFOPROC].taskPid != -1)
   {
      /* abort sendproc transfer */
      if (kill(taskList[INFOPROC].taskPid,SIGUSR2) == 0)
      {
    	DPRINT(2,"Signal to Inforproc suceeded.\n");
      }
   }
}

int chkAccess(char* filename)
{
    char testpath[512];

    strcpy(testpath,systemdir);
    strcat(testpath,filename);
    if ( access(testpath,R_OK | X_OK) != 0)
    {
       return(-1);
    }
    return(0);
}

int chkTaskActive(int task)
{
   return( (taskList[task].taskPid != -1) );
}

