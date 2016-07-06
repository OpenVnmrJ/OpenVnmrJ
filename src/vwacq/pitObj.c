/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
static char *sccsID(){
    return "pitObj.c Copyright(c)1994-1996 Varian";
}
/* 
 */

#include <vxWorks.h>
#include <vme.h>
#include "logMsgLib.h"
#include "pitObj.h"

/*
 * IPAC section
 * NOTE: Refer to manual for MV162: MVME162LX Embedded Controller
 *	 Programmer's Reference Guide, Chapter 5: IPIC.
 */
/* the PPC has to use the carrier - by default the
   162 uses the on-board carrier -- define IPC_CARRIER if you want to 
   force the carrier */
#if (CPU == PPC603)
#define IPC_CARRIER
#endif

#ifndef IPC_CARRIER
#define IPIC_BASE ((Reg8 *)(0xfff58000))
#define IPIC_CSR  ((Reg8 *)(0xfffbc000))

#else
#define GS_CARRIER  (0x6000)
Reg8 *IPIC_BASE=NULL;
#endif


IPAC *
ipacMv162Create(IpacSocket socket)
{
    int i;
    char *str;
    char *bytadr;
    char tstchr;
    IPAC *ipac;
    
#ifdef IPC_CARRIER
    sysBusToLocalAdrs(VME_AM_SUP_SHORT_IO,GS_CARRIER,&IPIC_BASE);
#endif
    
#ifndef IPC_CARRIER
    for (i=0, bytadr=(char *)IPIC_CSR; i<0x20; i++){
	if (vxMemProbe(bytadr, VX_READ, 1, &tstchr) == ERROR){
	    printf("ipacMv162Create(): IPIC chip not found\n");
	    return NULL;
	}
    }
#else
    bytadr= (char *) (IPIC_BASE + socket * 0x100);
    if (vxMemProbe(bytadr, VX_READ, 1, &tstchr) == ERROR){
	    return NULL;
	}
#endif 
    /* succeeded fill out the structure */
    ipac = (IPAC *)malloc(sizeof(IPAC));
    if (!ipac){
	errLogSysRet(LOGIT, debugInfo,
		     "ipacMv162Create(): malloc failed:");
	return NULL;
    }
    ipac->host = IPAC_HOST_MV162;
    ipac->socket = socket;
    ipac->ioaddr = IPIC_BASE + socket * 0x100;
    ipac->idaddr = ipac->ioaddr + 0x80;

    /* Check for presence of Industry Pack */
    /* Validate CSR addresses - 162 on board....*/
    /* Validate I/O and ID addresses */
    for (i=0, bytadr=(char *)ipac->ioaddr; i<0xc0; i++){
	if (vxMemProbe(bytadr, VX_READ, 1, &tstchr) == ERROR){
	    printf("ipacMv162Create(): Industry Pack I/O space not found\n"); 
            free(ipac);
	    return NULL;
	}
    }

    /* Check ID in the Industry Pack PROM */
    if (strcmp((str=ipacId(ipac)), "IPAC") != 0){
	printf("ipacMv162Create(): Industry Pack ID invalid: \"%s\"\n", str);
        ipacDelete(ipac);
	return NULL;
    }
   
    return ipac;
}

void
ipacMv162IntEnable(IPAC *ipac, IpacInt whichInt, int level, int polarity)
{
    Reg8 *intReg;
    unsigned char cmd;
#ifndef IPC_CARRIER
    intReg = IPIC_CSR + 0x10 + ipac->socket * 2 + whichInt;
    cmd = level & 0x07;		/* Set interrupt level bits */
    if (polarity){
	cmd |= 0x80;		/* Set polarity bit */
    }
    cmd |= 0x58;		/* Int enable bit set */
    *intReg = cmd;
#else
   return;
#endif
}

void
ipacMv162IntDisable(IPAC *ipac, IpacInt whichInt)
{
    Reg8 *intReg;
#ifndef IPC_CARRIER

    intReg = IPIC_CSR + 0x10 + ipac->socket * 2 + whichInt;
    *intReg = 0;
#else
    return;
#endif
}

void
ipacDelete(IPAC *ipac)
{
    ipacIntDisable(ipac, IPAC_INT_0);
    ipacIntDisable(ipac, IPAC_INT_1);
    free(ipac);
}

void
ipacIntEnable(IPAC *ipac, IpacInt whichInt, int level, int polarity)
{
    switch (ipac->host){
      case IPAC_HOST_MV162:
	ipacMv162IntEnable(ipac, whichInt, level, polarity);
	break;
      default:
	break;
    }
}

