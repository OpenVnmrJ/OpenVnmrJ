/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*----------------------------------------------------------------------
|
|	Header file for Acquisition Support
|
+----------------------------------------------------------------------*/

#ifndef INCacquisitionh
#define INCacquisitionh

/* get_acq returns  information about the acquisition interface */
/* set_acq sets information about the acquisition interface     */
extern int get_comm(int index, int attr, char *val);
extern int set_comm(int index, int attr, char *val);

extern void init_comm_addr();
extern void init_comm_port(char *hostval);
extern void init_acq(char *val);
extern void get_acq_id(char *id);
extern void set_acq_id(char *id);

extern int GetAcqStatus(int to_index, int from_index, char *host, char *user);
extern int getAcqStatusValue(int index, double *val);

extern int acq_errno;

/* Stuff defined in acqfuncs.c */
extern void interact_birth_record(char *tag, char *prog, int pid);
extern int interact_is_alive(char *tag, char *message);
extern int interact_is_connected(char *tag);
extern void interact_disconnect(char *tag);
extern void interact_kill(char *tag);
extern void interact_connect_status();
extern void interact_obituary(int pid);


#define ACQ_COMM_ID   0
#define VNMR_COMM_ID  1
#define LOCAL_COMM_ID 2
#define TCL_COMM_ID   3

#ifdef WINBRIDGE
// INFO_COMM_ID works with winBridge, which communicates status via a socket
// and not mmap like pre-inova, but still uses two ports like inova
#define INFO_COMM_ID  4
#endif

#define INITIALIZE      1
#define CONFIRM         2
#define ADDRESS         3
#define ADDRESS_VNMR    4
#define ADDRESS_ACQ     5
#define VNMR_CONFIRM    6

#define ACQOK(val)                get_comm(ACQ_COMM_ID,CONFIRM,val)
#define INIT_ACQ_COMM(dir)        init_acq(dir)
#define GET_ACQ_ADDR(val)         get_comm(ACQ_COMM_ID,ADDRESS_ACQ,val)

#define INIT_VNMR_COMM(hostname)  init_comm_port(hostname)
#define SET_ACQ_ID(ident)         set_acq_id(ident)
#define GET_ACQ_ID(ident)         get_acq_id(ident)

#define INIT_VNMR_ADDR()          init_comm_addr()
#define VNMR_ADDR_OK()            get_comm(VNMR_COMM_ID,VNMR_CONFIRM,NULL)
#define GET_VNMR_ADDR(val)        get_comm(VNMR_COMM_ID,ADDRESS_VNMR,val)
#define SET_VNMR_ADDR(val)        set_comm(VNMR_COMM_ID,ADDRESS,val)
#define GET_VNMR_HANDLE()         get_vnmr_handle(VNMR_COMM_ID)

#ifdef WINBRIDGE
#define GETACQSTATUS(host,user)   GetAcqStatus(INFO_COMM_ID,VNMR_COMM_ID,host,user)
#else
#define GETACQSTATUS(host,user)   GetAcqStatus(ACQ_COMM_ID,VNMR_COMM_ID,host,user)
#endif

#define ACQQUEUE 1		/* Enter New Experiment into QUEUE */
#define REGISTERPORT 2		/* Register Acq. Display Update port */
#define UNREGPORT 3		/* Remove Acq. Display Update port */
#define ACQABORT 4		/* Abort Specified Acquisition */
#define FGREPLY 5		/* VNMR FG Complete Reply */
#define ACQSTOP 6		/* Stop Specified Acquisition */
#define ACQSUPERABORT 7		/* Abort Acq & HAL to initial state */
#define ACQDEBUG_I 8
#define IPCTST 9		/* Echo back to Vnmr */
#define PARMCHG 10		/* change parameter (wexp,wnt,wbt,etc.) */
#define AUTOMODE 11		/* change to automation mode */
#define NORMAL 12		/* return to normal mode */
#define RESUME 13		/* send resume to autoproc */
#define SUPPEND 14		/* send suppend to autoproc */
#define ACQHALT 15		/* send halt, abort w/ Wexp processing */
#define ACQHARDWARE 16		/* insert, eject, set DACs, etc. */
#define READACQHW 17		/* read acquisition hardware parameters */
#define QUEQUERY 18		/* send acqproc queue status back */
#define ACCESSQUERY 19		/* check user for access permission  */
#define RECONREQUEST 20		/* request reconnection to Vnmr */
#define TRANSPARENT_I 21	/* transparent command, only used by INOVA */
#define RELEASECONSOLE 22	/* release console command, only used by INOVA */
#define QUERYSTATUS 23		/* query status of system */
#define RTAUPDT 24		/* Update realtime vars,acodes only in INOVA */
#define ATCMD 25		/* Queue an atcmd, Expproc proc only */
#define ROBOT 26                /* Send command to Roboproc */
#define AUTOQMSG 27             /* Send command to registered listeners */
#define ACQDEQUEUE 28		/* Dequeue Specified Acquisition */
#define ACQHALT2 29		/* send halt, abort w/ no processing */

/* Experiment Status */

#define EXP_VALID      100
#define EXP_LKPOWER    200
#define EXP_LKGAIN     300
#define EXP_LKPHASE    400
#define EXP_RCVRGAIN   500
#define EXP_SPINACT    600
#define EXP_LOCKFREQAP 700
#define EXP_SPINSET    800
#define EXP_GTUNEPX    900
#define EXP_GTUNEIX   1000
#define EXP_GTUNESX   1100
#define EXP_GTUNEPY   1200
#define EXP_GTUNEIY   1300
#define EXP_GTUNESY   1400
#define EXP_GTUNEPZ   1500
#define EXP_GTUNEIZ   1600
#define EXP_GTUNESZ   1700
#define EXP_GSTATUS   1800

#define EXP_NPERR     1900
#define EXP_RCVRNPERR 1910

/* Acquisition Status */

#define LOCKLEVEL  100
#define SUFLAG     200
#define USERID     300
#define EXPID      400
#define STATE      500

/* Communication Errors */

#define RET_OK     0
#define RET_ERROR -1

#define READ_ERROR 10
#define OPEN_ERROR 20
#define BIND_ERROR 30
#define CONNECT_ERROR 40
#define SELECT_ERROR 50
#define ACCEPT_ERROR 60
#define FCNTL_READ_ERROR 70
#define FCNTL_WRITE_ERROR 80
#define MSG_LEN_ERROR 90

#endif
