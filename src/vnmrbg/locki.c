/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <pwd.h>
#include <inttypes.h>
#ifndef LINUX
#include <X11/Intrinsic.h>
#endif

#include "data.h"
#include "group.h"
#include "ACQ_SUN.h"
#include "vnmrsys.h"
#include "shrexpinfo.h"
#include "expQfuncs.h"
#include "mfileObj.h"
#include "shrstatinfo.h"
#include "acqcmds.h"
#include "graphics.h"
#include "allocate.h"
#include "pvars.h"
#include "buttons.h"
#include "wjunk.h"

#ifndef ACQI_EXPERIMENT
#define  ACQI_EXPERIMENT  "0"
#endif
#ifndef HOSTLEN
#define HOSTLEN  128
#endif

#define  DELIMITER_2	'\n'
#define  DELIMITER_3	','

#define NDC_LOCKDATA_SHIFTCONST (4)
#define TIMER_OFF (0.0)
#define TIMER_TRY (0.1)
#define TIMER_RETRY (0.05)

#define COMPLETE 	0
#define ERROR 		1

#ifndef FALSE
#define FALSE           (0)
#define TRUE            (! FALSE)
#endif

extern int send_transparent_data( c68int buf[], int len );
extern int getAcqProcParam(char *thisProgramName );
extern int talk2Acq4Acqi(char *cmd_for_acq, char *msg_for_acq,
                         char *msg_for_vnmr, int mfv_len );
extern int release_console();
extern int acqproc_msge(int argc, char *argv[], int retc, char *retv[]);
extern int getparm(char *varname, char *vartype, int tree,
                   void *varaddr, int size);
extern int  nvAcquisition();
extern void register_child_exit_func(int signum, int pid,
             void (*func_exit)(), void (*func_kill)() );
extern int  is_datastation();
extern int is_acqproc_active();
extern char *append_char4Acqi(char *string, char character);
extern int cmpTimeStamp( TIMESTAMP *ts1, TIMESTAMP *ts2 );
extern void verifyExpproc();
extern int getStatLSDV();
extern int getStatRecvGain();
extern void     sleepMilliSeconds(int msecs);

#ifdef VNMRJ
extern int is_new_java_graphics();
#endif

static MFILE_ID ifile = NULL;
static TIMESTAMP currentTimeStamp;
static int dspGainBits = 0;
static int neg_ct_count;
static int  new_java_graphics = 0;
static char userName[HOSTLEN] = { '\0' };
static char hostName[HOSTLEN] = { '\0' };
static char procName[HOSTLEN] = { '\0' };
static char processIDstr[12]  = { '\0' };
static char addr[MAXPATHL];
static int ulen;

static int acqi_can_acquire = TRUE;
static int signal_avg = FALSE;
/*static int first_stat = TRUE;*/

static struct ybar *out[2];
static struct ybar *imgout[2]; /* for imaginary data */
static int ybarLen = 0;
static int ybarW = 0;
static int ybarH = 0;
static int old;
static int new;
static int lock_first;
static int acqi_connected = FALSE;
static float LKlevel = 0.0;
static FID_STAT_BLOCK *fidstataddr;
static unsigned long fidId = 0;

static struct {
	long	NumTrans;
	long	ctcount;
	long	elemid;
} prevFidStats;


int Procpid;			/* This process's PID */
static int nvlockiPid = 0;	/* nvlocki PID */
static int verbose = 0;
static int vtest = 0;
static int lockiFd = -1;
static int standBy = 0;
static int nvlockActive = 0;

#define START 1
#define STOP  2
#define DISP  3
#define LKDEBUG 4
#define READY 5
#define TEST  6
#define NEXT  7
#define QUIT  8
#define EXIT  9
#define STANDBY  10

#define CMDNUM  10

static char *nvCmds[] = { "start", "stop", "display", "debug", "ready", "test",
		"next", "quit", "exit", "standby" };


static void  outline_box();
static int  start_lockdisplay();
       int  start_lockexp();
       int  stop_lockexp();
       void next_lockdisplay(int shift, int amp, int phase);
static void drawIndicator(int shift, int amp, int phase);
       void acqconnect();
       int  stop_acqi(int);
       void shrmemToStatblk(struct ia_stat *statblk, int valid_data );
static void setAcqiTimeConst();
static void setAcqiStatus();
static void setLockRateAndTimeConst();
static void setDefaultRateAndTimeConst();

static int  recvInteractData( char *dbuf, size_t dsize );
static int  isAcqiDataCurrent();
static int  check4ConsoleError();
static void sortdata(int *iptr, int *optr, int size, int *sum);
static int recvfmHAL(void *dataspace, size_t datalen);
static void get_LKdata();
#ifndef LINUX
static void (*lock_timer_func)();
static XtIntervalId  lock_timer_id = 0;

static void
lock_timer_proc(XtPointer dum1, XtIntervalId *dum2)
{
    (void) dum1;
    (void) dum2;
    if(lock_timer_id == 0)
	return;
    lock_timer_id = 0;
    if (lock_timer_func != NULL)
	lock_timer_func();
}

static void
lock_timer_init(double timsec, void (*funccall)() )
{
    unsigned long  msec;

    if(lock_timer_id != 0)
    {
	XtRemoveTimeOut(lock_timer_id);
	lock_timer_id = 0;
    }
    lock_timer_func = funccall;
    if(funccall != NULL)
    {
	msec = timsec * 1000;  /* milliseconds */
	lock_timer_id = XtAddTimeOut(msec, lock_timer_proc, NULL);
    }
}

/*
 *  end_lockdisplay()
 */
static void
end_lockdisplay()
{
    lock_timer_init(TIMER_OFF, NULL);
/**
    if (out[0]) {
	free(out[0]);
	out[0] = 0L;
    }
    if (out[1]) {
	free(out[1]);
	out[1] = 0L;
    }
**/
}
#endif
void lock_get_fid()
{
   int cmd_argc;
   char *cmd_argvec[ 3 ];
	cmd_argvec[ 0 ] = "acqipctst";
	cmd_argvec[ 1 ] = "locki('data')";
	cmd_argvec[ 2 ] = NULL;
	cmd_argc	= 2;
        sleepMilliSeconds(100);
	acqproc_msge(cmd_argc, &cmd_argvec[ 0 ], 0, NULL);
}

/* when Vnmrbg exit, it may call this func to kill nvlocki */
static void
kill_nvlocki(int pid)
{
    if (verbose)
        fprintf(stderr, " kill locki pid %d \n", pid);
    if (pid > 0 && (pid == nvlockiPid)) {
	if (lockiFd >= 0) {
       int ret __attribute__((unused));
       ret = write(lockiFd, "quit", 4);
	    close (lockiFd);
	}
        nvlockiPid = 0;
        lockiFd = -1;
    }
    if (nvlockiPid > 0) {
	kill(nvlockiPid, SIGKILL);
        nvlockiPid = 0;
    }
}

