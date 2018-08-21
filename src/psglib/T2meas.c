// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
/*
 */
/******************************************/
/* T2meas -                               */
/*  carr-purcell meiboom-gill t2 sequence */
/******************************************/

#include <standard.h>

pulsesequence()
{   
   double cycles,
          bigtau = getval("bigtau"),
          tau    = getval("tau");

/* calculate 'big tau' values */
   bigtau = getval("bigtau");
   cycles = bigtau/(2.0*tau);
   cycles = (double)((int)((cycles/2.0) + 0.5)) * 2.0;
   initval(cycles,v3);

   if (satpwr > 45)
   {  printf("satpwr too large - acquisition aborted.\n");
      psg_abort(1);
   }

/* equilibration period */
   status(A);
   hsdelay(d1);

/* presaturation */
   if (satmode[A] == 'y')
   {
      if (fabs(tof-satfrq)>0.0)
         obsoffset(satfrq);
      obspower(satpwr);
      txphase(v5);
      rgpulse(satdly,v5,rof1,rof1);
      obspower(tpwr);
      if (fabs(tof-satfrq)>0.0)
      {  obsoffset(tof);
         delay(40.0e-6);
      }
   }

/* calculate exact delay and phases */
   mod2(oph,v2);   /* 0,1,0,1 */
   incr(v2);   /* 1,2,1,2 = y,y,-y,-y */

/* spin-echo loop */
   status(B);
   rgpulse(pw,oph,rof1,0.0);
   starthardloop(v3);
      delay(tau-p1/2.0-rof2);
      rgpulse(p1,v2,rof2,rof2); 
      delay(tau-p1/2.0-rof2);
   endhardloop();

/* observation period */
   status(C);
} 
