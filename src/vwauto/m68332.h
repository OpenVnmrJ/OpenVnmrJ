/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* m68332.h Copyright (c) 1994-1996 Varian Assoc.,Inc. All Rights Reserved */
/* m68332.h 1.7 4/11/95 Copyright (c) 1994-1996 Varian Assoc.,Inc. All Rights Reserved */
/* m68332.h - Motorola MC68332 CPU control registers */

/* Copyright 1984-1996 Wind River Systems, Inc. */
/*
modification history
--------------------
..., 2Feb95,gad  added TPU defines for mask set G & replaced UINT*.
01l,24oct92,jcf  fixed TPU defines.
01k,22sep92,rrr  added support for c++
01j,02jul92,caf  added TY_CO_DEV and function declarations for 5.1 upgrade.
		 for 5.0.x compatibility, define INCLUDE_TY_CO_DRV_50 when
		 including this header.
01e,26may92,rrr  the tree shuffle
01d,22jan92,caf  fixed M332_QSM_CMD_BASE, QSM_QILR_SPI_MASK and SIM_CSOR_IPL.
01c,04oct91,rrr  passed through the ansification filter
		  -fixed #else and #endif
		  -changed ASMLANGUAGE to _ASMLANGUAGE
		  -changed copyright notice
01b,30sep91,caf  added _ASMLANGUAGE conditional.
01a,13feb91,jcf	 based on Tektronix original.
*/

/*
This file contains I/O address and related constants for the MC68332.
*/

#ifndef __INCm68332h
#define __INCm68332h

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _ASMLANGUAGE

#include <tyLib.h>

#ifndef  INCLUDE_TY_CO_DRV_50

typedef struct		/* TY_CO_DEV */
    {
    TY_DEV	tyDev;
    BOOL	created;	/* true if this device has been created */
    int		baudFreq;	/* system clock frequency */
    } TY_CO_DEV;

#endif	/* INCLUDE_TY_CO_DRV_50 */

/* MC68332 parameter register addresses */

#define M332_SIM_MCR		((unsigned short *) 0xfffa00)
#define M332_SIM_SIMTR		((unsigned short *) 0xfffa02)
#define M332_SIM_SYNCR		((unsigned short *) 0xfffa04)
#define M332_SIM_RSR		((unsigned char  *) 0xfffa07)
#define M332_SIM_SIMTRE		((unsigned short *) 0xfffa08)
#define M332_SIM_PORTE		((unsigned char  *) 0xfffa11)
#define M332_SIM_DDRE		((unsigned char  *) 0xfffa15)
#define M332_SIM_PEPAR		((unsigned char  *) 0xfffa17)
#define M332_SIM_PORTF		((unsigned char  *) 0xfffa19)
#define M332_SIM_DDRF		((unsigned char  *) 0xfffa1d)
#define M332_SIM_PFPAR		((unsigned char  *) 0xfffa1f)
#define M332_SIM_SYPCR		((unsigned char  *) 0xfffa21)
#define M332_SIM_PICR		((unsigned short *) 0xfffa22)
#define M332_SIM_PITR		((unsigned short *) 0xfffa24)
#define M332_SIM_SWSR		((unsigned char  *) 0xfffa27)
#define M332_SIM_TSTMSRA	((unsigned short *) 0xfffa30)
#define M332_SIM_TSTMSRB	((unsigned short *) 0xfffa32)
#define M332_SIM_TSTSC		((unsigned short *) 0xfffa34)
#define M332_SIM_TSTRC		((unsigned short *) 0xfffa36)
#define M332_SIM_CREG		((unsigned short *) 0xfffa38)
#define M332_SIM_DREG		((unsigned short *) 0xfffa3a)
#define M332_SIM_PORTC		((unsigned char  *) 0xfffa41)
#define M332_SIM_CSPAR0		((unsigned short *) 0xfffa44)
#define M332_SIM_CSPAR1		((unsigned short *) 0xfffa46)
#define M332_SIM_CSBOOT		((SIM_CS *) 0xfffa48)
#define M332_SIM_CS0		((SIM_CS *) 0xfffa4c)
#define M332_SIM_CS1		((SIM_CS *) 0xfffa50)
#define M332_SIM_CS2		((SIM_CS *) 0xfffa54)
#define M332_SIM_CS3		((SIM_CS *) 0xfffa58)
#define M332_SIM_CS4		((SIM_CS *) 0xfffa5c)
#define M332_SIM_CS5		((SIM_CS *) 0xfffa60)
#define M332_SIM_CS6		((SIM_CS *) 0xfffa64)
#define M332_SIM_CS7		((SIM_CS *) 0xfffa68)
#define M332_SIM_CS8		((SIM_CS *) 0xfffa6c)
#define M332_SIM_CS9		((SIM_CS *) 0xfffa70)
#define M332_SIM_CS10		((SIM_CS *) 0xfffa74)
#define M332_RAM_RAMMCR		((unsigned short *) 0xfffb00)
#define M332_RAM_RAMTST		((unsigned short *) 0xfffb02)
#define M332_RAM_RAMBAR		((unsigned short *) 0xfffb02)
#define M332_QSM_QMCR		((unsigned short *) 0xfffc00)
#define M332_QSM_QTEST		((unsigned short *) 0xfffc02)
#define M332_QSM_QILR		((unsigned char  *) 0xfffc04)
#define M332_QSM_QIVR		((unsigned char  *) 0xfffc05)
#define M332_QSM_SCCR0		((unsigned short *) 0xfffc08)
#define M332_QSM_SCCR1		((unsigned short *) 0xfffc0a)
#define M332_QSM_SCSR		((unsigned short *) 0xfffc0c)
#define M332_QSM_SCDR		((unsigned short *) 0xfffc0e)
#define M332_QSM_QPDR		((unsigned char  *) 0xfffc15)
#define M332_QSM_QPAR		((unsigned char  *) 0xfffc16)
#define M332_QSM_QDDR		((unsigned char  *) 0xfffc17)
#define M332_QSM_QPA_DDR	((unsigned short *) 0xfffc16)
#define M332_QSM_SPCR0		((unsigned short *) 0xfffc18)
#define M332_QSM_SPCR1		((unsigned short *) 0xfffc1a)
#define M332_QSM_SPCR2		((unsigned short *) 0xfffc1c)
#define M332_QSM_SPCR3		((unsigned char  *) 0xfffc1e)
#define M332_QSM_SPSR		((unsigned char  *) 0xfffc1f)
#define M332_QSM_RX_BASE	((unsigned short *) 0xfffd00)
#define M332_QSM_RX_SIZE	16
#define M332_QSM_TX_BASE	((unsigned short *) 0xfffd20)
#define M332_QSM_TX_SIZE	16
#define M332_QSM_CMD_BASE	((unsigned char  *) 0xfffd40)
#define M332_QSM_CMD_SIZE	16
#define M332_TPU_TMCR		((unsigned short *) 0xfffe00)
#define M332_TPU_TTCR		((unsigned short *) 0xfffe02)
#define M332_TPU_DSCR		((unsigned short *) 0xfffe04)
#define M332_TPU_DSSR		((unsigned short *) 0xfffe06)
#define M332_TPU_TICR		((unsigned short *) 0xfffe08)
#define M332_TPU_CIER		((unsigned short *) 0xfffe0a)
#define M332_TPU_CFSR0		((unsigned short *) 0xfffe0c)
#define M332_TPU_CFSR1		((unsigned short *) 0xfffe0e)
#define M332_TPU_CFSR2		((unsigned short *) 0xfffe10)
#define M332_TPU_CFSR3		((unsigned short *) 0xfffe12)
#define M332_TPU_HSQR0		((unsigned short *) 0xfffe14)
#define M332_TPU_HSQR1		((unsigned short *) 0xfffe16)
#define M332_TPU_HSRR0		((unsigned short *) 0xfffe18)
#define M332_TPU_HSRR1		((unsigned short *) 0xfffe1a)
#define M332_TPU_CPR0		((unsigned short *) 0xfffe1c)
#define M332_TPU_CPR1		((unsigned short *) 0xfffe1e)
#define M332_TPU_CISR		((unsigned short *) 0xfffe20)
#define M332_TPU_CHN		((TPU_CHN *) 0xffff00)
#define M332_TPU_CHN0		((TPU_CHN *) 0xffff00)
#define M332_TPU_CHN1		((TPU_CHN *) 0xffff10)
#define M332_TPU_CHN2		((TPU_CHN *) 0xffff20)
#define M332_TPU_CHN3		((TPU_CHN *) 0xffff30)
#define M332_TPU_CHN4		((TPU_CHN *) 0xffff40)
#define M332_TPU_CHN5		((TPU_CHN *) 0xffff50)
#define M332_TPU_CHN6		((TPU_CHN *) 0xffff60)
#define M332_TPU_CHN7		((TPU_CHN *) 0xffff70)
#define M332_TPU_CHN8		((TPU_CHN *) 0xffff80)
#define M332_TPU_CHN9		((TPU_CHN *) 0xffff90)
#define M332_TPU_CHNa		((TPU_CHN *) 0xffffa0)
#define M332_TPU_CHN10	((TPU_CHN *) 0xffffa0)
#define M332_TPU_CHNb		((TPU_CHN *) 0xffffb0)
#define M332_TPU_CHN11	((TPU_CHN *) 0xffffb0)
#define M332_TPU_CHNc		((TPU_CHN *) 0xffffc0)
#define M332_TPU_CHN12	((TPU_CHN *) 0xffffc0)
#define M332_TPU_CHNd		((TPU_CHN *) 0xffffd0)
#define M332_TPU_CHN13	((TPU_CHN *) 0xffffd0)
#define M332_TPU_CHNe		((TPU_CHN *) 0xffffe0)
#define M332_TPU_CHN14	((TPU_CHN *) 0xffffe0)
#define M332_TPU_CHNf		((TPU_CHN *) 0xfffff0)
#define M332_TPU_CHN15	((TPU_CHN *) 0xfffff0)

