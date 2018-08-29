/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef INChardwareh
#define INChardwareh

/* ------------- Make C header file C++ compliant ------------------- */
#ifdef __cplusplus
extern "C" {
#endif

/*
DISCRIPTION

 MV147S-1 VMEbus short I/O Address: FFFF0000 - FFFFFFFF

*/

/* Hardware Base Address */
/* Short I/O space */
#define FIFO_BASE_ADR 0xffff0800L
#define STM_BASE_ADR  0xffff0800L
#define ADC_BASE_ADR  0xffff0900L

/* VME Access type */
#define FIFO_VME_ACCESS_TYPE 	VME_AM_SUP_SHORT_IO
#define STM_VME_ACCESS_TYPE 	VME_AM_SUP_SHORT_IO
#define ADC_VME_ACCESS_TYPE 	VME_AM_SUP_SHORT_IO


/* Long address/data space */
#define PFIFO_BASE_ADR 		0x90000000L
#define STM_ADR_BASE_ADR  	0x90000000L

/* VME Access type */
/*
/* #define FIFO_VME_ACCESS_TYPE VME_AM_EXT_SUP_DATA */
/* #define STM_VME_ACCESS_TYPE 	VME_AM_EXT_SUP_DATA */
#define FIFO_VME_ACCESS_TYPE 	VME_AM_SUP_SHORT_IO
#define STM_VME_ACCESS_TYPE 	VME_AM_SUP_SHORT_IO
#define ADC_VME_ACCESS_TYPE 	VME_AM_SUP_SHORT_IO

/* STM Memory Map */
#define STM_MEM_BASE_ADR 0xA0000000
#define STM_MEM_2MB_END  0xA01FFFFF
#define STM_MEM_4MB_END  0xA03FFFFF
#define STM_MEM_16MB_END 0xA0FFFFFF

#define STM_MEM_VME_ACCESS_TYPE VME_AM_EXT_SUP_DATA
           

/* Control Register Offsets */
#define FF_CR 		0x11  /* 8-bits */
#define STM_CR 		0x11  /* 8-bits */
#define FFSTM_CR 	0x11  /* 8-bits */
#define ITRNABLE_CR 	0x13  /* 8-bits */
#define ITRMASK_CR 	0x17  /* 8-bits */
#define DRAMCNTL_CR 	0x15  /* 8-bits */
#define ADC_CR 		0x01  /* 8-bits */

/* Status Register Offsets */

#define FF_SR	0x1A		/* 16-bit */
#define STM_SR	0x1A		/* 16-bit */
#define APB_SR	0x1C		/* 16-bit */
#define ADC_SR  0x01		/* 8-bit */

/* CT -> 23:0 == ( STM_CT_BYTE3 STM_CT_BYTE2  STM_CT_BYTE1 ) */
#define STM_CT_BYTE1	0x0
#define STM_CT_BYTE2	0x2
#define STM_CT_BYTE3	0x4

/* NP -> 23:0 == ( STM_NP_BYTE3 STM_NP_BYTE2  STM_NP_BYTE1 ) */
#define STM_NP_BYTE1	0x6
#define STM_NP_BYTE2	0x8
#define STM_NP_BYTE3	0xA

/* SS -> 15:0 == ( STM_SS_BYTE2  STM_SS_BYTE1 ) */
#define STM_SS_BYTE1	0xC
#define STM_SS_BYTE2	0xE

/* ADDR -> 23:0  */
#define STM_ADDR_LONG	0x18


/* Write PreFifo 32 bit Data Register */
/* #define PF_WRITE 0x20 */
#define PF_WRITE 0x00

/* ------- FIFO/STM Controler Register Bit Position -------*/
/* FFSTM_CR 	0x0810   8-bits */
#define OPCLR_BIT 	0x0
#define FFCLR_BIT 	0x1
#define APHSCLR_BIT	0x2
#define STMCLR_BIT	0x3
#define FFSTART_BIT 	0x4
#define FFSYNCST_BIT 	0x5
#define SYNCSELECT_BIT	0x6

#define OPCLR 		0x1
#define FFCLR 		0x2
#define APHSCLR		0x4
#define STMCLR		0x8
#define FFSTART 	0x10
#define FFSYNCST 	0x20
#define SYNCSELECT	0x40

#define SYNC_ON_LOCK	0
#define SYNC_ON_EXTEN	1

/*---  VME Interrupt 8-bit Enable & Mask Register Bit Positions ---*/
/* ITRNABLE_CR 	0x0812   8-bits */
/* ITRMASK_CR 		0x0816   8-bits */
#define FF_ALLITRPS	0xFF
#define PFF_AMFULL 	0x80
#define PFF_AMEMPTY 	0x40
#define FF_STOPPED 	0x20
#define FF_START_ERR 	0x10
#define FF_PRGITRP 	0x8
#define STM_DATA_ERR	0x4
#define STM_MAX_TRANS	0x2
#define STM_DATA_RDY	0x1
	     
/*---  FIFO/STM Status Register Bit Positions ---*/
/* FF_RSR	0x081A	16-bit */
#define HALTSTOP_BIT 	0x0
#define FFMT_BIT	0x1
#define INITHALT_BIT 	0x2
#define RUN_MT_BIT 	0x3
#define PFF_AE_BIT 	0x4
#define PFF_AF_BIT 	0x5
#define LOOPING_BIT 	0x6
#define RUNNING_BIT 	0x7
#define APTO_BIT 	0x8
#define SUBSYSALARM_BIT	0x9
#define LKSENS_BIT	0xA
#define MISSME_LA_BIT	0xB
#define BD_REV0_BIT	0xC
#define BD_REV1_BIT	0xD
#define BD_REV2_BIT	0xE
#define BD_REV3_BIT	0xF

#define HALTSTOP 	0x1
#define FFMT 		0x2
#define INITHALT 	0x4
#define RUN_MT 		0x8
#define PFF_AE 		0x10
#define PFF_AF 		0x20
#define LOOPING 	0x40
#define STOPPED 	0x80
#define APTO 		0x100
#define SUBSYSALARM	0x200
#define LKSENS		0x400
#define MISSME_LA	0x800
#define BD_REV0		0x1000
#define BD_REV1		0x2000
#define BD_REV2		0x4000
#define BD_REV3		0x8000

/*---  APbus Status Register Bit Positions ---*/
/* APB_RSR	0x081C   16-bit */
#define APD0_7	0x0
#define APA0_3	0x8
#define APSI	0xC
#define APMN	0xD
#define APRW	0xE
#define FF_APOP	0xF

/* ------- Write PreFifo Data Register Bit Positions ------*/
/* PF_WRITE 0x0820    32-bit */
#define HALT_OPCODE	0x0000e000L

/*
*  COUNT - bits 0 - 13, 0x00003FFF
*/
#define MAX_FIFO_COUNT  0x00003FFFL /* 14 bits of count */

/*********************************************************
* TIME BASE - bits 14 - 15
*/
#define NSEC_TIME_BASE		0x00000000L /* bit 15 & 14 = 0 */
#define MSEC_TIME_BASE		0x00004000L /* bit 15 & 14 = 0,1 */
#define APBUS_TIME_BASE		0x00008000L /* bit 15 & 14 = 1,0 */
#define APBUS_TIME_BASE2	0x0000C000L /* bit 15 & 14 = 1,1 */

/**********************************************************
* HSLINES 0x00FF0000  - bits 16 - 23
*/
#define HIGHBAND_TX_GATE 	0x00010000L /* bit 16 */
#define HIGHBAND_90_PHAS 	0x00020000L /* bit 17 */
#define HIGHBAND_180_PHAS 	0x00040000L /* bit 18 */
#define HIGHBAND_270_PHAS 	0x00060000L /* bit 17&18 */
#define LOWBAND_TX_GATE 	0x00080000L /* bit 19 */
#define LOWBAND_90_PHAS 	0x00100000L /* bit 20 */
#define LOWBAND_180_PHAS 	0x00200000L /* bit 21 */
#define LOWBAND_270_PHAS 	0x00300000L /* bit 20&21 */
#define RECEIVER_GATE	 	0x00400000L /* bit 22 */
#define SW_PRG_INTRP	 	0x00800000L /* bit 22 */

/*********************************************************
* FIFO CONTROL BITS: CTC-24, START_LP-25, END_LP-26, LPCOUNT-25&26
*/
#define FIFO_CTC_BIT	0x01000000L /* bit 24 */
#define FIFO_START_LOOP 0x02000000L /* bit 25 */
#define FIFO_END_LOOP 	0x04000000L /* bit 26 */
#define FIFO_LOOP_CNT 	0x06000000L /* bits 25 & 26 */

/**********************************************************
*  UNUSED: bits 27 - 31
*/

/*********************************************************
*   ADC Control Register BITS
*/
#define ADC_OBS_LOCK	0x0
#define ADC_EN_ITRP	0x1
#define ADC_RSET_OVRFLW 0x2
#define ADC_UNUSED	0x3
#define ADC_SHIFT_0	0x4
#define ADC_SHIFT_1	0x5
#define ADC_SHIFT_2	0x6
#define ADC_SHIFT_3	0x7

/*********************************************************
*   ADC Status Register BITS
*/
#define ADC_OVRFLOW	0x0
#define ADC_UNUSED1	0x1
#define ADC_UNUSED2	0x2
#define ADC_UNUSED3	0x3
#define ADC_BD_REV0	0x4
#define ADC_BD_REV1	0x5
#define ADC_BD_REV2	0x6
#define ADC_BD_REV3	0x7



/**********************************************************8
* hardware macros
*/
#define FF_REG(a,b)	( (volatile unsigned char* const) ((a) + (b)) )
#define FF_REGW(a,b)	( (volatile unsigned short* const) ((a) + (b)) )
#define FF_REGL(a,b)	( (volatile unsigned long* const) ((a) + (b)) )
#define STM_REG(a,b)	( (volatile unsigned char* const) ((a) + (b)) )
#define STM_REGL(a,b)	( (volatile unsigned long* const) ((a) + (b)) )
#define ADC_REG(a,b)	( (volatile unsigned char* const) ((a) + (b)) )
#define BIT_MASK(a)	( (1 << (a)) )

#define enableDRAM(A)	*((char *)DRAMCNTL_WCR + FIFO_BASE_ADDR) = A
#define set_ffCR(A)	*((char *)FFSTM_WCR + FIFO_BASE_ADDR) = A
#define set_itrpCR(A)	*((char *)ITRNABLE_WCR + FIFO_BASE_ADDR) = A
#define set_itrpMR(A)	*((char *)ITRMASK_WCR + FIFO_BASE_ADDR) = A

#define get_fifoSR(A)	*((int *)FF_RSR + FIFO_BASE_ADDR)
#define get_apSR(A)	*((int *)APB_RSR + FIFO_BASE_ADDR)

#ifdef __cplusplus
}
#endif

#endif
