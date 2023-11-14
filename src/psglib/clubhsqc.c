// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
/* 
 */
/* clubhsqc.c 

   13C HSQC for small molecules. The data obtained is FDM-ready.
   
   This program will let you obtain a phase-sensitive
   hsqc spectrum.  It uses a "CLUB" sandwich gradient on
   the heteronucleus, and a conventional PFG on protons
   to do the coherence transfer pathway selection.  It also
   has some tricky little delays that compensate for the
   evolution during the PFGs and the proton 180, as well
   as taking into account the finite X nucleus 90 time, and 
   the AP Bus-WFG delay, so that no phase correction is 
   required in F1. 
   References:
   V.A. Mandelshtam, H. Hu, A.J. Shaka, Magn. Reson. Chem. 1998,36,S17.
   H. Hu, A.J. Shaka, JMR 1999,136,54-62.   
   Gradient N,P- selection is done with the CLUB (by arraying 
   the parameter phase).
   Combines N-type and P-type to get phase sensitive spectra;
   Process data with Vnmr using wft2d(1,0,1,0,0,1,0,-1);
   Cycle: nt=8 or nt=32 (including CYCLOPS). 
   
   Choose ifad=1 to do F1 Axial Displacement(FAD) trick, moves 
   axial peak to the edge and it will take the same time (see 
   D. Marion et al, JMR 1989; also A. Hammarstrom and G. Otting,
   JMR A, 1994).

   Includes normal BIRDy presat for carbon-12 protons. Probably not
   strictly necessary, but doesn't hurt.

   Written by Anna De Angelis on 01/09/01; Last mod. 01/09/01.
   Tested at 500 MHz on Varian Unity plus.
   
   **************************************************************
   How to setup the experiment:
   
   tof=0 Hz, sw=6000-8000 Hz (center of proton spectrum at 5 ppm,
                              covers 12-16 ppm);
   IF water is present, set tof on HOD signal, i.e. tof=-176 Hz;
        
   dof=-3000-0 Hz, sw1=20000-22000 Hz (center of carbon spectrum at 
                                80-100 ppm, covers approx. 175 ppm);    
        
   WARNING: POWER SETTINGS in dB DEPEND ON PROBE!!
            POWER SETTINGS in DAC units FOR GRADIENTS DEPEND ON PROBE!!
            ALWAYS REFER TO THE VALUES IN Hz and G/cm:
            USE CORRECT VALUES FOR YOUR PROBE OR DAMAGE MAY OCCUR!!
              
   tpwr= 52 dB (25 kHz);                   RF field on proton; 
   cpwr= 59 dB (>=16 kHz);                 RF field on carbon;
   dpwr=47 dB                              decoupler RF on carbon;
   pw=10.05 us;                            hard 90 time on proton;
   cpw=15.2 us;                            hard 90 time on carbon-13;
   p1pat='hard', p1=2*pw;                  hard 180 pulse for proton;
   csweep='club180', csw180=125 us, use at 16 kHz or sligthly higher cpwr;
   hsweep='new540feb97', hsw180=60 us use at approximately 25 kHz;
   
   pfgon='nny' (only z gradients are used in this code)
   gzlvl0=330 DAC (use 0.6 G/cm or lower)    
   (gzlvl0 gradient should be WEAK and long)
   gtime0=0.040 s
   gstab0=0.0001 s

   For the CLUB sandwich:
   gzlvl1=gzlvl2=gzlvl=2500-4000 DAC (approx. 5-8 G/cm along z)
   gdel=10e-6 s
   gtime1=0.00124 s
   gtime2=0.00075 s
   gtime=0.001 s
   (NOTE. CLUB gradients must satisfy the condition:
   2.0*(gtime1+gtime2)/gtime = tfrq/dfrq)
   
   
   Decoupling:
   dn='C13'
   dm='nny'
   dmm='ccp'
   dseq='hs7' (hyperbolic secant, Ref. M.R.Bendall at al, JMR 1998)
   dmf=4000
   dpwr= 47 dB; (2 kHz r.m.s. field on carbon-13)
   at<=1 s; 
   WARNING: the peak power for hs7 is much higher than for CW 
            decoupling with the same r.m.s. field: DO NOT USE 47 dB
            with CW decoupling!!!
   
   jch=140-150 Hz (use 125 Hz if only sp3 carbons, 160 Hz for aromatics)
   nt=8 or 32 
   phase = 1,2
   ss=8 
   gain=60 dB (set gain as high as possible WITHOUT having ADC overflow);
   d1>=3 s
   delt is the time between the BIRDy and the observe 90:
   delt should be CALIBRATED especially if water is present;
   start with delt=1.3 s and setup the other parameters first.
   Acquire one increment, with ni=1 and phase=1, phase it and type
   calfa if necessary. 
   With ni=1, phase=1,ss=-8, array delt; to observe the suppression
   of 12C-H, do not decouple during acquisition (set dm='nnn');
   transform with wft and choose the value of delt that gives the 
   best suppression,for example the smallest water peak.
   Remember to set back ss to a positive number(ss=8) and dm='nny'
   before doing the 2D.
   
   Choose ifad=1 to do useful FAD trick; if ifad=0 FAD won't be 
   executed;
   
*/



#include <standard.h>

void pulsesequence()
     