#define	TPU_CIER_CISR_CH0  0x0001
#define	TPU_CIER_CISR_CH1  0x0002
#define	TPU_CIER_CISR_CH2  0x0004
#define	TPU_CIER_CISR_CH3  0x0008
#define	TPU_CIER_CISR_CH4  0x0010
#define	TPU_CIER_CISR_CH5  0x0020
#define	TPU_CIER_CISR_CH6  0x0040
#define	TPU_CIER_CISR_CH7  0x0080
#define	TPU_CIER_CISR_CH8  0x0100
#define	TPU_CIER_CISR_CH9  0x0200
#define	TPU_CIER_CISR_CH10 0x0400
#define	TPU_CIER_CISR_CH11 0x0800
#define	TPU_CIER_CISR_CH12 0x1000
#define	TPU_CIER_CISR_CH13 0x2000
#define	TPU_CIER_CISR_CH14 0x4000
#define	TPU_CIER_CISR_CH15 0x8000

/* SIM - Register definitions for the System Integration Module */

typedef struct		/* SIM_CS */
    {
    unsigned short csBar;		/* chip select base address register */
    unsigned short csOr;		/* chip select options register */
    } SIM_CS;

/* SIM_MCR - Module Configuration Register (Write Once Only) */

#define SIM_MCR_MM		0x0040	/* module mapping */
#define SIM_MCR_SUPV		0x0080	/* supervisor/unrestricted data space */
#define SIM_MCR_X		0x0000	/* no show cycle, ext. arbitration */
#define SIM_MCR_SH		0x0100	/* show cycle enabled, no ext. arb. */
#define SIM_MCR_SH_X		0x0200	/* show cycle enabled, ext. arb. */
#define SIM_MCR_SH_X_BG		0x0300	/* show cycle/ext arb; int halt w/ BG */
#define SIM_MCR_SLVEN		0x0800	/* slave mode enable */
#define SIM_MCR_FRZBM		0x2000	/* freeze bus monitor enable */
#define SIM_MCR_FRZSW		0x4000	/* freeze software enable */
#define SIM_MCR_EXOFF		0x8000	/* external clock off */

/* SIM_SIMTR - Module Test Register */

#define SIM_SIMTR_REV_MASK	0xfc00	/* revision number for this part */

/* SIM_SYNCR - Clock Synthesis Control Register */

#define SIM_SYNCR_STEXT		0x0001	/* stop mode external clock */
#define SIM_SYNCR_STSIM		0x0002	/* stop mode system integration clock */
#define SIM_SYNCR_RSTEN		0x0004	/* reset enable */
#define SIM_SYNCR_SLOCK		0x0008	/* synthesizer lock */
#define SIM_SYNCR_SLIMP		0x0010	/* limp mode */
#define SIM_SYNCR_EDIV 		0x0080	/* E clock divide rate */
#define SIM_SYNCR_Y_MASK 	0x3f00	/* Y - frequency control bits */
#define SIM_SYNCR_X    		0x4000	/* X - frequency control bit */
#define SIM_SYNCR_W    		0x8000	/* W - frequency control bit */

/* SIM_RSR - Reset Status Register */

#define SIM_RSR_TST		0x01	/* test submodule reset	*/
#define SIM_RSR_SYS		0x02	/* system reset (CPU reset) */
#define SIM_RSR_LOC		0x04	/* loss of clock reset */
#define SIM_RSR_HLT		0x10	/* halt monitor reset */
#define SIM_RSR_SW		0x20	/* software watchdog reset */
#define SIM_RSR_POW		0x40	/* power up reset */
#define SIM_RSR_EXT		0x80	/* external reset */

/* SIM_PEPAR - Port E Pin Assignment Register */

#define SIM_PEPAR_DSACK0	0x01	/* select bus control pin assignment */
#define SIM_PEPAR_DSACK1	0x02	/* select bus control pin assignment */
#define SIM_PEPAR_AVEC		0x04	/* select bus control pin assignment */
#define SIM_PEPAR_RMC		0x08	/* select bus control pin assignment */
#define SIM_PEPAR_DS		0x10	/* select bus control pin assignment */
#define SIM_PEPAR_AS		0x20	/* select bus control pin assignment */
#define SIM_PEPAR_SIZ0		0x40	/* select bus control pin assignment */
#define SIM_PEPAR_SIZ1		0x80	/* select bus control pin assignment */

/* SIM_PFPAR - Port F Pin Assignment Register */

#define SIM_PFPAR_MODCK		0x01	/* select bus control pin assignment */
#define SIM_PFPAR_IRQ1		0x02	/* select bus control pin assignment */
#define SIM_PFPAR_IRQ2		0x04	/* select bus control pin assignment */
#define SIM_PFPAR_IRQ3		0x08	/* select bus control pin assignment */
#define SIM_PFPAR_IRQ4		0x10	/* select bus control pin assignment */
#define SIM_PFPAR_IRQ5		0x20	/* select bus control pin assignment */
#define SIM_PFPAR_IRQ6		0x40	/* select bus control pin assignment */
#define SIM_PFPAR_IRQ7		0x80	/* select bus control pin assignment */

/* SIM_SYPCR - System Protection Control (Write Once Only) */

#define SIM_SYPCR_BMT_64	0x00	/* 64 clk cycle bus monitor timeout */
#define SIM_SYPCR_BMT_32	0x01	/* 32 clk cycle bus monitor timeout */
#define SIM_SYPCR_BMT_16	0x02	/* 16 clk cycle bus monitor timeout */
#define SIM_SYPCR_BMT_8		0x03	/* 8 clk cycle bus monitor timeout */
#define SIM_SYPCR_BME		0x04	/* bus monitor enable */
#define SIM_SYPCR_HME		0x08	/* halt monitor enable */
#define SIM_SYPCR_SWT_9		0x00	/* 2^9 extal cycle software timeout */
#define SIM_SYPCR_SWT_11	0x10	/* 2^11 extal cycle software timeout */
#define SIM_SYPCR_SWT_13	0x20	/* 2^13 extal cycle software timeout */
#define SIM_SYPCR_SWT_15	0x30	/* 2^15 extal cycle software timeout */
#define SIM_SYPCR_SWT_18	0x40	/* 2^18 extal cycle software timeout */
#define SIM_SYPCR_SWT_20	0x50	/* 2^20 extal cycle software timeout */
#define SIM_SYPCR_SWT_22	0x60	/* 2^22 extal cycle software timeout */
#define SIM_SYPCR_SWT_24	0x70	/* 2^24 extal cycle software timeout */
#define SIM_SYPCR_SWE		0x80	/* software watchdog enable */

/* SIM_PICR - Periodic Interrupt Control Register */

