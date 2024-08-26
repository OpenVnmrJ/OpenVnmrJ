/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*------------------------------------------------------------------
|
|   Modified   Author     Purpose
|   --------   ------     -------
|   8/15/89    Greg B.    1. Added Code to allow RPC for remotehost display
|			     can be invoke as before without using RPC
|			     if a remote host is specified then RPC is used.
|
|   9/12/89    Greg B.    1. Changes for RPC workaround for 3.5 destroy_clnt() bug
|			     and changes for SUN OS 4.0.3 
+------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <pwd.h>
#include <signal.h>
#include <sys/file.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>

#include "acqstat_item.h"
#include "STAT_DEFS.h"

#define  StatX 8
#define  StatY 0
#define  PendX 36
#define  PendY 0
#define  UserX 6
#define  UserY 1
#define  ExpX 19
#define  ExpY 1
#define  SampX 36
#define  SampY 1
#define  ArrayX 5
#define  ArrayY 2
#define  CTX 18
#define  CTY 2
#define  DecX 33
#define  DecY 2

#define  CompX 17
#define  CompY 3
#define  LockX 34
#define  LockY 3
#define  RemainX 16
#define  RemainY 4
#define  LKX 35
#define  LKY 4
#define  DataX 16
#define  DataY 5
#define  SpinX 9
#define  SpinY 6
#define  VTX 27
#define  VTY 6
#define  SPNX 9
#define  SPNY 7
#define  SPNX2 9
#define  SPNY2 8
#define  VTX1 27
#define  VTY1 7
#define  VTX2 27
#define  VTY2 8
#define  AIRX 33
#define  AIRY 5

#define  RFMonLimX 12
#define  RFMonLimY 9

#define  RF10s1AvgX 14
#define  RF10s1AvgY 10
#define  RF10s1LimX 20
#define  RF10s1LimY 10
#define  RF5m1AvgX 30
#define  RF5m1AvgY 10
#define  RF5m1LimX 36
#define  RF5m1LimY 10

#define  RF10s2AvgX 14
#define  RF10s2AvgY 11
#define  RF10s2LimX 20
#define  RF10s2LimY 11
#define  RF5m2AvgX 30
#define  RF5m2AvgY 11
#define  RF5m2LimX 36
#define  RF5m2LimY 11

#define  RF10s3AvgX 14
#define  RF10s3AvgY 12
#define  RF10s3LimX 20
#define  RF10s3LimY 12
#define  RF5m3AvgX 30
#define  RF5m3AvgY 12
#define  RF5m3LimX 36
#define  RF5m3LimY 12

#define  RF10s4AvgX 14
#define  RF10s4AvgY 13
#define  RF10s4LimX 20
#define  RF10s4LimY 13
#define  RF5m4AvgX 30
#define  RF5m4AvgY 13
#define  RF5m4LimX 36
#define  RF5m4LimY 13

#define GETTIMEINTERVAL 60L	/* every minute gettimeofday */
#define REREGISTERTIME 400L	/* interval to reregister with Infoproc
                                 * Infoproc will disconnect if not
                                 * reregistered every 600 seconds
                                 */

#include "ACQPROC_strucs.h"

int debug = 0;
int canvasOn = 0;
int acq_ok;
int StartY;
int VTpan,VTpan1,VTpan2,SPNpan2;
int  pPid = 0; /* parent pid */
int useInfostat = -1;
int StatPortId = -1;

#define HOSTLEN 40
#define FRAME_WIDTH  44
char User[HOSTLEN];
char LocalHost[HOSTLEN];
char RemoteHost[HOSTLEN];
char *graphics;
int  Procpid;
long PresentTime = GETTIMEINTERVAL + 1;
long IntervalTime;
static char statlogdir[256]="/vnmr/status/logs";
static int logging=0;


/*  local_addr_list and local_addr are required to support local_entry.
    You must create a new copy of the h_addr_list and the h_addr fields
    (see netdb.h); else when you call gethostbyname again, these will
    be replaced, even if you make a separate copy of the hostent itself.  */

struct hostent	 local_entry;
static char	*local_addr_list[ 2 ] = { NULL, NULL };
static int	 local_addr;
static void create_Statuspanel();
static void initDvals();
static int setup_signal_handlers();
void DoTheChores(int sig);

void setLogFilePath(char *path);

