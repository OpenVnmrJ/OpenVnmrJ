/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef INCainterph
#define INCainterph

#include "stmObj.h"
#include "adcObj.h"

/* ------------- Make C header file C++ compliant ------------------- */
#ifdef __cplusplus
extern "C" {
#endif

#define HW_DELAY_FUDGE		3	/* num ticks to subtract from 	   */
					/* timebase ticks for 1st 100 nsec */
					/* e.g. 
/* Defines for Delay  with 100 MHZ Clock */
/* #define NUM_TICKS_SECOND	100000000	/* Num of 10ns ticks for  */
						/* one second.		  */
/* #define ONE_SECOND_DELAY	100000000-HW_DELAY_FUDGE /* HW number of  */
				 		/* ticks for one second.  */
/* #define MIN_DELAY_TICKS	10				*/
/*** END Defines for Delay  with 100 MHZ Clock ***/

/*** Defines for Delay  with 80 MHZ Clock ***/
#define NUM_TICKS_SECOND	80000000	/* Num of 12,5ns ticks for  */
						/* one second.		  */
/* #define ONE_SECOND_DELAY	80000000-HW_DELAY_FUDGE /* HW number of  */
#define ONE_SECOND_DELAY	80000000  	/* HW number of  */
				 		/* ticks for one second.  */
#define MIN_DELAY_TICKS		8
/*** END Defines for Delay  with 80 MHZ Clock ***/

#define STD_APBUS_DELAY		32	/* 400 ns std delay after apbus write */
#define READ_APBUS_DELAY        40      /* 500 ns std delay after apbus read */

/* Defines for APbus */
#define APWRITE		0x0000		/* APbus write bit for addr word */
#define APREAD		0x1000		/* APbus write bit for addr word */
#define APSTDLATCH 	0xff00		/* msByte for apbus byte writes */

/* defines for phasetable indicies */
#define IPHASEBITS	4	/* location in phase table */
#define IPHASEQUADRANT	5	/* location in phase table */
#define IPHASESTEP	6	/* location in phase table */
#define IPHASEPREC	7	/* location in phase table */
#define IPHASEAPADDR	8	/* location in phase table */
#define IPHASEAPDELAY	9	/* location in phase table */

/* --------- ANSI/C++ compliant function prototypes --------------- */

#if defined(__STDC__) || defined(__cplusplus)

/***** High Speed Lines	*****/
extern void clrhslines(int which_hs_lines);
extern void loadhslines(int which_hs_lines, unsigned int hslines);
extern void maskhslines(int which_hs_lines, unsigned int bits_to_set);
extern void unmaskhslines(int which_hs_lines, unsigned int bits_to_clear);

/***** delays	*****/
extern void delay(int seconds, unsigned long ticks);
extern void vdelay(long multval, long increment, long offset);
extern void incrdelay(long delayword[2], long multval);

/***** phase	*****/
extern int setphase(ACODE_ID pAcodeId, short tbl, int phaseincr);
extern int setphase90(ACODE_ID pAcodeId, short tbl, int incr);
extern void japsetphase(ushort apdelay, ushort apaddr,long phase[], 
						ushort channel );


/***** APbus	*****/
extern unsigned short *apbcout(unsigned short *intrpp);
extern void writeapword(ushort apaddr,ushort apval, ushort apdelay);
extern int aprtout(ushort apdelay, ushort apaddr, short maxval, short minval,
						 short offset, int apval);
extern int getapread(ushort apaddr, long *rtvar);
extern int japbcout(ushort apdelay,ushort apaddr,long data[],short apcount);
extern int japwfgout(ushort apdelay,ushort apaddr,long wfgaddr,short offset,
						long data[],short count);
extern int jsetfreq(ushort apdelay,ushort baseapaddr,long data[],short ptsno);

/***** Gradient and Waveform Generator	*****/
extern int gradient(ushort apdelay,ushort apaddr,int maxval,int minval,
								int value);
extern int vgradient(ushort apdelay,ushort apaddr,int maxval,int minval,
					int vmult,int incr,int base);
extern int incgradient(ushort apdelay,ushort apaddr,int maxval,int minval,
    int vmult1,int incr1,int vmult2,int incr2,int vmult3,int incr3,int base);
extern void wgload(ACODE_ID pAcodeId);
extern void jsetgradmatrix(ushort gradcomponent,long data[]);
extern void jsetoblgrad(ushort apdelay,ushort apaddr,ushort axis,ushort select);

/***** fifo stuffing *****/
extern int init_fifo(ACODE_ID pAcodeId, short flags, short size);

/***** tables	*****/
extern char *tbl_element_addr(TBL_ID *tblptr,short tindex,int index);
extern char *aptbl_element_addr(ushort *acbase,ushort tindex,int index);
extern char *jtbl_element_addr(TBL_ID *tblptr,short tindex,
						short nindex,long index[]);

/***** acquire	*****/
extern int acquire(short flags, int np, int dwell);
extern void receivercycle(long *activeRcvrs, int oph, int *dtmcntrl);
/* extern void enableOvrLd(long *activeRcvrs, int ssct, int ct, ulong_t *adccntrl); */
/* extern void disableOvrLd(long *activeRcvrs, ulong_t *adccntrl); */
extern void enableOvrLdN(long *activeRcvrs, int ssct, int ct, int *dtmcntrl, ulong_t *adccntrl);
extern void disableOvrLdN(long *activeRcvrs, int *dtmcntrl, ulong_t *adccntrl);
extern void enableStmAdcOvrLd(int ssct, int ct, int *dtmcntrl);
extern void disableStmAdcOvrLd(int *dtmcntrl);
extern void clearscandata(long *activeRcvrs, int *dtmcntrl);

/***** scans	*****/
extern void initscan(long *activeRcvrs, ACODE_ID pAcodeId, int np, int ss,
		     int *ssct, int nt, int ct, int bs, int cbs, int maxsum);
extern void nextscan(long *activeRcvrs, ACODE_ID pAcodeId, int np, int dp,
		     int nt, int ct, int bs, int *dtmcntrl, int fidnum);
extern void endofscan(long *activeRcvrs, int ss, int *ssct, int nt, int *ct,
		      int bs, int *cbs);
extern void acqi_updt(ACODE_ID pAcodeId,int acode,int *acqi_flag,int updtdelay);
extern void set_acqi_updt_cmplt(void);
extern void start_acqi_updt(ACODE_ID pAcodeId);
extern void signal_completion(ushort tagword);
extern void setscanvalues(long *activeRcvrs, ACODE_ID pAcodeId, int srcoffset,
			  int destoffset, int np, int *dtmcntrl);

/**** arrays 	****/
extern unsigned short *nextcodeset(ACODE_ID pAcodeId,int il,int *cbs,int nt,int ct);

/**** automation   ****/
extern void autoGain(ACODE_ID pAcodeId,short ntindx,short npindx,short rcvgindx,
     short apaddr,short apdly,short maxval,short minval,short stepval,
     short dpfindx,short adcbits,short ac_offset);
extern void autoShim(ACODE_ID pAcodeId,short ntindx,short npindx,short dpfindx,
    short maxpwr,short maxgain,short ac_offset,short *methodstr, short freeMethodStrFlag);


/**** misc   ****/
extern void init_stm(long *activeRcvrs, ACODE_ID pAcodeId, ulong_t numpoints, 
		     ulong_t numbytes, ulong_t tot_elem, int OneMHzInputFlag);
extern void init_adc(long *activeRcvrs,
		     unsigned short flags, ulong_t *adccntrl);
extern int calcRecvGainVals( int gain, ushort *pPreampAttn, ushort *pRset );
extern void receivergain(ushort apaddr, ushort apdelay, int gain);
extern unsigned short *settunefreq(unsigned short *intrpp);
extern void setpresig(ushort signaltype);


#else                                                   
/* --------- NON-ANSI/C++ prototypes ------------  */

extern void clrhslines();
extern void loadhslines();
extern void maskhslines();
extern void unmaskhslines();

extern void delay();
extern void vdelay();
extern void incrdelay();

extern int setphase();
extern int setphase90();
extern void japsetphase();

extern void apbcout();
extern void writeapword();
extern int aprtout();
extern int getapread();
extern int japbcout();
extern int japwfgout();
extern int jsetfreq();

extern int gradient();
extern int vgradient();
extern int incgradient();
extern void wgload();
extern void jsetgradmatrix();
extern void jsetoblgrad();

extern int init_fifo();

extern char *tbl_element_addr();
extern char *aptbl_element_addr();
extern char *jtbl_element_addr();

extern int acquire();
extern void receivercycle();
extern void enableOvrLd();
extern void disableOvrLd();
extern void clearscandata();

extern void initscan();
extern void nextscan();
extern void endofscan();
extern void acqi_updt();
extern void signal_completion();

extern unsigned short *nextcodeset();

extern void autoGain();
extern void autoShim();

extern void init_stm();
extern void init_adc();
extern int calcRecvGainVals();
extern void receivergain();
extern unsigned short *settunefreq();
extern void setpresig();

#endif

#ifdef __cplusplus
}
#endif

#endif