/* when nvlocki exit, this func will be called */
static void
clear_nvlocki(int pid)
{
    if (verbose)
        fprintf(stderr, " clear locki pid %d \n", pid);
    if (pid > 0 && (pid == nvlockiPid)) {
	nvlockiPid = 0;
	acqi_can_acquire = FALSE;
        if (lockiFd >= 0)
	    close(lockiFd);
	lockiFd = -1;
        if (ifile != NULL) {
	    mClose(ifile);
	    ifile = NULL;
	}
    }
    if (nvlockiPid < 1) {
	if (out[0] != NULL) free(out[0]);
	if (out[1] != NULL) free(out[1]);
	if (imgout[0] != NULL) free(imgout[0]);
	if (imgout[1] != NULL) free(imgout[1]);
	out[0] = NULL;      out[1] = NULL;
	imgout[0] = NULL;   imgout[1] = NULL;
	ybarW = 0;
        ybarLen = 0;
    }
}

static void
sort_nvdata(iptr, optr, size, sum)
short *iptr;
int   *optr, size, *sum;
{
    int i;
    size /= 2;
    *sum = 0;
    for ( i=0; i < size; i++,iptr++)
    {
        *sum -= *iptr;                  /* calculate negative sum */
        *optr++ = *iptr++;              /* move every other word */
    }
}


// if SFU talks to native windows, do not use mmap use regular file i/o
#ifdef NO_WIN_MMAP
static void disp_nvdata() {
    char lockFilePath[256];
    FILE* lockDataFile;
    int readResponse;
    short sData[1024];
    double abuffer[1024];
    int len;
    char   buf[12];
    int     i, sum;
    long    lkdata[1024];

    free(fidstataddr);
    fidstataddr = malloc(sizeof(FID_STAT_BLOCK));

    sprintf(lockFilePath, "%s/acqqueue/nvlocki.Data", systemdir);
#ifdef VNMRJ
    new_java_graphics = is_new_java_graphics();
#endif

    lockDataFile = fopen(lockFilePath, "r");
    if (lockDataFile == NULL) {
        fprintf(stderr, "problem opening lock data file\n");
        return;
    }

    readResponse = fread(fidstataddr, sizeof(FID_STAT_BLOCK), 1, lockDataFile);

    if (readResponse != 1) {
        fprintf(stderr, "problem reading lock data file\n");
        fclose(lockDataFile);
        return;
    }

    len = (int) fidstataddr->dataSize;
    fidId = fidstataddr->elemId;
    if (len < 8 || len > 1024) {
	return;
    }

    readResponse = fread(sData, sizeof(short), len, lockDataFile);

    if (readResponse != len) {
        fprintf(stderr, "problem reading lock data\n");
        fclose(lockDataFile);
        return;
    }

    Wgetgraphicsdisplay(buf, 10);
    if (strcmp(buf, "locki") != 0) {
	// end_lockdisplay();
	sun_window();
        if (new_java_graphics == 0) {
	    sunGraphClear();
        }
	start_lockdisplay();
	Wsetgraphicsdisplay("locki");
    }

    if ((mnumxpnts != ybarW) || mnumypnts != ybarH)  {
        if (new_java_graphics == 0)
	    sunGraphClear();
	start_lockdisplay();
    }
    if (out[new] == NULL)
	return;
    normalmode();
    if (new_java_graphics > 0) {
        grf_batch(1);
        sunGraphClear();
    }
    color(SCALE_COLOR);
    outline_box(0, 0, mnumxpnts-1, mnumypnts - 1);

    for (i=0; i< 12; i++)
	lkdata[i]=0;
    sort_nvdata(sData, lkdata+10, len, &sum);
/*
    sum >>= NDC_LOCKDATA_SHIFTCONST;
*/
    for (i=0;i < 512;i++) {
      lkdata[i] = (long)-(( ((float)(lkdata[i] >> NDC_LOCKDATA_SHIFTCONST) + 1024)/2048.0) * (mnumypnts * 2.0 / 3.0));
    }
    expand32(&lkdata[0], 130, out[new], mnumxpnts, (mnumypnts * 2) / 3);
    /* calculate imaginary date */
    if (imgout[new] != NULL) {
        for (i=0; i< 12; i++)
	    lkdata[i]=0;
        sort_nvdata(sData+1, lkdata+10, len, &sum);
        for (i=0;i < 512;i++) {
         lkdata[i] = (long)-(( ((float)(lkdata[i] >> NDC_LOCKDATA_SHIFTCONST)
		 + 1024)/2048.0) * (mnumypnts * 2.0 / 3.0));
	}
        expand32(&lkdata[0], 130, imgout[new], mnumxpnts, (mnumypnts * 2) / 3);
    }
#ifndef __INTERIX
    next_lockdisplay();
#endif
    fclose(lockDataFile);
}
#else
#include <math.h>
void next_lockdisplay(int shift, int amp, int phase);
static void
disp_nvdata()
{
    int    len;
    char   buf[12];
    short  *sData;
    int     i, sum;
    int    lkdata[1024];        /* a few extra for the "porch" */
    double x,y,z,dz,adz,zold,zsum,qsum;
    if (nvlockiPid <= 0)
	return;
    if (ifile == NULL) {
       sprintf(addr, "%s/acqqueue/nvlocki.Data", systemdir);
       i = sizeof( short ) * 1024;
       sum = sizeof( TIMESTAMP ) + sizeof( FID_STAT_BLOCK ) + i;
       ifile = mOpen( addr, sum, O_RDONLY );
       if (ifile == NULL) {
           if (verbose)
		fprintf(stderr, "VBG: couldn't open %s \n", addr);
	   return;
       }
    }
    fidstataddr = (FID_STAT_BLOCK *)(ifile->mapStrtAddr + sizeof(TIMESTAMP));
    if (verbose) {
	fprintf(stderr, "VBG: display locki data \n");
        len = (int) fidstataddr->dataSize;
	fprintf(stderr, " Data size %d \n", len);
        sData = (short *) fidstataddr + sizeof( FID_STAT_BLOCK );
	fprintf(stderr, "fid: ");
        if (len > 16)
	   len = 16;
        for (i = 0; i < len; i++) {
	    fprintf(stderr, "%d ", *sData);
	    sData++;
	}
	fprintf(stderr, " \n");
    }
    len = (int) fidstataddr->dataSize;
    if (len < 8 || len > 1024)
	return;
#ifdef VNMRJ
    new_java_graphics = is_new_java_graphics();
#endif
    sData = (short *) fidstataddr + sizeof( FID_STAT_BLOCK );
/*
    if (fidId == fidstataddr->elemId)
	return;
*/
    fidId = fidstataddr->elemId;
    Wgetgraphicsdisplay(buf, 10);
    if (strcmp(buf, "locki") != 0) {
	// end_lockdisplay();
	sun_window();
        if (new_java_graphics == 0)
	   sunGraphClear();
	start_lockdisplay();
	Wsetgraphicsdisplay("locki");
    }
    if ((mnumxpnts != ybarW) || mnumypnts != ybarH)  {
        if (new_java_graphics == 0)
	    sunGraphClear();
	start_lockdisplay();
    }
    if (out[new] == NULL)
	return;
    normalmode();
    if (new_java_graphics > 0) {
        grf_batch(1);
        sunGraphClear();
    }
    color(SCALE_COLOR);
    outline_box(0, 0, mnumxpnts-1, mnumypnts - 1);
/***** data 4 phil *****/
    zsum = 0.0;
    zold = 0.0;
    qsum = 0.0;
    adz = 0.0;
    for (i = 0;  i < len; i++)
    {
       x = (double) sData[2*i];
       y = (double) sData[2*i+1];
       z = atan2(y,x);
       if ((i > 29) && (i < 120))
       {
     /*    zsum += z; */
	 qsum += x*x + y*y;
       }
       if (i == 0) dz = 0.0;
       else
          dz = z - zold;
       if (dz > M_PI) dz -= 2*M_PI;
       if (dz < -1.0*M_PI) dz += 2*M_PI;
       zold = z;
       if (i > 40)   /** why 40 **/
         adz += dz;
    }
    /*
    zsum += 314.0-60.0;
    if (zsum > 314.0) zsum -= 628.0;
    */


    for (i=0; i< 12; i++)
	lkdata[i]=0;
    sort_nvdata(sData, lkdata+10, len, &sum);
    LKlevel = (float) sum / 48000.0;
/*
    sum >>= NDC_LOCKDATA_SHIFTCONST;
*/
    for (i=0;i < 512;i++) {
      lkdata[i] = (long)-(( ((float)(lkdata[i] >> NDC_LOCKDATA_SHIFTCONST) + 1024)/2048.0) * (mnumypnts * 2.0 / 3.0));
    }
    expand32(&lkdata[0], 130, out[new], mnumxpnts, (mnumypnts * 2) / 3);
    /* calculate imaginary date */
    if (imgout[new] != NULL) {
        for (i=0; i< 12; i++)
	    lkdata[i]=0;
        sort_nvdata(sData+1, lkdata+10, len, &sum);
        for (i=0;i < 512;i++) {
         lkdata[i] = (int)-(( ((float)(lkdata[i] >> NDC_LOCKDATA_SHIFTCONST)
		 + 1024)/2048.0) * (mnumypnts * 2.0 / 3.0));
	}
        expand32(&lkdata[0], 130, imgout[new], mnumxpnts, (mnumypnts * 2) / 3);
    }

    adz *= 4.0;
    next_lockdisplay((int) adz,(int) log10(qsum), (int) zsum);
}
#endif // NO_WIN_MMAP

