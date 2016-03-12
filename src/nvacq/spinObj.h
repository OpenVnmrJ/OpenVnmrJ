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

#ifndef INCspinh
#define INCspinh


/* ------------- Make C header file C++ compliant ------------------- */
#ifdef __cplusplus
extern "C" {
#endif

/*   Defines Here */

#define	VERTICAL_PROBE 1    /* Control a Vertical Probe */
#define	MAS_PROBE      2    /* Control a MAS Probe */
#define	MAS_THRESHOLD 80    /* Default switch over threshold for MAS control */

#define RATE_MODE	1   /* setRate() has been used, constant air flow */
#define SPEED_MODE	2   /* setSpeed() has been used, speed regulation */

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
	char* pIdStr;        /* Identifier String (spinCreate) */
	unsigned short MASdac;    /* current value in MAS DAC */
        int   bearLevel;     /* on level for bearing air */
	int   bearDAC;	     /* 16-bit DAC value for bearing */
	int   driveDAC;	     /* 16-bit DAC value for drive */
	int   ejectDAC;	     /* 16-bit DAC value for eject/slow drop */
	int   SPNitrHndlr;   /* pointer to SpinUpdate handler */
	int   SPNmasthres;   /* threshold when to switch to MAS control. */
	int   SPNtype;       /* 0=unknown, 1=liquids, 2=tach, 3=mas */
	int   SPNrevs;       /* current revolutions/interrupt */
        int   SPNmode;	     /* in Rate (air flow) or Speed regualtion mode */
	long  SPNheartbeat;  /* inc by (intrp svc) */
	long  SPNeeg1;       /* check interrupts (chkSpeed) */
	long  SPNeeg2;       /* check interrupts (chkSpeed) */
	long long SPNsum;    /* current box car value of periods (intrp svc)*/
	long long SPNperiod; /* SPNsum/SPNnbr (chkspeed) */
	long  SPNregulate;   /* calculated from speedSet (setspeed) */
	long  SPNcount;      /* how many times thru the table (intrp svc) */
	long  SPNnbr;        /* size of SPNperiod (spinCreate) */
	long  SPNindex;      /* index to last/next period (intrp svc) */
	long  SPNspeed;      /* calculated speed (int) from SPNsum (chkspeed) */
	long  SPNspeedfrac;  /* calculated speed (fraction) see above */
	long  SPNspeedSet;   /* speed to regulate to (setspeed) */
	long  SPNrateSet;    /* Air Flow Rate,  value depends LIQ or MAS */
	long  SPNlerror;     /* last error value (spnPID) */
	long  SPNtolerance;  /* regulation tolerance (setSpeed) */
	long  SPNproportion; /* Proportion for PID (spnPID) */
	long  SPNintegral;   /* running Integral for PID (spnPID) */
	long  SPNderivative; /* Derivative for PID (spnPID) */
	long  SPNpefact;     /* PeriodError divisor */
	long  SPNsetting;    /* current air off time setting */
	long  PIDcorrection; /* correction to current air off time */
	long  PIDmode;       /* mode > 1 fix integral, 0=normal */
	long  PIDheartbeat;  /* for debug heartbeat of PID routine */
	long  PIDerrorbuf[PID_ERROR_BUFSIZE]; /* last 4 errors for Integral Er*/
	long  SpinLED;       /* state 0=off, 1=|__, 2=_||, 3=ON */
	long  SPNkO;         /* multiplier for PID calc to off time */
	long  SPNkP[2];      /* 0=slow P, 1=fast P (spinCreate/setLparam) */
	long  SPNkI[2];      /* 0=slow I, 1=fast I (spinCreate/setLparam) */
	long  SPNkD[2];      /* 0=slow D, 1=fast D (spinCreate/setLparam) */
	long  SPNkC[2];      /* max Change in air flow per adjustment period */
	long  SPNkF;         /* a divisor as poor mans fractional PID */
	long long  SPNperiods[SPNperiodSize]; /* table of periods (intrp) */
	int   PIDerrindex;  /* index into error buffer */
	int   PIDelements;    /* Total number of elements in PIDE */
	int   PIDposition;    /* current element to be filled */
	PIDE  pPIDE;          /* 0=no struct allocated else pntr to struct */ 
	int   SPNtaskid1;     /* contains a taskid for debugging */
	int   Vejectflag;     /* Eject has occured */
	int   Vdroptime;      /* Delay in sec between Eject/SDROP off */
	int   Vdropdelay;     /* Delay in sec between SDROP Off and regulate*/
	int   SPNtomb;        /* The special Tom Barbara diag print flag */
	int   SPNgad;         /* The special Glen Diestelhorst diag print flg */
			      /* The following are used in MAS rotor to track
				 changes that would indicate a rotor that can
				 not achieve speed do to air or rotor crash.
			       */
	long PrevDacVal;      /* Preceding MAS DAC setting */
	long PrevDacDelta;  /* The change in MAS DAC setting  */
	long PrevSpeed;     /* The rotor speed from previous chkSpeed */
	long SPNdelaySet;   /* speed to regulate to (setspeed) */
} SPIN_OBJ;

typedef SPIN_OBJ *SPIN_ID;
#define	UPk        0x1  /* index into PID constants for faster */
#define	DOWNk      0x0  /* index into PID constants for faster */

/* ...REVS is the number of revolutions per interrupt */
/* REVS is used in a caluclation of speed so must be tied to reg. pattern */
#define	VMC_REVS 1	/* One revolution per interrupt */
#define	MASMC_REVS 250	/* 250 revolutions per interrupt */

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
extern int setSpinnerType(int spinner);
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
extern void setSpinnerType();
extern int setSpinRegDelta();
extern void spinShow();

#endif
extern void resetLastSpinSpeed();

#ifdef __cplusplus
}
#endif

#endif
