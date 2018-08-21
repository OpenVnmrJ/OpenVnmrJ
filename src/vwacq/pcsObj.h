/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef INCpcsObjh
#define INCpcsObjh

#include "rngLLib.h"
#include "fifoObj.h"

#ifndef VXWORKS
/* if not vxworks then stub DPRINT with printfs   */
#define DPRINT(level, str) \
        printf(str)

#define DPRINT1(level, str, arg1) \
        printf(str,arg1)

#define DPRINT2(level, str, arg1, arg2) \
        printf(str,arg1,arg2)
 
#define DPRINT3(level, str, arg1, arg2, arg3) \
        printf(str,arg1,arg2,arg3)
 
#define DPRINT4(level, str, arg1, arg2, arg3, arg4) \
        printf(str,arg1,arg2,arg3,arg4)
 
#define DPRINT5(level, str, arg1, arg2, arg3, arg4, arg5 ) \
        printf(str,arg1,arg2,arg3,arg4,arg5)
 
#define DPRINT6(level, str, arg1, arg2, arg3, arg4, arg5, arg6 ) \
        printf(str,arg1,arg2,arg3,arg4,arg5,arg6)
 
#define DPRINT7(level, str, arg1, arg2, arg3, arg4, arg5, arg6, arg7 ) \
	  printf(str,arg1,arg2,arg3,arg4,arg5,arg6,arg7)
 
#define DPRINT8(level, str, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8 ) \
	  printf(str,arg1,arg2,arg3,arg4,arg5,arg6,arg7,arg8)
 
#define DPRINT9(level, str, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9 ) \
	  printf(str,arg1,arg2,arg3,arg4,arg5,arg6,arg7,arg8,arg9)
 
#endif

