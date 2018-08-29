/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
 /* rna_wroesy - transverse cross-relaxation experiment in rotating frame
            with PRESAT, 3919-WATERGATE, or WET water suppression

   	ref:  Shaka, et. al., JACS, 114, 3157 (1992).

	Features included:
		States-TPPI in F1
                Z-Filter option for zero-quantum suppression
		Solvent suppression during relaxation delay (Presat/WET)
		Solvent suppression prior to detection (Watergate/WET)
                45degree phase-shift before t1 to minimize effect of rad. damp.
                 if neither wet nor presat used in relaxation delay
		
	Paramters:
		sspul :		y - selects magnetization randomization
                wet     :       yn- wet suppression at end of d1
                                ny- wet suppression just before acquisition
                                yy- wet suppression in both periods
                wetpwr  :       power level for wet pulse
                wetpw   :       pulse wideth of wet pulse 
                wetshape:       shapelib file for wet pulse
                gswet   :       field recovery delay after wet pulse
                gtw     :       gradient duration in wet sequence
                gzlvlw  :       gradient strength in wet sequence
                flag3919:       y - selects 3919-watergate
                d3      :       interpulse delay in 3919-watergate
                tau     :       spinecho delay in 3919-watergate
		satmode	:	y - selects presaturation during relax
					delay
		satfrq	:	presaturation frequency
		satdly	:	presaturation delay
		satpwr	:	presaturation power
		mix	:	ROESY spinlock mixing time
                strength:       spin-lock field strength (in Hz)
		slpwr	:	spin-lock power level
                                 (derived from "strength")
		slpw	:	90 deg pulse width for spinlock
		d1	:	relaxation delay
		d2	:	Evolution delay

                KK, Varian,June 1997
                GG, Varian, July 1999

*/

#include <standard.h>

/* Chess - CHEmical Shift Selective Suppression */
Chess(pulsepower,pulseshape,duration,phase,rx1,rx2,gzlvlw,gtw,gswet)
double pulsepower,duration,rx1,rx2,gzlvlw,gtw,gswet;
  codeint phase;
  char* pulseshape;
{
  obspwrf(pulsepower);
  shaped_pulse(pulseshape,duration,phase,rx1,rx2);
  zgradpulse(gzlvlw,gtw);
  delay(gswet);
}

/* Wet4 - Water Elimination */
Wet4(phaseA,phaseB)
  codeint phaseA,phaseB;
{
  double finepwr,gzlvlw,gtw,gswet,gswet2,wetpwr,wetpw,dz;
  char   wetshape[MAXSTR];
  getstr("wetshape",wetshape);    /* Selective pulse shape (base)  */
  wetpwr=getval("wetpwr");        /* User enters power for 90 deg. */
  wetpw=getval("wetpw");        /* User enters power for 90 deg. */
  dz=getval("dz");
  finepwr=wetpwr-(int)wetpwr;     /* Adjust power to 152 deg. pulse*/
  wetpwr=(double)((int)wetpwr);
  if (finepwr==0.0) {wetpwr=wetpwr+5; finepwr=4095.0; }
  else {wetpwr=wetpwr+6; finepwr=4095.0*(1-((1.0-finepwr)*0.12)); }
  rcvroff();
  obspower(wetpwr);         /* Set to low power level        */
  gzlvlw=getval("gzlvlw");      /* Z-Gradient level              */
  gtw=getval("gtw");            /* Z-Gradient duration           */
  gswet=getval("gswet");        /* Post-gradient stability delay */
  gswet2=getval("gswet2");        /* Post-gradient stability delay */
  Chess(finepwr*0.5059,wetshape,wetpw,phaseA,20.0e-6,rof2,gzlvlw,gtw,gswet);
  Chess(finepwr*0.6298,wetshape,wetpw,phaseB,20.0e-6,rof2,gzlvlw/2.0,gtw,gswet);
  Chess(finepwr*0.4304,wetshape,wetpw,phaseB,20.0e-6,rof2,gzlvlw/4.0,gtw,gswet);
  Chess(finepwr*1.00,wetshape,wetpw,phaseB,20.0e-6,rof2,gzlvlw/8.0,gtw,gswet2);
  obspower(tpwr); obspwrf(tpwrf);     /* Reset to normal power level   */
  rcvron();
  delay(dz);
}
static int ph1[4] = {1,3,2,0},
           ph2[8] = {1,1,2,2,3,3,0,0},
           ph3[8] = {1,3,2,0,3,1,0,2},
           ph4[8] = {1,1,2,2,3,3,0,0},
           ph5[4] = {3,3,0,0},
           ph6[8] = {1,1,2,2,3,3,0,0};

