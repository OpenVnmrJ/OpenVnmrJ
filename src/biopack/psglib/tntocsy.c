/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* tntocsy - total correlation spectroscopy  (a.k.a. hohaha)
              "clean tocsy" used with windowing and MLEV16+60degree spin lock

    ref: a. bax and d.g. davis, j. magn. reson. 65, 355 (1985)
         m. levitt, r. freeman, and t. frenkiel, j. magn. reson. 47, 328 (1982)

  Parameters:
     window= clean-tocsy window(in us) ~pw
        pw = 90 degree excitation pulse (at power tpwr)
    satdly = length of presaturation;
  strength = spinlock field strength in Hz
    satmode  = flag for presat control
               'yn' for during relaxation delay 
               'yy' for during both "relaxation delay" and "d2" (recommended)
     phase = 1,2: for HYPERCOMPLEX phase-sensitive F1 quadrature
               3: for TPPI phase-sensitive F1 quadrature
     sspul = 'y':  hs-90-hs sequence initiates D1 delay
             'n':  normal D1 delay
      trim = spinlock trim pulse time
       mix = mixing time  (can be arrayed)
        nt = min:  multiple of 2
             max:  multiple of 8  (recommended)

   mfsat='y'
           Multi-frequency saturation. 
           Requires the frequency list mfpresat.ll in current experiment
           Pbox creates (saturation) shape "mfpresat.DEC"

                  use mfll('new') to initialize/clear the line list
                  use mfll to read the current cursor position into
                  the mfpresat.ll line list that is created in the
                  current experiment. 

           Note: copying pars or fids (mp or mf) from one exp to another does not copy
                 mfpresat.ll!
           Note: the line list is limited to 128 frequencies ! 
            E.Kupce, Varian, UK June 2005 - added multifrequency presat option 
           TRANSMITTER MUST BE POSITIONED AT SOLVENT FREQUENCY 
             this pulse sequence requires a T/R switch,
             linear amplifiers and computer-controlled attenuators on the
             observe channel.

        contact- G. Gray (palo alto)  revision- from tocsy.c
	P.A.Keifer - 940916 - fixed d2 timing


        made C13refoc flag for C13 decoupling in t1.
        15N refocussing done as if N15refoc='y'
        Both 13C and 15N refocussing done if CNrefoc='y' 
        Dropped power 3db down for both N15 and 13C if simulaneous 180's.
        Proper refocussing in t1 limited by bandwidth of 13C pulse in t1
        Uses composite 13C 180 in t1.

        revised to let the parameter "strength" control amplitude
        use pw for first pulse at power tpwr
         (G.Gray, Feb 2005)

        moved rcvroff() statement to just after status(A) statement to 
          enable successful use of mfsat (Josh Kurutz, U. of Chicago May06)
 */


#include <standard.h>
#include <mfpresat.h>

mleva(slpw,window)
double slpw,window;
{
   txphase(v2); delay(slpw);
   xmtroff(); delay(window); xmtron();
   txphase(v3); delay(2 * slpw);
   xmtroff(); delay(window); xmtron();
   txphase(v2); delay(slpw);
}

mlevb(slpw,window)
double slpw,window;
{
   txphase(v4); delay(slpw);
   xmtroff(); delay(window); xmtron();
   txphase(v5); delay(2 * slpw);
   xmtroff(); delay(window); xmtron();
   txphase(v4); delay(slpw);
}


