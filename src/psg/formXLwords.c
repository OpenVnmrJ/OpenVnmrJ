/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <stdio.h>
#include "rfconst.h"
#include "acqparms.h"

extern int bgflag;		/* debug flag */
extern int curfifocount;

/*-----------------------------------------------------------------------
|
|	formXLwords()/2
|	forms the XL interface words to go out on apbuss to control RF
|
|        PTS160 does not have a 100MHz decade therefore the 
|	  10MHz decade is programmed in HEX not BCD
|
|				Author Greg Brissey  5/13/86
|   Modified   Author     Purpose
|   --------   ------     -------
|   6/22/89    Greg B.	   1. pointer where calc apbus words are placed.
+------------------------------------------------------------------------*/
formXLwords(value,num,digoffset,device,words)
double value;
int num;		/* number of XL word needed for device */
int digoffset;		/* register offset on XL interface for device */
int device;		/* address of device on XL interface */
int *words;
{
    int ival;
    int digit;
    int mdigit;
    int bcd;

    ival = (int) (value + 0.0005);	/* not rounding, making sure 30.0=30*/

    if (bgflag)
       fprintf(stderr,"formXLwords(): value = %d \n",ival);

    for(digit = (0 + digoffset); digit < (digoffset + num); digit++)
    {
	/* --- special fix to allow VXR500 dmf > 9900Hz --- */
	if ((digit == 16) && (device == DMFDEV))
	    mdigit = 11;
	else
	    mdigit = digit;
	bcd = ival % 10;	/* BCD */
	bcd = 0x8000 | (device << DEVPOS) | (mdigit << DIGITPOS) | bcd ;
        *words++ = bcd;
	/*putcode((codeint) bcd);*/
	ival /= 10;
	/* curfifocount++; */
    }
}
/*-----------------------------------------------------------------------
|
|	formXL16words()/2
|	forms the XL interface words to go out on apbuss to control RF
|	used mainly for XL automated decoupler power attenuator
|
|   Modified   Author     Purpose
|   --------   ------     -------
|   6/22/89    Greg B.	   1. pointer where calc apbus words are placed.
+------------------------------------------------------------------------*/
formXL16words(value,num,digoffset,device,words)
double value;
int num;		/* number of XL word needed for device */
int digoffset;		/* register offset on XL interface for device */
int device;		/* address of device on XL interface */
int *words;
{
    int ival;
    int digit;
    int bcd;

    ival = (int) (value + 0.0005);	/* not rounding, making sure 30.0=30*/

    if (bgflag)
       fprintf(stderr,"formXL16words(): value = %d \n",ival);

    for(digit = (0 + digoffset); digit < (digoffset + num); digit++)
    {
	bcd = ( ~(ival % 16) ) & 0x0f;
	bcd = 0x8000 | (device << DEVPOS) | (digit << DIGITPOS) | bcd;
	/* putcode((codeint) bcd); */
        *words++ = bcd;
	ival /= 16;
	/* curfifocount++; */
    }
}

/*-----------------------------------------------------------------------
|
|	formPTSwords()/2
|	forms the XL interface words to go out on apbuss to control RF
|
|        PTS160 does not have a 100MHz decade therefore the 
|	  10MHz decade is programmed in HEX not BCD
|
|				Author Greg Brissey  7/12/89
+------------------------------------------------------------------------*/
formPTSwords(value,num,digoffset,ptstype,words)
double value;
int num;		/* number of XL word needed for device */
int digoffset;		/* register offset on XL interface for device */
int ptstype;		/* PTS type 160,200,250,400,500, etc */
int *words;
{
    int ival;
    int digit;
    int bcd;

    ival = (int) (value + 0.0005);	/* not rounding, making sure 30.0=30*/

    if (bgflag)
       fprintf(stderr,"formPTSwords(): value = %d \n",ival);

    for(digit = (0 + digoffset); digit < (digoffset + num); digit++)
    {
       /* --- test for PTS160 case --- */
       if ( (ptstype == 160) && (digit >= (digoffset + (num-1))) )
       {
          bcd = ival % 16;        /* Hex not BCD */
          ival /= 16;
       }   
       else
       {  
	  bcd = ival % 10; /* BCD */
          ival /= 10;
       }  
       bcd = 0x8000 | (PTSDEV << DEVPOS) | (digit << DIGITPOS) | bcd ;
       *words++ = bcd;
    }
}
