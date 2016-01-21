/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef INCshrstatinfoh
#define INCshrstatinfoh

#include <time.h>
#include <sys/time.h>

#include "hostAcqStructs.h"

#define EXPSTAT_STR_SIZE 128

typedef struct timeval TIMESTAMP;

/* 
   This is the shared Status Structure of the Acqusition System
   that is updated by various processes and displayed in user form
   by Acqstat or Equivient.
*/

/*===============================================================*/

typedef struct _statstruct_ {
		        TIMESTAMP TimeStamp;
                    	long  CompletionTime;
                    	long  RemainingTime;
                    	long  DataTime;
                    	long  ExpTime;
                    	long  StartTime;
                    	long  ExpInQue;
	       unsigned long  CT;
               unsigned long  FidElem;
                    	long  Sample;
			int   GoFlag;
			int   SystemVerId;
			int   InterpVerId;
			/* Experiment Acquiring Data  */
                    	char  ExpIdStr[EXPSTAT_STR_SIZE];
                    	char  UserID[EXPSTAT_STR_SIZE];
                    	char  ExpID[EXPSTAT_STR_SIZE];
			/* Experiment Processing Data  */
                    	char  ProcExpIdStr[EXPSTAT_STR_SIZE];
                    	char  ProcUserID[EXPSTAT_STR_SIZE];
                    	char  ProcExpID[EXPSTAT_STR_SIZE];
                        char  ReportAcqState[EXPSTAT_STR_SIZE];
				        /* Idle, Acquiring, VT Regulation, 
					 Spin Regulation, Auto Set Gain, 
					 Auto Locking, Lock: Find Res., 
					 Shimming, Changing Sample, 
					 Interactive, Tuning, Inactive */

			CONSOLE_STATUS	csb;
                } EXP_STATUS_STRUCT;

typedef EXP_STATUS_STRUCT *EXP_STATUS_INFO;


/*===============================================================*/
/*===============================================================*/

/* --------- ANSI/C++ compliant function prototypes --------------- */
 
#if defined(__STDC__) || defined(__cplusplus)
 
extern int initExpStatus(int clean);   /* zero out Exp Status */

extern void getStatTimeStamp(TIMESTAMP *time);
extern int isStatTimeStampNew(TIMESTAMP *time);

extern int getStatUserId(char *usrname,int maxsize);
extern int setStatUserId(char *usrname);
extern int getStatExpName(char *expname,int maxsize);
extern int setStatExpName(char *expname);
extern int getStatExpId(char *expidstr,int maxsize);
extern int setStatExpId(char *expidstr);
extern int getStatProcUserId(char *usrname,int maxsize);
extern int setStatProcUserId(char *usrname);
extern int getStatProcExpName(char *expname,int maxsize);
extern int setStatProcExpName(char *expname);
extern int getStatProcExpId(char *expidstr,int maxsize);
extern int setStatProcExpId(char *expidstr);
extern int getStatGoFlag();
extern int setStatGoFlag(int goflag);
extern unsigned long getStatCT();
extern int setStatCT(unsigned long ct);
extern int setStatElem(unsigned long elem);
extern int setStatDataTime();
extern int getSystemVerId();
extern int setSystemVerId(int version);
extern int getInterpVerId();
extern int setInterpVerId(int version);

extern int getStatAcqState();
extern int setStatAcqState(int status);
extern long getStatAcqCtCnt();
extern int setStatAcqCtCnt( long ctCnt );
extern long getStatAcqFidCnt();
extern int setStatAcqFidCnt( long fidCnt );
extern int getStatLockFreq1();
extern int getStatLockFreq2();
extern int getStatLkLevel();
extern int setStatLkLevel(int lklev);
extern int getStatLkGain();
extern int setStatLkGain(int lkval);
extern int getStatLkPower();
extern int setStatLkPower(int lkval);
extern int getStatLkMode();
extern int getStatLkPhase();
extern int setStatLkPhase(int lkval);
extern int getStatSpinAct();
extern int setStatSpinAct(int spin_val);
extern int getStatVTAct();
extern int getStatVTSet();
extern int getStatVTC();
extern int getStatPneuBearing();
extern int getStatPneuVtAir();
extern int getStatPneuVTAirLimits();
extern int getStatPneuSpinner();
extern int getStatShimValue(int dacnum);
extern int setStatShimValue(int dacnum, int value);
extern int getStatShimSet();

#else
/* --------- NON-ANSI/C++ prototypes ------------  */

extern int initExpStatus();   /* zero out Exp Status */

extern void getStatTimeStamp();
extern int isStatTimeStampNew();

extern int getStatUserId();
extern int setStatUserId();
extern int getStatExpName();
extern int setStatExpName();
extern int getStatExpId();
extern int setStatExpId();
extern int getStatProcUserId();
extern int setStatProcUserId();
extern int getStatProcExpName();
extern int setStatProcExpName();
extern int getStatProcExpId();
extern int setStatProcExpId();
extern int getStatGoFlag();
extern int setStatGoFlag();
extern unsigned long getStatCT();
extern int setStatCT();
extern int setStatElem();
extern int setStatDataTime();
extern int getSystemVerId();
extern int setSystemVerId();
extern int getInterpVerId();
extern int setInterpVerId();

extern int getStatAcqState();
extern int setStatAcqState();
extern long getStatAcqCtCnt();
extern int setStatAcqCtCnt();
extern long setStatAcqFidCnt();
extern int setStatAcqFidCnt();
extern int getStatLkLevel();
extern int setStatLkLevel();
extern int getStatLkGain();
extern int setStatLkGain();
extern int getStatLkPower();
extern int setStatLkPower();
extern int getStatLkPhase();
extern int setStatLkPhase();
extern int getStatSpinAct();
extern int setStatSpinAct();
extern int getStatVTAct();
extern int getStatVTSet();

extern int getStatShimValue();
extern int setStatShimValue();
extern int getStatShimSet();

#endif
 
#ifdef __cplusplus
}
#endif
 
#endif
