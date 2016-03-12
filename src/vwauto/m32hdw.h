/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* m32hdw.h Copyright (c) 1994-1996 Varian Assoc.,Inc. All Rights Reserved */
/*
 */

#ifndef INCm32hdwh
#define INCm32hdwh

/* ------------- Make C header file C++ compliant ------------------- */
#ifdef __cplusplus
extern "C" {
#endif

/*
DESCRIPTION

Automation 332 System Memory Map 

*/

#define MBOX_BASE_VEC  24
#define GEN_MBOX_ITRP_OFFSET  6
#define SPIN_MBOX_ITRP_OFFSET  5
#define VT_MBOX_ITRP_OFFSET  4
#define SHIM_MBOX_ITRP_OFFSET  3

#define  MPU332_RAM_ADDR 0x00800000

#define  MPU332_GENMAIL	  0x00201000	/* General Mail Box  16 - Bit R_Only */
#define  MPU332_SPINMAIL  0x00201002	/* Spinner Mail Box  16 - Bit R_Only */
#define  MPU332_VTMAIL	  0x00201004	/* VT Mail Box  16 - Bit R_Only */
#define  MPU332_SHIMMAIL  0x00201006	/* Shim Mail Box  16 - Bit R_Only */
#define  MPU332_SOLID_DAC 0x00201008	/* Solids SPinner DAC Cntrl 16-bit W_Only */

#define  MPU332_VME_INTRP 0x0020100A	/* Post VME Interrupt, 8-bit (value=vector) W_Only */

#define  MPU332_CNTRL_STAT 0x0020100C	/* Auto Cntrl/Status Register R/W 8-bit */
/* ------ bit assignements ----- */
/* write to register meaning */
#define  SET_SPN_NOT_REG   0x01  /* Set Spin Not Regulated bit */
#define  SET_SPN_SPD_ZRO   0x02  /* Set Spin Speed Zero bit */
#define  SET_VT_ATTEN	   0x04  /* Set VT requires Attention bit */
#define  SET_QUART_RESET   0x08	 /* Reset Quart */
#define  SET_DISABLE_SYSF  0x10	 /* 0 = red, 1 = green */
#define  SET_EJECT_AIR_ON  0x20	 /* Turn Eject Air & Set eject air on bit */
#define  SET_BEAR_AIR_ON   0x40	 /* Turn Bearing Air & Set Bearing air on bit */
#define  SET_SDROP_AIR_ON  0x80	 /* Turn Slow Drop Air & Set Slow Drop on bit */

/* read from register meaning */
#define RD_SPIN_NOT_REG	0x01 /* 1 = Spin Not Regulated */
#define RD_SPIN_ZERO	0x02 /* 1 = Spin is Zero */
#define RD_VT_ATTEN	0x04 /* 1 = Vt Requires Attnetion */
#define RD_SAMP_IN_PROBE	0x08 /* 1 = Sample detected in probe */
#define RD_SAMP_NOT_LOCKED 0x10 /* 1 = Sample not Locked */
#define RD_EJECT_ON	0x20 /* 1 = Eject air on */
#define RD_BEAR_ON		0x40 /* 1 = Bearing air on */
#define RD_SDROP_ON	0x80 /* 1 = Slow Drop air on */


#define MPU332_LK_REG   0x0020100E

/**********************************************************8
* hardware macros
*/

#define M32_MAILBOX(a)	( (volatile unsigned short* const) (a))
#define M32_SHARED_MEM(a) ( (volatile unsigned short* const) (MPU332_RAM_SHR + (a)))
#define M32_VME_INTRP ( (volatile unsigned char* const) (MPU332_VME_INTRP))
#define M32_CNTRL ( (volatile unsigned char* const) (MPU332_CNTRL_STAT))
#define M32_LOCK_VALUE ( (volatile unsigned short* const) (MPU332_LK_REG))

#ifdef __cplusplus
}
#endif

#endif
