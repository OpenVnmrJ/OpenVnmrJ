/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* spinObj.h Copyright (c) 1994-1996 Varian Assoc.,Inc. All Rights Reserved */
/*
 */

/* #define _POSIX_SOURCE /* defined when source is waited tobe POSIX-compliant */
/* #define _SYSV_SOURCE /* defined when source is System V */
/* #ifdef __STDC__ /* used to determine if using an ANSI compiler */


#ifndef INCspinh
#define INCspinh


/* ------------- Make C header file C++ compliant ------------------- */
#ifdef __cplusplus
extern "C" {
#endif

/*   Defines Here */

#define	VERTICAL_PROBE 1	/* Control a Vertical Probe */
#define	MAS_PROBE 2		/* Control a MAS Probe */
#define	MAS_THRESHOLD 80	/* Default switch over threshold for MAS control */

#define RATE_MODE	1	/* setRate() has been used, constant air flow */
#define SPEED_MODE	2	/* setSpeed() has been used, speed regulation */

/* this struct is the template for an array of PID calculations that can
|  be dynamically allocated and pointed to in SPIN_OBJ.
*/
typedef struct {
	long PIDcorrection; /* Air valve off time */
	long PIDperiod;     /* copy of SPNperiod */
	long PIDproportion; /* copy of SPNproportion */
	long PIDintegral;   /* copy of SPNintegral */
	long PIDderivative; /* copy of SPNderivative */
	long PIDlerror;     /* copy of SPNlerror */
	long PIDspeed;      /* speed * 10 + frac */
	long PIDheartbeat;  /* copy of PIDheartbeat */
} PIDelement;
typedef PIDelement *PIDE;
#define PIDsize sizeof(PIDelement);


#define SPNperiodSize 16
#define PID_ERROR_BUFSIZE 4

typedef struct {
	int  V_ItrVectNum;        /* (spinCreate) */
	int  MAS_ItrVectNum;      /* (spinCreate) */
	char* pIdStr;             /* Identifier String (spinCreate) */
	unsigned short MASdac;    /* current value in MAS DAC */
	VOIDFUNCPTR *V_ItrVect;   /* Vector offset */
	VOIDFUNCPTR *MAS_ItrVect; /* Vector offset */
	int SPNitrHndlr;    /* pointer to SpinUpdate handler */
	int XitrHndlr;      /* pointer to Unused vector handler */
	int  SPNtype;       /* Vertical = 1, MAS = 2. */
	int  SPNmasthres;   /* spinner threshold speed which result in switching to MAS control. */
	int  SPNrevs;       /* current revolutions/interrupt */
	int  SPNitrpmask;   /* current interrupt mask */
	int  Zero;          /* Initialized to 0, used to write only a Zero */
                      /* Compilers have a tendency to use clr=read mod write */
        int  SPNmode;	    /* in Rate (air flow) or Speed regualtion mode */
	long BooBooheart;   /* if unused interrupt is ever fired */
	long SPNheartbeat;  /* inc by (intrp svc) */
	long SPNeeg1;       /* check interrupts (chkSpeed) */
	long SPNeeg2;       /* check interrupts (chkSpeed) */
	long SPNsum;        /* current box car value of periods (intrp svc)*/
	long SPNperiod;     /* SPNsum/SPNnbr (chkspeed) */
	long SPNregulate;   /* calculated from speedSet (setspeed) */
	long SPNcount;      /* how many times thru the table (intrp svc) */
	long SPNnbr;        /* size of SPNperiod (spinCreate) */
	long SPNindex;      /* index to last/next period (intrp svc) */
	long SPNspeed;      /* calculated speed (int) from SPNsum (chkspeed) */
	long SPNspeedfrac;  /* calculated speed (fraction) see above */
	long SPNspeedSet;   /* speed to regulate to (setspeed) */
	long SPNrateSet;    /* Air Flow Rate to Go to,  value depends LIQ or MAS */
	long Vairofftime;   /* air off time when regulating */
	long SPNlerror;     /* last error value (spnPID) */
	long SPNwindow;     /* speed tolerance in rev/sec (initxx/setWindow) */
	long SPNtolerance;  /* regulation tolerance (setSpeed) */
	long SPNproportion; /* Proportion for PID (spnPID) */
	long SPNintegral;   /* running Integral for PID (spnPID) */
	long SPNderivative; /* Derivative for PID (spnPID) */
	long SPNpefact;     /* PeriodError divisor */
	long SPNsetting;    /* current air off time setting */
	long PIDcorrection; /* correction to current air off time */
	long PIDmode;       /* mode > 1 fix integral, 0=normal */
	long PIDheartbeat;  /* for debug heartbeat of PID routine */
	long PIDerrorbuf[PID_ERROR_BUFSIZE]; /* last 4 errors for Integral Er*/
	long TCR1x10period; /* ns * 10 per TCR1 clock */
	long TCR2x10period; /* ns * 10 per TCR2 clock */
	long QOMoffRegs;    /* number of registers used for OFF time */
	long QOM_500ms;     /* 500ms period at TCR2 clock */
	long QOM_9ms;       /* 9ms period at TCR2 clock for ON time */
	long SpinLED;       /* state 0=off, 1=|__, 2=_||, 3=ON */
	long SPNkO;         /* multiplier for PID calc to off time */
	long SPNkP[2];      /* 0=slow P, 1=fast P (spinCreate/setLparam) */
	long SPNkI[2];      /* 0=slow I, 1=fast I (spinCreate/setLparam) */
	long SPNkD[2];      /* 0=slow D, 1=fast D (spinCreate/setLparam) */
	long SPNkC[2];      /* max Change in air flow per adjustment period */
	long SPNkF;         /* Actually a divisor as poor mans fractional PID */
	long SPNperiods[SPNperiodSize]; /* table of periods (intrp svc) */
	int  PIDerrindex;  /* index into error buffer */
	int PIDelements;    /* Total number of elements in PIDE */
	int PIDposition;    /* current element to be filled */
	PIDE pPIDE;         /* 0=no struct allocated else pointer to struct */ 
	int	SPNtaskid1;     /* contains a taskid for debugging */
	int	SPNtaskid2;     /* contains a taskid for debugging */
	int	SPNchkSpeedtid; /* contains the taskid for chkSpeed */
	int Vejectflag;     /* Eject has occured */
	int Vdroptime;      /* Delay in seconds between Eject Off and SDROP off */
	int Vdropdelay;     /* Delay in seconds between SDROP Off and regulation */
	int SPNtomb;        /* The special Tom Barbara diag print flag */
	int SPNgad;         /* The special Glen Diestelhorst diag print flag */
			    /* The following are used in MAS rotor to track changes that would
			       indicate a rotor that con not achieve speed do to air or rotor
			       crash.
			    */
	long PrevDacVal;    /* Preceding MAS DAC setting */
	long PrevDacDelta;  /* The change in MAS DAC setting  */
	long PrevSpeed;     /* The rotor speed from previous chkSpeed */
	long SPNdelaySet;   /* speed to regulate to (setspeed) */
} SPIN_OBJ;

typedef SPIN_OBJ *SPIN_ID;
#define	UPk        0x1  /* index into PID constants for faster */
#define	DOWNk      0x0  /* index into PID constants for faster */

#define M332_SYS_CLOCK 32768 * 512	/* Automation board clock */

#define	MASCC	0x07	/* rising edge of TCR1 */
#define	VCC	0x07	/* rising edge of TCR1 */
/* ...REVS is the number of revolutions per interrupt */
/* ...PC is the pattern for TPU "period/count" reg.   */
/* REVS is used in a caluclation of speed so must be tied to reg. pattern */
#define	VMC_REVS 1	/* One revolution per interrupt */
#define	VMC_PC   (VMC_REVS * 2) << 8	/* count 2 periods (one revolution) */
#define	MASMC_REVS 250	/* 250 revolutions per interrupt */
#define	MASMC_PC (MASMC_REVS) << 8	/* 250 counts 250 revs */
#define	V_WINDOW 1	/* Vertical speed +/- 1 rev/sec */
#define	MAS_WINDOW 10	/* MAS speed +/- 10 rev/sec */



/* --------- ANSI/C++ compliant function prototypes --------------- */

#if defined(__STDC__) || defined(__cplusplus)

extern SPIN_ID  spinCreate();
extern void spinReset();
extern void spinItrpEnable();
extern void spinItrpDisable();
extern short spinStatReg();
extern short spinCntrlReg();
extern short spinIntrpMask();
extern void spinShow();
extern int getSetSpeed(long *speed, int *mode);
extern int setRate(long newRate);
extern int setSpeed(long newspeed);
extern int setMASThreshold(int threshold);
extern int setSpinRegDelta(int delta);

#else
/* --------- NON-ANSI/C++ prototypes ------------  */

extern SPIN_ID  spinCreate();
extern void spinReset();
extern void spinItrpEnable();
extern void spinItrpDisable();
extern short spinStatReg();
extern short spinCntrlReg();
extern short spinIntrpMask();
extern int getSetSpeed();
extern int setRate();
extern int setSpeed();
extern int setMASThreshold();
extern int setSpinRegDelta();
extern void spinShow();

#endif

#ifdef __cplusplus
}
#endif

#endif
