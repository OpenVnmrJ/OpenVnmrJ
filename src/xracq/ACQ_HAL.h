/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/* Start of Free Memory in VM02 for LowCore,Acodes & other structs */
#define LC_ST_VB	0x42300
#define LC_ST_LB	0x2300
#define LC_END_VB	0x47fff
#define LC_END_LB	0x7fff


/* test and set timeouts */
#define STATTIMEOUT $fffe/* statblock timeout count down value */
#define ITRTIMEOUT $fffe/* hal or acq interrupt timeout count down value */

/* acquisition status */
#define VBUG    'v '
#define PROM    'p '
#define RAM     'r '
#define WAIT    'w '


/* hal status */
#define HBUG    'h '
#define TAPE    't '/* or 0x0 ? */
/* PROM and RAM are shared with above definitions */

/* WARNING this header used in assembler routines therefore DO NOT  */
/* put tabs or spaces between define values and any comments    Greg */

/* interrupt semaphore values */
#define ACTIVE          0x80/* consistent with test-and-set */
#define IDLE            0
#define LOCKED          0x8000/* lock out interrupt routine  */
#define LOCKEDASM       $8000/* lock out interrupt routine  */

/* --- SA semaphore Value --- */
#define STOPPENDING	0x7fff/* intrpt routine will not set saflag if set */
#define STOPEDASM	$7fff/* intrpt routine will not set saflag if set */


/* types of interrupts HAL --> ACQ (acqir1typ,acqir2typ) */
/* for all levels !! */
#define INVALID         0
#define SEND_STRING     1/* send string to acq */
#define ABORT_ACQ       2
#define STOP_ACQ        3
#define RESUME_ACQ      4
#define EXECUTE_PGM     5/* jump to long address in mbox */
#define ACK_IRQ         6/* no-op - see if acq is alive */
#define STOP_LOCK       7/* stop background lock acquisition */
#define HWCHANGE        8/* change acq hardware settings */
#define	WAKEUPACQ	9/* wake up a sleeping ACQ */
#define	ACK_ECHO	10/* returns interrupt on same level to HAL */
#define STOP_EOS	11/* sa at end of scan */
#define STOP_EOB	12/* sa at end of bs */
#define STOP_EOF	13/* sa at end of fid */
#define STOP_EOC	14/* sa at end of interleave cycle */
#define MAXTYPE1        STOP_EOC


/* types of interrupts ACQ --> HAL (halir1typ, halir2typ) */
#define RECV_STRING     100/* send string to HAL */
#define RECV_BINARY     101/* send 1 long int in mbox to HAL */
#define CT_EQ_NT        102
#define AT_BS           103
#define AT_EOS          104
#define ACQ_ERR         105
#define ACQ_WARN        106/* hi/lo noise, adc oflo, etc */
#define ABORT_COMPLETE  107
#define AT_STOP  	108



/* return values for call_acq() and its acq analog */
/*#define	OK	0 */ /* defined elsewhere */
#define	SLEEP		'sl'/* should be > 0 ! */
/* note well that error returns are < 0 ! */
