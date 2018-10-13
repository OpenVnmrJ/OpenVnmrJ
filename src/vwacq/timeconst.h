/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef INCtimeconsth
#define INCtimeconsth

#define PARALLEL_CHANS

#define CLOCK_80MHZ
#ifdef CLOCK_80MHZ

#ifndef PARALLEL_CHANS
#define MIN_FIFO_CNT 	5   /* minimum count that fifo takes, = 100nsec */
#else
#define MIN_FIFO_CNT 	8   /* minimum count that fifo takes, = 100nsec */
#endif
#define CNT100NSEC_MIN  MIN_FIFO_CNT
#define MIN_TIME_RESOLUTION 100
#define MIN_RESOLUTION  12.5	/* min time resolution of FIFO */

/* 
         APbus Min Time is 600nsec (at present) apbus is 100nsec itself, 
         so we and a pad delay of 500 to give the proper time
   	 5 = 100nsec,  400 nsec = 32 * 12.5,count 37 = 500 nsec
*/
/* for parallel channels move the HW_DELAY_FUDGE correction from prior fifoStuffCmd() call to
   within the function call itself.  This allow the parallel sorting not to have to
   worry about the HW_DELAY_FUDGE correction during it's sort
*/

#ifndef PARALLEL_CHANS

#define AP_MIN_DELAY_CNT 32  /* 400 nsec without HW specific mod*/
#define AP_RD_MIN_DELAY_CNT 40  /* 500 nsec  without HW specific mod to yield 600 nsec readback */
#define AP_HW_MIN_DELAY_CNT 29  /* 400 nsec */
#define AP_ITRP_MIN_DELAY_CNT  79997 /* 1msec absolute ticks */
#define PFG_MIN_DELAY_CNT 72  /* 900 nsec without HW specific mod*/
#define PFG_HW_MIN_DELAY_CNT 69  /* 900 nsec */
/* Min Delay Between ApBus Transactions 400 nsec */
/* 100nsec for APbus transaction & add 400 nsec pad delay */
/* #define AP_MIN_DELAY_CNT 47 */ /* 7 = 100nsec,  300 nsec = 30 * 10nsec */

/* Interrupt Delays */
/* We wanted all the interrupt delays to be the same so users would not	*/
/* have to worry about which cpu they had.  They are seperated here 	*/
/* for testing.  The initial interrupt delays for the 162 board were	*/
/* 500 us.  This was too long for the powerpc and actually too long for	*/
/* the 162.  We looked at the interrupt handling times (using WindView)	*/
/* and determined that 150 us seemed to be the optimum time.  The STM	*/
/* interrupt handling times were 80-100 us (normally 85us) and a  	*/
/* possible fifo full/empty preemptive interrupt was 20us/50-100us 	*/
/* (normally 20/55 us).  Since the interrupt handler does not need the	*/
/* entire interrupt delay, 150 us seemed to be fine.  We also tested at	*/
/* 100 us which seemed to work fine, but 150 us seemed better.		*/

#if (CPU != PPC603)
#define STM_INTERRUPT_DELAY	12000		/* 150 us delay	*/
#define STM_HW_INTERRUPT_DELAY	11997		/* 150 us delay	*/
#define SW1_INTERRUPT_DELAY	12000		/* 150 us delay	*/
#define SW1_HW_INTERRUPT_DELAY	11997		/* 150 us delay	*/
#else
/* PPC */
/* the stm one's appear to work */
/* Interrupt Delays */
#define STM_INTERRUPT_DELAY	12000		/* 150 us delay	*/
#define STM_HW_INTERRUPT_DELAY	11997		/* 150 us delay	*/
#define SW1_INTERRUPT_DELAY	12000		/* 150 us delay	*/
#define SW1_HW_INTERRUPT_DELAY	11997		/* 150 us delay	*/
#endif


#define CNT312_5USEC 	24997L		/* 312.5 us */
#define CNT100_MSEC 	7999997L	/* 100ms */
#define CNT40_MSEC 	3199997L	/* 40ms */
#define CNT12_5USEC	997L		/* 12.5 us */
#define CNT1_USEC	77L		/* 1 us */
#define CNT2_USEC	157L		/* 2 us */
#define CNT4_USEC	317L		/* 4 us */
#define CNT5_USEC	397L		/* 5 us */
#define CNT10_USEC	797L		/* 10 us */
#define CNT100_USEC	7997L		/* 100 us */

#if (CPU != PPC603)
#define WGLOAD_DELAY	250000L		/* ~ 25 msec */
#else
#define WGLOAD_DELAY	17000L		/* ~ 1.7 msec */
#endif

/*  This delay count was determined experimentally to work well in the lock
    acquisition, to give a suitable "front porch", not too short, not too
    long.  It only applies for the 80 MHz clock.				*/

#define CNTLKACQUIRE	3399997		/* ca. 42.5 ms */

#else /* is parallel channels */

