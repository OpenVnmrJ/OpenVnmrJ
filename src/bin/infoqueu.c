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

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <sys/mman.h>
#include <netinet/in.h>
#include <signal.h>
#include <netdb.h>

#include "ACQPROC_strucs.h"
#include "hostAcqStructs.h"
/**
#include "STAT_DEFS.h"
**/
#include "shrstatinfo.h"

#define EXP_STATUS_NAME "/tmp/ExpStatus"


#define TESTINTERVAL 600	/* Test Port every 10 Min */

#define TIME_OFF       1
#define TIME_ON        2
#define TIME_RESET     3
#define TIME_NORMAL    4
#define TIME_FAST      5
#define TIME_FASTER    6
#define TIME_SLOWER    7
#define TIME_SLOW      8


/* --- Acquisition Status Update Port Registry Queue structure --- */
static
struct regval    {
                  char *UserID;     		/* Users name */
                  int   UserPid;     		/* status display pid */
                  char *HostID;     		/* Host name of INET socket */
                  int   UpdatePort;     	/* Inter-Net Port Number */
		  struct sockaddr_in PortAddr;  /* inter-Net Address */
                  long  RegSub;       	/* Date and Time of day of submission.*/
        	};
typedef struct regval RegValue ;

/* --- Registry Queue packet structure --- */
static
struct regpacket   {   struct regval  *valreg;
                       struct regpacket *nextrp;
                   };
typedef struct regpacket RegPacket ;


int	      consoleActive;
AcqStatBlock  acqinfo;
extern int    Acqdebug;
extern char   vnmrsystem[];



static RegPacket *Registery = NULL;
static int    Regsd;
static int    activePort = 0;

long   PresentTime;     /* present Time and Day */
messpacket MessPacket;

initregqueue()
{
    Registery = (RegPacket *)0;
    Regsd = socket(AF_INET,SOCK_DGRAM,0); /* create a socket */
    if (Regsd == -1)
    {
        perror("initregqueue(): socket");
        return(-1);
    }
    if (Acqdebug)
        fprintf(stderr,"initregqueue(): socket created fd=%d\n",Regsd);
    return(0);
}

/*------------------------------------------------------------------
|
|   logstatusport()/3
|       enter an acquisition display update port into the register queue
|       pid - The registering process's PID. 
|       hostname - hostname of the inter-process communications socket
|                    that originated this queue submission.
|       inetport - The inter-net port number of socket
|
+------------------------------------------------------------------*/
logstatusport(username,pid,hostname,inetport,inetent)
int pid;              /* unique process's ID number */
int inetport;           /* inter-net port number of socket */
char *username;
char *hostname;
struct hostent *inetent;	/* host entry for system requesting status */
{
    RegPacket *getlastRp();
    RegPacket *getRegPacket();
    RegPacket *p;
    RegPacket *lastp;
    struct timeval clock;
    struct timezone tzone;
    long datetime;

    gettimeofday(&clock,&tzone);        /* get Time & Date of submission */
    datetime = PresentTime = clock.tv_sec;
 
    rmstatusport(hostname,inetport);	/* remove any identical entry */

    p = getRegPacket(username,pid,hostname,inetport,datetime,inetent);
    if ( p == NULL )
    {
        if (Acqdebug)
              fprintf(stderr, "Failed to Register: User '%s' on Machine '%s'\n",
                   username,MessPacket.Hostname);
        return(-1);
    }
    if (Registery == NULL)
        Registery = p;
    else
    {   lastp = getlastRp();
        lastp->nextrp = p;
        lastp->nextrp->nextrp = 0;
    }
    return (0);
}

/*--------------------------------------------------------------
|
|       prints the registery queue information 
|
+---------------------------------------------------------------*/
checkRegQue()
{  
    RegPacket *p;
    RegValue  *valp;
    char *chrptr;
    char  datetim[26];
    struct tm *tmtime;
 
    activePort = 0;
    if (Registery != NULL)
    {   
	p = Registery;
        while (p)
        {   valp = p->valreg;
	    activePort++;
            tmtime = localtime(&(valp->RegSub));
            chrptr = asctime(tmtime);
            strcpy(datetim,chrptr);
            datetim[24] = 0;
	    if (Acqdebug)
                fprintf(stderr,
	          "User: '%s', PID: %d, Submitted: %s, Host: '%s', InterNet Port : %d \n",
              valp->UserID,valp->UserPid,datetim,valp->HostID,valp->UpdatePort);
            p = p->nextrp;
        }
    }
    else     
	if (Acqdebug)
            fprintf(stderr,"No Display Ports Registered.\n");
}
 
