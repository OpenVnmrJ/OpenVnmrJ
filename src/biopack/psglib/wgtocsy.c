/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* wgtocsy - 
   Homonuclear TOCSY with DIPSI spinlock and watergate
   water suppression.


   made C13refoc flag for C13 decoupling in t1.
   15N refocussing done if N15refoc='y'
   Both 13C and 15N refocussing done if CNrefoc='y' 
   Dropped power 3db down for both N15 and 13C if simulaneous 180's.

   Uses composite C13 180s in t1

   Limited bandwidth of C13 180s in t1 favors
   aliphatic-only or aromatic-only tocsys

   if gt2>0 then homospoil gradient and gstab delay put after
   flipback pulse.

   Modified to set spinlock power using fine attenuator. G.Gray Feb2005
*/


#include <standard.h>

void dipsi(phse1,phse2)
codeint phse1,phse2;
{
        double slpw5;
        slpw5 = 1.0/(4.0*getval("strength")*18.0);

        rgpulse(64*slpw5,phse1,0.0,0.0);
        rgpulse(82*slpw5,phse2,0.0,0.0);
        rgpulse(58*slpw5,phse1,0.0,0.0);
        rgpulse(57*slpw5,phse2,0.0,0.0);
        rgpulse(6*slpw5,phse1,0.0,0.0);
        rgpulse(49*slpw5,phse2,0.0,0.0);
        rgpulse(75*slpw5,phse1,0.0,0.0);
        rgpulse(53*slpw5,phse2,0.0,0.0);
        rgpulse(74*slpw5,phse1,0.0,0.0);

}

/*
static int ph1[8]  = {0,0,0,0,1,1,1,1},
           ph2[16]  = {2,2,2,2,1,1,1,1,0,0,0,0,3,3,3,3},                 
           ph3[4]  =  {0,2,1,3},                           
           ph4[16]  =  {0,2,1,3,2,0,3,1,2,0,3,1,0,2,1,3};        
*/

static int ph1[16]  = {0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1},
           ph2[16]  = {2,0,2,0,2,0,2,0,3,1,3,1,3,1,3,1},                 
           ph3[8]  =  {0,0,2,2,1,1,3,3},    
           ph5[8]  =  {0,0,0,0,1,1,1,1},                       
           ph4[8]  =  {0,2,2,0,1,3,3,1};
