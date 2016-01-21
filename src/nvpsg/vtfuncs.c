/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#include <string.h>
#include "group.h"
#include "ACode32.h"
#include "acqparms.h"
#include "expDoneCodes.h"
#include "pvars.h"
#include "Bridge.h"

extern double sign_add(double, double);

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
+-----------------------------------------------------------------*/
void setvt()
{
int setting;
int pid;
int codeStream[20],i;

   /* setVT acodes always sent even if vttype = 0, to allow acq. to */
   /* reset its vttype to 0, for stat display */
   if ( tempactive )
      setting = (int) (sign_add((vttemp * 10.0),0.50005));/* temp .1 degrees */
   else
      setting = 30000;	/* temp setting that makes VT go inactive */
   pid = getvtpid();
   i = 0;
   codeStream[i++] = vttype;	/* Highland (=2) or None (=0)*/
   codeStream[i++] = pid;	/* PID value default is P=4, I=4, D=0 (440) */
   codeStream[i++] = setting;	/* temperature to set VT to */
   codeStream[i++] = sign_add((vtc * 10.0),0.50005); /*low tmp cutoff*/
   if ( (interLock[2] != 'N') && (interLock[2] != 'n'))
      codeStream[i++] = ( (interLock[2] == 'y') || (interLock[2] == 'Y') ) ?
                        HARD_ERROR : WARNING_MSG;
   else
      codeStream[i++] = 0;
   /* now write to all controllers, master will set, others wait */
   broadcastCodes(SETVT, i, codeStream);
   delay(0.100);  /* delays added to prevent DDR from reboot when spin=value, GMB 1/18/05 */
}
/*------------------------------------------------------------------
|
|	wait4vt()/0
|	send acode to wait for VT to obtain temperature 
|       only enable errors if finalcheck is true
+-----------------------------------------------------------------*/
void wait4vt(int finalcheck)
{
int codeStream[20],i;

   /* wait4vt only if one is there & tin=y & temp != no */
   if ( (vttype != 0) && (tempactive) )
   {
      i=0;
      if ( finalcheck &&  (interLock[2] != 'N') && (interLock[2] != 'n'))
         codeStream[i++] = ( (interLock[2] == 'y') || (interLock[2] == 'Y') ) ?
                        HARD_ERROR : WARNING_MSG;   // error type
      else
         codeStream[i++] = 0;
      codeStream[i++] = (int) vtwait;		    // how long to wait
      /* now write to all controllers, master will check, others wait */
      broadcastCodes(WAIT4VT, i, codeStream);
      delay(0.100);  /* delays added to prevent DDR from reboot when spin=value, GMB 1/18/05 */
   }
}