AcqStatBlock  CurrentStatBlock;

#ifndef TRUE
#define	TRUE	1
#define	FALSE   !TRUE
#endif

void sleepMilliSeconds(int msecs)
{
   struct timespec req;
   req.tv_sec = msecs / 1000;
   req.tv_nsec = (msecs % 1000) * 1000000;
   nanosleep( &req, NULL);
}

static int input_fd = -1;

void register_input_event(int fd)
{
   input_fd = fd;
}


void acqstat_window_loop()
{
   int  res;
   fd_set  rfds;

   while (1)
    {
       if (input_fd == -1)
       {
//          DoTheChores();
          sleepMilliSeconds(IntervalTime*1000);
       }
       else
       {
       FD_ZERO( &rfds );
       FD_SET( input_fd, &rfds);
       res = select(input_fd+1, &rfds, 0, 0, 0);
       if (res > 0)
       {
          int ch;
          if (FD_ISSET(input_fd, &rfds) )
             DoTheChores(0);
       }
       }
    }
}

/*------------------------------------------------------------------
|
|
|
+------------------------------------------------------------------*/
int main(int argc, char *argv[])
{
	int n;
	int pid;
	int ival;
	char *d_opt = "-debug";
	struct passwd *pasinfo;
	struct hostent *local_hp;

	if (strstr(argv[0], "Infostat"))
		useInfostat = 0;
	RemoteHost[0] = '\0'; /* null name */
	for (n = 1; n < argc; n++) {
		if (!strcmp("-debug", argv[n]))
			debug = 1;
		else if (!strcmp("-l", argv[n]))
			logging = 1;
		else if (!strcmp("-log", argv[n]) && n<argc-1){
			logging = 1;
			n++;
			strcpy(statlogdir,argv[n]);
		}
		else if (!strcmp("-port", argv[n])) {
			if (useInfostat == 0) {
				n++;
				if (n < argc)
					if (strlen(argv[n]) > 0)
						if (atoi(argv[n]))
							StatPortId = atoi(argv[n]);
			}
		} else if (!strcmp("-d", argv[n])) {
			if (n == argc - 1 || argv[n + 1][0] == '-') {
				argv[n] = d_opt;
				debug = 1;
			}
		} else if ((argv[n][0] == '-') && (argv[n][1] == 'P')) {
			char *ptr;
			ptr = argv[n];
			ptr += 2;
			pPid = atoi(ptr);
		} else if (argv[n][0] == '-') {
			if (argv[n + 1] != NULL && argv[n + 1][0] != '-')
				n++;
		} else
			strcpy(RemoteHost, argv[n]);
	}
	if (pPid == 0) {
		pPid = getppid();
		if (pPid == -1)
			pPid = 0;
	}

	if(logging>0){
	    setLogFilePath(statlogdir);
	}
	else
	    setLogFilePath("");

	if (useInfostat < 0) {

		/* Disassociate from Control Terminal */
		pid = fork();
		if (debug)
			if (pid > 0)
				fprintf(stderr, "Acqstat PID: %d\n", pid);

		if (pid != 0)
			exit(0); /* parent process */
	}

	if (useInfostat < 0 /* && StatPortId<0 */)
		for (n = 3; n < 20; n++) /* close any inherited open file descriptors */
			close(n);

	freopen("/dev/null", "r", stdin);
	if (debug == 0 && useInfostat < 0 /* && StatPortId<0 */) {
		freopen("/dev/console", "a", stdout);
		freopen("/dev/console", "a", stderr);
	}

	ival = setsid(); /* the setsid program will disconnect from */
	/* controlling terminal and create a new */
	/* process group, with this process as the leader */
#ifdef SIGTTOU
	signal(SIGTTOU, SIG_IGN);
#endif
	setup_signal_handlers();

	if (useInfostat == 0 && StatPortId < 0 && !logging)
		fprintf(stderr, "****** starting Infostat ******\n");

	if (debug)
		fprintf(stderr, "requested RemoteHost:'%s'\n", RemoteHost);

	acq_ok = 0;
	if (useInfostat == 0) {
		graphics = "sun";
	} else {
		graphics = "sun";
	}

	/* get process id */
	Procpid = getpid();
	if (debug)
		fprintf(stderr, "Process ID: %d\n", Procpid);

	/* get Host machine name */
	gethostname(LocalHost, sizeof(LocalHost));
	if (debug)
		fprintf(stderr, "Local Host Name: %s\n", LocalHost);
	if (useInfostat == 0 && StatPortId < 0 && !logging)
		fprintf(stderr, "Local Host Name: %s\n", LocalHost);

	local_hp = gethostbyname(LocalHost);
	if (local_hp->h_length > sizeof(int)) {
		fprintf(stderr,
				"programming error, size of host address is %d, expected %d\n",
				local_hp->h_length, sizeof(int));
		exit(1);
	}
	memcpy(&local_entry, local_hp, sizeof(local_entry));
	memcpy(&local_addr, local_entry.h_addr, local_entry.h_length );
	local_entry.h_addr_list = &local_addr_list[0];
	local_entry.h_addr= (char *) &local_addr;

	/*  WARNING:  any address fields in local_entry not explicitly set
	 may be reset the next time you call gethostbyname.	*/

	if (RemoteHost[0] == '\0')
		strcpy(RemoteHost, LocalHost);
	/* --- get user's name --- */
	/*        get the password file information for user */
	pasinfo = getpwuid((int) getuid());
	strcpy(User, pasinfo->pw_name); /* Store user name */
	if (debug)
		fprintf(stderr, "User Name: %s\n", User);

	if (useInfostat == 0) {
		if (StatPortId > 0) {
			if ((strcmp(LocalHost, "") != 0) && (StatPortId > 0)) {
				initStatSocket();
			}
		} else if (debug) {
			fprintf(stderr, "Requested RemoteHost: '%s'\n", RemoteHost);
		}
		if (debug) {
			if (StatPortId < 0)
				fprintf(stderr, "StatPortId = %d: invalid port id\n",
						StatPortId);
			else
				fprintf(stderr, "StatPortId = %d: port id ok\n", StatPortId);
		}
	}

	/* create socket for RPC communication */
	/**
	 if ( (strcmp(RemoteHost,LocalHost) != NULL) && (RemoteHost[0] != NULL) )
	 {
	 if (initrpctcp(RemoteHost) == -1)
	 exit(1);
	 }
	 **/
#ifdef MOTIF
	create_Bframe(argc, argv);
	create_Statuspanel();
#endif
#ifdef USE_RPC
	if ((strcmp(RemoteHost, LocalHost)) && (RemoteHost[0] != '\0'))
		initrpctcp(RemoteHost);
#endif
	/* to obtain acq. status info */
	DoTheChores(0);
#ifdef MOTIF
	load_proc();
#endif
	acqstat_window_loop();
}

