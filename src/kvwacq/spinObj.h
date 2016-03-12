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
/*
 */

#ifndef INCspinobjh
#define INCspinobjh


/* ------------- Make C header file C++ compliant ------------------- */
#ifdef __cplusplus
extern "C" {
#endif

/* VT Constants */
#define VTOFF   30000   /* temp value if VT controller is tobe passive */
#define VT_NONE    0       /* No VT controller */
#define  LSDV_VTBIT1		0x800   /* 11 */
#define  LSDV_VTBIT2		0x1000   /* 12 */
#define  VT_NORMAL_UPDATE	3 /* second */
#define  VT_WAITING_UPDATE	1 /* second */

/* Commands to for SPIN decode */
#define SPIN_INIT 	1
#define SPIN_RESET 	2
#define SPIN_WAIT4REG	3
#define SPIN_SETPID	4

typedef struct {
	 int VTCmd;
	 int VTArg1;
	 int VTArg2;
} SPIN_CMD;

typedef struct {
	char *pSID;		/* SCCS ID */
	int spinSetSpeed;	/* requested speed */
	int spinTruSpeed;	/* actual speed */
	int spinModeMask;	/* regulated, waiting, etc. */
	int spinPort;		/* serial port of Spin */
	int spinInterlock;	/* in='y' or 'n' 0=no 1=enable 2=ready */
	int spinType;		/* 1=simple 2 = Auto */
	int spinStat;		/* Spin Status */
	int spinErrorType;	/* HARD_ERROR (15) == Error, */
				/* WARNING_MSG(14) == Warning */
	int spinError;		/* VT Error Number */
	int VTRegCnt;		/* Internal Counter used to */
				/* determine Oxford VTs regulation */
	ulong_t VTRegTimeout;	/* Time to allow for VT regulation */
				/* before reporting Error */
	int spinUpdateRate;	/* Update Rate is in seconds */
        MSG_Q_ID pSpinMsgQ;	/* Message Queue for Spin server */
	SEM_ID pSpinMutex;	/* Spin serial port mutual exclusion */
	SEM_ID pSpinRegSem;	/* Spin Reg Sem */
	char SPINIDSTR[129];	/* SpinVT Version string */
} SPIN_OBJ;

typedef SPIN_OBJ *SPIN_ID;

/* --------- ANSI/C++ compliant function prototypes --------------- */

#if defined(__STDC__) || defined(__cplusplus)

extern int setVT(int temp, int ltmpxoff, int pid);
extern int wait4VT(int time);
extern void resetVT(void);
extern int VTchk(void);
extern void setVT_LSDVbits(void);

#else
/* --------- NON-ANSI/C++ prototypes ------------  */

extern int setVT();
extern int wait4VT();
extern void resetVT();
extern int VTchk();
extern void setVT_LSDVbits();

#endif

#ifdef __cplusplus
}
#endif

#endif

