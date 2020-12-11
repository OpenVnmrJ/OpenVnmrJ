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
#include "apdelay.h"
#include "rfconst.h"
#include "macros.h"
/*-------------------------------------------------------------------------
|
|	hsdelay(time)
|	Forms a homospoil delay if homospoil is set (status)
|	else just a delay is created
|
|				Author Greg Brissey  5/19/86
+------------------------------------------------------------------------*/

extern int homospoil_bit;
extern int  bgflag;	/* debug flag */
extern int  newacq;	/* inova flag */
extern int putcode();
extern void HSgate(int ch, int state);

#define ERROR 1
#define FALSE 0
#define OK 0
#define TRUE 1

int hsdelay(double time)
{
    int index;

    index = statusindx;
    if (statusindx >= hssize) 
	index = hssize - 1;

    if (bgflag)
    {
	fprintf(stderr,
        "hsdelay(): hst=%g,statusindx= %d, index= %d, hs[%d]= %c, hs='%s'\n",
	  hst,statusindx,index,index,hs[index],hs);
    }

    if ((hs[index] == 'y') || (hs[index] == 'Y'))
    {
       time = time - hst;
       if (time < 0.0)
       {
          if (ix == 1)
          {
             fprintf(stdout, " \n");
             fprintf(stdout, "Delay time is less than homospoil time (hst).\n");
             fprintf(stdout, "No homospoil pulse produced.\n");
             fprintf(stdout, " \n");
          }
          time = time + hst;
       }
       else
       {
	  if (newacq)
	  {
		putcode(IHOMOSPOIL);
		putcode(TRUE);
	  }
	  else
          	HSgate(homospoil_bit,TRUE);
          G_Delay(DELAY_TIME, hst, 0);
	  if (newacq)
	  {
		putcode(IHOMOSPOIL);
		putcode(FALSE);
	  }
	  else
          	HSgate(homospoil_bit,FALSE);
       }
       /* Subtract overhead time for hst pulse */
       if (newacq)
	  time = time - (2.0*INOVA_STD_APBUS_DELAY);
       if (time < 0.0) time = 0.0;
    }

    G_Delay(DELAY_TIME, time, 0);
    return(OK);
}


/*-------------------------------------------------------------------------
|
|       homospoil_on() 
|       Sets homospoil line on, TAKEN FROM hsdelay(time) above
|
|                               Bayard Fetler 8/15/96
+------------------------------------------------------------------------*/

int homospoil_on()
{
    if (bgflag)  fprintf(stderr, "homospoil_on()\n");
    if (newacq)
        {
        putcode(IHOMOSPOIL);
        putcode(TRUE);
        }
    else
        HSgate(homospoil_bit,TRUE);
    return(OK);
}


/*-------------------------------------------------------------------------
|
|       homospoil_off() 
|       Sets homospoil line off, TAKEN FROM hsdelay(time) above
|
|                               Bayard Fetler 8/15/96
+------------------------------------------------------------------------*/

int homospoil_off()
{
    if (bgflag)  fprintf(stderr, "homospoil_off()\n");
    if (newacq)
        {
        putcode(IHOMOSPOIL);
        putcode(FALSE);
        }
    else
        HSgate(homospoil_bit,FALSE);
    return(OK);
}
