// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.

/******************************************/
/* cpmgt2 -                               */
/*  carr-purcell meiboom-gill t2 sequence */
/******************************************/

#include <standard.h>
#include <chempack.h>

static int phs1[4] = {0,2,1,3},
           phs3[4] = {0,0,1,1},
           phs2[8] = {0,0,1,1,2,2,3,3};

void pulsesequence()
{   
  double cycles,
	 bigtau = getval("bigtau"),
	 tau = getval("tau"),
  	  satdly = getval("satdly");
  int prgcycle=(int)(getval("prgcycle")+0.5);
  char  satmode[MAXSTR],
        sspul[MAXSTR];

  getstr("satmode",satmode);
  getstr("sspul",sspul);

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

  settable(t1,4,phs1);
  settable(t2,8,phs2);
  settable(t3,4,phs3);
  getelem(t1,v17,v1);
  getelem(t2,v17,v2);
  getelem(t3,v17,v4);
  assign(v1,oph);
  add(oph,v18,oph);
  add(oph,v19,oph);

  if (getflag("prgflg") && (satmode[0] == 'y'))
        assign(v4,v2);

/* calculate 'big tau' values */
   cycles = bigtau/(2.0*tau);
   cycles = (double)((int)((cycles/2.0) + 0.5)) * 2.0;
   initval(cycles,v3);


/* equilibration period */
   status(A);

   delay(5.0e-5);
   if (sspul[0] == 'y')
        steadystate();

   if (satmode[0] == 'y')
     {
        if ((d1-satdly) > 0.02)
                delay(d1-satdly);
        else
                delay(0.02);
        if (getflag("slpsat"))
           {
                shaped_satpulse("relaxD",satdly,v2);
                if (getflag("prgflg"))
                   shaped_purge(v1,v2,v18,v19);
           }
        else
           {
                satpulse(satdly,v2,rof1,rof1);
                if (getflag("prgflg"))
                   purge(v1,v2,v18,v19);
           }
     }
   else
        delay(d1);

/* calculate exact delay and phases */
   mod2(oph,v5);  
   incr(v5); 

/* spin-echo loop */
   status(B);
   rgpulse(pw,v1,rof1,0.0);
   starthardloop(v3);
      delay(tau - p1/2.0 - rof2);
      rgpulse(p1,v5,rof2,rof2); 
      delay(tau - p1/2.0 - rof2);
   endhardloop();

/* observation period */
   status(C);
} 
