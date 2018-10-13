/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*  

 soggy (an option in Exitation Sculpting (dpfgse='y'))

dpfgse='y'
   This adds a double pulsed-field gradient spin echo pair after the last pulse
   for shaped='n', presat='y'. The spin echos include a selective 180 degree pulse
   on water (typ 1-2msec) which is controlled by pwpwr, pwshape and flippw. This
   may be used for water suppression without presat.
     See Hwang and Shaka, JMR (A), 112, 275(1995).

soggy='y'
   This replaces a simple 180 in the dpfgse sequence with a composite pulse 180. See
   Nguyen, Meng, Donovan and Shaka, JMR, 184, 263-274(2007). Must be used with
   dpfgse='y'.


Other Options:

   This pulse sequence has options for the most popular water suppression
methods. Each option has its own dedicated parameters which become visible
in "dg" when the option has the value 'y'. No attempt is made in "dg" to
prevent more than one method from being used (neither is there any attemp
in psg code). Users could either modify the code or modify (or create) a
usergo macro that would check on the values of options and abort acquisition
with message if more than one option is accidentally selected.

Presat='y'
   Performs classic observe xmter presat with optional pfg homospoil(sspul='y').  
   Homospoil is controlled by a gradient time of gt0 (sec) at level gzlvl0.
   Satmode is used like "dm" to select periods of saturation at frequency
   satfrq, power satpwr, for duration d1. A composite 90 option is
   excercised when composit='y'. Sequence is essentially s2pul for d1,p1,d2,pw.
   The delay d0 can be used for any additional time before the d1 delay.

Scuba='y'
   When presat='y', scuba may be used to restore some "bleached" proton      
   intensity for those protons near in shift to the water. 

purge='y'
   Does a version of a presat experiment having superior suppression.
   Literature: Andre J. Simpson and Sarah A. Brown, JMR, 175, 340-346(2005).

Shaped='y'
   Same as presat='y' but all rf controlled by waveform generator. Useful for
   calibrating shaped pulses even in non-suppression experiments. 
   The p1 pulse is at p1frq, of shape p1shape, at power p1pwr and for
   duration p1. The second pulse is of length pw, at frequency tof, at power
   pwpwr and of shape pwshape. The solvent saturation is optional during
   relaxation and d2 delays under satmode control (like "dm") at satfrq
   with power satpwr and shape satshape (use "hard" for normal operation).

dpfgse='y'
   This adds a double pulsed-field gradient spin echo pair after the last pulse
   for shaped='n', presat='y'. The spin echos include a selective 180 degree pulse
   on water (typ 1-2msec) which is controlled by pwpwr, pwshape and flippw. This
   may be used for water suppression without presat. 
     See Hwang and Shaka, JMR (A), 112, 275(1995).

soggy='y'
   This replaces a simple 180 in the dpfgse sequence with a composite pulse 180. See
   Nguyen, Meng, Donovan and Shaka, JMR, 184, 263-274(2007). Must be used with 
   dpfgse='y'.

