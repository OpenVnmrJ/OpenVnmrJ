/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef INCgsClkLibh
#define INCgsClkLibh


/* ------------- Make C header file C++ compliant ------------------- */
#ifdef __cplusplus
extern "C" {
#endif

/* test green springs interrupt clock board DIGITAL-48 in 610 carrier */

#define BASE_ADDR	0xffff1000

/* Industial Packs A, B, C, D */
#define IP_A_OFFSET 	0x0
#define IP_B_OFFSET 	0x100
#define IP_C_OFFSET 	0x200
#define IP_D_OFFSET 	0x300

/* DIGITAL-48   2 - MC68230 offsets */
#define CLK_1_OFFSET 0x20
#define CLK_2_OFFSET 0x60

/* Timer Control Register */
#define TCR_OFFSET	0x1

/* Timer Interrupt Vector Register */
#define TIVR_OFFSET	0x3

/* Counter Perload Register H-L */
#define CPRH_OFFSET	0x7
#define CPRM_OFFSET	0x9
#define CPRL_OFFSET	0xB

/* Timer counter register  H-L	*/
#define CRH_OFFSET	0xF
#define CRM_OFFSET	0x11
#define CRL_OFFSET	0x13


/* TIMER_STATUS_REG	*/
#define TSR_OFFSET	0x15


/* reg address; CLK_REG(TCR_OFFSET,CLK_1_OFFSET,IP_A_OFFSET) */
#define CLK_REG(a, b)		((volatile unsigned char* const) (a + b))
#define ADD_OFFSET(a, b)	(BASE_ADDR + a + b)

/* Note: internal clock 8MHz, prescaler = 32 -> 250KHz or 4usec resolution */
/*       external clock can only 1/8 on internal input clock or 8Mhz/8 = 1Mhz */
/*       or a maximum resolution of 1usec */
#define INTERNAL_CLK		0x0
#define EXTERNAL_CLK		0x1
#define INTRP_RELOAD_INTCLK	0xa0
#define INTRP_ROLLOVR_INTCLK	0xb0
#define INTRP_ROLLOVR_EXTCLK	0xb6


#define MAX_CLOCKS 4

typedef struct          /* CLOCK Object structure */
    {
    FUNCPTR 	pIntrHandler;
    char* 	clkBaseAddr;
    int		TcrVal;
    int		clkNum;
    int		UserArg;
    int		vmeItrVector;
    int		vmeItrLevel;
    int		clkRes; /* resolution in usec internal=4usec,ext=1usec */
}  CLOCK_OBJ;

typedef CLOCK_OBJ *CLOCK_ID;


/* --------- ANSI/C++ compliant function prototypes --------------- */

#if defined(__STDC__) || defined(__cplusplus)

 
IMPORT    CLOCK_ID     gsClkCreate(int vec, FUNCPTR IntrHandler, int arg);
IMPORT    int          gsClkStart(CLOCK_ID pClkId, unsigned long count);
IMPORT    int          gsClkStop(CLOCK_ID pClkId);
IMPORT	  int 	       gsClkItrLatency(int clock);
IMPORT    VOID         gsClkReset(CLOCK_ID pClkId);
IMPORT    VOID         gsClkResetZDS(CLOCK_ID pClkId);
IMPORT    int          gsClkDelete (CLOCK_ID);
 
/* --------- NON-ANSI/C++ prototypes ------------  */

#else
 
 
#endif  /* __STDC__ */
 
#ifdef __cplusplus
}
#endif

#endif /* INCrngBlkLibh */
