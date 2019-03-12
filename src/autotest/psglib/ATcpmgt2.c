// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
/* 
 */
/* ATcpmgt2 -
  carr-purcell meiboom-gill t2 sequence
   with gradients in echo time */

#include <standard.h>
void pulsesequence()
{   
    double loops,r,gzlvl1,gzlvl2,gt1;
    char shaped[MAXSTR],gradaxis[MAXSTR];

    getstr("shaped",shaped);
    getstrnwarn("gradaxis",gradaxis);
    if (( gradaxis[A] != 'x') && ( gradaxis[A] != 'y') &&
        ( gradaxis[A] != 'z') )
      strcpy(gradaxis,"z");

    gzlvl1=getval("gzlvl1");
    gzlvl2=getval("gzlvl2");
    gt1=getval("gt1");
    if (p1<0.1*pw)
      gzlvl2=-gzlvl2;
    else
      gzlvl2=gzlvl2; 
    loops=getval("loops");
    loops=2*loops;                    /* to ensure even # of echos */
    initval(loops,v3);
    r = d2-p1/2.0-rof2-gt1;   /* correct delay for pulse width */
    mod2(oph,v2);   /* 0,1,0,1 */
    incr(v2);   /* 1,2,1,2 = y,y,-y,-y */

/* equilibration period */
    status(A);
    lk_sample();
    hsdelay(d1);
    lk_hold();
/* spin-echo loop */
    status(B);
    rgpulse(pw,oph,rof1,rof2);
    if (shaped[A]=='s')
     {
      loop(v3,v5);
        zgradpulse(gzlvl1,gt1);
    	delay(r);
    	rgpulse(p1,v2,rof2,rof2); 
        zgradpulse(gzlvl2,gt1);
    	delay(r);
      endloop(v5);
     } 
    else
     {
      loop(v3,v5);
        rgradient(gradaxis[A],gzlvl1);
        delay(gt1);
        rgradient(gradaxis[A],0.0);
    	delay(r);
    	rgpulse(p1,v2,rof2,rof2); 
        rgradient(gradaxis[A],gzlvl2);
        delay(gt1);
        rgradient(gradaxis[A],0.0);
    	delay(r);
      endloop(v5);
   }
/* observation period */
    status(C);
} 
