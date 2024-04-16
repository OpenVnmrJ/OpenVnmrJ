/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*----------------------------------------------------------------------------
|  AP delay constants
+----------------------------------------------------------------------------*/


#define VXR_POWER_OVERHEAD		4.5e-6
#define VXR_PWRF_OVERHEAD		6.45e-6
#define VXR_SAPS_OVERHEAD		2.35e-6
#define VXR_OFFSET_OVERHEAD		15.25e-6
#define VXR_OFFSET_LTCH_OVERHEAD	21.7e-6
#define VXR_WFG_START_OVERHEAD		10.95e-6
#define VXR_WFG_VSTART_OVERHEAD		27.95e-6
#define VXR_WFG_ISTART_OVERHEAD		23.65e-6
#define VXR_WFG_OFFSET_OVERHEAD		1.5e-6
#define VXR_WFG_STOP_OVERHEAD		4.5e-6
#define VXR_WFG2_START_OVERHEAD		21.7e-6
#define VXR_WFG2_OFFSET_OVERHEAD	1.5e-6
#define VXR_WFG2_STOP_OVERHEAD		8.8e-6
#define VXR_WFG3_START_OVERHEAD		32.45e-6
#define VXR_WFG3_OFFSET_OVERHEAD	1.5e-6
#define VXR_WFG3_STOP_OVERHEAD		13.1e-6
#define VXR_PRG_START_OVERHEAD		10.95e-6
#define VXR_PRG_STOP_OVERHEAD		4.5e-6
#define VXR_SPNLCK_START_OVERHEAD	10.95e-6
#define VXR_SPNLCK_STOP_OVERHEAD	4.5e-6
#define VXR_SPN2LCK_START_OVERHEAD	21.7e-6
#define VXR_SPN2LCK_STOP_OVERHEAD	8.8e-6
#define VXR_ACQUIRE_START_OVERHEAD	0.0
#define VXR_ACQUIRE_STOP_OVERHEAD	0.0
#define VXR_PRG_OFFSET_OVERHEAD		VXR_WFG_OFFSET_OVERHEAD


/* 8/21/92 HS lines are now output with AP Bus events, no longer need 200nsec
           delay   gmb */
/* apbus cycle = 2.15 usec */

#define UNITY_POWER_OVERHEAD		4.3e-6	/*
					    	   power()
					    	   rlpower()
						*/
#define UNITY_PWRF_OVERHEAD		6.45e-6	/* pwrf() */

#define UNITY_SAPS_OVERHEAD		2.15e-6	/*
					    	   xmtrphase()
					    	   dcplrphase()
					    	   dcplr2phase()
					    	   gensaphase()
						*/

#define UNITY_OFFSET_OVERHEAD		15.05e-6
#define UNITY_OFFSET_LTCH_OVERHEAD	21.5e-6	/*
					    	   offset()
						*/

#define UNITY_WFG_START_OVERHEAD	10.75e-6
#define UNITY_WFG_VSTART_OVERHEAD	27.95e-6
#define UNITY_WFG_ISTART_OVERHEAD	23.65e-6
#define UNITY_WFG_OFFSET_OVERHEAD	1.5e-6
#define UNITY_WFG_STOP_OVERHEAD		4.3e-6	/*
					    	   shaped_pulse()
					    	   decshaped_pulse()
					    	   dec2shaped_pulse()
					    	   genshaped_pulse()
						*/

#define UNITY_WFG2_START_OVERHEAD	21.5e-6
#define UNITY_WFG2_OFFSET_OVERHEAD	1.5e-6
#define UNITY_WFG2_STOP_OVERHEAD	8.6e-6	/*
						   sim2shaped_pulse()
						   gensim2shaped_pulse()
						*/

