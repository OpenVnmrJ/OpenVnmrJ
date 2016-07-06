/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* Id: dmaDcr.h,v 1.3 2004/04/30 15:05:12 rthrift Exp rthrift  */
/* dmaDcr.h - IBM DMA controller DCR access assembly routines */

/*******************************************************************************
   This source and object code has been made available to you by IBM on an
   AS-IS basis.

   IT IS PROVIDED WITHOUT WARRANTY OF ANY KIND, INCLUDING THE WARRANTIES OF
   MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE OR OF NONINFRINGEMENT
   OF THIRD PARTY RIGHTS.  IN NO EVENT SHALL IBM OR ITS LICENSORS BE LIABLE
   FOR INCIDENTAL, CONSEQUENTIAL OR PUNITIVE DAMAGES.  IBM'S OR ITS LICENSOR'S
   DAMAGES FOR ANY CAUSE OF ACTION, WHETHER IN CONTRACT OR IN TORT, AT LAW OR
   AT EQUITY, SHALL BE LIMITED TO A MAXIMUM OF $1,000 PER LICENSE.  No license
   under IBM patents or patent applications is to be implied by the copyright
   license.

   Any user of this software should understand that neither IBM nor its
   licensors will be responsible for any consequences resulting from the use
   of this software.

   Any person who transfers this source code or any derivative work must
   include the IBM copyright notice, this paragraph, and the preceding two
   paragraphs in the transferred software.

   Any person who transfers this object code or any derivative work must
   include the IBM copyright notice in the transferred software.

   COPYRIGHT   I B M   CORPORATION 2000
   LICENSED MATERIAL  -  PROGRAM PROPERTY OF  I B M"

*******************************************************************************/

/* Copyright 1984-2000 Wind River Systems, Inc. */

/*
modification history
--------------------
12 Nov. 2003, R. Thrift, Varian, Inc.:
	Cleanup of formatting & comments.
	Fixed incorrect channel priority definitions.
01c,12jun02,pch  SPR 74987: C++
01b,19nov01,pch  cleanup
01a,23may00,mcg  created
*/

/*
This file contains the definitions of the Device Control Register (DCR)
access functions of the IBM DMA controller core.
*/

#ifndef INCdmadcrh
#define INCdmadcrh

