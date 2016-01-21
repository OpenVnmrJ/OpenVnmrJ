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
/*  cpmg - CPMG T2 pulse sequence

		p1 - {r - p2 -r}*n - Acquire
		
            ticks=1 enables external trigger  
            p1pat - pulse shape (=hard)
            tpwr1  - pulse power (dB)
            p1    - 90deg pulse width; p2=p1*2
            bigtau    - t2 delay; set to bigtauarray
            d2	  - user defined delay unit, r 
            
 Version: 20060505
*/

#include "sgl.c"

pulsesequence()
{
   double pd, seqtime;
   double n,r,bigtau;
   double restol, resto_local;

   init_mri();

   restol=getval("restol");   //local frequency offset
   roff=getval("roff");       //receiver offset

   init_rf(&p1_rf,p1pat,p1,flip1,rof1,rof2);   /* hard pulse */
   calc_rf(&p1_rf,"tpwr1","tpwr1f");
   init_rf(&p2_rf,p2pat,p2,flip2,rof1,rof2);   /* hard pulse */
   calc_rf(&p2_rf,"tpwr2","tpwr2f");

/* calculate 'big tau' values */
   bigtau = getval("bigtau");
   n =  bigtau/(2.0*d2);
   n = (double)((int)((n/2.0) + 0.5)) * 2.0;
   initval(n,v3);

   seqtime = at+p1+rof1+rof2;
   seqtime += 2*d2+p2+rof1+rof2;  /* cpmg pulse and delay */
   
   pd = tr - seqtime;  /* predelay based on tr */
   if (pd <= 0.0) {
      abort_message("%s: Requested tr too short.  Min tr = %f ms",seqfil,seqtime*1e3);
    }

   resto_local=resto-restol; 

   status(A);
   delay(pd);
   xgate(ticks);
   
/* calculate exact delay and phases */

   r = d2-p2/2.0-rof2;   /* correct delay for pulse width */
   mod2(oph,v2);   /* 0,1,0,1 */
   incr(v2);   /* 1,2,1,2 = y,y,-y,-y */

   obsoffset(resto_local); 
   obspower(p1_rf.powerCoarse);
   obspwrf(p1_rf.powerFine);
   rgpulse(p1,oph,rof1,rof2);  /* 90deg */
   obspower(p2_rf.powerCoarse);
   obspwrf(p2_rf.powerFine);
   starthardloop(v3);
      delay(r);
      rgpulse(p2,v2,rof1,rof2);   /* 180deg pulse */
      delay(r);
   endhardloop();
   startacq(alfa);
   acquire(np,1.0/sw);
   endacq();
}


