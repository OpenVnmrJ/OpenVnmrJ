/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef INCnhardwareh
#define INCnhardwareh

/* ------------- Make C header file C++ compliant ------------------- */
#ifdef __cplusplus
extern "C" {
#endif

typedef short	shim_t;

/*
DESCRIPTION

System VME Memory Map of console

Standard: 0x0 - 0x00FFFFFF (16 MB)
Extended: 0x01000000 - 0xF0000000 (3.824 GB)
Short I/O: 0xFFFF0000 - 0xFFFFFFFF (64 KB)

1 MVME162LX at 4MB STD Adr Space, 4-64MB EXT Adr Space
4 FIFO boards at 512K STD Adr Space
4 STM boards at 512K STD Adr Space, 64MB EXT Adr Space
4 ADC boards at 512K STD Adr Space
1 Automation board at 512 KB of STD and 1MB EXT Adr Space

*/

/* Hardware Base Address */
/* --- OUTPUT Boards --- */
#define FIFO_BASE_ADR 	0x00400000L
#define FIFO_BASE_ADR2 	0x00480000L
#define FIFO_BASE_ADR3	0x00500000L
#define FIFO_BASE_ADR4	0x00580000L

/** REMOVE THIS AFTER TEST */
#define STM_ORIG_VERS

/* --- STM Boards --- */
#define STM_BASE_ADR	0x00600000L
#define STM_BASE_ADR2	0x00680000L
#define STM_BASE_ADR3	0x00700000L
#define STM_BASE_ADR4	0x00780000L

/* --- STM EEProm Address --- special address that have to use the VME diagnostic prom AM code */
#define STM_EEBASE_ADR    0xe0600000L
#define STM_EEBASE_ADR2   0xe0680000L
#define STM_EEBASE_ADR3   0xe0700000L
#define STM_EEBASE_ADR4   0xe0780000L

#define STM_VME_A24_EEBASE_OFFSET   0x00040000L


/* --- STM Data Memory --- */
/* 64MB address space A32/D64 */
#define STM_MEM_BASE_ADR	0x10000000L
#define STM_MEM_BASE_ADR2	0x14000000L
#define STM_MEM_BASE_ADR3	0x18000000L
#define STM_MEM_BASE_ADR4	0x1C000000L

#define STM_AP_ADR	0x0E00
#define STM_AP_ADR2	0x0E20
#define STM_AP_ADR3	0x0E40
#define STM_AP_ADR4	0x0E60


/* --- ADC board --- */
#define ADC_BASE_ADR	0x00800000L
#define ADC_BASE_ADR2	0x00880000L
#define ADC_BASE_ADR3	0x00900000L
#define ADC_BASE_ADR4	0x00980000L

#define ADC_AP_ADR1A	0x0E80		/* ap address 1st ADC on 1st STM */
#define ADC_AP_ADR1B	0x0E90		/* ap address 2nd ADC on 1st STM */
#define ADC_AP_ADR2A	0x0EA0		/* ap address 1st ADC on 2nd STM */
#define ADC_AP_ADR2B	0x0EB0		/* ap address 2nd ADC on 2nd STM */
#define ADC_AP_ADR3A	0x0EC0	
#define ADC_AP_ADR3B	0x0ED0
#define ADC_AP_ADR4A	0x0EE0
#define ADC_AP_ADR4B	0x0EF0

/* --- Automation Board --- */
#define AUTO_BRD_TYPE_I         1	/* MSR I */
#define AUTO_BRD_TYPE_II        2	/* MSR II */
#define AUTO_BASE_ADR		0x00A00000L
#define AUTO_MEM_BASE_ADR	0x20000000L

/* Note: The SM defines are now dynamicly determine since there are 
         2 MSR card with different memory sizes  9/25/00 
         AUTO_SM_NET_ANCHOR_VME = 0x200F7000L or 0x203F7000L 
*/
#define AUTO_SM_NET_ANCHOR_VME 	0x200F7000L	/* 32+K top memory shared */
#define AUTO_SM_NET_MEM_SIZE 	0x00008000L	/* 32K for SM backplane NET */
#define AUTO_SHARED_MEM_VME 	0x200FF000L	/* 4K top memory mailboxs */
#define AUTO_SHARED_MEM_OFFSET 	0x000FF000L	/* 4K top memory shared */
#define AUTO_MEM_END_OFFSET	0x000FFFFFL
#define AUTO_MEM_BASE_END	0x200FFFFFL

#define AUTO_AP_ADR	0x0D00

/* --- BreakOut Board --- */
#define BOB_AP_ADR	0x0D10


/* Short I/O space */
/* for mv162 checks to be sure address is not greater than 0xffffL   */
/*  then or's in 0xffff0000 , this is also compatible with mv147 */

/* VME Access type , see vme.h */
#define FIFO_VME_ACCESS_TYPE 	VME_AM_STD_USR_DATA
#define STM_VME_ACCESS_TYPE 	VME_AM_STD_USR_DATA
#define ADC_VME_ACCESS_TYPE 	VME_AM_STD_USR_DATA
#define AUTO_VME_ACCESS_TYPE 	VME_AM_STD_USR_DATA
#define STM_MEM_ACCESS_TYPE 	VME_AM_EXT_USR_DATA
#define AUTO_MEM_ACCESS_TYPE 	VME_AM_EXT_USR_DATA

/* ------------------- STM --------------------------------- */
/* STM Register Offsets  */
#ifdef STM_V1
#define STM_CR		0x00	/* 16 bits R/W */
#else
#define STM_CR		0x1C	/* 16 bits R/W,only lower 8 bits meaningfull */
#endif

#define STM_SR		0x10	/* 16 bits R Only */

/* BIG ENDIAN ORDER */
/* Read Registers 16-bit access only (no 32-bit) */
#define STM_SRC_ADR0	0x02	/* Source Address low word (16 bits) */
#define STM_SRC_ADR1	0x00	/* Source Address High word (16 bits) */
#define STM_DST_ADR0	0x06	/* Destination Address low word (16 bits) */
#define STM_DST_ADR1	0x04	/* Destination Address High word (16 bits) */
#define STM_NP_CNT0	0x0a	/* Remaining # Points low word (16 bits) */
#define STM_NP_CNT1	0x08	/* Remaining # Points High word (16 bits) */
#define STM_CT_CNT0	0x0e	/* Remaining # Transients low word (16 bits) */
#define STM_CT_CNT1	0x0c	/* Remaining # Transients High word (16 bits) */
#define STM_TAG_ADDR	0x12	/* Tag Word (16 bits) */

/* Write Registers 16-bit access only (no 32-bit) */
#define STM_TWRD_A0	0x12	/* Test Word A0 low word (16-bits) */
#define STM_TWRD_A1	0x10	/* Test Word A1 high word (16-bits) */
#define STM_TWRD_B0	0x16	/* Test Word B0 low word (16-bits) */
#define STM_TWRD_B1	0x14	/* Test Word B1 high word (16-bits) */

/* different for the HS DTM  (ADM) */
#define HSADM_TWRD_B0   0x10    /* Test Word A0 low word (16-bits) */
#define HSADM_TWRD_A0   0x14    /* Test Word B0 low word (16-bits) */
 
#ifndef STM_V1
#define STM_MAX_SUML    0x18    /* low word of maxsum apbus register setting */
#define STM_MAX_SUMH    0x1a    /* high word of maxsum apbus register setting */
#define STM_APITR_MASK  0x1E	/* VME access to APbus intrp mask register */
#endif

/*---  STM Status Register Bit Positions ---*/
#define ADC_1_CONNECT 	0x1	/* bit 0 - ADC 1 connected if bit 0 */
#define ADC_2_CONNECT 	0x2	/* bit 1 - ADC 2 connected if bit 0 */
#define ADM_OVFLOW 	0x8	/* bit 3 - HS ADM ADC OverFlow Condition if bit 1 */
#define CT_COMPLETE 	0x10	/* bit 4 - Remaining Transients == 0 if bit=1 */
#define NP_INCOMPLETE 	0x20	/* bit 5 - Remaining Points != 0 if bit=1 */
#define MAX_SUM		0x40	/* bit 6 - Maxsum met or execeeded if bit=1 */
#define APBUS_ITRP	0x80	/* bit 7 - APbus interrupt occurred if bit=1 */


/*---  STM Control Register Bit Positions ---*/
#define READ_TST_DATA 	0x1	/* bit 0 - Read test data if bit=1 */
#define ADM_ADCOVFL_MASK 0x8	/* bit 3 - Enable HS DTM's ADC OverFlow interrupt if bit=1 */
#define RTZ_ITRP_MASK   0x10	/* bit 4 - Enable RTZ interrupt if bit=1 */
#define RPNZ_ITRP_MASK 	0x20	/* bit 5 - Enable RPNZ if bit=1 */
#define MAX_SUM_ITRP_MASK 0x40	/* bit 6 - Enable Maxsum interrupt if bit=1 */
#define APBUS_ITRP_MASK	0x80	/* bit 7 - Enable APbus interrupt if bit=1 */
#define STM_RESET	0x100	/* bit 8 - Reset STM Board if bit=1 */
#define FAKE_CTC_PULSE	0x200	/* bit 9 - Fake CTC Pulse if bit=1 */
#define MAXSUM_ADMOVFL_RESET 0x400 /* bit 10 - Reset HSADM MAX_SUM and ADC OverFlow if bit=1 */

#define STM_ALLITRPS	0xF8	/* VME Interrupt Bits 1 */

/* STM APbus Register Offset Addresses */

#define STM_AP_CR	0x00	/* AP Sequence Control Register */
#define STM_AP_TAG_ADDR	0x02	/* Tag Word (16 bits) */
#define STM_AP_SPARE1	0x04
#define STM_AP_SPARE2	0x06
#define STM_AP_SPARE3	0x08
#define STM_AP_SPARE4	0x0a

#define STM_AP_MAXSUM0	0x0c	/* Maxsum word 0 */
#define STM_AP_MAXSUM1	0x0e	/* Maxsum word 1 */
#define STM_AP_SRC_ADR0	0x10	/* Source Address low word (16 bits) */
#define STM_AP_SRC_ADR1	0x12	/* Source Address High word (16 bits) */
#define STM_AP_DST_ADR0	0x14	/* Destination Address low word (16 bits) */
#define STM_AP_DST_ADR1	0x16	/* Destination Address High word (16 bits) */
#define STM_AP_NP_CNT0	0x18	/* Remaining # Points low word (16 bits) */
#define STM_AP_NP_CNT1	0x1a	/* Remaining # Points High word (16 bits) */
#define STM_AP_NTR_CNT0	0x1c	/* Remaining # Transients low word (16 bits) */
#define STM_AP_NTR_CNT1	0x1e	/* Remaining # Transients High word (16 bits) */

/*---  STM APbus Control Register Bit Positions ---*/
#define STM_AP_PHASE_CYCLE_MASK  0x3	    /* bit 0 - 1 Mode bits */
#define STM_AP_REVERSE_PHASE_CYCLE  0x4	    /* bit 2 - Mode bit */
#define STM_AP_MEM_ZERO          0x8    /* bit 3 - Memory zero source data */
#define STM_AP_SINGLE_PRECISION  0x10   /* bit 4 - 1=16 bit, 0=32 bit */
#define STM_AP_ENABLE_ADC1	     0x20   /* bit 5 - Enable STM */
#define STM_AP_ENABLE_ADC2	     0x40   /* bit 6 - Enable STM */
#define STM_AP_ENABLE_STM	     0x80   /* bit 7 - Enable STM */
/* the one bit loads both np and address, loading separately is not an option */
#define STM_AP_RELOAD_NP_ADDRS	     0x100  /* bit 8 - Reload np and address*/
/* #define STM_AP_RELOAD_NP	     0x100  /* bit 8 - Reload np */
/* #define STM_AP_RELOAD_ADDRS	     0x200  /* bit 9 - Reload Addresses */
#define ADM_AP_LIQ_OR_WL_INPUT       0x400 /* bit 10 - Switch between liquids & solids signal inputs */
#define ADM_AP_OVFL_ITRP_MASK     0x800 /* bit 11 - En. HS DTM ADC's OverFlow */
#define STM_AP_RTZ_ITRP_MASK     0x1000 /* bit 12 - En. Remaining transients==0 */
#define STM_AP_RPNZ_ITRP_MASK    0x2000 /* bit 13 - En. Remaining Pts != 0  */
#define STM_AP_MAX_SUM_ITRP_MASK 0x4000 /* bit 14 - Enable Maxsum interrupt */
#define STM_AP_IMMED_ITRP_MASK   0x8000 /* bit 15 - Enable Immediate interrupt */

/* ---- STM AP Interrupt Mask Bit Positions (via VME) ------ */
#define AP_RTZ_ITRBIT	0x1
#define AP_RPNZ_ITRBIT	0x2
#define AP_MAX_ITRBIT	0x4
#define AP_IMMED_ITRBIT 0x8
#define VME_RTZ_ITRBIT	RTZ_ITRP_MASK  /* 0x10 */
#define VME_RPNZ_ITRBIT	RPNZ_ITRP_MASK  /* 0x20 */
#define VME_MAX_ITRBIT	MAX_SUM_ITRP_MASK  /* 0x40 */
#define VME_IMMED_ITRBIT APBUS_ITRP_MASK  /* 0x80 */

  /*   New for 5 MHz High Speed  STM/ADC */
#define AP_ADCOVFL_ITRBIT  0x100
#define VME_ADCOVFL_ITRBIT  0x200


/* ----------------------------------------------------------- */
/* ----------------------------------------------------------- */

/* ------------------- FIFO  --------------------------------- */
/* FIFO Register Offsets  */

/* -------------------------------------------------------*/
#define FIFO_DIAG_PROM	0x00000	/* Diagnostic Prom */

/* -------------------------------------------------------*/
#define PFIFO_WRITE 	0x40000	/* Write into Pre-FIFO 64-bit */
#define VME_FIFO_RD 	0x40000	/* Read Contents of FIFO 64-bits */

/* -------------------------------------------------------*/
/* 2 32-bits read to obtain 64-bit FIFO Word */
#define LASTFIFOWRD 	0x60018	/* Read Last FIfo Word Latched Out 32-bits */

/* -------------------------------------------------------*/
#define DIAGTIMER 	0x6001C	/* Read Timer Count 24-bits */

/* -------------------------------------------------------*/
/* Contents: 17-8 Address(10bits) 7-0 Data (8bits) */
#define AP_RD_BK_FIFO	0x60010	/* Apbus Read Back Fifo  (32-bit)*/

/* -------------------------------------------------------*/
#define TAG_FIFO	0x60014	/* TAG Value 18-bits */

/* -------------------------------------------------------*/
#define FIFO_SR		0x60000	/* 32 bits */

/* --- Definition of Bits --- */
#define FNOTRUNNING	0x0000001L  	/* Fifo running if bit=1 */
#define FLOOPING	0x0000002L  	/* Fifo Looping if bit=1 */
#define FNOTEMPTY	0x0000004L  	/* Fifo Not Empty if bit=1 */
#define FNOTFULL	0x0000008L  	/* Fifo Not Full  if bit=1 */
#define PFNOTEMPTY	0x0000010L  	/* Pre Fifo Not Empty if bit=1 */
#define PFAMEMPTY	0x0000020L  	/* Pre Fifo Almost Empty if bit=1 */
#define PFAMFULL	0x0000040L  	/* Pre Fifo Almost Full if bit=1 */
#define PFNOTFULL	0x0000080L	/* pre Fifo Not Full if bit=1 */
#define FSTRTEMPTY	0x0000100L	/* Fifo Started while empty if bit=1 */
#define FSTRTHALT	0x0000200L	/* Fifo Started with haltop if bit=1 */
#define NETBL		0x0000400L	/* Not Enough Time Between Loops */
#define FORP		0x0000800L	/* FIFO Out Run Pre-fifo */
#define FSTOPOEMPTY	0x0001000L	/* FIFO Stopped on Empty */
#define APTIMEOUT	0x0002000L	/* APbus Time Out */
#define RESERVE1	0x0004000L	/* */
#define RESERVE2	0x0008000L	/* */
#define APRDBKFNEMPTY 	0x0010000L	/* APbus Read Back FIFO Not Empty */
#define APRDBKFAFULL	0x0020000L	/* APbus Read Back FIFO Almost Full */
#define APRDBKFFULL	0x0040000L	/* APbus Read Back FIFO Full */
#define TAGFNOTEMPTY	0x0080000L	/* TAG FIFO NOT Empty */
#define TAGFAFULL 	0x0100000L	/* TAG FIFO Almost Full */
#define TAGFNOTFULL 	0x0200000L	/* TAG FIFO Not Full */
#define FWRTINSYNC 	0x0400000L	/* Writes to FIFO still in Sync */
#define NETBAP 		0x0800000L	/* Not Enough Time Between APbus */


/* -------------------------------------------------------*/
#define FIFO_CR		0x60004	/* 8 bits */
/* --- Definition of Bits --- */
#define STARTFIFO	0x01	/* Start the FIFO */
#define STARTONSYNC	0x02	/* Start on Sync */
#define AUTOSTART	0x04	/* Auto Start */
#define RESETFIFO	0x08	/* Reset FIFO */
#define RESETSTATEMACH	0x10	/* Reset State Machine */
#define RESETAPBUS	0x20	/* Reset AP Bus */
#define RESETTAGFIFO	0x40	/* Reset TAG FIFO */
#define RESETAPRDBKFIFO	0x80	/* Reset APbus Read Back FIFO */
#define RESETFIFOBRD    0xf8	/* Reset Entire Board */

/* -------------------------------------------------------*/
#define FIFO_OPSR	0x60008	/* 16 bits */
/* --- Definition of Bits (positive logic, bit=1) --- */
#define APBUSTYPE2	0x0001	/* AP bus type 2 present */
#define HSLINEMEZZ	0x0002	/* HSLINES Mezzanine present */
#define ROTORSYNC	0x0004	/* RotoSync Present */

/* -------------------------------------------------------*/
#define FIFO_INTRP_MASK	0x6000C	/* FIFO Interrupt Mask */
/* --- Definition of Bits (positive logic, bit=1) --- */
#define PFAMEMPTY_I	0x8000	/* Pre-Fifo Almost Empty */
#define PFAMFULL_I	0x4000  /* Pre Fifo Almost Full */
#define FSTOPPED_I	0x2000  /* Fifo Stopped */
#define FSTRTEMPTY_I	0x1000	/* Fifo Started while empty */
#define FSTRTHALT_I	0x0800	/* Fifo Started with haltop */
#define NETBL_I		0x0400	/* Not Enough Time Between Loops */
#define FORP_I		0x0200	/* FIFO Out Run Pre-fifo */
#define NETBAP_TOUT_I 	0x0100  /* Not Enough Time Between APbus or AP Timeout*/
#define SW1_I 		0x0080  /* Software Programmed Intrp 1 */
#define SW2_I 		0x0040  /* Software Programmed Intrp 2 */
#define SW3_I 		0x0020  /* Software Programmed Intrp 3 */
#define SW4_I 		0x0010  /* Software Programmed Intrp 4 */
#define TAGFNOTEMPTY_I	0x0008  /* TAG FIFO NOT Empty */
#define TAGFAFULL_I 	0x0004  /* TAG FIFO Almost Full */
#define APRDBKFNEMPTY_I 0x0002  /* APbus Read Back FIFO Not Empty */
#define APRDBKFAFULL_I	0x0001  /* APbus Read Back FIFO Almost Full */
#define FF_ALLITRPS	0xFFFF  /* All Fifo interrupt Bits */

/* Control Lines for FIFO Word in 32 bit integer */
/* The control lines are the 8 msb's of the 64 or 96 bit fifo word */
/*			  87654321      */
#define	RSYNC		0x80000000
#define	EXTCLK		0x40000000
#define	SWINTRP		0x20000000		/* just the control field */
#define	SWIDTAG		0x10000000
#define	CTC		0x08000000
#define	LPSTRT		0x04000000
#define	LPEND		0x02000000
#define	LPCNT		0x06000000
#define APWRT		0x01000000
#define APRD		0x01800000
#define HALTOP		0x00800000
#define	SWINT1		0x20000040
#define	SWINT2		0x20000020
#define	SWINT3		0x20000010
#define	SWINT4		0x20000008

#define HS_LINE_MASK	0x07FFFFFF
#define CW_DATA_FIELD_MASK 0x00FFFFFF
#define DATA_FIELD_SHFT_IN_HSW	5
#define DATA_FIELD_SHFT_IN_LSW	27
#define MAX_HW_LOOPS	16777215

#define CL_ROTOR_SYNC   RSYNC
#define CL_EXT_CLOCK    EXTCLK
#define CL_SW_PRG_INT   SWINTRP
#define CL_SW_ID_TAG    SWIDTAG
#define CL_CTC          CTC
#define CL_START_LOOP   LPSTRT
#define CL_END_LOOP     LPEND
#define CL_LOOP_COUNT   LPCNT
#define CL_AP_BUS       APWRT
#define CL_DELAY        0x00000000
 
#define ROTOR_SYNC_HSLINE 0x4000000	/* bit 26 from bit 0 */


/* -------------------------------------------------------*/
/*	ADC Defines
/* -------------------------------------------------------*/
/* -------------------------------------------------------*/
#define DSP_COEF_OFFSET	0x04000
#define DSP_PROM_VERSION_OFFSET	0x07ffbc
#define DSP_PROM_NO_DSP		0
#define DSP_PROM_NO_DOWNLOAD	1
#define DSP_PROM_DOWNLOAD	2

#define ADC_SR		0x7fff6
/* --- Definition of Bits (positive logic, bit=1) --- */
#define RCVR_OVRLOAD1	0x0001		/* Receiver 1 Over Load */
#define RCVR_OVRLOAD2	0x0002		/* Receiver 1 Over Load */
#define ADC_OVRFLOW	0x0004		/* ADC Over Flow */
#define ADC_DSP_NOTHOLD	0x0008		/* ADC DSP Not Holding */
#define ADC_DSP_NOTPRESENT 0x0010	/* ADC DSP Not Present */
#define ADC_DSP_NOTBUSY	0x0020		/* ADC DSP Not Present */
#define ADC_INTR_PEND	0x0040		/* Interrupt Pending */
#define ADC_BUS_ITR_LVL	0x0080		/* VME Interrupt Level 2 */

/* -------------------------------------------------------*/
#define ADC_CR		0x7fff0
/* --- Definition of Bits (positive logic, bit=1) --- */
/* interrupts enable when bit = 1 */
#define ADC_READ_IMGREG	 0x0008	/* This bit, 0 for real & 1 for imagnary */
#define ADC_RC1_OVRFLW_I 0x0010	/* ADC Receiver #1 OverLoad Interrupt */
#define ADC_RC2_OVRFLW_I 0x0020	/* ADC Receiver #2 OverLoad Interrupt */
#define ADC_OVRFLOW_I 	 0x0040	/* ADC OverFlow interrupt (enable bit=1) */
#define ADC_RESET	 0x0100	/* ADC Reset Board */
#define ADC_FAKE_CT	 0x0200	/* Fake CTC for ADC */

#define ADC_ALLITRPS	 0x0070  /* All ADC interrupt Bits */

/* -------------------------------------------------------*/
#define ADC_AP_ITRP_MASK 0x7fff2		/* Apbus interrupt Mask */
/* --- Definition of Bits (positive logic, bit=1) --- */
/* interrupts enable when bit = 1 */
#define AP_RCV1_OVERLD_ENABLED	0x01		/* recv 1 overload ap mask bit is set */
#define AP_RCV2_OVERLD_ENABLED	0x02		/* recv 2 overload ap mask bit is set */
#define AP_ADC_OVERLD_ENABLED	0x04		/* ADC overflow ap mask bit is set */
#define AP_SPARE_MASK_ENABLED	0x08		/* SPARE ap mask bit is set */
#define RCV1_OVERLD_LATCHED	0x10		/* Recv 1  overload has occurred, bit cleared when reg read */
#define RCV2_OVERLD_LATCHED	0x20		/* Recv 2  overload has occurred, bit cleared when reg read */
#define ADC_OVERFLOW_LATCHED	0x40		/* ADC overflow has occurred, bit cleared when reg read */
#define SPARE_MASK_LATCHED	0x80		/* SPARED Bit, bit cleared when reg read */


#define ADC_READR	0x7fff4		/* ADC converted Value */

   
/* -------------------------------------------------------*/
/*  AP Control register offsets			*/
/* -------------------------------------------------------*/
#define ADC_AP_CNTRL1 0x00
#define ADC_AP_CNTRL2 0x02
#define ADC_AP_DSPCNTRL 0x06

/* -------------------------------------------------------*/
/*  AP Control register Bit definitions			*/
/* -------------------------------------------------------*/
#define ADC_AP_DSP_PHASE0 0x00000001
#define ADC_AP_DSP_PHASE1 0x00000002
#define ADC_AP_DSP_PHASE2 0x00000004
#define ADC_AP_DSP_MEM_ZERO 0x00000008 

#define ADC_AP_ENABLE_RCV1_OVERLD 0x00010000 
#define ADC_AP_ENABLE_RCV2_OVERLD 0x00020000
#define ADC_AP_ENABLE_ADC_OVERLD  0x00040000
#define ADC_AP_ENABLE_CTC	  0x00100000  /* enable CTC & Fake CTC */
#define ADC_AP_ENABLE_APTO_DSP	  0x00200000  /* enable Apbus access to DSP regs */
#define ADC_AP_CHANSELECT_POS	  24
#define ADC_AP_SELECT_CHAN0       0x00000000  /* enable 1st audio input */
#define ADC_AP_SELECT_CHAN1       0x01000000  /* enable 2nd audio input */
#define ADC_AP_SELECT_CHAN2       0x02000000  /* enable 3rd audio input */
#define ADC_AP_SELECT_CHAN3       0x03000000  /* enable 4th audio test input */
#define ADC_AP_NBUS_REQUEST       0x80000000  /* DSP sram access ?? */

/* -------------------------------------------------------*/
/*	Automation Defines  (162 perspective VME)
/* -------------------------------------------------------*/
/* -------------------------------------------------------*/
#define AUTO_SR		0x40000		/* R Only, 16-bits */

#define SPIN_NOT_REG	0x0001		/* Spinner not regulated */
#define BEAR_AIR_ON	0x0002		/* Bear Air On */
#define EJECT_AIR_ON	0x0004		/* Eject Air On */
#define SLWDROP_AIR_ON  0x0008		/* Slow Drop Air On */
#define SPIN_SPD_ZERO   0x0010		/* Slow Drop Air On */
#define SMPLE_AT_BOT    0x0020		/* Sample at Bottom of Magnet Bore  */
#define SMPLE_AT_TOP    0x0040		/* Sample at Top of Magnet Bore  */
#define NOT_LOCKED      0x0080		/* Not Lock on Sample */
#define VT_ATTEN        0x0100		/* VT Requires Attention */

/* -------------------------------------------------------*/
#define AUTO_CR		0x40002		/* R/W  8 - bits */

#define AUTO_RESET_ALL  0x01		/* Reset Board all Functions */
#define AUTO_RESET_332  0x02		/* reset mc86332 */
#define AUTO_RESET_AP   0x04		/* reset AP Bus */
#define AUTO_RESET_DCNTL   0x08		/* reset DRAM Controller */
#define AUTO_RESET_QUART   0x10		/* reset Quad UART Controller */

/* -------------------------------------------------------*/
#define AUTO_INTRP_MASK	0x40003		/* Automation Interrupt mask 8-bits*/

/* --- Definition of Bits (positive logic, bit=1) --- */
#define GNRL_MAIL_I	0x80		/* General Interrupt MailBox  */
#define SPIN_MAIL_I	0x40  		/* Spinner Interrupt MailBox */
#define VT_MAIL_I	0x20  		/* Spinner Interrupt MailBox */
#define SHIM_MAIL_I	0x10  		/* Shim Interrupt MailBox */
#define QUART_I		0x08  		/* QUART Interrupt */
#define MC332_I		0x04  		/* 332 MCU VME Interrupt */
#define AUTO_ALLITRPS   0xFC		/* All Bits */
/* --   new for MSR II ----- */
#define MISC_I		0x02		/* Misc Interrupts, probe id, timestamp clock */
#define MSRUNUSED	0x01
#define AUTOII_ALLITRPS   0xFE		/* All Bits */

/* -------------------------------------------------------*/
#define AUTO_SPIN_CR	0x40004		/* Spinner Control Register 8-bit */

#define TURN_EJECT_BIT   0x01		/* Turn Eject Air on */
#define TURN_BEAR_BIT    0x02		/* Turn Bearing Air on */
#define TURN_SDROP_BIT   0x04		/* Turn Slow Drop Air on */

/* -------------------------------------------------------*/
#define AUTO_INTRP_TREG 0x40005		/* Interrupt Test Register 8-bits W-Only */

#define POST_GMAIL_I	0x80		/* Post a General Mail Box Interrupt */
#define POST_SPNMAIL_I	0x40		/* Post a Spinner Mail Box Interrupt */
#define POST_VTMAIL_I	0x20		/* Post a VT Mail Box Interrupt */
#define POST_SHMMAIL_I	0x10		/* Post a Shim Mail Box Interrupt */
#define POST_332MPU_I	0x04		/* Post a 332 MCU VME Interrupt */


/* -------------------------------------------------------*/
#define AUTO_HSnR_SR  0x40006		/* Interrupt Test Register 8-bits R-Only */

#define HSR_CONFIG_MASK 0x0F		/* COnfiguration bits 0 - 3 */
#define HSR_GATING_MASK 0xF0		/* Transmitter Gating bits 4 - 7 */

/* -------------------------------------------------------*/
/* 332 MailBox Address 16 - bit Registers   R/W */
#define GEN_MAILBOX	0x40008		/* General MailBox Interrupt */
#define SPIN_MAILBOX	0x4000A		/* Spinner MailBox Interrupt */
#define VT_MAILBOX	0x4000C		/* VT MailBox Interrupt */
#define SHIM_MAILBOX	0x4000E		/* Shim MailBox Interrupt */

/*-------------------------------------------------------*/
/* Shared Memory Offset for Mail Boxs, etc..		*/
#define  GEN_MBOX_OFFSET 	0x0		/* Size 252, ack 4 =  0x100  = 256 */
#define  GEN_MBOX_OFFSET_ACK 	0x0FC		/* Size 4 0xFB */
#define  SPIN_MBOX_OFFSET 	0x100
#define  SPIN_MBOX_OFFSET_ACK 	0x1FC
#define  SHIM_MBOX_OFFSET 	0x200
#define  SHIM_MBOX_OFFSET_ACK 	0x2FC
#define  VT_MBOX_OFFSET 	0x300
#define  VT_MBOX_OFFSET_ACK 	0x3FC
#define  SPIN_AP_MBOX_OFFSET 	0x400
#define  SPIN_AP_MBOX_OFFSET_ACK 	0x4FC
#define  SHIM_AP_MBOX_OFFSET 	0x500
#define  SHIM_AP_MBOX_OFFSET_ACK 	0x5FC
#define  VT_AP_MBOX_OFFSET 	0x600
#define  VT_AP_MBOX_OFFSET_ACK 	0x6FC
#define  AUTO_MBOX_SIZE		0x0FC   /* 252 + 4 ack = 0x100 total 256 */
#define  AUTO_MBOX_TOTAL_SIZE	0x700
#define  SHARED_SHIMS_OFFSET	0x800
#define  SHARED_VT_OFFSET	0x900
#define  SHARED_GPA_OFFSET	0xA00

/*-------------------------------------------------------*/
/* Shared M332 HeartBeat Address 			*/
#define AUTO_HEARTBEAT_OFFSET	(AUTO_MEM_END_OFFSET - 3)  /* 0x000FFFFCL */


/* AUTO APbus Register Offset Addresses */

#define AUTO_AP_DATA	0x00	/* 16-bit Data Word */
#define AUTO_AP_DEST	0x02	/* Data Destination (Spin,Shim,VT) */

#define AUTO_AP_DEST_SPIN 0x1	/* Data directed at Spinner */
#define AUTO_AP_DEST_VT   0x2	/* Data directed at VT */
#define AUTO_AP_DEST_SHIM 0x4	/* Data directed at Shim */

/* -------------------------------------------------------*/
/*       >>>>>>> 332 Addresses <<<<<          */
#define  MPU332_RAM	0x00800000
#define  MPU332_RAMEND  0x008FFFFF	/* 1 MB DRAM */


/**********************************************************8
* hardware macros
*/
#define FF_IMASK(a)	( (volatile unsigned long* const) (FIFO_INTRP_MASK + (a)))
#define FF_STATR(a)	( (volatile unsigned long* const) (FIFO_SR + (a)))
#define FF_OPTSR(a)	( (volatile unsigned long* const) (FIFO_OPSR + (a)))
#define FF_CNTRL(a)	( (volatile unsigned long* const) (FIFO_CR + (a)))
#define FF_PFLW(a)	( (volatile unsigned long* const) (PFIFO_WRITE + (a)))
#define FF_PFHW(a)    ( (volatile unsigned long* const) ((PFIFO_WRITE + 4) + (a)))
#define FF_PFEW(a)    ( (volatile unsigned long* const) ((PFIFO_WRITE + 8) + (a)))
#define FF_DIAGTIMER(a)	( (volatile unsigned long* const) (DIAGTIMER + (a)))
#define FF_APRDBK(a)	( (volatile unsigned long* const) (AP_RD_BK_FIFO + (a)))
#define FF_TAGFF(a)	( (volatile unsigned long* const) (TAG_FIFO + (a)))
#define FF_LASTWRD(a)	( (volatile unsigned long* const) (LASTFIFOWRD + (a)))
#define FF_READWRD(a)	( (volatile unsigned long* const) (VME_FIFO_RD + (a)))
#define FF_REGL(a,b)	( (volatile unsigned long* const) ((a) + (b)) )

#define STM_STATR(a)	( (volatile unsigned short* const) (STM_SR + (a)))
#define STM_CNTLR(a)	( (volatile unsigned short* const) (STM_CR + (a)))
#define STM_TAG(a)	( (volatile unsigned short* const) (STM_TAG_ADDR + (a)))
#define STM_CTHW(a)	( (volatile unsigned short* const) (STM_CT_CNT0 + (a)))
#define STM_CTLW(a)	( (volatile unsigned short* const) (STM_CT_CNT1 + (a)))
#define STM_NPHW(a)	( (volatile unsigned short* const) (STM_NP_CNT0 + (a)))
#define STM_NPLW(a)	( (volatile unsigned short* const) (STM_NP_CNT1 + (a)))
#define STM_SRCHW(a)	( (volatile unsigned short* const) (STM_SRC_ADR0 + (a)))
#define STM_SRCLW(a)	( (volatile unsigned short* const) (STM_SRC_ADR1 + (a)))
#define STM_DSTHW(a)	( (volatile unsigned short* const) (STM_DST_ADR0 + (a)))
#define STM_DSTLW(a)	( (volatile unsigned short* const) (STM_DST_ADR1 + (a)))
#ifndef STM_V1
#define STM_MAXHW(a)    ( (volatile unsigned short* const) (STM_MAX_SUMH + (a)))
#define STM_MAXLW(a)    ( (volatile unsigned short* const) (STM_MAX_SUML + (a)))
#define STM_AP_IMASK(a) ( (volatile unsigned short* const) (STM_APITR_MASK + (a)))
#endif
#define AP_STM_CNTLR(a)	((unsigned short const) (AP_STM_CR+(a)))
#define AP_STM_TAG(a)	((unsigned short const) (AP_STM_TAG_ADDR+(a)))
#define AP_STM_NTRHW(a)	((unsigned short const) (AP_STM_NTR_CNT0+(a)))
#define AP_STM_NTRLW(a)	((unsigned short const) (AP_STM_NTR_CNT1+(a)))
#define AP_STM_NPHW(a)	((unsigned short const) (AP_STM_NP_CNT0+(a)))
#define AP_STM_NPLW(a)	((unsigned short const) (AP_STM_NP_CNT1+(a)))
#define AP_STM_SRCHW(a)	((unsigned short const) (AP_STM_SRC_ADR0+(a)))
#define AP_STM_SRCLW(a)	((unsigned short const) (AP_STM_SRC_ADR1+(a)))
#define AP_STM_DSTHW(a)	((unsigned short const) (AP_STM_DST_ADR0+(a)))
#define AP_STM_DSTLW(a)	((unsigned short const) (AP_STM_DST_ADR1+(a)))

#define STM_REG(a,b)	( (volatile unsigned char* const) ((a) + (b)) )
#define STM_REGW(a,b)	( (volatile unsigned short* const) ((a) + (b)) )
#define STM_REGL(a,b)	( (volatile unsigned long* const) ((a) + (b)) )
#define ADC_REG(a,b)	( (volatile unsigned char* const) ((a) + (b)) )
#define BIT_MASK(a)	( (1 << (a)) )

#define ADC_STATR(a)	( (volatile unsigned short* const) (ADC_SR + (a)))
#define ADC_CNTRL(a)	( (volatile unsigned short* const) (ADC_CR + (a)))
#define ADC_ISTATR(a)	( (volatile unsigned short* const) (ADC_AP_ITRP_MASK + (a)))
#define ADC_READ(a)	( (volatile unsigned short* const) (ADC_READR + (a)))

#define AUTO_STATR(a)	( (volatile unsigned short* const) (AUTO_SR + (a)))
#define AUTO_CNTRL(a)	( (volatile unsigned char* const) (AUTO_CR + (a)))
#define AUTO_IMASK(a)	( (volatile unsigned char* const) (AUTO_INTRP_MASK + (a)))
#define AUTO_SPINCNTRL(a) ( (volatile unsigned char* const) (AUTO_SPIN_CR + (a)))
#define AUTO_INTRP_TST(a) ( (volatile unsigned char* const) (AUTO_INTRP_TREG + (a)))
#define AUTO_HSR_SR(a) ( (volatile unsigned char* const) (AUTO_HSnR_SR + (a)))
#define AUTO_GEN_MB(a) ( (volatile unsigned short* const) (GEN_MAILBOX + (a)))
#define AUTO_SPIN_MB(a) ( (volatile unsigned short* const) (SPIN_MAILBOX + (a)))
#define AUTO_VT_MB(a) ( (volatile unsigned short* const) (VT_MAILBOX + (a)))
#define AUTO_SHIM_MB(a) ( (volatile unsigned short* const) (SHIM_MAILBOX + (a)))
#define AUTO_CMD_ACK_REG(a) ( (volatile unsigned short* const) (CMD_ACK_REG + (a)))
#define AUTO_GEN_MBOX(a) ( (volatile unsigned char* const) (GEN_MBOX_OFFSET + (a)))
#define AUTO_GEN_MBOX_ACK(a) ( (volatile unsigned int* const) (GEN_MBOX_OFFSET_ACK + (a)))
#define AUTO_SPIN_MBOX(a) ( (volatile unsigned char* const) (SPIN_MBOX_OFFSET + (a)))
#define AUTO_SPIN_MBOX_ACK(a) ( (volatile unsigned int* const) (SPIN_MBOX_OFFSET_ACK + (a)))
#define AUTO_SHIM_MBOX(a) ( (volatile unsigned char* const) (SHIM_MBOX_OFFSET + (a)))
#define AUTO_SHIM_MBOX_ACK(a) ( (volatile unsigned int* const) (SHIM_MBOX_OFFSET_ACK + (a)))
#define AUTO_VT_MBOX(a) ( (volatile unsigned char* const) (VT_MBOX_OFFSET + (a)))
#define AUTO_VT_MBOX_ACK(a) ( (volatile unsigned int* const) (VT_MBOX_OFFSET_ACK + (a)))

#define AUTO_SPIN_APMBOX(a) ( (volatile unsigned char* const) (SPIN_AP_MBOX_OFFSET + (a)))
#define AUTO_SPIN_APMBOX_ACK(a) ( (volatile unsigned int* const) (SPIN_AP_MBOX_OFFSET_ACK + (a)))
#define AUTO_SHIM_APMBOX(a) ( (volatile unsigned char* const) (SHIM_AP_MBOX_OFFSET + (a)))
#define AUTO_SHIM_APMBOXACK(a) ( (volatile unsigned int* const) (SHIM_AP_MBOX_OFFSET_ACK + (a)))
#define AUTO_VT_APMBOX(a) ( (volatile unsigned char* const) (VT_AP_MBOX_OFFSET + (a)))
#define AUTO_VT_APMBOX_ACK(a) ( (volatile unsigned int* const) (VT_AP_MBOX_OFFSET_ACK + (a)))

#define AUTO_SHARED_SHIMS(a) ( (volatile shim_t* const) (SHARED_SHIMS_OFFSET + (a)))
#define AUTO_SHARED_VT(a) ( (volatile shim_t* const) (SHARED_VT_OFFSET + (a)))
#define AUTO_HEARTBEAT_DYN(a) ( (volatile unsigned long* const) (((u_long)(a) - 3L)))
#define AUTO_HEARTBEAT(a) ( (volatile unsigned long* const) (AUTO_HEARTBEAT_OFFSET + (a)))
#define AUTO_SHARED_GPA(a) ( (volatile shim_t* const) (SHARED_GPA_OFFSET + (a)))

/* some basic casting typed predefined */
#define VOL_CHAR_PTR	(volatile unsigned char* const)
#define VOL_SHORT_PTR	(volatile unsigned short* const)
#define VOL_INT_PTR	(volatile unsigned int* const)
#define VOL_LONG_PTR	(volatile unsigned long* const)

#ifdef __cplusplus
}
#endif

#endif
