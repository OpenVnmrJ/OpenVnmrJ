/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/* #define _POSIX_SOURCE /* defined when source is waited tobe POSIX-compliant */
/* #define _SYSV_SOURCE /* defined when source is System V */
/* #ifdef __STDC__ /* used to determine if using an ANSI compiler */


#ifndef INCtuneObjh
#define INCtuneObjh


/* ------------- Make C header file C++ compliant ------------------- */
#ifdef __cplusplus
extern "C" {
#endif

#define  MAX_TUNE_CHAN  7  /* Hardware provides only three lines for channel switch
                                                     four lines for atten. switch
                              so 7 tune channels are available (1-7)
                              "0" is non-tune position (this is the normal operation)
                           */

#define  MAX_CHANNELS    8            
#define  MAX_FREQ_CODES		17
#define  JPSGFLAG	0x0100	/* 256 */


/*************************** Matt's idea *****************************************/
/*********************************************************************************/

typedef struct {
             short int   ptsapb[MAX_FREQ_CODES];
             short int   band;
             short int	 type;
} tuneFreq;

typedef struct {
             tuneFreq  tuneChannel[ MAX_CHANNELS ];
	     SEM_ID    pSemAccessFIFO;
	     SEM_ID    pSemOK2Tune;
             /* can be more other things from this line for tune object */
} TUNE_OBJ;

typedef TUNE_OBJ  *TUNE_ID;

/*********************************************************************************/
/*********************************************************************************/


/* --------- ANSI/C++ compliant function prototypes --------------- */

#if defined(__STDC__) || defined(__cplusplus)

extern TUNE_ID  tuneCreate(void);
extern void tuneShow(TUNE_ID pTuneId,int level);
extern void tuneStart(int channel, int attenuation);

#else
/* --------- NON-ANSI/C++ prototypes ------------  */

extern TUNE_ID tuneCreate();
extern void tuneShow();
extern void tuneStart();

#endif

#ifdef __cplusplus
}
#endif

#endif INCtuneObjh
