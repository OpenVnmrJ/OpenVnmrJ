// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.

/* T1Rmeas - T1Rho measurement with MLEV17c spinlock

	Features included:
		z-filter pulse with flipback option
		
	Paramters:
		sspul :		selects magnetization randomization option
		d2	:	spinlock mixing time
		slpatT	:	spinlock pattern [mlev17,mlev17c]
		slpwrT	:	spin-lock power level
		slpwT	:	90 deg pulse width for spinlock
		zfilt	:	selects z-filter with flipback option
				[Requires gradient and PFGflg=y]

************************************************************************
****NOTE:  v20,v21,v22,v23 and v24 are used by Hardware Loop and reserved ***
************************************************************************

KrishK	-	Last revision	: June 1997
KrishK	-	Revised		: July 2004
KrishK  -       Includes slp saturation option : July 2005
KrishK - includes purge option : Aug. 2006
****v17,v18,v19 are reserved for PURGE ***
JohnR - includes CPMG option : Jan 2015
****v15 is reserved for CPMG ***
BertH - T1rho version

*/


#include <standard.h>
#include <chempack.h>

extern int dps_flag;

static int ph1[4] = {0,0,1,1},   		      /* presat */
           ph2[4] = {1,3,2,0},                  /* 90 */
           ph3[8] = {1,3,2,0,3,1,0,2},          /* receiver */
           ph4[4] = {0,0,1,1};                  /* spin lock */
	   
void pulsesequence()
{
   double          slpwrT = getval("slpwrT"),
                   slpwT = getval("slpwT"),
		   gzlvlz = getval("gzlvlz"),
		   gtz = getval("gtz"),
		   zfphinc = getval("zfphinc");
   char		   slpatT[MAXSTR];
   int		   phase1 = (int)(getval("phase")+0.5),
                   prgcycle = (int)(getval("prgcycle")+0.5);

/* LOAD AND INITIALIZE VARIABLES */
   getstr("slpatT",slpatT);

   if (strcmp(slpatT,"mlev17c") &&
        strcmp(slpatT,"dipsi2") &&
        strcmp(slpatT,"dipsi3") &&
        strcmp(slpatT,"mlev17") &&
        strcmp(slpatT,"mlev16"))
        abort_message("SpinLock pattern %s not supported!.\n", slpatT);


  assign(ct,v17);
  assign(zero,v18);
  assign(zero,v19);

  if (getflag("prgflg") && (satmode[0] == 'y') && (prgcycle > 1.5))
    {
        hlv(ct,v17);
        mod2(ct,v18); dbl(v18,v18);
        if (prgcycle > 2.5)
           {
                hlv(v17,v17);
                hlv(ct,v19); mod2(v19,v19); dbl(v19,v19);
           }
     }

/*  
   mod2(id2,v14);
   dbl(v14,v14);
*/

 
   settable(t1,4,ph1); 	getelem(t1,v17,v6);
   settable(t2,4,ph2); 	getelem(t2,v17,v1);
   settable(t3,8,ph3); 	
   settable(t4,4,ph4); 	getelem(t4,v17,v21);

   assign(v1,oph);
   
   if (getflag("prgflg") && (satmode[0] == 'y'))
	sub(v6,one,v6);

   add(oph,v18,oph);
   add(oph,v19,oph);
 
   add(v1, v14, v1);
   add(v6, v14, v6);
   add(oph,v14,oph);

/* BEGIN ACTUAL PULSE SEQUENCE CODE */
   status(A);
      obspower(tpwr);

   if (satmode[0] == 'y')
     {
        if ((d1-satdly) > 0.02)
                delay(d1-satdly);
        else
                delay(0.02);
        if (getflag("slpsat"))
	   {
                shaped_satpulse("relaxD",satdly,v6);
                if (getflag("prgflg"))
                   shaped_purge(v1,v6,v18,v19);
           }
        else
	   {
                satpulse(satdly,v6,rof1,rof1);
                if (getflag("prgflg"))
                   purge(v1,v6,v18,v19);
	   }
     }
   else
        delay(d1);

   if (getflag("wet"))
     wet4(zero,one);

   status(B);
      rgpulse(pw, v1, rof1, rof1);
      obspower(slpwrT);
      txphase(v21);

      if (d2 > 0.0)
      { 
	if (dps_flag)
	  rgpulse(d2,v21,0.0,0.0);
	else
	  SpinLock(slpatT,d2,slpwT,v21);
       }
	
/* Add 2*pw/PI to final rof2, so ddrpm can be set to
   a value of 'p' for both zfilt options */

           delay(rof2+(2*pw/PI));
           
   status(C);
}