mfsat='y'
           Multi-frequency saturation.
           Requires the frequency list mfpresat.ll in current experiment
           Pbox creates (saturation) shape "mfpresat.DEC"

                  use mfll('new') to initialize/clear the line list
                  use mfll to read the current cursor position into
                  the mfpresat.ll line list that is created in the
                  current experiment.

           Note: copying pars or fids (mp or mf) from one exp to another does
                 not copy mfpresat.ll!
           Note: the line list is limited to 128 frequencies !

            G.Gray, Varian, Palo Alto  November 1999
            E. Hoffmann,Varian, Darmstadt- presat phase cycle
            G.Gray, Varian, Palo Alto  September 2003 -
              modifications of watergate
              to permit fine power adjustment and use of phase-corrected shapes
            G.Gray, Varian, Palo Alto June 2005 - added dpfgse option following
              watergate( from Alan Kenwright, U.Durham (UK)
            E.Kupce, Varian, UK June 2005 - added multifrequency presat option


Jumpret='y'
   Does a simple jump-and-return experiment with pulses jrp1 and jrpw
   separated by delay jrdelay. jrpw is usually 25ns or so shorter for
   best suppression. Vary tof over +-100 Hz to find best suppression.
   sspul='y' does gradient homospoil prior to d1, with different lengths
   of homospoil for each transient to avoid gradient-recalled echos.

   For Literature discussion: see P.J. Hore, J.Magn.Reson.,55,283-300(1983)

purge='y'
   Does a version of a presat experiment having superior suppression.

   Literature: Andre J. Simpson and Sarah A. Brown, JMR, 175, 340-346(2005).

Wet='y'
   Does a presat experiment in which the presat can take only 20-100msec.
   After d1 there is a series of four selective pulses on water, each 
   followed by a gradient. This can achieve very good suppression and
   also be used for multiple line suppression by making the selective
   pulse multifrequency (use convolute macro or Pbox). The gradient is
   controlled by the time gtw at level gzlvlw. The selective pulse
   wetshape is at a power level wetpwr for a time wetpw. wetpw should be
   calibrated as a 90 degree pulse length for the shape wetshape and power
   wetpwr. The wet "pulse" is broken up into 4 individual pulses each
   followed by different gradients. In some cases use of a composite pulse
   will produce better suppression (composit='y').

   Literature: S.Smallcombe, S.L.Patt and P.A. Keifer, J.Magn.Reson.A,117,295(1995).

Watergate='y'
   This may be done in either a soft-pulse(flagsoft='y') or hard-pulse 
   (flag3919='y' or flagW5='y') version.

   The water-selective soft pulse can be of any desired shape. If autosoft='y',the
   power is calculated by the psg code (always a "sinc") based on pw and compH.
   The soft pulse width is user-enterable. This soft pulse may be used for
   flipback purposes for all methods of watergate. It is also used within
   the spin-echo if flagsoft='y'.

   A soft pulse prior to the first high power pulse is done when flipback='y',
   for purposes of keeping water along Z during d1 and at (to reduce 
   intensity losses of other protons via chemical exchange with water.
   Further empirical improvement in suppression can be had by arraying
   phincr1 and phincr2 from -10 to 10. Fine adjustment of the 180 is
   controlled by p180 (vary for best suppression). flippw should be short
   enough so relaxation is no problem, but long enough so that nearby
   protons are not suppressed as well (suggest 2-5msec).

   If autosoft='y' no small-angle phase correction is applied. The pulse
   sequence, in this case, uses H2Osinc_u.RF for flipup pulses and H2Osinc_d.RF
   for flipdown sequences. These shapes have internal phase corrections determined
   in the BioPack autocalibrate procedure (when the shapes are created). Each of
   these pulses also have fine power adjustments prior to the pulses (tpwrsf_u and
   tpwrsf_d) which also have been determined in the BioPack autocalibrate process
   for best water suppression. If autosoft='n' the shape used is under user control and
   phincr1 and phincr2 are active for small-angle phase corrections for the soft
   pulses. Fine power control is still active with tpwrsf_u and tpwrsf_d.

   The tpwrsf_u and tpwrsf_d values are stored in the probefile and are determined
   in the BioPack AutoCalibrate procedure. Fine power adjustments in all cases are
   only active if tpwrsf_u<4095.0.

swet='y'
	"Secure wet". Cold probe/high Q probe specific modification of the WET water
	 suppression sequence using a train of small flip angle pulses instead of the
	 standard shaped pulse, interleaved with bipolar gradients.  See ref below.
	 
	 Recommend approprimate swetpwr for swetpw*90 = 90 degree pulse and then 
	 fine tune pulse length for water minimization.  Paper recommend 1.4us swetpw
	 We found 1.5kHz was min on high field instruments.
         (submitted by Ryan McKay, NANUC,U.Alberta)
	

   Literature: soft-pulse  Piotto et.al., J.Biomol.NMR, 2,661(1992)
               hard-pulse  Sklenar et.al., J.Magn.Res., 102, 241(1993)
                       W5  Liu et.al, J.Magn.Res., 132, 125(1998)
		SWET	We and Otting, J. Biomol. NMR, 32, 243-250 (2005)
            PURGE       Andre J. Simpson and Sarah A. Brown, JMR, 175, 340-346(2005).

            G.Gray, Varian, Palo Alto  November 1999 
            E. Hoffmann,Varian, Darmstadt- presat phase cycle
            G.Gray, Varian, Palo Alto  September 2003 - modifications of watergate
              to permit fine power adjustment and use of phase-corrected shapes 
            G.Gray, Varian, Palo Alto  October 2005 - added PURGE
            N.Murali, Varian, Palo Alto December 2006 - added SOGGY option to dpfgse 

*/
#include <standard.h>
#include "mfpresat.h"

/* Chess - CHEmical Shift Selective Suppression */
static void Chess(pulsepower,pulseshape,duration,phase,rx1,rx2,gzlvlw,gtw,gstab)
  double pulsepower,duration,rx1,rx2,gzlvlw,gtw,gstab;
  codeint phase;
  char* pulseshape;
{
  obspwrf(pulsepower);
  shaped_pulse(pulseshape,duration,phase,rx1,rx2);
  zgradpulse(gzlvlw,gtw);
  delay(gstab);
}


/* Wet4 - Water Elimination */
static void Wet4(pulsepower,wetshape,duration,phaseA,phaseB)
  double pulsepower,duration;
  codeint phaseA,phaseB;
  char* wetshape;
{
  double wetpw,finepwr,gzlvlw,gtw,gstab;
  gzlvlw=getval("gzlvlw"); gtw=getval("gtw"); gstab=getval("gstab");
  wetpw=getval("wetpw");
  finepwr=pulsepower-(int)pulsepower;     /* Adjust power to 152 deg. pulse*/
  pulsepower=(double)((int)pulsepower);
  if (finepwr == 0.0) {pulsepower=pulsepower+5; finepwr=4095.0; }
  else {pulsepower=pulsepower+6; finepwr=4095.0*(1-((1.0-finepwr)*0.12)); }
  rcvroff();
  obspower(pulsepower);         /* Set to low power level        */
  Chess(finepwr*0.6452,wetshape,wetpw,phaseA,20.0e-6,rof1,gzlvlw,gtw,gstab);
  Chess(finepwr*0.5256,wetshape,wetpw,phaseB,20.0e-6,rof1,gzlvlw/2.0,gtw,gstab);
  Chess(finepwr*0.4928,wetshape,wetpw,phaseB,20.0e-6,rof1,gzlvlw/4.0,gtw,gstab);
  Chess(finepwr*1.00,wetshape,wetpw,phaseB,20.0e-6,rof1,gzlvlw/8.0,gtw,gstab);
  obspower(tpwr); obspwrf(tpwrf);  /* Reset to normal power level   */
  rcvron();
}

composite_pulse(width,phasetable,rx1,rx2,phase)
  double width,rx1,rx2;
  codeint phasetable,phase;
{
  getelem(phasetable,ct,phase); /* Extract observe phase from table */
  incr(phase); rgpulse(width,phase,rx1,rx1);  /*  Y  */
  incr(phase); rgpulse(width,phase,rx1,rx1);  /* -X  */
  incr(phase); rgpulse(width,phase,rx1,rx1);  /* -Y  */
  incr(phase); rgpulse(width,phase,rx1,rx2);  /*  X  */
}


static int phi1[4] = {0,2,1,3},
           phi2[8] = {1,1,0,0,3,3,2,2},
           phi3[1] = {0},
           phi4[8] = {1,1,2,2,3,3,0,0},
	   phi5[4] = {90,112,78,178},  
          phi11[4] = {0,0,2,2},
	 phi12[16] = {0,0,0,0,2,2,2,2,1,1,1,1,3,3,3,3},
         phi13[16] = {0,2,0,2,2,0,2,0,1,3,1,3,3,1,3,1},
         phi14[16] = {0,2,2,0,2,0,0,2,3,1,1,3,1,3,3,1};

pulsesequence()
{
  char   fsqd[MAXSTR], presat[MAXSTR], scuba[MAXSTR], jumpret[MAXSTR],
         dpfgse[MAXSTR],shaped[MAXSTR], wet[MAXSTR],  watergate[MAXSTR],
         purge[MAXSTR], flagW5[MAXSTR], flagsoft[MAXSTR], autosoft[MAXSTR],
         composit[MAXSTR], flipback[MAXSTR], sspul[MAXSTR], mfsat[MAXSTR],
         wetshape[MAXSTR], flipshap[MAXSTR], flag3919[MAXSTR], soggy[MAXSTR],
         p1shape[MAXSTR], pwshape[MAXSTR], satshape[MAXSTR], swet[MAXSTR];   
         
  double   gzlvl0=getval("gzlvl0"),  /* used for homospoil only          */
           gzlvl1=getval("gzlvl1"),  /* used in watergate                */
           gzlvl2=getval("gzlvl2"),  /* used in purge                    */
           gzlvl3=getval("gzlvl3"),  /* used in purge                    */
           gzlvl4=getval("gzlvl4"),  /* used in purge                    */
             tau1=getval("tau1"),    /* used in purge                    */
             tau2=getval("tau2"),    /* used in purge                    */
              gt0=getval("gt0"),     /* used for homospoil in presat     */
              gt1=getval("gt1"),     /* used in watergate                */
              gt2=getval("gt2"),     /* used in purge                    */
              gt3=getval("gt3"),     /* used in purge                    */
              gt4=getval("gt4"),     /* used in purge                    */
              gstab=getval("gstab"), /* used in watergate                */
            compH=getval("compH"),   /* used in watergate                */
          flippwr=getval("flippwr"), /* used in watergate                */
          flippw=getval("flippw"),   /* used in watergate                */
            wetpwr=getval("wetpwr"), /* used in wet                      */
             wetpw=getval("wetpw"),  /* used in wet                      */
            p1pwr=getval("p1pwr"),   /* used in shaped pulse presat      */
            pwpwr=getval("pwpwr"),   /* used in shaped pulse presat      */
      tpwrsf_u=getval("tpwrsf_u"),   /* used in watergate                */
      tpwrsf_d=getval("tpwrsf_d"),   /* used in watergate                */
            tofwg=getval("tofwg"),   /* used in watergate                */
          phincr1=getval("phincr1"), /* used in watergate                */
          phincr2=getval("phincr2"), /* used in watergate                */
             p180=getval("p180"),    /* used in watergate                */
               dz=getval("dz"),      /* used in wet                      */
               d3=getval("d3"),      /* used in watergate/scuba          */
          oslsfrq=getval("oslsfrq"), /* used in "dqd"                    */
          jrdelay=getval("jrdelay"), /* used in jumpreturn               */
             jrp1=getval("jrp1"),    /* used in jumpreturn               */
             jrpw=getval("jrpw"),    /* used in jumpreturn               */
            p1frq=getval("p1frq"),   /* used in shaped pulse presat      */
	    swetpw=getval("swetpw"), 	/* ~1 degree pulse at swetpwr	*/
	    swetpwr=getval("swetpwr"),	/* power for swet pulse train	*/
	    gzlvl_swet=getval("gzlvl_swet"), 	/* 10G/cm gradient pulse */
	    gt_swet=getval("gt_swet"), 		/* length of swet gradient */
            hpw = 0.0,
	    hpwr = 0.0;
	    

           getstr("purge",purge);
           getstr("wet",wet);
           getstr("mfsat",mfsat);
           getstr("dpfgse",dpfgse);
           getstr("presat",presat);
           getstr("scuba",scuba);
           getstr("jumpret",jumpret);
           getstr("shaped",shaped);
           getstr("watergate",watergate);
           getstr("autosoft",autosoft);
           getstr("flagsoft",flagsoft);
           getstr("flagW5",flagW5);
           getstr("flag3919",flag3919);
           getstr("composit",composit); /* used in presat/wet               */
           getstr("flipshap",flipshap); /* used in watergate                */
           getstr("flipback",flipback); /* used in watergate                */
           getstr("sspul",sspul);       /* used in presat                   */
           getstr("wetshape",wetshape); /* used in wet                      */
           getstr("p1shape",p1shape);   /* used in shaped pulse presat      */
           getstr("pwshape",pwshape);   /* used in shaped pulse presat      */
           getstr("satshape",satshape); /* used in shaped pulse presat      */
           getstr("fsqd",fsqd);         /* for "dqd"                        */
	   getstr("swet",swet);		/* used in swet sequence 	*/
	   getstr("soggy",soggy);       /* used for soggy composite in DPFGSE */

/*  ******************************************************************** */

  obsoffset(tof);
  obspower(tpwr);


 if (purge[A] == 'y')
  {
  settable(t11,4,phi11);
  settable(t12,16,phi12);
  settable(t13,16,phi13);
  settable(t14,16,phi14);
    if (satpwr > 12)
      {printf("satpwr too large - acquisition aborted./n"); psg_abort(1); }

    status(A);
      zgradpulse(gzlvl0,gt0);
      if (sspul[A] =='y')
      {
       zgradpulse(2.2*gzlvl0,gt0);
       rgpulse(pw90,zero,rof1,0.0); 
       zgradpulse(1.1*gzlvl0,gt0);
      }
      if (satmode[A] == 'y') 
       {
        
        obspower(satpwr);
        rgpulse(d1,zero,rof1,rof1); 
        obspower(tpwr);
       }
       else
        delay(d1);

    status(B);
      rgpulse(pw,zero,rof1,rof1);
      obspower(satpwr);
      rgpulse(tau1,zero,rof1,rof1);
      obspower(tpwr);
      rgpulse(2.0*pw,zero,rof1,rof1);
      obspower(satpwr);
      rgpulse(tau1,zero,rof1,rof1);
      obspower(tpwr);
      rgpulse(pw,t11,rof1,rof1);
      zgradpulse(-1.0*gzlvl1,gt1);
      delay(gstab);
      obspower(satpwr);
      rgpulse(tau2,zero,rof1,rof1); 
      obspower(tpwr);
      zgradpulse(gzlvl2,gt2);
      delay(gstab);
      rgpulse(pw,t12,rof1,rof1);
      obspower(satpwr);
      rgpulse(tau1,zero,rof1,rof1);
      obspower(tpwr);
      rgpulse(2.0*pw,zero,rof1,rof1);
      obspower(satpwr);
      rgpulse(tau1,zero,rof1,rof1);
      obspower(tpwr);
      rgpulse(pw,t13,rof1,rof1);
      zgradpulse(-1.0*gzlvl3,gt3);
      delay(gstab);
      obspower(satpwr);
      rgpulse(tau2,zero,rof1,rof1); 
      obspower(tpwr);
      zgradpulse(gzlvl4,gt4);
      delay(gstab);
      rgpulse(pw,t12,rof1,rof2);
      setreceiver(t14);
}

 if (presat[A] == 'y')
  {
  settable(t1,4,phi1);
  settable(t2,8,phi2);
  settable(t3,1,phi3);
  settable(t4,8,phi4);
  settable(t5,4,phi5);
  
  sub(ct,ssctr,v12);
  getelem(t1,v12,oph);
  getelem(t2,v12,v1);
  getelem(t3,v12,v5);
  getelem(t4,v12,v6);

    if (satpwr > 25)
      {printf("satpwr too large - acquisition aborted./n"); psg_abort(1); }

    status(A);
      if (sspul[A] =='y')
      {
       zgradpulse(2.2*gzlvl0,gt0);
       rgpulse(pw90,zero,rof1,0.0); 
       zgradpulse(1.1*gzlvl0,gt0);
      }
      if (satmode[A] == 'y') 
       {
        if (mfsat[A] == 'y')
          {obsunblank(); mfpresat_on(); delay(satdly); mfpresat_off();obspower(tpwr); obsblank();} 
        else
        {
         if (tof != satfrq) obsoffset(satfrq);
         obspower(satpwr);
         /* rgpulse(d1,v6,rof1,rof1); */
         rgpulse(d1,zero,0.0,0.0); 
         obspower(tpwr);
         if (tof != satfrq) obsoffset(tof);
        }
       }
       else
        delay(d1);

      if ((scuba[A] == 'y') && (satmode[A] == 'y'))
       {
        add(v6,one,v7);
        delay(d3);
        rgpulse(pw,v6,rof1,0.0);
        rgpulse(2.0*pw,v7,rof1,0.0);
        rgpulse(pw,v6,rof1,0.0);
        txphase(zero);
        delay(d3);
       } 
    status(B);
      rgpulse(p1,zero,rof1,rof1);
      if (satmode[B] == 'y') 
       {
        if (mfsat[B] == 'y')
         {obsunblank(); mfpresat_on(); delay(d2); mfpresat_off(); obspower(tpwr); obsblank();}
        else
        {
         if (tof != satfrq) obsoffset(satfrq);
         obspower(satpwr); 
         rgpulse(d2,v6,rof1,rof1); 
         obspower(tpwr);
         if (tof != satfrq) obsoffset(tof);
        }
       }
      else 
       delay(d2);
      if (composit[A] == 'y')
       {
       add(oph,one,v2); add(oph,two,v3); add(oph,three,v4);
       rgpulse(pw,v2,rof1,1.0e-6);   /* 90(+y)90(-x)90(-y)90(x) */   
       rgpulse(pw,v3,0.0,1.0e-6);
       rgpulse(pw,v4,0.0,1.0e-6);
       rgpulse(pw,oph,0.0,rof2);
       }
     else
       rgpulse(pw,oph,rof1,rof2);
     if (dpfgse[A] == 'y')
       {
        if (autosoft[A] == 'y')
         {
              /* selective H2O rectangular 180 degree pulse */
           pwpwr = tpwr - 20.0*log10(flippw/(pw*compH)) + 6.0; 
           pwpwr = (int) (pwpwr +0.5);  
         }
        else
         {
          if (pwpwr > (tpwr-20)) 
          {
            printf("dpfgse shaped pulse power too large - acquisition aborted./n");
            psg_abort(1); 
          }
         }
        if (soggy[A] == 'y')
        {
          hpw = (40.0e-6)*500.0/sfrq;
          hpwr = (int) (tpwr-20.0*log10(hpw/(pw*compH)) + 0.5);
        }
        add(oph,two,v9);mod4(v9,v9);
        zgradpulse(gzlvl0*1.8,gt0);
        delay(gstab);
        obspower(pwpwr);
        if (autosoft[A] == 'y')
	 shaped_pulse("hard",flippw,oph,rof1,rof1); 
        else
	 shaped_pulse(pwshape,flippw,oph,rof1,rof1);
        if (soggy[A] == 'y')
        {
         obspower(hpwr);
         rgpulse(hpw*81.0/90.0,v9,rof1,0.0);
         rgpulse(hpw*81.0/90.0,oph,0.0,0.0);
         rgpulse(hpw*342.0/90.0,v9,0.0,0.0);
         rgpulse(hpw*162.0/90.0,oph,0.0,0.0);
         
        }
        else
        {
 
        obspower(tpwr);
        rgpulse(2*pw,v9,rof1,rof1);
        }
        zgradpulse(gzlvl0*1.8,gt0);
        delay(gstab); 
        zgradpulse(gzlvl1,gt1);
        obspower(pwpwr);
        delay(gstab); 
        if (autosoft[A] == 'y')
	 shaped_pulse("hard",flippw,oph,rof1,rof1); 
        else
	 shaped_pulse(pwshape,flippw,oph,rof1,rof1); 
        if (soggy[A] == 'y')
        {
         obspower(hpwr);
         rgpulse(hpw*81.0/90.0,v9,rof1,0.0);
         rgpulse(hpw*81.0/90.0,oph,0.0,0.0);
         rgpulse(hpw*342.0/90.0,v9,0.0,0.0);
         rgpulse(hpw*162.0/90.0,oph,0.0,0.0);
        }
        else
        {
        obspower(tpwr);
        rgpulse(2*pw,v9,rof1,rof1);
        }
        zgradpulse(gzlvl1,gt1);
        delay(gstab);  
       }
 if (fsqd[A] == 'y') obsoffset(tof+oslsfrq);
 status(C);
   }
/* **************************************************************** */
if (jumpret[A] == 'y')
  {
   mod4(ct,v2); incr(v2); dbl(v2,v2);   /* v2=2,4,6,8  */
   gt0=gt0/8.0;
   add(oph, two, v1);
   status(A);
      if (sspul[A] =='y')
       {
       starthardloop(v2);
        zgradpulse(gzlvl0,gt0); /* 1,2,3,or 4 homospoil pulses  */
       endhardloop();
       }
      txphase(oph);
      delay(d1);
      rgpulse(jrp1, oph, rof1,rof1);
      txphase(v1);
      delay(jrdelay - jrpw);
      rgpulse(jrpw, v1, rof1, rof2);
   status(B);
 if (fsqd[A] == 'y') obsoffset(tof+oslsfrq);
   }

/* **************************************************************** */
/* phase table for WET portion of water.c

t1 = (0 2 0 2)4 (1 3 1 3)4 (2 0 2 0)4 (3 1 3 1)4 
t2 = (0 0 2 2)4 (1 1 3 3)4 (2 2 0 0)4 (3 3 1 1)4
t3 = 0 0 0 0 2 2 2 2 1 1 1 1 3 3 3 3
t4 = 0 0 0 0 2 2 2 2 1 1 1 1 3 3 3 3
 */
 
if ((wet[A] == 'y') && (swet[A] == 'n'))
{
  loadtable("water");              /* Phase table                   */
  status(A);
    if (mfsat[A] == 'y')
      {obsunblank(); mfpresat_on(); delay(d1); mfpresat_off(); obspower(tpwr); obsblank();}
    else
     delay(d1);
  status(B);
    if (autosoft[A] == 'y') 
     { 
         /* selective H2O one-lobe sinc pulse */
      wetpwr = tpwr - 20.0*log10(wetpw/(pw*compH*1.69));  /* sinc needs 1.69 times longer */
      wetpwr = (int) (wetpwr+0.5);                   /* power than a square pulse */
      Wet4(wetpwr,"H2Osinc",wetpw,t1,t2); delay(dz); 
     } 
    else
     Wet4(wetpwr,wetshape,wetpw,t1,t2); delay(dz); 
  status(C); 
    if (composit[A] == 'y') composite_pulse(pw,t3,rof1,rof2,v1);
      else rgpulse(pw,t3,rof1,rof2);
    setreceiver(t4);
 if (fsqd[A] =='y') obsoffset(tof+oslsfrq);
}
/* **************************************************************** */

if (swet[A] == 'y')
{
  /* loads wet specific values */
  double gzlvlw,gtw,gstab; 
  gzlvlw=getval("gzlvlw"); gtw=getval("gtw"); gstab=getval("gstab");

  /* table "water" is in /vnmr/tablib and is described in the wet section above */
  loadtable("water");              /* Phase table                   */

  /* set swetpwr so that 1 degree swetpw is ~1.4us */
  obspower(swetpwr);
  status(A);
  	delay(d1);
  status(B);
  	
	initval(90.0,v4);
  	starthardloop(v4);
  		rgpulse(swetpw,t1,rof1,0.0);
        	zgradpulse(gzlvl_swet,gt_swet);
		delay(50.0e-6);
		zgradpulse(-1*gzlvl_swet,gt_swet);
		delay(50.0e-6);
	endhardloop();
	zgradpulse(gzlvlw,gtw);
	delay(gstab);
	  	
	initval(112.0,v5);
	starthardloop(v5);
  		rgpulse(swetpw,t2,rof1,0.0);
        	zgradpulse(gzlvl_swet,gt_swet);
		delay(50.0e-6);
		zgradpulse(-1*gzlvl_swet,gt_swet);
		delay(50.0e-6);
	endhardloop();
	zgradpulse(gzlvlw/2.0,gtw);
	delay(gstab);
	
	initval(78.0,v6);
	starthardloop(v6);
  		rgpulse(swetpw,t2,rof1,0.0);
        	zgradpulse(gzlvl_swet,gt_swet);
		delay(50.0e-6);
		zgradpulse(-1*gzlvl_swet,gt_swet);
		delay(50.0e-6);
	endhardloop();
	
	zgradpulse(gzlvlw/4,gtw);
	delay(gstab);
  	
	initval(178.0,v7);
	starthardloop(v7);
  		rgpulse(swetpw,t2,rof1,0.0);
        	zgradpulse(gzlvl_swet,gt_swet);
		delay(50.0e-6);
		zgradpulse(-1*gzlvl_swet,gt_swet);
		delay(50.0e-6);
	endhardloop();
	
	zgradpulse(gzlvlw/8,gtw);	
        obspower(tpwr);
   	delay(gstab);

  status(C); 
    if (composit[A] == 'y') composite_pulse(pw,t3,rof1,rof2,v1);
      else rgpulse(pw,t3,rof1,rof2);
    setreceiver(t4);
}
/* **************************************************************** */

if (shaped[A] == 'y')
  {     
     dbl(ct,v1);      /*v1=02020202	*/
     hlv(ct,v2);      /* v2=00112233	*/
     mod2(v2,v3);     /* v3=00110011	*/
     add(v1,v3,v4);   /* v4=02130213	*/
     add(v2,one,v5);  /* v5=11223300	*/
     mod4(v4,oph);    /* oph=02130213	*/

    /* equilibrium period */
    status(A);
     zgradpulse(gzlvl0,gt0);
     if (sspul[A] == 'y')
       { 
        zgradpulse(2.1*gzlvl0,gt0);
        rgpulse(pw90,zero,rof1,rof1);
        zgradpulse(1.5*gzlvl0,gt0);
       }
     if (satmode[A] == 'y') 
     {
       if (mfsat[A] == 'y')
         {obsunblank(); mfpresat_on(); delay(satdly); mfpresat_off(); obspower(tpwr); obsblank();}
       else
       {
        if (satfrq != tof)  obsoffset(satfrq);
        obspower(satpwr); 
        shaped_pulse(satshape,d1,v5,rof1,rof1);
       } 
     }
     else
      delay(d1);
    status(B);
     if (p1>0.0)
      {
       if (satfrq != p1frq)  obsoffset(p1frq);
       obspower(p1pwr); 
       shaped_pulse(p1shape,p1,zero,20.0e-6,rof1);
       if (satmode[B] == 'y')
        {
         if (mfsat[B] == 'y')
         {obsunblank(); mfpresat_on(); delay(satdly); mfpresat_off(); obspower(tpwr); obsblank();}
         else
         {
          if (satfrq != p1frq) obsoffset(satfrq); 
          obspower(satpwr); shaped_pulse(satshape,d2,v5,rof1,rof1);
          if (satfrq !=tof) obsoffset(tof);
          obspower(pwpwr);
         }
        }
       else
        {
         if (p1frq != tof) obsoffset(tof); 
         obspower(pwpwr); delay(d2);
        }
       }
      
      else
       {
        if (satfrq != tof) obsoffset(tof); obspower(pwpwr);
       }
     status(C);
       shaped_pulse(pwshape,pw,oph,20.0e-6,rof2);
 if (fsqd[A] == 'y') obsoffset(tof+oslsfrq);
  }

/* ************************************************************* */

if (watergate[A] == 'y')
 {

 if (autosoft[A] == 'y') 
  { 
      /* selective H2O one-lobe sinc pulse */
   flippwr = tpwr - 20.0*log10(flippw/(pw*compH*1.69));  /* needs 1.69 times more */
   flippwr = (int) (flippwr);                        /* power than a square pulse */
  } 

   if (tpwrsf_u < 4095.0) flippwr=flippwr+6;  /* use fine power ~2048 for control */

   if((flag3919[A] == 'y' && flagW5[A] == 'y' ))
  { text_error("incorrect flags! only one watergate pulse flag can be 'y'   ");
    psg_abort(1); }

   if((flag3919[A] == 'y' && flagsoft[A] == 'y' ))
  { text_error("incorrect flags! only one watergate pulse flag can be 'y'   ");
    psg_abort(1); }

   if((flagsoft[A] == 'y' && flagW5[A] == 'y' ))
  { text_error("incorrect flags! only one watergate pulse flag can be 'y'   ");
    psg_abort(1); }


   obsoffset(tofwg);
   obsstepsize(1.0);
   if (phincr1 < 0.0) phincr1=360+phincr1;
   if (phincr2 < 0.0) phincr2=360+phincr2;
   initval(phincr1,v3);
   initval(phincr2,v2);
   add(oph,two,v1);
   
status(A);
   zgradpulse(gzlvl0,gt0);
   if (mfsat[A] == 'y')
    { obsunblank(); mfpresat_on(); delay(d1); mfpresat_off(); obspower(tpwr); obsblank();
     obsoffset(tofwg);
    }
   else
    {
     obspower(tpwr);
     obsoffset(tofwg);
     delay(d1);
    }
status(B);
   rcvroff();
   if (flipback[A] == 'y')
   {
       obspower(flippwr);       /* already higher by 6dB if tpwrsf_u<4095.0 */
       obspwrf(tpwrsf_d);       /* fine power adjustment */
       if (autosoft[A] == 'y') /* phase correction already in shape  */
        shaped_pulse("H2Osinc_d",flippw,v1,rof1,rof1);
       else
        {
         xmtrphase(v3);         /* for "flipdown"  */
         shaped_pulse(flipshap,flippw,v1,rof1,rof1);
         xmtrphase(zero);
        }
       obspower(tpwr);
       obspwrf(4095.0);
   }
   rgpulse(pw, oph,rof1,rof1);
   zgradpulse(gzlvl1,gt1);
   delay(gstab-pw/2-rof1);
   if (flag3919[A] == 'y')
     { obsoffset(tofwg);
       rgpulse(pw*0.231,oph,rof1,rof1);
       delay(d3);
       rgpulse(pw*0.692,oph,rof1,rof1);
       delay(d3);
       rgpulse(pw*1.462,oph,rof1,rof1);
       delay(d3);
       rgpulse(pw*1.462,v1,rof1,rof1);
       delay(d3);
       rgpulse(pw*0.692,v1,rof1,rof1);
       delay(d3);
       rgpulse(pw*0.231,v1,rof1,rof1);
       obsoffset(tof);
    }
  if (flagW5[A] == 'y')
    {   obsoffset(tofwg);
        rgpulse(pw*0.087,oph,rof1,rof1);
        delay(d3);
        rgpulse(pw*0.206,oph,rof1,rof1);
        delay(d3);
        rgpulse(pw*0.413,oph,rof1,rof1);
        delay(d3);
        rgpulse(pw*0.778,oph,rof1,rof1);
        delay(d3);
        rgpulse(pw*1.491,oph,rof1,rof1);
        delay(d3);
        rgpulse(pw*1.491,v1,rof1,rof1);
        delay(d3);
        rgpulse(pw*0.778,v1,rof1,rof1);
        delay(d3);
        rgpulse(pw*0.413,v1,rof1,rof1);
        delay(d3);
        rgpulse(pw*0.206,v1,rof1,rof1);
        delay(d3);
        rgpulse(pw*0.087,v4,rof1,rof1);
        obsoffset(tof);
    }
  if (flagsoft[A] == 'y')
    {
       obspower(flippwr);
       if (autosoft[A] == 'y')
         if (flipback[A] == 'y')
          {
            obspwrf(tpwrsf_d);
            shaped_pulse("H2Osinc_d",flippw,v1,rof1,rof1);
          }
         else
          {
            obspwrf(tpwrsf_u);
            shaped_pulse("H2Osinc_u",flippw,v1,rof1,rof1);
          }
       else
        {
         if (flipback[A] == 'y')
          {
           obspwrf(tpwrsf_d);
           xmtrphase(v3);
          }
          else
          {
           obspwrf(tpwrsf_u);
           xmtrphase(v2);
          }
         shaped_pulse(flipshap,flippw,v1,rof1,rof1);
         xmtrphase(zero);
        }
       obspower(tpwr);
       obspwrf(4095.0);
       rgpulse(p180, oph,rof1,rof1);
       obspower(flippwr);
       obspwrf(tpwrsf_u);
       if (autosoft[A] == 'y')
         shaped_pulse("H2Osinc_u",flippw,v1,rof1,rof1);
       else
        {
         xmtrphase(v2);    
         shaped_pulse(flipshap,flippw,v1,rof1,rof1);
         xmtrphase(zero);
        }

       obspower(tpwr); obspwrf(4095.0);
    }
   zgradpulse(gzlvl1,gt1);
   delay(gstab+rof2);
   rcvron();
status(C);
 if (fsqd[A] == 'y') obsoffset(tofwg+oslsfrq);
 }
} /* END OF SEQUENCE  */
