/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <stdio.h>
#include "group.h"
#include "acodes.h"
#include "rfconst.h"
#include "acqparms.h"

extern int bgflag;
/*----------------------------------------------------------------------
|
|   Intended for use in pulse programming.
|   Observe power is set to default value (tpwrr) prior to start of first scan.
|   127 gives maximum power (minimum attenuation).
|   0 gives minimum power (maximum attenuation).
|   Once observepower is used to set a transmitter power, that power setting
|   remains in effect until changed by another observepower.
|   Time duration = 2 ap bus words + ap bus dead time.
|   Ok in a hardware loop.
|-----------------------------------------------------------------------*/
observepower(reqpower)
double	reqpower;
{
   setrfattenuation(reqpower,PATOBSCH);
}

/*-----------------------------------------------------------------------*/

/*----------------------------------------------------------------------
|
|   Intended for use in pulse programming.
|   Decoupler power is set to default value (dpwrr) prior to start of 1st scan.
|   127 gives maximum power (minimum attenuation).
|   0 gives minimum power (maximum attenuation).
|   Once decpower is used to set a decoupler power, that power setting
|   remains in effect until changed by another decpower.
|   Time duration = 2 ap bus words + ap bus dead time.
|   Ok in a hardware loop.
|-----------------------------------------------------------------------*/
decouplepower(reqpower)
double	reqpower;
{
   setrfattenuation(reqpower,PATDECCH);
}

/*-----------------------------------------------------------------------*/

setrfattenuation(ampval, device)
int 	device;
double 	ampval;
{
int 	i,apaddr;
int	modulatorbd,TRANSorDEC;	

   if ( device == PATOBSCH ) TRANSorDEC =0 ;
   if ( device == PATDECCH ) TRANSorDEC =1 ;

   if (rftype[TRANSorDEC] == 'M' || rftype[TRANSorDEC] == 'm')
      modulatorbd =1;
   else
      {		
         text_error("CANT ATTENUATE NON-VIS RF BOARD \n"); 
         text_error("PLEASE CHECK 'rftype\?' \n"); 
	 abort(1); 
      }

   if ((device == PATOBSCH && modulatorbd) || (device == PATDECCH && modulatorbd)) 

    {
	okinhwloop();
	if ( ampval > 127.0 )	 i = 0;

	else if (ampval < 0.0 )  i = 127;

	else
	   i = 127 - ampval;

	apaddr = PATAPBASE + device;
	   putcode(RFSHPAMP);
	      putcode(apaddr);
	   putcode(i);
	curfifocount = curfifocount + 3;
	if (bgflag)
	   fprintf(stderr,"setrfattenuation(): apaddr = %x, Attenu. = %d\n",
		apaddr, i); 
     }
  else 
      {		
         text_error("CANT ATTENUATE UNKNOWN RF CHANNEL \n"); 
	 abort(1); 
      }
}

