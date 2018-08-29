/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/* rna_WGnoesy  -  NOESY experiment with water suppression by gradient echo. 
               No attempt is made to suppress J-cross peaks in this pulse
               sequence. F1 axial peaks are shifted by States TPPI method.

               ech dast feb.93  gg palo alto jan 95
               added flipback 16 april 95
               added gradients in t1 to eliminated radiation damping effects
                (gzlvl2 can be set very low, gzlvl2=100 suggested)

               set d4=0 and gt1=mix to enhance water exchange crosspeaks by
               preventing radiation damping during mix. Phase of flipback
               pulse is currently only optimal for the case of radiation
               damping during mix, so set flipback='n' for enhancing
               water exchange crosspeaks. With proper phase cycling of the
               flipback pulse, it could be used. Again, set gzlvl2=100.

                5jan96 GG
                added to BioPack june 1998
                made identical to proteinpack/wgnoesy.c 10/3/00  GG
*/
#include <standard.h>
pulsesequence()
{
   double          arraydim,
                   ss,
                   p180 = getval("p180"),
                   gzlvl1 = getval("gzlvl1"),
                   gt1 = getval("gt1"),
                   gzlvl0 = getval("gzlvl0"),
                   gt0 = getval("gt0"),
                   gzlvl2 = getval("gzlvl2"),
                   gzlvl3 = getval("gzlvl3"),
                   gstab0 = getval("gstab0"),
                   gstab1 = getval("gstab1"),
                   gstab2 = getval("gstab2"),
                   phincr1 = getval("phincr1"),
                   phincr2 = getval("phincr2"),
                   flippwr = getval("flippwr"),
                   flippw  = getval("flippw"),
                   mix = getval("mix");
   int             t1_counter,
                   iphase;
   char            flag3919[MAXSTR],flipshap[MAXSTR],flipback[MAXSTR];


/* LOAD VARIABLES */
   getstr("flag3919",flag3919);
   getstr("flipshap",flipshap);
   getstr("flipback",flipback);
   arraydim = getval("arraydim");
   iphase = (int) (getval("phase") + 0.5);
   ss = getval("ss");
   if (phincr1 < 0.0) phincr1=360+phincr1;
   initval(phincr1,v8);
   if (phincr2 < 0.0) phincr2=360+phincr2;
   initval(phincr2,v11);

   if (iphase == 3)
   {
      t1_counter = ((int) (ix - 1)) / (arraydim / ni);
      initval((double) (t1_counter), v14);
   }
   else
      assign(zero, v14);


/* CHECK CONDITIONS */
   if ((rof1 < 9.9e-6) && (ix == 1))
      fprintf(stdout,"Warning:  ROF1 is less than 10 us\n");


/* This section determines if the phase calculations trigger off of (SS - SSCTR)
   or off of CT */

   ifzero(ssctr);
      mod2(ct, v2);
      hlv(ct, v3);
   elsenz(ssctr);
      sub(ssval, ssctr, v12);	/* v12 = 0,...,ss-1 */
      mod2(v12, v2);
      hlv(v12, v3);
   endif(ssctr);


/* CALCULATE PHASECYCLE */
   dbl(v2, v2);
   hlv(v3, v10);
   hlv(v10, v10);
   if (iphase == 0)
   {
      assign(v10, v9);
      hlv(v10, v10);
      mod2(v9, v9);
   }
   else
   {
      assign(zero, v9);
   }
   assign(v10,v1);
   hlv(v10, v10);
   mod2(v1, v1);
   dbl(v1, v1);
   add(v9, v2, v2);
   mod2(v10, v10);
   add(v1, v2, oph);
   add(v3, oph, oph);
   add(v10, oph, oph);
   add(v10, v1, v1);
   add(v10, v2, v2);
   add(v10, v3, v3);
   add(v10,v14,v5);
   if (iphase == 2)
      { incr(v2); incr(v5); }
   if (iphase == 3)
      add(v2, v14, v2);		/* TPPI phase increment */

/*HYPERCOMPLEX MODE USES REDFIELD TRICK TO MOVE AXIAL PEAKS TO EDGE */
    initval(2.0*(double)(((int)(d2*getval("sw1")+0.5)%2)),v6);
  if ((iphase==1)||(iphase==2))
       {add(v2,v6,v2); add(oph,v6,oph); add(v5,v6,v5);}  

/* BEGIN THE ACTUAL PULSE SEQUENCE */
 status(A);
      zgradpulse(gzlvl0,gt0);
      obsstepsize(45.0);
      initval(7.0,v7);
      xmtrphase(v7);
      delay(d1);
 status(B);
      rgpulse(pw, v2, rof1, rof1);
      if (d2 > 0.001)
       {
     zgradpulse(gzlvl2,0.4*d2-SAPS_DELAY/2.0-2.0*GRADIENT_DELAY-(2.0*pw/PI));
     delay(0.1*d2-rof1);
     zgradpulse(-gzlvl2,0.4*d2-SAPS_DELAY/2.0-2.0*GRADIENT_DELAY-(2.0*pw/PI));
     delay(0.1*d2-rof1);
       }
       else delay(d2);
      xmtrphase(zero);
      rgpulse(pw, v1, rof1, rof1);
 status(C);
      delay(d4);
      add(v3,two,v4);
      if (flipback[A] == 'y')
       {
        obspower(flippwr);
        obsstepsize(1.0);
        xmtrphase(v8);
        zgradpulse(gzlvl3,mix-flippw -d4-gstab0);
        delay(gstab0);
        shaped_pulse(flipshap,flippw,v4,20.0e-6,rof1);
        xmtrphase(zero);
        obspower(tpwr);
       }
      else
       {
        zgradpulse(gzlvl3,mix - d4-gstab0 );
        delay(gstab0);
       }


   rgpulse(pw, v3,rof1,rof1);
   delay(tau-pw/2-rof1);
   zgradpulse(gzlvl1,gt1);
   if (flag3919[A] == 'y')
     {
       delay(gstab1);
       pulse(pw*0.231,v3);
       delay(d3);
       pulse(pw*0.692,v3);
       delay(d3);
       pulse(pw*1.462,v3);
       delay(d3);
       pulse(pw*1.462,v4);
       delay(d3);
       pulse(pw*0.692,v4);
       delay(d3);
       pulse(pw*0.231,v4);
    }
  else
    {
       delay(gstab1-2.0*SAPS_DELAY);
       obspower(flippwr);
       xmtrphase(v11);
       shaped_pulse(flipshap,flippw,v4,rof1,rof1);
       obspower(tpwr);
       xmtrphase(zero);
       rgpulse(p180, v3,rof1,rof1);
       obspower(flippwr);
       shaped_pulse(flipshap,flippw,v4,rof1,rof1);
       obspower(tpwr);
    }
   delay(tau);
   zgradpulse(gzlvl1,gt1);
   statusdelay(D,gstab2);

/* phase cycle: .....pw(v2)..d2..pw(v1)..mix...pw(v3)..at(oph)
    (for phase=1 for phase = 2 incr(v2) and incr(v5) )
     
        v1 =[02]16 for axial peaks
        v3 =[0123]2  "4 step phase cycle selection"
       v10 =[01]8   for quad image
       oph = v1+v2+v3+v10

v2: 0 2 0 2 0 2 0 2 0 2 0 2 0 2 0 2 1 3 1 3 1 3 1 3 1 3 1 3 1 3 1 3
v1: 0 0 0 0 0 0 0 0 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 3 3 3 3 3 3 3 3
v3: 0 0 1 1 2 2 3 3 0 0 1 1 2 2 3 3 1 1 2 2 3 3 0 0 1 1 2 2 3 3 0 0
oph:0 2 1 3 2 0 3 1 2 0 3 1 0 2 1 3 1 3 2 0 3 1 0 2 3 1 0 2 1 3 2 0    */
}
