/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <stdio.h>
#include "acodes.h"
#include "acqparms.h"

extern int curfifocount;
extern int HSlines;
extern  int fifolpsize;	/* size in words of looping fifo */

/*delays pulse sequence execution until count number of
   external clocks have been received
   count should be an integer 0 < count <4096
   count > 4095 gives count = 4095
   count < 1 gives no xgate al all*/

/* external clock is a TTL level normally high (low-going
      pulse) connected to XL interface board, 
      New output board has connector, not XL interface board */

/*  Modified for New Output board 8/8/88   Greg Brissey   */

xgate(count)
codeint count;
{
   int clocktix;
   extern int okinhwloop();

   if (count > 0)
   {
       if (fifolpsize < 65) /* determine output board type */
       {
	   if (count > 4095)
	      clocktix = 4095;	/*count larger than 12 bits*/
	   else
	      clocktix = count;
       }
       else
       {
	   if (count > 4096)	/* remember count of 4095 gives 4096 counts */
	      clocktix = 4095;	/*count larger than 12 bits*/
	   else
	      clocktix = count - 1;  /* new output board count of 0 equals 1 */
       }
       putcode(EVENT1);
       putcode((codeint)clocktix);
       putcode((codeint)HSlines);
       curfifocount++;
   }
   return;
}