void pulsesequence()
{
   int             iphase;
   char            flipback[MAXSTR],
                   watergate[MAXSTR],
                   flipshap[MAXSTR],
                   flag3919[MAXSTR],
                   sspul[MAXSTR],
                   C13refoc[MAXSTR],N15refoc[MAXSTR],CNrefoc[MAXSTR];

   double          slpwr,
                   slpw,
                   slpw5,
                   mix,
                   pwN = getval("pwN"),
                   pwNlvl = getval("pwNlvl"),
                   pwC = getval("pwC"),
                   pwClvl = getval("pwClvl"),
		   gstab = getval("gstab"),
 /*                  cor = getval("cor"),         */
		   p180 = getval("p180"),
                   gzlvl2 = getval("gzlvl2"),
                   gt2 = getval("gt2"),
                   gzlvl3 = getval("gzlvl3"),
                   gt3 = getval("gt3"),
                   gzlvl0 = getval("gzlvl0"),
                   gt0 = getval("gt0"),
                   phincr1 = getval("phincr1"),
                   phincr2 = getval("phincr2"),
                   flippw = getval("flippw"),
                   flippwr = getval("flippwr"),

    compH = getval("compH"),    /* adjustment for H1 amp. compression */
    pwHs = getval("pwHs"),	        /* H1 90 degree pulse length at tpwrs */
    tpwrs,	  	              /* power for the pwHs ("H2Osinc") pulse */
    strength = getval("strength"),          /* spinlock field strength in Hz */
                   cycles;


/* LOAD AND INITIALIZE VARIABLES */
   mix = getval("mix");

   getstr("C13refoc",C13refoc);
   getstr("N15refoc",N15refoc);
   getstr("CNrefoc",CNrefoc);
   getstr("watergate",watergate);
   getstr("flipback",flipback);
   getstr("flipshap",flipshap);
   getstr("flag3919",flag3919);
   getstr("sspul",sspul);
   iphase = (int) (getval("phase") + 0.5);
   if (phincr1 < 0.0) phincr1=360+phincr1;
   if (phincr2 < 0.0) phincr2=360+phincr2;
   initval(phincr1,v8);
   initval(phincr2,v11);

 
   settable(t1,16,ph1);
   settable(t2,16,ph2);
   settable(t3,8,ph3);
   settable(t5,8,ph5);
   settable(t4,8,ph4);

   getelem(t1,ct,v1);
   getelem(t4,ct,oph);
   getelem(t3,ct,v3);
   add(v3,two,v5);
   
   assign(one,v2);
   add(two, v2, v4);

   if (iphase == 2)
      incr(v1);

   /* selective H20 one-lobe sinc pulse */
   tpwrs = tpwr - 20.0*log10(pwHs/((compH*pw)*1.69));   /* sinc=1.69xrect */
   tpwrs = (int) (tpwrs);               
   /* power level and pulse time for spinlock */
   slpw = 1/(4.0 * strength) ;           /* spinlock field strength  */
   slpw5 = slpw/18;
   slpwr = 4095*(compH*pw/slpw);
   slpwr = (int) (slpwr + 0.5);
   initval(2.0*(double)(((int)(d2*getval("sw1")+0.5)%2)),v14);
   cycles = (mix/(2072*slpw5));
   cycles = 2.0*(double)(int)(cycles/2.0);
   initval(cycles, v9);                      /* V9 is the MIX loop count */

   add(v1, v14, v1);
   add(oph,v14,oph);
   
/* BEGIN ACTUAL PULSE SEQUENCE CODE */
   status(A);
      obspower(tpwr);
      decpower(pwClvl); dec2power(pwNlvl);
      if (CNrefoc[A] == 'y')
      {
       decpower(pwClvl-3.0); pwC=1.4*pwC;
       dec2power(pwNlvl-3.0); pwN=1.4*pwN;
      }
      if (sspul[0] == 'y')
	{
	 zgradpulse(1000.0,0.001);
	 rgpulse(pw,zero,rof1,rof1);
	 zgradpulse(1000.0,0.0015);
	}
      obsstepsize(45.0);
      initval(7.0,v7);
      xmtrphase(v7);
      delay(d1);
/*      rcvroff();     */
      
   status(B);
      rgpulse(pw, v1, rof1, 2.0e-6);
      xmtrphase(zero);
      if (d2>0.0)
       {
        if ((C13refoc[A] == 'n') &&  (N15refoc[A] == 'n') && (CNrefoc[A] == 'n'))
            delay(d2-1.28*pw-SAPS_DELAY-2.0e-6-rof1);

        if ((C13refoc[A] == 'n') && (N15refoc[A] == 'n') && (CNrefoc[A] == 'y'))
         {
          if (pwN > 2.0*pwC)
           {
            if (d2/2.0 > (pwN +0.64*pw+rof1))
             {
              delay(d2/2.0-pwN-0.64*pw-2.0e-6-SAPS_DELAY);
              dec2rgpulse(pwN-2.0*pwC,zero,0.0,0.0);
              sim3pulse(0.0,pwC,pwC, zero,zero,zero, 0.0, 0.0);
              sim3pulse(0.0,2.0*pwC,2.0*pwC, zero,one,zero, 0.0, 0.0);
              sim3pulse(0.0,pwC,pwC, zero,zero,zero, 0.0, 0.0);
              dec2rgpulse(pwN-2.0*pwC,zero,0.0,0.0);
              delay(d2/2.0-pwN-0.64*pw-rof1);
             }
            else
              delay(d2-1.28*pw-SAPS_DELAY-2.0e-6-rof1);
           }
          else
           {
             if (d2/2.0 > (pwN +pwC+ 0.64*pw+rof1))
             {
              delay(d2/2.0-pwN-pwC-0.64*pw-2.0e-6-SAPS_DELAY);
              decrgpulse(pwC,zero,0.0,0.0);
              sim3pulse(0.0,2.0*pwC,2.0*pwN, zero,one,zero, 0.0, 0.0);
              decrgpulse(pwC,zero,0.0,0.0);
              delay(d2/2.0-pwN-pwC-0.64*pw-rof1);
             }
            else
              delay(d2-1.28*pw-SAPS_DELAY-2.0e-6-rof1);
           }
         }

        if ((C13refoc[A] == 'n') && (N15refoc[A] == 'y') && (CNrefoc[A] == 'n'))
         {
          if (d2/2.0 > (pwN + 0.64*pw+rof1))  
           {
             delay(d2/2.0-pwN-0.64*pw-2.0e-6-SAPS_DELAY);
             dec2rgpulse(2.0*pwN, zero, 0.0, 0.0);
             delay(d2/2.0-pwN-0.64*pw-rof1);
           }
           else
             delay(d2-1.28*pw-SAPS_DELAY-2.0e-6-rof1);
         }

        if ((C13refoc[A] == 'y') &&  (N15refoc[A] == 'n') && (CNrefoc[A] == 'n'))
         {
          if (d2/2.0 > (2.0*pwC + 0.64*pw+rof1))  
           {
             delay(d2/2.0-2.0*pwC-0.64*pw-SAPS_DELAY-2.0e-6);
             decrgpulse(pwC,zero,0.0,0.0);
             decrgpulse(2.0*pwC, one, 0.0, 0.0);
             decrgpulse(pwC,zero,0.0,0.0);
             delay(d2/2.0-2.0*pwC-0.64*pw-rof1);
           }
           else
             delay(d2-1.28*pw-SAPS_DELAY-2.0e-6-rof1);
         }
       }
      rgpulse(pw,t2,rof1,rof1);
      /*magnetization now on "Z"*/
      obspwrf(slpwr);
      zgradpulse(gzlvl0*1.5,gt0);
      delay(gstab);
     if (cycles > 1.0)
      {
         
         starthardloop(v9);
             dipsi(v2,v4); dipsi(v4,v2); dipsi(v4,v2); dipsi(v2,v4);
         endhardloop();
       }
      zgradpulse(-gzlvl0*1.5,gt0);
      obspwrf(4095.0);
      delay(gstab);

      if (flipback[A] == 'y')
       {
        obspower(tpwrs);
        obsstepsize(1.0);
        xmtrphase(v8);
        shaped_pulse("H2Osinc",pwHs,v5,rof1,rof1);
        xmtrphase(zero);
        if (gt2>0.0) 
         {
           zgradpulse(gzlvl2,gt2);
           delay(gstab);                  
         }
       }
  obspower(tpwr);
  rgpulse(pw,v3,rof1,0.0);
  if (watergate[A] == 'y')
   {
      txphase(v3);
      delay(tau - pw/2);
      zgradpulse(gzlvl3,gt3);

   if (flag3919[A] == 'y')
     { 
       delay(gstab);
       rgpulse(pw*0.231,v3,2.0e-6,rof1);
       delay(d3);
       rgpulse(pw*0.692,v3,rof1,rof1);
       delay(d3);
       rgpulse(pw*1.462,v3,rof1,rof1);
       txphase(v4);
       delay(d3);
       rgpulse(pw*1.462,v5,rof1,rof1);
       delay(d3);
       rgpulse(pw*0.692,v5,rof1,rof1);
       delay(d3);
       rgpulse(pw*0.231,v5,rof1,2.0e-6);
    }
  else
    { 
      delay(gstab-2.0*SAPS_DELAY); 
      txphase(v5);
      obspower(flippwr);
      obsstepsize(1.0);
      xmtrphase(v11);
      shaped_pulse(flipshap,flippw,v5,2.0e-6,rof1);
      txphase(v3);
      obspower(tpwr);
      xmtrphase(zero);
      rgpulse(p180, v3,rof1,rof1);
      txphase(v5);
      obspower(flippwr);
      xmtrphase(v11);
      shaped_pulse(flipshap,flippw,v5,rof1,2.0e-6);
      obspower(tpwr);
      xmtrphase(zero);
   }
      delay(tau);
      zgradpulse(gzlvl3,gt3);
      delay(gstab);
  }
      delay(rof2);
      decpower(dpwr);
      dec2power(dpwr2);
     status(C);
}