#define SIM_PICR_PIV_MASK	0x00ff	/* periodic interrupt vector */
#define SIM_PICR_PIRQL_MASK	0x0700	/* periodic interrupt request level */

/* SIM_PITR - Periodic Interrupt Timer */

#define SIM_PITR_PITR_MASK	0x00ff	/* periodic interrupt timing register */
#define SIM_PITR_PTP		0x0100	/* periodic timer prescale bit */

/* SIM_SWSR - Software Service Register */

#define SIM_SWSR_ACK1		0x55	/* software watchdog ack. part 1 */
#define SIM_SWSR_ACK2		0xaa	/* software watchdog ack. part 2 */

/* SIM_CSPAR0 - Chip Select Pin Assignement Register 0 */

#define SIM_CSPAR0_CSBOOT_DIS	0x0000	/* discrete output */
#define SIM_CSPAR0_CSBOOT_DEF	0x0001	/* default pin function */
#define SIM_CSPAR0_CSBOOT_8BIT	0x0002	/* chip select with 8 bit port */
#define SIM_CSPAR0_CSBOOT_16BIT	0x0003	/* chip select with 16 bit port */
#define SIM_CSPAR0_CS0_DIS	0x0000	/* discrete output */
#define SIM_CSPAR0_CS0_BR	0x0004	/* default pin function */
#define SIM_CSPAR0_CS0_8BIT	0x0008	/* chip select with 8 bit port */
#define SIM_CSPAR0_CS0_16BIT	0x000c	/* chip select with 16 bit port */
#define SIM_CSPAR0_CS1_DIS	0x0000	/* discrete output */
#define SIM_CSPAR0_CS1_BG	0x0010	/* default pin function */
#define SIM_CSPAR0_CS1_8BIT	0x0020	/* chip select with 8 bit port */
#define SIM_CSPAR0_CS1_16BIT	0x0030	/* chip select with 16 bit port */
#define SIM_CSPAR0_CS2_DIS	0x0000	/* discrete output */
#define SIM_CSPAR0_CS2_BGACK	0x0040	/* default pin function */
#define SIM_CSPAR0_CS2_8BIT	0x0080	/* chip select with 8 bit port */
#define SIM_CSPAR0_CS2_16BIT	0x00c0	/* chip select with 16 bit port */
#define SIM_CSPAR0_CS3_DIS	0x0000	/* discrete output */
#define SIM_CSPAR0_CS3_FC0	0x0100	/* default pin function */
#define SIM_CSPAR0_CS3_8BIT	0x0200	/* chip select with 8 bit port */
#define SIM_CSPAR0_CS3_16BIT	0x0300	/* chip select with 16 bit port */
#define SIM_CSPAR0_CS4_DIS	0x0000	/* discrete output */
#define SIM_CSPAR0_CS4_FC1	0x0400	/* default pin function */
#define SIM_CSPAR0_CS4_8BIT	0x0800	/* chip select with 8 bit port */
#define SIM_CSPAR0_CS4_16BIT	0x0c00	/* chip select with 16 bit port */
#define SIM_CSPAR0_CS5_DIS	0x0000	/* discrete output */
#define SIM_CSPAR0_CS5_FC2	0x1000	/* default pin function */
#define SIM_CSPAR0_CS5_8BIT	0x2000	/* chip select with 8 bit port */
#define SIM_CSPAR0_CS5_16BIT	0x3000	/* chip select with 16 bit port */

/* SIM_CSPAR1 - Chip Select Pin Assignement Register 1 */

#define SIM_CSPAR1_CS6_DIS	0x0000	/* discrete output */
#define SIM_CSPAR1_CS6_A19	0x0001	/* default pin function */
#define SIM_CSPAR1_CS6_8BIT	0x0002	/* chip select with 8 bit port */
#define SIM_CSPAR1_CS6_16BIT	0x0003	/* chip select with 16 bit port */
#define SIM_CSPAR1_CS7_DIS	0x0000	/* discrete output */
#define SIM_CSPAR1_CS7_A20	0x0004	/* default pin function */
#define SIM_CSPAR1_CS7_8BIT	0x0008	/* chip select with 8 bit port */
#define SIM_CSPAR1_CS7_16BIT	0x000c	/* chip select with 16 bit port */
#define SIM_CSPAR1_CS8_DIS	0x0000	/* discrete output */
#define SIM_CSPAR1_CS8_A21	0x0010	/* default pin function */
#define SIM_CSPAR1_CS8_8BIT	0x0020	/* chip select with 8 bit port */
#define SIM_CSPAR1_CS8_16BIT	0x0030	/* chip select with 16 bit port */
#define SIM_CSPAR1_CS9_DIS	0x0000	/* discrete output */
#define SIM_CSPAR1_CS9_A22	0x0040	/* default pin function */
#define SIM_CSPAR1_CS9_8BIT	0x0080	/* chip select with 8 bit port */
#define SIM_CSPAR1_CS9_16BIT	0x00c0	/* chip select with 16 bit port */
#define SIM_CSPAR1_CS10_E_CLK	0x0000	/* E clock output */
#define SIM_CSPAR1_CS10_A23	0x0100	/* default pin function */
#define SIM_CSPAR1_CS10_8BIT	0x0200	/* chip select with 8 bit port */
#define SIM_CSPAR1_CS10_16BIT	0x0300	/* chip select with 16 bit port */

/* SIM_CSBAR - Chip Select Base Address Register */

#define SIM_CSBAR_2K		0x0000	/* 2k block size */
#define SIM_CSBAR_8K		0x0001	/* 8k block size */
#define SIM_CSBAR_16K		0x0002	/* 16k block size */
#define SIM_CSBAR_64K		0x0003	/* 64k block size */
#define SIM_CSBAR_128K		0x0004	/* 128k block size */
#define SIM_CSBAR_256K		0x0005	/* 256k block size */
#define SIM_CSBAR_512K		0x0006	/* 512k block size */
#define SIM_CSBAR_1024K		0x0007	/* 1024k block size */

/* SIM_CSOR - Chip Select Option Register */

#define SIM_CSOR_AVEC		0x0001	/* autovector */
#define SIM_CSOR_IPL_ALL	0x0000	/* assert chip select for any level */
#define SIM_CSOR_IPL_1		0x0002	/* assert chip select for level 1 */
#define SIM_CSOR_IPL_2		0x0004	/* assert chip select for level 2 */
#define SIM_CSOR_IPL_3		0x0006	/* assert chip select for level 3 */
#define SIM_CSOR_IPL_4		0x0008	/* assert chip select for level 4 */
#define SIM_CSOR_IPL_5		0x000a	/* assert chip select for level 5 */
#define SIM_CSOR_IPL_6		0x000c	/* assert chip select for level 6 */
#define SIM_CSOR_IPL_7		0x000e	/* assert chip select for level 7 */
#define SIM_CSOR_SP_CPU		0x0000	/* CPU space */
#define SIM_CSOR_SP_USER	0x0010	/* user space */
#define SIM_CSOR_SP_SUPV	0x0020	/* supervisor space */
#define SIM_CSOR_SP_SU		0x0030	/* either supervisor or user space */
#define SIM_CSOR_WAIT0		0x0000	/* dsack with 0 wait states */
#define SIM_CSOR_WAIT1		0x0040	/* dsack with 1 wait states */
#define SIM_CSOR_WAIT2		0x0080	/* dsack with 2 wait states */
#define SIM_CSOR_WAIT3		0x00c0	/* dsack with 3 wait states */
#define SIM_CSOR_WAIT4		0x0100	/* dsack with 4 wait states */
#define SIM_CSOR_WAIT5		0x0140	/* dsack with 5 wait states */
#define SIM_CSOR_WAIT6		0x0180	/* dsack with 6 wait states */
#define SIM_CSOR_WAIT7		0x01c0	/* dsack with 7 wait states */
#define SIM_CSOR_WAIT8		0x0200	/* dsack with 8 wait states */
#define SIM_CSOR_WAIT9		0x0240	/* dsack with 9 wait states */
#define SIM_CSOR_WAIT10		0x0280	/* dsack with 10 wait states */
#define SIM_CSOR_WAIT11		0x02c0	/* dsack with 11 wait states */
#define SIM_CSOR_WAIT12		0x0300	/* dsack with 12 wait states */
#define SIM_CSOR_WAIT13		0x0340	/* dsack with 13 wait states */
#define SIM_CSOR_WAIT_FAST	0x0380	/* fast termination */
#define SIM_CSOR_WAIT_EXT	0x03c0	/* external dsack generation */
#define SIM_CSOR_AS	 	0x0000	/* strobe with AS* */
#define SIM_CSOR_DS		0x0400	/* strobe with DS* */
#define SIM_CSOR_READ		0x0800	/* read only */
#define SIM_CSOR_WRITE		0x1000	/* write only */
#define SIM_CSOR_RW		0x1800	/* read/write */
#define SIM_CSOR_BYTE_OFF	0x0000	/* off */
#define SIM_CSOR_BYTE_LOWER	0x2000	/* lower byte access */
#define SIM_CSOR_BYTE_UPPER	0x4000	/* upper byte access */
#define SIM_CSOR_BYTE_BOTH	0x6000	/* either upper or lower access */
#define SIM_CSOR_ASYNC		0x0000	/* asynchronous */
#define SIM_CSOR_SYNC		0x8000	/* synchronous */


