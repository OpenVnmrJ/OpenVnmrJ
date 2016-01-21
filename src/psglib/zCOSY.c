// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
/* zCOSY - z-COSY with optional ZQ suppression
  ref: H. Oschkinat, A. Pastore, P. Pfaendler, G. Bodenhausen;
             J.Magn. Reson. 69 559-566 (1986).
       M.J. Trippleton and J. Keeler;
             Angew. Chem. Int. Ed. 2003, 42 3938-3941.

Parameters:
        sspul   - flag for optional GRD-90-GRD steady state block
        gzlvl1  - gradient power level for sspul
        gt2     - gradient duration for sspul
        satmode - flag for optional presaturation
        satdly  - presaturation delay
        satpwr  - presaturation power level
	flip1	- flip angle of the 1st pulse (calculated from pw)
	flip2	- flip angle of the 2nd and 3rd pulses (calculated from pw)
        antiz_flg - run antiz-COSY
        compH   - 1H amplifier compression factor
        gt2     - homospoil gradient duration after mixing
        gzlvl2  - homospoil gradient level after mixing
        zqflg   - optional flag for ZQ suppression
                  The shaped pulses and gradient powers are
                  calculatred on-the-fly.
        alt_grd - alternate gradient sign(s) in ZQ-filter on even transients
 The parameters: gcal_local, coil_size and h1freq_local necessary
 for ZQ suppression are taken from the probe file by the setup macro or
 calculated automatically.
 The parameter swfactor controling the width of the frequency range to be
 refocused for the ZQ filter is defaulted to 9.0 in the pulse sequence (as
 recommended by the literature reference).
 On the other hand, for very wide spectral windows the inversion range is
 limited to 60 kHZ to prevent dangerously high gradient levels to be set.

p.s. Aug. 2004.
b.h. Aug. 2010. */

#include <standard.h>
#include <chempack.h>

static int ph1[8] = {0,2,0,2,0,2,0,2}, 
           ph2[8] = {0,0,0,0,0,0,0,0},
           ph3[8] = {0,0,1,1,2,2,3,3},
           ph4[8] = {2,0,3,1,0,2,1,3};

static char shapename[MAXSTR];

pulsesequence()
{
   double  gzlvl1 = getval("gzlvl1"),
              gt1 = getval("gt1"),
           zqfpw1 = getval("zqfpw1"),
          zqfpwr1 = getval("zqfpwr1"),
         gzlvlzq1 = getval("gzlvlzq1"),
	    gstab = getval("gstab"),
     h1freq_local = getval("h1freq_local"),
            flip1 = getval("flip1"),
            flip2 = getval("flip2"),
         swfactor = 9.0,    /* do the adiabatic sweep over 9.0*sw  */
         gzlvlzq,invsw;
   int     iphase = (int) (getval("phase") + 0.5),
	 prgcycle = (int)(getval("prgcycle")+0.5);
   char		   satmode[MAXSTR],
		   zqfpat1[MAXSTR],
		   wet[MAXSTR],
		   antiz_flg[MAXSTR],
		   alt_grd[MAXSTR];
           

   getstr("satmode",satmode);
   getstr("wet",wet);
   getstr("zqfpat1",zqfpat1);
   getstr("antiz_flg", antiz_flg);
   getstr("alt_grd",alt_grd);

   invsw = sw*swfactor;
   if (invsw > 60000.0) invsw = 60000.0; /* do not exceed 60 kHz */
   invsw = invsw/0.97;     /* correct for end effects of the cawurst-20 shape */

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

//   sub(ct,ssctr,v12);
   settable(t1,8,ph1);  getelem(t1,v17,v1);
   settable(t2,8,ph2);  getelem(t2,v17,v2);
   settable(t3,8,ph3);  getelem(t3,v17,v3);
   settable(t4,8,ph4);  getelem(t4,v17,oph);

   add(oph,v18,oph);
   add(oph,v19,oph);

   if (alt_grd[0] == 'y') mod2(ct,v8); /* alternate gradient sign */
   if (getflag("Gzqfilt")) add(oph,two,oph);
   if (iphase == 2) incr(v1);

/* HYPERCOMPLEX MODE USES REDFIELD TRICK TO MOVE AXIAL PEAKS TO EDGE */
   initval(2.0*(double)(((int)(d2*getval("sw1")+0.5)%2)),v6);
   if ((iphase==1)||(iphase==2))
      {add(v1,v6,v1); add(oph,v6,oph);}

/* BEGIN THE ACTUAL PULSE SEQUENCE */
   status(A);
   delay(5.0e-5);
   if (getflag("sspul"))
        steadystate();

   if (satmode[0] == 'y')
     {
        if ((d1-satdly) > 0.02)
                delay(d1-satdly);
        else
                delay(0.02);
	if (getflag("slpsat"))
	     {
		shaped_satpulse("relaxD",satdly,v6);
               	if (getflag("prgflg"))
                   shaped_purge(v1,v6,v18,v19);
             }

	else
	     {
        	satpulse(satdly,v6,rof1,rof1);
		if (getflag("prgflg"))
		   purge(v1,v6,v18,v19);
	     }
     }
   else
        delay(d1);

   if (wet[0] == 'y')
     wet4(zero,one);

      obsstepsize(45.0);
      initval(7.0,v7);
      xmtrphase(v7);
   status(B);
      if (antiz_flg[0] == 'n') rgpulse(flip1*pw/90.0,v1,rof1,1.0e-6);
                         else rgpulse(flip1*pw/90.0+2.0*pw,v1,rof1,1.0e-6);
      xmtrphase(zero);
      if (d2 > 0.0)
        {
         if (antiz_flg[0] == 'n') 
           delay(d2-1.0e-6-rof1-SAPS_DELAY-(2.0*pw/3.14159)*(flip1+flip2)/90.0);
         else
           delay(d2-1.0e-6-rof1-SAPS_DELAY-(2.0*pw/3.14159)*(flip1+flip2)/90.0+4.0*pw); 
        }
        else delay(0.0);
      if (antiz_flg[0] == 'n') rgpulse(flip2*pw/90.0,v2,rof1,1.0e-6);
                         else rgpulse(flip2*pw/90.0+2.0*pw,v2,rof1,1.0e-6);
   status(C);
        if (getflag("Gzqfilt"))
        {
         obspower(zqfpwr1);
         rgradient('z',gzlvlzq1);
         delay(100.0e-6);
         shaped_pulse(zqfpat1,zqfpw1,zero,rof1,rof1);
         delay(100.0e-6);
         rgradient('z',0.0);
         delay(gstab);
	 obspower(tpwr);
        }
        ifzero(v8); zgradpulse(gzlvl1,gt1);
              elsenz(v8); zgradpulse(-1.0*gzlvl1,gt1); endif(v8);
        delay(gstab);
   status(D);
      rgpulse(flip2*pw/90.0,v3,rof1,rof2);
}
