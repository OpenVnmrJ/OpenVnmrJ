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
#ifndef INCvtfuncsh
#define INCvtfuncsh


/* ------------- Make C header file C++ compliant ------------------- */
#ifdef __cplusplus
extern "C" {
#endif

/* VT Constants */
#define VTOFF			30000   /* temp value if VT is to be passive */
#define VT_NONE			0       /* No VT controller */
#define LSDV_VTBIT1		0x800   /* 11 */
#define LSDV_VTBIT2		0x1000   /* 12 */
#define LSDV_EBITS		0xe000   /* 13,14,15 */
#define VT_NORMAL_UPDATE	3	/* second */
#define VT_WAITING_UPDATE	1	/* second */

/* Commands to VT */
#define VT_INIT 	10
#define VT_RESET 	11
#define VT_SETTEMP 	12
#define VT_SETPID 	13
#define VT_GETTEMP 	14
#define VT_GETSTAT 	15
#define VT_WAIT4REG	16
#define VT_SETSLEW	17	/* HighLand only */
#define VT_SETCALIB	18	/* HighLand only */
#define VT_GETHEATPWR	19	/* HighLand only */
#define VT_GETSWVER	20	/* HighLand only */
/*********************************************/
  /* these are UNUSED ON MERCURY */
#define VT_SET_INTERLOCK 21    
#define VT_SET_ERRORMODE 22
#define VT_SET_TYPE      23
  /* these are new and under development */
#define VT_SET_RANGE     24
#define VT_SET_SLEW      25


typedef struct {
		 int VTCmd;
		 int VTArg1;
		 int VTArg2;
#ifndef MERCURY
                 int VTArg3;
#endif
	       } VT_CMD;



#ifdef __cplusplus
}
#endif

#ifndef MERCURY
/* status bits... */
#define VT_MANUAL_ON_BIT 0x01	
#define VT_HEATER_ON_BIT 0x02
#define VT_GAS_ON_BIT    0x04
#define VT_SPONSTAT_BIT  0x08
#define VT_WAITING4_REG  0x10
#define VT_IS_REGULATED  0x20
#define VT_TEMP_IS_SET   0x40
#endif

#define CR 13
#define CMD 0
#define TIMEOUT 98
#define CMDTIMEOUT -10000

#ifndef MERCURY
typedef struct {
	int VTSetTemp;	/* setting of VT */
	int VTTruTemp;	/* present temperature of VT */
	int VTTempRate;	/* Rate in degrees/minute to raise/lower temp */
        int VTRange;    /* regulation range */
	int VTModeMask;	/* active, manual, passive, ,regulated, waiting, etc. */
	int VTLTmpXovr; /* Low Temp Cross Over for cooling gas */
	int VT_PID;     /* PID of VT  */
	int VTPrvTemp;  /* Previous Temp in Ramp */
	int VTpresent;	/* VT present on system */
	int VTport;	/* serial port of VT */
	int VTinterLck; /* tin= 'y' or 'n' 0 = no, 1 = enable, 2 = ready*/
	int VTtype;	/* 0 == NONE, 2 = Oxford */
	int VTstat;	/* VT Status */
	int VTerrorType;/* HARD_ERROR (15)==Error, WARNING_MSG(14)==Warning */
	int VTerror;	/* VT Error Number */
	int VTRegCnt;   /* Internal Counter determines regulation for VT */
	int VTUpdateRate;  /* Update Rate is in seconds */
	int VTAirFlow;	/* setting of DAC air flow */
        int VTAirLimits;   /* Air flow detection limits */
	ulong_t VTRegTimeout; /* Time for VT regulation before Error */
	MSG_Q_ID pVTMsgQ;  /* Message Queue for VT server */
	SEM_ID pVTFlowSem; /* VT Air Flow Semaphore */
	SEM_ID pVTmutex;   /* VT serial port mutual exclusion */
	SEM_ID pVTRegSem;  /* VT Reg Sem */
        char VTIDSTR[129]; /* VT Version string,
			   /* Oxford starts 'VTC, Version 5.01' */
			   /* HighLand starts 'HIGHLAND... PROGRAM 26901B...'*/
} VT_OBJ;

typedef VT_OBJ *VT_ID;
#endif

#if (CPU != CPU32)
/* --------- ANSI/C++ compliant function prototypes --------------- */

#if defined(__STDC__) || defined(__cplusplus)

extern int   setVT(int temp, int ltmpxoff, int pid);
extern int   wait4VT(int time);
extern void  resetVT(void);
extern int   VTchk(void);
extern void  setVT_LSDVbits(void);
extern VT_ID vtCreate(void);
extern void  vtTask(void);
extern void  vtShow(void);
extern void  wait4VTAirFlow(void);
extern void  setVTAirFlow(int);
extern void  setVTAirLimit(int);


#else
/* --------- NON-ANSI/C++ prototypes ------------  */

extern int   setVT();
extern int   wait4VT();
extern void  resetVT();
extern int   VTchk();
extern void  setVT_LSDVbits();
extern VT_ID vtCreate();
extern void  vtTask();
extern void  vtShow();
extern void  wait4VTAirFlow();
extern void  setVTAirFlow();
extern void  setVTAirLimit();

#endif

#ifdef __cplusplus
}
#endif

#endif
#endif