void
ipacIntDisable(IPAC *ipac, IpacInt whichInt)
{
    switch (ipac->host){
      case IPAC_HOST_MV162:
	ipacMv162IntDisable(ipac, whichInt);
	break;
      default:
	break;
    }
}

char *
ipacId(IPAC *ipac)
{
    int i;
    static char id[5];

    for (i=0; i<4; i++){
	id[i] = *(ipac->idaddr + 2 * i + 1);
    }
    id[4] = '\0';
    return id;
}

unsigned long
ipacModel(IPAC *ipac)
{
    unsigned long rtn;

    rtn = (ipac->idaddr[0x09] << 16 /* Mfg. ID */
	   | ipac->idaddr[0x0b] << 8 /* Model # */
	   | ipac->idaddr[0x0d]); /* Revision */
    return rtn;
}


/*
 * PI/T section
 * NOTE: Refer to manual: Motorola data sheet for MC68230
 */

/* Register Definitions */
/* "Port" Registers */
#define PGCR(pit) ((Reg8 *)(pit->ioaddr + 0x01)) /* General Control Reg */
#define PSRR(pit) ((Reg8 *)(pit->ioaddr + 0x03)) /* Service Request Reg */
#define PADDR(pit) ((Reg8 *)(pit->ioaddr + 0x05)) /* A Data Direction Reg */
#define PBDDR(pit) ((Reg8 *)(pit->ioaddr + 0x07)) /* B Data Direction Reg */
#define PCDDR(pit) ((Reg8 *)(pit->ioaddr + 0x09)) /* C Data Direction Reg */
#define PIVR(pit) ((Reg8 *)(pit->ioaddr + 0x0B)) /* Interrupt Vector Reg */
#define PACR(pit) ((Reg8 *)(pit->ioaddr + 0x0D)) /* A Port Control Reg */
#define PBCR(pit) ((Reg8 *)(pit->ioaddr + 0x0F)) /* B Port Control Reg */
#define PADR(pit) ((Reg8 *)(pit->ioaddr + 0x11)) /* A Data Reg */
#define PBDR(pit) ((Reg8 *)(pit->ioaddr + 0x13)) /* B Data Reg */
#define PAAR(pit) ((Reg8 *)(pit->ioaddr + 0x15)) /* A Alternate Data Reg */
#define PBAR(pit) ((Reg8 *)(pit->ioaddr + 0x17)) /* B Alternate Data Reg */
#define PCDR(pit) ((Reg8 *)(pit->ioaddr + 0x19)) /* C Data Reg */
#define PSR(pit) ((Reg8 *)(pit->ioaddr + 0x1B))	/* Status Reg */
/* "Timer" Registers */
#define TCR(pit) ((Reg8 *)(pit->ioaddr + 0x21))	/* Control Reg */
#define TIVR(pit) ((Reg8 *)(pit->ioaddr + 0x23)) /* Interrupt Vector Reg */
#define CPRH(pit) ((Reg8 *)(pit->ioaddr + 0x27)) /* Preload Reg - High */
#define CPRM(pit) ((Reg8 *)(pit->ioaddr + 0x29)) /* Preload Reg - Middle */
#define CPRL(pit) ((Reg8 *)(pit->ioaddr + 0x2B)) /* Preload Reg - Low */
#define CRH(pit) ((Reg8 *)(pit->ioaddr + 0x2F))	/* Counter Reg - High */
#define CRM(pit) ((Reg8 *)(pit->ioaddr + 0x31))	/* Counter Reg - Middle */
#define CRL(pit) ((Reg8 *)(pit->ioaddr + 0x33))	/* Counter Reg - Low */
#define TSR(pit) ((Reg8 *)(pit->ioaddr + 0x35))	/* Status Reg */

/* Defines for PGCR bits (manual page 4-2) */
#define PGCR_MODE_SET(x) (((x) & 0x3) << 6)
#define PGCR_H34_ENB 0x20
#define PGCR_H12_ENB 0x10
#define PGCR_H4_ASSERTED_HIGH 0x8
#define PGCR_H3_ASSERTED_HIGH 0x4
#define PGCR_H2_ASSERTED_HIGH 0x2
#define PGCR_H1_ASSERTED_HIGH 0x1