/* ------------- Make C header file C++ compliant ------------------- */
#ifdef __cplusplus
extern "C" {
#endif

/* HIDDEN */

/* typedefs */

#define MAXSRCBUFFERS 16

typedef struct __pcsobj          /* RING_BLKING - blocking ring buffer */ 
    {
    char*	pPcSortIdStr;
	int numSrcs;		/* number on input parallel channels */
	RINGL_ID pFreeBufList;	/* List of Free Buffer that can be used */
	RINGL_ID SrcBufs[MAXSRCBUFFERS];
	RINGL_ID DstBuf;	/* only can be one Destination Buffer */
				/* Being none Null means this is an Embedded */
				/* Parallel Channel */
	FIFO_ID pFifoId; 	/* Non-Embedded Parallel generates actual fifo */
				/* words */

        int presentChanNum;		/* present input channel */
        RINGL_ID pInputBuf;

        int presentPChan;	/* present sorting PChan */
        long wordLatch[MAXSRCBUFFERS*2];
        long carryOver[MAXSRCBUFFERS];	/* the negative delay goes here to be removed from next delay obtained. */
        int pchanEmpty[MAXSRCBUFFERS];
	long prevDelay[2];
        unsigned long prevHSLines;
	long TimeIndex;
	long FifoTI;
	long ChanTimeAccum[MAXSRCBUFFERS];
	long ChanTimeIndex[MAXSRCBUFFERS];
        int numPChanEmpty;
        unsigned long GateWord;  
        long delay;
        long ApBusCntDwn;
        long presentMinDelay;
        int presentMinChan;
        int AcqLpCnt;
	unsigned long AcqAccumTime;

	struct __pcsobj *Parent;

    } PCHANSORT_OBJ;

/* END_HIDDEN */

typedef PCHANSORT_OBJ *PCHANSORT_ID;

#define PCHAN_GATE_ON  0xFF000000L	/* parallel channel Pseudo Gate Commands */
#define PCHAN_GATE_OFF 0xFE000000L
#define  PCHAN_CHAN_LOCK 0xFD000000L 
#define  PCHAN_CHAN_UNLOCK 0xFC000000L 
#define  PCHAN_SETOBLMATRIX 0xFB000000L 
#define  PCHAN_SETOBLGRADZ 0xFA000000L 
#define  PCHAN_SETOBLGRADY 0xF9000000L 
#define  PCHAN_SETOBLGRADX 0xF8000000L 
#define  PCHAN_ACQUIRESTRT 0xF7000000L 
#define  PCHAN_ACQUIREEND 0xF6000000L 
#define  PCHAN_LOOP_CNT    0x06000000L 
#define  PCHAN_STLOOP_CTC    0x0C000000L 
#define  PCHAN_ENDLOOP_CTC    0x0A000000L 
#define  PCHAN_CTC    0x08000000L 

#define STD_APBUS_DELAY		32	/* 400 ns std delay after apbus write */
#define PFG_APBUS_DELAY		72	/* 900 ns delay after pfg apbus write */
#define PFGL200_APBUS_DELAY    232	/* 2.9 us delay after triax apbus write */

/* Defines for APbus take from A_interp.h */
#define APWRITE		0x0000		/* APbus write bit for addr word */
#define APREAD		0x1000		/* APbus write bit for addr word */
#define APSTDLATCH 	0xff00		/* msByte for apbus byte writes */

/* For the oblique gradient settings all values will be scaled to 	*/
/* 20 bits and then right shifted to their proper values.  So max and	*/
/* min values will be defined from 20 bits.				*/
#define OBLGRAD_BITS2SCALE 20
#define GRADMAX_20	1048575
#define GRADMIN_20	-1048576

/* Gradient type selection information for SpinCad OblGradient */

#define  SEL_WFG	0x0000
#define  SEL_PFG1	0x1000
#define  SEL_PFG2	0x2000
#define  SEL_TRIAX	0x3000

#define WFG_BITS	16
#define PFG1_BITS	12
#define PFG2_BITS	20
#define TRIAX_BITS	16

#define  PFG2_AMP_ADDR_REG	0x0
#define  PFG2_AMP_VALUE_REG	0x1
#define  TRIAX_AMP_ADDR_REG	0x2	/* note: in psg this is TRIAX_AMP_RESET_REG */
#define  TRIAX_AMP_VALUE_REG	0x0


/* --------- ANSI/C++ compliant function prototypes --------------- */
/* function declarations */

#if defined(__STDC__) || defined(__cplusplus)

extern RINGL_ID initializeSorterBufs(int maxChans,int MaxSize);
 
extern PCHANSORT_ID pchanSortCreate(int numChans,char *IdStr, RINGL_ID FreeBufs, PCHANSORT_ID parent,FIFO_ID fifoobj);
extern void pchanSortDelete(PCHANSORT_ID pcsId);
extern void pchanStart(PCHANSORT_ID pPCSId, int chanNumber);
extern pchanPut(PCHANSORT_ID pcsId, unsigned long Cntr, unsigned long Ticks);
extern RINGL_ID  pchanGetActiveChanBuf(PCHANSORT_ID pPCSId);
extern PCHANSORT_ID  pchanGetParent(PCHANSORT_ID pPCSId);
extern void pchanSort(PCHANSORT_ID pcsId);

/* Oblique Gradient routines */
extern void clearOblGradMatrix();
extern void setoblgrad(unsigned short axis,unsigned long tmpval);
extern void setgradmatrix(unsigned long tmpval);
extern int oblgradient(unsigned short apdelay,unsigned short apaddr,int maxval,int minval,
								int value);
extern void owriteapword(unsigned short apaddr,unsigned short apval, unsigned short apdelay);

/*

IMPORT    int          pcSortBufPut(PCHANSORT_ID pcsortId, int pchanNum,unsigned long Cmd, unsigned long ticks);
*/
 
/* --------- NON-ANSI/C++ prototypes ------------  */

#else
 
#endif  /* __STDC__ */
 
#ifdef __cplusplus
}
#endif

#endif /* INCrngBlkLibh */
