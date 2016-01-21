/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/* Shared Memory Experiment Information Structure Header */

#ifndef INCshrexpinfoh
#define INCshrexpinfoh

/* ExpFlags , defines */
/* ASM defines used by multiple programs */

#define AUTOMODE_BIT 0x8000/* bit set in ExpFlags in psg to indicate automode*/

/* bit set in ExpFlags in Acqproc to indicate resume was send to Autoproc*/

#define RESUME_SENT_BIT 0x4000

/* bit set in ExpFlags in Acqproc to indicate a Resume Acquisition (RA) */
#define RESUME_ACQ_BIT 0x1000

/* ExpFlags bit set in psg to indicate data buffer is cleared at each BS */
#define CLR_AT_BS_BIT 0x800

#define VPMODE_BIT 0x400/* bit set in ExpFlags in psg to indicate vpmode*/

/* GoFlag possible values */
#define EXEC_GO 0
#define EXEC_SU 1
#define EXEC_SHIM 2
#define EXEC_LOCK 3
#define EXEC_SPIN 4
#define EXEC_CHANGE 5
#define EXEC_SAMPLE 6
#define EXEC_GA 7
#define EXEC_AU 8

#define ACQI_LOCK 9
#define ACQI_GO 10

/* When Conditional Processing Bit Values */
#define WHEN_ERR_PROC 0x1
#define WHEN_EXP_PROC 0x2
#define WHEN_NT_PROC  0x4
#define WHEN_BS_PROC  0x8
#define WHEN_SU_PROC 0x10
#define WHEN_GA_PROC 0x20

/* ProcWait possible values */
#define AU_NOWAIT 0
#define AU_WAIT   1


#define EXPSTATE_NULL    	0
#define EXPSTATE_EQUEUED  	110
#define EXPSTATE_STARTED  	120
#define EXPSTATE_ACQUIRING  	130
#define EXPSTATE_PQUEUED  	140
#define EXPSTATE_PROCESSING  	150
#define EXPSTATE_TERMINATE 	800	/* values above this result in Premature Exp. Termination (for Sendproc) */
#define EXPSTATE_ABORTED 	810
#define EXPSTATE_HALTED   	820
#define EXPSTATE_STOPPED   	830
#define EXPSTATE_HARDERROR   	840

#define EXPINFO_STR_SIZE 256