#define AP_MIN_DELAY_CNT 32  /* 400 nsec without HW specific mod*/
#define AP_RD_MIN_DELAY_CNT 40  /* 500 nsec  without HW specific mod to yield 600 nsec readback */
#define AP_HW_MIN_DELAY_CNT 32  /* 400 nsec */
#define AP_ITRP_MIN_DELAY_CNT  80000 /* 1msec absolute ticks */
#define PFG_MIN_DELAY_CNT 72  /* 900 nsec without HW specific mod*/
#define PFG_HW_MIN_DELAY_CNT 72  /* 900 nsec */
/* Min Delay Between ApBus Transactions 400 nsec */
/* 100nsec for APbus transaction & add 400 nsec pad delay */
/* #define AP_MIN_DELAY_CNT 47 */ /* 7 = 100nsec,  300 nsec = 30 * 10nsec */

/* Interrupt Delays */
/* We wanted all the interrupt delays to be the same so users would not	*/
/* have to worry about which cpu they had.  They are seperated here 	*/
/* for testing.  The initial interrupt delays for the 162 board were	*/
/* 500 us.  This was too long for the powerpc and actually too long for	*/
/* the 162.  We looked at the interrupt handling times (using WindView)	*/
/* and determined that 150 us seemed to be the optimum time.  The STM	*/
/* interrupt handling times were 80-100 us (normally 85us) and a  	*/
/* possible fifo full/empty preemptive interrupt was 20us/50-100us 	*/
/* (normally 20/55 us).  Since the interrupt handler does not need the	*/
/* entire interrupt delay, 150 us seemed to be fine.  We also tested at	*/
/* 100 us which seemed to work fine, but 150 us seemed better.		*/

#if (CPU != PPC603)
#define STM_INTERRUPT_DELAY	12000		/* 180 us delay	*/
#define STM_HW_INTERRUPT_DELAY	12000		/* 180 us delay	*/
#define SW1_INTERRUPT_DELAY	12000		/* 180 us delay		*/
#define SW1_HW_INTERRUPT_DELAY	12000		/* 180 us delay		*/
#else
/* PPC */
/* the stm one's appear to work */
/* Interrupt Delays */
#define STM_INTERRUPT_DELAY	12000		/* 180 us delay*/
#define STM_HW_INTERRUPT_DELAY	12000		/* 150 us delay*/
#define SW1_INTERRUPT_DELAY	12000		/* 150 us delay*/
#define SW1_HW_INTERRUPT_DELAY	12000		/* 150 us delay*/
#endif


#define CNT312_5USEC 	25000L		/* 312.5 us */
#define CNT100_MSEC 	8000000L	/* 100ms */
#define CNT40_MSEC 	3200000L	/* 40ms */
#define CNT12_5USEC	1000L		/* 12.5 us */
#define CNT1_USEC	80L		/* 1 us */
#define CNT2_USEC	160L		/* 2 us */
#define CNT4_USEC	320L		/* 4 us */
#define CNT5_USEC	400L		/* 5 us */
#define CNT10_USEC	800L		/* 10 us */
#define CNT100_USEC	8000L		/* 100 us */

#if (CPU != PPC603)
#define WGLOAD_DELAY	250000L		/* ~ 25 msec */
#else
#define WGLOAD_DELAY	17000L		/* ~ 1.7 msec */
#endif

/*  This delay count was determined experimentally to work well in the lock
    acquisition, to give a suitable "front porch", not too short, not too
    long.  It only applies for the 80 MHz clock.				*/

#define CNTLKACQUIRE	3400000		/* ca. 42.5 ms */

#endif  /* parallel channels */

#else   /* 100 MHZ */

#define MIN_FIFO_CNT 	7   /* minimum count that fifo takes, = 100nsec */
#define CNT100NSEC_MIN  MIN_FIFO_CNT
#define MIN_TIME_RESOLUTION 100
#define MIN_RESOLUTION  10.0	/* min time resolution of FIFO */

/* Min Delay Between ApBus Transactions 400 nsec */
/* 100nsec for APbus transaction & add 400 nsec pad delay */
#define AP_MIN_DELAY_CNT 50 /* 7 = 100nsec,  400 nsec = 40 * 10nsec, 500nsec  */
#define AP_HW_MIN_DELAY_CNT 47 /* 7 = 100nsec,  400 nsec = 40 * 10nsec, 500nsec  */
#define PFG_MIN_DELAY_CNT 90  /* 900 nsec without HW specific mod*/ 
#define PFG_HW_MIN_DELAY_CNT 87  /* 900 nsec */ 
#define STM_INTERRUPT_DELAY	50000		/* 500 us delay		*/
#define STM_HW_INTERRUPT_DELAY	49997		/* 500 us delay		*/
#define SW1_INTERRUPT_DELAY	50000		/* 500 us delay		*/
#define SW1_HW_INTERRUPT_DELAY	49997		/* 500 us delay		*/

#endif

#define TIME_TO_FIFO_CNT(a) (  ((unsigned long)((double)(a - MIN_TIME_RESOLUTION) / (double) MIN_RESOLUTION )) + CNT100NSEC_MIN)

#endif