pulsesequence()
{
   double          slpwr, strength = getval("strength"),
                   slpw, compH = getval("compH"),
                   mix, tau = getval("tau"),
                   gzlvlz,
                   gzlvl0,
                   gtz,
		   satfrq,
		   satpwr,
                   gzlvl1 = getval("gzlvl1"),
                   gt0 = getval("gt0"),
                   gt1 = getval("gt1"),
                   gstab1 = getval("gstab1"),
                   gstab2 = getval("gstab2"),
		   satdly,
                   cycles,
                   wetpw,gtw,gswet;

   int             iphase;

   char            flag3919[MAXSTR],sspul[MAXSTR],
		   satmode[MAXSTR],
		   zfilt[MAXSTR],
                   wet[MAXSTR];


/* LOAD VARIABLES */
  wetpw=getval("wetpw");        /* User enters power for 90 deg. */
  gtw=getval("gtw");            /* Z-Gradient duration           */
  gswet=getval("gswet");        /* Post-gradient stability delay */
   getstr("wet", wet);
   getstr("flag3919",flag3919);
/* LOAD AND INITIALIZE PARAMETERS */
   mix = getval("mix");
   iphase = (int) (getval("phase") + 0.5);
   slpwr = getval("slpwr");
   slpw = getval("slpw");
        satfrq = getval("satfrq");
        satdly = getval("satdly");
        satpwr = getval("satpwr");
        getstr("satmode",satmode);

   getstr("sspul", sspul);
   gzlvl0 = getval("gzlvl0");
   gzlvlz = getval("gzlvlz");
   gtz = getval("gtz");
   getstr("zfilt",zfilt);

   sub(ct,ssctr,v7);

   settable(t1,4,ph1);	getelem(t1,v7,v1);
   settable(t2,8,ph2);	getelem(t2,v7,v2);	add(v2,two,v3);
   settable(t3,8,ph3);	getelem(t3,v7,oph);
   settable(t4,8,ph4);	getelem(t4,v7,v4);
   settable(t5,4,ph5);	getelem(t5,v7,v5);
   settable(t6,8,ph6);	getelem(t5,v7,v6);
   
   if (zfilt[0] == 'n') assign(v1,oph);

   if (iphase == 2)
      {incr(v1); incr(v6);}			/* hypercomplex method */

   initval(2.0*(double)(((int)(d2*getval("sw1")+0.5)%2)),v13);
       add(v1,v13,v1);
       add(v6,v13,v6);
       add(oph,v13,oph);

   /* power level and pulse time for spinlock */
   slpw = 1/(4.0 * strength) ;           /* spinlock field strength  */
   slpwr = tpwr - 20.0*log10(slpw/(compH*pw));
   slpwr = (int) (slpwr + 0.5);


   cycles = mix / (4.0 * slpw);
   initval(cycles, v10);	/* mixing time cycles */


/* BEGIN ACTUAL PULSE SEQUENCE */
   status(A);
      obspower(tpwr);
      delay(5.0e-6);
   if (sspul[0] == 'y')        /* randomizes all magnetizations */
     {
         zgradpulse(gzlvl0,gt0);
         rgpulse(pw,zero,rof1,rof1);
         zgradpulse(0.8*gzlvl0,gt0);
     }
   else
         zgradpulse(gzlvl0,gt0);                   /* does homospoil */
   if (wet[A] == 'y')
    {
      delay(d1-4.0*wetpw-4.0*gswet);
      Wet4(zero,one);                     /* WET suppression in d1 */
    }
   else
      delay(d1);                          /* relaxation delay */

   if (satmode[0] == 'y')
    {
     if (satfrq != tof)
       obsoffset(satfrq);
     obspower(satpwr);
     rgpulse(satdly,v6,rof1,rof1);
     obspower(tpwr);
     if (satfrq != tof)
      obsoffset(tof);
    }
   if ((satmode[0] == 'n') || (wet[0] == 'n'))
    {
     obsstepsize(45.0);        /* H2O has large magnetization */
     initval(7.0,v9);          /* 45 degree phase shift treats phase=1 */
     xmtrphase(v9);            /* the same as phase=2 */
    }

   status(B);
      rgpulse(pw, v1, rof1, rof1);
      if (d2 > 0.0)
       {
        if ((satmode[0] == 'y') || (wet[0] == 'y'))
         {
          delay(d2 - SAPS_DELAY -POWER_DELAY - (2*pw/PI) - rof1);
          xmtrphase(zero);
         }
        else
          delay(d2 - POWER_DELAY - (2*pw/PI) - rof1);
       }
      obspower(slpwr);

      if (cycles > 1.5000)
       {
         rcvroff();
	 xmtron();
         starthardloop(v10);
		txphase(v2);
		delay(2*slpw);
		txphase(v3);
		delay(2*slpw);
         endhardloop();
	 xmtroff();
         rcvron();
       }

       if (zfilt[0] == 'y')
        {
           obspower(tpwr);
           rgpulse(pw,v4,1.0e-6,rof1);
           zgradpulse(gzlvlz,gtz);
           delay(gtz/3);
           if (wet[B] == 'y') Wet4(zero,one);
           rgpulse(pw,v5,rof1,0.0);
        }

       if (flag3919[A] == 'y')
        {
         add(v5,two,v8);
         rgpulse(pw, v5,rof1,rof1);
         delay(tau-pw/2-rof1);
         zgradpulse(gzlvl1,gt1);
         delay(gstab1);
         pulse(pw*0.231,v5);
         delay(d3);
         pulse(pw*0.692,v5);
         delay(d3);
         pulse(pw*1.462,v5);
         delay(d3);
         pulse(pw*1.462,v8);
         delay(d3);
         pulse(pw*0.692,v8);
         delay(d3);
         pulse(pw*0.231,v8);
         delay(tau);
         zgradpulse(gzlvl1,gt1);
         delay(gstab2);
        }

   status(C);
}
