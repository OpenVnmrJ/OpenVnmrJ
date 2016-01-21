/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*
 * altered from the original statusscrn.c for SFU Infostat. 
 * got rid of X windows references, etc.    JGW  
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <netdb.h>
#include <pwd.h>
#include "ACQPROC_strucs.h"
#include "STAT_DEFS.h"

#define HOSTLEN 40
#define GETTIMEINTERVAL 60L     /* every minute gettimeofday */
#define REREGISTERTIME 400L     /* interval to reregister with Infoproc
                                 * Infoproc will disconnect if not
                                 * reregistered every 600 seconds
                                 */

#include <time.h>
time_t currentTime;


int debug = 0;
int acq_ok;
int pPid = 0;                   /* parent pid */
int useInfostat = -1;
int StatPortId = -1;

char *logfile = "/tmp/Infostat_stdout";
char User[HOSTLEN];
char LocalHost[HOSTLEN];
char RemoteHost[HOSTLEN];
int  Procpid;
long PresentTime = GETTIMEINTERVAL + 1;
long IntervalTime;

AcqStatBlock  CurrentStatBlock;

#ifdef __INTERIX
int     inputfd = -1;
int     VTpan;
int     VTpan1;
int     SPNpan2;
int     VTpan2;
fd_set  rfds;
#endif

void DoTheChores();
void inittimer(double, double, void (*) () );
void updatestatscrn(AcqStatBlock*);
void showInfostatus(AcqStatBlock);

static int setup_signal_handlers(void);
static int initDvals(void);


/*  local_addr_list and local_addr are required to support local_entry.
    You must create a new copy of the h_addr_list and the h_addr fields
    (see netdb.h); else when you call gethostbyname again, these will
    be replaced, even if you make a separate copy of the hostent itself.  */
struct hostent	 local_entry;
static char	*local_addr_list[ 2 ] = { NULL, NULL };
static int	 local_addr;

struct timeval select_timeout;

