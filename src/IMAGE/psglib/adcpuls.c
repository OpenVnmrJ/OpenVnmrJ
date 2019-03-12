/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* 
 */
/*  adcpuls - ADC pulse sequence 

            P1-tdelta-trise-ted1-P2-tdelta-trise-ted2-Acquire

            ticks=1 enables external trigger  
            p1,p2   - 90,180 deg pulses
            flip1,flip2  - 90, 180 deg
            p1pat,p2pat - pulse shape (e.g. hard, gauss)
            tpwr1,tpwr2  - pulse power (dB)
	    to calculate correct tpwr for a shaped pulse, set fliplist to a 
	    particular flipangle and type ssprep
            
	    gdiff - gradient amplitude G/cm
	    gf - gradient on/off flag
            gradient is applied along slice direction and selected via orient
*/

#include <standard.h>
#include <sgl.c>

static int ph180[2] = {1,3};

void pulsesequence()
{
   double pd, seqtime;
   double mintDELTA,ted1,ted2,gf;
   double restol, resto_local;

   init_mri();              /****needed ****/

   restol=getval("restol");   //local frequency offset
   roff=getval("roff");       //receiver offset

   init_rf(&p1_rf,p1pat,p1,flip1,rof1,rof2);   /* hard pulse */
   calc_rf(&p1_rf,"tpwr1","tpwr1f");
   init_rf(&p2_rf,p2pat,p2,flip2,rof1,rof2);   /* hard pulse */
   calc_rf(&p2_rf,"tpwr2","tpwr2f");

   gf=1.0;
   if(diff[0] == 'n') gf=0;
   int  vph180     = v2;  /* Phase of 180 pulse */

   mintDELTA = tdelta + trise + rof1 + p2 + rof2;
   if(tDELTA <= mintDELTA) {
       abort_message("%s: tDELTA too short. Min tDELTA = %f ms",seqfil,mintDELTA*1e3);
   }
   ted1 = tDELTA - tdelta + trise + p2 + rof1 + rof2;
   te = p1/2 + rof2 + tdelta + trise + ted1 + rof1 + p2/2;   /* first half-te */
   ted2 = te - p2/2 - rof2 - tdelta - trise;
   if((ted1 <= 0)||(ted2 <= 0) ) {
       abort_message("%s: tDELTA too short. Min tDELTA = %f ms",seqfil,mintDELTA*1e3);
   }
   te = te*2.0;
   putvalue("te",te);
   seqtime = at+(p1/2.0)+rof1+te;
   pd = tr - seqtime;  /* predelay based on tr */
   if (pd <= 0.0) {
      abort_message("%s: Requested tr too short.  Min tr = %f ms",seqfil,seqtime*1e3);
   }

   resto_local=resto-restol; 

   status(A);
   rotate();
   delay(pd);
   xgate(ticks);

   /* --- observe period --- */
   obsoffset(resto_local); 
   obspower(p1_rf.powerCoarse);
   obspwrf(p1_rf.powerFine);
   shapedpulse(p1pat,p1,oph,rof1,rof2);

   obl_gradient(0,0,gdiff*gf);   /* x,y,z gradients selected via orient */
   delay(tdelta);
   zero_all_gradients();
   delay(trise);
   delay(ted1);
     
   obspower(p2_rf.powerCoarse);
   obspwrf(p2_rf.powerFine);   
   settable(t2,2,ph180);        /* initialize phase tables and variables */
   getelem(t2,ct,v6);  /* 180 deg pulse phase alternates +/- 90 off the rcvr */
   add(oph,v6,vph180);      /* oph=zero */
   shapedpulse(p2pat,p2,vph180,rof1,rof2);

   obl_gradient(0,0,gdiff);   /* x,y,z gradients selected via orient */
   delay(tdelta);
   zero_all_gradients();
   delay(trise);
   delay(ted2);
   startacq(alfa);
   acquire(np,1.0/sw);
   endacq();
}


