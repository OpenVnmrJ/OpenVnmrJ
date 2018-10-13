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
/*  t2puls - T2 pulse sequence 
            if d2=0 no 180 pulse applied

            ticks=1 enables external trigger  
            p1,p2   - 90,180 deg pulses
            flip1,flip2  - 90, 180 deg
            p1pat,p2pat - pulse shape (e.g. hard, gauss)
            tpwr1,tpwr2  - pulse power (dB)
	    to calculate correct tpwr for a shaped pulse, set fliplist to a 
	    particular flipangle and type ssprep
*/

#include <standard.h>
#include "sgl.c"
static int ph180[2] = {1,3};

pulsesequence()
{
   double pd, seqtime;
   double minte,ted1,ted2;
   double restol, resto_local;

   int  vph180     = v2;  /* Phase of 180 pulse */
   init_mri();              /****needed ****/

   restol=getval("restol");   //local frequency offset
   roff=getval("roff");       //receiver offset

   init_rf(&p1_rf,p1pat,p1,flip1,rof1,rof2);   /* hard pulse */
   calc_rf(&p1_rf,"tpwr1","tpwr1f");
   init_rf(&p2_rf,p2pat,p2,flip2,rof1,rof2);   /* hard pulse */
   calc_rf(&p2_rf,"tpwr2","tpwr2f");

   seqtime = at+(p1/2.0)+rof1+d2;

   pd = tr - seqtime;  /* predelay based on tr */
   if (pd <= 0.0) {
      abort_message("%s: Requested tr too short.  Min tr = %f ms",seqfil,seqtime*1e3);
    }
   minte = p1/2.0 + p2 + 2*rof2 + rof1;
   if(d2 > 0) {
     if(d2 < minte+4e-6) 
       abort_message("%s: TE too short. Min te = %f ms",seqfil,minte*1e3);
   }
   ted1 = d2/2 - p1/2 - p2/2 + rof2 + rof1;
   ted2 = d2/2 - p2/2 + rof2;
   resto_local=resto-restol; 

   status(A);
   xgate(ticks);
   delay(pd);

   /* --- observe period --- */
   obsoffset(resto_local);
   obspower(p1_rf.powerCoarse);
   obspwrf(p1_rf.powerFine);
   shapedpulse(p1pat,p1,oph,rof1,rof2);
   /* if d2=0 no 180 pulse applied */
   if (d2 > 0) {
     obspower(p2_rf.powerCoarse);
     obspwrf(p2_rf.powerFine);   
     settable(t2,2,ph180);        /* initialize phase tables and variables */
     getelem(t2,ct,v6);  /* 180 deg pulse phase alternates +/- 90 off the rcvr */
     add(oph,v6,vph180);      /* oph=zero */
     delay(ted1);
     shapedpulse(p2pat,p2,vph180,rof1,rof2);
     delay(ted2);
   }
   startacq(alfa);
   acquire(np,1.0/sw);
   endacq();
}


