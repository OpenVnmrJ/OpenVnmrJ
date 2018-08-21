/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
 /* wroesy - transverse cross-relaxation experiment in rotating frame
            with PRESAT, 3919-WATERGATE, or WET water suppression

   	ref:  Shaka, et. al., JACS, 114, 3157 (1992).

	Features included:
		States-TPPI in F1
                Z-Filter option for zero-quantum suppression
                T-ROESY option
		Solvent suppression during relaxation delay (Presat/WET)
		Solvent suppression prior to detection (Watergate/WET)
                45degree phase-shift before t1 to minimize effect of rad. damp.
                 if neither wet nor presat used in relaxation delay
		
	Paramters:
		sspul :		y - selects magnetization randomization
                T_flg :         Troesy option
                wet     :       yn- wet suppression at end of d1
                                ny- wet suppression just before acquisition
                                yy- wet suppression in both periods
                wetpwr  :       power level for wet pulse
                wetpw   :       pulse wideth of wet pulse 
                wetshape:       shapelib file for wet pulse
                autosoft:       y-automatic power calculation for wet
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
                                  determines power and 180 pulse widths in mix
		d1	:	relaxation delay
		d2	:	Evolution delay

                KK, Varian,June 1997
                GG, Varian, July 1999
                GG, Varian, Sept 2000 (added autosoft option)
                    Included option for alternating gradient sign for
                    odd/even transients respectively, Ryan McKay,
                    NANUC Aug.2011 

*/

#include <standard.h>

/* Chess - CHEmical Shift Selective Suppression */
static void Chess(pulsepower,pulseshape,duration,phase,rx1,rx2,gzlvlw,gtw,gswet)
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
static void Wet4(pulsepower,wetshape,duration,phaseA,phaseB)
  double pulsepower,duration;
  codeint phaseA,phaseB;
  char* wetshape;
{
  double wetpw,finepwr,gzlvlw,gtw,gswet;
  gzlvlw=getval("gzlvlw"); gtw=getval("gtw"); gswet=getval("gswet");
  wetpw=getval("wetpw");
  finepwr=pulsepower-(int)pulsepower;     /* Adjust power to 152 deg. pulse*/
  pulsepower=(double)((int)pulsepower);
  if (finepwr == 0.0) {pulsepower=pulsepower+5; finepwr=4095.0; }
  else {pulsepower=pulsepower+6; finepwr=4095.0*(1-((1.0-finepwr)*0.12)); }
  rcvroff();
  obspower(pulsepower);         /* Set to low power level        */
  Chess(finepwr*0.6452,wetshape,wetpw,phaseA,20.0e-6,rof1,gzlvlw,gtw,gswet);
  Chess(finepwr*0.5256,wetshape,wetpw,phaseB,20.0e-6,rof1,gzlvlw/2.0,gtw,gswet);
  Chess(finepwr*0.4928,wetshape,wetpw,phaseB,20.0e-6,rof1,gzlvlw/4.0,gtw,gswet);
  Chess(finepwr*1.00,wetshape,wetpw,phaseB,20.0e-6,rof1,gzlvlw/8.0,gtw,gswet);
  obspower(tpwr); obspwrf(tpwrf);  /* Reset to normal power level   */
  rcvron();
}

