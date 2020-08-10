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
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/time.h>



#include "errLogLib.h"

extern char systemdir[];
extern char atCmdPath[];
extern void lockAtcmd(const char *dir);
extern void unlockAtcmd(const char *dir);

extern void shutdownComm();
int ShutDownProc();
int doAtcmd();

static int VnmrActive = 0;

#ifndef WCOREDUMP
#define  WCOREDUMP( statval )   ((( (statval) & 0x80) != 0) ? 1 : 0)
#endif

static void vnmrExitHandler(int sig)
{
    int coredump;
    int pid;
    int status;
    int termsig;
    int kidstatus;

    DPRINT(1,"|||||||||||||||||||  SIGCHLD   ||||||||||||||||||||||||\n");
    DPRINT(1,"vnmrExitHandler(): At Work !!!\n");

    /* while ((pid = wait3(&kidstatus,WNOHANG,NULL)) > 0) */
    /* pid = wait(&kidstatus); */
    while ((pid = waitpid( -1, &kidstatus, WNOHANG | WUNTRACED )) > 0)
    {
        if ( WIFSTOPPED(kidstatus) ) /* Is this an exiting or stopped Process */
           continue;             /* If a STOPPED Process go to next waitpid() */
        if (WIFEXITED( kidstatus ) != 0)
          status = WEXITSTATUS( kidstatus );
        else
          status = 0;
        if (WIFSIGNALED( kidstatus ) != 0)
          termsig = WTERMSIG( kidstatus );
        else
          termsig = 0;
        coredump = WCOREDUMP( kidstatus );

        /*status = kidstatus.w_T.w_Retcode;
        coredump = kidstatus.w_T.w_Coredump;
        termsig = kidstatus.w_T.w_Termsig;*/

        DPRINT3(1,"Child Status: %d, Core Dumped: %d, Termsig: %d\n",
            status,coredump,termsig);
        DPRINT1(1,"vnmrExitHandler: pid = %d \n",pid);

        DPRINT(1,"|||||||||||||||||  SIGCHLD  Completed |||||||||||||||||\n");
   }
   VnmrActive = 0;
   if (doAtcmd() == -1)
      ShutDownProc();
   return;
}

void atcmd2(int sig)
{
   if (VnmrActive)
      return;
   if (doAtcmd() == -1)
      ShutDownProc();
}

static void startVnmrExitHandler()
{
    struct sigaction    intserv;
    sigset_t            qmask;

    /* --- set up signal handler --- */
    sigemptyset( &qmask );
    sigaddset( &qmask, SIGIO );
    sigaddset( &qmask, SIGALRM );
    intserv.sa_handler = vnmrExitHandler;
    intserv.sa_mask    = qmask;
    intserv.sa_flags   = SA_RESTART;
    sigaction(SIGCHLD,&intserv,0L);
    sigemptyset( &qmask );
    sigaddset( &qmask, SIGIO );
    intserv.sa_handler = atcmd2;
    intserv.sa_mask    = qmask;
    intserv.sa_flags   = SA_RESTART;
    sigaction(SIGALRM,&intserv,0L);
}