static int
run_nvlocki()
{
    int     child, toKill;
    char    pfd[12];

    int pipeFd[2];

    if (nvlockiPid > 0) {
	toKill = 0;
	if ( (kill(nvlockiPid, 0) == -1) && (errno == ESRCH)) {
	   if (verbose)
	     fprintf(stderr, " nvlocki pid %d  no respond, kill it  \n", nvlockiPid);
	   toKill = 1;
	}
	else if (lockiFd < 0)
	   toKill = 1;
	if (toKill) {
	   if (verbose)
	     fprintf(stderr, " kill old nvlocki pid %d \n", nvlockiPid);
	   if (lockiFd >= 0)
	      close(lockiFd);
	   lockiFd = -1;
           if (ifile != NULL) {
	      mClose(ifile);
	      ifile = NULL;
	   }
	   kill(nvlockiPid, SIGKILL);
           nvlockiPid = 0;
	}
	else {
      int ret __attribute__((unused));
      ret = write(lockiFd, "start", 5);
	   if (verbose)
	      fprintf(stderr, " nvlocki pid %d  is active. \n", nvlockiPid);
	   return (1);
	}
    }
    if (lockiFd >= 0) {
	close(lockiFd);
	lockiFd = -1;
    }
    if (pipe(pipeFd) < 0) {
        Werrprintf("locki: couldn't open pipe.");
	return (0);
    }

    if (getparm("vnmraddr", "STRING", GLOBAL, addr, MAXPATHL-1)) {
        Werrprintf("locki: couldn't get vnmraddr");
	return (0);
    }

    child = fork();
    if (child != 0) {  /* this process */
	nvlockiPid = child;
	register_child_exit_func(SIGCHLD, child, clear_nvlocki, kill_nvlocki);
        lockiFd = pipeFd[1];
	close(pipeFd[0]);
	if (verbose)
	   fprintf(stderr, "nvlocki pid %d  pipe %d addr %s\n", nvlockiPid, lockiFd, addr);
    }
    else { /* child process */
	close(pipeFd[1]);
	sprintf(pfd, "%d", pipeFd[0]);
	#ifdef WINBRIDGE
        sprintf(procName, "%s/bin/nvlocki.exe", systemdir);
    #else
        sprintf(procName, "%s/bin/nvlocki", systemdir);
    #endif
	if (verbose) {
	  if (vtest)
	    execlp(procName, "-debug", "-test", "-pipe", pfd, "-addr", addr, NULL);
	  else {
	    if (standBy)
	        execlp(procName, "-standby", "-debug", "-pipe", pfd, "-addr", addr, NULL);
	    else
	        execlp(procName, "-debug", "-pipe", pfd, "-addr", addr, NULL);
	  }
	}
	else {
	  if (standBy)
	      execlp(procName, "-standby", "-pipe", pfd, "-addr", addr, NULL);
	  else
	      execlp(procName, "-pipe", pfd, "-addr", addr, NULL);
	}
	exit(1);
    }
    return (1);
}

void quit_nvlocki()
{
   int tmp;

   tmp = nvlockiPid;
   if (lockiFd >= 0)
   {
      int ret __attribute__((unused));
      ret = write(lockiFd, "quit", 4);
      close(lockiFd);
      lockiFd = -1;
   }
   if ( (tmp > 0) && nvlockActive)
   {
      setDefaultRateAndTimeConst();
   }
   nvlockActive = 0;
   verbose = 0;
   vtest = 0;
   P_getstring(GLOBAL,"acqmode",addr,1, 12);
   if (strcmp(addr,"lock") == 0)
   {
      P_setstring(GLOBAL,"acqmode","",0);
      appendvarlist("acqmode");
      sunGraphClear();
      Wsetgraphicsdisplay("");
   }
}

void stop_nvlocki()
{
   int ret __attribute__((unused));
#ifdef WINBRIDGE      // following line added jgw 27 sep 07
   setDefaultRateAndTimeConst();
#endif
   if (nvlockiPid > 0) {
      if (lockiFd >= 0)
          ret = write(lockiFd, "stop", 4);
#ifndef WINBRIDGE      // jgw 27 sep 07
      if (nvlockActive)
#endif
         setDefaultRateAndTimeConst();
   }
   else if (lockiFd >= 0) {
      close(lockiFd);
      lockiFd = -1;
   }
   nvlockActive = 0;
   P_getstring(GLOBAL,"acqmode",addr,1, 12);
   if (strcmp(addr,"lock") == 0)
   {
      P_setstring(GLOBAL,"acqmode","",0);
      appendvarlist("acqmode");
      sunGraphClear();
      Wsetgraphicsdisplay("");
   }
}