/* ------------- Make C header file C++ compliant ------------------- */
#ifdef __cplusplus
extern "C" {
#endif

/* 
	8-10-94, Here's the 1st draft of this structure. It
	is bound to evolve is we go.
	The envisioned use has go & psg creating this
	shared structure via shrMLib. The filepath used
        shrmCreate() is passed on to Expproc which in turn 
	as necessary tell other process (via msgQ) to map in 
	this structure.
*/
/*===============================================================*/

typedef struct _expinfo {
	double	ExpDur;		/* Expected Exp. Duration in seconds */
	unsigned long long DataSize;	/* Complete Data Size (all FIDs,headers) */
	unsigned int	CurrentElem;	/* Current Element, Start on this elem (RA) */
	unsigned int	RAFlag;		/* Resume acquisition flag. (RA) */
	unsigned int	ArrayDim;	/* Total Elements of this Exp.) */
	unsigned int	FidSize;	/* FID Size in bytes */
	unsigned int	DataPtSize;	/* Data Point Size in bytes */
	unsigned int	NumAcodes;	/* Number of Acodes to Down Load to console */
	unsigned int	NumTables;	/* Number of Tables to Down Load to console */
	unsigned int	NumFids;	/* Number of Fids (NF) */
	unsigned int	NumTrans;	/* Number of Transients (NT) */
	unsigned int	NumInBS;	/* Number of Transients in BLockSize (BS) */
	unsigned int	NumDataPts;	/* Number of Data Points (NP) */
	unsigned int	LastCodeSent;	/* The last FID Acode sent to console */
	unsigned int	FidsCmplt;	/* The largest FID number completed  */
				/* Completion order is assumed in numeric*/
				/* Order, i.e. FID 1,2,3,4,5,6,etc... */
				/* Updated via Recvproc used by Sendproc */
	int	ExpNum;		/* Experiment # to perform processing in */
	int	ExpFlags;	/* Experiment Flags, i.e. automation, ra */
	int	IlFlag;		/* Interleave Flag */
	int	GoFlag;		/* Type of GO, SHIM,SU,GO,AU,SPIN, etc. */
	int	ProcMask;	/* Conditional Processing Mask */
	int	ProcWait;	/* Conditional Processing Flag */
	int	SampleLoc;	/* Sample Number, 0 - 100 (sample changer) */
	int	Priority;	/* Sample Priority */
        int	Gain;           /* Current Gain value */
        int	Spin;           /* Current Spin value */
	int	CurrentTran;	/* Current Transient */
        int	Celem;          /* Current Element */
        int	acode_1_size;   /* size of first acode set */
        int	acode_max_size; /* maximum size of acode sets */
        int	InteractiveFlag; /* acode set for acqi */
	

	long	DateSub;	/* Date & Time Exp. Submitted to run */
	long	DateStrt;	/* Date & Time Exp. was Started */
	long	DateFin;	/* Date & Time Exp. was FInished */

        int     DspGainBits;    /* DSP gain increase (real-time only) */

        int     DspOversamp;    /* DSP Oversampling factor */
        int     DspOsCoef;      /* DSP Oversampling coefs */
        int     DspOsskippts;   /* DSP skip points        */
        double  DspSw;          /* DSP Oversampling sw */
        double  DspFb;          /* DSP Oversampling fb */
        double  DspOslsfrq;     /* DSP Oversampling lsfrq */
        char    DspFiltFile[EXPINFO_STR_SIZE]; /* DSP filter file */

	pid_t	InetPid;	/* Vnmr pid, return address */
	int	InetPort;	/* Socket Port */
	long	InetAddr;	/* Inet Address */
	char	MachineID[EXPINFO_STR_SIZE];	/* Vnmr Host Name*/

	char	InitCodefile[EXPINFO_STR_SIZE];	/* Init stage codes */
	char	PreCodefile[EXPINFO_STR_SIZE];	/* Pre  stage codes only for Master Controller */
	char	PSCodefile[EXPINFO_STR_SIZE];	/* PS   stage codes */
	char	PostCodefile[EXPINFO_STR_SIZE]; /* post stage codes */
	char	AcqRTTablefile[EXPINFO_STR_SIZE]; /* New LC & Table parameter file path */

	char	RTParmFile[EXPINFO_STR_SIZE];	/* Acodes RT parameter file path */
	char	WaveFormFile[EXPINFO_STR_SIZE];	/* Waveform Pattern File path */

	char	Codefile[EXPINFO_STR_SIZE];	/* Acodes file path */
	char	TableFile[EXPINFO_STR_SIZE];	/* Acodes parameter file path */
	char	GradFile[EXPINFO_STR_SIZE];	/* Gradient WF file path */

	char	DataFile[EXPINFO_STR_SIZE];	/* FID Data file path */
	char	CurExpFile[EXPINFO_STR_SIZE];	/* User's exp directory */
	char	UsrDirFile[EXPINFO_STR_SIZE];	/* User's vnmrsys directory */
	char	SysDirFile[EXPINFO_STR_SIZE];	/* Vnmr System directory */
    char	UserName[EXPINFO_STR_SIZE];	/* User's Log Name */
	char	AcqBaseBufName[EXPINFO_STR_SIZE];/* Base Name of Named Buffers*/
	char	VpMsgID[EXPINFO_STR_SIZE];	/* Viewport message address*/
	mode_t	UserUmask;			/* file's umask, as presented by PSG */
    int 	ExpState;			/* Experiments State, e.g. Aborted, etc.. used & set by the Procs */
    int 	PSGident;			/* PSG identifier, Java = 100 , 'C' = 1 PSG */
	char	RvcrMapping[EXPINFO_STR_SIZE];	/* DDR mapping for muliple recievers */
    int     NFmod;                          /* nf modulus */
        struct  bill {  /* info used by accounting package */
           int    enabled;      /* write the file or not */
           int    wroteRecord;	/* ensure record is written only once */
           long   submitTime;
           long   startTime;    /* sec since UTC Jan 1, 1970, 00:00:00 */
           long   doneTime;
           char   goID[EXPINFO_STR_SIZE];      /* ID string */
           char   Operator[EXPINFO_STR_SIZE];   /* the name of the operator */
           char   account[EXPINFO_STR_SIZE];
           char   seqfil[EXPINFO_STR_SIZE];
        } Billing;

} SHR_EXP_STRUCT;

typedef SHR_EXP_STRUCT *SHR_EXP_INFO;


/*===============================================================*/
/*===============================================================*/


/* --------- ANSI/C++ compliant function prototypes --------------- */
 
#if defined(__STDC__) || defined(__cplusplus)
 

#else
/* --------- NON-ANSI/C++ prototypes ------------  */

#endif
 
#ifdef __cplusplus
}
#endif
 
#endif