/*---------------------------------------------------------------
|
|       getlastRp()/1
|         Scans down the link list of the Update register ports and returns
|         RegPacket pointer of the last RegPacket in the link list
|
+----------------------------------------------------------------*/
static RegPacket *
getlastRp()
{   RegPacket *p;
 
    p = Registery;
    while (p->nextrp)
    {
          p = p->nextrp;
    }
    return(p);
}
/*-------------------------------------------------------------------
|
|    getRegPacket()/8
|       Allocates memory (malloc) for a new RegPacket, RegValue Structure
|       and strings
|       From passed information the following parameters are set:
|               pid - set to registering process's ID  
|               SocketID - set to originating machine name (Unix hostname)
|               InetPort - set to the inter-net Port Number (for IPC)
|               DatSub - set to the date & time of Exp Submission
|
|    getRegPacket used to call gethostbyname to obtain information about
|    the system requesting the status.  Since this could cause Acqproc to
|    core dump, it had to be removed.  However Acqproc must send status
|    to any system that requests.  Therefore the Acqstat program is being
|    modified to send the required information returned by gethostbyname
|    This stuff is then passed to getRegPacket via logstatusport in the
|    inetent argument.                                 05/19/1994
+-------------------------------------------------------------------*/
static RegPacket *
getRegPacket(username,pid,hostname,portnum,datetime,inetent)
char *username;		/* users name */
int   pid;              /* process ID of registering process */ 
char *hostname;         /* acqfile where data will be stored */
int   portnum;          /* Inter-Net Port number  of the Above */
long  datetime;		/* date & time of registery submission */
struct hostent *inetent;	/* host entry for system requesting status */
{
    struct sockaddr_in *sinp;
    RegPacket *p;
 
    if (p = (RegPacket *)malloc(sizeof(RegPacket)))
    {   p->nextrp = 0L;
        if ( p->valreg = (RegValue *)malloc(sizeof(RegValue)))
        {   
	    if (p->valreg->HostID = (char *)malloc(strlen(hostname)+1))
            {   
		strcpy(p->valreg->HostID,hostname);
                if (p->valreg->UserID = (char *)malloc(strlen(username)+1))
                {   
		    strcpy(p->valreg->UserID,username);
                    p->valreg->UserPid = pid;
                    p->valreg->UpdatePort = portnum;
                    p->valreg->RegSub = datetime;
#if 0
    		    hp = gethostbyname(hostname);/*get inet address eg. 192.9.200.3 */
    		    if (hp == NULL)
    		    {
       		        fprintf(stderr,"getRegPacket(): Unknown Host: '%s'\n",
		        	hostname);
			free(p->valreg->UserID); /* release string memory */
			free(p->valreg->HostID); /* release string memory */
			free(p->valreg);         /* release Value memory */
			free(p);                 /* release Packet memory */
       		        return(NULL);
    		    }
#endif
		    sinp = &(p->valreg->PortAddr);
                    memset((char *) sinp,0,sizeof(struct sockaddr_in));/* clear */
		    /* copy inet address */
    		    memcpy( (char *)&(sinp->sin_addr), inetent->h_addr, inetent->h_length );
    		    sinp->sin_family = inetent->h_addrtype;
    		    sinp->sin_port = portnum;
                    return(p);
	        }
	        else
	        {
		    fprintf(stderr,"getRegPacket():malloc out of space\n");
                    free(p->valreg->HostID); /* release string memory */
                    free(p->valreg);         /* release Value memory */
                    free(p);                 /* release Packet memory */
		    return(NULL);
	        }
	    }
	    else
	    {
		fprintf(stderr,"getRegPacket():malloc out of space\n");
                free(p->valreg);         /* release Value memory */
                free(p);                 /* release Packet memory */
		return(NULL);
	    }
	}
	else
	{   fprintf(stderr,"getRegPacket():malloc out of space\n");
            free(p);                 /* release Packet memory */
            return(NULL);
	}
    }
    else
    {   fprintf(stderr,"getRegPacket():malloc out of space\n");
        return(NULL);
    }
}

