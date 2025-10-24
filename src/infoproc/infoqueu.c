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
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <sys/mman.h>
#include <netinet/in.h>
#include <signal.h>
#include <netdb.h>

#include "ACQPROC_strucs.h"
#include "hostAcqStructs.h"
#include "shrstatinfo.h"

#define EXP_STATUS_NAME "/vnmr/acqqueue/ExpStatus"


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
struct regval    {
                  char *UserID;     		/* Users name */
                  int   UserPid;     		/* status display pid */
                  char *HostID;     		/* Host name of INET socket */
                  int   UpdatePort;     	/* Inter-Net Port Number */
		  struct sockaddr_in PortAddr;  /* inter-Net Address */
                  int  RegSub;       	/* Date and Time of day of submission.*/
        	};
typedef struct regval RegValue ;

/* --- Registry Queue packet structure --- */
struct regpacket   {   struct regval  *valreg;
                       struct regpacket *nextrp;
                   };
typedef struct regpacket RegPacket ;


int	      consoleActive;
AcqStatBlock  acqinfo;
extern int    Acqdebug;
extern char   vnmrsystem[];
extern int GetMessage();
extern int getinttoken(char **strptr);
extern int getstrtoken(char *substring, int maxlen, char **strptr);
extern int setStatRemTime(int diftime);
int update_statinfo();

int encodedAcqSample = 0;   /* encoded AcqSample as comes from console, by consolestatAction() in nddsinfofuncs.c */

static RegPacket *Registery = NULL;
static int    Regsd;
static int    activePort = 0;

static int   PresentTime;     /* present Time and Day */
messpacket MessPacket;

static void setInfoTimer(int action);
static void setupInfopoller();
static RegPacket *getRegPacket(char *username, int pid, char *hostname,
             int portnum, int datetime, struct hostent *inetent);
static RegPacket *getlastRp();

int initregqueue()
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
int logstatusport(username,pid,hostname,inetport,inetent)
int pid;              /* unique process's ID number */
int inetport;           /* inter-net port number of socket */
char *username;
char *hostname;
struct hostent *inetent;	/* host entry for system requesting status */
{
    RegPacket *p;
    RegPacket *lastp;
    struct timeval clock;
    int datetime;

    gettimeofday(&clock, NULL);        /* get Time & Date of submission */
    datetime = PresentTime = clock.tv_sec;
 
