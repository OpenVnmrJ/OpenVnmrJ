/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* m68332_nmri.h Copyright (c) 1991-1996 Varian Assoc.,Inc. All Rights Reserved */
/*
 */

/*
modification history
--------------------
..., 2Feb95,gad  added TPU defines for mask set G
*/

/*
This file contains I/O address and related constants for the MC68332.
as modified and extended at NMR Instruments, Varian Associates
*/

#ifndef __INCm68332_nmri_h
#define __INCm68332_nmri_h

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _ASMLANGUAGE

#include <drv/multi/m68332.h>

#define TPU_USHORT_ADDR(a) ( (volatile unsigned short* const) (a))
#define TPU_UINT_ADDR(a)  ( (volatile unsigned int* const) (a))

/* MC68332 parameter register addresses, extensions required for NMRI */

#define M332_QSM_QPA_DDR	((unsigned short *) 0xfffc16)

#define M332_QSM_RX_SIZE	16
#define M332_QSM_TX_SIZE	16
#define M332_QSM_CMD_SIZE	16

#define M332_TPU_CHAN0		0xffff00
#define M332_TPU_CHAN1		0xffff10

#define M332_TPU_CHN0		((TPU_CHN_NMRI *) 0xffff00)
#define M332_TPU_CHN1		((TPU_CHN_NMRI *) 0xffff10)
#define M332_TPU_CHN2		((TPU_CHN_NMRI *) 0xffff20)
#define M332_TPU_CHN3		((TPU_CHN_NMRI *) 0xffff30)
#define M332_TPU_CHN4		((TPU_CHN_NMRI *) 0xffff40)
#define M332_TPU_CHN5		((TPU_CHN_NMRI *) 0xffff50)
#define M332_TPU_CHN6		((TPU_CHN_NMRI *) 0xffff60)
#define M332_TPU_CHN7		((TPU_CHN_NMRI *) 0xffff70)
#define M332_TPU_CHN8		((TPU_CHN_NMRI *) 0xffff80)
#define M332_TPU_CHN9		((TPU_CHN_NMRI *) 0xffff90)
#define M332_TPU_CHNa		((TPU_CHN_NMRI *) 0xffffa0)
#define M332_TPU_CHN10	((TPU_CHN_NMRI *) 0xffffa0)
#define M332_TPU_CHNb		((TPU_CHN_NMRI *) 0xffffb0)
#define M332_TPU_CHN11	((TPU_CHN_NMRI *) 0xffffb0)
#define M332_TPU_CHNc		((TPU_CHN_NMRI *) 0xffffc0)
#define M332_TPU_CHN12	((TPU_CHN_NMRI *) 0xffffc0)
#define M332_TPU_CHNd		((TPU_CHN_NMRI *) 0xffffd0)
#define M332_TPU_CHN13	((TPU_CHN_NMRI *) 0xffffd0)
#define M332_TPU_CHNe		((TPU_CHN_NMRI *) 0xffffe0)
#define M332_TPU_CHN14	((TPU_CHN_NMRI *) 0xffffe0)
#define M332_TPU_CHNf		((TPU_CHN_NMRI *) 0xfffff0)
#define M332_TPU_CHN15	((TPU_CHN_NMRI *) 0xfffff0)

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


/* QSM - Register definitions for the Queued Serial Module (QSM)
         extensions required by NMRI				*/

/* QSM_MCR - Module Configuration Register */

#define QSM_MCR_IARB		0x000f	/* interrupt arbitration number mask */

/* QSM_QILR -  QSM Interrupt Level Register */

#define QSM_QILR_SPI_SHIFT	0x3	/* shift to position SPI */

/* QSM_QPDR/QSM_QDDR - Port Data and Data Direction Register */

#define QSM_QDDR_MISO		0x01	/* MISO pin */
#define QSM_QDDR_MOSI		0x02	/* MOSI pin */
#define QSM_QDDR_SCK		0x04	/* SCK pin */
#define QSM_QDDR_PCS0		0x08	/* PCS0 pin */
#define QSM_QDDR_PCS1		0x10	/* PCS1 pin */
#define QSM_QDDR_PCS2		0x20	/* PCS2 pin */
#define QSM_QDDR_PCS3		0x40	/* PCS3 pin */
#define QSM_QDDR_TXD		0x80	/* TXD pin */
#define	QSM_QPDR_PCS		0x78	/* all 4 PCS pins */
#define	QSM_QDDR_PCS		0x78	/* all 4 PCS pins */
#define	QSM_QPDR_PCS_SHIFT	0x3	/* justify PCS pins */

/* QSM_SPCR0 - Serial Peripheral Control Register 0 */

#define QSM_SPCR0_SPBR		0x00ff	/* mask for baud rate */
#define QSM_SPCR0_CPHA		0x0100	/* mask for change/capture data */
#define QSM_SPCR0_CPOL		0x0200 	/* mask for clock polarity */
#define QSM_SPCR0_BITS		0x3c00	/* bits mask */
#define QSM_SPCR0_BITS_SHIFT	0x000a	/* justify bits shift */

/* QSM_SPCR1 - Serial Peripheral Control Register 1 */

#define QSM_SPCR1_DSCLK		0x7f00	/* DSCLK mask */
#define QSM_SPCR1_DSCLK_SHIFT	0x0008	/* justify DSCLK */
#define QSM_SPCR1_DTL		0x00ff	/* DTL mask */

/* QSM_SPCR2 - Serial Peripheral Control Register 2 */

#define QSM_SPCR2_NEWQP		0x000f	/* NEWQP mask */
#define QSM_SPCR2_ENDQP		0x0f00	/* ENDQP mask */
#define QSM_SPCR2_ENDQP_SHIFT	0x0008	/* justify ENDQP */

/* QSM_SPSR - Serial Peripheral Status Register */

#define QSM_SPSR_CPTQP		0x0f	/* Completed queue pointer mask */


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

#define QOM_REFADDR_B_LASTOFFADDR_A  	0x00
#define QOM_LOOPCNT_OFFPTR  		0x02
#define QOM_OFFSET_1			0x04
#define QOM_OFFSET_2			0x06
#define QOM_OFFSET_3			0x08
#define QOM_OFFSET_4			0x0A


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

#define PTA_CHAN_CNTRL 		0x00
#define PTA_MAXCNT_PERIODCNT 	0x02
#define PTA_LAST_TIME 		0x04
#define PTA_ACCUM 		0x06
#define PTA_HW 			0x08
#define PTA_LW 			0x0A

/* PTA_HSQR - Host Sequence Register */

#define PTA_HSQR_HI_ACCUM	0x0	/* Hi time accumulate */
#define PTA_HSQR_LOW_ACCUM	0x1	/* Lo time accumulate */
#define PTA_HSQR_PER_RISE	0x2	/* Period accumulate Rising */
#define PTA_HSQR_PER_FALL	0x3	/* Period accumulate Falling */

/* PTA_HSRR - Host Service Register */

#define PTA_HSRR_NONE		0x0	/* none */
#define PTA_HSRR_INIT		0x3	/* initialize */

typedef struct		/* TPU_CHN_NMRI */
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
    } TPU_CHN_NMRI;


/* TPU_CFSR - Channel Function Select Register */

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

#endif	/* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* __INCm68332h */
