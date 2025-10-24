/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef HOSTACQSTRUCTS_H
#define HOSTACQSTRUCTS_H

#ifdef LINUX
#include <netinet/in.h>
#endif

/*  Define configuration for STM  */

#define MAX_STM_OBJECTS 4
#define STM1 0
#define STM2 1
#define STM3 2
#define STM4 3

/*  Define bits representing presence/absence of key hardware components  */

#define  FIFO_PRESENT	0x0001
#define  MSR_PRESENT	0x0002

#define MAX_ADC_OBJECTS 4
#define ADC_BASE_PRESENT	0x0004
#define ADC_PRESENT( index )	((ADC_BASE_PRESENT) << (index))

/*  Use STM1, STM2, ... STM4 as the index for STM_PRESENT  */

#define  STM_BASE_PRESENT	(ADC_BASE_PRESENT << MAX_ADC_OBJECTS)
#define  STM_PRESENT( index )	((STM_BASE_PRESENT) << (index))

/*  Define situations when the status of the system is stored in non-volatile RAM  */

#define  SYSTEM_STARTUP			0
#define  SYSTEM_ABORT			(SYSTEM_STARTUP + 1)
#define  MAX_CONSOLE_DEBUG_INDEX	SYSTEM_ABORT

/* console to hosts uplink Synchronizing Message */
#define ACQ_UPLINK_START_MSG 	("@START")
#define ACQ_UPLINK_MSG 		("UPLINK")
#define ACQ_UPLINK_ENDDATA_MSG 	("ENDDAT")
#define ACQ_UPLINK_INTERA_MSG	("INTERA")
#define ACQ_UPLINK_NODATA_MSG	("NODATA")
#define ACQ_UPLINK_XFR_MSG_SIZE (6)

/* definitions of possible state values */
#define NOT_ALLOCATED 		(-99)
#define	DATA_OK			(0x0)
#define	DATA_STOP		(0x1)
#define	DATA_ABORT		(0x2)
#define	DATA_WARNING_ERROR	(0x3)
#define	DATA_SOFT_ERROR		(0x4)
#define	DATA_HARD_ERROR		(0x5)

#define DATA_REALLOC		(-10)
#define DATA_TERMINATE		(-20)

/* STM Interrupt Base defines	*/
#define RTZ_DATA_CMPLT 		(0x100)
#define USER_FLAG 		(0x200)
#define MAX_TRANS 		(0x400)
#define RPNZ_DATA_ERROR 	(0x800)
#define STM_ITRP_BIT_MASK	(0xF00)
    

/* Data Transfer Size */
#define XFR_SIZE        (65536)

/* Status used by downLinker in console */
#define XFER_STATUS_BLK	(123454321L)  /* indicate not data but a status block */
#define XFER_COMPLETE	(6543210L)  /* transfer complete */
#define XFER_ABORTED	(9876543L)  /* transfer has been aborted */
#define XFER_RESETCNT	(43211234)  /* reset count back to zero, so that next name buff will be exp#f0 */

/* DownLinker Transfer Sizes */
#define DLCMD_SIZE 200
#define DLXFR_SIZE 8192
#define DLRESPB	10

/* Async message from Console to Host are this size, no matter 
   what actual structure or string is being sent */
#define CONSOLE_MSGE_SIZE	(512)

/* type of command, parse string command (typically and embedded reply)
   of a console case based action
*/
#define PARSE		(1)
#define CASE		(2)
#define STATBLK		(3)
#define TUNE_UPDATE	(4)
#define	CONF_INFO	(5)
#define	CDB_INFO	(6)

/* Host Case Actions, also see expDoneCodes.h expproc */
#define PANIC		(1)
#define EXP_CMPLT	(2)
#define IL_CMPLT	(3)	/* used for SA */
#define CT_CMPLT	(4)	/* used for SA */
/* EXP_FID_CMPLT, WARNING_MSG, SOFT_ERROR, HARD_ERROR, EXP_ABORTED 
		(see expDoneCodes.h) */
#define EXP_HALTED	(7)
#define EXP_STOPPED	(8)
#define GET_SAMPLE	(9)
#define LOAD_SAMPLE	(10)
#define HEARTBEAT_REPLY (11)
#define REPUT_SAMPLE    (30)
#define FAILPUT_SAMPLE  (31)
/* must skip 11-19 */