#ifdef __cplusplus
    extern "C" {
#endif

/* 
 * -----------------------------------------------------------------------
 * DMA Controller Register Definitions
 * -----------------------------------------------------------------------
 */
/* ---------- DMA Controller Channel 0 Register Definitions ----------- */
#define DMA_CR0   (DMA_DCR_BASE+0x00)    /* channel control register 0        */
#define DMA_CT0   (DMA_DCR_BASE+0x01)    /* count register 0                  */
#define DMA_DA0   (DMA_DCR_BASE+0x02)    /* destination address register 0    */
#define DMA_SA0   (DMA_DCR_BASE+0x03)    /* source address register 0         */
#define DMA_SG0   (DMA_DCR_BASE+0x04)    /* scatter/gather descriptor addr 0  */

/* ---------- DMA Controller Channel 1 Register Definitions ----------- */
#define DMA_CR1   (DMA_DCR_BASE+0x08)    /* channel control register 1        */
#define DMA_CT1   (DMA_DCR_BASE+0x09)    /* count register 1                  */
#define DMA_DA1   (DMA_DCR_BASE+0x0a)    /* destination address register 1    */
#define DMA_SA1   (DMA_DCR_BASE+0x0b)    /* source address register 1         */
#define DMA_SG1   (DMA_DCR_BASE+0x0c)    /* scatter/gather descriptor addr 1  */

/* ---------- DMA Controller Channel 2 Register Definitions ----------- */
#define DMA_CR2   (DMA_DCR_BASE+0x10)    /* channel control register 2        */
#define DMA_CT2   (DMA_DCR_BASE+0x11)    /* count register 2                  */
#define DMA_DA2   (DMA_DCR_BASE+0x12)    /* destination address register 2    */
#define DMA_SA2   (DMA_DCR_BASE+0x13)    /* source address register 2         */
#define DMA_SG2   (DMA_DCR_BASE+0x14)    /* scatter/gather descriptor addr 2  */

/* ---------- DMA Controller Channel 3 Register Definitions ----------- */
#define DMA_CR3   (DMA_DCR_BASE+0x18)    /* channel control register 3        */
#define DMA_CT3   (DMA_DCR_BASE+0x19)    /* count register 3                  */
#define DMA_DA3   (DMA_DCR_BASE+0x1a)    /* destination address register 3    */
#define DMA_SA3   (DMA_DCR_BASE+0x1b)    /* source address register 3         */
#define DMA_SG3   (DMA_DCR_BASE+0x1c)    /* scatter/gather descriptor addr 3  */

/* ------------ DMA Controller Registers for All Channels ------------- */
#define DMA_SR    (DMA_DCR_BASE+0x20)    /* status register                   */
#define DMA_SGC   (DMA_DCR_BASE+0x23)    /* scatter/gather command register   */
#define DMA_SLP   (DMA_DCR_BASE+0x25)    /* sleep mode                        */
#define DMA_POL   (DMA_DCR_BASE+0x26)    /* polarity                          */

/* 
 * -----------------------------------------------------------------------
 * DMA Channel Control Register Bits (Same for all channels)
 * -----------------------------------------------------------------------
 */
#define DMA_CR_CE         0x80000000    /* channel enable                     */
#define DMA_CR_CIE        0x40000000    /* channel interrupt enable           */
#define DMA_CR_TD         0x20000000    /* transfer direction                 */
#define DMA_CR_PL         0x10000000    /* peripheral location                */
#define DMA_CR_PW         0x0C000000    /* peripheral width mask              */
#define DMA_CR_PW_BYTE    0x00000000    /* peripheral width byte              */
#define DMA_CR_PW_HW      0x04000000    /* peripheral width halfword          */
#define DMA_CR_PW_WORD    0x08000000    /* peripheral width word              */
#define DMA_CR_PW_DW      0x0C000000    /* peripheral width doubleword        */
#define DMA_CR_DAI        0x02000000    /* destination address increment      */
#define DMA_CR_SAI        0x01000000    /* source address increment           */
#define DMA_CR_BEN        0x00800000    /* buffer enable                      */
#define DMA_CR_TM         0x00600000    /* transfer mode mask                 */
#define DMA_CR_TM_PER     0x00000000    /* transfer mode peripheral           */
#define DMA_CR_TM_SWMM    0x00400000    /* transfer mode SW memory-to-memory  */
#define DMA_CR_TM_HWDP    0x00600000    /* transfer mode HW deviced paced     */
#define DMA_CR_PSC        0x00180000    /* peripheral setup cycles mask       */
#define DMA_CR_PSC_NONE   0x00000000    /* peripheral setup cycles 0          */
#define DMA_CR_PSC_1      0x00080000    /* peripheral setup cycles 1          */
#define DMA_CR_PSC_2      0x00100000    /* peripheral setup cycles 2          */
#define DMA_CR_PSC_3      0x00180000    /* peripheral setup cycles 3          */
#define DMA_CR_PWC        0x0007E000    /* peripheral wait cycles mask        */
#define DMA_CR_PHC        0x00001C00    /* peripheral hold cycles mask        */
#define DMA_CR_ETD        0x00000200    /* end-of-transfer / terminal count   */
#define DMA_CR_TCE        0x00000100    /* terminal count enable              */
/* 
 * -----------------------------------------------------------------------
 * Note!
 * Channel priority bits for medium-low and medium-high have been modified
 * from the original VxWorks dmaDcr.h file. They were both mistakenly set
 * to 0x000000C0, which is actually high priority. -RLT
 * -----------------------------------------------------------------------
 */
#define DMA_CR_CP         0x000000C0    /* channel priority mask              */
#define DMA_CR_CP_LOW     0x00000000    /* channel priority low               */
#define DMA_CR_CP_MEDL    0x00000040    /* channel priority medium-low        */
#define DMA_CR_CP_MEDH    0x00000080    /* channel priority medium-high       */
#define DMA_CR_CP_HIGH    0x000000C0    /* channel priority high              */
#define DMA_CR_PF         0x00000030    /* memory read prefetch mask          */
#define DMA_CR_PF_1       0x00000000    /* memory read prefetch 1 doubleword  */
#define DMA_CR_PF_2       0x00000010    /* memory read prefetch 2 doubleword  */
#define DMA_CR_PF_4       0x00000020    /* memory read prefetch 4 doubleword  */
#define DMA_CR_PCE        0x00000008    /* parity check enable                */
#define DMA_CR_DEC        0x00000004    /* address decrement                  */

/* 
 * -----------------------------------------------------------------------
 * DMA Status Register Bits
 * -----------------------------------------------------------------------
 */
#define DMA_SR_CS         0xF0000000    /* channel terminal count mask        */
#define DMA_SR_CS_0       0x80000000    /* channel 0 terminal count           */
#define DMA_SR_CS_1       0x40000000    /* channel 1 terminal count           */
#define DMA_SR_CS_2       0x20000000    /* channel 2 terminal count           */
#define DMA_SR_CS_3       0x10000000    /* channel 3 terminal count           */

#define DMA_SR_TS         0x0F000000    /* channel end-of-transfer status mask*/
#define DMA_SR_TS_0       0x08000000    /* channel 0 end-of-transfer status   */
#define DMA_SR_TS_1       0x04000000    /* channel 1 end-of-transfer status   */
#define DMA_SR_TS_2       0x02000000    /* channel 2 end-of-transfer status   */
#define DMA_SR_TS_3       0x01000000    /* channel 3 end-of-transfer status   */

#define DMA_SR_RI         0x00F00000    /* channel error status mask          */
#define DMA_SR_RI_0       0x00800000    /* channel 0 error status             */
#define DMA_SR_RI_1       0x00400000    /* channel 1 error status             */
#define DMA_SR_RI_2       0x00200000    /* channel 2 error status             */
#define DMA_SR_RI_3       0x00100000    /* channel 3 error status             */

#define DMA_SR_IR         0x000F0000    /* channel internal request mask      */
#define DMA_SR_IR_0       0x00080000    /* channel 0 internal request         */
#define DMA_SR_IR_1       0x00040000    /* channel 1 internal request         */
#define DMA_SR_IR_2       0x00020000    /* channel 2 internal request         */
#define DMA_SR_IR_3       0x00010000    /* channel 3 internal request         */

#define DMA_SR_ER         0x0000F000    /* channel external request mask      */
#define DMA_SR_ER_0       0x00008000    /* channel 0 external request         */
#define DMA_SR_ER_1       0x00004000    /* channel 1 external request         */
#define DMA_SR_ER_2       0x00002000    /* channel 2 external request         */
#define DMA_SR_ER_3       0x00001000    /* channel 3 external request         */

#define DMA_SR_CB         0x00000F00    /* channel busy mask                  */
#define DMA_SR_CB_0       0x00000800    /* channel 0 busy                     */
#define DMA_SR_CB_1       0x00000400    /* channel 1 busy                     */
#define DMA_SR_CB_2       0x00000200    /* channel 2 busy                     */
#define DMA_SR_CB_3       0x00000100    /* channel 3 busy                     */

#define DMA_SR_SG         0x000000F0    /* channel scatter/gather status mask */
#define DMA_SR_SG_0       0x00000080    /* channel 0 scatter/gather status    */
#define DMA_SR_SG_1       0x00000040    /* channel 1 scatter/gather status    */
#define DMA_SR_SG_2       0x00000020    /* channel 2 scatter/gather status    */
#define DMA_SR_SG_3       0x00000010    /* channel 3 scatter/gather status    */

/* 
 * -----------------------------------------------------------------------
 * DMA Scatter/gather Command Register Bits
 * -----------------------------------------------------------------------
 */
#define DMA_SGC_SSG0      0x80000000    /* channel 0 scatter/gather enable    */
#define DMA_SGC_SSG1      0x40000000    /* channel 1 scatter/gather enable    */
#define DMA_SGC_SSG2      0x20000000    /* channel 2 scatter/gather enable    */
#define DMA_SGC_SSG3      0x10000000    /* channel 3 scatter/gather enable    */

#define DMA_SGC_EM0       0x00008000    /* channel 0 scatter/gather mask      */
#define DMA_SGC_EM1       0x00004000    /* channel 1 scatter/gather mask      */
#define DMA_SGC_EM2       0x00002000    /* channel 2 scatter/gather mask      */
#define DMA_SGC_EM3       0x00001000    /* channel 3 scatter/gather mask      */

/* 
 * -----------------------------------------------------------------------
 * DMA Polarity Configuration Register Bits
 * -----------------------------------------------------------------------
 */
#define DMA_POL_R0P       0x80000000    /* DMAReq0 Polarity                   */
#define DMA_POL_A0P       0x40000000    /* DMAAck0 Polarity                   */
#define DMA_POL_E0P       0x20000000    /* EOT0[TC0] Polarity                 */

#define DMA_POL_R1P       0x10000000    /* DMAReq1 Polarity                   */
#define DMA_POL_A1P       0x08000000    /* DMAAck1 Polarity                   */
#define DMA_POL_E1P       0x04000000    /* EOT1[TC1] Polarity                 */

#define DMA_POL_R2P       0x02000000    /* DMAReq2 Polarity                   */
#define DMA_POL_A2P       0x01000000    /* DMAAck2 Polarity                   */
#define DMA_POL_E2P       0x00800000    /* EOT2[TC2] Polarity                 */

#define DMA_POL_R3P       0x00400000    /* DMAReq3 Polarity                   */
#define DMA_POL_A3P       0x00200000    /* DMAAck3 Polarity                   */
#define DMA_POL_E3P       0x00100000    /* EOT3[TC3] Polarity                 */

#ifndef _ASMLANGUAGE

/* 
 * -----------------------------------------------------------------------
 * DMA DCR Access Functions
 * -----------------------------------------------------------------------
 */
/* --------------------- Configuration Registers ---------------------- */
UINT32  sysDcrDmacr0Get(void);              /* DMA Chn 0 Config Reg           */
void    sysDcrDmacr0Set(UINT32);

UINT32  sysDcrDmacr1Get(void);              /* DMA Chn 1 Config Reg           */
void    sysDcrDmacr1Set(UINT32);

UINT32  sysDcrDmacr2Get(void);              /* DMA Chn 2 Config Reg           */
void    sysDcrDmacr2Set(UINT32);

UINT32  sysDcrDmacr3Get(void);              /* DMA Chn 3 Config Reg           */
void    sysDcrDmacr3Set(UINT32);

/* ------------------------- Count Registers -------------------------- */
UINT32  sysDcrDmact0Get(void);              /* DMA Chn 0 Count Reg            */
void    sysDcrDmact0Set(UINT32);

UINT32  sysDcrDmact1Get(void);              /* DMA Chn 1 Count Reg            */
void    sysDcrDmact1Set(UINT32);

UINT32  sysDcrDmact2Get(void);              /* DMA Chn 2 Count Reg            */
void    sysDcrDmact2Set(UINT32);

UINT32  sysDcrDmact3Get(void);              /* DMA Chn 3 Count Reg            */
void    sysDcrDmact3Set(UINT32);

/* ------------------ Destination Address Registers ------------------- */
UINT32  sysDcrDmada0Get(void);              /* DMA Chn 0 Dest Addr Reg        */
void    sysDcrDmada0Set(UINT32);

UINT32  sysDcrDmada1Get(void);              /* DMA Chn 1 Dest Addr Reg        */
void    sysDcrDmada1Set(UINT32);

UINT32  sysDcrDmada2Get(void);              /* DMA Chn 2 Dest Addr Reg        */
void    sysDcrDmada2Set(UINT32);

UINT32  sysDcrDmada3Get(void);              /* DMA Chn 3 Dest Addr Reg        */
void    sysDcrDmada3Set(UINT32);

/* --------------------- Source Address Registers --------------------- */
UINT32  sysDcrDmasa0Get(void);              /* DMA Chn 0 Src Addr Reg         */
void    sysDcrDmasa0Set(UINT32);

UINT32  sysDcrDmasa1Get(void);              /* DMA Chn 1 Src Addr Reg         */
void    sysDcrDmasa1Set(UINT32);

UINT32  sysDcrDmasa2Get(void);              /* DMA Chn 2 Src Addr Reg         */
void    sysDcrDmasa2Set(UINT32);

UINT32  sysDcrDmasa3Get(void);              /* DMA Chn 3 Src Addr Reg         */
void    sysDcrDmasa3Set(UINT32);

/* --------------------- Scatter/Gather Registers --------------------- */
UINT32  sysDcrDmasg0Get(void);              /* DMA Chn 0 scatter/gather reg   */
void    sysDcrDmasg0Set(UINT32);

UINT32  sysDcrDmasg1Get(void);              /* DMA Chn 1 scatter/gather reg   */
void    sysDcrDmasg1Set(UINT32);

UINT32  sysDcrDmasg2Get(void);              /* DMA Chn 2 scatter/gather reg   */
void    sysDcrDmasg2Set(UINT32);

UINT32  sysDcrDmasg3Get(void);              /* DMA Chn 3 scatter/gather reg   */
void    sysDcrDmasg3Set(UINT32);

/* ------------------------ General Registers ------------------------- */
UINT32  sysDcrDmasrGet(void);               /* DMA Status register            */
void    sysDcrDmasrSet(UINT32);

UINT32  sysDcrDmasgcGet(void);              /* DMA Scatter/Gather cmd reg     */
void    sysDcrDmasgcSet(UINT32);

UINT32  sysDcrDmaslpGet(void);              /* DMA Sleep Mode reg             */
void    sysDcrDmaslpSet(UINT32);

UINT32  sysDcrDmapolGet(void);              /* DMA Polarity reg               */
void    sysDcrDmapolSet(UINT32);

#endif /* _ASMLANGUAGE */

#ifdef __cplusplus
    }
#endif

#endif  /* INCdmadcrh */
