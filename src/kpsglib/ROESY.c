// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
#ifndef LINT
#endif
/* 
 */
/* ROESY - cross-relaxation experiment in rotating frame

	Features included:
		States-TPPI in F1
		z-filter with flipback option
		
	Paramters:
		sspul :		selects magnetization randomization option
		mixR	:	ROESY spinlock mixRing time
		slpwrR	:	spin-lock power level
		slpwR	:	90 deg pulse width for spinlock
		slpatR	:	Spinlock pattern [cw and troesy]
		zfilt	:	selects z-filter with flipback option
				[requires gradient and PFGflg=y]

************************************************************************
****NOTE:  v2,v3,v4,v5 and v9 are used by Hardware Loop and reserved ***
   v2 is spinlock phase for t-roesy
   v3 is spinlock phase for dante and cw roesy
************************************************************************

KrishK	-	Last revision	: June 1997
KrishK	-	Revised		: July 2004

*/



#include <standard.h>
#include <chempack.h>
extern int dps_flag;

static int ph1[4] = {1,3,2,0},
	   ph2[8] = {3,3,0,0,1,1,2,2},
           ph3[8] = {1,3,2,0,3,1,0,2},
           ph8[4] = {3,3,0,0},
           ph6[8] = {1,1,2,2,3,3,0,0};

void pulsesequence()
{
   double          slpwrR = getval("slpwrR"),
                   slpwR = getval("slpwR"),
                   mixR = getval("mixR"),
                   gzlvlz = getval("gzlvlz"),
                   gtz = getval("gtz"),
		   zfphinc = getval("zfphinc");
   char		   slpatR[MAXSTR];
   int		   phase1 = (int)(getval("phase")+0.5);

/* LOAD AND INITIALIZE PARAMETERS */
   getstr("slpatR",slpatR);

   if (strcmp(slpatR,"cw") &&
        strcmp(slpatR,"troesy") &&
        strcmp(slpatR,"dante"))
        abort_message("SpinLock pattern %s not supported!.\n", slpatR);


   sub(ct,ssctr,v7);

   settable(t1,4,ph1);	getelem(t1,v7,v1);
   settable(t2,8,ph2);	getelem(t2,v7,v2);
   settable(t3,8,ph3);	
   settable(t8,4,ph8);	getelem(t8,v7,v8);
   settable(t6,8,ph6);	getelem(t6,v7,v6);
  
   assign(v1,oph); 
   if (getflag("zfilt"))
	getelem(t3,v7,oph);

   add(v2,one,v3);
   add(v2,two,v4);
   add(v2,three,v5);

   if (phase1 == 2)
      {incr(v1); incr(v6);}			/* hypercomplex method */
/*
   mod2(id2,v13);
   dbl(v13,v13);
*/
  initval(2.0*(double)(((int)(d2*getval("sw1")+0.5)%2)),v13);

       add(v1,v13,v1);
       add(v6,v13,v6);
       add(oph,v13,oph);

/* The following is for flipback pulse */
   zfphinc=zfphinc+180;
   if (zfphinc < 0) zfphinc=zfphinc+360;
   initval(zfphinc,v10);

/* BEGIN ACTUAL PULSE SEQUENCE */
   status(A);
      obspower(tpwr);

   delay(5.0e-5);
   if (getflag("sspul"))
        steadystate();

   if (satmode[0] == 'y')
     {
        if ((d1-satdly) > 0.02)
                delay(d1-satdly);
        else
                delay(0.02);
        satpulse(satdly,v6,rof1,rof1);
     }
   else
        delay(d1);

   if (getflag("wet"))
     wet4(zero,one);
   decpower(dpwr);

   status(B);
      rgpulse(pw, v1, rof1, rof1);
      if (d2 > 0.0)
       delay(d2 - POWER_DELAY - (2*pw/PI) - rof1);
      else
       delay(d2);
      
      obspower(slpwrR);

      if (mixR > 0.0)
      {
        if (dps_flag)
	  {
	     if (!strcmp(slpatR,"troesy"))
          	rgpulse(mixR,v2,0.0,0.0);
	     else
		rgpulse(mixR,v3,0.0,0.0);
	  }
        else
          SpinLock(slpatR,mixR,slpwR,v2,v3,v4,v5, v9);
      }

       if ((getflag("zfilt")) && (getflag("PFGflg")))
        {
           obspower(tpwr);
           rgpulse(pw,v2,1.0e-6,rof1);
           zgradpulse(gzlvlz,gtz);
           delay(gtz/3);
           if (getflag("flipback"))
                FBpulse(v8,v10);
           rgpulse(pw,v8,rof1,rof2);
        }
       else
           delay(rof2);

   status(C);
}
