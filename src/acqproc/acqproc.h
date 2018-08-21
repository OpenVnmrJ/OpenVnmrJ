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

#define MAXHalID  100


/* ------------------  Defines for logprint() --------------- */
#define USER 1
#define MASTER 2
#define STDERR 4

#define TRUE 1
#define FALSE 0

#define OK 0
#define ERROR 1
#define NOACQS -1
#define NO_BGFORKED -11
#define MAXPTHL 256


/* ------------------  Conditional processing defines ---------------- */
#define PROCSTRT 0
#define SKIPPED 1
#define SKIP 1
#define PROC_WAIT 2
#define QUEUE 3
#define QUEUED 3
#define WAITNHOLD 4
#define COMPLETE 4
#define FALSE 0
#define TRUE 1
#define FGREPLY 5
#define FGNOREPLY 6

#define INACTIVE -99
#define WEXP 0
#define WBS 1
#define WFID 2
#define WERR 3
#define BG 1
#define FG 2


/* WARNING: Don't change these defines */
/* --- conditional processing priority --- */
#define WBSPRI 0
#define WFIDPRI 1
#define WERRPRI 2
#define WEXPPRI 3

/*-------------------- automation defines  -----------------*/
#define AUTORESUME 1
#define AUTOCMPLT 100
#define AUTO_PENDING -2

/*-------------------- acqproc error level messages -----------------*/
#define DEBUG9 3
#define DEBUG8 2
#define DEBUG7 1
#define DEBUG6 6
#define DEBUG5 5
#define DEBUG4 4
#define DEBUG3 3
#define DEBUG2 2
#define DEBUG1 1
#define DEBUG0 0

/*------------------------------ HAL status block defines -----------------*/
/*#define HALOK 1*/
#define H_IDLE 1
#define H_ACTIVE 2
/*------------------------------ HAL command defines -----------------*/
#ifndef GEMPLUS
#define STATUS 1
#endif
#define SEND_ID 2
#define SEND_FIDN 3
#define SEND_ACQP 4
#define SEND_CODE 5
#define SET_MAXTRNSFR 6
#define RECV_ACQP 7
#define RECV_FID 8

#define LOCAL_ELE   1
#define GLOBAL_ELE  2
/*------------------------------ Disk I/O commands -----------------*/
#define BUFFER 1
#define FLUSH 2
#define DELETE 3

/* --- acqproc global prarmeters --- */

extern messpacket MessPacket;

extern int fromsocket;	/* socket descripter of originating message */
extern int acqsockrd;	/* acquisition process socket descriptor (reading)*/
extern int acqsockwr;	/* acquisition process socket descriptor (writing)*/
extern int messocket;	/* message process async socket descriptor */

/* extern struct sockaddr sockname;	/* name binded to socket */
extern struct sockaddr_in acqread;     /* Inet address bound to socket */
extern struct sockaddr_in acqwrite;     /* Inet address bound to socket */
extern struct sockaddr_in messname;     /* Inet address bound to socket */

extern short	HalStatus;		/* request sense status from HAL */
extern ACQstatblock Statusblock;
extern AcqStatBlock acqinfo;	/* status block sent to acq. display proc */
struct exp_block HalTransBlk;	/* ACQ. <---> HAL Exp Transfer param. block */

extern Value *expqueuelist[MAXHalID];
extern Expparmstruc *expparmlist[MAXHalID];

extern int Acq_Active;     /* True if Acquisition is currently underway */
extern int activeID;
extern int halpollcnt; /*count of hal polling of not getting large stat block*/
extern int totalinque;	/* total pending exper in the acqproc acq. queue*/
extern int totaldoneque;/* total completed exper in the acqproc acq. queue */
extern int totalinAcq;	/* total experiments in the acquisition system(HAL)*/
extern int acqstatus;		/* acquisition status */

/* --- log file externals --- */
extern int logfd;
extern int fidfd;
extern int acqfd;
extern int codefd;

extern int    Acqdebug;		/* debugging flag */
extern long   PresentTime;	/* present Time and Day */
extern long   AcqStatLastRead;   /*last time of obtaining long stat block */
extern long   AcqStatIntval;	/*min time between obtaining long stat block*/
extern long   M_LogLastWrt;	/*last time that Log file written to disk */
extern long   M_LogIntval;	/*min time between writing Log file out */
				/*   acquisition status */
extern int    InterActive_Flag; /* Flag is set when a direct socket */
				/*   connection is made, the  msgehandler */
				/*   checks this to decide on reactiving */
				/*   the HAL polling */

extern int AutoMode;		/* automation mode flag */
extern int Auto_Pid;		/* Autoproc PID */
extern int Auto_Active;		/* Autoproc Resume Processing Active flag */
extern int Resume_Sent;		/* Resume sent flag */
extern int LastResume;		/* LastResume sent flag */
 
extern char Vnmrpath[256];	/* file path to Background Vnmr */
extern char Autopath[256];	/* file path to Autoproc Vnmr */
extern char doneQpath[256];	/* file path to Automation done Q */
extern char LocalAcqHost[256];	/* AcqProc's Host machine Name */
extern int  LocalAcqPort;	/* Acqproc's Ports */
extern int  LocalAcqPid;	/* Acqproc's PID */
extern char vnmrsystem[];    	/* vnmrsystem path */

extern int Waited4ID;		/* Exp ID of FG processing */

extern char wallpath[];		/* path to the wall command */

extern char *shimnames[]; 	/* shim variablies in coil order */
