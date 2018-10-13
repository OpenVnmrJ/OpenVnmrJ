/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#ifndef FFKEYS_H
#define FFKEYS_H

#define TIMER_MASK (0x03ffffff)
/******* these are the REGISTER LAYER KEYS **********/
/** these are updated to Match Pete Johnsons definitions */
/* a few general defines */
/* these should be resolved with nvacq/nvhardware.h */

#define LATCHKEY           (0x80000000)
#define SPIKEY             (1 << 30)
#define DURATIONKEY        (1 << 26)
#define GATEKEY            (2 << 26)
#define MXGATE             (3 << 26)
#define USER               (8 << 26)
#define AUX                (9 << 26)

/* GATE FIELDS */
/* critical (arg) ! */
#define RFGATEON(arg)  (((arg) << 12) | (arg))
#define RFGATEOFF(arg) ((arg) << 12) 
#define R_LO_NOT     (0x1)
#define X_LO         (0x2)
#define X_OUT	     (0x4)
#define X_IF	     (0x8)
#define RF_TR_GATE    (0x10)
#define RFAMP_UNBLANK (0x20)
#define RFUNBLANK_TRGATE (RFAMP_UNBLANK | RF_TR_GATE)

/*
#define RF_STD_XMTR    (R_LO_NOT | X_LO | X_OUT | X_IF ) 
#define RF_DEC_XMTR    (X_LO | X_OUT | X_IF ) 
*/

#define RF_OBS_PRE_LIQUIDS   (RFUNBLANK_TRGATE | X_IF | R_LO_NOT)
#define RF_OBS_PULSE_LIQUIDS (X_LO | X_OUT)
#define RF_OBS_POST_LIQUIDS  (X_IF | R_LO_NOT)

#define RF_DEC_PRE_LIQUIDS   (RFUNBLANK_TRGATE | X_IF)
#define RF_DEC_PULSE_LIQUIDS (X_LO | X_OUT)
#define RF_DEC_POST_LIQUIDS  (X_IF)

#define RF_MIXER      (0x40)
#define RF_CW_MODE    (0x80)
#define RF_SWINT1    (0x100)
#define RF_SWINT2    (0x200)
#define RF_SWINT3    (0x400)
#define RF_SWINT4    (0x800)

#define AUXFREQ0     (0x000)
#define AUXATTEN     (0x600)
#define AUXTCOMP     (0x500)  // controls ICAT temperature compensation

/* MASTER SPECIFIC */
/* reference only 0x80 master sync bit 0x40 ROTOR_SYNC */
/* these are full encoded
 * #define BLAF_ON     (0x20020)
 * #define BLAF_OFF    (0x20000)
 */

/* registers */
/* no existing better def */
#define ROTOSYNC           (0x0C000000)
#define MRTSHIMLSB	   (AUX + 0x000)
#define MRTSHIMMSB	   (AUX + 0x100)
#define MRTSHIMID	   (AUX + 0x200)
#define MRTID    	   (AUX + 0x300)
#define LOCKTCPLUS    	   (AUX + 0x400)
#define LOSELECT    	   (AUX + 0x500)
/* Lock TC masks WAG */
#define LOCKHOLD           (8)
#define LOCKTRACK          (0)
#define LOCKTC(arg)        (arg & 3) 

/* RF SPECIFIC */
#define RFPHASEKEY         (4 << 26)
#define RFPHASECYCLEKEY    (5 << 26)
#define RFAMPKEY           (6 << 26)
#define RFAMPSCALEKEY      (7 << 26)
#define RFUSERKEY          (8 << 26)

#define RFPHASEWORD(phasebits)  (RFPHASEKEY | (phasebits & 0x0ffff))
#define RFPHASECYCLEWORD(phasebits)  (RFPHASECYCLEKEY | (phasebits & 0x0ffff))
#define RFPHASEINCWORD(phasebits, counts, clear) (RFPHASEKEY | ((clear & 1) << 25) \
        | (((counts) & 0xff) << 16) | (phasebits & 0xffff))
/* matching phase cycle inc word by special usage...  */
#define RFPHASECYCLEINCWORD(phasebits, counts, clear) (RFPHASECYCLEKEY | ((clear & 1) << 25) \
        | (((counts) & 0xff) << 16) | (phasebits & 0xffff))
#define RFAMPWORD(ampbits)  (RFAMPKEY | (ampbits & 0x0ffff))
#define RFAMPSCALEWORD(ampbits)  (RFAMPSCALEKEY | (ampbits & 0x0ffff))
#define RFAMPINCWORD(ampbits, counts, clear) (RFAMPKEY | ((clear & 1) << 25) \
        | (((counts) & 0xff) << 16) | (ampbits & 0xffff))

#define RFATTN		   (AUX +0x000)
#define RFFREQ_01	   (AUX +0x100)
#define RFFREQ1		   (AUX +0x200)
#define RFFREQ100	   (AUX +0x300)
#define RFFREQ10K	   (AUX +0x400)
#define RFFREQ1M	   (AUX +0x500)
#define RFFREQ100M	   (AUX +0x600)
#define RFFREQLATCH	   (AUX +0x700)
#define RFFREQr1	   (AUX +0x800)
#define RFFREQr2	   (AUX +0x900)

/* PFG */
#define XPFGAMPKEY             (2 << 26)
#define YPFGAMPKEY             (3 << 26)
#define ZPFGAMPKEY             (4 << 26)
#define XPFGAMPSCALEKEY        (5 << 26)
#define YPFGAMPSCALEKEY        (6 << 26)
#define ZPFGAMPSCALEKEY        (7 << 26)
#define PFGGATESKEY            (11 << 26)
#define PFGUSERKEY             (12 << 26)


/* Gradients ... */

#define XGRADKEY	    (2<<26)
#define YGRADKEY	    (3<<26)
#define ZGRADKEY	    (4<<26)
#define B0GRADKEY           (5<<26)
#define XGRADSCALEKEY       (6<<26)
#define YGRADSCALEKEY       (7<<26)
#define ZGRADSCALEKEY       (8<<26)
#define B0GRADSCALEKEY      (9<<26)
#define XECCGRADKEY        (10<<26)
#define YECCGRADKEY        (11<<26)
#define ZECCGRADKEY        (12<<26)
#define B0ECCGRADKEY       (13<<26)
#define GRADSHIMKEY        (14<<26)
#define GRADGATEKEY        (15<<26)
#define GRADUSERKEY        (16<<26)


#define GRADAMPMASK         (0x0ffff)


/* DDR */
/*  none built */
#define DDRSTAGE	   (0x060000000)
#define DSPSTAGE	   (0x070000000)

#define DDR_TG1          0x0080   // test gate (j2-pin9)
#define DDR_IEN          0x0001   // AD6634 IENA (input enable)
#define DDR_PINS         0x0042   // AD6634 PINSYNC + C67 SCAN interrupt (j2-pin8)
#define DDR_ACQ          0x0004   // fifo collect
#define DDR_RG           0x0008   // receiver gate
#define DDR_SCAN         0x0010   // new scan flag
#define DDR_FID          0x0020   // new fid flag

#endif