static void
nvlocki(int argc, char **argv)
{
    int  k, i, cmd, toStart, toStop, toDisplay;
    int  doNext, debug, btest, toQuit;

    toStop = 0;
    toStart = 0;
    debug = 0;
    btest = 0;
    doNext = 0;
    toDisplay = 0;
    toQuit = 0;
    standBy = 0;
    for (k = 0; k < argc; k++) {
	cmd = 0;
        if (verbose)
	    fprintf(stderr, "%s ", argv[k]);
        for (i= 0; i < CMDNUM; i++) {
            if (strcasecmp(argv[k], nvCmds[i]) == 0) {
		cmd = i + 1;
		break;
	    }
	}
	switch (cmd) {
	   case START:
	    	toStop = 0;
	    	toQuit = 0;
	   	standBy = 0;
	    	toStart = 1;
		break;
	   case STOP:
	    	toStop = 1;
	    	toStart = 0;
	   	standBy = 0;
		break;
	   case QUIT:
	   case EXIT:
	    	toQuit = 1;
	    	toStart = 0;
	   	standBy = 0;
		break;
	   case DISP:
	    	toDisplay = 0;
	    	toStop = 0;
		break;
	   case LKDEBUG:
	    	debug = 1;
		break;
	   case READY:
	    	toDisplay = 1;
		break;
	   case TEST:
	    	btest = 1;
		break;
	   case NEXT:
		doNext = 1;
		break;
	   case STANDBY:
	    	toStop = 0;
	    	toQuit = 0;
	   	toStart = 0;
	   	standBy = 1;
		break;
	}
    }
    if (verbose)
        fprintf(stderr, " \n");

    if (toQuit) {
        quit_nvlocki();
	return;
    }
    if (toStart || standBy) {
	fidId = 0;
        verbose = debug;
        vtest = btest;
	if (run_nvlocki()) {
    	    lock_first = TRUE;
	    if (toStart) {
	       sun_window();
               sunGraphClear();
	       Wsetgraphicsdisplay("locki");
	       P_setstring(GLOBAL,"acqmode","lock",0);
               appendvarlist("acqmode");
               setLockRateAndTimeConst();
	    }
	}
	if (toStart)
        {
	    start_lockdisplay();
            nvlockActive = 1;
        }
	return;
    }
    if (toStop) {
        stop_nvlocki();
	return;
    }
    if (toDisplay) {
        if (nvlockActive)
	   disp_nvdata();
	return;
    }
    if (doNext) {
	if (lockiFd >= 0) {
      int ret __attribute__((unused));
	   ret = write(lockiFd, "fake", 4);
	}
    }
}

int
locki(int argc, char **argv, int retc, char **retv)
{
    struct ia_stat  *iaptr;
    char  iablock[1536];

    (void) retc;
    (void) retv;
    if (nvlockiPid < 1) {
       if (is_datastation()) {
	   Werrprintf("Cannot run %s on a data station", argv[0]);
	   RETURN;
       }
    }
    if (!is_acqproc_active())
    {
        Werrprintf("The acquisition system is not active.");
        RETURN;
    }
    if ((nvlockiPid > 0) || nvAcquisition() )
    {
	nvlocki(argc, argv);
        RETURN;
    }
    if (argc == 2 && strcasecmp(argv[1], "start") == 0) {
	acqconnect();
	if (start_lockexp() != 0) {
	    /* NB Release resources; see LKshow() */ /*CMP*/
	    appendvarlist("acqmode");
	    ABORT;
	}
	iaptr = (struct ia_stat *)iablock;
	shrmemToStatblk(iaptr, LK_STRUC);
	acqi_connected = TRUE;
	P_setstring(GLOBAL,"acqmode","lock",0);
	appendvarlist("acqmode");
	/*sunGraphClear();
	  start_lockdisplay();*/
    } else if (argc == 2 && strcasecmp(argv[1], "stop") == 0) {
	stop_acqi( 0 );
    } else if (argc == 2 && strcasecmp(argv[1], "data") == 0) {
        if (acqi_connected)
            get_LKdata();
    } else if ( acqi_connected && (argc == 2) &&
                strcasecmp(argv[1], "display") == 0) {
	/* end_lockdisplay(); */
	sun_window();
	sunGraphClear();
	start_lockdisplay();
	Wsetgraphicsdisplay("locki");
	get_LKdata();
    }
    RETURN;
}

int stop_acqi( int abortall )
{
    int rtn = acqi_connected;
    char tmp[MAXPATH];
    P_getstring(GLOBAL,"acqmode",tmp,1, 12);
    if (strcmp(tmp,"lock") == 0)
    { if (stop_lockexp()) {
	P_setstring(GLOBAL,"acqmode","",0);
	appendvarlist("acqmode");
	/* end_lockdisplay(); */
	Wclear_graphics();
	Wsetgraphicsdisplay("");
      }
    }
    else if (strcmp(tmp,"fidscan") == 0)
    { P_setstring(GLOBAL,"acqmode","",0);
      Wturnoff_buttons();
      Wsetgraphicsdisplay("");
      execString("menu('main')\n");
      appendvarlist("acqmode");
      P_setstring(CURRENT,"wbs","",0);
      if (abortall > 0)
      {
	char *cmd_argvec[ 2 ];

	cmd_argvec[ 0 ] = "aa";
	cmd_argvec[ 1 ] = NULL;
	acqproc_msge(1, &cmd_argvec[ 0 ], 0, NULL);
/*	if (abortall > 1)
	  sleep(1); */ /* acq must stop before starting another acq */
      }
    }

    return rtn;
}

/*------------------------------------------------------------------
|  start_lockdisplay()
|  add addition 10 data points to malloc space since there is a round
|  off error in rene's expand routine, thus can write out too many points
|  This the main reason why the lockdisplay would crash all the time
+-------------------------------------------------------------*/
static int start_lockdisplay()
{
    int k;

    /*canvas_open = 1;*/
    /* allocate memory for ybar buffers */
    if (ybarLen <= mnumxpnts) {
	if (out[0] != NULL) free(out[0]);
	if (out[1] != NULL) free(out[1]);
	if (imgout[0] != NULL) free(imgout[0]);
	if (imgout[1] != NULL) free(imgout[1]);
	imgout[0] = NULL;
	imgout[1] = NULL;
	ybarLen = mnumxpnts + 10;
	k = ybarLen * sizeof(struct ybar);
        out[0] = (struct ybar *) malloc(k);
        out[1] = (struct ybar *) malloc(k);
#ifndef WINBRIDGE // jgw 1 oct 07
        if (nvlockiPid > 0) {
#endif
           imgout[0] = (struct ybar *) malloc(k);
           imgout[1] = (struct ybar *) malloc(k);
	   if (imgout[0] == NULL || imgout[1] == NULL) {
		if (imgout[0] != NULL) free(imgout[0]);
		if (imgout[1] != NULL) free(imgout[1]);
		imgout[0] = NULL;
		imgout[1] = NULL;
	   }
#ifndef WINBRIDGE // jgw 1 oct 07
	}
#endif
    }
    if ((out[0]== NULL) || (out[1]== NULL))
    { /*  DPRINT0("cannot allocate memory for ybar buffer\n"); */
	if (out[0] != NULL) free (out[0]);
	if (out[1] != NULL) free (out[1]);
	out[0] = NULL;
	out[1] = NULL;
	return 1;
    }
    old = 0;
    new = 1;
    ybarH = mnumypnts;
    ybarW = mnumxpnts;
    lock_first = TRUE;
    LKlevel = 0.0;
    normalmode();
    color(SCALE_COLOR);
    outline_box(0, -1, mnumxpnts-1, mnumypnts);
    /*  DPRINT1("xcharpixels = %d\n",xcharpixels); */
    return 0;
}


