/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* mapEEprom.c  11.1 07/09/07 - Routine that changes VMEchip2 map4 to read EEproms */
/* 
 */


#define _POSIX_SOURCE	/* defined when source is to be POSIX-compliant */
#include <vxWorks.h>
#include <stdlib.h>
#include <vme.h>
#include <iv.h>
#include <msgQLib.h>
#include "logMsgLib.h"
#include "hostAcqStructs.h"

/*
modification history
--------------------
6-30-95,gad  created 
*/

/* Exception Msges to Phandler, e.g. FOO, etc. */
extern EXCEPTION_MSGE HardErrorException;
extern EXCEPTION_MSGE GenericException;

/* defines for VMEchip2 */
#define VME2_MEA1_MSA1    ((volatile unsigned long* const) 0xfff40014)
 /* Master Ending addr 1, Starting addr 1 */
#define VME2_MEA2_MSA2    ((volatile unsigned long* const) 0xfff40018)
 /* Master Ending addr 2, Starting addr 2 */
#define VME2_MEA3_MSA3    ((volatile unsigned long* const) 0xfff4001c)
 /* Master Ending addr 3, Starting addr 3 */
#define VME2_MEA4_MSA4    ((volatile unsigned long* const) 0xfff40020)
 /* Master Ending addr 4, Starting addr 4 */
#define VME2_MATA4_MATS4  ((volatile unsigned long* const) 0xfff40024)
 /* Master addr tran addr 1, Starting addr 1 */
#define VME2_AM_ATTR      ((volatile unsigned long* const) 0xfff40028)
 /* Master Address modifier/attribute regs */
#define VME2_CTRL_REG     ((volatile unsigned long* const) 0xfff4002c)
 /* Control Register bits 19-16 only */

/* defines for AM 2f operation */
#define VME2_M1_DATA      0xdfff0040L /* remove e000-efff from map */
#define VME2_M4_DATA      0xefffe000L /* map e000-efff to map4 */
#define VME2_MAAT4_DATA   0x0000ff00L /* force 00 instead of e0 */
#define VME2_M4_AM_DATA   0x2f000000L /* OR in this data for map4 AM code */
#define VME2_M4_AM_MASK   0xff000000L /* mask for map4 AM code */
#define VME2_M4_ENABLE    0x00080000L /* OR in bit 19 to enable map4 */

/*-------------------------------------------------------------
|
|  VMEchip2 definitions:
|  14 Master ending address 1               Master starting address 1
|  18 Master ending address 2               Master starting address 2
|  1c Master ending address 1               Master starting address 1
|  20 Master ending address 1               Master starting address 1
|  24 Master Address Translation address 4  Master Address translation select 4
|  28 Master AM 4    28 Master AM 3        28 Master AM 4    28 Master AM 4
|  2c bit 19 map4 enable.
|
|  Currently map1 is set for 0040 - efff
|  Currently map2 is set for f000 - f0ff
|  Currently map3 and map4 are unused.
|
|  For AM 2f for access to EEproms:
|  Change map1 to 0040 - dfff (loc 0xfff4014 = dfff0040).
|  Leave  map2 and map3 as is
|  Add map4 from e000 - efff (loc 0xfff40020 = efffe000).
|  Add Translation address for map4 = 0 and translation select = ff00
|       (loc 0xfff40024 = 0x0000ff00).
|  Add AM4 = 2f (OR 0x2f000000 to loc 0xfff40028).
|  Add Master enable (OR 0x00080000 to loc 0xfff4002c).
|
+--------------------------------------------------------------*/

/*-------------------------------------------------------------
|
|  Internal Functions
|
+--------------------------------------------------------------*/
/*-------------------------------------------------------------
| 
+--------------------------------------------------------------*/
/***************************************************************
*
* EEmapSetup
*
*   EEmapsetup insures map 4 at 0xe0xxxxxx maps to VME bus at 0x00xxxxxx.
*     using address modifier 0x2f.
*
* RETURNS:
*  TRUE if already set.
*  OK if successful set.
*  ERROR is set if unsuccessful.
*
* NOMANUAL
*/

/*   This code is only valid for motorla mvme162 vmechip2 */
#if (CPU == MC68040)