/* RAM - Register definitions for the RAM Control Registers */

/* RAM_RAMMCR - RAM Module Configuration Register */

#define RAM_RAMMCR_SUPV		0x0100	/* supv. access to on chip RAM only */
#define RAM_RAMMCR_STOP		0x8000	/* low power stop mode */

/* RAM_RAMTST - RAM Test Register */

#define RAM_RAMTST_RTBA		0x0100	/* RAMBAR may be written as desired */

/* RAM_RAMBAR - RAM Base Address Register */

#define RAM_RAMBAR_DISABLE	0x0001	/* RAM array is disabled */
#define RAM_RAMBAR_ENABLE	0x0000	/* RAM array is enabled */


/* QSM - Register definitions for the Queued Serial Module (QSM) */

/* QSM_MCR - Module Configuration Register */

#define QSM_MCR_SUPV		0x0080	/* supervisor/unrestricted access */
#define QSM_MCR_FRZ0		0x2000	/* reserved */
#define QSM_MCR_FRZ1		0x4000	/* halt QSM on transfer bndry */
#define QSM_MCR_STOP		0x8000	/* stop enable */
#define QSM_MCR_IARB		0x000f	/* interrupt arbitration number mask */

/* QSM_QILR -  QSM Interrupt Level Register */

#define QSM_QILR_SCI_MASK	0x07	/* SCI interrupt level (0 = disabled) */
#define QSM_QILR_SPI_MASK	0x38	/* SCI interrupt level (0 = disabled) */
#define QSM_QILR_SPI_SHIFT	0x3	/* shift to position SPI */

/* QSM_SCCR1 - Control Register 1 */

#define QSM_SCCR1_SBK		0x0001	/* send break */
#define QSM_SCCR1_RWU		0x0002	/* receiver wakeup enable */
#define QSM_SCCR1_RE		0x0004	/* receiver enable */
#define QSM_SCCR1_TE		0x0008	/* transmitter enable */
#define QSM_SCCR1_ILIE		0x0010	/* idle-line interrupt enable */
#define QSM_SCCR1_RIE		0x0020	/* receiver interrupt enable */
#define QSM_SCCR1_TCIE		0x0040	/* transmit complete interrupt enable */
#define QSM_SCCR1_TIE		0x0080	/* transmit interrupt enable */
#define QSM_SCCR1_WAKE		0x0100	/* wakeup by address mark */
#define QSM_SCCR1_M		0x0200	/* mode select: 0= 8 data bits, 1 = 9 */
#define QSM_SCCR1_PE		0x0400	/* parity enable */
#define QSM_SCCR1_PT		0x0800	/* parity type: 1=Odd, 0=Even */
#define QSM_SCCR1_ILT		0x1000	/* idle line detect type */
#define QSM_SCCR1_WOMS		0x2000	/* wired or mode for SCI Pins */
#define QSM_SCCR1_LOOPS		0x4000	/* test SCI operation */

/* QSM_SCSR - Status Register */

#define QSM_SCSR_PF		0x0001	/* parity error flag */
#define QSM_SCSR_FE		0x0002	/* framing error flag */
#define QSM_SCSR_NF		0x0004	/* noise error flag */
#define QSM_SCSR_OR		0x0008	/* overrun error flag */
#define QSM_SCSR_IDLE		0x0010	/* idle-line detected flag */
#define QSM_SCSR_RAF		0x0020	/* receiver active flag */
#define QSM_SCSR_RDRF		0x0040	/* receive data register full flag */
#define QSM_SCSR_TC		0x0080	/* transmit complete */
#define QSM_SCSR_TDRE		0x0100	/* transmit date register empty flag */

/* QSM_QPDR/QSM_QDDR - Port Data and Data Direction Register */

#define QSM_QPDR_MISO		0x01	/* MISO pin */
#define QSM_QDDR_MISO		0x01	/* MISO pin */
#define QSM_QPDR_MOSI		0x02	/* MOSI pin */
#define QSM_QDDR_MOSI		0x02	/* MOSI pin */
#define QSM_QPDR_SCK		0x04	/* SCK pin */
#define QSM_QDDR_SCK		0x04	/* SCK pin */
#define QSM_QPDR_PCS0		0x08	/* PCS0 pin */
#define QSM_QDDR_PCS0		0x08	/* PCS0 pin */
#define QSM_QPDR_PCS1		0x10	/* PCS1 pin */
#define QSM_QDDR_PCS1		0x10	/* PCS1 pin */
#define QSM_QPDR_PCS2		0x20	/* PCS2 pin */
#define QSM_QDDR_PCS2		0x20	/* PCS2 pin */
#define QSM_QPDR_PCS3		0x40	/* PCS3 pin */
#define QSM_QDDR_PCS3		0x40	/* PCS3 pin */
#define QSM_QPDR_TXD		0x80	/* TXD pin */
#define QSM_QDDR_TXD		0x80	/* TXD pin */
#define	QSM_QPDR_PCS		0x78	/* all 4 PCS pins */
#define	QSM_QDDR_PCS		0x78	/* all 4 PCS pins */
#define	QSM_QPDR_PCS_SHIFT	0x3	/* justify PCS pins */

/* QSM_QPAR - Pin Assignment Register */

#define QSM_QPAR_MISO		0x0001	/* MISO pin */
#define QSM_QPAR_MOSI		0x0002	/* MOSI pin */
#define QSM_QPAR_PCS0		0x0008	/* PCS0 pin */
#define QSM_QPAR_PCS1		0x0010	/* PCS1 pin */
#define QSM_QPAR_PCS2		0x0020	/* PCS2 pin */
#define QSM_QPAR_PCS3		0x0040	/* PCS3 pin */

/* QSM_SPCR0 - Serial Peripheral Control Register 0 */

#define QSM_SPCR0_SPBR		0x00ff	/* mask for baud rate */
#define QSM_SPCR0_CPHA		0x0100	/* mask for change/capture data */
#define QSM_SPCR0_CPHA_CAP	0x0000	/* capture data on leading edge */
#define QSM_SPCR0_CPHA_CHG	0x0100	/* change data on leading edge */
#define QSM_SPCR0_CPOL		0x0200 	/* mask for clock polarity */
#define QSM_SPCR0_CPOL_LOW	0x0000 	/* clock polarity; SCK inactive low */
#define QSM_SPCR0_CPOL_HIGH	0x0200 	/* clock polarity; SCK inactive high */
#define QSM_SPCR0_BITS		0x3c00	/* bits mask */
#define QSM_SPCR0_BITS_SHIFT	0x000a	/* justify bits shift */
#define QSM_SPCR0_WOMQ		0x4000	/* wired or mode */
#define QSM_SPCR0_MASTER	0x8000	/* device as QSM master */
#define QSM_SPCR0_SLAVE		0x0000	/* device as QSM slave */

/* QSM_SPCR1 - Serial Peripheral Control Register 1 */

#define QSM_SPCR1_SPE		0x8000	/* QSPI is enabled */
#define QSM_SPCR1_DSCLK		0x7f00	/* DSCLK mask */
#define QSM_SPCR1_DSCLK_SHIFT	0x0008	/* justify DSCLK */
#define QSM_SPCR1_DTL		0x00ff	/* DTL mask */

/* QSM_SPCR2 - Serial Peripheral Control Register 2 */