int main(int argc, char* argv[]) {
    int    n;
    int  pid;
    int ival,result;
    char  *d_opt = "-debug";
    struct passwd *getpwuid();
    struct passwd *pasinfo;
    struct hostent *local_hp;
    
    useInfostat = 0;
    
    RemoteHost[0] = '\0';    /* null name */

    // fclose(stdin);
    // freopen(logfile,"a",stdout);
    // freopen(logfile,"a",stderr);
    
    for(n = 1; n < argc; n++) {
        if ( !strcmp("-debug", argv[n])) {
            debug = 1;
        } else if ( !strcmp("-port", argv[n])) {
            if (useInfostat == 0) {
                n++;
                if (n < argc) {
                    if (strlen(argv[n]) > 0) {
                        if ( atoi(argv[n]) ) {
                            StatPortId =  atoi(argv[n]);
			    // fprintf(stderr,"PortId: %d\n",StatPortId);
			    // StatPortId = 0xFFFF & htons(StatPortId);
                        }
                    }
                }
            }
        } else if (!strcmp("-d", argv[n])) {
            if (n ==  argc-1 || argv[n+1][0] == '-') {
                argv[n] = d_opt;
                debug = 1;
            }
        } else if ((argv[n][0] == '-' ) && (argv[n][1] == 'P' )) {
            char *ptr;
            ptr = argv[n];
            ptr += 2;
            pPid = atoi(ptr);
        } else if (argv[n][0] == '-' ) {
            if (argv[n+1] != NULL && argv[n+1][0] != '-') {
                 n++;
            }
        } else {
             strcpy(RemoteHost,argv[n]);
        }
    }

    // debug = 1;

    if (debug < 1)
    {
       fclose(stdin);
       freopen("/dev/null","w",stdout);
       freopen("/dev/null","w",stderr);
    }
    if ( (debug > 0) && (StatPortId > 0) ) 
    {
       // freopen("/dev/null","r",stdin);
       fclose(stdin);
       freopen(logfile,"a",stdout);
       freopen(logfile,"a",stderr);
       // stdouterr_log = fopen(logfile,"a");
    }
    
    
    if (debug > 0)
    { 
       n = 0;
       while (argv[n] != NULL) {
           fprintf(stderr,"argv[%d] = %s\n", n, argv[n]);
           n++;
       }
       fprintf(stderr,"useInfostat = %d\n", useInfostat);
       fprintf(stderr,"RemoteHost = %s\n", RemoteHost);
       fprintf(stderr,"debug = %d\n", debug);
       fprintf(stderr,"pPid = %d (before getppid)\n", pPid);
    }
    
    if (pPid == 0) {
        pPid = getppid();
        if (pPid == -1) {
            pPid = 0;
        }
    }
    
    /* unfortunately in Interix a shell is used as an intermediary, thus the parent PID
     * is of the shell not the invoking program
    */
    if (debug > 0)
       fprintf(stderr,"pPid = %d (after getppid)\n", pPid);

    if ( useInfostat==0 && StatPortId > 0) 
    {
       ival = setsid();            /* the setsid program will disconnect from */
                                   /* controlling terminal and create a new */
                                   /* process group, with this process as the leader */
    }
    if (ival == -1)
       perror("setsid failed: ");

    setup_signal_handlers();   /* term pipe, etc. */

    if ( useInfostat==0 && StatPortId < 0) {
        fprintf(stderr, "****** starting Infostat ******\n");
    }

    if (debug > 0) {
        fprintf(stderr,"requested RemoteHost:'%s'\n",RemoteHost);
    }

    acq_ok = 0;

    /* get process id */
    Procpid = getpid();
    if (debug > 0) {
        fprintf(stderr,"Process ID: %d\n",Procpid);
    }
 
    /* get Host machine name */
    gethostname(LocalHost,sizeof(LocalHost));
    if (debug > 0) {
        fprintf(stderr,"Local Host Name: %s\n",LocalHost);
    }
    if (useInfostat==0 && StatPortId<0) {
        fprintf(stderr,"Local Host Name: %s\n",LocalHost);
    }

    if (debug > 0) {
       fprintf(stderr,"StatPortId = %d\n", StatPortId);
       fprintf(stderr,"LocalHost = %s\n", LocalHost);
       fprintf(stderr,"Procpid= %d\n", Procpid);
    }
    
    local_hp = gethostbyname( LocalHost );
    if (local_hp->h_length > sizeof(int) ) {
        fprintf(stderr, "programming error, size of host address is %d, expected %d\n",
                         local_hp->h_length, sizeof(int) );
        exit(1);
    }
    memcpy( &local_entry, local_hp, sizeof( local_entry ) );
    memcpy( &local_addr, local_entry.h_addr, local_entry.h_length );
    local_entry.h_addr_list = &local_addr_list[ 0 ];
    local_entry.h_addr = (char *) &local_addr;
 
/*  WARNING:  any address fields in local_entry not explicitly set
              may be reset the next time you call gethostbyname.        */

    if ( RemoteHost[0] == '\0' ) {
        strcpy(RemoteHost, LocalHost);
    }
    /* --- get user's name --- */
    /*        get the password file information for user */
    pasinfo = getpwuid((int) getuid());
    strcpy(User,pasinfo->pw_name); /* Store user name */
    if (debug > 0) {
        fprintf(stderr,"User Name: %s\n", User);
    }

    if (useInfostat == 0) {
        if (StatPortId > 0) {
            if ((strcmp(LocalHost,"") != 0) && (StatPortId > 0)) {
                /* fprintf(stderr,"JGW debug: calling initStatSocket()\n"); */
                /* fprintf(stderr,"JGW debug: LocalHost = %s\n", LocalHost); */
                initStatSocket();
            }
        } else {
            fprintf(stderr,"requested RemoteHost: '%s'\n",RemoteHost);
        }
        if (debug > 0) {
            if (StatPortId < 0) {
                fprintf(stderr,"StatPortId = %d: invalid port id\n",StatPortId);
            } else {
                fprintf(stderr,"StatPortId = %d: port id ok\n",StatPortId);
            }
        }
    }

    // legacy flags in statdispfuncs
    VTpan = 1;
    VTpan1 = 1;
    SPNpan2 = 1;
    VTpan2 = 1;

    /* since acq_ok == 0, this call will initalize values & communication to Infoproc */
    DoTheChores();


    if (inputfd < 0) 
    { 
        inputfd = -1;
        acq_ok = 0; 
    }
    
    /* timeout for 10 seconds */
    select_timeout.tv_sec = (time_t) 10;
    select_timeout.tv_usec = 0;
    while (1) 
    {
        if (inputfd>0) 
        {   // might be re-set in DoTheChores
            FD_ZERO( &rfds );
            FD_SET( inputfd, &rfds);
            result = select(inputfd+1, &rfds, 0, 0, &select_timeout);
	    // fprintf(stderr,"select return: %d\n",result);
            if (result == 0)
            {
		// fprintf(stderr,"select timeout....\n");
		acq_ok = 0;
                DoTheChores();
            }
            if (result == -1)
            {
	        perror("select error: ");
		acq_ok = 0;
                // DoTheChores();
            }
            if (result > 0) 
            {
               if (FD_ISSET(inputfd, &rfds) ) {
		   // fprintf(stderr,"FD_ISSET returned: call DoTheChores()\n");
                   DoTheChores();
               }
            }
        }
        else
        {
            FD_ZERO( &rfds );
            result = select(1, &rfds, 0, 0, &select_timeout);
            acq_ok = 0; 
            DoTheChores();
	}
    }
    
}