/*-----------------------------------------------------------------
|
|    rmstatusport()/2
|       remove entry form a register queue
|       searches down the registery link list and removes and frees
|       the memory space if found.
|       Return the link list position if found
|       else it returns 0.
|        
+-----------------------------------------------------------------*/
rmstatusport(hostname,port)
char *hostname;
int port;
{   int listpos;
    RegPacket *p;
    RegPacket *lastp;
    RegValue  *valp;
 
    listpos = 1;
    lastp = NULL;
    p = Registery;
    if (p == NULL) return(0);
    while (p)
    {   
        valp = p->valreg;
        if ((port == valp->UpdatePort) && (strcmp(hostname,valp->HostID) == 0))
        {                               /* its a match */
	    if (p == Registery)
                Registery = p->nextrp;
            else
                lastp->nextrp = p->nextrp;
            free(valp->HostID); /* release string memory */
            free(valp->UserID); /* release string memory */
            free(valp);         /* release Value memory */
            free(p);            /* release Packet memory */
            return(listpos);    /* position in this priority list */
        }
        else
        {   
            listpos++;
            lastp = p;
            p = p->nextrp;
        }
    }    
    return(0);
}
/*-----------------------------------------------------------------------
|
|	send UPdate of acquisition information to Registered Ports
|
+------------------------------------------------------------------------*/
SendAcqStat()
{
    RegPacket *p;
    RegValue  *valp;
    int status;
    int pos;

    if (Acqdebug)
	fprintf(stderr," SendAcqStat ===>\n");
    if (Registery != NULL)
    {   
        p = Registery;
        while (p)
        {   
	    valp = p->valreg;
            if ((PresentTime - valp->RegSub) > TESTINTERVAL)
	    {
		if (PortThere(valp))
		{
		    valp->RegSub = PresentTime;
		}
		else
		{
                    p = p->nextrp;
		    pos = rmstatusport(valp->HostID,valp->UpdatePort);
		    /*printf("rm Port from posistion %d in list \n",pos);*/
		    if (!p) break;
        	    valp = p->valreg;
		}
	    }
	    if (Acqdebug)
                fprintf(stderr,"   Send Update to: '%s', InterNet Port : %d \n",
                    valp->HostID,valp->UpdatePort);
            status = sendto(Regsd,(char *)&acqinfo,sizeof(AcqStatBlock),0,
		(struct sockaddr *)&(valp->PortAddr),sizeof(struct sockaddr_in));
	    if (Acqdebug)
                fprintf(stderr,"   sendto stat = %d\n",status);
            p = p->nextrp;
        }
    }
    else     
	if (Acqdebug > 1)
            fprintf(stderr,"   No Display Ports Registered.\n");
    if (Acqdebug)
	fprintf(stderr," SendAcqStat <===\n");
}
/*-----------------------------------------------------------------------
|
|	PortThere()/1
|	Test and see if Port is still present 
|
+------------------------------------------------------------------------*/
PortThere(ptr)
RegValue *ptr;
{
    return(1);
}

#define STRLEN 128
#define REGISTERPORT 2
#define UNREGPORT 3
#define NOMESSAGE 0

extern struct hostent *this_hp;

static void
register_local_port( username, userpid )
char *username;
int userpid;
{
         if (Acqdebug)
	     fprintf(stderr, "    register_local_port  %s\n", username);
        logstatusport( username, userpid, MessPacket.Hostname,
		 	MessPacket.Port, this_hp);
}


static void
register_remote_port( username, userpid, messptrptr )
char *username;
int userpid;
char **messptrptr;
{
        int              stat_addr;
        char            *inet_addr_list[ 2 ];
        struct hostent   stat_entry;

        if (Acqdebug)
	     fprintf(stderr, "    register_remote_port  %s\n", username);
        memset( &stat_entry, 0, sizeof( stat_entry ) );
        stat_addr = getinttoken( messptrptr );
        stat_entry.h_addrtype = getinttoken( messptrptr );
        stat_entry.h_length = getinttoken( messptrptr );
        if (stat_entry.h_length > sizeof( int )) {
                fprintf( stderr,
           "for remote host %s, received address length of %d, expected %d\n",
		 MessPacket.Hostname, stat_entry.h_length, sizeof( int ));
                return;
        }
        stat_entry.h_addr_list = &inet_addr_list[ 0 ];
        inet_addr_list[ 1 ] = NULL;
        inet_addr_list[ 0 ] = (char *) &stat_addr;

        logstatusport( username, userpid, MessPacket.Hostname, 
			MessPacket.Port, &stat_entry);
}


