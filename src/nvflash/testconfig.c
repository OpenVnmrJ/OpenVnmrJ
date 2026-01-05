/* 
 * Varian,Inc. All Rights Reserved.
 * This software contains proprietary and confidential
 * information of Varian, Inc. and its contributors.
 * Use, disclosure and reproduction is prohibited without
 * prior consent.
 */

/*
*  Author: Greg Brissey   11/06/2007
*/

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include "errLogLib.h"
// #include "ndds/ndds_c.h"
#include "NDDS_Obj.h"
#ifdef RTI_NDDS_4x
#include "Console_Conf.h"
#endif

#include "Monitor_Cmd.h"

extern void initiatePubSub();
extern NDDS_ID initiateNDDS(int);
extern int wait4Master2Connect(int timeout);
extern int send2Monitor(int, int, int,int, char *, int);
extern void MonitorReply(int *, int *, int *, int *, char *);
extern void DestroyDomain();
extern int initConfSub();

#define  NUMOFOPTIONS 3
static char *options[] =
     {"-debug", "-d", "-querynddsver" };

char ProcName[80];
int multicast = 1;  /* enable mulsticasting for NDDS */
int ndds_debuglevel = 1;

char *ConsoleHostName = "wormhole";

char *ConsoleNicHostname = "wormhole";

static int gotIssue = 0;

static int debuglevel = 0;	
static int nddsQueryFlag = 0;

// call by NDDS callback when Conf Issue recieved
#ifdef RTI_NDDS_4x
void processConfIssue(Console_Conf *recvIssue)
{
   if (debuglevel > 0)
   {
     printf("VxWorks Version: '%s'\n",recvIssue->VxWorksVersion);
     printf("RTI NDDS Version: '%s'\n",recvIssue->RtiNddsVersion);
     printf("PSG/Interpreter Version: '%s'\n",recvIssue->PsgInterpVersion);
     printf("Console Type: %d\n", recvIssue->ConsoleTypeFlag);
     printf("System Rev Id: %d\n", recvIssue->SystemRevId);
     printf("FPGA Loaded With: '%s'\n", recvIssue->fpgaLoadStr);
     printf("Compile Data Time: '%s'\n",recvIssue->CompileDate);
     printf("MD5 Signitures for:\n");
     printf("         ddr: '%s'\n",recvIssue->ddrmd5);
     printf("    gradient: '%s'\n",recvIssue->gradientmd5);
     printf("        lock: '%s'\n",recvIssue->lockmd5);
     printf("      master: '%s'\n",recvIssue->mastermd5);
     printf("       nvlib: '%s'\n",recvIssue->nvlibmd5);
     printf("    nvScript: '%s'\n",recvIssue->nvScriptmd5);
     printf("         pfg: '%s'\n",recvIssue->pfgmd5);
     printf("        lpfg: '%s'\n",recvIssue->lpfgmd5);
     printf("          rf: '%s'\n",recvIssue->rfmd5);
     printf("     vxWorks: '%s'\n",recvIssue->vxWorksKernelmd5);
   }
   gotIssue = gotIssue + 1;
}
#endif

static FILE *histFd;

static void
exit_close(int result)
{
   if (histFd != NULL)
      fclose(histFd);
   exit(result);
}

static void
printInfo(char *format, ...)
{
     va_list   vargs;

     if (debuglevel > 0)
     {
        va_start(vargs,format);
        vfprintf(stdout, format, vargs);
        va_end(vargs);
     }
     if (histFd != NULL)
     {
        va_start(vargs,format);
        vfprintf(histFd, format, vargs);
        va_end(vargs);
     }
}

static void setupTimeout(double);
static int setRtimer(double timsec,double interval);

