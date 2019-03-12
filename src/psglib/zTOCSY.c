// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.

/* zTOCSY -

	Features included:
		States-TPPI in F1
		z-filter pulse with flipback option
		
	Paramters:
		sspul :		selects magnetization randomization option
		mixT	:	TOCSY spinlock mixTing time
		slpatT	:	TOCSY pattern [mlev17,mlev17c,dipsi2,dipsi3]
		trim	:	trim pulse preceeding spinlock
		slpwrT	:	spin-lock power level
		slpwT	:	90 deg pulse width for spinlock
		zfilt	:	selects z-filter with flipback option
				[Requires gradient and PFGflg=y]

************************************************************************
****NOTE:  v20,v21,v22,v23 and v24 are used by Hardware Loop and reserved ***
************************************************************************

HaitaoH		-	ZQ suppression Sept 2004
KrishK		-	Revised for CP3 May 2005
KrishK  -       Includes slp saturation option : July 2005
KrishK - includes purge option : Aug. 2006
****v17,v18,v19 are reserved for PURGE ***
JohnR - includes CPMG option : Jan 2015
****v15 is reserved for CPMG ***

*/


#include <standard.h>
#include <chempack.h>

extern int dps_flag;

static int ph1[4] = {0,0,1,1},   		/* presat */
           ph2[4] = {1,3,2,0},                  /* excite */
           ph3[8] = {1,3,2,0,3,1,0,2},          /* receiver */
           ph5[4] = {0,0,1,1},                  /* spin lock */
	   ph7[8] = {1,1,2,2,3,3,0,0},		/* flipback */
	   ph8[4] = {3,3,0,0};			/* observe */
	   
void pulsesequence()
{
   double          slpwrT = getval("slpwrT"),
                   slpwT = getval("slpwT"),
                   mixT = getval("mixT"),
                   gzlvl1 = getval("gzlvl1"),
                   gt1 = getval("gt1"),
                   gzlvl2 = getval("gzlvl2"),
                   gt2 = getval("gt2"),
                   zqfpw1 = getval("zqfpw1"),
                   zqfpwr1 = getval("zqfpwr1"),
                   zqfpw2 = getval("zqfpw2"),
                   zqfpwr2 = getval("zqfpwr2"),
                   gzlvlzq1 = getval("gzlvlzq1"),
                   gzlvlzq2 = getval("gzlvlzq2");
   char            slpatT[MAXSTR],
                   zqfpat1[MAXSTR],
                   zqfpat2[MAXSTR];
   int             phase1 = (int)(getval("phase")+0.5),
                   prgcycle = (int)(getval("prgcycle")+0.5);

/* LOAD AND INITIALIZE VARIABLES */
   getstr("slpatT",slpatT);
   getstr("zqfpat1",zqfpat1);
   getstr("zqfpat2",zqfpat2);

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
  initval(2.0*(double)(((int)(d2*getval("sw1")+0.5)%2)),v14);
 
   settable(t1,4,ph1); 	getelem(t1,v17,v6);
   settable(t2,4,ph2); 	getelem(t2,v17,v1);
   settable(t3,8,ph3); 	getelem(t3,v17,oph);
   settable(t5,4,ph5); 	getelem(t5,v17,v21);
   settable(t7,8,ph7); 	getelem(t7,v17,v7);
   settable(t8,4,ph8); 	getelem(t8,v17,v8);   
    
  if (getflag("prgflg") && (satmode[0] == 'y'))
        sub(v6,one,v6);

   add(oph,v18,oph);
   add(oph,v19,oph);
 
   if (phase1 == 2)
      {incr(v1); incr(v6);}

   add(v1, v14, v1);
   add(v6, v14, v6);
   add(oph,v14,oph);


/* BEGIN ACTUAL PULSE SEQUENCE CODE */
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
      if (getflag("cpmgflg"))
      {
       rgpulse(pw, v1, rof1, 0.0);
       cpmg(v1, v15);
      }
      else
       rgpulse(pw, v1, rof1, 2.0e-6);
      if (d2 > 0.0)
	delay(d2 - 4.0e-6 - (4*pw/PI));
      else
	delay(d2);
      rgpulse(pw,v7,2.0e-6,rof1);

      if (mixT > 0.0)
      {
        if (getflag("Gzqfilt"))
        {
         obspower(zqfpwr1);
         rgradient('z',gzlvlzq1);
         delay(100.0e-6);
         shaped_pulse(zqfpat1,zqfpw1,zero,rof1,rof1);
         delay(100.0e-6);
         rgradient('z',0.0);
         delay(100e-6);
        }
        obspower(slpwrT);
        zgradpulse(gzlvl1,gt1);
        delay(gt1);
        
        if (dps_flag)
          rgpulse(mixT,v21,0.0,0.0);
        else
          SpinLock(slpatT,mixT,slpwT,v21);

        if (getflag("Gzqfilt"))
        {
         obspower(zqfpwr2);
         rgradient('z',gzlvlzq2);
         delay(100.0e-6);
         shaped_pulse(zqfpat2,zqfpw2,zero,rof1,rof1);
         delay(100.0e-6);
         rgradient('z',0.0);
         delay(100e-6);
        }
        obspower(tpwr);
        zgradpulse(gzlvl2,gt2);
        delay(gt2);
      }

     rgpulse(pw,v8,rof1,rof2);
           
   status(C);
}