static int runVnmr(int uid, int gid, int umask4Vnmr, char *host, char *userdir)
{
    char cmdstring[128];
    char vnmrmode[20];
    char expnum[16];
    char userpath[1024+32];
    char hostname[1024+32];
    char Vnmrpath[256];
    FILE   *ret2 __attribute__((unused));
    int    ret __attribute__((unused));
    pid_t childpid;

    /* Be Absolutely sure there is a Vnmr out there to RUN */
    strcpy(Vnmrpath,systemdir);
#ifdef LINUX
    strcat(Vnmrpath,"/bin/Vnmrbg");
#else
    strcat(Vnmrpath,"/bin/Vnmr");
#endif
    if ( access(Vnmrpath,R_OK | X_OK) != 0)
    {
       errLogRet(ErrLogOp,debugInfo, "'%s' not found or can not run.\n",
                   Vnmrpath);
       return(0);   /* don't do any more */
    }
    startVnmrExitHandler();
    sprintf(cmdstring,"atcmd\n");

    sprintf(userpath,"-u%s",userdir);
    sprintf(hostname,"-h%s",host);
    sprintf(vnmrmode,"-macq");
    sprintf(expnum,"-n0");
 
    /* save User is a static location for forked child */
    childpid = fork();  /* fork a child */
    DPRINT(1,"Start BG process ---- \n");
    /* -----  Parent vs Child ---- */
    if (childpid == 0)
    {
       sigset_t signalmask;

       /* set signal mask for child ti zip */
       sigemptyset( &signalmask );
       sigprocmask( SIG_SETMASK, &signalmask, NULL );

       /* change the user and group ID of the child so that VNMR
          will run with those ID's */
       ret = seteuid(getuid());
       ret2 = freopen("/dev/null","r",stdin);
       ret2 = freopen("/dev/console","a",stdout);
       ret2 = freopen("/dev/console","a",stderr);
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
              
       ret=execl(Vnmrpath,"Vnmr",vnmrmode,hostname,expnum,userpath,cmdstring,NULL);
       if (ret == -1)
       {
          DPRINT1(1, "execl on '%s' failed.\n", Vnmrpath);
          return(0);   /* don't do any more */
       }
    }
    VnmrActive = 1;
    return(0);
}


/*
  time  user 
static int runVnmr(int uid, int gid, int umask, char *host, char *userdir)

  time user host userdir umask timespec command
  time uid gid umask host userdir user timespec command

 */

int doAtcmd()
{
   int res;
   FILE *fd;
   int  doVnmr = 0;
   long atTime;
   int  uid;
   int  gid;
   int  umask4Vnmr;
   char host[1024];
   char userdir[1024];
   char dummy[1024];
   struct timeval clock;
   long minTime;

   gettimeofday(&clock, NULL); 

   res = access(atCmdPath, R_OK);
   DPRINT2(1,"atcmd called file %s access= %d\n", atCmdPath, res);
   if (res)
     return(-1);
   lockAtcmd(systemdir);
   fd = fopen(atCmdPath,"r");
   if (fd == NULL)
   {
     unlockAtcmd(systemdir);
     return(-1);
   }

   
   DPRINT1(1,"current time %ld\n", clock.tv_sec);
   minTime = 0;
   while ( (res = fscanf(fd,"%ld %d %d %d %s %s %[^\n]\n",
                      &atTime, &uid, &gid, &umask4Vnmr, host, userdir, dummy) ) == 7)
   {
      DPRINT7(1,"atcmd: time %ld uid %d gid %d umask %d host %s userdir %s dummy %s\n",
                      atTime, uid, gid, umask4Vnmr, host, userdir, dummy);
      if (atTime <= clock.tv_sec)
      {
         doVnmr = 1;
         break;
      }
      if (minTime == 0)
        minTime = atTime;
      if (atTime < minTime)
        minTime = atTime;
   }
   fclose(fd);
   unlockAtcmd(systemdir);

   if (doVnmr)
   {
      DPRINT7(1,"atcmd: time %ld uid %d gid %d umask %d host %s userdir %s dummy %s\n",
                      atTime, uid, gid, umask4Vnmr, host, userdir, dummy);
      runVnmr(uid, gid, umask4Vnmr, host, userdir);
   }
   else if (minTime > clock.tv_sec)
   {
      DPRINT2(1,"current time %ld minTime= %ld\n", clock.tv_sec, minTime);
      DPRINT1(1,"set alarm for %d seconds\n", (int) (minTime - clock.tv_sec));
      startVnmrExitHandler();
      alarm((int) (minTime - clock.tv_sec));

   }
   else
   {
      return(-1);
   }
   return(0);
}

int terminate(char *str)
{
    shutdownComm();
    exit(0);
    return 0;
}

int ShutDownProc()
{
    shutdownComm();
    exit(0);
    return 0;
}

int atcmd(char *str)
{
   if (VnmrActive)
      return(0);
   if (doAtcmd() == -1)
      ShutDownProc();
   return(0);
}

int debugLevel(char *str)
{
    char *value;
    int  val;
    extern int DebugLevel;

    value = strtok(NULL," ");
    val = atoi(value);

    DebugLevel = val;
    DPRINT1(1,"debugLevel: New DebugLevel: %d\n",val);

    return(0);
}