#define QSM_SPCR2_NEWQP		0x000f	/* NEWQP mask */
#define QSM_SPCR2_ENDQP		0x0f00	/* ENDQP mask */
#define QSM_SPCR2_ENDQP_SHIFT	0x0008	/* justify ENDQP */
#define QSM_SPCR2_WRTO		0x2000	/* wrap to NEWQP (0 otherwise) */
#define QSM_SPCR2_WREN		0x4000	/* wrap enabled */
#define QSM_SPCR2_SPIFIE	0x8000	/* finished interrupt enabled */

/* QSM_SPCR3 - Serial Peripheral Control Register 3 */

#define QSM_SPCR3_HALT		0x01	/* halt after current transfer */
#define QSM_SPCR3_HMIE		0x02	/* HALTA or MODF interrupt enable */
#define QSM_SPCR3_LOOPQ		0x04	/* loop back mode */

/* QSM_SPSR - Serial Peripheral Status Register */

#define QSM_SPSR_CPTQP		0x0f	/* Completed queue pointer mask */
#define QSM_SPSR_HALTA		0x20	/* QSPI halted */
#define QSM_SPSR_MODF		0x40	/* mode fault (multiple masters) */
#define QSM_SPSR_SPIF		0x80	/* QSPI executed last command */

/* QSM_SPCCB - Serial Peripheral Command Control Byte */

#define QSM_SPCCB_DSCK		0x10	/* SPCR1 contains PCS setup time */
#define QSM_SPCCB_DT		0x20	/* SPCR1 contains inter-trans delay */
#define QSM_SPCCB_BITSE		0x40	/* SPCR0 contains number of bits */
#define QSM_SPCCB_CONT		0x80	/* continue to assert PCS after trans */


/* TPU - Time Processing Unit */

/* Channel control options */

#define CHN_PSC_PAC		0x0000	/* force pin to PAC latches */
#define CHN_PSC_HIGH		0x0001	/* force pin high */
#define CHN_PSC_LOW		0x0002	/* force pin low */
#define CHN_PSC_NC		0x0003	/* do not force any state */
#define CHN_PAC_NCM		0x0000	/* no change on match */
#define CHN_PAC_NO_DETECT	0x0000	/* do not detect transition */
#define CHN_PAC_HIGH		0x0004	/* high on match */
#define CHN_PAC_RISING		0x0004	/* detect rising edge */
#define CHN_PAC_LOW		0x0008	/* low on match */
#define CHN_PAC_FALLING		0x0008	/* detect falling edge */
#define CHN_PAC_TOGGLE		0x000c	/* toggle on match */
#define CHN_PAC_EITHER		0x000c	/* detect either edge transition */
#define CHN_PAC_NC		0x0010	/* don not change PAC */
#define CHN_TBS_INP_C1_M1	0x0000	/* input - capture TCR1, match TCR1 */
#define CHN_TBS_INP_C1_M2	0x0020	/* input - capture TCR1, match TCR2 */
#define CHN_TBS_INP_C2_M1	0x0040	/* input - capture TCR2, match TCR1 */
#define CHN_TBS_INP_C2_M2	0x0060	/* input - capture TCR2, match TCR2 */
#define CHN_TBS_OUT_C1_M1	0x0080	/* output - capture TCR1, match TCR1 */
#define CHN_TBS_OUT_C1_M2	0x00a0	/* output - capture TCR1, match TCR2 */
#define CHN_TBS_OUT_C2_M1	0x00c0	/* output - capture TCR2, match TCR1 */
#define CHN_TBS_OUT_C2_M2	0x00e0	/* output - capture TCR2, match TCR2 */
#define CHN_TBS_NC		0x0100	/* do not change TBS */


typedef struct		/* TPU_DIO */	/* discrete input/output */
    {
    unsigned short	chnCont;
    unsigned short	pinLvl;
    unsigned short	matchRate;
    } TPU_DIO;

/* DIO_HSQR - Host Sequence Register */

#define DIO_HSQR_TRANS		0x0	/* record pin on transition */
#define DIO_HSQR_MATCH		0x1	/* record pin at matchRate */
#define DIO_HSQR_RECORD		0x2	/* record pin on HSRR = 0x3 */

/* DIO_HSRR - Host Service Register */

#define DIO_HSRR_NONE		0x0	/* none */
#define DIO_HSRR_HIGH		0x1	/* force output high */
#define DIO_HSRR_LOW		0x2	/* force output low */
#define DIO_HSRR_INIT		0x3	/* initialization */


typedef struct		/* TPU_ITC */	/* input transition counter */
    {
    unsigned short	chnCont;
    unsigned short	linkBankAdrs;
    unsigned short	maxCount;
    unsigned short	transCount;
    unsigned short	finalTransTime;
    unsigned short	lastTransTime;
    } TPU_ITC;

/* ITC_HSQR - Host Sequence Register */

#define ITC_HSQR_SNGL		0x0	/* no link, single mode */
#define ITC_HSQR_CONT		0x1	/* no link, continuous mode */
#define ITC_HSQR_LINK_SNGL	0x2	/* link, single mode */
#define ITC_HSQR_LINK_CONT	0x3	/* link, continuous mode */

/* ITC_HSRR - Host Service Register */

#define ITC_HSRR_NONE		0x0	/* none */
#define ITC_HSRR_INIT		0x1	/* initialization */


typedef struct		/* TPU_OC */	/* output compare */
    {
    unsigned short	chnCont;
    unsigned short	offset;
    unsigned short	ratioRefAddr1;
    unsigned short	refAddr2Addr3;
    unsigned short	refTime;
    unsigned short	matchTime;
    } TPU_OC;

/* OC_HSQR - Host Sequence Register */

#define OC_HSQR_ALL		0x0	/* all pulse mode code executed */
#define OC_HSQR_RESTRICT	0x2	/* pulse mode code 0xec,0xee executed */

/* OC_HSRR - Host Service Register */

#define OC_HSRR_NONE		0x0	/* none */
#define OC_HSRR_HIPM		0x1	/* host initiated pulse mode */
#define OC_HSRR_INIT		0x3	/* initialization */


typedef struct		/* TPU_PWM */	/* pulse width modulation */
    {
    unsigned short	chnCont;
    unsigned short	oldris;
    unsigned short	pwmhi;
    unsigned short	pwmper;
    unsigned short	pwmris;
    } TPU_PWM;

/* PWM_HSRR - Host Service Register */

#define PWM_HSRR_NONE		0x0	/* none */
#define PWM_HSRR_UPDATE		0x1	/* immediate update request */
#define PWM_HSRR_INIT		0x2	/* initialization */


typedef struct		/* TPU_SPWM_M0 */ /* synchronized PWM mode 0*/
    {
    unsigned short	chnCont;
    unsigned short	nextRise;
    unsigned short	highTime;
    unsigned short	period;
    unsigned short	refAddr1;
    unsigned short 	delay;
    } TPU_SPWM_M0;

typedef struct		/* TPU_SPWM_M1 */ /* synchronized PWM mode 1*/
    {
    unsigned short	chnCont;
    unsigned short	nextRise;
    unsigned short	highTime;
    unsigned short	delay;
    unsigned short	refAddr1Addr2;
    unsigned short	refValue;
    } TPU_SPWM_M1;

typedef struct		/* TPU_SPWM_M2 */ /* synchronized PWM mode 2*/
    {
    unsigned short	chnCont;
    unsigned short	nextRise;
    unsigned short	highTime;
    unsigned short	period;
    unsigned short	linkRefAddr1;
    unsigned short 	delay;
    } TPU_SPWM_M2;

/* SPWM_HSQR - Host Sequence Register */

#define SPWM_HSQR_MODE0		0x0	/* mode 0 */
#define SPWM_HSQR_MODE1		0x1	/* mode 1 */
#define SPWM_HSQR_MODE2		0x2	/* mode 2 */

/* SPWM_HSRR - Host Service Register */

#define SPWM_HSRR_NONE		0x0	/* none */
#define SPWM_HSRR_INIT		0x2	/* initialization */
#define SPWM_HSRR_UPDATE	0x3	/* immediate update request */


typedef struct		/* TPU_PMA */	/* period measure w/additional trans */
    {
    unsigned short	chnCont;
    unsigned short	maxAddNumTeeth;
    unsigned short	addRollCnt;
    unsigned short	ratioTcrMax;
    unsigned short	periodHigh;
    unsigned short	periodLow;
    } TPU_PMA;

/* PMA_HSQR - Host Sequence Register */