#define UNITY_WFG3_START_OVERHEAD	32.25e-6
#define UNITY_WFG3_OFFSET_OVERHEAD	1.5e-6
#define UNITY_WFG3_STOP_OVERHEAD	12.9e-6	/*
						   sim3shaped_pulse()
						   gensim3shaped_pulse()
						*/

#define UNITY_PRG_START_OVERHEAD	10.75e-6	/*
					    	   obsprgon()
					    	   decprgon()
					    	   dec2prgon()
					    	   prg_dec_on()
						*/

#define UNITY_PRG_STOP_OVERHEAD		4.3e-6	/*
					    	   obsprgoff()
					    	   decprgoff()
					    	   dec2prgoff()
					    	   prg_dec_off()
						*/

#define UNITY_SPNLCK_START_OVERHEAD	10.75e-6
#define UNITY_SPNLCK_STOP_OVERHEAD	4.3e-6	/*
					    	   spinlock()
					    	   decspinlock()
					    	   dec2spinlock()
					    	   genspinlock()
						*/

#define UNITY_SPN2LCK_START_OVERHEAD	21.5e-6
#define UNITY_SPN2LCK_STOP_OVERHEAD	8.6e-6	/*
						   gen2spinlock()
						*/

#define UNITY_P_GRADIENT			12.9e-6
#define UNITY_L_GRADIENT			8.6e-6
#define UNITY_W_GRADIENT			6.9e-6
						/*
						  rgradient()
						  vgradient()
						  gradtype=p long time for both
						  gradtype=w short time for both
						*/
#define UNITY_ACQUIRE_START_OVERHEAD	0.0
#define UNITY_ACQUIRE_STOP_OVERHEAD	0.0
#define UNITY_SETDECMOD_OVERHEAD	2.15e-6 /* setdecmodulation() */
#define UNITY_DECMODFREQ_OVERHEAD	0.0
#define UNITY_PRG_OFFSET_OVERHEAD		UNITY_WFG_OFFSET_OVERHEAD

/* for now I'll half the unity times ap times */
/* The WFG#_STOP_DELAY's should be 0.0 for Unity+.  SF */
/* apbus cycle = 1.15 usec */

#define UNITYPLUS_POWER_OVERHEAD		2.3e-6
#define UNITYPLUS_PWRF_OVERHEAD			4.6e-6
#define UNITYPLUS_SAPS_OVERHEAD			3.45e-6
#define UNITYPLUS_OFFSET_OVERHEAD		10.35e-6
#define UNITYPLUS_OFFSET_LTCH_OVERHEAD		10.35e-6
#define UNITYPLUS_WFG_START_OVERHEAD		5.75e-6
#define UNITYPLUS_WFG_VSTART_OVERHEAD		14.95e-6
#define UNITYPLUS_WFG_ISTART_OVERHEAD		12.65e-6
#define UNITYPLUS_WFG_OFFSET_OVERHEAD		0.45e-6
#define UNITYPLUS_WFG_STOP_OVERHEAD		0.0
#define UNITYPLUS_WFG2_START_OVERHEAD		11.5e-6
#define UNITYPLUS_WFG2_OFFSET_OVERHEAD		0.45e-6
#define UNITYPLUS_WFG2_STOP_OVERHEAD		0.0
#define UNITYPLUS_WFG3_START_OVERHEAD		17.25e-6
#define UNITYPLUS_WFG3_OFFSET_OVERHEAD		0.45e-6
#define UNITYPLUS_WFG3_STOP_OVERHEAD		0.0
#define UNITYPLUS_PRG_START_OVERHEAD		5.75e-6
#define UNITYPLUS_PRG_STOP_OVERHEAD		2.3e-6
#define UNITYPLUS_SPNLCK_START_OVERHEAD		5.75e-6
#define UNITYPLUS_SPNLCK_STOP_OVERHEAD		2.3e-6
#define UNITYPLUS_SPN2LCK_START_OVERHEAD	11.5e-6
#define UNITYPLUS_SPN2LCK_STOP_OVERHEAD		4.6e-6
#define UNITYPLUS_P_GRADIENT			6.9e-6
#define UNITYPLUS_L_GRADIENT			4.6e-6
#define UNITYPLUS_W_GRADIENT			3.45e-6
#define UNITYPLUS_T_GRADIENT			6.9e-6
#define UNITYPLUS_ACQUIRE_START_OVERHEAD	0.0
#define UNITYPLUS_ACQUIRE_STOP_OVERHEAD		0.0
#define UNITYPLUS_SETDECMOD_OVERHEAD	1.15e-6 /* set_channel_modulation() */
#define UNITYPLUS_DECMODFREQ_OVERHEAD		3.45e-6	/* initdecmodfreq() */
#define UNITYPLUS_PRG_OFFSET_OVERHEAD		UNITYPLUS_WFG_OFFSET_OVERHEAD