Wet4B(pulsepower,wetshape,duration,phaseA,phaseB)
  double pulsepower,duration;
  codeint phaseA,phaseB;
  char* wetshape;
{
  double wetpw,finepwr,gzlvlw,gtw,gswet;
  gzlvlw=0.94*getval("gzlvlw"); gtw=getval("gtw"); gswet=getval("gswet");
  wetpw=getval("wetpw");
  finepwr=pulsepower-(int)pulsepower;     /* Adjust power to 152 deg. pulse*/
  pulsepower=(double)((int)pulsepower);             
  if (finepwr == 0.0) {pulsepower=pulsepower+5; finepwr=4095.0; }
  else {pulsepower=pulsepower+6; finepwr=4095.0*(1-((1.0-finepwr)*0.12)); }
  rcvroff();
  obspower(pulsepower);         /* Set to low power level        */
  Chess(finepwr*0.6452,wetshape,wetpw,phaseA,20.0e-6,rof1,gzlvlw,gtw,gswet);
  Chess(finepwr*0.5256,wetshape,wetpw,phaseB,20.0e-6,rof1,gzlvlw/2.0,gtw,gswet);
  Chess(finepwr*0.4928,wetshape,wetpw,phaseB,20.0e-6,rof1,gzlvlw/4.0,gtw,gswet);
  Chess(finepwr*1.00,wetshape,wetpw,phaseB,20.0e-6,rof1,gzlvlw/8.0,gtw,gswet);
  obspower(tpwr); obspwrf(tpwrf);  /* Reset to normal power level   */
  rcvron();
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
                   gstab = getval("gstab"),
		   satdly,
                   cycles,
                   wetpw=getval("wetpw"),dz=getval("dz"),
                   wetpwr=getval("wetpwr"),gtw,gswet;

   int             iphase;

   char            flag3919[MAXSTR],sspul[MAXSTR],
                   autosoft[MAXSTR],wetshape[MAXSTR],
		   satmode[MAXSTR], T_flg[MAXSTR],
		   zfilt[MAXSTR], alt_grd[MAXSTR],
                   wet[MAXSTR];


/* LOAD VARIABLES */
  wetpw=getval("wetpw");        /* User enters power for 90 deg. */
  gtw=getval("gtw");            /* Z-Gradient duration           */
  gswet=getval("gswet");        /* Post-gradient stability delay */
   getstr("wetshape", wetshape);
   getstr("autosoft", autosoft);
   getstr("wet", wet);
   getstr("T_flg", T_flg);
   getstr("flag3919",flag3919);
   getstr("alt_grd",alt_grd);   /* alternating gradients flag */

/* LOAD AND INITIALIZE PARAMETERS */
   mix = getval("mix");
   iphase = (int) (getval("phase") + 0.5);
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


   if (alt_grd[0] == 'y') mod2(ct,v11);
  
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
      if (autosoft[A] == 'y') 
       { 
           /* selective H2O one-lobe sinc pulse */
        wetpwr = tpwr - 20.0*log10(wetpw/(pw*compH*1.69));  /* sinc needs 1.69 times more */
        wetpwr = (int) (wetpwr +0.5);                       /* power than a square pulse */
        Wet4(wetpwr,"H2Osinc",wetpw,zero,one); delay(dz); 
       } 
      else
       Wet4(wetpwr,wetshape,wetpw,zero,one); delay(dz); 
       delay(1.0e-4);
       obspower(tpwr);
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
   if ((satmode[0] == 'n') && (wet[0] == 'n'))  /* H2O has large magnetization */
    {
     obsstepsize(45.0);        
     initval(7.0,v9);          /* 45 degree phase shift treats phase=1 */
     xmtrphase(v9);            /* the same as phase=2 */
    }

   status(B);
      rgpulse(pw, v1, rof1, rof1);
      if (d2 > 0.0)
       {
        if ((satmode[0] == 'n') && (wet[0] == 'n'))
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
         if (T_flg[0] == 'y')
         {
            starthardloop(v10);
		txphase(v2);
		delay(2*slpw);
		txphase(v3);
		delay(2*slpw);
            endhardloop();
         }
         else
           delay(mix);
	 xmtroff();
         rcvron();
         delay(rof2);
       }

       if (zfilt[0] == 'y')
        {
           obspower(tpwr);
           rgpulse(pw,v4,1.0e-6,rof1);
           zgradpulse(gzlvlz,gtz);
           delay(gtz/3);
           if (wet[B] == 'y')
            {
            if (autosoft[A] == 'y') 
              { 
                 /* selective H2O one-lobe sinc pulse */
               wetpwr = tpwr - 20.0*log10(wetpw/(pw*compH*1.69));  /* sinc needs 1.69 times more */
               wetpwr = (int) (wetpwr +0.5);                       /* power than a square pulse */
               Wet4B(wetpwr,"H2Osinc",wetpw,zero,one); delay(dz); 
              } 
              else
              {
               Wet4B(wetpwr,wetshape,wetpw,zero,one); delay(dz); 
               delay(1.0e-4);
               obspower(tpwr);
              }
            }
            rgpulse(pw,v5,rof1,rof2);
        }

       if (flag3919[A] == 'y')
        {
         obspower(tpwr);
         add(v5,two,v8);
         delay(tau-rof2);
         zgradpulse(gzlvl1,gt1);
         delay(gstab);
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
         delay(gstab);
        }

   status(C);
}