/*-------------------------------------------------------------------
|
|	DoTheChores()
|       1. Update present time, just add interval time, then every min
|		get actual timeofday.
|	2. check interactive input time, for inactivity then disconnect
|	3. get the acquisition status information and update
|	     the status screen
+-------------------------------------------------------------------*/
void DoTheChores(int sig)
{
	struct timeval clock;
	static long lastgettime = 0L;
	static long lastregister = 0L;
	extern void exitproc();
	char buffer[256];
	int ret;
        static int first = 1;

	alarm(0);
	if (pPid != 0) {
		if ((kill(pPid, 0) == -1) && (errno == ESRCH)) {
			exitproc();
		}
	}
	if (PresentTime - lastgettime > GETTIMEINTERVAL) {
		gettimeofday(&clock, NULL); /* get time of connect */
		PresentTime = lastgettime = clock.tv_sec;
	} else
		PresentTime += IntervalTime;

	if (debug)
		fprintf(stderr, "DoTheChores acq_ok = %d\n", acq_ok);
	if (acq_ok) {
		AcqStatBlock statbuf; /* acq update status buffer */

		if (debug)
			fprintf(stderr, "DoTheChores updating screen\n");
		if ((ret = readacqstatblock(&statbuf)) > 0) {
			if (statbuf.Acqstate <= ACQ_INACTIVE)
				initDvals();
			else {
				if (useInfostat == 0 && StatPortId < 0 && !logging){
	                gettimeofday(&clock, NULL); /* get time of connect */
	                time_t curtime=clock.tv_sec;
	                struct tm* ptm=localtime (&curtime);
	                strftime(buffer,sizeof(buffer)-1,"%m-%d-%Y %T",ptm);
					fprintf(stderr, "****** update status ****** %s\n",buffer);
				}
				updatestatscrn(&statbuf);
			}
		} else if (ret <= -1)
			acq_ok = Acqproc_ok(RemoteHost);
		if (debug)
			fprintf(stderr, " result = %d acq_ok= %d\n", ret, acq_ok);
		if (!acq_ok)
                {
			initDvals();
                    first = 0;
                }
		else if (PresentTime - lastregister > REREGISTERTIME) {
			reregister();
			lastregister = PresentTime;
		}
//		inittimer(2.0, 2.0, DoTheChores);
		if (acq_ok)
		   IntervalTime = 5;
                else
		   IntervalTime = 1;
                alarm(IntervalTime);
	} else {
                if (first)
		   initDvals();
                first = 0;
		acq_ok = initIPCinfo(RemoteHost);
		if (acq_ok) {
			if (debug)
				fprintf(stderr, "DoTheChores initializing socket\n");
			initsocket();
			acqregister(); /* register status port with acquisition */
			lastregister = PresentTime;
		} else if (useInfostat == 0) {
			char *istat;
			istat = (char *) getenv("infostatautoexit");
			if (istat != NULL)
				if (istat[0] == 'y') {
					if (StatPortId < 0 && !logging)
						fprintf(stderr,
								"Acqproc does not appear to be active on %s\n",
								RemoteHost);
					else
						writestatToVnmrJ("acqproc", "inactive");
					exit(1);
				}
		}
		if (acq_ok) {
			IntervalTime = 5;
		} else {
			IntervalTime = 2;
		}
                alarm(IntervalTime);
	}
}
/*--------------------------------------------------------------------------
|
|	Queuedisp()/0
|
+-------------------------------------------------------------------------*/
Queuedisp()
{
}