/* Defines for PSRR bits (manual page 4-3) */
#define PSRR_DMA_BITS 0x60
#define PSRR_DMA_PORT_A 0x40
#define PSRR_DMA_PORT_B 0x60
#define PSRR_INT_BITS 0x18
#define PSRR_INT_AUTOVECTORED 0x08
#define PSRR_INT_VECTORED 0x18;
#define PSRR_INT_PRIORITY_BITS 0x07 /* Order of priority for H1-H4 */

/* Defines for PACR: mode 0, submode 1X (manual page 3-7) */
#define PACR_SUBMODE_BITS 0xC0
#define PACR_SUBMODE_1X 0x80
#define PACR_H2_CTL_BITS 0x38
#define PACR_H2_CTL_INPUT 0
#define PACR_H2_CTL_OUT_NEG 0x20
#define PACR_H2_CTL_OUT_POS 0x08
#define PACR_H2_INT_ENB 0x04
#define PACR_H1_INT_ENB 0x02

/* Defines for PBCR: mode 0, submode 1X (manual page 3-7) */
#define PBCR_SUBMODE_BITS 0xC0
#define PBCR_SUBMODE_1X 0x80
#define PBCR_H4_CTL_BITS 0x38
#define PBCR_H4_CTL_INPUT 0
#define PBCR_H4_CTL_OUT_NEG 0x20
#define PBCR_H4_CTL_OUT_POS 0x08
#define PBCR_H4_INT_ENB 0x04
#define PBCR_H3_INT_ENB 0x02

/* Defines for PSR bits (manual page 4-6) */
#define PSR_HANDSHAKE_LEVELS_GET(x) (((x) & 0xF0) >> 4)
#define PSR_HANDSHAKE_SENSE_GET(x) ((x) & 0x0F)


PIT *
pitCreate(IPAC *ipac,	/* Ipack socket the PI/T is mounted in */
          PitChip chip)		/* Which chip to use */
{
    PIT *pit;
    unsigned long id;

    pit = (PIT *)malloc(sizeof(PIT));
    if (!pit){
	errLogSysRet(LOGIT, debugInfo, "pitCreate(): malloc failed:");
	return NULL;
    }
    pit->ipac = ipac;
    pit->chip = chip;
    pit->ioaddr = ipac->ioaddr + chip * 0x40;

    id = ipacModel(pit->ipac) & 0x00ff00;
    if (id != 0x2300 && id != 0x2400){
	printf("pitCreate(): Industry Pack is not a PI/T\n");
	pitDelete(pit);
	return NULL;
    }
    return pit;
}

void
pitDelete(PIT *pit)
{
    ipacDelete(pit->ipac);
    free(pit);
}

void
pitDirectionSet(PIT *pit, PitReg datareg, int bits)
{
    Reg8 *cmdreg;

    switch (datareg){
      case PIT_REG_A:
	cmdreg = PADDR(pit);
	break;
      case PIT_REG_B:
	cmdreg = PBDDR(pit);
	break;
      case PIT_REG_C:
	cmdreg = PCDDR(pit);
	break;
      default:
	errLogRet(LOGIT, debugInfo, "pitDirectionSet(): Illegal register\n");
	return;
    }
    *cmdreg = (Reg8)(bits & 0xff);
}

unsigned char
pitRead(PIT *pit, PitReg whichReg)
{
    unsigned char rtn;

    switch (whichReg){
      case PIT_REG_A:
	rtn = *PADR(pit);
	break;
      case PIT_REG_B:
	rtn = *PBDR(pit);
	break;
      case PIT_REG_C:
	rtn = *PCDR(pit);
	break;
      case PIT_REG_H:
	rtn = *PSR(pit);
	rtn = PSR_HANDSHAKE_LEVELS_GET(rtn);
	break;
      default:
	errLogRet(LOGIT, debugInfo, "pitRead(): Illegal register\n");
	return 0;
    }
    return rtn;
}


void
pitWrite(PIT *pit, PitReg whichReg, int value)
{
    Reg8 val;

    val = (Reg8)(value & 0xff);
    switch (whichReg){
      case PIT_REG_A:
	*PADR(pit) = val;
	break;
      case PIT_REG_B:
	*PBDR(pit) = val;
	break;
      case PIT_REG_C:
	*PCDR(pit) = val;
	break;
      default:
	errLogRet(LOGIT, debugInfo, "pitWrite(): Illegal register\n");
	return;
    }
}