#define PMA_HSQR_BANK		0x0	/* bank mode */
#define PMA_HSQR_COUNT		0x1	/* count mode */

/* PMA_HSRR - Host Service Register */

#define PMA_HSRR_NONE		0x0	/* none */
#define PMA_HSRR_INIT		0x1	/* initialization */


typedef struct		/* TPU_PMM */	/* period measure w/missing trans */
    {
    unsigned short	chnCont;
    unsigned short	maxMissNumTeeth;
    unsigned short	missRollCnt;
    unsigned short	ratioTcrMax;
    unsigned short	periodHigh;
    unsigned short	periodLow;
    } TPU_PMM;

/* PMM_HSQR - Host Sequence Register */

#define PMM_HSQR_BANK		0x2	/* bank mode */
#define PMM_HSQR_COUNT		0x3	/* count mode */

/* PMM_HSRR - Host Service Register */

#define PMM_HSRR_NONE		0x0	/* none */
#define PMM_HSRR_INIT		0x1	/* initialization */


typedef struct		/* TPU_PSP */	/* position-sync. pulse generator */
    {
    unsigned short	chnCont;
    unsigned short	r2A2Tmp;
    unsigned short	angleTime;
    unsigned short	ratioTmp;
    unsigned short	ratioAngle1;
    unsigned short	ratioAngle2;
    } TPU_PSP;

/* PSP_HSQR - Host Sequence Register */

#define PSP_HSQR_ANGLE		0x0	/* angle-angle mode */
#define PSP_HSQR_TIME		0x1	/* angle-time mode */

/* PSP_HSRR - Host Service Register */

#define PSP_HSRR_NONE		0x0	/* none */
#define PSP_HSRR_UPDATE		0x1	/* immediate update request */
#define PSP_HSRR_INIT		0x2	/* initialization */
#define PSP_HSRR_FORCE		0x3	/* force mode */


typedef struct		/* TPU_SM_PRI */ /* stepper motor primary channel */
    {
    unsigned short	chnCont;
    unsigned short	pinCont;
    unsigned short	currentPos;
    unsigned short	desiredPos;
    unsigned short	modCntNextStep;
    unsigned short	stepCntLastChn;
    } TPU_SM_PRI;


typedef struct		/* TPU_SM_SEC */ /* stepper motor secondary channel */
    {
    unsigned short	chnCont;
    unsigned short	pinCont;
    unsigned short	stepCntl0;
    unsigned short	stepCntl1;
    } TPU_SM_SEC;

/* SM_HSRR - Host Service Register */

#define SM_HSRR_NONE		0x0	/* none */
#define SM_HSRR_INIT		0x2	/* initialization */
#define SM_HSRR_STEP		0x3	/* step request */


typedef struct		/* TPU_PPWA */	/* period/pulse width accumulator */
    {
    unsigned short	chnCont;
    unsigned short	maxPeriodCnt;
    unsigned short	lastAccum;
    unsigned short	accum;
    unsigned short	accumRatePpwaUb;
    unsigned short	ppwaLw;
    } TPU_PPWA;

/* PPWA_HSQR - Host Sequence Register */

#define PPWA_HSQR_ACC_24	0x0	/* accumulate 24 bit periods */
#define PPWA_HSQR_ACC_16	0x1	/* accumulate 16 bit periods */

/* PPWA_HSRR - Host Service Register */

#define PPWA_HSRR_NONE		0x0	/* none */
#define PPWA_HSRR_INIT		0x2	/* initialization */

	/* mask set G */


typedef struct		/* TPU_FQD */	/* Fast Quadrature Decode Pri chan */
    {
    unsigned short	edgeTime;
    unsigned short	positionCount;
    unsigned short	tcr1Value;
    unsigned short	chanPinState;
    unsigned short	corrPinStateAdrs;
    unsigned short	edgeTimeLsbAdrs;
    } TPU_FQD_P;


typedef struct		/* TPU_FQD */	/* Fast Quadrature Decode Sec chan */
    {
    unsigned short	unused0;
    unsigned short	unused2;
    unsigned short	tcr1Value;
    unsigned short	chanPinState;
    unsigned short	corrPinStateAdrs;
    unsigned short	edgeTimeLsbAdrs;
    } TPU_FQD_S;

/* FQD_HSQR - Host Sequence Register */

#define FQD_HSQR_PRI_NORM	0x0	/* primary channel (normal mode) */
#define FQD_HSQR_SEC_NORM	0x1	/* secondary channel (normal mode) */
#define FQD_HSQR_PRI_FAST	0x2	/* primary channel (fast mode) */
#define FQD_HSQR_SEC_FAST	0x3	/* secondary channel (fast mode) */

/* FQD_HSRR - Host Service Register */

#define FQD_HSRR_NONE		0x0	/* none */
#define FQD_HSRR_INIT_TCR	0x2	/* read TCR1 */
#define FQD_HSRR_INIT_PARAM	0x3	/* initialize */


typedef struct		/* TPU_MCPWM */ /* Multichannel PWM Master */
    {
    unsigned short	period;
    unsigned short	irqRatePeriodCnt;
    unsigned short	lastRiseTime;
    unsigned short	lastFallTime;
    unsigned short	riseTimePointer;
    unsigned short	fallTimePointer;
    } TPU_MCPWM_M;

typedef struct		/* TPU_MCPWM */ /* Multichannel PWM Slave Edge mode */
    {
    unsigned short	period;
    unsigned short	highTime;
    unsigned short	unused4;
    unsigned short	highTimePointer;
    unsigned short	riseTimePointer;
    unsigned short	fallTimePointer;
    } TPU_MCPWM_SE;

typedef struct		/* TPU_MCPWM */ /* Multichan PWM Slave Center A mode */
    {
    unsigned short	period;
    unsigned short	nextBriseTime;
    unsigned short	nextBfallTime;
    unsigned short	deadHiTimePtr;
    unsigned short	riseTimePointer;
    unsigned short	fallTimePointer;
    } TPU_MCPWM_SCA;

typedef struct		/* TPU_MCPWM */ /* Multichan PWM Slave Center B mode */
    {
    unsigned short	highTime;
    unsigned short	currentHighTime;
    unsigned short	tempStorage;
    unsigned short	unused6;
    unsigned short	BfallTimePointer;
    unsigned short	BriseTimePointer;
    } TPU_MCPWM_SCB;

/* MCPWM_HSQR - Host Sequence Register */

#define MCPWM_HSQR_EDGE		0x0	/* Edge aligned mode */
#define MCPWM_HSQR_CNTR_A	0x1	/* Slave A Center aligned mode */
#define MCPWM_HSQR_CNTR_B	0x2	/* Slave B Center aligned mode */
#define MCPWM_HSQR_CNTR_B3	0x3	/* Slave B Center aligned mode */

/* MCPWM_HSRR - Host Service Register */

#define MCPWM_HSRR_NONE		0x0	/* none */
#define MCPWM_HSRR_INIT_SLAVI	0x1	/* initialize as Slave Inverted */
#define MCPWM_HSRR_INIT_SLAVN	0x2	/* initialize as Slave Normal */
#define MCPWM_HSRR_INIT_MSTR	0x3	/* initialize as Master */


typedef struct		/* TPU_HALLD */	/* Hall Effect Decode */
    {
    unsigned short	unused0;
    unsigned short	unused2;
    unsigned short	unused4;
    unsigned short	direction;
    unsigned short	stateNoAdrs;
    unsigned short	pinState;
    } TPU_HALLD;

/* HALLD_HSQR - Host Sequence Register */

#define HALLD_HSQR_CHAN_A	0x0	/* Channel A */
#define HALLD_HSQR_CHAN_B	0x1	/* Channel B */
#define HALLD_HSQR_CHAN_B2	0x1	/* Channel B */
#define HALLD_HSQR_CHAN_C	0x2	/* Channel C (3 chan mode only) */

/* HALLD_HSRR - Host Service Register */

#define HALLD_HSRR_NONE		0x0	/* none */
#define HALLD_HSRR_INIT_2CH	0x2	/* initialize 2 channel mode */
#define HALLD_HSRR_INIT_3CH	0x3	/* initialize 3 channel mode */


typedef struct		/* TPU_COMM_M */ /* Multiphase Motor Commutation */
    {
    unsigned short	desiredPosition;
    unsigned short	currentPosition;
    unsigned short	tableSizeIndex;
    unsigned short	slewPeriod;
    unsigned short	startPeriod;
    unsigned short	pinSequence;
    } TPU_COMM_M;


