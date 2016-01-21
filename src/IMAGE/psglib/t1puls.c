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
/*  t1puls - T1 pulse sequence 

            ticks=1 enables external trigger  
            p1,p2  - 90,180deg pulses
            flip1,flip2  - 90, 180 deg
            p1pat,p2pat - pulse shape (e.g. hard, gauss)
            tpwr1,tpwr2  - pulse power (dB)
            d2 - inversion time (d2=ti)
*/

#include <standard.h>
#include "sgl.c"
/* Phase cycling of 180 degree pulse */
static int ph180[2] = {1,3};

pulsesequence()
{
  double pd,seqtime,minti;
  double restol, resto_local;
  int  vph180     = v7;  /* Phase of 180 pulse */
  init_mri();              /****needed ****/

  restol=getval("restol");   //local frequency offset
  roff=getval("roff");       //receiver offset

  init_rf(&p1_rf,p1pat,p1,flip1,rof1,rof2);   /* hard pulse */
  calc_rf(&p1_rf,"tpwr1","tpwr1f");
  init_rf(&p2_rf,p2pat,p2,flip2,rof1,rof2);   /* hard pulse */
  calc_rf(&p2_rf,"tpwr2","tpwr2f");

  minti = p1/2.0 + p2/2.0 + rof2 + rof1;
  if(d2 <= minti) { 
    abort_message("%s: TI too short. Min ti = %f ms",seqfil,minti*1e3);
  }
  seqtime = at+p1+rof1+rof2+alfa;
  if (ir[0] == 'y')
    seqtime += d2+p2+rof1+rof2;  /* inversion pulse and delay */
  pd = tr - seqtime;  /* predelay based on tr */
  if (pd <= 0.0) {
    abort_message("%s: Requested tr too short.  Min tr = %f ms",seqfil,seqtime*1e3);
  }
  resto_local=resto-restol; 

  status(A);
  xgate(ticks);
  delay(pd);
  obsoffset(resto_local);
  /* --- observe period --- */
  if (ir[0] == 'y') {
    obspower(p2_rf.powerCoarse);
    obspwrf(p2_rf.powerFine);   
    settable(t2,2,ph180);        /* initialize phase tables and variables */
    getelem(t2,ct,v6);  /* 180 deg pulse phase alternates +/- 90 off the rcvr */
    add(oph,v6,vph180);      /* oph=zero */
    shapedpulse(p2pat,p2,vph180,rof1,rof2);
    delay(d2);
  }
  obspower(p1_rf.powerCoarse);
  obspwrf(p1_rf.powerFine);
  shapedpulse(p1pat,p1,oph,rof1,rof2);
  startacq(alfa);
  acquire(np,1.0/sw);
  endacq();
}