int main(int argc, char **argv)
{
    int countdown,i;
    int cmd;
    int replycmd,replyarg1,replyarg2,replyarg3;
    char replymsge[512];
    int result __attribute__((unused));
    char sysDir[512],objDir[512+64];
    char *res;


    DebugLevel=-5;

    // check of options within passed arguments
    for (i = 1; i < argc; i++) 
    {
        int scount;
	     for (scount = 0; scount < NUMOFOPTIONS; scount++) 
        {
           /* printf("argv[%d]: '%s', options[%d]: '%s'\n",i, argv[i], scount, options[scount]); */
	        if (strcmp(argv[i], options[scount]) == 0) 
           {
		        break;
	        }
        }
        if ( (scount == 0) || (scount == 1) )
        {
	    	  debuglevel = 1;	
           // DebugLevel=5;    /* uncomment to obtain NDDS diagnostics as well */
           printf("debug level = %d\n",debuglevel);
        }
        if ( (scount == 2) )
        {
           nddsQueryFlag = 1;
        }
	}
	if ((debuglevel == 1) && (nddsQueryFlag == 1))
       printf("Use direct Query of NDDS Version\n");	

    strcpy(ProcName,"testconfig");
    initiateNDDS(0);   /* NDDS_Domain is set */
    sleep(5);

    if (getenv("vnmrsystem") == NULL)
       strcpy(sysDir, "/vnmr");
    else
       sprintf(sysDir, "%s", getenv("vnmrsystem"));
#ifdef RTI_NDDS_4x
          sprintf(objDir, "%s/acq/download/loadhistory", sysDir);
#else
          sprintf(objDir, "%s/acq/download3x/loadhistory", sysDir);
#endif
     histFd = fopen(objDir,"a+");
     if (histFd == NULL)
        printf("Trouble opening %s\n",objDir);

#ifdef RTI_NDDS_4x
    if (nddsQueryFlag == 0)
    {
	    if (debuglevel == 1)
         printf("subscribe to configuration publication\n");

       initConfSub();

       countdown = 30;
       while(1)
       {
         // printf("Issues received: %d, countdown: %d\n", gotIssue,countdown);
         sleep(1);
         if ( (gotIssue > 0) || (countdown <= 0))
           break;
         countdown--;
       }
       if (debuglevel == 1)
          printf("Is NDDS 4x: '%s'\n",(gotIssue == 0) ? "FALSE" : "TRUE");

       DestroyDomain();  // make sure to notify console this pub is going away

       if (gotIssue == 0)
         exit_close(EXIT_FAILURE);
       else
         exit_close(EXIT_SUCCESS);
    }
    else
    {
#endif
	    if (debuglevel == 1)
         printf("Query master for NDDS Version\n");

       initiatePubSub();  /* monitor command pub/sub */

       if ( wait4Master2Connect(20) == -1)  /* wait for 10 sec for Master to connect */
       {
          printInfo("Master did not connect to publication, continuing...\n");
          DestroyDomain();  // make sure to notify console this pub is going away
          if (debuglevel == 1)
#ifdef RTI_NDDS_4x
             printf("Assumption: NDDS is NOT 4x\n");
#else
             printf("Assumption: NDDS is NOT 3x\n");
#endif
          exit_close(EXIT_FAILURE);
       }
       
       memset(replymsge,0,10);
       cmd = NDDS_VER_QUERY;
       setupTimeout(15.0);
       result = send2Monitor(cmd, 60 /* timeout 60 sec */, 0, 0, 0, 0 );
       /* wait for reply */
       MonitorReply(&replycmd,&replyarg1, &replyarg2, &replyarg3, replymsge);
       printInfo("NDDS Version Query Response: '%s\n", replymsge);
       DestroyDomain();  // make sure to notify console this pub is going away
#ifdef RTI_NDDS_4x
       res = strstr(replymsge,"4.2");
#else
       res = strstr(replymsge,"3.1");
#endif
       if (debuglevel == 1)
#ifdef RTI_NDDS_4x
          printf("Is NDDS 4x: '%s'\n",(res == NULL) ? "FALSE" : "TRUE");
#else
          printf("Is NDDS 3x: '%s'\n",(res == NULL) ? "FALSE" : "TRUE");
#endif
       if (res == NULL)
         exit_close(EXIT_FAILURE);
       else
         exit_close(EXIT_SUCCESS);
#ifdef RTI_NDDS_4x
    }
#endif
}

static void TimeoutHandler()
{
   DestroyDomain();  // make sure to notify console this pub is going away
   fprintf(stderr,"Master did not respond to query within timeout period, continuing...\n");
   exit_close(EXIT_FAILURE);
}
/*-------------------------------------------------------------------------
|
|   Setup the interrupt handler for the timer interval alarm 
|	Statuscheck routine checks HAL status and broadcast the Acq. status.
|	Done code processing.
|    
|   added SIGALRM to the interrupt mask with Solaris
+--------------------------------------------------------------------------*/
static void setupTimeout(double sec)
{
    sigset_t		qmask;
    struct sigaction intserv;

    /* --- set up signal handler --- */

    sigemptyset( &qmask );
    // sigaddset( &qmask, SIGCHLD );
    sigaddset( &qmask, SIGALRM );
    // sigaddset( &qmask, SIGIO );
    intserv.sa_handler = TimeoutHandler;
    intserv.sa_mask = qmask;
#ifdef SOLARIS
    intserv.sa_flags = SA_RESTART;
#else
    intserv.sa_flags = 0;
#endif

    sigaction( SIGALRM, &intserv, NULL );
    setRtimer(sec,sec);
}
/*-------------------------------------------------------------------------
|
|   Setup the timer interval alarm 
|    
+--------------------------------------------------------------------------*/
static int setRtimer(double timsec,double interval)
{
    long sec,frac;
    struct itimerval timeval,oldtime;

    sec = (long) timsec;
    frac = (long) ( (timsec - (double)sec) * 1.0e6 ); /* usecs */
    // printf("setRtimer(): sec = %ld, frac = %ld\n",sec,frac);
    timeval.it_value.tv_sec = sec;
    timeval.it_value.tv_usec = frac;
    sec = (long) interval;
    frac = (long) ( (interval - (double)sec) * 1.0e6 ); /* usecs */
    // printf("setRtimer(): sec = %ld, frac = %ld\n",sec,frac);
    timeval.it_interval.tv_sec = sec;
    timeval.it_interval.tv_usec = frac;
    if (setitimer(ITIMER_REAL,&timeval,&oldtime) == -1)
    {
	     perror("setitimer error");
         return(1);
    }
    return(0);
}
