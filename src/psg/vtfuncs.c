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
#include "expDoneCodes.h"

extern double sign_add();
extern int  bgflag;	/* debug flag */

static int getvtpid()
{
   int pid;
   char masvt[128];

   pid = 440;                           /* default value */
   if (!P_getstring(GLOBAL,"masvt",masvt,1,10))
   {
      if ((strcmp(masvt,"y") == 0) || (strcmp(masvt,"Y") == 0))
         pid = 171;                     /* value for Sorenson controller */
   }
   return(pid);
}

/*------------------------------------------------------------------
|
|	setvt()/0
|	set vt controller to proper value
|				Author Greg Brissey 6/26/86
|
|   Modified   Author     Purpose
|   --------   ------     -------
|   6/15/89   Greg B.     1. Use new global parameters to instead of getval(),
|			     getstr(),var_active().
+-----------------------------------------------------------------*/
setvt()
{
    int setting;
    int pid;

    /* setVT acodes always sent even if vttype = 0, to allow acq. to */
    /* reset its vttype to 0, for stat display */
    if ( tempactive )
      setting = (int) (sign_add((vttemp * 10.0),0.50005));/* temp .1 degrees */
    else
        setting = 30000;	/* temp setting that makes VT go inactive */
    pid = getvtpid();
    putcode(SETVT);		/* acode to setvt */
    putcode((codeint) vttype);	/* Varian or Oxford VT */
    putcode((codeint) pid);	/* PID value default is P=4, I=4, D=0 (440) */
    putcode((codeint) setting);	/* temperature to set VT to */
    putcode((codeint) (sign_add((vtc * 10.0),0.50005))); /*low tmp cutoff*/
    if ( (interLock[2] != 'N') && (interLock[2] != 'n'))
       putcode( ( (interLock[2] == 'y') || (interLock[2] == 'Y') ) ?
                   HARD_ERROR : WARNING_MSG);
    else
       putcode(0);
}
/*------------------------------------------------------------------
|
|	wait4vt()/0
|	send acode to wait for VT to obtain temperature 
|				Author Greg Brissey 6/26/86
|   Modified   Author     Purpose
|   --------   ------     -------
|   6/15/89   Greg B.     1. Use new global parameters to instead of getval(),
|			     getstr(),var_active().
+-----------------------------------------------------------------*/
wait4vt()
{
    int topword;
    int botword;

    if (bgflag)
	fprintf(stderr,"wait4vt(): \n");
    /* wait4vt only if one is there & tin=y & temp != no */
    if ( (vttype != 0) && (tempactive) &&
	 ((interLock[2] != 'N') && (interLock[2] != 'n')) )
    {
	putcode(WT4VT);	/* acode to wait for VT */
	convertdbl(vtwait,&topword,&botword);
	/* putcode((codeint) topword); */ /* top word has always been ignored in the console */
        putcode( ( (interLock[2] == 'y') || (interLock[2] == 'Y') ) ?
                   HARD_ERROR : WARNING_MSG);
	putcode((codeint) botword);
    }
}
