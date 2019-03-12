/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* wgnoesy  -  NOESY experiment with water suppression by gradient echo.  
               No attempt is made to suppress J-cross peaks in this pulse 
               sequence. F1 axial peaks are shifted by States TPPI method. 
 
               Phase of flipback pulse is currently only optimal for the case of radiation 
               damping during mix, so set flipback='n' for enhancing 
               water exchange crosspeaks.  
 
               Buffer line(s) suppression is available via a WET pulse during the mix period. 
               For this, set wet='y' and create shape using Pbox to do selective 90 degree  
               pulse on buffer line(s). The 1H xmtr will remain at the H2O position so create 
               the shape with an ofs value of the distance to the H2O from the buffer line(s). 
               The wet pulse width must be less than ~1/4 of the mix time. The watergate 
               portion of the pulse sequence does the water suppression. 
                
               cor permits correction of delay for proper lp=0. Try small values (0-10usec)
               on first increment spectra and choose cor value for getting lp=0. 
 
               C13refoc flag for C13 decoupling in t1. 
               15N refocussing done if N15refoc='y' 
               Both 13C and 15N refocussing done if CNrefoc='y'  
               Dropped power 3db down for both N15 and 13C if simulaneous 180's. 
 
               Uses composite C13 180s in t1 
 
               Limited bandwidth of C13 180s in t1 favors aliphatic-only or aromatic-only 
               noesy, 

               ech dast feb.93  gg palo alto jan 95 
               added flipback 16 april 95 
               5jan96 GG 
               added to BioPack june 1998 
               3jan01 GG 
               modified phase of flipback pulse (EK) 
               added wet 15apr03 GG 
               added dpfgse option, and fixed phase cycle. Moved flipback pulse during mix
                 to just prior to read pulse (let radiation damping return water to +Z) 8dec03 PS (dast)
               added offset control via "tofwg" to watergate portion of sequence. 2nov06 GG
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


static int 
 
    ph2[32] = {2,0,2,0,2,0,2,0,3,1,3,1,3,1,3,1,2,0,2,0,2,0,2,0,3,1,3,1,3,1,3,1}, 
    ph1[32] = {0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,3,3,3,3,3,3,3,3}, 
    ph3[32] = {0,0,2,2,1,1,3,3,1,1,3,3,2,2,0,0,0,0,2,2,1,1,3,3,1,1,3,3,2,2,0,0}, 
    ph4[32] = {2,0,0,2,3,1,1,3,3,1,1,3,0,2,2,0,0,2,2,0,1,3,3,1,1,3,3,1,2,0,0,2}, 
    ph5[32] = {0,2,0,2,1,3,1,3,1,3,1,3,2,0,2,0,2,0,2,0,3,1,3,1,3,1,3,1,0,2,0,2},
    phr[32] = {0,2,2,0,1,3,3,1,1,3,3,1,2,0,0,2,2,0,0,2,3,1,1,3,3,1,1,3,0,2,2,0}; 