int EEmapSetup()
{

   /* unsigned long CR_REG; */

   /*
   printf("present: VME2_MEA1_MSA1 = 0x%lx, VME2_MEA4_MSA4 = 0x%lx, VME2_MATA4_MATS4 = 0x%lx\n",
			(*VME2_MEA1_MSA1), (*VME2_MEA4_MSA4),  (*VME2_MATA4_MATS4));
   printf("tobe to: VME2_MEA1_MSA1 = 0x%lx, VME2_MEA4_MSA4 = 0x%lx, VME2_MATA4_MATS4 = 0x%lx\n",
			(VME2_M1_DATA), (VME2_M4_DATA),  (VME2_MAAT4_DATA));

   printf("present VME2_AM_ATTR (0x%lx) & VME2_M4_AM_MASK (0x%lx) = 0x%lx \n",
			*VME2_AM_ATTR,VME2_M4_AM_MASK, (*VME2_AM_ATTR & VME2_M4_AM_MASK));
   printf("present VME2_CTRL_REG (0x%lx) & VME2_M4_ENABLE (0x%lx) = 0x%lx \n",
			*VME2_CTRL_REG,VME2_M4_ENABLE, (*VME2_CTRL_REG & VME2_M4_ENABLE));
   */


	/* check if in virgin condition, if so, set it up else
			check if already set up and exit(TRUE) else  exit w/ERROR */


	if ( ( *VME2_MEA1_MSA1 == 0xefff0040 ) &&

	   ( *VME2_MEA4_MSA4 == 0x00000000 ) &&

	   ( *VME2_MATA4_MATS4 == 0x00000000) &&

	   ( ( *VME2_AM_ATTR & VME2_M4_AM_MASK)  == 0x00000000 ) &&

	   ( ( *VME2_CTRL_REG & VME2_M4_ENABLE) == 0x00000000 ) )
	{
		*VME2_MEA1_MSA1 = VME2_M1_DATA;

		*VME2_MEA4_MSA4 = VME2_M4_DATA;

		*VME2_MATA4_MATS4 = VME2_MAAT4_DATA;

		*VME2_AM_ATTR |= VME2_M4_AM_DATA;

		*VME2_CTRL_REG |= VME2_M4_ENABLE;

		return(OK);
	}

	else if ( ( *VME2_MEA1_MSA1 == VME2_M1_DATA) &&

		( *VME2_MEA4_MSA4 == VME2_M4_DATA ) &&

		( *VME2_MATA4_MATS4 == VME2_MAAT4_DATA ) &&

		( ( *VME2_AM_ATTR & VME2_M4_AM_MASK ) == VME2_M4_AM_DATA )  &&

	        ( ( *VME2_CTRL_REG & VME2_M4_ENABLE ) == VME2_M4_ENABLE ) ) 
       {

		    return(TRUE);

                /* tried reset all register but it appears only to work after
		   a reset of the board. Just reseting the registers
		   after a  reboot causes an address trap when access EEPROM addresses
                   Oh well, I tried. Not enough time to look into it further.
			gmb 4/24/01
                */

        }
	else
	{
		printf("\nEEmapSetup: EEPROM address mapping cannot be configured.\n");
		/*
		printf("\nEEmapSetup: Map is incorrect\n");
		printf("   M1=0x%08x SB 0x%08x\n",*VME2_MEA1_MSA1,VME2_M1_DATA);
		printf("   M4=0x%08x SB 0x%08x\n",*VME2_MEA4_MSA4,VME2_M4_DATA);
		printf(" MAT4=0x%08x SB 0x%08x\n",*VME2_MATA4_MATS4,VME2_MAAT4_DATA);
		printf(" MAM4=0x%08x SB 0x%08x\n",(*VME2_AM_ATTR & VME2_M4_AM_MASK),VME2_M4_AM_DATA);
		printf(" M4EN=0x%08x SB 0x%08x\n",(*VME2_CTRL_REG & VME2_M4_ENABLE),VME2_M4_ENABLE);
		*/
		return(ERROR);
	}
}

#else   /* (CPU == MC68040) */

  /* for PPC varient just return OK, EEPROM mapping is done via 
     sysUniverseCSCSR()  enable the CS/CSR space for Diagnostic EEPROMS
      			 shows up on A24 space - replaces standard A24
     sysUniverseA24()    re-enable the standard A24 space
  */

  int EEmapSetup()
  {
    return(OK);
  }
#endif  /* (CPU == MC68040) */
/* end of mapEEprom.c */
