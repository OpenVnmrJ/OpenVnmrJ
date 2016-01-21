/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <stdio.h>
#include "acqparms.h"

extern double getval();
extern double sign_add();

extern int  bgflag;	/* debug flag */
extern int  newacq;

/*-------------------------------------------------------------------
|
|	G_initval()/2 
|	initialize a real time variable v1-v14, (contain within the Acq code) 
|	The double real is rounded up and made integer.
|				Author Greg Brissey  6/23/86
|	initval() moved to macros.h for typeless arguments.
|
|   Modified   Author     Purpose
|   --------   ------     -------
|   6/15/89   Greg B.     1. Use new global parameters to calc lc offsets 
+------------------------------------------------------------------*/
G_initval(value,index)
double value;		/* value to set variable to */
codeint    index;		/* offset into acq code of variable */
{
   value = (value > 0.0) ? (value + 0.5) : (value - 0.5); /* round up value */
   if (bgflag)
       fprintf(stderr,"initval(): value: %8.1lf , index: %d \n",value,index);
   /* --- force value to be in limits of integer value --- */
   if (newacq)
   {
   	if (value > 2147483647.0)
           value = 2147483647.0;
   	else
           if (value < -2147483648.0)
	   	value = -2147483648.0;
   }
   else
   {
   	if (value > 32767.0)
           value = 32767.0;
   	else
           if (value < -32768.0)
	   	value = -32768.0;
   }
   set_acqvar(index,(int) sign_add(value,0.0005) );
   notinhwloop("initval");	/* not allowed in hardware loop  */
}