#ifdef MOTIF
/*------------------------------------------------------------------
|
|	create_Statuspanel()
|	create the base frame and command panel of buttons for 
|	the acquisition status display
+------------------------------------------------------------------*/
static void create_Statuspanel()
{
        create_disp_panel();

/* all items must be created in order */
        create_item( StatusTitle, StatX, StatY, "STATUS:",0 ,0);
        create_item( StatusVal, StatX, StatY," ", 19, 0);
	create_item( PendTitle, PendX, PendY, "QUEUED:",0, 0);
        create_item( PendVal, PendX, PendY, " ", 8, 0);
	create_item( UserTitle, UserX, UserY, "USER:",0, 0 );
        create_item( UserVal, UserX, UserY, " ",7, 0);
	create_item( ExpTitle, ExpX, ExpY, "EXP:",0, 0);
        create_item( ExpVal, ExpX, ExpY, " ", 8, 0);
	create_item( SampleTitle, SampX, SampY, "SAMPLE:",0, 0);
        create_item( SampleVal, SampX, SampY, " ", 7, 0);
	create_item( ArrayTitle, ArrayX, ArrayY, "FID:",0, 0);
        create_item( ArrayVal, ArrayX, ArrayY, " ", 8, 0);
	create_item( CT_Title, CTX, CTY, "CT:",0, 0 );
        create_item( CT_Val, CTX, CTY, " ", 9, 0);
	create_item( DecTitle, DecX, DecY, "DEC:",0, 0);
        create_item( DecVal, DecX, DecY, " ", 11, 0);
	create_item( CompTitle, CompX, CompY,"Completion Time:",0, 0);
        create_item( CompVal, CompX, CompY, " ", 10, 0);
	create_item( LockTitle, LockX, LockY, "LOCK:",0, 0);
        create_item( LockVal, LockX, LockY, " ", 9, 0);
	create_item( RemainTitle, RemainX, RemainY, "Remaining Time:",0, 0);
        create_item( RemainVal, RemainX, RemainY, " ", 10, 0);
	create_item( LKTitle, LKX, LKY, "level:",0, 0);
        create_item( LKVal, LKX, LKY, " ", 9, 0);
	create_item( DataTitle, DataX, DataY, "Data Stored at:",0, 0);
        create_item( DataVal, DataX, DataY, " ", 10, 0);
	create_item( SpinTitle, SpinX, SpinY, "SPINNER:",0, 0);
        create_item( SpinVal, SpinX, SpinY, " ", 12, 0);
	create_item( VT_Title, VTX, VTY, "VT:",0, 1);
        create_item( VT_Val, VTX, VTY, " ", 11, 0);
	VTpan = 1;	/* VT title is displayed */
	create_item( SPNTitle, SPNX, SPNY, "Actual:",0, 0);
        create_item( SPNVal, SPNX, SPNY, " ", 10, 0);
	create_item( VTTitle, VTX1, VTY1, "Actual:",0, 1);
        create_item( VTVal, VTX1, VTY1, " ", 10, 0);
	VTpan1 = 1;     /* VT title is displayed */
	create_item( SPN2Title, SPNX2, SPNY2, "Setting:",0, 0);
        create_item( SPNVal2, SPNX2, SPNY2, " ", 8, 0);
	SPNpan2 = 1;    /* VT title is displayed */
	create_item( VT2Title, VTX2, VTY2, "Setting:",0, 1);
        create_item( VTVal2, VTX2, VTY2, " ", 10, 0);
	VTpan2 = 1;     /* VT title is displayed */
	create_item( AIRTitle, AIRX, AIRY, "Air:",0, 1);
        create_item( AIRVal, AIRX, AIRY, " ", 12, 0);

	fit_win_height();

        create_item( RFMonTitle, RFMonLimX, RFMonLimY, "RF Monitor:", 0, 0);
        create_item( RFMonVal, RFMonLimX, RFMonLimY, " ", 5, 0);

        create_item( RF10s1AvgTitle, RF10s1AvgX, RF10s1AvgY,
                     "Ch1 pwr: 10s:", 0, 1);
        create_item( RF10s1AvgVal, RF10s1AvgX, RF10s1AvgY, " ", 4, 0);
        create_item( RF5m1AvgTitle, RF5m1AvgX, RF5m1AvgY, "5m:", 0, 0);
        create_item( RF5m1AvgVal, RF5m1AvgX, RF5m1AvgY, " ", 4, 0);

        create_item( RF10s2AvgTitle, RF10s2AvgX, RF10s2AvgY,
                     "Ch2 pwr: 10s:", 0, 0);
        create_item( RF10s2AvgVal, RF10s2AvgX, RF10s2AvgY, " ", 4, 0);
        create_item( RF5m2AvgTitle, RF5m2AvgX, RF5m2AvgY, "5m:", 0, 0);
        create_item( RF5m2AvgVal, RF5m2AvgX, RF5m2AvgY, " ", 4, 0);

        create_item( RF10s3AvgTitle, RF10s3AvgX, RF10s3AvgY,
                     "Ch3 pwr: 10s:", 0, 1);
        create_item( RF10s3AvgVal, RF10s3AvgX, RF10s3AvgY, " ", 4, 0);
        create_item( RF5m3AvgTitle, RF5m3AvgX, RF5m3AvgY, "5m:", 0, 1);
        create_item( RF5m3AvgVal, RF5m3AvgX, RF5m3AvgY, " ", 4, 0);

        create_item( RF10s4AvgTitle, RF10s4AvgX, RF10s4AvgY,
                     "Ch4 pwr: 10s:", 0, 0);
        create_item( RF10s4AvgVal, RF10s4AvgX, RF10s4AvgY, " ", 4, 0);
        create_item( RF5m4AvgTitle, RF5m4AvgX, RF5m4AvgY, "5m:", 0, 0);
        create_item( RF5m4AvgVal, RF5m4AvgX, RF5m4AvgY, " ", 4, 0);

        create_item( RF10s1LimTitle, RF10s1LimX, RF10s1LimY, "/", 0, 0);
        create_item( RF10s1LimVal, RF10s1LimX, RF10s1LimY, " ", 5, 0);
        create_item( RF5m1LimTitle, RF5m1LimX, RF5m1LimY, "/", 0, 0);
        create_item( RF5m1LimVal, RF5m1LimX, RF5m1LimY, " ", 5, 0);

        create_item( RF10s2LimTitle, RF10s2LimX, RF10s2LimY, "/", 0, 0);
        create_item( RF10s2LimVal, RF10s2LimX, RF10s2LimY, " ", 5, 0);
        create_item( RF5m2LimTitle, RF5m2LimX, RF5m2LimY, "/", 0, 0);
        create_item( RF5m2LimVal, RF5m2LimX, RF5m2LimY, " ", 5, 0);

        create_item( RF10s3LimTitle, RF10s3LimX, RF10s3LimY, "/", 0, 1);
        create_item( RF10s3LimVal, RF10s3LimX, RF10s3LimY, " ", 5, 0);
        create_item( RF5m3LimTitle, RF5m3LimX, RF5m3LimY, "/", 0, 1);
        create_item( RF5m3LimVal, RF5m3LimX, RF5m3LimY, " ", 5, 0);

        create_item( RF10s4LimTitle, RF10s4LimX, RF10s4LimY, "/", 0, 0);
        create_item( RF10s4LimVal, RF10s4LimX, RF10s4LimY, " ", 5, 0);
        create_item( RF5m4LimTitle, RF5m4LimX, RF5m4LimY, "/", 0, 0);
        create_item( RF5m4LimVal, RF5m4LimX, RF5m4LimY, " ", 5, 0);

        create_item( ProbeID1Title, 0, 15, " ", 5, 0);
        create_item( ProbeID1, 0, 15, " ", 5, 0);
        create_item( GradCoilIDTitle, 0, 15, " ", 5, 0);
        create_item( GradCoilID, 0, 15, " ", 5, 0);
        create_item( MASLimitTitle, 0, 15, " ", 5, 0);
        create_item( MASSpeedLimit, 0, 15, " ", 5, 0);
        create_item( MASSpanTitle, 0, 15, " ", 5, 0);
        create_item( MASBearSpan, 0, 15, " ", 5, 0);
        create_item( MASAdjTitle, 0, 15, " ", 5, 0);
        create_item( MASBearAdj, 0, 15, " ", 5, 0);
        create_item( MASMaxTitle, 0, 15, " ", 5, 0);
        create_item( MASBearMax, 0, 15, " ", 5, 0);
        create_item( MASActiveSPTitle, 0, 15, " ", 5, 0);
        create_item( MASActiveSetPoint, 0, 15, " ", 5, 0);
        create_item( MASProfileTitle, 0, 15, " ", 5, 0);
        create_item( MASProfileSetting, 0, 15, " ", 5, 0);

	setup_set_proc(0,0,0);
}
#endif
/*------------------------------------------------------------------
|
+------------------------------------------------------------------*/
static void initDvals()
{
    static AcqStatBlock  StatBlock;

    StatBlock.Acqstate = ACQ_INACTIVE;
    StatBlock.AcqExpInQue = 0;
    strcpy(StatBlock.AcqUserID,"");
    strcpy(StatBlock.AcqExpID,"");
    StatBlock.AcqFidElem = 0;
    StatBlock.AcqCT = 0;
    StatBlock.AcqLSDV = 0;
    StatBlock.AcqCmpltTime = 0L;
    StatBlock.AcqRemTime = 0L;
    StatBlock.AcqDataTime = 0L;
    StatBlock.AcqLockLevel = 0;
    StatBlock.AcqSpinSet = -1;
    StatBlock.AcqSpinAct = -1;
    StatBlock.AcqVTSet = 30000;
    StatBlock.AcqVTAct = 30000;
    StatBlock.AcqSample = -1;
    updatestatscrn(&StatBlock);
}


