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
#ifndef INCfifoh
#define INCfifoh

/* ------------- Make C header file C++ compliant ------------------- */
#ifdef __cplusplus
extern "C" {
#endif

/* ----------- FIFO Status ---------- */
#define FIFO_ERROR -1
#define FIFO_ALMOST_FULL 1
#define FIFO_EMPTY 0
#define FIFO_STOPPED -1

/* ----------- FIFO Error Codes, start at 20 so that it can be append to those
	 codes in errorcodes.h in vnmr
*/
#define FIFO_UNDERFLOW 		20
#define FIFO_START_ON_HALT 	21
#define FIFO_START_ON_EMPTY 	22
#define FIFO_NETBL 		23
#define FIFO_FORP 		25
#define FIFO_NETBAP 		26

#define FIFO_WORD_BUF_SIZE   8192

#define FIFO_SW_INTRP1		2048
#define FIFO_SW_INTRP2		1024
#define FIFO_SW_INTRP3		512
#define FIFO_SW_INTRP4		256

#define STD_HS_LINES 0
#define EXT_HS_LINES 1

/* --- Interrupt bits ---- */
#define FF_OVERFLOW    (1)       /* bit - 0 */
#define FF_UNDERFLOW   (2)       /* bit - 1 */
#define FF_FINISHED    (4)       /* bit - 2 */
#define FF_STARTED     (8)       /* bit - 3 */

#define FF_SW1_IRQ     (0x10)    /* bit - 4 */
#define FF_SW2_IRQ     (0x20)    /* bit - 5 */
#define FF_SW3_IRQ     (0x40)    /* bit - 6 */
#define FF_SW4_IRQ     (0x80)    /* bit - 7 */

#define FF_FAILURE     (0x100)    /* bit - 8 */
#define FF_WARNING     (0x200)    /* bit - 9 */
#define FF_DATA_AMMT   (0x400)    /* bit - 10 */
#define FF_INSTR_AMF   (0x800)    /* bit - 11 */

#define FF_INVALID_OPCODE (0x1000)    /* bit - 12 */


/* -----------------  Specific flavor interrupts   ----- */
/*
bit    mask		MASTER		PFG		GRADIENT		DDR
---    ------		-----------	---------	----------		--------
13    0x2000		probe_id_int 	amp_duration_too_short	
14    0x4000		tune 		amp_duration_too_short 
15    0x8000		uart_int	amp_duration_too_short 
16   0x10000		uart_int 
17   0x20000		uart_int 
18   0x40000		uart_int 
19   0x80000		timer_int 
20  0x100000		eject_switch_int 
21  0x200000		pneumatic_fault_int 
22  0x400000		spi_failed_busy0 
23  0x800000		spi_failed_busy1 
24 0x1000000		spi_failed_busy2 
25 0x2000000		spinner_int 

*/
/* RF Specific  */
/* NONE */

/* MASTER specific */
/* 0x2000 bit - 13, probe_id_int */
/* 0x4000 bit - 14, tune_int */
/* 0x8000 bit - 15, uart_int */
/* 0x10000 bit - 16, uart_int */
/* 0x20000 bit - 17, uart_int */
/* 0x40000 bit - 18, uart_int */
/* 0x80000 bit - 19, timer_int */
/* 0x100000 bit - 20, eject_switch_int */
/* 0x200000 bit - 21, pneumatic_fault_int */
/* 0x400000 bit - 22, spi_failed_busy0 */
/* 0x800000 bit - 23, spi_failed_busy1 */
/* 0x1000000 bit - 24, spi_failed_busy2 */
/* 0x2000000 bit - 25, spinner_int */


/* PFG Specific 
13	amp_duration_too_short
14	amp_duration_too_short
15	amp_duration_too_short

*/

/* GRADIENT Specific  
13	spi_failed_busy
14	fifo_amp_duration_too_short
15	fifo_amp_duration_too_short
16	fifo_amp_duration_too_short
17	fifo_amp_duration_too_short
18	ecc_duration_too_short
19	ecc_duration_too_short
20	ecc_duration_too_short
21	ecc_duration_too_short
22	slew_limit_exceeded
23	slew_limit_exceeded
24	slew_limit_exceeded
25	slew_limit_exceeded
26	grad_isi_fault

*/

/* DDR  Specific 
12 	invalid_opcode_int_status_0
13	host_int
14	adc_overflow_int
15	adc_underflow_int
16	adc_threshold_int
17	AD6634_OVF_int
18	sync_int
19	sync_int
20	sync_int
21	sync_int
22	gp_int	

*/
/* ----  FIFO Gate Bits ----- */
#define FF_GATE_0	(0x1)
#define FF_GATE_1	(0x2)
#define FF_GATE_2	(0x4)
#define FF_GATE_3	(0x8)
#define FF_GATE_4	(0x10)
#define FF_GATE_5	(0x20)
#define FF_GATE_6	(0x40)
#define FF_GATE_7	(0x80)
#define FF_GATE_SW1	(0x100)
#define FF_GATE_SW2	(0x200)
#define FF_GATE_SW3	(0x400)
#define FF_GATE_SW4	(0x800)


/* --------- SW vs Fifo control switch define -------- */
#define SELECT_SW_CONTROLLED_OUTPUT 0
#define SELECT_FIFO_CONTROLLED_OUTPUT 1

/* Fifo Object State */

/* list of common FIFO registers for all controllers (except lock wich has none )*/

typedef struct _fifo_reg_addrs {
	volatile unsigned int* const pFifoWrite;
	volatile unsigned int* const pFifoControl;
        volatile unsigned int* const pFifoStatus;
        volatile unsigned int* const pFifoIntStatus;
        volatile unsigned int* const pFifoIntEnable;
        volatile unsigned int* const pFifoIntClear;
        volatile unsigned int* const pFifoClrCumDuration;
        volatile unsigned int* const pFifoCumDurationLow;
        volatile unsigned int* const pFifoCumDurationHi;
        volatile unsigned int* const pFifoInstructionFIFOCount; 
        volatile unsigned int* const pFifoDataFIFOCount;
        volatile unsigned int* const pFifoInvalidOpCode;
        volatile unsigned int* const pFifoInstrFIFOCountTotal;
        volatile unsigned int* const pFifoClearInstrFIFOCountTotal;
	} FIFO_REGS;

typedef struct _fifo_error_msg {
	int donecode;
	int errorcode;
       } FIFO_ERROR_MSG;

/* --------- ANSI/C++ compliant function prototypes --------------- */

#if defined(__STDC__) || defined(__cplusplus)

extern void  cntrlFifoInit(FIFO_REGS *pFifoRegs, unsigned int intMask, int fifoInstSize);
extern void  cntrlFifoDelete();
extern int   cntrlFifoStart();
extern int   cntrlFifoStartOnSync();
extern int   cntrlFifoStartAndSync();
extern void  cntrlFifoClearStartMode();
extern void  cntrlFifoReset();
extern void  cntrlFifoPut(long codes);
extern void  cntrlFifoPIO(long *pCodes, int num);
extern unsigned int  cntrlFifoStatReg();
extern unsigned int cntrlFifoIntrpMask();
extern int cntrlFifoIntrpSetMask(unsigned int maskbits);
extern int cntrlFifoIntrpClearMask(unsigned int maskbits);
extern int   cntrlFifoRunning();
extern int   cntrlFifoEmpty();
extern int cntrlInstrFifoCount();
extern int   cntrlDataFifoCount();
extern void  cntrlFifoWait4Stop();
extern void  cntrlFifoBusyWait4Stop();
extern void  cntrlFifoWait4StopItrp();
extern void  cntrlFifoCumulativeDurationGet(long long *duration);
extern int cntrlInvalidOpCode();
extern long cntrlInstrCountTotal();
extern int cntrlClearInstrCountTotal();


/* Though these functions are in the controller specific Fifo files 
 * (e.g. rfFifo.c, pfgFifo.c) The functions are common to all FIFO 
 * base controllers, as such are include here in this header file.
 *
 *	Greg Brissey 9/16/04
 */

extern int fifoEncodeSWItr(int SWitr, int *instrwords);
extern int fifoEncodeDuration(int write, int duration,int *instrwords);
extern void setFifoOutputSelect(int SwOrFifo);

#else
/* --------- NON-ANSI/C++ prototypes ------------  */

#endif

#ifdef __cplusplus
}
#endif

#endif
