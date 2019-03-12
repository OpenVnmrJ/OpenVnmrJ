/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*  Hadamard-SOFAST-NHMQC               */
/*  P.Schanda and B. Brutscher, JMR, 178, 334-339(2006)
    Modified for BioPack, GG, Palo Alto, March 2006

    Fine power (tpwrsf) should be arrayed for best S/N
    Requires Hadamard transform software (VNMRJ 2.1B) */  
#include <standard.h>



static int 

   phi1[2] = {0,2},
   phi2[2] = {0,2};


void pulsesequence()
{
   char   
          shname1[MAXSTR],
	  shname2[MAXSTR],
	  shname3[MAXSTR],
          pshape[MAXSTR];

   int   
          phase;


   double 
   	  adjust = getval("adjust"),
          gzlvl1 = getval("gzlvl1"),
          gzlvl2 = getval("gzlvl2"), 
          gzlvl3 = getval("gzlvl3"), 
          gt1 = getval("gt1"),
          gt2 = getval("gt2"),
          gt3 = getval("gt3"),
          tpwrsf = getval("tpwrsf"),
          shlvl1 = getval("shlvl1"),
          shlvl2 = getval("shlvl2"),
          shpw1 = getval("shpw1"),
          shpw2 = getval("shpw2"),
          pwNlvl = getval("pwNlvl"),
          pwN = getval("pwN"),
          dpwr2 = getval("dpwr2"),
          taunh = 1.0/(2.0* getval("JNH")),
         compN = getval("compN");   /* adjust for N15 amplifier compression */


   shape    hdx;                                 /* HADAMARD stuff */




   getstr("shname1", shname1);
   getstr("shname2", shname2);
   getstr("shname3", shname3);
   getstr("pshape", pshape);
   getstr("pshape",pshape);

   if(getval("ni") > getval("nimax"))
   { printf("ni exceeds nimax. Aborting...\n"); psg_abort(1); }
   hdx = pboxHT_F1r(pshape, pwN*compN, pwNlvl); /* HADAMARD stuff */

   setlimit("ni", getval("ni"), getval("nimax"));     /* limits max ni */

  phase = (int) (getval("phase") + 0.5);
   
   settable(t1,2,phi1);
   settable(t2,2,phi2);



   status(A);

   dec2power(pwNlvl);
   dec2offset(dof2);
   obsoffset(tof);
   if (tpwrsf<4095.0)
    {shlvl1=shlvl1+6.0;
     shlvl2=shlvl2+6.0;}
   obspwrf(tpwrsf);

   zgradpulse(gzlvl2, gt2);
   lk_sample();
   delay(1.0e-4);
   delay(d1-gt2);
   lk_hold();
   if (ix < (int)(2.0*ni)) lk_hold();

   obspower(shlvl1);
   shaped_pulse(shname1,shpw1,zero,2.0e-4,2.0e-6);
     
   zgradpulse(gzlvl1, gt1);
   delay(1.0e-4);
   delay(taunh-gt1-1.0e-4-adjust);
   obspower(shlvl2);
   dec2power(pwNlvl); 
   dec2rgpulse(pwN,t1,0.0,0.0);
/************************************************************/
/* Hadamard 15N labeling using phase-altered refoc. pulses  */
/************************************************************/
   zgradpulse(gzlvl3, gt3);
   delay(1.0e-4);
   dec2power(hdx.pwr); 
   sim3shaped_pulse(shname2, "", hdx.name, shpw2, 0.0, hdx.pw,
                zero, zero, zero, 2.0e-6,2.0e-6);  
   zgradpulse(gzlvl3, gt3);
   delay(1.0e-4);
   dec2power(pwNlvl); 
/***********************************************************/
 
   dec2rgpulse(pwN,zero,0.0,0.0);
   dec2power(dpwr2); 

   zgradpulse(gzlvl1, gt1);
   obspower(shlvl1);

   delay(1.0e-4);
   delay(taunh-gt1-1.0e-4);


   status(C);
   setreceiver(t2);


}