static void
outline_box(xmin, ymin, width, height)
     int xmin, ymin, width, height;
{
    int xmax = xmin + width;
    int ymax = ymin + height;
    amove(xmin,ymin);
    adraw(xmax,ymin);
    adraw(xmax,ymax);
    adraw(xmin,ymax);
    adraw(xmin,ymin);
}
#define BOXX 15
#define BOXY 20
/* improved indicator .. 4 boxes 
 * 1 - large neg good signal
 * 2 - close neg or poor amplitude
 * 3 - close pos or poor amplitude
 * 4 - large pos with good signal
 */
static void drawIndicator(int fshift, int amp, int phase)
{
     int leftc1,leftc2, rightc1,rightc2, xbase, ybase;
     char buff[128];

     (void) phase;
     leftc1 = WHITE; rightc1 = WHITE;
     leftc2 = WHITE; rightc2 = WHITE;
     ybase = BOXY;
     xbase = mnumxpnts/2 - 2*BOXX;
     sprintf(buff,"shift = %d  amp = %d\n",fshift,amp); 
     if (fshift > 1) rightc1 = GREEN;
     if ((fshift > 12) && (amp > 6)) rightc2 = ORANGE-1;
     if (fshift < -1) leftc1 = ORANGE;
     if ((fshift < -12) && (amp > 6)) leftc2 = ORANGE+1;
     amove(xbase,ybase); // lower left
     color(leftc2);
     box(BOXX,BOXY);
     amove(xbase+BOXX,ybase);
     color(leftc1);
     box(BOXX,BOXY);
     amove(xbase+2*BOXX,ybase);
     color(rightc1);
     box(BOXX,BOXY);
     amove(xbase+3*BOXX,ybase);
     color(rightc2);
     box(BOXX,BOXY);
     /* label */
     color(BLACK);
     amove(xbase+BOXX-2,ybase+3);
     dchar('-');
     amove(xbase+3*BOXX-2, ybase+3);
     dchar('+');
     amove(xbase,ybase);
     rdraw(4*BOXX,0);
     rdraw(0,BOXY);
     rdraw(-4*BOXX,0);
     rdraw(0,-BOXY);
     amove(xbase,ybase+BOXY+5);
     dstring("Z0 Direction ");
     /* dstring(buff);  */
}

static void
get_LKdata()
{
    char buf[32];
    struct  ia_stat *statptr;
    int    data[512];
    struct ia_stat statblk;
    char    lock_string[80];
    int    lkdata[512];
    int     iter,sum;
    int     ival __attribute__((unused));
    float   level;

    Wgetgraphicsdisplay(buf, sizeof(buf));
    if (strcmp(buf, "locki") != 0) {
	sun_window();
	sunGraphClear();
	start_lockdisplay();
	Wsetgraphicsdisplay("locki");
    }

    statptr = &statblk;
    shrmemToStatblk(statptr, LK_STRUC);

    if ( recvfmHAL(data,sizeof(data)) == -1)  /* get lock data from HAL */
    {
	fprintf(stderr,"get_LKdata(): recvfmHAL error.\n");
	/*disconnect();
	enable_disconnect();*/
        return;
    }

#ifdef XXX
    if (!isAcqiDataCurrent())
    {
       int i = 0;
       sleepMilliSeconds(120);
       while ( !isAcqiDataCurrent() && (i < 10) )
       {
          sleepMilliSeconds(10);
           i++;
       }
       if (i >= 10)
       {
           lock_get_fid();
	/*enable_disconnect();*/
	   return;
       }
    }
#endif
    if (!isAcqiDataCurrent())
    {
           lock_get_fid();
	/*enable_disconnect();*/
	   return;
    }
    if (check4ConsoleError() != 0) {
	/*storeConsoleError();
	disconnect();
	SendConsoleError();
	enable_disconnect();*/
	return;
    }

    ival = recvInteractData((char *)data, sizeof(data));

    if (statptr->valid_data) {
	/*DPRINT1("valid data = %d\n",statptr->valid_data);*/
        /*change_count = 0;*/
	/*lock_states(statptr);*/
    }

    sortdata(data, lkdata, 512, &sum);
    sum >>= NDC_LOCKDATA_SHIFTCONST;

#ifdef EIGHTEENBIT_ADC
    sortdata(&data[512], lkdata, 256, &sum);	/* 18-bit ADC */
#endif

#if 0
    {
       int i;
       for (i=65;i<70;i++) {
	   Wscrprintf("lkdata[%d]=%d\n", i, lkdata[i]);
       }
    }
#endif

    if (out[new] == NULL || ybarLen <= mnumxpnts)
	return;

    /* Put zero level 1/3 of the way up the screen, full-scale at top. */
    for (iter=0;iter<512;iter++) {
	lkdata[iter] = (int)-(( ((float)(lkdata[iter] >> NDC_LOCKDATA_SHIFTCONST) + 1024)/2048.0) * (mnumypnts * 2.0 / 3.0));
    }
    expand32(&lkdata[10], 130, out[new], mnumxpnts, (mnumypnts * 2) / 3);

#ifdef EIGHTEENBIT_ADC
    /*	18-bit ADC	*/
    for (i=0;i<256;i++)
    {
	lkdata[i] = (c68int) -(
			       ( ((float)(lkdata[i]) + 1024)/2048.0) * (float)(mnumypnts/2));
    }
    expand(&lkdata[15],110,out[new],mnumxpnts,mnumypnts/2);
#endif


#if 0
    {
       struct  ybar *ptr;
       ptr = out[new];
       for (i=65;i<70;i++) {
	   Wscrprintf("out[%d]: min=%d, max=%d\n", i, ptr->mn, ptr->mx);
	   ptr++;
       }
    }
#endif

    next_lockdisplay(0,0,0);

    level = (float) sum / 3950.0;
#ifdef EIGHTEENBIT_ADC
    level = (float) sum / 1975.0;	/* 18-bit ADC */
#endif

    sprintf(lock_string," lock level = %6.1f ",level);

#ifdef NOTDEFINED
#ifdef X11
    {
       int ipos;
       normalmode();
       ipos = (graf_width-charWidth*21) / 2;
       amove(ipos, mnumypnts - charHeight - 5);
       color(WHITE);
       box(charWidth * 21, charHeight+2);
       dispw_text(ipos, charHeight, lock_string);
    }
#else
    dispw_text(graf_width/2-80, charHeight + 5, lock_string);
#endif

    /*enable_disconnect();*/
#endif
    lock_get_fid();
}

/*
 *  next_lockdisplay()
 */
void next_lockdisplay(int shift, int amp, int phase)
{
    int z;

    if (new_java_graphics > 0) {
        lock_first = TRUE;
    }
    else {
        grf_batch(1);
        xormode();
    }

    color(IMAG_COLOR);

    if (lock_first) {
	ybars(0, mnumxpnts-1, out[new], 0, mnumypnts-2, 1);
    } else {
	ybars(0, mnumxpnts-1, out[old],  0, 0, 0);
	ybars(0, mnumxpnts-1, out[new], 0, mnumypnts-2, 1);
    }
#ifndef WINBRIDGE // jgw 1 oct 07
    if (nvlockiPid > 0 && imgout[new] != NULL) {
#endif
        color(FID_COLOR);
        if (lock_first) {
           if (LKlevel > 15.0)
	      ybars(0, mnumxpnts-1, imgout[new], 0, mnumypnts-2, 1);
	}
	else {
	   ybars(0, mnumxpnts-1, imgout[old],  0, 0, 0);
	   ybars(0, mnumxpnts-1, imgout[new], 0, mnumypnts-2, 1);
	}
#ifndef WINBRIDGE // jgw 1 oct 07
    }
#endif
#ifndef OUT
    if (nvlockiPid > 0)
       drawIndicator(shift, amp, phase);
#endif
    lock_first = FALSE;
    grf_batch(0);
    normalmode();
    z = old;
    old = new;
    new = z;
}