    p = Registery;
    /* If entry already exists, just update its time stamp */
    while (p)
    {   
        RegValue  *valp;
        valp = p->valreg;
        if ((inetport == valp->UpdatePort) &&
            (strcmp(hostname,valp->HostID) == 0))
        {                               /* its a match */
           p->valreg->RegSub = datetime;
           if (Acqdebug)
              fprintf(stderr, "Re-register: User '%s' on %s's Port %d\n",
                   username,hostname,inetport);
           return(0);
        }
        p = p->nextrp;
    }

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
static void checkRegQue()
{  
    RegPacket *p;
    RegValue  *valp;
    char *chrptr;
    char  datetim[26];
    struct tm *tmtime;
 
    activePort = 0;
    if (Registery != NULL)
    {   
        time_t tvsec;
        p = Registery;
        while (p)
        {   valp = p->valreg;
	         activePort++;
            tvsec = valp->RegSub;
            tmtime = localtime( &tvsec );
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
static RegPacket *getlastRp()
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
|
|    char *username;		users name
|    int   pid;                 process ID of registering process
|    char *hostname;            acqfile where data will be stored
|    int   portnum;             Inter-Net Port number  of the Above
|    int   datetime;		date & time of registery submission
|    struct hostent *inetent;	host entry for system requesting status
+-------------------------------------------------------------------*/
static RegPacket *getRegPacket(char *username, int pid, char *hostname,
             int portnum, int datetime, struct hostent *inetent)
{
    struct sockaddr_in *sinp;
    RegPacket *p;
 
    if ( (p = (RegPacket *)malloc(sizeof(RegPacket))) )
    {   p->nextrp = 0L;
        if ( (p->valreg = (RegValue *)malloc(sizeof(RegValue))) )
        {   
	    if ( (p->valreg->HostID = (char *)malloc(strlen(hostname)+1)) )
            {   
		strcpy(p->valreg->HostID,hostname);
                if ( (p->valreg->UserID = (char *)malloc(strlen(username)+1)) )
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
static int rmstatusport(hostname,port)
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
static void SendAcqStat()
{
    RegPacket *p;
    RegValue  *valp;
    int status;
    int pos __attribute__((unused));

    if (Acqdebug)
	fprintf(stderr," SendAcqStat ===>\n");
    if (Registery != NULL)
    {   
        p = Registery;
        while (p)
        {   
	    valp = p->valreg;
	    if (Acqdebug)
                fprintf(stderr,"   %d seconds since last registration\n",
                    PresentTime - valp->RegSub);
            if ((PresentTime - valp->RegSub) > TESTINTERVAL)
	    {
               p = p->nextrp;
	       pos = rmstatusport(valp->HostID,valp->UpdatePort);
	       /*printf("rm Port from posistion %d in list \n",pos);*/
	       if (!p) break;
               valp = p->valreg;
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
           "for remote host %s, received address length of %d, expected %zd\n",
		 MessPacket.Hostname, stat_entry.h_length, sizeof( int ));
                return;
        }
        stat_entry.h_addr_list = &inet_addr_list[ 0 ];
        inet_addr_list[ 1 ] = NULL;
        inet_addr_list[ 0 ] = (char *) &stat_addr;

        logstatusport( username, userpid, MessPacket.Hostname, 
			MessPacket.Port, &stat_entry);
}


void Smessage()
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

void initinfo()
{

    acqinfo.Acqstate = 0;  /* inactive */
    acqinfo.AcqExpInQue = 0;
    strcpy(acqinfo.AcqUserID,"");
    strcpy(acqinfo.AcqExpID,"");
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
    acqinfo.AcqSpinSpeedLimit = -1;
    acqinfo.AcqSpinSpan = -1;
    acqinfo.AcqSpinAdj = -1;
    acqinfo.AcqSpinMax = -1;
    acqinfo.AcqSpinActSp = -1;
    acqinfo.AcqSpinProfile = -1;
    strcpy(acqinfo.probeId1,"");
    strcpy(acqinfo.gradCoilId,"");
    acqinfo.AcqVTSet = -30000;
    acqinfo.AcqVTAct = -30000;
    acqinfo.AcqSample = 0;
    acqinfo.AcqZone = 0;
    acqinfo.AcqRack = 0;
    consoleActive = 0;
    update_statinfo();
    setupInfopoller();
}
static int timecmp(TIMESTAMP t1, TIMESTAMP t2)
{
    if (t1.tv_sec != t2.tv_sec) {
	return t1.tv_sec - t2.tv_sec;
    } else {
	return t1.tv_usec - t2.tv_usec;
    }
}

static void
update_rfinfo(EXP_STATUS_STRUCT *statblock)
{
    /* 5 min trip level in milliwatts vs. knob position */
    static int tripLevels[] = {1000,   1300,   1600,   2000,
			       2500,   3200,   4000,   5000,
			       6300,   7900,   10000,  12600,
			       15900,  20000,  25100,  31600,
			       39800,  50100,  63100,  79400,
			       100000, 126000, 159000, 200000,
			       0, 0, 0, 0,
                               0, 0, 600, 800};
    static TIMESTAMP t;	/* Last time long t.c. power was updated */
    static int ibuf = 0;	/* Where we are in the circular buffers */
    static unsigned int cbuf[4][30]; /* Circular buffers of past 10s values */
    static unsigned int rsum[4]; /* Running sum of long t.c. power */
    const static int tinc = 10;	/* How often to update long t.c. power (s) */
    int rf[4];
    unsigned int limit[4];	/* microwatts */
    unsigned int pwr[4];	/* microwatts */
    int chan;

    /* Initialize timer first time through */
    if (t.tv_sec == 0) {
	t = statblock->TimeStamp;
    }

    /* Set easy stuff in acqinfo */
    for (chan=0; chan<4; chan++) {
        int knob;
	rf[chan] = statblock->csb.rfMonitor[chan];
        knob = (rf[chan] >> 6) & 0x1f;
	limit[chan] = 1000 * tripLevels[knob]; /* In uW */
        /* NB: limit for short time is 5x limit for long time constant,
         * and the only status we get is for short time constant */
	pwr[chan] = (rf[chan] & 0x3f) * (5 * limit[chan] / 63);
	acqinfo.AcqShortRfPower[chan] = pwr[chan];
	acqinfo.AcqShortRfLimit[chan] = 5 * limit[chan];
	acqinfo.AcqLongRfLimit[chan] = limit[chan];
    }

    /* Estimate long time constant power and put it in acqinfo */
    for ( ; timecmp(statblock->TimeStamp, t) >= 0; t.tv_sec += tinc, ibuf++) {
	if (ibuf >= 30) {ibuf = 0;} /* Wrap circular buffer */
	for (chan=0; chan<4; chan++) {
	    unsigned int pwr2;	/* Power scaled to avg over 30 samples (uW) */
	    pwr2 = (pwr[chan] + 15) / 30;
	    rsum[chan] += pwr2 - cbuf[chan][ibuf];
	    cbuf[chan][ibuf] = pwr2;
	    acqinfo.AcqLongRfPower[chan] = rsum[chan];
	}
    }
}

static EXP_STATUS_STRUCT  *statusBlk = NULL;
static TIMESTAMP lastTime;

int
update_statinfo()
{
    static int  fd = -1;
    int i;
    struct stat s;
    struct timeval clock;
    static int   oldClock = 0;
    static int   deadTime = 0;
    char   mapfile[128];
    void *p;
    FILE   *fd2;
#ifdef NIRVANA
    int    tmpRem;
#endif

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
                fprintf(stderr,"File %s size is %ld\n", mapfile, s.st_size);
		fprintf(stderr," It must be larger than %zd\n", sizeof(EXP_STATUS_STRUCT));
	   }
	   close(fd);
	   fd = -1;
	   return(0);
	}

	if((p = mmap(0,sizeof(EXP_STATUS_STRUCT),PROT_READ, MAP_SHARED,fd,0)) == (void *)-1)
	{
	   if (Acqdebug)
        	fprintf(stderr,"Could not map file %s\n", mapfile);
	   close(fd);
	   fd = -1;
	   return(0);
	}
        if (Acqdebug)
                fprintf(stderr," map addr is %p \n", p);
	statusBlk = (EXP_STATUS_STRUCT *) p;
	lastTime.tv_sec = 0;
	lastTime.tv_usec = 0;
	oldClock = 0;
    }
    if (Acqdebug)
    {
       	fprintf(stderr," Acqinfo   old time = %d %d\n",
		lastTime.tv_sec, lastTime.tv_usec);
       	fprintf(stderr,"           new time = %d %d\n",
		statusBlk->TimeStamp.tv_sec, statusBlk->TimeStamp.tv_usec);
    }
    gettimeofday(&clock, NULL);
    if ((statusBlk->TimeStamp.tv_sec  == lastTime.tv_sec) &&
        (statusBlk->TimeStamp.tv_usec == lastTime.tv_usec))
    {
	if ((clock.tv_sec - oldClock) >= 10)
	{  /* if no chage longer than 10 seconds */
	   if ( !Acqdebug )
	      acqinfo.Acqstate = ACQ_INACTIVE;
#ifdef NIRVANA
           setStatAcqState(ACQ_INACTIVE);
#endif
	}
	if (Acqdebug)
            fprintf(stderr,"Acqinfo time stamp does not change\n");
	else
            return((int)acqinfo.Acqstate);
    }
    lastTime.tv_sec = statusBlk->TimeStamp.tv_sec;
    lastTime.tv_usec = statusBlk->TimeStamp.tv_usec;


    if (statusBlk->csb.rfMonitor[0] != -1) {
	/* RF Monitor present; update average power data */
	update_rfinfo(statusBlk);
    }
#ifdef NIRVANA
    else
    {
        acqinfo.AcqLongRfLimit[0] = -1;     /* takes monitor block of the hardwarebar */
    }
#endif


    acqinfo.AcqCT = statusBlk->CT;
#ifndef NIRVANA
    acqinfo.AcqCT += 1;
#endif
    acqinfo.AcqDataTime = statusBlk->DataTime;
    acqinfo.Acqstate = statusBlk->csb.Acqstate;
    acqinfo.AcqExpInQue = statusBlk->ExpInQue;
    acqinfo.AcqFidElem = statusBlk->FidElem;
    acqinfo.AcqLSDV = statusBlk->csb.AcqLSDVbits;
    acqinfo.AcqLockLevel = statusBlk->csb.AcqLockLevel;
    acqinfo.AcqSpinSet = statusBlk->csb.AcqSpinSet;
    acqinfo.AcqSpinAct = statusBlk->csb.AcqSpinAct;
    acqinfo.AcqSpinSpeedLimit = statusBlk->csb.AcqSpinSpeedLimit;
    acqinfo.AcqSpinSpan = statusBlk->csb.AcqSpinSpan;
    acqinfo.AcqSpinAdj = statusBlk->csb.AcqSpinAdj;
    acqinfo.AcqSpinMax = statusBlk->csb.AcqSpinMax;
    acqinfo.AcqSpinActSp = statusBlk->csb.AcqSpinActSp;
    acqinfo.AcqSpinProfile = statusBlk->csb.AcqSpinProfile;
    acqinfo.AcqVTSet = statusBlk->csb.AcqVTSet;
    acqinfo.AcqVTAct = statusBlk->csb.AcqVTAct;
    acqinfo.AcqSample = statusBlk->csb.AcqSample;
    if (encodedAcqSample > 999) 
    {
       acqinfo.AcqRack = ( encodedAcqSample / 1000000L );
       acqinfo.AcqZone = ( encodedAcqSample  / 10000L % 100);
       /*  acqinfo.AcqSample = (encodedAcqSample % 1000); */
    }
    else
    {
       acqinfo.AcqRack = 0;
       acqinfo.AcqZone = 0;
    }
 
    strcpy(acqinfo.probeId1, statusBlk->csb.probeId1);
    strcpy(acqinfo.gradCoilId, statusBlk->csb.gradCoilId);
    strcpy(acqinfo.AcqUserID, statusBlk->UserID);
    strcpy(acqinfo.AcqExpID, statusBlk->ExpID);
    acqinfo.AcqLockGain = statusBlk->csb.AcqLockGain;
    acqinfo.AcqLockPower = statusBlk->csb.AcqLockPower;
    acqinfo.AcqLockPhase = statusBlk->csb.AcqLockPhase;
    acqinfo.AcqShimSet = statusBlk->csb.AcqShimSet;
    for (i=0; i<MAX_SHIMS_CONFIGURED; i++) {
	acqinfo.AcqShimValues[i] = statusBlk->csb.AcqShimValues[i];
    }
#ifdef NIRVANA
    tmpRem = acqinfo.AcqRemTime;
#endif
    if((acqinfo.Acqstate != ACQ_INACTIVE) && (strlen(acqinfo.AcqExpID) > 0))
    {
       if ((acqinfo.Acqstate != ACQ_ACQUIRE) &&
           (acqinfo.Acqstate != ACQ_PAD) &&
           (acqinfo.Acqstate != ACQ_FINDZ0) &&
           (acqinfo.Acqstate != ACQ_HOSTGAIN) &&
           (acqinfo.Acqstate != ACQ_HOSTSHIM) )
       {
          deadTime += clock.tv_sec - oldClock;
       }
       if (acqinfo.Acqstate == ACQ_IDLE)
          deadTime = 0L;
       acqinfo.AcqRemTime = acqinfo.AcqCmpltTime = statusBlk->StartTime + statusBlk->ExpTime + deadTime;
       acqinfo.AcqRemTime -= clock.tv_sec;
       if (acqinfo.AcqRemTime < 0L)
	 acqinfo.AcqRemTime = 0L;
    }
    else
    {
      acqinfo.AcqCmpltTime = 0L;
      acqinfo.AcqRemTime = 0L;
      deadTime = 0L;
    }
#ifdef NIRVANA
    if (tmpRem != acqinfo.AcqRemTime)
       setStatRemTime(acqinfo.AcqRemTime);
#endif

    oldClock = clock.tv_sec;
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
static void setupInfopoller()
{
#ifndef NIRVANA
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

    sigemptyset( &qmask );
    sigaddset( &qmask, SIGUSR2 );
    sigprocmask( SIG_UNBLOCK, &qmask, NULL );
#endif
}


static void setInfoTimer(int action)
{
   static double pollTime = 3.0;

   switch (action)
   {
     case TIME_OFF:
         if (Acqdebug)
            fprintf(stderr,"timer off");
         alarm(0.0);
         break;
     case TIME_ON:
         setupInfopoller();
     case TIME_RESET:
         pollTime = 3.0;
         if (Acqdebug)
            fprintf(stderr,"timer on/reset %g\n",pollTime);
         alarm(pollTime);
         break;
      case TIME_NORMAL:
         if (Acqdebug)
            fprintf(stderr,"timer normal %g\n",pollTime);
         pollTime = 3.0;
         alarm(pollTime);
         break;
      case TIME_FAST:
         pollTime = 2.0;
         if (Acqdebug)
            fprintf(stderr,"timer fast   %g\n",pollTime);
         break;
      case TIME_FASTER:
         pollTime = 1.0;
         if (Acqdebug)
            fprintf(stderr,"timer faster %g\n",pollTime);
         alarm(pollTime);
         break;
      case TIME_SLOWER:
         pollTime = 5.0;
         if (Acqdebug)
            fprintf(stderr,"timer slower %g\n",pollTime);
         alarm(pollTime);
         break;
      case TIME_SLOW:
         pollTime = 4.0;
         if (Acqdebug)
            fprintf(stderr,"timer slower %g\n",pollTime);
         alarm(pollTime);
         break;
   }
}


void
Statuscheck()
{
    struct timeval clock;
    int		 timeit;



    if (Acqdebug)
	fprintf(stderr, "  Statuscheck\n");
    setInfoTimer(TIME_OFF);
    if (Acqdebug)
    {
        fprintf(stderr,
                "~~~~~~~~> SIGALRM INTERRUPT PROCESSING STARTING <~~~~~~\n");
    }

    gettimeofday(&clock, NULL);
    PresentTime = clock.tv_sec;

    consoleActive = update_statinfo();
    if (activePort > 0)
    {
    	SendAcqStat();
	if ( consoleActive <= ACQ_IDLE)
	     timeit = TIME_SLOW;
	else if (consoleActive == ACQ_INTERACTIVE)
	     timeit = TIME_SLOW;
	else if (consoleActive == ACQ_ACQUIRE)
	     timeit = TIME_NORMAL;
	else
	     timeit = TIME_NORMAL;
	setInfoTimer(timeit);
    }
}