{
  double        cpw,cpwr,jch,gtime1,gtime2,gtime,gdel,gzlvl1,
                gzlvl2,gzlvl,gzlvl0,gtime0,gstab0,hsw180,
                csw180,delt;
  
  int           iphase,icosel,ifad;
  
  char          p1pat[MAXSTR],hsweep[MAXSTR],csweep[MAXSTR];
  
  cpw = getval("cpw");
  cpwr = getval("cpwr");
  jch = getval("jch"); 
  gtime0 = getval("gtime0");
  gtime1 = getval("gtime1");
  gtime2 = getval("gtime2");
  gtime = getval("gtime");
  gzlvl0 = getval("gzlvl0");
  gzlvl1 = getval("gzlvl1");
  gzlvl2 = getval("gzlvl2");
  gzlvl = getval("gzlvl");
  gdel = getval("gdel");
  gstab0 = getval("gstab0");
  hsw180 = getval("hsw180");
  csw180 = getval("csw180");
  delt = getval("delt"); 
  
  getstr("p1pat",p1pat);
  getstr("hsweep",hsweep);
  getstr("csweep",csweep);
  
  iphase=(int)(getval("phase")+0.5);
  
  if(iphase==2)
    {
      icosel=-1;
    }
  
  else
    {
      icosel=1;
    }
  
  ifad=(int)(getval("ifad")+0.5);      /* if ifad != 0 do FAD */
  
  /* STEADY-STATE PHASECYCLING */
  /* This section determines if the phase calculations trigger off of (SS - SSCTR)
     or off of CT */
  
  ifzero(ssctr);
  assign(ct,v14);
  elsenz(ssctr);
  sub(ssval,ssctr,v14); /* v14 = 0,1,2,...,ss-1 */
  endif(ssctr);
  
  /* PHASE CYCLING */
  
  mod2(v14,v1);
  dbl(v1,v1);   /* v1=0202 */
  hlv(v14,v14);
  mod2(v14,v2);
  dbl(v2,v2);           /* v2=0022 */
  add(v1,v2,oph);
  hlv(v14,v14);
  mod2(v14,v3);
  dbl(v3,v3);
  add(one,v3,v3);       /* v3=4x1,4x3 */
  hlv(v14,v14);
  add(v14,v1,v1);
  add(v14,v2,v2);
  add(v14,v3,v3);
  add(v14,oph,oph);
  assign(v14,v12);
  add(one,v14,v13);
  
  if(ifad != 0)
    {
      initval(2.0*(double)(((int)(d2*sw1+0.5)%2)),v10);
      add(v10,oph,oph);
      add(v10,v12,v12); 
      add(v10,v1,v1);
    }
 
  /* check parameters */ 

   if((delt-gtime0-gstab0)<0.0)
     { 
       text_error("ABORT: delt-gtime0-gstab0 is negative");
       psg_abort(1);
     }                /* gtime has to be shorter than 1/(4j) */
 

   if((1.0/(4.0*jch)-gtime)<=0.0)
     { 
       text_error("ABORT: gtime too long!");
       psg_abort(1);
     }                /* gtime has to be shorter than 1/(4j) */
 
   
   if( (dpwr > 50) || (at > 1.2) )
     { 
       text_error("don't fry the probe!!!");
       psg_abort(1);
     }
   

   /* equilibrium period */
   
   status(A);
   
   delay(10.0e-6);
   decpower(cpwr);
   decoffset(dof);         /*Middle of the X spectrum*/
   delay(d1-delt-(10.0e-6));
   rgpulse(pw,zero,rof1,rof2);
   delay(1.0/(2.0*jch));
   simshaped_pulse(p1pat,csweep,p1,csw180,one,one,rof1,rof2);
   delay(1.0/(2.0*jch));
   rgpulse(pw,zero,rof1,rof2); 
   rgradient('z',gzlvl0);
   delay(gtime0);            
   rgradient('z',0.0); 
   delay(gstab0);
   delay(delt-gtime0-gstab0);
   
   /*   Normal BIRDy presat for carbon-12 protons.  Probably not
        strictly necessary, but doesn't hurt.  */
   
   
   status(B);
   
   /*  Start of pulse sequence. INEPT in: */
   
   rgpulse(pw,v14,rof1,rof2);
   delay(1.0/(4.0*jch));
   simshaped_pulse(p1pat,csweep,p1,csw180,v14,v12,rof1,rof2);
   delay(1.0/(4.0*jch));
   rgpulse(pw,v13,rof1,rof2);
   
   /* a short x or y gradient can be added right here to
      dephase any tranvserse magnetization */
   
   decrgpulse(cpw,v1,rof1,1.0e-6);
   
   delay(d2/2.0);
   shaped_pulse(hsweep,hsw180,v3,1.0e-6,1.0e-6);
   delay(d2/2.0);
   
   /*   CLUB sandwich: */
   
   zgradpulse(gzlvl1*(double)icosel,gtime1);
   delay(gdel);   
   decshaped_pulse(csweep,csw180,v14,rof1,rof2);
   zgradpulse(-gzlvl1*(double)icosel,gtime1);
   delay(gdel);    
   delay(hsw180+(4.0*cpw/3.14159265358979323846)+(4.0e-6)+WFG_START_DELAY+WFG_STOP_DELAY);
   zgradpulse(-gzlvl2*(double)icosel,gtime2);
   delay(gdel);    
   decshaped_pulse(csweep,csw180,v14,rof1,rof2);
   zgradpulse(gzlvl2*(double)icosel,gtime2);
   delay(gdel);    
   
   /* INEPT out */
   decrgpulse(cpw,v2,1.0e-6,rof2);
   rgpulse(pw,v14,rof1,rof2);
   delay(1.0/(4.0*jch));
   simshaped_pulse(p1pat,csweep,p1,csw180,v14,v14,rof1,rof2);
   zgradpulse(gzlvl,gtime);
   delay(1.0/(4.0*jch)-gtime); 
   
   decpower(dpwr);  /* CAUTION set dpwr to 47dB and acquire for < 1 sec */
   
   status(C);
   
}