void
Smessage()
{
    char username[STRLEN];
    char *messptr;
    int userpid;

    if (Acqdebug)
        fprintf(stderr,
           "========> SIGIO INTERRUPT PROCESSING STARTING <======\n");
    if ( GetMessage() != NOMESSAGE)     /* if no message return */
    {
      messptr = MessPacket.Message;
      switch(MessPacket.CmdOption)
      {
         case REGISTERPORT:   /* Register Acq. Display Update port */
                if (Acqdebug)
                    fprintf(stderr,"Register Update socket Port\n");

                getstrtoken( &username[ 0 ], STRLEN, &messptr );
                userpid = getinttoken( &messptr );
                if ((int)strlen( messptr ) <= 0)
                  register_local_port( &username[ 0 ], userpid );
                else
                  register_remote_port( &username[ 0 ], userpid, &messptr );
                checkRegQue();
                if (activePort > 0)
                {
                   SendAcqStat();
                }
	      break;
        case UNREGPORT:         /* Remove Acq. Display Update port */
                if (Acqdebug)
                   fprintf(stderr,"Delete Update socket Port\n");
                rmstatusport(MessPacket.Hostname,MessPacket.Port);
                checkRegQue();
                break;

	default:
                if (Acqdebug)
                  fprintf(stderr, " ** Unknown message '%d' **\n", MessPacket.CmdOption);
                break;
	}
	if (activePort > 0)
		setInfoTimer(TIME_NORMAL);
     }
}

initinfo()
{

    acqinfo.Acqstate = 0;  /* inactive */
    acqinfo.AcqExpInQue = 0;
    strcpy(acqinfo.AcqUserID," ");
    strcpy(acqinfo.AcqExpID," ");
    acqinfo.AcqFidElem = 0L;
    acqinfo.AcqCT = 0L;
    acqinfo.AcqLSDV = 0x0;
    acqinfo.AcqSuFlag = -1;             /* Use -1 to imply not-set; */
                                        /* value of 0 == EXEC_GO    */
    acqinfo.AcqCmpltTime = 0L;
    acqinfo.AcqRemTime = 0L;
    acqinfo.AcqDataTime = 0L;
    acqinfo.AcqLockLevel = 0;
    acqinfo.AcqSpinSet = -1;
    acqinfo.AcqSpinAct = -1;
    acqinfo.AcqVTSet = -30000;
    acqinfo.AcqVTAct = -30000;
    acqinfo.AcqSample = 0;
    consoleActive = 0;
    update_statinfo();
    setupInfopoller();
}

static EXP_STATUS_STRUCT  *statusBlk = NULL;
static struct timeval lastTime;