pulsesequence()
{
   double         
                   trim,
                   rfmod,   /* fine power for tocsy spinlock */
                   mix,
                   compH,
                   slpw,
                   pwNlvl,
                   pwN,
                   pwClvl,
                   pwC,
                   window,
                   strength,
                   cycles;

   char  mfsat[MAXSTR],sspul[MAXSTR],C13refoc[MAXSTR],N15refoc[MAXSTR],CNrefoc[MAXSTR];
   

/* LOAD AND INITIALIZE VARIABLES */
   pwNlvl=getval("pwNlvl");
   pwClvl=getval("pwClvl");
   pwN=getval("pwN");
   pwC=getval("pwC");
   compH=getval("compH");
   strength=getval("strength");
  

/* LOAD AND INITIALIZE VARIABLES */
   mix = getval("mix");
   trim = getval("trim");
   window=getval("window");
   getstr("mfsat", mfsat);
   getstr("sspul", sspul);
   getstr("C13refoc",C13refoc);
   getstr("N15refoc",N15refoc);
   getstr("CNrefoc",CNrefoc);

/* CHECK CONDITIONS */
   if ((phase1 != 3) && (arrayelements > 2))
   {
      fprintf(stdout, "PHASE=3 is required if MIX is arrayed!\n");
      psg_abort(1);
   }
   if (satdly > 9.999)
   {
      fprintf(stdout, "Presaturation period is too long.\n");
      psg_abort(1);
   }
   if ((rof1 < 9.9e-6) && (ix == 1))
      fprintf(stdout,"Warning:  ROF1 is less than 10 us\n");

   if (satpwr > 20)
        {
         printf("satpwr too large  - acquisition aborted.\n");
         psg_abort(1);
        }

/* CHECK CONDITIONS */
   if ((dm[A]=='y') || (dm2[A] == 'y') || (dm[B] == 'y') || (dm2[B] == 'y'))
     {
      fprintf(stdout,"Set dm and dm2 to be nnn or nny\n");
      psg_abort(1);
     }

/* STEADY-STATE PHASECYCLING */
/* This section determines if the phase calculations trigger off of (SS - SSCTR)
   or off of CT */

   ifzero(ssctr);
      hlv(ct, v13);
      mod2(ct, v1);
      hlv(ct, v2);
   elsenz(ssctr);
      sub(ssval, ssctr, v12);	/* v12 = 0,...,ss-1 */
      hlv(v12, v13);
      mod2(v12, v1);
      hlv(v12, v2);
   endif(ssctr);


/* CALCULATE PHASES */
/* A 2-step cycle is performed on the first pulse (90 degrees) to suppress
   axial peaks in the first instance.  Second, the 2-step F2 quadrature image
   suppression subcycle is added to all pulse phases and receiver phase.
   Finally, a 2-step cycle is performed on the spin-lock pulses. */

   mod2(v13, v13);
   dbl(v1, v1);
   incr(v1);
   hlv(v2, v2);
   mod2(v2, v2);
   dbl(v2, v2);
   incr(v2);
   add(v13, v2, v2);
   sub(v2, one, v3);
   add(two, v2, v4);
   add(two, v3, v5);
   add(v1, v13, v1);
   assign(v1, oph);
   if (phase1 == 2)
      incr(v1);
   if (phase1 == 3)
      add(v1, id2, v1);

 /*HYPERCOMPLEX MODE USES REDFIELD TRICK TO MOVE AXIAL PEAKS TO EDGE*/
   if ((phase1==1)||(phase1==2))
   {
      initval(2.0*(double)(d2_index%2),v6);
      add(v1,v6,v1); add(oph,v6,oph);
   } 

  slpw=1/(4.0*strength);
  rfmod=4095*(compH*pw/slpw); 

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
         zgradpulse(5000.0,.001);
         rgpulse(pw, zero, rof1, 0.0e-6);
         zgradpulse(3000.0,.001);
      }
      delay(d1);
     rcvroff();
     if (satmode[A] == 'y')
     {
      if (mfsat[A] == 'y')
       {mfpresat_on(); delay(satdly); mfpresat_off();}
      else
       {
        obspower(satpwr);
        rgpulse(satdly,zero,rof1,rof2);
       }
      obspower(tpwr);
     }
   status(B);
      rgpulse(pw, v1, rof1, 0.0);
      if (d2>0.0)
      {
        if ((C13refoc[A] == 'n') &&  (N15refoc[A] == 'n') && (CNrefoc[A] == 'n'))
         if (d2 > (2.0*pw/PI-POWER_DELAY))  
           delay(d2-2.0*pw/PI-POWER_DELAY);

        if ((C13refoc[A] == 'n') && (N15refoc[A] == 'n') && (CNrefoc[A] == 'y'))
         {
          if (pwN > 2.0*pwC)
           {
            if (d2/2.0 > (pwN +0.64*pw))
             {
               delay(d2/2.0-pwN-2.0*pw/PI);
               dec2rgpulse(pwN-2.0*pwC,zero,0.0,0.0);
               sim3pulse(0.0,pwC,pwC, zero,zero,zero, 0.0, 0.0);
               sim3pulse(0.0,2.0*pwC,2.0*pwC, zero,one,zero, 0.0, 0.0);
               sim3pulse(0.0,pwC,pwC, zero,zero,zero, 0.0, 0.0);
               dec2rgpulse(pwN-2.0*pwC,zero,0.0,0.0);
               delay(d2/2.0-pwN-POWER_DELAY);
             }
            else
               delay(d2-2.0*pw/PI-POWER_DELAY);
           }
          else
           {
           if (d2/2.0 > (pwN +pwC+ 2.0*pw/PI))  
            {
               delay(d2/2.0-pwN-pwC-2.0*pw/PI);
               decrgpulse(pwC,zero,0.0,0.0);
               sim3pulse(0.0,2.0*pwC,2.0*pwN, zero,one,zero, 0.0, 0.0);
               decrgpulse(pwC,zero,0.0,0.0);
               delay(d2/2.0-pwN-pwC-POWER_DELAY);
            }
           else
               delay(d2-2.0*pw/PI-POWER_DELAY);
           }
        }

        if ((C13refoc[A] == 'n') && (N15refoc[A] == 'y') && (CNrefoc[A] == 'n'))
         {
         if (d2/2.0 > (pwN + 2.0*pw/PI))  
          {
           delay(d2/2.0-pwN-2.0*pw/PI);
           dec2rgpulse(2.0*pwN, zero, 0.0, 0.0);
           delay(d2/2.0-pwN-POWER_DELAY);
          }
           else
               delay(d2-2.0*pw/PI-POWER_DELAY);
         }

        if ((C13refoc[A] == 'y') &&  (N15refoc[A] == 'n') && (CNrefoc[A] == 'n'))
         {
         if (d2/2.0 > (2.0*pwC + 2.0*pw/PI))  
          {
             delay(d2/2.0-2.0*pwC-2.0*pw/PI);
             decrgpulse(pwC,zero,0.0,0.0);
             decrgpulse(2.0*pwC, one, 0.0, 0.0);
             decrgpulse(pwC,zero,0.0,0.0);
             delay(d2/2.0-2.0*pwC-POWER_DELAY);
          }
           else
               delay(d2-2.0*pw/PI-POWER_DELAY);
         }
      }
      obsunblank();
      obspwrf(rfmod); 
      txphase(v3);
      xmtron();
      delay(trim);
/* CALCULATE AND INITIALIZE LOOP COUNTER */
      if (slpw > 0.0)
      {
         cycles = (mix - trim) / (64.66*slpw+32*window);
         cycles = 2.0*(double) (int) (cycles/2.0);
      }
      else
      {
         cycles = 0.0;
      }
      initval(cycles, v9);			/* V9 is the MIX loop count */
      if (cycles > 0.5)
      {
         starthardloop(v9);
 mleva(slpw,window); mleva(slpw,window); mlevb(slpw,window); mlevb(slpw,window);
 mlevb(slpw,window); mleva(slpw,window); mleva(slpw,window); mlevb(slpw,window);
 mlevb(slpw,window); mlevb(slpw,window); mleva(slpw,window); mleva(slpw,window);
 mleva(slpw,window); mlevb(slpw,window); mlevb(slpw,window); mleva(slpw,window);
      txphase(v3); delay(0.66*pw);
         endhardloop();
      }
      xmtroff();
      decpower(dpwr); dec2power(dpwr2);  
/* detection */
      delay(rof2);
      rcvron();
      obsblank();
   status(C);
}