void pulsesequence() 
{ 
   double          arraydim, 
                   ss, 
                   compH=getval("compH"),
                   cor = getval("cor"), 
                   p180 = getval("p180"), 
                   gzlvl3 = getval("gzlvl3"), 
                   gt3 = getval("gt3"), 
                   gzlvl0 = getval("gzlvl0"), 
                   gt0 = getval("gt0"), 
                   gstab = getval("gstab"), 
                   flippwr = getval("flippwr"), 
                   flippw  = getval("flippw"), 
                   pwClvl  = getval("pwClvl"), 
                   pwNlvl  = getval("pwNlvl"), 
                   pwC  = getval("pwC"), 
                   pwN  = getval("pwN"), 
                   dz=getval("dz"), 
                   wetpw=getval("wetpw"),        /* User enters power for 90 deg. */ 
                   wetpwr=getval("wetpwr"),        
                   gtw=getval("gtw"),            /* Z-Gradient duration           */ 
                   gswet=getval("gswet"),        /* Post-gradient stability delay */ 
                   mix = getval("mix"),
                   tofwg=getval("tofwg"),
                   phincr2 = getval("phincr2"),
                   tpwrsf_d = getval("tpwrsf_d"),
                   tpwrsf_u = getval("tpwrsf_u"),
		   wrefpwr = getval("wrefpwr"),
                   wrefpw = getval("wrefpw");
 
   int             t1_counter, 
                   iphase; 
   char  C13refoc[MAXSTR],N15refoc[MAXSTR],CNrefoc[MAXSTR],wetshape[MAXSTR],wet[MAXSTR],sspul[MAXSTR],flag3919[MAXSTR],flipshap[MAXSTR],flipback[MAXSTR],dpfgse[MAXSTR],autosoft[MAXSTR],wrefshape[MAXSTR]; 
 
 
/* LOAD VARIABLES */ 
 
   getstr("C13refoc",C13refoc); 
   getstr("N15refoc",N15refoc); 
   getstr("CNrefoc",CNrefoc); 
   getstr("sspul",sspul); 
   getstr("flag3919",flag3919); 
   getstr("flipshap",flipshap); 
   getstr("flipback",flipback); 
   getstr("wetshape", wetshape); 
   getstr("wet", wet); 
   getstr("autosoft", autosoft);
   getstr("dpfgse", dpfgse);
   getstr("wrefshape", wrefshape);
   arraydim = getval("arraydim"); 
   iphase = (int) (getval("phase") + 0.5); 
   ss = getval("ss"); 
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
   if ((dm[A]=='y') || (dm2[A] == 'y') || (dm[B] == 'y') || (dm2[B] == 'y')) 
     { 
      fprintf(stdout,"Set dm and dm2 to be nnn or nny\n"); 
      psg_abort(1); 
     } 
   if ((rof1 < 9.9e-6) && (ix == 1)) 
      fprintf(stdout,"Warning:  ROF1 is less than 10 us\n"); 
 
/* CALCULATE PHASECYCLE */ 
         
   settable(t1,32,ph1); 
   settable(t2,32,ph2); 
   settable(t3,32,ph3); 
   settable(t4,32,ph4);
   settable(t5,32,ph5);
   settable(t6,32,phr); 
 
   sub(ct,ssctr,v12); 
   getelem(t1,v12,v1); 
   getelem(t2,v12,v2); 
   getelem(t3,v12,v3); 
   getelem(t4,v12,v9);
   getelem(t5,v12,v10);
   getelem(t6,v12,oph); 
 
   assign(zero,v5); 
   add(v3,two,v4); 
 
 
   if (iphase == 2) 
      { incr(v2); incr(v5); } 
   if (iphase == 3) 
      add(v2, v14, v2);         /* TPPI phase increment */ 
 
 
/*HYPERCOMPLEX MODE USES REDFIELD TRICK TO MOVE AXIAL PEAKS TO EDGE */ 
    initval(2.0*(double)(((int)(d2*getval("sw1")+0.5)%2)),v6); 
  if ((iphase==1)||(iphase==2)) 
       {add(v2,v6,v2); add(oph,v6,oph); add(v5,v6,v5);}   
   add(two,oph,v13); 
 
 
/* BEGIN THE ACTUAL PULSE SEQUENCE */ 
 status(A);
      delay(100e-6); 
      obspower(tpwr); 
      decpower(pwClvl); dec2power(pwNlvl); 
      if (CNrefoc[A] == 'y') 
      { 
       decpower(pwClvl-3.0); pwC=1.4*pwC; 
       dec2power(pwNlvl-3.0); pwN=1.4*pwN; 
      } 
      if (sspul[A] == 'y') 
      { 
       zgradpulse(gzlvl0,gt0); 
       rgpulse(pw,zero,1.0e-3,rof1); 
       zgradpulse(gzlvl0,gt0); 
      } 
      if (satmode[A] == 'y') 
       { 
        obsoffset(satfrq); 
        obspower(satpwr); 
        rgpulse(d1,zero,rof1,0.0); 
        obsoffset(tof); 
        obspower(tpwr); 
       } 
      else 
       delay(d1); 
      obsstepsize(45.0); 
      initval(7.0,v7); 
      xmtrphase(v7); 
      delay(.0001); 
      if (autosoft[A] == 'y')
       {
         /* H2Osinc_d.RF or H2Osinc_u.RF flipdown pulse */
         flippwr = tpwr - 20.0*log10(flippw/(pw*compH*1.69));
         flippwr = (int) (flippwr +0.5);
       }
     status(B);
      if (flipback[A] == 'y') 
      { 
       add(v2,two,v2); 
       obspower(flippwr); 
       if (tpwrsf_d<4095.0) flippwr=flippwr+6.0;
       obspower(flippwr); obspwrf(tpwrsf_d);
       if (autosoft[A] == 'y')
        shaped_pulse("H2Osinc_d",flippw,v2,20e-6,rof1); 
       else
        shaped_pulse(flipshap,flippw,v2,20e-6,rof1); 
       obspower(tpwr); obspwrf(4095.0); 
      } 
      add(v2,two,v2); 
      rgpulse(pw, v2, rof1, 0.0); 
      xmtrphase(zero); 
 
      if (d2>0.0)
       { 
        if ((C13refoc[A] == 'n') &&  (N15refoc[A] == 'n') && (CNrefoc[A] == 'n')) 
         if (d2/2.0 > 0.64*pw)   
           delay(d2-4.0*pw/PI-SAPS_DELAY-rof1); 
 
 
        if ((C13refoc[A] == 'n') && (N15refoc[A] == 'n') && (CNrefoc[A] == 'y')) 
         {
         if (pwN > 2.0*pwC) 
          { 
           if (d2/2.0 > (pwN +0.64*pw+rof1))   
            { 
             delay(d2/2.0-pwN-0.64*pw-SAPS_DELAY); 
             dec2rgpulse(pwN-2.0*pwC,zero,0.0,0.0); 
             sim3pulse(0.0,pwC,pwC, zero,zero,zero, 0.0, 0.0); 
             sim3pulse(0.0,2.0*pwC,2.0*pwC, zero,one,zero, 0.0, 0.0); 
             sim3pulse(0.0,pwC,pwC, zero,zero,zero, 0.0, 0.0); 
             dec2rgpulse(pwN-2.0*pwC,zero,0.0,0.0); 
             delay(d2/2.0-pwN-0.64*pw-SAPS_DELAY-rof1); 
            } 
           else
             delay(d2-4.0*pw/PI-SAPS_DELAY-rof1);
          }
         else 
          {
           if (d2/2.0 > (pwN +pwC+ 0.64*pw+rof1))   
            { 
             delay(d2/2.0-pwN-pwC-0.64*pw-SAPS_DELAY); 
             decrgpulse(pwC,zero,0.0,0.0); 
             sim3pulse(0.0,2.0*pwC,2.0*pwN, zero,one,zero, 0.0, 0.0); 
             decrgpulse(pwC,zero,0.0,0.0); 
             delay(d2/2.0-pwN-pwC-0.64*pw-rof1); 
            } 
           else
             delay(d2-4.0*pw/PI-SAPS_DELAY-rof1);
          } 
         } 
 
        if ((C13refoc[A] == 'n') && (N15refoc[A] == 'y') && (CNrefoc[A] == 'n')) 
         {
          if (d2/2.0 > (pwN + 0.64*pw+rof1))   
           { 
            delay(d2/2.0-pwN-0.64*pw-SAPS_DELAY); 
            dec2rgpulse(2.0*pwN, zero, 0.0, 0.0); 
            delay(d2/2.0-pwN-0.64*pw-rof1); 
           } 
          else
            delay(d2-4.0*pw/PI-SAPS_DELAY-rof1);
         } 
 
 
        if ((C13refoc[A] == 'y') &&  (N15refoc[A] == 'n') && (CNrefoc[A] == 'n')) 
         {
         if (d2/2.0 > (2.0*pwC + 0.64*pw+rof1))   
          { 
           delay(d2/2.0-2.0*pwC-0.64*pw-SAPS_DELAY); 
           decrgpulse(pwC,zero,0.0,0.0); 
           decrgpulse(2.0*pwC, one, 0.0, 0.0); 
           decrgpulse(pwC,zero,0.0,0.0); 
           delay(d2/2.0-2.0*pwC-0.64*pw-rof1); 
          } 
          else
            delay(d2-4.0*pw/PI-SAPS_DELAY-rof1);
         }
      }
      rgpulse(pw, v1, rof1, 0.0); 
       if (wet[A] == 'n') 
        { 
         delay(mix/2.0-flippw-gt0/2.0); 
         zgradpulse(gzlvl0,gt0); 
         delay(mix/2.0-gt0/2.0); 
        } 
       else 
        { 
         delay(mix -dz -flippw -(4.0*wetpw) - (4.0*20.0e-6) 
         - (4.0*rof1) - (4.0*gtw) - (4.0*gswet)); 
         Wet4(wetpwr,wetshape,wetpw,zero,one); 
         delay(dz); 
        } 
      if ((flipback[A] == 'y') && (wet[A] == 'n')) 
       {
        add(v3,two,v3); 
        obspower(flippwr); obspwrf(tpwrsf_d);
        if (autosoft[A] == 'y')
         shaped_pulse("H2Osinc_d",flippw,v3,20e-6,rof1); 
        else
         shaped_pulse(flipshap,flippw,v3,20e-6,rof1); 
        add(v3,two,v3);
       } 

   obspower(tpwr); obspwrf(4095.0);
   decpower(dpwr); dec2power(dpwr2); 
 
 
   rgpulse(pw, v3,rof1,0.0); 
   if (dpfgse[A] == 'n')
   {
     if (flag3919[A] == 'y') 
     { 
         obsoffset(tofwg);
         delay(tau-pw/2 -OFFSET_DELAY); 
         zgradpulse(gzlvl3,gt3); 
         delay(gstab); 
         rgpulse(pw*0.231,v3,rof1,rof1); 
         delay(d3); 
         rgpulse(pw*0.692,v3,rof1,rof1); 
         delay(d3); 
         rgpulse(pw*1.462,v3,rof1,rof1); 
         delay(d3); 
         rgpulse(pw*1.462,v4,rof1,rof1); 
         delay(d3); 
         rgpulse(pw*0.692,v4,rof1,rof1); 
         delay(d3); 
         rgpulse(pw*0.231,v4,rof1,rof1); 
         delay(tau);
         obsoffset(tof);
     } 
    else 
     { 
        if (autosoft[A] == 'y')
        {
         obspower(flippwr); obspwrf(tpwrsf_d);
         obsoffset(tofwg);
         delay(tau -pw/2-0.0-2.0*SAPS_DELAY-4.0*POWER_DELAY); 
         zgradpulse(gzlvl3,gt3); 
         delay(gstab); 
         shaped_pulse("H2Osinc_d",flippw,v4,rof1,rof1); 
         obspower(tpwr); obspwrf(4095.0); 
         rgpulse(p180, v3,rof1,rof1); 
         obspower(flippwr); obspwrf(tpwrsf_u);
         shaped_pulse("H2Osinc_u",flippw,v4,rof1,rof1); 
         obspower(tpwr); obspwrf(4095.0);
         delay(tau-4.0*POWER_DELAY);
         obsoffset(tof);
        }
        else
        {
         obsoffset(tofwg);
         delay(tau -pw/2-0.0-2.0*SAPS_DELAY-4.0*POWER_DELAY); 
         zgradpulse(gzlvl3,gt3); 
         delay(gstab); 
         obspower(flippwr); obspwrf(tpwrsf_d); 
         obsstepsize(1.0); 
         xmtrphase(v11); 
         shaped_pulse(flipshap,flippw,v4,rof1,rof1); 
         obspower(tpwr); obspwrf(4095.0); 
         xmtrphase(zero); 
         rgpulse(p180, v3,rof1,rof1); 
         obspower(flippwr); obspwrf(tpwrsf_u); 
         shaped_pulse(flipshap,flippw,v4,rof1,rof1); 
         obspower(tpwr);obspwrf(4095.0); 
         delay(tau-4.0*POWER_DELAY);
         obsoffset(tof);
        }
     }
     zgradpulse(gzlvl3,gt3); 
     delay(gstab); 
   }
   else              /* if(dpfgse[A] == 'y') */
   {


     if (autosoft[A] == 'y')
      {
        /* selective H2O rectangular 180 degree pulse */
        wrefpwr = tpwr - 20.0*log10(wrefpw/(pw*compH)) +6.0;
        wrefpwr = (int) (wrefpwr +0.5);
      }
     zgradpulse(3.0*gzlvl3,gt3);
     obspower(wrefpwr);
     add(v9,two,v9);
     delay(gstab);
     if (autosoft[A] == 'y')
         shaped_pulse("hard",wrefpw,v9,rof1,rof1);
        else
         shaped_pulse(wrefshape,wrefpw,v9,rof1,rof1);
     add(v9,two,v9);
     obspower(tpwr);
     rgpulse(p180,v9,rof1,rof1);
     zgradpulse(3.0*gzlvl3,gt3);
     obspower(wrefpwr);
     add(v10,two,v10);
     delay(gstab);
     zgradpulse(gzlvl3,gt3);
     delay(gstab);
     if (autosoft[A] == 'y')
         shaped_pulse("hard",wrefpw,v10,rof1,rof1);
        else
         shaped_pulse(wrefshape,wrefpw,v10,rof1,rof1);
     add(v10,two,v10);
     obspower(tpwr);
     rgpulse(p180,v10,rof1,rof1);
     zgradpulse(gzlvl3,gt3);
     delay(gstab);
   }
  delay(cor);
  status(C); 
 
} 