int
update_statinfo()
{
    static int  fd = -1;
    struct stat s;
    struct timeval clock;
    struct timezone tzone;
    static long   oldClock = 0;
    char   mapfile[128];
    caddr_t p;
    FILE   *fd2;
    int	   inactive;

    if (Acqdebug)
	fprintf(stderr, "   update_statinfo ...\n");
    if (fd < 0)
    {
	sprintf(mapfile, "%s/acqqueue/acqinfo.map", vnmrsystem);
	if ( (fd2 = fopen(mapfile, "r")) == NULL)
        {
	    if (Acqdebug)
        	fprintf(stderr,"Could not open file %s\n", mapfile);
	    strcpy(mapfile, EXP_STATUS_NAME);
	}
	else
	{
	    if (fscanf(fd2, "%s", mapfile) != 1)
	    {
		if (Acqdebug)
                   fprintf(stderr,"File %s is empty\n", mapfile);
	    	strcpy(mapfile, EXP_STATUS_NAME);
	    }
	    fclose(fd2);
	}

	if (Acqdebug)
	    fd = open(mapfile,O_RDWR | O_CREAT, 0666);
	else
	    fd = open(mapfile,O_RDONLY);
        if (fd < 0)
	{
	   if (Acqdebug)
        	fprintf(stderr,"Could not open file %s\n", mapfile);
	   return(0);
	}
	if (fstat(fd, &s))
	{
	   if (Acqdebug)
        	fprintf(stderr,"Could not read file %s\n", mapfile);
	   close(fd);
	   fd = -1;
	   return(0);
	}
	if (s.st_size < sizeof(EXP_STATUS_STRUCT))
	{
	   if (Acqdebug)
	   {
                fprintf(stderr,"File %s size is %d\n", mapfile, s.st_size);
		fprintf(stderr," It must be larger than %d.\n", sizeof(EXP_STATUS_STRUCT));
	   }
	   close(fd);
	   fd = -1;
	   return(0);
	}

	if((p = mmap(0,sizeof(EXP_STATUS_STRUCT),PROT_READ, MAP_SHARED,fd,0)) == (caddr_t)-1)
	{
	   if (Acqdebug)
        	fprintf(stderr,"Could not map file %s\n", mapfile);
	   close(fd);
	   fd = -1;
	   return(0);
	}
        if (Acqdebug)
                fprintf(stderr," map addr is %ul \n", p);
	statusBlk = (EXP_STATUS_STRUCT *) p;
	lastTime.tv_sec = 0;
	lastTime.tv_usec = 0;
	oldClock = 0;
    }
    if (Acqdebug)
    {
       	fprintf(stderr," Acqinfo   old time = %d\n",lastTime.tv_sec);
       	fprintf(stderr,"           new time = %d\n",statusBlk->TimeStamp.tv_sec);
    }
    gettimeofday(&clock,&tzone);
    inactive = 0;
    if (statusBlk->TimeStamp.tv_sec == lastTime.tv_sec)
    {
	if ((clock.tv_sec - oldClock) >= 10)
	{  /* if no chage longer than 10 seconds */
	   inactive = 1;
	   if ( !Acqdebug )
    	      acqinfo.Acqstate = ACQ_INACTIVE;
	}
	if (Acqdebug)
            fprintf(stderr,"Acqinfo time stamp does not change\n");
	else
            return((int)acqinfo.Acqstate);
    }
    else
        oldClock = clock.tv_sec;
    lastTime.tv_sec = statusBlk->TimeStamp.tv_sec;
    lastTime.tv_usec = statusBlk->TimeStamp.tv_usec;
    if (Acqdebug && inactive)
    {
        acqinfo.AcqCT = acqinfo.AcqCT + 1;
	if (acqinfo.Acqstate >= ACQ_SMPCHANGE)
    		acqinfo.Acqstate = ACQ_IDLE;
	else
    		acqinfo.Acqstate = acqinfo.Acqstate + 10;
    	strcpy(acqinfo.AcqUserID, "debug");
    	strcpy(acqinfo.AcqExpID, "none");
        return((int)acqinfo.Acqstate);
    }

    acqinfo.AcqCT = statusBlk->CT;
    acqinfo.AcqCmpltTime = statusBlk->CompletionTime;
    acqinfo.AcqRemTime = statusBlk->RemainingTime;
    acqinfo.AcqDataTime = statusBlk->DataTime;
    acqinfo.Acqstate = statusBlk->csb.Acqstate;
    acqinfo.AcqExpInQue = statusBlk->ExpInQue;
    acqinfo.AcqFidElem = statusBlk->FidElem;
    acqinfo.AcqLSDV = statusBlk->csb.AcqLSDVbits;
    acqinfo.AcqLockLevel = statusBlk->csb.AcqLockLevel;
    acqinfo.AcqSpinSet = statusBlk->csb.AcqSpinSet;
    acqinfo.AcqSpinAct = statusBlk->csb.AcqSpinAct;
    acqinfo.AcqVTSet = statusBlk->csb.AcqVTSet;
    acqinfo.AcqVTAct = statusBlk->csb.AcqVTAct;
    acqinfo.AcqSample = statusBlk->csb.AcqSample;
    strcpy(acqinfo.AcqUserID, statusBlk->UserID);
    strcpy(acqinfo.AcqExpID, statusBlk->ProcExpID);

    if (Acqdebug)
	fprintf(stderr, "   update_statinfo ... done \n");
    return((int)acqinfo.Acqstate);
}


/*-------------------------------------------------------------------------
|
|   Setup the interrupt handler for the timer interval alarm 
|	Statuscheck routine checks Console status and broadcast the Acq. status.
|    
|   added SIGCHLD and SIGALRM to the interrupt mask with Solaris
+--------------------------------------------------------------------------*/
setupInfopoller()
{
    void Statuscheck();
    sigset_t		qmask;
    struct sigaction intserv;

    /* --- set up signal handler --- */

    sigemptyset( &qmask );
    sigaddset( &qmask, SIGCHLD );
    sigaddset( &qmask, SIGALRM );
    sigaddset( &qmask, SIGIO );
    sigaddset( &qmask, SIGUSR2 );
    intserv.sa_handler = Statuscheck;
    intserv.sa_mask = qmask;
    intserv.sa_flags = 0;

    sigaction( SIGALRM, &intserv, NULL );
    sigaction( SIGUSR2, &intserv, NULL );
}