/* INOVA */
#define INOVA_STD_APBUS_DELAY			0.5e-6

#define INOVA_POWER_OVERHEAD			0.5e-6
#define INOVA_PWRF_OVERHEAD			0.5e-6
#define INOVA_SAPS_OVERHEAD			0.5e-6
#define INOVA_OFFSET_OVERHEAD			4.00e-6
#define INOVA_OFFSET_LTCH_OVERHEAD		4.00e-6
#define INOVA_WFG_VSTART_OVERHEAD		2.25e-6   
#define INOVA_WFG_ISTART_OVERHEAD		2.25e-6   

#define INOVA_WFG_START_OVERHEAD		1.25e-6   
#define INOVA_WFG_OFFSET_OVERHEAD		0.45e-6
#define INOVA_WFG_STOP_OVERHEAD			0.5e-6
#define INOVA_WFG2_START_OVERHEAD		2.5e-6
#define INOVA_WFG2_OFFSET_OVERHEAD		0.45e-6
#define INOVA_WFG2_STOP_OVERHEAD		0.5e-6
#define INOVA_WFG3_START_OVERHEAD		3.75e-6
#define INOVA_WFG3_OFFSET_OVERHEAD		0.45e-6
#define INOVA_WFG3_STOP_OVERHEAD		0.5e-6

#define INOVA_PRG_START_OVERHEAD		1.25e-6
#define INOVA_PRG_STOP_OVERHEAD			0.5e-6
#define INOVA_SPNLCK_START_OVERHEAD		1.25e-6
#define INOVA_SPNLCK_STOP_OVERHEAD		0.5e-6
#define INOVA_SPN2LCK_START_OVERHEAD		1.75e-6
#define INOVA_SPN2LCK_STOP_OVERHEAD		1.0e-6
#define INOVA_P_GRADIENT			4.0e-6
#define INOVA_L_GRADIENT			1.0e-6
#define INOVA_W_GRADIENT			0.5e-6
#define INOVA_T_GRADIENT			4.0e-6
#define INOVA_ACQUIRE_START_OVERHEAD		1.0e-6
#define INOVA_ACQUIRE_STOP_OVERHEAD		0.5e-6
#define INOVA_SETDECMOD_OVERHEAD		0.5e-6 
#define INOVA_DECMODFREQ_OVERHEAD		1.5e-6	/* initdecmodfreq() */
#define INOVA_PRG_OFFSET_OVERHEAD		INOVA_WFG_OFFSET_OVERHEAD
#define INOVA_CRB_ROTATE			60.0e-6
#define INOVA_CRB_INIT	    			60.0e-6

#define VXR 			1
#define UNITY 			2
#define UNITYPLUS		3
#define INOVA			4

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

extern double eventovrhead(int event_type);


#ifdef NVPSG
#define PWRF_DELAY  		( 0.0 )
#else
#define PWRF_DELAY  		( eventovrhead(PWRF_EVENT) )
#endif

#define POWER_DELAY  		( eventovrhead(POWER_EVENT) )
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