int
Wissun()
{
    static int retval = -1;

    if ( retval == -1 )
       if (strcmp(graphics,"sun")==0 || strcmp(graphics,"suncolor")==0)
          retval = 1;
       else
          retval = 0;
    return retval;
}

/*  following array MUST end in -1, or a Segmented Violation may occur.
    It lets the programmer encode a list of signals to be caught, all
    using the same signal handler.					*/

static int signum_array[] = { SIGHUP, SIGINT, SIGQUIT, SIGTERM, -1 };

static int setup_signal_handlers()
{
	struct sigaction	intserv;
	sigset_t		qmask;
	extern void		exitproc();
	int			iter, ival, signum;

	for (iter = 0; ( (signum = signum_array[ iter ]) != -1 ); iter++) {
		sigemptyset( &qmask );
		sigaddset( &qmask, signum );
		intserv.sa_handler = exitproc;
		intserv.sa_mask = qmask;
		intserv.sa_flags = 0;

		ival = sigaction( signum, &intserv, NULL );
	}
		sigemptyset( &qmask );
		sigaddset( &qmask, SIGIO );
		intserv.sa_handler = DoTheChores;
		intserv.sa_mask = qmask;
		intserv.sa_flags = 0;

		ival = sigaction( SIGALRM, &intserv, NULL );

	return( 0 );
}
