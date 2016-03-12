/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* VME_EEprom.c 11.1 07/09/07 - EEprom - VME Object Source Modules */
/*
*/
/* Automation module to read/write EEprom */

#include <vxWorks.h>
#include <stdlib.h>
#include <msgQLib.h>
#include <vme.h>
#include "hardware.h"
#include "EEpromSupport.h"

#define LONG	4

extern int EEmapSetup();

int EEdiagFlag = 1;

int EEpromRead(unsigned long *EEpromAdr, int EEpromBytes, char *EEpromData)
{
register unsigned long *fetch_addr;
unsigned long *eeprom_addr;
int i;
long loopcnt;
unsigned long eewrite = 0L;
 
#if (CPU != MC68040)

int ppcLockMask;
int ppcEEpromAdrEnabled;

ppcEEpromAdrEnabled = 0;     /* accessing std A24 space, default */

#endif

  /* ------ Translate Bus address to CPU Board local address ----- */
/* maybe later.
  if (sysBusToLocalAdrs(VME_AM_STD_USR_DATA,
          ((long)AUTO_BASE_ADR & 0xffffff),&fetch_addr) == -1)
  {
    printf( "AutoEEpromRead: Can't Obtain Board Bus (0x%lx) to Local Address.",
          AUTO_BASE_ADR);
    return(ERROR);
  }
*/
	fetch_addr = EEpromAdr;
	if ( ((ulong_t) EEpromAdr & 0xF0000000L) == (ulong_t) 0xE0000000)	/* EEprom address space */
	{

#if (CPU == MC68040)

	   if(EEmapSetup() == ERROR) return(ERROR);
	    fetch_addr = EEpromAdr;

#else /* #if (CPU == PPC603) */

            /* strip off 0xE0000000 and obtain STD address space for PPC access to EEPROM */
	    if (sysBusToLocalAdrs(VME_AM_STD_USR_DATA,
          	    ((long)EEpromAdr & 0xffffff),&eeprom_addr) == -1)
  	    {
    	        return(ERROR);
  	    }

	    fetch_addr = eeprom_addr;
            /* printf("EEpromRead: EEPROM adrs: 0x%lx\n",fetch_addr); */

	    /* MVME2303 PPC */
	    /* enable the CS/CSR space for Diagnostic EEPROMS */
            /* shows up on A24 space - replaces standard A24 */
            /* since this mapping replaces the standard A24 space */
            /* ISRs should not be permitted to run during this time */
            /* thus the intLock() & intUnlock() calls    */

            /*    printf("Enable EEPROM AM code\n");  */
           ppcEEpromAdrEnabled = 1;   /* enabling EEPROM address space */
           ppcLockMask = intLock();
           sysUniverseCSCSR();

#endif

        }

        /* printf("EEPROM access address: 0x%lx\n",fetch_addr); */

        if ( vxMemProbe((short*) (fetch_addr), VX_WRITE, LONG, &eewrite) == ERROR)
        {

#if (CPU != MC68040) /* #if (CPU == PPC603) */

	    if (ppcEEpromAdrEnabled == 1)
	    {
               /* re-enable the standard A24 space */
               sysUniverseA24();
               intUnlock(ppcLockMask);
            }
#endif

            printf("\n ---- EEpromRead: EEPROM adrs: 0x%lx,  NOT accessible.\n\n",fetch_addr);

	    return(ERROR);
        }


	if(EEdiagFlag & 0x2) printf("EEpromRead: address = 0x%06lx\n",fetch_addr);

	/* place in read mode */
	*fetch_addr = VME_EEP_READ;

	for(i=0;i<EEpromBytes;i++)
	{
	  /* printf("%d, addr: 0x%lx, '%c' 0x%x\n",i,fetch_addr,*fetch_addr,*fetch_addr); */
	  *EEpromData++ = *fetch_addr++ & 0x000000ff;
	}

#if (CPU != MC68040) /* #if (CPU == PPC603) */
	if (ppcEEpromAdrEnabled == 1)
	{
           /* printf("Disable EEPROM AM code\n"); */
           /* re-enable the standard A24 space */
           sysUniverseA24();
           intUnlock(ppcLockMask);
        }
#endif

	if(EEdiagFlag & 0x2) printf("EEpromRead: chars = %d\n",i);
	return(i);

} /* end EEpromRead(...) */

void wait4EEp(int microseconds)	/* wait n us. */
{
int i,j;
volatile int k;

	for(i=0;i<=microseconds;i++)
	{
		for(j=0;j<=1000;j++) k=j;	/* this may or may not be 1 microsecond */
	}
	
}


int EEpromWrite(unsigned long *EEpromAdr, int EEpromBytes, char *EEpromData)
{
register unsigned long *fetch_addr;
register int BytesToDo;
char verify_Reg;
int trys;
char EEdata;

	if(EEmapSetup() == ERROR) return(ERROR);

	fetch_addr = EEpromAdr;
	BytesToDo = EEpromBytes;

	if(EEdiagFlag & 0x2)
		printf("EEpW: addr=0x%08x, bytes=%d\n",fetch_addr,BytesToDo);

	while(BytesToDo)
	{
		EEdata = *EEpromData++;
		trys = 0;
		if(EEdiagFlag & 0x4) printf("%02d ",BytesToDo);
		while(TRUE)
		{
			*fetch_addr = VME_EEP_WRITE;
			*fetch_addr = (long) EEdata;

			wait4EEp(10);	/* wait 10 us. */

			*fetch_addr = VME_EEP_VERIFY;

			wait4EEp(6);	/* wait 6 us. */

			verify_Reg = (char) ( *fetch_addr & 0x000000ffL);

			if(EEdiagFlag & 0x8) printf(" [%d,0x%02x,0x%02x]",trys,EEdata,verify_Reg);

			if(verify_Reg == EEdata)
			{
				if(EEdiagFlag & 0x8) printf("\n ! ");
				BytesToDo--;
				fetch_addr++;
				break;
			}
			else trys++;

			if(trys >= VME_EEP_LIMIT)return( (EEpromBytes - BytesToDo) );

		}	/* end while(TRUE) */

		if(EEdiagFlag & 0xC) printf(".");

	}	/* end while(EEpromBytes) */

	if(EEdiagFlag & 0xC) printf("\n");

	return( (EEpromBytes - BytesToDo) );

} /* end EEpromWrite(...) */

/* end VME_EEprom.c */