/* Console Case Actions */
#define ECHO		(1)
#define XPARSER		(2)
#define APARSER		(3)
#define ABORTACQ	(4)
#define HALTACQ		(5)
#define STATINTERV	(6)
#define STARTLOCK	(7)
#define STARTINTERACT	(8)
#define GETINTERACT	(9)
#define STOPINTERACT	(10)
#define AUPDT		(11)
#define STOP_ACQ	(12)
#define ACQDEBUG	(13)
#define HEARTBEAT	(14)
#define GETSTATBLOCK	(15)
#define ABORTALLACQS	(16)
#define OK2TUNE		(17)
#define ROBO_CMD_ACK	(18)

/* Distinguish this one (console information)
   from CONF_INFO (configuration information) */

#define CONSOLEINFO	(19)

#define DOWNLOAD	(20)
#define STARTFIDSCOPE	(21)
#define STOPFIDSCOPE	(22)
#define GETCONSOLEDEBUG	(23)

#ifndef MAX_SHIMS_CONFIGURED
#define MAX_SHIMS_CONFIGURED	(48)
#endif

#define MIN_STATUS_INTERVAL	10
#define MAX_STATUS_INTERVAL	5000

/*-------------------------------------------------------------------
|   acquisition status codes for status display on Host
|    values assigned to 'Acqstate' in structure CONSOLE_STATUS;
|    symbols and values match definitions in STAT_DEFS.h,
|    SCCS category xracq
+-------------------------------------------------------------------*/
#ifndef ACQ_INACTIVE
#define ACQ_INACTIVE 	0
#define ACQ_REBOOT 	5
#define ACQ_IDLE 	10
#define ACQ_PARSE	15
#define ACQ_PREP 	16
#define ACQ_SYNCED 	17
#define ACQ_ACQUIRE 	20
#define ACQ_PAD		25
#define ACQ_VTWAIT 	30
#define ACQ_SPINWAIT 	40
#define ACQ_AGAIN 	50
#define ACQ_HOSTGAIN   	55
#define ACQ_ALOCK 	60
#define ACQ_AFINDRES 	61
#define ACQ_APOWER 	62
#define ACQ_APHASE 	63
#define ACQ_FINDZ0 	65
#define ACQ_SHIMMING 	70
#define ACQ_HOSTSHIM   	75
#define ACQ_SMPCHANGE 	80
#define ACQ_RETRIEVSMP 	81
#define ACQ_LOADSMP 	82
#define ACQ_ACCESSSMP 	83   /* Access open status */
#define ACQ_ESTOPSMP 	84   /* Estop status */
#define ACQ_MMSMP 	85   /* Magnet motion status */
#define ACQ_HOMESMP 	86   /* Homing */
#define ACQ_INTERACTIVE 90
#define ACQ_TUNING	100
#define ACQ_PROBETUNE	105

/* Console ID definitions */
#define CONSOLE_VNMRS_ID 0
#define CONSOLE_400MR_ID 1
#define CONSOLE_SILKVNMRS_ID 2
#define CONSOLE_SILK400MR_ID 3
#endif // ACQ_INACTIVE

/*  VT defines that maybe used in AcqVTSet, AcqVTAct */
#define VTOFF   30000   /* temp value if VT controller is tobe passive */

#define SPINOFF (-1)   /* spinner value if spinner controller is tobe passive */

/* Bits to be set when a setup complete occurs or when an experiment
   completes.  Lets the host know when a setup has been done, so any
   access or operations (ACQI, qtune, etc.) that require a setup can
   be blocked until this setup (or go) is done.				*/

#define SETUP_CMPLT_FLAG	0x01
#define EXP_CMPLT_FLAG		0x02