/*-------------------------------------------------------------------
|
|       DoTheChores()
|       1. Update present time, just add interval time, then every min
|               get actual timeofday.
|       2. check interactive input time, for inactivity then disconnect
|       3. get the acquisition status information and update
|            the status screen
+-------------------------------------------------------------------*/
void DoTheChores() {
    struct timeval clock;
    struct timezone tzone;
    static long lastgettime = 0L;
    static long lastregister = 0L;
    extern void exitproc();
    int ret;
    
    // fprintf(stderr,"DEBUG jgw: DoTheChores called, acq_ok = %d\n", acq_ok);

    //if (pPid != 0) {
    //    if ((kill(pPid,0) == -1) && (errno == ESRCH)) {
    //        exitproc();
    //    }
    //}
    
    if (debug) {
        fprintf(stderr, "DoTheChores acq_ok = %d\n", acq_ok);
    }
    
    if (acq_ok) {
        AcqStatBlock statbuf;    /* acq update status buffer */

        if (debug) {
            fprintf(stderr,"DoTheChores updating screen\n");
        }

        if ( (ret = readacqstatblock(&statbuf)) > 0 ) {
            if (debug > 0)
               fprintf(stderr,"Infostat: DoTheChores %d bytes, Acqstate = %d\n", ret, statbuf.Acqstate);
            if (statbuf.Acqstate <= ACQ_INACTIVE) {
                initDvals();
            } else {
                if (useInfostat==0 && StatPortId<0) {
                    fprintf(stderr, "****** update status screen ******\n");
                 }
                 updatestatscrn(&statbuf);
            }
        } else if (ret <= -1) {
            acq_ok = Acqproc_ok(RemoteHost);
            if (debug > 0)
	        fprintf(stderr," read failed, check Infoproc, there? : %d\n",acq_ok);
        }
        if (!acq_ok) {
            initDvals();
        } 
    } else {
        initDvals();
        acq_ok = initIPCinfo(RemoteHost);
        fprintf(stderr,"JGW debug: initIPCinfo() returned %d\n", acq_ok);
        if (acq_ok) {
            if (debug) {
                fprintf(stderr,"DoTheChores initializing socket\n");
            }
            initsocket();
            #ifdef __INTERIX
            inputfd = acqregister();
            fprintf(stderr,"DEBUG jgw: inputfd = %d\n", inputfd);
            #else
            acqregister();        /* register status port with acquisition */
            #endif
            lastregister = PresentTime;
        }
    }
}


/*------------------------------------------------------------------
|
+------------------------------------------------------------------*/
static initDvals() {
    static AcqStatBlock  StatBlock;
    
    fprintf(stderr,"JGW debug: in initDvals()\n");

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


/*  following array MUST end in -1, or a Segmented Violation may occur.
    It lets the programmer encode a list of signals to be caught, all
    using the same signal handler.                                      */

static int signum_array[] = { SIGHUP, SIGINT, SIGQUIT, SIGTERM, -1 };

static int setup_signal_handlers() {
        struct sigaction        intserv;
        sigset_t                qmask;
        extern void             exitproc(int);
        int                     iter, ival, signum;

        for (iter = 0; ( (signum = signum_array[ iter ]) != -1 ); iter++) {
                sigemptyset( &qmask );
                sigaddset( &qmask, signum );
                intserv.sa_handler = exitproc;
                intserv.sa_mask = qmask;
                intserv.sa_flags = 0;

                ival = sigaction( signum, &intserv, NULL );
        }
        return( 0 );
}

void (*timerfunc) () = NULL;

void inittimer(double timsec, double intvl, void (*funccall)() ) {
    unsigned long  msec;

    timerfunc = funccall;
    //if(timerId != NULL)
    //{
    //     XtRemoveTimeOut(timerId);
    //     timerId = NULL;
    //}
    //msec = timsec * 1000;  /* milliseconds */
    //if(funccall != NULL)
    //     timerId = XtAddTimeOut(msec, timerproc, NULL);
}

int Wissun() {
    return 1;
}

int set_item_string(int item, char* str) {
    return 0;
}

int show_item(int item, int on) {
    return 0;
}