#ifdef XXX
/*
 *  LKcanvas_repaint
 */
static int
LKcanvas_repaint()
{
    /*setdisplay();
    end_lockdisplay();*/
    sunGraphClear();
    /*start_lockdisplay();*/
}
#endif

/*
 *	sort the real - imaginary to reals then imaginary
 */
static void
sortdata(int *iptr, int *optr, int size, int *sum)
{
    int i;
    size /= 2;
    *sum = 0;
    for (i=0; i < size; i++,iptr++)
    {
        *sum -= *iptr;			/* calculate negative sum */
	*optr++ = *iptr++;		/* move every other word */
    }
}

/*  Although the New Digital Console has no HAL device, it is still useful
 *  for recvfmHAL to zero out the data space it was called with.		 */
static int recvfmHAL(void *dataspace, size_t datalen)
{
    memset(dataspace, 0, datalen);
    return 0;
}

static void initIPCinfo( char *programName )
{
    int		 limitIndex, processID;
    struct passwd	*pasinfo;

    /* --- get user's name --- */
    /*        get the password file information for user */

    (void) programName;
    Procpid = getpid();

    limitIndex = sizeof( userName ) - 1;
    pasinfo = getpwuid((int) getuid());
    if ((int) strlen( pasinfo->pw_name ) > limitIndex) {
	strncpy( &userName[ 0 ], pasinfo->pw_name, limitIndex );
	userName[ limitIndex ] = '\0';
    }
    else
	strcat(&userName[ 0 ], pasinfo->pw_name);
    ulen = strlen( &userName[ 0 ] );

    limitIndex = sizeof( hostName ) - 1;
    gethostname( &hostName[ 0 ], limitIndex );
    hostName[ limitIndex ] = '\0';

    strcpy( &procName[ 0 ], "acqi" );

    processID = getpid();
    sprintf( &processIDstr[ 0 ], "%d", processID );

    getAcqProcParam( "acqi" );
}

static char *
buildAuthParam()
{
    char	*retaddr;
    int	 plen;

    if (userName[ 0 ] == '\0' || hostName[ 0 ] == '\0') {
	initIPCinfo( "acqi" );
    }
    plen = strlen( &userName[ 0 ] ) +
	strlen( &hostName[ 0 ] ) +
	strlen( &processIDstr[ 0 ] ) +
	2 + 1;

    retaddr = (char *) allocateWithId( plen, "ipc" );
    if (retaddr == NULL)
	return( NULL );

    strcpy( retaddr, &userName[ 0 ] );
    append_char4Acqi( retaddr, DELIMITER_3 );
    strcat( retaddr, &hostName[ 0 ] );
    append_char4Acqi( retaddr, DELIMITER_3 );
    strcat( retaddr, &processIDstr[ 0 ] );

    return( retaddr );
}

static int
insertAuth(msg_for_acq, mfa_len)
     char *msg_for_acq;
     int mfa_len;
{
    int authLen, iter, mfa_current_len;
    char *authInfo, *tptrstart, *tptrend;

    mfa_current_len = strlen(msg_for_acq);
    authInfo = buildAuthParam();
    authLen = strlen(authInfo);

    if (mfa_current_len + authLen + 1 >= mfa_len) {
	release(authInfo);
	return -1;
    }

    tptrstart = msg_for_acq + mfa_current_len;
    tptrend = msg_for_acq + mfa_current_len + authLen + 1;
    for (iter = 0; iter < mfa_current_len+1; iter++)
	*(tptrend--) = *(tptrstart--);

    strncpy( msg_for_acq, authInfo, authLen+1 );
    msg_for_acq[ authLen ] = DELIMITER_2;

    release(authInfo);
    return 0;
}


#ifdef XXX
#define MAX_RESERVE_CONSOLE_TRIES 3
static int
reserveConsole(char *level)
{
    int	iter, ival;
    char NDCcommand[122], expprocReply[256];

    strcpy( &NDCcommand[ 0 ], "acquire" );
    insertAuth(&NDCcommand[0], sizeof(NDCcommand));

    for (iter = 0; iter < MAX_RESERVE_CONSOLE_TRIES; iter++) {
	ival = talk2Acq4Acqi("reserveConsole",
			     &NDCcommand[0],
			     &expprocReply[0],
			     sizeof(expprocReply));
	if (ival != 0)
	    return(-1);

	if (strcmp( &expprocReply[0], "BUSY") == 0) {
	    sleep(1);
	    continue;
	} else if (strcmp(&expprocReply[0], "OK") == 0) {
	    return 0;		/*  Here is the successful return.  */
	} else {
	    return -1;
	}
    }
    return -1;			/* Never got unbusy; didn't work  */
}
#undef  MAX_RESERVE_CONSOLE_TRIES

static void
check_console()
{
	/*TODO*/

	char NDCcommand[122], expprocReply[256];

	/*lock_timer_init_2(0.0,NULL);*/
	if (acqi_can_acquire)
	    return;
	if (reserveConsole("acquire") != 0)
	{
	    /*lock_timer_init_2(5.0,check_console);*/
	}
	else
	{
	    acqi_can_acquire = TRUE;
	    /*enable_shim_panel();*/
	    strcpy( &NDCcommand[ 0 ], "acquire" );
            insertAuth( &NDCcommand[ 0 ], sizeof( NDCcommand ) );
            talk2Acq4Acqi( "releaseConsole", &NDCcommand[ 0 ],
		 &expprocReply[ 0 ], sizeof( expprocReply ));
	}
}
#endif

static int
can_acqi_acquire()
{
    return acqi_can_acquire;
}

static int
signal_avg_on()
{
    return signal_avg;
}



/*  Please consider the values returned by these programs
    to be const values and do not alter them.			*/

char *
ipcGetUserName()
{
	if (userName[ 0 ] == '\0')
	  initIPCinfo( "acqi" );

	return( &userName[ 0 ] );
}

char *ipcGetHostName()
{
	if (hostName[ 0 ] == '\0')
	  initIPCinfo( "acqi" );

	return( &hostName[ 0 ] );
}

char *
ipcGetProcName()
{
	if (procName[ 0 ] == '\0')
	  initIPCinfo( "acqi" );

	return( &procName[ 0 ] );
}

/*
 * The New Digital Console starts the lock by preparing a Queue Entry
 * and submitting it to Expproc.  We use the High Priority Queue to
 * designate it as an interactive experiment.  In normal operations
 * the Expproc may say NO.  For example, you can be in shim display
 * and be refused lock display, since shim display is permitted during
 * a non-interactive acquisition whereas lock display is not.  This
 * contrasts with the Gemini and HAL based systems, where ACQI could
 * expect to have free reign once Acqproc turned over the console to
 * it.
 */

