/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*
 * Varian Assoc.,Inc. Varian, Inc. All Rights Reserved.
 */

/*----------------------------------------------------------------------------
|  AP delay constants
+----------------------------------------------------------------------------*/

/* apbus cycle = 1.15 usec */

#define MERCURY_POWER_OVERHEAD			4.6e-6
#define MERCURY_PWRF_OVERHEAD			4.6e-6
#define MERCURY_SAPS_OVERHEAD			13.8e-6
/* offset:
/* Hiband BB	( 0  + 76 )*1.15 =  87.40 usec
/*        4-Nuc ( 0  + 76 )*1.15 =  87.40 usec
/*        New   ( 0  + 34 )*1.15 =  39.10 usec
/* Low Band BB  ( 18 + 76 )*1.15 = 108.10 usec
/*        4-Nuc ( 11 + 76 )*1.15 = 100.05 usec 
 */
#define MERCURY_OFFSET_OVERHEAD			97.75e-6
#define MERCURY_OFFSET_LTCH_OVERHEAD		97.75e-6
#define MERCURY_WFG_START_OVERHEAD		0.0
#define MERCURY_WFG_VSTART_OVERHEAD		0.0
#define MERCURY_WFG_ISTART_OVERHEAD		0.0
#define MERCURY_WFG_OFFSET_OVERHEAD		0.0
#define MERCURY_WFG_STOP_OVERHEAD		0.0
#define MERCURY_WFG2_START_OVERHEAD		0.0
#define MERCURY_WFG2_OFFSET_OVERHEAD		0.0
#define MERCURY_WFG2_STOP_OVERHEAD		0.0
#define MERCURY_WFG3_START_OVERHEAD		0.0
#define MERCURY_WFG3_OFFSET_OVERHEAD		0.0
#define MERCURY_WFG3_STOP_OVERHEAD		0.0
#define MERCURY_PRG_START_OVERHEAD		0.0
#define MERCURY_PRG_STOP_OVERHEAD		0.0
#define MERCURY_SPNLCK_START_OVERHEAD		0.0
#define MERCURY_SPNLCK_STOP_OVERHEAD		0.0
#define MERCURY_SPN2LCK_START_OVERHEAD		0.0
#define MERCURY_SPN2LCK_STOP_OVERHEAD		0.0
#define MERCURY_P_GRADIENT			6.9e-6
#define MERCURY_L_GRADIENT			4.6e-6
#define MERCURY_W_GRADIENT			0.0
#define MERCURY_T_GRADIENT			0.0
#define MERCURY_ACQUIRE_START_OVERHEAD		0.0
#define MERCURY_ACQUIRE_STOP_OVERHEAD		0.0
#define MERCURY_SETDECMOD_OVERHEAD		5.45e-6
#define MERCURY_DECMODFREQ_OVERHEAD		13.8e-6	/* initdecmodfreq() */
#define MERCURY_PRG_OFFSET_OVERHEAD		MERCURY_WFG_OFFSET_OVERHEAD

#define MERCRUY			1

#define POWER_EVENT 		1
#define SAPS_EVENT 		2
#define OFFSET_FREQ_EVENT 	3
#define OFFSET_LATCH_EVENT 	4
#define WFG_START_EVENT 	5
#define WFG_STOP_EVENT 		6
#define WFG2_START_EVENT 	7
#define WFG2_STOP_EVENT 	8
#define WFG3_START_EVENT 	9
#define WFG3_STOP_EVENT 	10
#define PRG_START_EVENT 	11
#define PRG_STOP_EVENT 		12
#define SPNLCK_START_EVENT 	13
#define SPNLCK_STOP_EVENT 	14
#define SPN2LCK_START_EVENT 	15
#define SPN2LCK_STOP_EVENT 	16
#define GRADIENT_EVENT		17
#define WFG_OFFSET_EVENT 	18
#define WFG2_OFFSET_EVENT 	19
#define WFG3_OFFSET_EVENT 	20
#define WFG_VSTART_EVENT	21
#define WFG_ISTART_EVENT	22
#define ACQUIRE_START_EVENT	23
#define ACQUIRE_STOP_EVENT	24
#define SETDECMOD_EVENT		25
#define DECMODFREQ_EVENT	26
#define PRG_OFFSET_EVENT	27
#define VAGRADIENT_EVENT	28
#define OBLIQUEGRADIENT_EVENT	29
#define PWRF_EVENT		30

extern double eventovrhead();


#define POWER_DELAY  		( eventovrhead(POWER_EVENT) )
#define PWRF_DELAY  		( eventovrhead(PWRF_EVENT) )
#define SAPS_DELAY		( eventovrhead(SAPS_EVENT) )
#define OFFSET_DELAY 		( eventovrhead(OFFSET_FREQ_EVENT) )
#define OFFSET_LTCH_DELAY 	( eventovrhead(OFFSET_LATCH_EVENT) )
#define WFG_START_DELAY 	( eventovrhead(WFG_START_EVENT) +	\
					eventovrhead(WFG_OFFSET_EVENT) )
#define WFG_VSTART_DELAY 	( eventovrhead(WFG_VSTART_EVENT) )
#define WFG_INCSTART_DELAY 	( eventovrhead(WFG_ISTART_EVENT) )
#define WFG_OFFSET_DELAY 	( eventovrhead(WFG_OFFSET_EVENT) )
#define WFG_STOP_DELAY 		( eventovrhead(WFG_STOP_EVENT) )
#define WFG2_START_DELAY 	( eventovrhead(WFG2_START_EVENT) +	\
					eventovrhead(WFG2_OFFSET_EVENT) )
#define WFG2_OFFSET_DELAY 	( eventovrhead(WFG2_OFFSET_EVENT) )
#define WFG2_STOP_DELAY 	( eventovrhead(WFG2_STOP_EVENT) )
#define WFG3_START_DELAY 	( eventovrhead(WFG3_START_EVENT) +	\
					eventovrhead(WFG3_OFFSET_EVENT) )
#define WFG3_OFFSET_DELAY 	( eventovrhead(WFG3_OFFSET_EVENT) )
#define WFG3_STOP_DELAY 	( eventovrhead(WFG3_STOP_EVENT) )
#define PRG_START_DELAY 	( eventovrhead(PRG_START_EVENT) )
#define PRG_STOP_DELAY 		( eventovrhead(PRG_STOP_EVENT) )
#define SPNLCK_START_DELAY 	( eventovrhead(SPNLCK_START_EVENT) )
#define SPNLCK_STOP_DELAY 	( eventovrhead(SPNLCK_STOP_EVENT) )
#define SPN2LCK_START_DELAY 	( eventovrhead(SPN2LCK_START_EVENT) )
#define SPN2LCK_STOP_DELAY 	( eventovrhead(SPN2LCK_STOP_EVENT) )
#define GRADIENT_DELAY 		( eventovrhead(GRADIENT_EVENT) )
#define VAGRADIENT_DELAY 	( eventovrhead(VAGRADIENT_EVENT) )
#define OBLIQUEGRADIENT_DELAY 	( eventovrhead(OBLIQUEGRADIENT_EVENT) )
#define ACQUIRE_START_DELAY	( eventovrhead(ACQUIRE_START_EVENT) )
#define ACQUIRE_STOP_DELAY 	( eventovrhead(ACQUIRE_STOP_EVENT) )
#define SETDECMOD_DELAY 	( eventovrhead(SETDECMOD_EVENT) )
#define DECMODFREQ_DELAY 	( eventovrhead(DECMODFREQ_EVENT) )
#define PRG_OFFSET_DELAY 	( eventovrhead(PRG_OFFSET_EVENT) )