typedef struct		/* TPU_COMM_S */ /* Multiphase Motor Comm. States*/
    {
    unsigned short	state1;
    unsigned short	state2;
    unsigned short	state3;
    unsigned short	state4;
    unsigned short	state5;
    unsigned short	state6;
    unsigned short	state7;
    unsigned short	state8;
    } TPU_COMM_S;

/* COMM_HSQR - Host Sequence Register */

#define COMM_HSQR_SMUM		0x0	/* Sensorless match update mode */
#define COMM_HSQR_SMUM1		0x1	/* Sensorless match update mode */
#define COMM_HSQR_SLUM		0x2	/* Sensorless link update mode */
#define COMM_HSQR_SM		0x3	/* Sensored mode */

/* COMM_HSRR - Host Service Register */

#define COMM_HSRR_NONE		0x0	/* none */
#define COMM_HSRR_NONE1		0x1	/* none */
#define COMM_HSRR_INIT_PARAM	0x2	/* initialize or force state */
#define COMM_HSRR_INIT_MOVE	0x3	/* init or force immed state test */


typedef struct		/* TPU_NITC */	/* New input transition counter */
    {
    unsigned short	chnCont;
    unsigned short	linkParamAdrs;
    unsigned short	maxCount;
    unsigned short	transCount;
    unsigned short	finalTransTime;
    unsigned short	lastTransTime;
    } TPU_NITC;

/* NITC_HSQR - Host Sequence Register */

#define NITC_HSQR_SNGL		0x0	/* no link, single mode */
#define NITC_HSQR_CONT		0x1	/* no link, continuous mode */
#define NITC_HSQR_LINK_SNGL	0x2	/* link, single mode */
#define NITC_HSQR_LINK_CONT	0x3	/* link, continuous mode */

/* NITC_HSRR - Host Service Register */

#define NITC_HSRR_NONE		0x0	/* none */
#define NITC_HSRR_INIT_TCR	0x1	/* initialize TCR mode */
#define NITC_HSRR_INIT_PARAM	0x2	/* initialize Parameter mode */


typedef struct		/* TPU_UART_T */	/* Asynchronous Transmitter */
    {
    unsigned short	parityTemp;
    unsigned short	matchRate;
    unsigned short	tdreXmittData;
    unsigned short	dataSize;
    unsigned short	actualBitCount;
    unsigned short	shiftRegister;
    } TPU_UART_T;

typedef struct		/* TPU_UART_R */	/* Asynchronous Receiver */
    {
    unsigned short	parityTemp;
    unsigned short	matchRate;
    unsigned short	peFeReceiveData;
    unsigned short	dataSize;
    unsigned short	actualBitCount;
    unsigned short	shiftRegister;
    } TPU_UART_R;

/* UART_HSQR - Host Sequence Register */

#define UART_HSQR_NOPARITY	0x0	/* no parity */
#define UART_HSQR_EVEN		0x2	/* even parity */
#define UART_HSQR_ODD		0x3	/* odd parity */

/* UART_HSRR - Host Service Register */

#define UART_HSRR_XMITT		0x2	/* Transmitt */
#define UART_HSRR_RECEIVE	0x2	/* Receive */

typedef struct		/* TPU_FQM */	/* Frequency Measurement */
    {
    unsigned short	unused1;
    unsigned short	unused2;
    unsigned short	chnCont;
    unsigned short	windowSize;
    unsigned short	pulseCount;
    unsigned short	inWindowAccum;
    } TPU_FQM;

/* FQM_HSQR - Host Sequence Register */

#define FQM_HSQR_SNGL_FALL	0x0	/* no link, single mode */
#define FQM_HSQR_CONT_FALL	0x1	/* no link, continuous mode */
#define FQM_HSQR_SNGL_RISE	0x2	/* link, single mode */
#define FQM_HSQR_CONT_RISE	0x3	/* link, continuous mode */

/* FQM_HSRR - Host Service Register */

#define FQM_HSRR_NONE		0x0	/* none */
#define FQM_HSRR_INIT		0x2	/* initialize */

typedef struct		/* TPU_TSM_M */	/* Table stepper motor Master mode*/
    {
    unsigned short	desiredPosition;
    unsigned short	currentPosition;
    unsigned short	tableSizeIndex;
    unsigned short	slewPeriod;
    unsigned short	startPeriod;
    unsigned short	pinSequence;
    } TPU_TSM_M;


typedef struct		/* TPU_TSM_S */	/* Table stepper motor Slave mode*/
    {
    unsigned short	accelRatio21;
    unsigned short	accelRatio43;
    unsigned short	accelRatio65;
    unsigned short	accelRatio87;
    unsigned short	accelRatio109;
    unsigned short	accelRatio1211;
    unsigned short	accelRatio1413;
    unsigned short	accelRatio1615;
    } TPU_TSM_S;

/* TSM_HSQR - Host Sequence Register */

#define TSM_HSQR_LOCAL		0x0	/* local mode acceleration table */
#define TSM_HSQR_SPLIT		0x1	/* split mode acceleration table */
#define TSM_HSQR_ROT_ONCE	0x2	/* rotate pin.sequence once bet step */
#define TSM_HSQR_ROT_TWICE	0x3	/* rotate pin.sequence twice bet step */

/* TSM_HSRR - Host Service Register */

#define TSM_HSRR_NONE		0x0	/* none */
#define TSM_HSRR_INIT_TCR	0x1	/* initialize pin low */
#define TSM_HSRR_INIT_PARAM	0x2	/* initialize pin high */
#define TSM_HSRR_INIT_MOVE	0x3	/* move request (master only) */


typedef struct		/* TPU_QOM */	/* Queued output match */
    {
    unsigned short	refLastOffAdrs;
    unsigned short	loopOffPtr;
    unsigned short	offset1;
    unsigned short	offset2;
    unsigned short	offset3;
    unsigned short	offset4;
    unsigned short	offset5;
    unsigned short	offset6;
    } TPU_QOM;


typedef struct		/* TPU_QOM14 */	/* Queued output match using CH 14*/
    {
    unsigned short	refLastOffAdrs;
    unsigned short	loopOffPtr;
    unsigned short	offset1;
    unsigned short	offset2;
    unsigned short	offset3;
    unsigned short	offset4;
    unsigned short	offset5;
    unsigned short	offset6;	/* last offset in ch 14 */
    unsigned short	offset7;	/* first offset in ch 15 */
    unsigned short	offset8;
    unsigned short	offset9;
    unsigned short	offset10;
    unsigned short	offset11;
    unsigned short	offset12;
    unsigned short	offset13;
    unsigned short	offset14;	/* last offset avaliable */
    } TPU_QOM14;

/* QOM_CHAN - Channel definitions */
#define	QOM_TCR1 0
#define	QOM_TCR2 1

#define	QOM_LAST_OFF_ADR0 0
#define	QOM_LAST_OFF_ADR1 0x10
#define	QOM_LAST_OFF_ADR2 0x20
#define	QOM_LAST_OFF_ADR3 0x30
#define	QOM_LAST_OFF_ADR4 0x40
#define	QOM_LAST_OFF_ADR5 0x50
#define	QOM_LAST_OFF_ADR6 0x60
#define	QOM_LAST_OFF_ADR7 0x70
#define	QOM_LAST_OFF_ADR8 0x80
#define	QOM_LAST_OFF_ADR9 0x90
#define	QOM_LAST_OFF_ADRa 0xa0
#define	QOM_LAST_OFF_ADRb 0xb0
#define	QOM_LAST_OFF_ADRc 0xc0
#define	QOM_LAST_OFF_ADRd 0xd0
#define	QOM_LAST_OFF_ADRe 0xe0
#define	QOM_LAST_OFF_ADRf 0xf0

#define	QOM_LAST_OFF_1 4
#define	QOM_LAST_OFF_2 6
#define	QOM_LAST_OFF_3 8
#define	QOM_LAST_OFF_4 10
#define	QOM_LAST_OFF_5 12
#define	QOM_LAST_OFF_6 14
#define	QOM_LAST_OFF_7 16
#define	QOM_LAST_OFF_8 18
#define	QOM_LAST_OFF_9 20
#define	QOM_LAST_OFF_10 22
#define	QOM_LAST_OFF_11 24
#define	QOM_LAST_OFF_12 28
#define	QOM_LAST_OFF_13 30
#define	QOM_LAST_OFF_14 32

