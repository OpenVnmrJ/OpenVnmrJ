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
					/* e.g.  */

#define NUM_TICKS_SECOND	0x43e8		/* Num of 12,5ns ticks for  */
						/* one second.		  */
#define ONE_SECOND_DELAY	0x43E8		/* HW number of  */
				 		/* ticks for one second.  */
#define MIN_DELAY_TICKS		8


#define STD_APBUS_DELAY		50	/* std delay after apbus write	*/

/* Defines for APbus */
#define APWRITE		0x0000		/* APbus write bit for addr word */
#define APREAD		0x1000		/* APbus write bit for addr word */
#define AP_WRITE_BYTE	0		/* APbus mode for byte writes */
#define AP_WRITE_WORD	1		/* APbus mode for word writes */
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


/***** APbus	*****/
extern unsigned short *apbcout(unsigned short *intrpp);
extern void writeapword(ushort apaddr,ushort apval, ushort apdelay);
extern int aprtout(ushort apdelay, ushort apaddr, short maxval, short minval,
						 short offset, int apval);
extern int getapread(ushort apaddr, long *rtvar);

/***** Gradient and Waveform Generator	*****/
extern int gradient(ushort apdelay,ushort apaddr,int maxval,int minval,
								int value);
extern int vgradient(ushort apdelay,ushort apaddr,int maxval,int minval,
					int vmult,int incr,int base);
extern int incgradient(ushort apdelay,ushort apaddr,int maxval,int minval,
    int vmult1,int incr1,int vmult2,int incr2,int vmult3,int incr3,int base);
extern void wgload(ACODE_ID pAcodeId);

/***** fifo stuffing *****/
extern int init_fifo(ACODE_ID pAcodeId, short flags, short size);

/***** tables	*****/
extern char *tbl_element_addr(TBL_ID *tblptr,short tindex,int index);
extern char *aptbl_element_addr(ushort *acbase,ushort tindex,int index);

/***** acquire	*****/
extern int acquire(short flags, int np, int dwell);
extern void receivercycle(int oph, int *dtmcntrl);
extern void enableOvrLd(int ssct, int ct, ulong_t *adccntrl);
extern void disableOvrLd(ulong_t *adccntrl);
extern void enableStmAdcOvrLd(int ssct, int ct, int *dtmcntrl);
extern void disableStmAdcOvrLd(int *dtmcntrl);
extern void clearscandata(int *dtmcntrl);

/***** scans	*****/
extern void initscan(ACODE_ID pAcodeId, int np, int ss, int *ssct, 
				int nt, int ct, int bs, int cbs, int maxsum);
extern void nextscan(ACODE_ID pAcodeId, int np, int dp, int nt, int ct, 
					int bs, int *dtmcntrl, int fidnum);
extern void endofscan(int ss, int *ssct, int nt, int *ct, int bs, int *cbs);
extern void acqi_updt(ACODE_ID pAcodeId,int acode,int *acqi_flag,int updtdelay);
extern void set_acqi_updt_cmplt(void);
extern void start_acqi_updt(ACODE_ID pAcodeId);
extern void signal_completion(ushort tagword);
extern void setscanvalues(ACODE_ID pAcodeId,int srcoffset,int destoffset,
							int np, int dtmcntrl);

/**** arrays 	****/
extern unsigned short *nextcodeset(ACODE_ID pAcodeId,int il,int *cbs,int nt,int ct);

/**** misc   ****/
extern void init_stm(ACODE_ID pAcodeId, STMOBJ_ID pStmId, ulong_t numpoints, 
				ulong_t numbytes, ulong_t tot_elem);
extern void init_adc(ADC_ID pAdcId, unsigned short flags,
			 ulong_t *adccntrl, int ss);
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

extern void apbcout();
extern void writeapword();
extern int aprtout();
extern int getapread();

extern int gradient();
extern int vgradient();
extern int incgradient();
extern void wgload();

extern int init_fifo();

extern char *tbl_element_addr();
extern char *aptbl_element_addr();

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