int
start_lockexp()
{
    char		tmpfilename[ 2 * MAXPATH ];
    char		systemdir[ MAXPATH ];
    char		params4cmd[ 122 ];
    char		expproc_reply[ 256 ];
    char		expinfo[ 16 ];
    int		fd;
    unsigned long	*iaddr;
    SHR_EXP_INFO 	lockentry;
    SHR_EXP_STRUCT      lockStruct;
    int ret __attribute__((unused));

    if (P_getstring(GLOBAL,"systemdir",systemdir,1,MAXPATH))
	strcpy(systemdir,"/vnmr");
    lockentry = &lockStruct;

    memset( lockentry, 0, sizeof( *lockentry ) );

    lockentry->DataPtSize = sizeof( int );
    lockentry->NumDataPts = 512;
    lockentry->FidSize = lockentry->DataPtSize * lockentry->NumDataPts;

    lockentry->DataSize = sizeof( TIMESTAMP ) + sizeof( FID_STAT_BLOCK ) +
	lockentry->DataPtSize * lockentry->NumDataPts;
    lockentry->NumFids = 1;	/* Number of Fids (arraydim) */
    lockentry->NumTrans = 1;	/* Number of Transients (NT) */
    strcpy(lockentry->UsrDirFile, getenv("vnmruser"));
    strcpy(lockentry->UserName, ipcGetUserName());

    snprintf(lockentry->DataFile,EXPINFO_STR_SIZE,
	    "%.32s/acqqueue/%.192s.Data", systemdir, ipcGetProcName());
    lockentry->ExpNum = 0;	/* Experiment # to perform processing in */
    lockentry->GoFlag = ACQI_LOCK;
    lockentry->InteractiveFlag = 1;
    lockentry->Codefile[0] = '\0';
    lockentry->RTParmFile[0] = '\0';
    lockentry->TableFile[0] = '\0';
    lockentry->WaveFormFile[0] = '\0';
    lockentry->GradFile[0] = '\0';
    dspGainBits = 0;


    initExpQs( 0 );
    sprintf( tmpfilename, "%s/acqqueue/%s.Info",
	     systemdir, ipcGetProcName());

    fd = open( &tmpfilename[ 0 ], O_RDWR | O_CREAT | O_TRUNC, 0666 );
    ret = write( fd, lockentry, sizeof( *lockentry ) );
    close( fd );

    strcpy( &expinfo[ 0 ], ACQI_EXPERIMENT );
    strcat( &expinfo[ 0 ], " " );
    strcat( &expinfo[ 0 ], ipcGetUserName() );

    expQaddToTail( HIGHPRIO, &tmpfilename[ 0 ], &expinfo[ 0 ] );
    params4cmd[ 0 ] = '\0';
    insertAuth( &params4cmd[ 0 ], sizeof( params4cmd ) );
    talk2Acq4Acqi("startInteract",
		  &params4cmd[ 0 ],
		  &expproc_reply[ 0 ],
		  sizeof( expproc_reply )
		  );
    if (strcmp( &expproc_reply[ 0 ], "NO" ) == 0) {
	release_console();
	return( -1 );
    }

    ifile = mOpen( &lockentry->DataFile[ 0 ], lockentry->DataSize, O_RDONLY );
    if (ifile == NULL) {
	fprintf( stderr, "can't access data in shared memory %s\n", lockentry->DataFile );
	release_console();
	return( -1 );
    }

    iaddr = (unsigned long *) ifile->mapStrtAddr;
    currentTimeStamp = *(TIMESTAMP *) iaddr;

    return 0;
}

int
stop_lockexp()
{
    int rtn = FALSE;
    int i;
    char params4cmd[122], expproc_reply[256];

    if (acqi_connected) {
	params4cmd[0] = '\0';
	insertAuth(params4cmd, sizeof(params4cmd));
	i = talk2Acq4Acqi("stopInteract",
			  params4cmd,
			  expproc_reply,
			  sizeof(expproc_reply));
	rtn = (i == 0);

	strcpy( params4cmd, "shim");
	insertAuth(params4cmd, sizeof(params4cmd));
	i = talk2Acq4Acqi("releaseConsole",
			  params4cmd,
			  expproc_reply,
			  sizeof(expproc_reply));
	rtn = rtn && (i == 0);

	mClose(ifile);
	ifile = NULL;

	/*  Make sure no one tries to update real time parameters  */
	/*rtparsize = 0;
	  rtparsfile[0] = '\0';*/
	acqi_connected = FALSE;
    }
    return rtn;
}

/*  This program does not actually receive interactive data from the
    console.  It accesses the shared memory where the Expproc puts the
    data it receives from the console.  Previously isAcqiDataCurrent
    found this data to be current, so this program just copies it into
    the data space for FID display.  FID display uses a separate space
    (and not the shared memory directly) so it can modify this data if
    necessary and to help maintain compatibility with other version of
    ACQI.								*/

static int
recvInteractData( char *dbuf, size_t dsize )
{
    if (ifile == NULL)
	return( -1 );

    if (can_acqi_acquire()) {
	char	*interactDataAddr;
	size_t	 interactDataHdrSize;
	FID_STAT_BLOCK	*interacqHdrAddr;

	interactDataHdrSize = sizeof( TIMESTAMP ) + sizeof( FID_STAT_BLOCK );
	interacqHdrAddr = (FID_STAT_BLOCK *)( ifile->mapStrtAddr +
					      sizeof( TIMESTAMP ));

	interactDataAddr = ifile->mapStrtAddr + interactDataHdrSize;
	if (ifile->byteLen - interactDataHdrSize < dsize) {
	    if (ifile->byteLen < interactDataHdrSize)
		return( -1 );
	    else if (ifile->byteLen == interactDataHdrSize)
		return( 0 );
            dsize = ifile->byteLen - interactDataHdrSize;
	}
	if (signal_avg_on() && (interacqHdrAddr->ct > 0))
	    neg_ct_count = interacqHdrAddr->ct;

	/* A problem appeared when FIDscope was introduced, spikes in
	    the display.  It was caused by data being simultaneously
	    transferred into and out of the shared memory.  Data needs
	    to be transferred atomically as long words to prevent
	    this.  See recvInteract, in recvfuncs.c, SCCS category
	    recvproc for more information.

	    At one time I tried transferring the data here as atomic
	    long words, rather than using memcpy.  This has proven (so
	    far) to not be necessary.  If spikes do ever appear (and
	    after several months of product release that should be a
	    remote possibility), try doing the transfer here using
	    long word operation.  July 1996 ROL */

	memcpy( dbuf, interactDataAddr, dsize );
    }

    /*  FIDmonitor:  access to the current experiment data is already present.
	Set neg_ct_count for shrmemToStatblk's use. */

    else {
	size_t	 	 dataOffset;
#ifdef DEBUG_FIDMONITOR
	int		 iter;
	int		*laddr;
#endif
	char		*iaddr;
	dfilehead	*fhptr;
	dblockhead	*bhptr;

	iaddr = ifile->mapStrtAddr;
	fhptr = (dfilehead *) iaddr;
	bhptr = (dblockhead *) (iaddr + sizeof( dfilehead ) +
				(prevFidStats.elemid-1) * fhptr->bbytes);
	neg_ct_count = -(bhptr->ctcount);
	dataOffset = (sizeof( dfilehead ) +
		      (prevFidStats.elemid-1) * fhptr->bbytes +
		      sizeof( dblockhead ));
	iaddr += dataOffset;

	/* If for some reason the file is not large enough,
	   reduce the size of the transfer accordingly.	*/

	if (ifile->byteLen - dataOffset < dsize) {
	    if (ifile->byteLen < dataOffset)
		return( -1 );
	    else if (ifile->byteLen == dataOffset)
		return( 0 );
	    dsize = ifile->byteLen - dataOffset;
	}
	memcpy( dbuf, iaddr, dsize );

#ifdef DEBUG_FIDMONITOR
	laddr = (int *) iaddr;
	for (iter = 0; iter < 10; iter++)
	    fprintf( stderr, "%d ", *(laddr + iter) );
	fprintf( stderr, "\n" );
#endif
    }
    return( dsize );
}

