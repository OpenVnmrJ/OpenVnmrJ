/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef INCvmeIntrph
#define INCvmeIntrph

/* ------------- Make C header file C++ compliant ------------------- */
#ifdef __cplusplus
extern "C" {
#endif

/*
DISCRIPTION

 VME Interrupt Vectors for Console Tasks

*/

/* --------- vxWorks 5.1.1  intterupt vector numbers ------------ */
/* 
   vxWorks 5.1.1 VME Interrupt Vector Usage:

   0x00 - 0x3F : Reserved 68K MMU 
		 Vector: 32 - 47 TRAP 0-15 instructions.

   0x40 - 0x4B : ACFAIL, BERR, ABORT, SERIAL, LANCE,
		 SCSI, SCSI_DMA, PRINTER, SYSCLOCK, AUXCLOCK
   0x51		 VME Mail Box 

   0x80 - 0x8E : 2 serial ports
   0x90 - 0x9E : 2 serial ports

   Console:
	0xB0 - 0xB7 : STM 2 vector per Board (total 4)
	0xB8 - 0xBF : ADC 1 vector per Board (total 8)
	0xC0 - 0xFF : Output Board 16 vectors (total 4)
*/

/* ----- Varian Console Interrupt Vector Block 0x60 - x6F -------- */

#define MIN_VME_ITRP_VEC 0xB0
#define MAX_VME_ITRP_VEC 0xFF
#define MIN_VME_ITRP_LEVEL 1
#define MAX_VME_ITRP_LEVEL 5

/* --------- FIFO interupt vector numbers ------------ */
#define FIFO_VME_INTRP_LEVEL 4

/* Base Vector for the 4 possible output board in system */
#define BASE_INTRP_VEC			0xC0
#define BASE2_INTRP_VEC			0xD0
#define BASE3_INTRP_VEC			0xE0
#define BASE4_INTRP_VEC			0xF0

/* Vector Offset All Boards */
#define FIFO_ALMOST_EMPTY_ITRP_VEC 	0x00
#define FIFO_ALMOST_FULL_ITRP_VEC 	0x01
#define FIFO_STOP_ITRP_VEC 		0x02

#define FIFO_ERROR_ITRP_VEC 		0x03   /* Generic Form */
#define FIFO_MT_START_ERR_ITRP_VEC 	0x03
#define FIFO_HT_START_ERR_ITRP_VEC 	0x03
#define FIFO_NETBL_ERR_ITRP_VEC 	0x03
#define FIFO_FORP_ERR_ITRP_VEC 		0x03

#define FIFO_APBUS_ITRP_VEC		0x04   /* Generic Form */
#define FIFO_AP_NOT_MT_ITRP_VEC		0x04
#define FIFO_AP_AM_FULL_ITRP_VEC	0x04
#define FIFO_NETBAP_ERR_ITRP_VEC 	0x04

#define FIFO_TAG_ITRP_VEC		0x05   /* Generic Form */
#define FIFO_TAG_NOT_MT_ITRP_VEC	0x05
#define FIFO_TAG_AM_FULL_ITRP_VEC	0x05

#define FIFO_PROG1_ITRP_VEC		0x06
#define FIFO_PROG2_ITRP_VEC		0x07
#define FIFO_PROG3_ITRP_VEC		0x08
#define FIFO_PROG4_ITRP_VEC		0x09


/* --------- STM interupt vector numbers ------------ */
/* Base Vector for the 4 possible stm board in system */
#define STM_VME_INTRP_LEVEL 3

#define STM_BASE_INTRP_VEC			0xB0
#define STM_BASE2_INTRP_VEC			0xB2
#define STM_BASE3_INTRP_VEC			0xB4
#define STM_BASE4_INTRP_VEC			0xB6

#define STM_ERROR_ITRP_VEC  		0x0
#define STM_MAX_TRANS_ITRP_VEC  	0x0
#define STM_DATA_ERROR_ITRP_VEC 	0x0

#define STM_DATA_ITRP_VEC 		0x1
#define STM_DATA_READY_ITRP_VEC 	0x1
#define STM_USER_FLAG_ITRP_VEC 		0x1

/* --------- ADC interupt vector numbers ------------ */
/* Each Addition ADC board, add 1 to vector number */
/* Covers ADC OverFlow, Receiver 1 or 2 OverLoad */

#define ADC_ITRP_VEC  			0xB8

/* --------- Automation Board interupt vector numbers ------------ */

#define AUTO_ITRP_VEC  			0x60
#define GEN_ACK_VEC   			0x00
#define SPIN_ACK_VEC   			0x01
#define VT_ACK_VEC   			0x02
#define SHIM_ACK_VEC   			0x03
#define AUTO_EXCPT_VEC   		0x04

/* --------- Safety Interface Board interupt vector ------------ */

#define SIB_ITRP_LEVEL 2
#define SIB_ITRP_VEC  			0x73

#ifdef __cplusplus
}
#endif

#endif