#define QOM_RISING_EDGE 1
#define QOM_FALLING_EDGE 0

/* QOM_HSQR - Host Sequence Register */

#define QOM_HSQR_SNGL		0x0	/* single shot mode */
#define QOM_HSQR_LOOP		0x1	/* loop mode */
#define QOM_HSQR_CONT2		0x2	/* continuous mode */
#define QOM_HSQR_CONT3		0x3	/* continuous mode */

/* QOM_HSRR - Host Service Register */

#define QOM_HSRR_NONE		0x0	/* none */
#define QOM_HSRR_INIT_PINNC	0x1	/* initialize No pin change */
#define QOM_HSRR_INIT_PINLO	0x2	/* initialize Pin Lo */
#define QOM_HSRR_INIT_PINHI	0x2	/* initialize Pin Hi */


typedef struct		/* TPU_PTA */	/* Programmable Time Accumulator */
    {
    unsigned short	chnCont;
    unsigned short	maxPeriodCount;
    unsigned short	lasttime;
    unsigned short	accum;
    unsigned int	hiLoWord;
    } TPU_PTA;

/* PTA_HSQR - Host Sequence Register */

#define PTA_HSQR_HI_ACCUM	0x0	/* Hi time accumulate */
#define PTA_HSQR_LOW_ACCUM	0x1	/* Lo time accumulate */
#define PTA_HSQR_PER_RISE	0x2	/* Period accumulate Rising */
#define PTA_HSQR_PER_FALL	0x3	/* Period accumulate Falling */

/* PTA_HSRR - Host Service Register */

#define PTA_HSRR_NONE		0x0	/* none */
#define PTA_HSRR_INIT		0x3	/* initialize */


typedef struct		/* TPU_CHN */
    {
    union
	{
	/* mask set A */
	TPU_DIO		dio;		/* discrete input/output */
	TPU_ITC		itc;		/* input transition counter */
	TPU_OC		oc;		/* output compare */
	TPU_PWM		pwm;		/* pulse width modulation */
	TPU_SPWM_M0	spwmM0;		/* synchronized pulse width modulation*/
	TPU_SPWM_M1	spwmM1;		/* synchronized pulse width modulation*/
	TPU_SPWM_M2	spwmM2;		/* synchronized pulse width modulation*/
	TPU_PMA		pma;		/* period measure w/additional trans */
	TPU_PMM		pmm;		/* period measure w/missing trans */
	TPU_PSP		psp;		/* position-sync. pulse generator */
	TPU_SM_PRI	smPri;		/* stepper motor primary channel */
	TPU_SM_SEC	smSec;		/* stepper motor secondary channel */
	TPU_PPWA	ppwa;		/* period/pulse width accumulator */
	/* mask set G */
	TPU_FQD_P	fqdP;	 	/* Fast Quadrature Decode Primary */
	TPU_FQD_S	fqdS;	 	/* Fast Quadrature Decode Secondary */
	TPU_MCPWM_M	mcpwmM;		/* Multichannel pulse width modulation*/
	TPU_MCPWM_SE	mcpwmSE;	/* MCPWM Slave Edge aligned */
	TPU_MCPWM_SCA	mcpwmSCA;	/* MCPWM Slave Center aligned A */
	TPU_MCPWM_SCB	mcpwmSCB;	/* MCPWM Slave Center aligned B */
	TPU_HALLD	halld;	 	/* Hall Effect Decode */
	TPU_COMM_M	commM;		/* Multiphase Motor Commutation Master */
	TPU_COMM_S	commS;		/* Multiphase Motor Commutation Slave */
	TPU_NITC	nitc;		/* New input transition counter */
	TPU_UART_T	uartT;		/* Asynchronous Reciver/Transmitter */
	TPU_UART_R	uartR;		/* Asynchronous Reciver/Transmitter */
	TPU_FQM		fqm;		/* Frequency Measurement */
	TPU_TSM_M	tsmM;		/* Table stepper motor Master */
	TPU_TSM_S	tsmS;		/* Table stepper motor Slave */
	TPU_QOM		qom;		/* Queued output match */
	TPU_QOM14		qom14;		/* Queued output match using CH 14/15 together */
	TPU_PTA		pta;		/* Programmable Time Accumulator */

	unsigned short	pad[8];		/* each channel has 8 parameters */
	} tf;
    } TPU_CHN;

/* TPU_TMCR - TPU Module Configuration Register */

#define TPU_TMCR_PSCK_32X	0x0000	/* clock/32 is TCR1 prescaler input */
#define TPU_TMCR_PSCK_4X	0x0040	/* clock/4 is TCR1 prescaler input */
#define TPU_TMCR_SUPV		0x0080	/* supervisor access only */
#define TPU_TMCR_STF		0x0100	/* TPU is stopped */
#define TPU_TMCR_T2CG		0x0200	/* TCR2 pin is gate of DIV8 clock */
#define TPU_TMCR_EMU		0x0400	/* TPU and RAM in emulation mode */
#define TPU_TMCR_TCR2_1X	0x0000	/* TCR2 divide by 1 prescaler */
#define TPU_TMCR_TCR2_2X	0x0800	/* TCR2 divide by 2 prescaler */
#define TPU_TMCR_TCR2_4X	0x1000	/* TCR2 divide by 4 prescaler */
#define TPU_TMCR_TCR2_8X	0x1800	/* TCR2 divide by 8 prescaler */
#define TPU_TMCR_TCR1_1X	0x0000	/* TCR1 divide by 1 prescaler */
#define TPU_TMCR_TCR1_2X	0x2000	/* TCR1 divide by 2 prescaler */
#define TPU_TMCR_TCR1_4X	0x4000	/* TCR1 divide by 4 prescaler */
#define TPU_TMCR_TCR1_8X	0x6000	/* TCR1 divide by 8 prescaler */
#define TPU_TMCR_STOP		0x8000	/* stop the clocks and microengine */

/* TPU_CFSR - Channel Function Select Register */

	/* Functions in mask set A */
#define TPU_CFSR_DIO		0x8 	/* discrete input/output */
#define TPU_CFSR_ITC		0xa	/* input transition counter */
#define TPU_CFSR_OC		0xe	/* output compare */
#define TPU_CFSR_PWM		0x9	/* pulse width modulation */
#define TPU_CFSR_SPWM		0x7	/* synchronized pulse width modulation*/
#define TPU_CFSR_PMA		0xb	/* period measure w/additional trans */
#define TPU_CFSR_PMM		0xb	/* period measure w/missing trans */
#define TPU_CFSR_PSP		0xc	/* position-sync. pulse generator */
#define TPU_CFSR_SM		0xd	/* stepper motor */
#define TPU_CFSR_PPWA		0xf	/* period/pulse width accumulator */
	/* Functions in mask set G */
#define TPU_CFSR_FQD		0x6 	/* Fast Quadrature Decode */
#define TPU_CFSR_MCPWM		0x7	/* Multichannel pulse width modulation*/
#define TPU_CFSR_HALLD		0x8 	/* Hall Effect Decode */
#define TPU_CFSR_COMM		0x9	/* Multiphase Motor Commutation */
#define TPU_CFSR_NITC		0xa	/* New input transition counter */
#define TPU_CFSR_UART		0xb	/* Asynchronous Reciver/Transmitter */
#define TPU_CFSR_FQM		0xc	/* Frequency Measurement */
#define TPU_CFSR_TSM		0xd	/* Table stepper motor */
#define TPU_CFSR_QOM		0xe	/* Queued output match */
#define TPU_CFSR_PTA		0xf	/* Programmable Time Accumulator */

/* TPU_CPR - Channel Priority Register */

#define TPU_CPR_DISABLE		0x0	/* channel disabled */
#define TPU_CPR_LOW		0x1	/* low priority */
#define TPU_CPR_MEDIUM		0x2	/* medium priority */
#define TPU_CPR_HIGH		0x3	/* high priority */

/* function declarations */

#ifndef INCLUDE_TY_CO_DRV_50
#if defined(__STDC__) || defined(__cplusplus)

IMPORT  void    tyCoInt (void);

#else   /* __STDC__ */

IMPORT  void    tyCoInt ();

#endif  /* __STDC__ */
#endif  /* INCLUDE_TY_CO_DRV_50 */

#endif	/* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* __INCm68332h */
