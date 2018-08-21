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
/*------------------------------------------------------------------------
|
|	The AcqProc Queue structure 
|	  This information is passed to the Acq. process by PSG.
|
|   Modified   Author     Purpose
|   --------   ------     -------
|   2/22/89   Greg B.    1. Added int CurrentElem and int CompleteElem to
|			    struct _value for Resume Acquistion (RA)
+-----------------------------------------------------------------------*/
struct _value    { 
		    char *Acqfile;	/* acqfile path */
		    char *Codefile;	/* acqqueue path */
		    char *RF_Patfile;	/* RF Obs. & Dec. pattern file */
		    char *Gradfile;	/* Gradient file */
		    char *Xpanfile;	/* Future Expansion file */
		    char *MachineID;	/* Host name of INET socket */
		    int   InetPort;	/* Inter-Net Port Number */
		    int   InetPid;	/* Inter-Net Port Process ID Number */
		    int   SuFlag;	/* suflag sets go, shim, spin, etc.*/
		    int   ExpFlags;	/* Experiment Flags */
		    int   AcqPos;	/* index in Acq system,a status also */
		    int   HalID;	/* Acq. system's Exper. ID  0-1024 */
		    long  DatSub;       /* Date & Time of day of submission.*/
		    long  DatAct;	/* Date & Time became active in Acq */
		    long  DatFin;       /* Date & Time of day of completion.*/
		    double  ExpDuration;/* Approx. time duration of Exp (sec)*/
	   unsigned long  CurrentElem;	/* Current Element, Exp to Start at  (RA)*/
	   unsigned long  CompleteElem;/* Total Complete Elements of Exp. (RA) */
		};
typedef struct _value Value;

/*---------------------------------------------------------------------
|	This the acquisition information maintain by the Acq. process
|	and sent to the Acquisition display program for user consumption.
|	mod 6/8/88 to be consistent on both sun3 & sun4
+---------------------------------------------------------------------*/
#ifndef MAX_SHIMS_CONFIGURED
#define MAX_SHIMS_CONFIGURED (48) /* Match define in hostAcqStructs.h */
#endif
struct _acqstat {

                    long  AcqCT;
                    long  AcqCmpltTime;
                    long  AcqRemTime;
                    long  AcqDataTime;
		    long  AcqSuFlag;
                    long  AcqSpinSet;
                    long  AcqSpinAct;
                    long  AcqSpinSpeedLimit;
                    short Acqstate;
                    short AcqExpInQue;
           unsigned long  AcqFidElem;
                    short AcqLSDV;
                    short AcqLockLevel;
		    short AcqSpinSpan;
		    short AcqSpinAdj;
		    short AcqSpinMax;
		    short AcqSpinActSp;
		    short AcqSpinProfile;
                    short AcqVTSet;
                    short AcqVTAct;
                    short AcqSample;
                    char  AcqUserID[10];
                    char  AcqExpID[11];
    		    char  dummy;
		    char  probeId1[20];
                    char  gradCoilId[12];
		    short AcqShimValues[MAX_SHIMS_CONFIGURED];
    		    short AcqShimSet;
		    short AcqLockGain;
		    short AcqLockPower;
		    short AcqLockPhase;
    	   unsigned long  AcqShortRfPower[4]; /* microwatts */
    	   unsigned long  AcqShortRfLimit[4];
    	   unsigned long  AcqLongRfPower[4];
    	   unsigned long  AcqLongRfLimit[4];
                    short AcqZone;
                    short AcqRack;
                };
typedef struct _acqstat AcqStatBlock;

/*---------------------------------------------------------------------
|	Experiment Data that relevent to each element (fid) during acquisition 
|	This is additional information on any experiments that have been
|	sent to acquisition and have not finshed yet (includes processing).
+---------------------------------------------------------------------*/
struct _elemvar {
	   unsigned long ElemNum;/* Element (FID) number of this data */
		    long   PCt;		/* CT for this FID */
		    long   PGain;	/* reciver gain for this FID */
		    long   PSpin;	/* spinner rate for this FID */
		};

typedef struct _elemvar Expelemstruc;

/*---------------------------------------------------------------------
|	Experiment Data that is used or updated during acquisition 
|	This is addition information on any experiments that have been
|	sent to acquisition and have not finshed yet (includes processing).
|
|   Modified   Author     Purpose
|   --------   ------     -------
|   2/22/89   Greg B.    1. Added long  ExpLastElemSent  to struct _expvar
|
+---------------------------------------------------------------------*/
struct _expvar {
		    char *ExpFid;
		    char *ExpCode;
		    char *ExpProc;
		    char *ExpAcq;
		    char *ExpUserDir;
		    Expelemstruc **ExpElem; /* pointer array for elemvar */
		    long *ExpRF_Indx;	/* Obs & Dec RF pattern index */
		    long *ExpGradIndx;	/* Gradient index */
		    long *ExpXpanIndx;	/* future expansion index */
	   unsigned long  NxtFid;
	   unsigned long  ExpLastElemSent; /* last element sent down the pipe */
		    long  ExpIndx[4];	/* Code index (offset for each FID)*/
		    long  DataPtSiz;	/* data point size in bytes */
		    long  FidSiz;	/* total data size */
		    long  N_Pts;	/* number of data points (np) */
		    long  N_Fids;	/* number of fids (nf) */
		    long  N_Trans;	/* number of transients (nt)  */
		    long  N_Bs;         /* transients per block size (bs) */
		    short  ExpWkNum;	/* Experiment # to do processing */
		    short  ExpIlFlag;	/* Experiment interleaving flag */
		    int    ExpProcMask;	/* Experiment conditional processing mask */
		    int    ExpPipeSiz;  /* Pipeline size of HAL for this Exp.*/
		    int    ExpInPipe;	/* Exp. Element in Pipe */
	   unsigned long   ExpElemFin;	/* Number of Exp. Element Finished */
		    int    ExpSuFlag;	/* Experiment Setup type(alias of go)*/
		    int    ExpProcWait;	/* Conditional Processing Flag */
		    mode_t umask4Vnmr;  /* umask as set by PSG */
		};
typedef struct _expvar Expparmstruc;

/*---------------------------------------------------------------------
|	Message packet from external task to AcqProc
+---------------------------------------------------------------------*/

struct _messpacket {
			int CmdOption;		/* command Option */
			char Hostname[50];	/* Who to reply to */
			int Port;		/* Inter-Net Port Number */
			int PortPid;		/* Inter-Net Port Process # */
			char Message[256];	/* message */
		  };
typedef struct _messpacket messpacket;