/*-------------------------------------------------------------------------
|
|   Setup the timer interval alarm 
|    
+--------------------------------------------------------------------------*/
static setRtimer(timsec,interval)
double timsec;
double interval;
{
    long sec,frac;
    struct itimerval timeval,oldtime;

    if (Acqdebug)
	fprintf(stderr, "   set timer:  %f sec\n", timsec, interval);
    sec = (long) timsec;
    frac = (long) ( (timsec - (double)sec) * 1.0e6 ); /* usecs */
    if (Acqdebug)
        fprintf(stderr,"setRtimer(): sec = %ld, frac = %ld\n",sec,frac);
    timeval.it_value.tv_sec = sec;
    timeval.it_value.tv_usec = frac;
    sec = (long) interval;
    frac = (long) ( (interval - (double)sec) * 1.0e6 ); /* usecs */
    if (Acqdebug)
        fprintf(stderr,"setRtimer(): sec = %ld, frac = %ld\n",sec,frac);
    timeval.it_interval.tv_sec = sec;
    timeval.it_interval.tv_usec = frac;
    if (setitimer(ITIMER_REAL,&timeval,&oldtime) == -1)
    {
	 perror("setitimer error");
         return(1);
    }
    return(0);
}

setInfoTimer(action)
int action;
{
   static double initTime = 5.0;
   static double pollTime = 5.0;

   switch (action)
   {
     case TIME_OFF:
         if (Acqdebug)
            fprintf(stderr,"timer off");
         setRtimer(0.0,0.0);         /* stop acqhandler from executing */
         break;
     case TIME_ON:
         setupInfopoller();
     case TIME_RESET:
         pollTime = 5.0;
         initTime = 5.0;
         if (Acqdebug)
            fprintf(stderr,"timer on/reset %g %g\n",initTime,pollTime);
         setRtimer(initTime,pollTime);
         break;
      case TIME_NORMAL:
         if (Acqdebug)
            fprintf(stderr,"timer normal %g %g\n",initTime,pollTime);
         initTime = 3.0;
         setRtimer(initTime,pollTime);
         break;
      case TIME_FAST:
         initTime = 2.0;
         if (Acqdebug)
            fprintf(stderr,"timer fast   %g %g\n",initTime,pollTime);
         setRtimer(initTime,pollTime);
         break;
      case TIME_FASTER:
         initTime = 1.0;
         if (Acqdebug)
            fprintf(stderr,"timer faster %g %g\n",initTime,pollTime);
         setRtimer(initTime,pollTime);
         break;
      case TIME_SLOWER:
         initTime = 10.0;
         if (Acqdebug)
            fprintf(stderr,"timer slower %g %g\n",initTime,pollTime);
         setRtimer(initTime,pollTime);  /* restart SIGALRM SIGNAL */
         break;
      case TIME_SLOW:
         initTime = 5.0;
         if (Acqdebug)
            fprintf(stderr,"timer slower %g %g\n",initTime,pollTime);
         setRtimer(initTime,pollTime);  /* restart SIGALRM SIGNAL */
         break;
   }
}


void
Statuscheck()
{
    struct timeval clock;
    struct timezone tzone;
    int		 timeit;



    if (Acqdebug)
	fprintf(stderr, "  Statuscheck\n");
    setInfoTimer(TIME_OFF);
    if (Acqdebug)
    {
        fprintf(stderr,
                "~~~~~~~~> SIGALRM INTERRUPT PROCESSING STARTING <~~~~~~\n");
    }

    gettimeofday(&clock,&tzone);
    PresentTime = clock.tv_sec;

    consoleActive = update_statinfo();
    if (activePort > 0)
    {
    	SendAcqStat();
	if ( consoleActive <= ACQ_IDLE)
	     timeit = TIME_SLOWER;
	else if (consoleActive == ACQ_INTERACTIVE)
	     timeit = TIME_SLOW;
	else if (consoleActive == ACQ_ACQUIRE)
	     timeit = TIME_SLOWER;
	else
	     timeit = TIME_NORMAL;
	setInfoTimer(timeit);
    }
}

