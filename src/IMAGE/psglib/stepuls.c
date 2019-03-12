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
/*  stepuls - ADC pulse sequence 

            P1-tdelta-trise-P1-tm-P1-tdelta-trise-Acquire

            ticks=1 enables external trigger  
            p1   - 90 deg pulse
            flip1  - 90deg
            p1pat - pulse shape (e.g. hard, gauss)
            tpwr1  - pulse power (dB)
            tpwr1f - fine power (0-4k)
            
	    gdiff - gradient amplitude G/cm
	    gf - gradient on/off flag
            gradient is applied along slice direction and selected via orient
*/

#include <standard.h>
#include "sgl.c"

/* Phases for phase cycling */
static int ph1[8]   = {0, 2, 0, 2,  0, 2, 0, 2};
static int ph2[8]   = {0, 0, 2, 2,  0, 0, 2, 2};
static int ph3[8]   = {0, 0, 0, 0,  2, 2, 2, 2};
static int phobs[8] = {0, 2, 2, 0,  2, 0, 0, 2};

void pulsesequence()
{
   double pd, seqtime;
   double ted1,ted2,gf,tm,mintDELTA;
   double restol, resto_local;

  /* Real-time variables used in this sequence **************/
  int    vph1 = v1;   // Phase of first RF pulse  
  int    vph2 = v2;   // Phase of second RF pulse  
  int    vph3 = v3;   // Phase of third RF pulse  

   init_mri();              /****needed ****/

   restol=getval("restol");   //local frequency offset
   roff=getval("roff");       //receiver offset

   init_rf(&p1_rf,p1pat,p1,flip1,rof1,rof2);   /* hard pulse */
   calc_rf(&p1_rf,"tpwr1","tpwr1f");

   gf=1.0;
   if(diff[0] == 'n') gf=0;
  
   temin =  2*(p1 + rof1 + rof2 + tdelta + trise + alfa);
   temin = temin + 4e-6;  /* ensure that te_delay is at least 4us */
   if (minte[0] == 'y') {
     te = temin;
     putvalue("te",te);
   }
   if (te < temin) {
    abort_message("TE too short.  Minimum TE= %.2fms\n",temin*1000+0.005);
   }

   ted1 = te/2 - p1 - rof1 - rof2 - tdelta - trise;
   ted2 = te/2 - p1 - rof1 - rof2 - tdelta - trise - alfa;

   if(ted2 <= 0) {
       abort_message("%s: TE too short. Min TE = %f ms",seqfil,temin*1e3);
   }

   mintDELTA = ted1 + 2*(p1+rof1+rof2) + tdelta + trise;
   if(tDELTA <= mintDELTA) {
       abort_message("%s: tDELTA too short. Min tDELTA = %f ms",seqfil,mintDELTA*1e3);
   }
   tm = tDELTA - ted1 - 2*(p1+rof1+rof2) - trise - tdelta;

   seqtime = at + 3*(p1+rof1+rof2) + 2*(tdelta+trise) + tm + ted1 + ted2 + alfa;
   pd = tr - seqtime;  /* predelay based on tr */
   if (pd <= 0.0) {
      abort_message("%s: Requested TR too short.  Min TR = %f ms",seqfil,seqtime*1e3);
   }

   resto_local=resto-restol; 

  /* Create phase cycling */
  settable(t1,8,ph1);   getelem(t1,ct,vph1);
  settable(t2,8,ph2);   getelem(t2,ct,vph2);
  settable(t3,8,ph3);   getelem(t3,ct,vph3);
  settable(t4,8,phobs); getelem(t4,ct,oph);

   rotate();
   delay(pd);
   if (ticks) {
     xgate(ticks);
     grad_advance(gpropdelay);
   }

   /* --- observe period --- */
   obsoffset(resto_local); 
   obspower(p1_rf.powerCoarse);
   obspwrf(p1_rf.powerFine);
   shapedpulse(p1pat,p1,vph1,rof1,rof2);

   obl_gradient(0,0,gdiff*gf);   /* x,y,z gradients selected via orient */
   delay(tdelta);
   zero_all_gradients();
   delay(trise);
   delay(ted1);
     
   shapedpulse(p1pat,p1,vph2,rof1,rof2);
   delay(tm);
   shapedpulse(p1pat,p1,vph3,rof1,rof2);

   obl_gradient(0,0,gdiff);   /* x,y,z gradients selected via orient */
   delay(tdelta);
   zero_all_gradients();
   delay(trise);
   delay(ted2);
   startacq(alfa);
   acquire(np,1.0/sw);
   endacq();
}
/*******************************************************************************
	Modification History

20080104 minte; phase cycling included

*******************************************************************************/