/* ------------- Make C header file C++ compliant ------------------- */
#ifdef __cplusplus
extern "C" {
#endif

#ifdef LINUX
#define FSB_CONVERT(data) \
{ \
   data.elemId = ntohl(data.elemId); \
   data.startct = ntohl(data.startct); \
   data.ct = ntohl(data.ct); \
   data.nt = ntohl(data.nt); \
   data.np = ntohl(data.np); \
   data.dataSize = ntohl(data.dataSize); \
   data.dcLevelReal = ntohl(data.dcLevelReal); \
   data.dcLevelImag = ntohl(data.dcLevelImag); \
   data.recvGain = ntohl(data.recvGain); \
   data.doneCode = ntohl(data.doneCode); \
   data.errorCode = ntohl(data.errorCode); \
}
#endif

/* ----------------- FID Stat Block Element --------------------- */
/* This structure is 1st to send to host prior to each FID to be
   transfer to host 
   This structure contains all the information required by the
   data tranfer task and by the host computer.
*/
typedef struct _datastat {
	unsigned int	elemId;		/* FID number */
        unsigned int	startct;	/* logical starting CT */
        unsigned int	ct;		/* Completed Transients */
        unsigned int	nt;		/* Number of  Transients */
	unsigned int   np;		/* number of data points */
	unsigned int   dataSize;	/* Fid Size in bytes */
	unsigned int   dcLevelReal;
	unsigned int   dcLevelImag;
	int	recvGain;
        int	doneCode;
        int	errorCode;
#ifdef LINUX
	int	fidAddr;	/* keep struct the same size when compiled as 64-bit */
#else
	int*	fidAddr;	/* Fid Memory Address */
#endif
    /*unsigned int	rcvrIndex; */ 	/* Which rcvr this data is from */
        	         } FID_STAT_BLOCK;

#ifdef LINUX
#define FSB_CONVERT_NTOH(data) \
{  FID_STAT_BLOCK *pFidStatBlk; \
   pFidStatBlk = data;		\
   pFidStatBlk->elemId = ntohl(pFidStatBlk->elemId); \
   pFidStatBlk->startct = ntohl(pFidStatBlk->startct); \
   pFidStatBlk->ct = ntohl(pFidStatBlk->ct); \
   pFidStatBlk->nt = ntohl(pFidStatBlk->nt); \
   pFidStatBlk->np = ntohl(pFidStatBlk->np); \
   pFidStatBlk->dataSize = ntohl(pFidStatBlk->dataSize); \
   pFidStatBlk->dcLevelReal = ntohl(pFidStatBlk->dcLevelReal); \
   pFidStatBlk->dcLevelImag = ntohl(pFidStatBlk->dcLevelImag); \
   pFidStatBlk->recvGain = ntohl(pFidStatBlk->recvGain); \
   pFidStatBlk->doneCode = ntohl(pFidStatBlk->doneCode); \
   pFidStatBlk->errorCode = ntohl(pFidStatBlk->errorCode); \
}
#else
#define FSB_CONVERT_NTOH(data)
#endif


/* --------------------------------------------------------------------- */
/*  Console Status Block, relay to Expproc via hostAgent task            */
/* --------------------------------------------------------------------- */
/*  The AcqOpsComplCnt field lets get_ia_stat tell if it has received a  */
/*  stat block subsequent to the console receiving its Get Stat Block    */
/*  command.  See monitor.c and X_interp.c, SCCS category vwacq, and     */
/*  get_ia_stat in socket.c, SCCS category vnmr.                         */
/* --------------------------------------------------------------------- */

#ifdef LINUX
#define CSB_CONVERT(data) \
{ \
   int count; \
   data->AcqCtCnt = ntohl(data->AcqCtCnt); \
   data->AcqFidCnt = ntohl(data->AcqFidCnt); \
   data->AcqSample = ntohl(data->AcqSample); \
   data->AcqLockFreqAP = ntohl(data->AcqLockFreqAP); \
   data->AcqLockFreq1 = ntohl(data->AcqLockFreq1); \
   data->AcqLockFreq2 = ntohl(data->AcqLockFreq2); \
   data->AcqNpErr = ntohl(data->AcqNpErr); \
   data->AcqSpinSet = ntohl(data->AcqSpinSet); \
   data->AcqSpinAct = ntohl(data->AcqSpinAct); \
   data->AcqSpinSpeedLimit = ntohl(data->AcqSpinSpeedLimit); \
   data->AcqPneuBearing = ntohl(data->AcqPneuBearing); \
   data->AcqPneuStatus = ntohl(data->AcqPneuStatus); \
   data->AcqPneuVtAir = ntohl(data->AcqPneuVtAir); \
   data->AcqTickCountError = ntohl(data->AcqTickCountError); \
   data->AcqChannelBitsConfig1 = ntohl(data->AcqChannelBitsConfig1); \
   data->AcqChannelBitsConfig2 = ntohl(data->AcqChannelBitsConfig2); \
   data->AcqChannelBitsActive1 = ntohl(data->AcqChannelBitsActive1); \
   data->AcqChannelBitsActive2 = ntohl(data->AcqChannelBitsActive2); \
   data->AcqRcvrNpErr = ntohs(data->AcqRcvrNpErr); \
   data->Acqstate = ntohs(data->Acqstate); \
   data->AcqOpsComplCnt = ntohs(data->AcqOpsComplCnt); \
   data->AcqLSDVbits = ntohs(data->AcqLSDVbits); \
   data->AcqLockLevel = ntohs(data->AcqLockLevel); \
   data->AcqRecvGain = ntohs(data->AcqRecvGain); \
   data->AcqSpinSpan = ntohs(data->AcqSpinSpan); \
   data->AcqSpinAdj = ntohs(data->AcqSpinAdj); \
   data->AcqSpinMax = ntohs(data->AcqSpinMax); \
   data->AcqSpinActSp = ntohs(data->AcqSpinActSp); \
   data->AcqSpinProfile = ntohs(data->AcqSpinProfile); \
   data->AcqVTSet = ntohs(data->AcqVTSet); \
   data->AcqVTAct = ntohs(data->AcqVTAct); \
   data->AcqVTC = ntohs(data->AcqVTC); \
   data->AcqPneuVTAirLimits = ntohs(data->AcqPneuVTAirLimits); \
   data->AcqPneuSpinner = ntohs(data->AcqPneuSpinner); \
   data->AcqLockGain = ntohs(data->AcqLockGain); \
   data->AcqLockPower = ntohs(data->AcqLockPower); \
   data->AcqLockPhase = ntohs(data->AcqLockPhase); \
   for (count = 0; count < MAX_SHIMS_CONFIGURED; count++) \
      data->AcqShimValues[count] = ntohs(data->AcqShimValues[count]); \
   data->AcqShimSet = ntohs(data->AcqShimSet); \
   data->AcqOpsComplFlags = ntohs(data->AcqOpsComplFlags); \
   data->rfMonError = ntohs(data->rfMonError); \
   for (count = 0; count < 8; count++) \
      data->rfMonitor[count] = ntohs(data->rfMonitor[count]); \
   data->statblockRate = ntohs(data->statblockRate); \
   for (count = 0; count < 9; count++) \
      data->gpaTuning[count] = ntohs(data->gpaTuning[count]); \
   data->gpaError = ntohs(data->gpaError); \
   data->consoleID = ntohs(data->consoleID); \
}
#endif


typedef struct _consolestat {
		int	AcqCtCnt;
		int	AcqFidCnt;
		int	AcqSample;
		int	AcqLockFreqAP;
		int	AcqLockFreq1;
		int	AcqLockFreq2;
		int	AcqNpErr;
                int     AcqSpinSet;
                int     AcqSpinAct;
		int	AcqSpinSpeedLimit;
                int     AcqPneuBearing;
                int     AcqPneuStatus;
                int     AcqPneuVtAir;
                int     AcqTickCountError;
                int     AcqChannelBitsConfig1;
                int     AcqChannelBitsConfig2;
                int     AcqChannelBitsActive1;
                int     AcqChannelBitsActive2;
		short	AcqRcvrNpErr;
		short	Acqstate;
		short	AcqOpsComplCnt;
		short	AcqLSDVbits;
		short	AcqLockLevel;
		short	AcqRecvGain;
		short   AcqSpinSpan;
		short	AcqSpinAdj;
		short	AcqSpinMax;
		short	AcqSpinActSp;
		short	AcqSpinProfile;
		short	AcqVTSet;
		short	AcqVTAct;
                short   AcqVTC;
                short   AcqPneuVTAirLimits;
                short   AcqPneuSpinner;
		short	AcqLockGain;
		short	AcqLockPower;
		short	AcqLockPhase;
		short	AcqShimValues[MAX_SHIMS_CONFIGURED];
		short	AcqShimSet;
		short	AcqOpsComplFlags;
    		short	rfMonError; 	/* RF Monitor error code */
		short	rfMonitor[8];	/* Avg RF power on up to 6 channels:
                                         * Bits 0-5: power (63 = trip level)
                                         * 6-10: knob position
                                         */
		short	statblockRate;	/* Time between statblocks in ms */
                short   gpaTuning[9];   /* 3 parameters per axis; order is:
                                         *  x-proportional, x-integral, x-spare,
                                         *  y-p, y-i, y-s, z-p, z-i, z-s */
                short   gpaError;       /* Grad Power Amp error code:
                                         * Bit 0: Amp Fault,  1: Amp Warning
                                         * 2: Amp Comm Fault, 3: Power Fault
                                         * 4: Power Warning,  5: PS Comm Fault
                                         * 6: NIC Fault,      7: External Fault
                                         * 8: No Comm w/ GPA
                                         */
		char	probeId1[20];
		char	gradCoilId[12];
      short consoleID;
} CONSOLE_STATUS;

/*-------------------------------------------------------------------*/
/*  Construct to allow Expproc to understand its a Console Status Block */
/*-------------------------------------------------------------------*/

typedef struct _statusblk
{
	int  dataTypeID;		/* Set to STATBLK (above) to */
				  /* identify this as a Status Block */
	CONSOLE_STATUS	stb;
} STATUS_BLOCK;


/*-------------------------------------------------------------------
* Internal to console Error or Exception Structure send to PHandler
*  by who error has a exception, E.G. fifo FOO, etc
*-------------------------------------------------------------------*/

typedef struct _phandlrmsg_
{
	short  exceptionType;	/* Exection Type (HAARDERROR,WARNING,etc) */
	short  reportEvent;	/* Actual Warning,Error, or other Event */
} EXCEPTION_MSGE;

/*-------------------------------------------------------------------
* Internal to console STM Data Ready/Error FIFO Error Message for UpLinkers 
*  provide DoneCode & ErrorCodes, etc... 
*-------------------------------------------------------------------*/

/* Types of ITR_MSG's */
#define  INTERRUPT_OCCURRED	0	/* Std Acquisition */
#define  SEND_DATA_NOW		1	/* for FID scope */
#define  DTM_COMPLETE		2
#define  START_INTERVALS	3
#define  STOP_INTERVALS		4

/* ------------- interrupt Message struct ----------------- */
/* no longer really an interrupt message struct - but we retain the name */

typedef struct {
		short tag;
		short donecode;
		short errorcode;
	        short msgType;
	        unsigned int count;
    		void *stmId;
	       }  ITR_MSG;


#define CONSOLE_DEBUG_MAGIC	0x0124

/* Enforce 32 bit alignment */

typedef struct {
	void	*stmSrcAddr;		/* STM */
	void	*stmDstAddr;
	int	 stmNpReg;
	int	 stmNtReg;
	short	 stmStatus;
	short	 stmTag;
} STM_DEBUG;

typedef struct {
	int	 timeStamp;		/* general information */
	short	 magic;
	short	 revNum;
	int	 startupSysConf;
	int	 currentSysConf;
	short	 Acqstate;
	short	 pad1;

/* Enforce 32 bit alignment here */

	int	 lastFIFOword[ 3 ];	/* FIFO */
	int	 fifoStatus;
	short	 fifoIntrpMask;
	short	 pad2;

/* Enforce 32 bit alignment here */

	STM_DEBUG	stmHwRegs[ MAX_STM_OBJECTS ];

/* Enforce 32 bit alignment here */

	short	 autoStatus;		/* MSR (Automation) */
	short	 autoHSRstatus;

/* Enforce 32 bit alignment here */

	short	 adcStatus;		/* ADC */
	short	 adcIntrpMask;
} CONSOLE_DEBUG;


typedef struct	_cdb_msg {
	int	msg_type;
	int	index;
	CONSOLE_DEBUG	cdb;
} CDB_BLOCK;


#ifdef __cplusplus
}
#endif
 
#endif
