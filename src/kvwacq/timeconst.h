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
#ifndef INCtimeconsth
#define INCtimeconsth

#define CLOCK_80MHZ
#ifdef CLOCK_80MHZ

#define MIN_FIFO_CNT 	5   /* minimum count that fifo takes, = 100nsec */
#define CNT100NSEC_MIN  MIN_FIFO_CNT
#define MIN_TIME_RESOLUTION 100
#define MIN_RESOLUTION  12.5	/* min time resolution of FIFO */

/* 
         APbus Min Time is 600nsec (at present) apbus is 100nsec itself, 
         so we and a pad delay of 500 to give the proper time
   	 5 = 100nsec,  400 nsec = 32 * 12.5,count 37 = 500 nsec
*/
#define AP_MIN_DELAY_CNT	32	/* 400 nsec without HW specific mod */
#define AP_RD_MIN_DELAY_CNT	40	/* 500 nsec without HW specific mod
					   to yield 600 nsec readback */
#define AP_HW_MIN_DELAY_CNT	29	/* 400 nsec */
#define AP_ITRP_MIN_DELAY_CNT	79997	/* 1msec absolute ticks */
#define PFG_MIN_DELAY_CNT	72	/* 900 nsec without HW specific mod*/
#define PFG_HW_MIN_DELAY_CNT	69	/* 900 nsec */
/* Min Delay Between ApBus Transactions 400 nsec */
/* 100nsec for APbus transaction & add 400 nsec pad delay */
/* #define AP_MIN_DELAY_CNT 47 */ /* 7 = 100nsec,  300 nsec = 30 * 10nsec */

/* Interrupt Delays */
#define STM_INTERRUPT_DELAY	40000	/* 500 us delay		*/
#define STM_HW_INTERRUPT_DELAY	 4980	/* 500 us delay	MERCURY	*/
#define SW1_INTERRUPT_DELAY	 4980	/* 500 us delay		*/
#define SW1_HW_INTERRUPT_DELAY	39997	/* 500 us delay		*/


#define CNT312_5USEC 	3123L		/* 312.5 us MERCURY*/
#define CNT100_MSEC 	1000000L	/* 100ms relaxdelay in 100nsec MERCURY*/
#define CNT40_MSEC 	0x4028L		/* 40ms MERCURY*/
#define CNT12_5USEC	123		/* 12.5 us MERCURY*/
#define CNT1_USEC	77L		/* 1 us */
#define CNT2_USEC	157L		/* 2 us */
#define CNT4_USEC	38		/* 4 us */
#define CNT5_USEC	48		/* 5 us MERCURY*/
#define CNT10_USEC	797L		/* 10 us */
#define CNT100_USEC	7997L		/* 100 us */

#define WGLOAD_DELAY	250000L		/* ~ 25 msec */

/*  This delay count was determined experimentally to work well in the lock
    acquisition, to give a suitable "front porch", not too short, not too
    long.  It only applies for the 80 MHz clock.				*/

#define CNTLKACQUIRE	3399997		/* ca. 42.5 ms */

#else   /* 100 MHZ */

#endif

#define TIME_TO_FIFO_CNT(a) (  ((unsigned long)((double)(a - MIN_TIME_RESOLUTION) / (double) MIN_RESOLUTION )) + CNT100NSEC_MIN)

#endif