static int
check4ConsoleError()
{
    if (ifile == NULL) {
	return FALSE;
    }
    fidstataddr = (FID_STAT_BLOCK *)(ifile->mapStrtAddr + sizeof(TIMESTAMP));
    if (fidstataddr->doneCode == HARD_ERROR) {
	return TRUE;
    }
    return FALSE;
}

static int
isAcqiDataCurrent()
{
    if (can_acqi_acquire()) {

	TIMESTAMP	tmpTimeStamp;

	if (ifile != NULL) {
	    tmpTimeStamp = *(TIMESTAMP *) ifile->mapStrtAddr;
	    if (cmpTimeStamp(
			     (TIMESTAMP *) ifile->mapStrtAddr,
			     &currentTimeStamp
			     ) > 0) {
		currentTimeStamp = tmpTimeStamp;
		return( 1 );
	    }
	    else
		return( 0 );
	}
	else
	    return( 0 );
    }
    else {
	char		*iaddr;
	int		 retval, statElem;
	dfilehead	*fhptr;
	dblockhead	*bhptr;

	retval = 0;
	iaddr = ifile->mapStrtAddr;
	fhptr = (dfilehead *) iaddr;

	/*  If on the last time through, the CT count reached NT, move on to the next element  */

	if (prevFidStats.ctcount >= prevFidStats.NumTrans) {
	    prevFidStats.ctcount = 0;	/* won't work for IL!! */
	    statElem = getStatElem();
	    prevFidStats.elemid++;
	    if (statElem > prevFidStats.elemid)
		prevFidStats.elemid = statElem;

	}

	/*  Access the Block Header for the current element.  */

	bhptr = (dblockhead *) (iaddr + sizeof( dfilehead ) +
				(prevFidStats.elemid-1) * fhptr->bbytes);

	if (bhptr->ctcount > prevFidStats.ctcount) {
	    prevFidStats.ctcount = bhptr->ctcount;
	    retval = 1;
	}

	return( retval );
    }
}

static void setAcqiTimeConst()
{
   c68int buf[ 4 ];

   buf[0] = LKTC;
   buf[1] = 0;
   send_transparent_data( buf, 2 );
}

static void setLockRateAndTimeConst()
{
   c68int buf[ 6 ];

   buf[0] = LKRATE;
   buf[1] = 20;
   buf[2] = LKTC;
   buf[3] = 0;
   send_transparent_data( buf, 4 );
}

static void setDefaultRateAndTimeConst()
{
   c68int buf[ 6 ];

   buf[0] = LKRATE;
   buf[1] = 2000;
   buf[2] = LKACQTC;
   buf[3] = 0;
   send_transparent_data( buf, 4 );
}

static void setAcqiStatus()
{
   c68int buf[ 4 ];

   buf[0] = SETSTATUS;
   buf[1] = ACQ_INTERACTIVE;
   send_transparent_data( buf, 2 );
}

#ifdef XXX
static void setDefaultStatus()
{
   c68int buf[ 4 ];

   buf[0] = SETSTATUS;
   buf[1] = ACQ_IDLE;
   send_transparent_data( buf, 2 );
}
#endif

void acqconnect()
{
    int	ival;
    int	ok2acquire;
    char NDCcommand[122], expprocReply[256];

    ok2acquire = TRUE;	/* it's OK to acquire unless informed otherwise */

    verifyExpproc();
    strcpy(NDCcommand, "shim");
    insertAuth(NDCcommand, sizeof(NDCcommand));
    ival = talk2Acq4Acqi("reserveConsole",
			 NDCcommand,
			 expprocReply,
			 sizeof(expprocReply));
    if (ival < 0) {
	return;
    } else if (strncmp(expprocReply, "ACQUIRING", strlen("ACQUIRING")) == 0) {
	/*
	 * If an acquisition is in progress, the Expproc may send back
	 * the name of the Experiment Information File, to assist FID
	 * monitor.  Use strncmp to avoid confusion.
	 */
	ok2acquire = FALSE;
    } else if (strcmp(expprocReply, "OK") != 0) {
	if (strcmp(expprocReply, "OK2") == 0) {  /* need an su? */
	    Werrprintf("Run \"su\" or \"go\" first");

	    /*
	     * Release access to the console, since Expproc reserves access
	     * even if an su has not been done.  After all, the request for
	     * access could be so an su can be performed.
	     */
	    strcpy(NDCcommand, "shim" );
	    insertAuth(NDCcommand, sizeof(NDCcommand));
	    ival = talk2Acq4Acqi("releaseConsole",
				 NDCcommand,
				 expprocReply,
				 sizeof(expprocReply));
	}
	return;
    }

    if (ok2acquire) {
	setAcqiTimeConst();
	setAcqiStatus();
    }

    /*maybe move to confirmer?? */
    /*ival = setAcqiInterval();
    if (ival < 0) {
	return;
    }*/

    initExpStatus(0);
}

#define VT_REPORTED_OFF	30000

void shrmemToStatblk(struct ia_stat *statblk, int valid_data )
{
    int	iter;
    int	NDCAcqState, NDCVTact;

    NDCAcqState = getStatAcqState();

    /* valid_data is whatever the application wants.  At this time the
     * application relies on other schemes involving time stamps to
     * see if the data is valid.
     */

    statblk->valid_data = valid_data;

    for (iter = 0; iter < (int) (sizeof( statblk->sh_dacs ) / sizeof( short )); iter++)
	statblk->sh_dacs[ iter ] = getStatShimValue( iter );

    statblk->lk_lvl = getStatLkLevel();
    statblk->LSDV = getStatLSDV();
    statblk->lk_gain = getStatLkGain();
    statblk->lk_power = getStatLkPower();
    statblk->lk_phase = getStatLkPhase();

    statblk->spinspd = getStatSpinAct();

    /*  Default value for ADC width or precision.  */

    statblk->adc_size = 16 + dspGainBits;

    if (can_acqi_acquire())
    {
	if (!signal_avg_on() || (neg_ct_count < 1))
	    neg_ct_count--;
    }
    statblk->neg_ct = neg_ct_count;  /* negative ct value for FID display */

    statblk->sh_smplx = (NDCAcqState == ACQ_SHIMMING) ? WORKING : FINISHED;
    statblk->rcvr_gain = getStatRecvGain();
    NDCVTact = getStatVTAct();
    if (NDCVTact == VT_REPORTED_OFF)
	statblk->VT_temp = 0.0;
    else
	statblk->VT_temp = NDCVTact;

    /*  You may find other statblk fields need to be filled in ...  */

}