void
pitInterruptEnable(PIT *pit,
		   PitInterrupt whichInt,
		   int level,	/* Priority level: 0-7 */
		   int polarity) /* 0 for active low; 1 for active high */
  /*
   * Enables interrupts on the "whichInt" handshake line.
   * Interrupt occurs when the line transitions to "polarity".
   * As a side effect, clears any pending interrupts on the "whichInt"
   * line and on its companion (H1/H2 and H3/H4 are companions.)
   * *** Assumes the interrupt vector has already been set. ***
   * Assumes 68230 is in mode 0, but looks like it should work in
   * other modes too.
   * 1. Disable H12 or H34 in PGCR and set requested polarity.
   * 2. If H2 or H4, set line for input in PACR or PBCR.
   * 3. Enable interrupt in PACR or PBCR.
   * 4. Enable H12 or H34 in PGCR.
   * 5. Clear/Enable interrupt in the parent IPAC object.
   */
{
    int hEnbBit;
    int polarityBit;
    Reg8 *reg;
    int ioBits;
    int iEnbBit;
    IpacInt ipacInt;

    switch (pit->chip){
      case PIT_CHIP_X:
	ipacInt = IPAC_INT_0;
	break;
      case PIT_CHIP_Y:
	ipacInt = IPAC_INT_1;
	break;
    }

    switch (whichInt){
      case PIT_INT_H1:
	hEnbBit = PGCR_H12_ENB;
	polarityBit = PGCR_H1_ASSERTED_HIGH;
	reg = PACR(pit);
	ioBits = 0;
	iEnbBit = PACR_H1_INT_ENB;
	break;
      case PIT_INT_H2:
	hEnbBit = PGCR_H12_ENB;
	polarityBit = PGCR_H2_ASSERTED_HIGH;
	reg = PACR(pit);
	ioBits = PACR_H2_CTL_BITS;
	iEnbBit = PACR_H2_INT_ENB;
	break;
      case PIT_INT_H3:
	hEnbBit = PGCR_H34_ENB;
	polarityBit = PGCR_H3_ASSERTED_HIGH;
	reg = PBCR(pit);
	ioBits = 0;
	iEnbBit = PBCR_H3_INT_ENB;
	break;
      case PIT_INT_H4:
	hEnbBit = PGCR_H34_ENB;
	polarityBit = PGCR_H4_ASSERTED_HIGH;
	reg = PBCR(pit);
	ioBits = PBCR_H4_CTL_BITS;
	iEnbBit = PBCR_H4_INT_ENB;
	break;
    }

    *PGCR(pit) &= ~(hEnbBit | polarityBit); /* Clear enable and polarity */
    if (polarity){
	*PGCR(pit) |= polarityBit; /* Set polarity if requested */
    }
    *reg &= ~ioBits;		/* Set H2 or H4 for input */
    *reg |= iEnbBit;		/* Enable interrupt */
    *PSRR(pit) &= ~PSRR_INT_BITS; /* Set vectored interrupt mode */
    *PSRR(pit) |= PSRR_INT_VECTORED;
    *PGCR(pit) |= hEnbBit;	/* Enable handshake line */
    ipacIntEnable(pit->ipac, ipacInt, level, polarity);
}

void
pitIntReset(PIT *pit, PitInterrupt pInt)
{
    int enbBit;
    IpacInt ipacInt;

    switch (pInt){
      case PIT_INT_H1:
      case PIT_INT_H2:
	enbBit = PGCR_H12_ENB;
	break;
      case PIT_INT_H3:
      case PIT_INT_H4:
	enbBit = PGCR_H34_ENB;
	break;
    }
    switch (pit->chip){
      case PIT_CHIP_X:
	ipacInt = IPAC_INT_0;
	break;
      case PIT_CHIP_Y:
	ipacInt = IPAC_INT_1;
	break;
    }
    *PGCR(pit) &= ~enbBit;	/* Clear interrupt on device ... */
    ipacIntDisable(pit->ipac, ipacInt);	/* and on IPAC interface */
}

void
pitIntVectorSet(PIT *pit, int intVector)
{
    *PIVR(pit) = intVector;
}

void
pitSubmodeSet(PIT *pit, int submode)
{
    unsigned char gcr;

    gcr = *PGCR(pit);
    *PGCR(pit) &= ~(PGCR_H12_ENB | PGCR_H34_ENB); /* Disable H bits */
    *PACR(pit) = submode << 6;
    *PBCR(pit) = submode << 6;
    *PGCR(pit) = gcr;		/* Restore GCR state */
}
