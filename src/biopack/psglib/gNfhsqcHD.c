/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* gNfhsqcHD.c    "Fast N15 hsqc using watergate suppression and optional "
                 1H homonuclear decoupling"

   minimum step cycling is 2
   program written by Susumu Mori and Chitrananda Abeygunawardana,Johns Hopkins
   modified for 3rd Channel operation BKJ 970430 
   TROSY option is included. When TROSY is selected use nt=8
   
   Modified by Eriks Kupce :
   the TROSY phase cycle corrected to pick the correct component and to use the 
   sensitivity enhanced version. The minimum phase cycle for TROSY reduced to 4 steps.
   Corrected the d2 timing in the TROSY version. Added soft watergate option;   
   Use f1coef = '1 0 -1 0 0 1 0 1' for TROSY
   E.K., 17.05.2002.
   
   HH homo-decoupling:
   ===================
   Activated by setting the Hdecflg='y'. The dsp flag should be set to 'n'.
   The decoupling waveform is created automatically at the 'go' (or 'dps') time.

   Note - complete de-phasing of water is the key requirement for obtaining good
   quality spectra with HH homo-decoupling !!! This may reduce the overall s/n 
   in proteins with fast exchanging NH groups. Note also that the selective 
   pulses in watergate are not affected by Radiation Damping.  
   The residual signal from water is eliminated by phase cycling and hence the
   spectra will improve with increasing ss and nt.                   
   The power level for decoupling can be changed by setting Hdecpwr to a non-zero value.
   This may be necessary to improve water suppression.
   If Hdecpwr=0, the Pbox-calculated value will be used.

   Due to the Bloch-Siegert effects the alfa delay can be relatively large and 
   negative.

   The effect from HH homo-decoupling is greatest when the HH coupling
   is resolved. This usually requires a fairly long acquisition time (at >= 0.1s)
   a good digital resolution (fn >= 4k) and a sensible window function. For the
   purpose of comparison the HH decoupling can be switched off by Hdecflg='off'.
   This uses the same (explicit acquisition) pulse sequence but without decoupling.
   This is different from Hdecflg='n' where the sequence reverts to the normal
   gNfhsqc without explicit acquisition.

   CH decoupling:
   ==============
   Resolution is improved by using C-13 decoupling during acquisition. 
   This is best achieved using low-power WURST-40 decoupling designed for long 
   range CH decoupling (set Cdecflg = 'y'). 

   The power level for decoupling can be changed by setting Cdecpwr to a non-zero value.
   If Cdecpwr=0, the Pbox-calculated value will be used.

   The decoupling waveform is created automatically at the 'go' or 'dps' time.

   Ref.: Kupce and Wagner, JMR B109, 329 (1995).
   
*/


#include <standard.h>
#include "Pbox_psg.h"

static double d2_init = 0.0;

static int phi1[4] = {1,1,3,3},
           phi2[2] = {0,2},
           phi3[8] = {0,0,0,0,2,2,2,2}, 
           phi4[1] = {0},
           rec[8]  = {2,0,0,2,0,2,2,0},
                                
           phT1[1] = {1},                           /* phase cycling for TROSY */
           phT2[4] = {1,3,0,2},                     
           phT4[1] = {3},                     
           recT[4] = {1,3,2,0};                              /* minimum nt = 4 */

shape   HHdseq, Cdseq;

pulsesequence()
{
  void      makeHHdec(), makeCdec(); 	                  /* utility functions */
  int       ihh=1,        /* used in HH decoupling to improve water suppression */
            t1_counter;
  char	    C13refoc[MAXSTR],		/* C13 sech/tanh pulse in middle of t1 */
	    Hdecflg[MAXSTR],                        /* HH-homo decoupling flag */
	    Cdecflg[MAXSTR],                 /* low power C-13 decoupling flag */
            TROSY[MAXSTR],
            wtg3919[MAXSTR];
  double    tauxh, tau1,
            pwNt = 0.0,               /* pulse only active in the TROSY option */
            gsign = 1.0,
            gzlvl3=getval("gzlvl3"),
            gt3=getval("gt3"),
            JNH = getval("JNH"),
            pwN = getval("pwN"),
            pwNlvl = getval("pwNlvl"),  
            pwHs, tpwrs=0.0, compH,      /* H1 90 degree pulse length at tpwrs */               
            sw1 = getval("sw1"),
            pwClvl = getval("pwClvl"), 	         /* coarse power for C13 pulse */
            pwC = getval("pwC"),       /* C13 90 degree pulse length at pwClvl */
            rfst = 4095.0,	            /* fine power for the stCall pulse */
            compC = getval("compC"),   /* adjustment for C13 amplifier compr-n */
            tpwrsf = getval("tpwrsf");   /* adjustment for soft pulse power*/


/* INITIALIZE VARIABLES */

    getstr("C13refoc",C13refoc);
    getstr("TROSY",TROSY);
    getstr("wtg3919",wtg3919);
    getstr("Hdecflg", Hdecflg);
    getstr("Cdecflg", Cdecflg);
    
    tauxh = ((JNH != 0.0) ? 1/(4*(JNH)) : 2.25e-3);

    if (C13refoc[A]=='y')  /* 180 degree adiabatic C13 pulse from 0 to 200 ppm */
    {
      rfst = (compC*4095.0*pwC*4000.0*sqrt((30.0*sfrq/600.0+7.0)/0.35));   
      rfst = (int) (rfst + 0.5);
      if ( 1.0/(4000.0*sqrt((30.0*sfrq/600.0+7.0)/0.35)) < pwC )
      { 
        text_error( " Not enough C13 RF. pwC must be %f usec or less.\n", 
	          (1.0e6/(4000.0*sqrt((30.0*sfrq/600.0+7.0)/0.35))) );    
	psg_abort(1); 
      }
    }

    if(wtg3919[0] != 'y')      /* selective H20 one-lobe sinc pulse needs 1.69  */
    { pwHs = getval("pwHs");            /* times more power than a square pulse */
      compH = getval("compH");
      if (pwHs > 1e-6) tpwrs = tpwr - 20.0*log10(pwHs/(compH*pw*1.69));  
      else tpwrs = 0.0;
      tpwrs = (int) (tpwrs); }
    else
      pwHs = pw*2.385+7.0*rof1+d3*2.5;                       	  

    if(Cdecflg[0] == 'y') makeCdec();     /* make shapes for HH homo-decoupling */
    if(Hdecflg[0] == 'y') makeHHdec();
    if(Hdecflg[0] != 'n') ihh = -3;

/* check validity of parameter range */

    if((dm[A] == 'y' || dm[B] == 'y' || dm[C] == 'y' ))
    { text_error("incorrect Dec1 decoupler flags!  "); psg_abort(1); } 

    if((dm2[A] == 'y' || dm2[B] == 'y') )
    { text_error("incorrect Dec2 decoupler flags!  "); psg_abort(1); } 

    if( dpwr > 0 )
    { text_error("don't fry the probe, dpwr too large!  "); psg_abort(1); }

    if( dpwr2 > 50 )
    { text_error("don't fry the probe, dpwr2 too large!  "); psg_abort(1); }

    if ((TROSY[A]=='y') && (dm2[C] == 'y'))
    { text_error("Choose either TROSY='n' or dm2='n' ! "); psg_abort(1); }

/* LOAD VARIABLES */

    if(ix == 1) d2_init = d2;
    t1_counter = (int) ( (d2-d2_init)*sw1 + 0.5);
    
    tau1 = d2/2.0 - pw;
    if(tau1 < 0.0) tau1 = 0.0;

/* LOAD PHASE TABLES */

      settable(t6, 4, recT); 
    if (TROSY[A] == 'y')
    { gsign = -1.0;
      pwNt = pwN;
      assign(zero,v7); 
      assign(two,v8);
      settable(t1, 1, phT1);
      settable(t2, 4, phT2);
      settable(t3, 1, phT4); 
      settable(t4, 1, phT4);
      settable(t5, 4, recT); }
    else
    { assign(one,v7); 
      assign(three,v8);
      settable(t1, 4, phi1);
      settable(t2, 2, phi2);
      settable(t3, 8, phi3);
      settable(t4, 1, phi4);
      settable(t5, 8, rec); } 

      if ( phase1 == 2 )                  /* Hypercomplex in t1 */
      { if (TROSY[A] == 'y')          
        { tsadd(t3, 2, 4); tsadd(t5, 2, 4); }                      
        else tsadd(t2, 1, 4); }
                                   
    if(t1_counter %2)          /* calculate modification to phases based on */
    { tsadd(t2,2,4); tsadd(t5,2,4); tsadd(t6,2,4); }   /* current t1 values */

    if(wtg3919[0] != 'y') 
    { add(one,v7,v7); add(one,v8,v8); }
         
                           /* sequence starts!! */
   status(A);
     
     obspower(tpwr);
     dec2power(pwNlvl);
     decpower(pwClvl);
     decpwrf(rfst);
     if(Hdecflg[0] != 'n')
     {
       delay(5.0e-5);
       rgpulse(pw,zero,rof1,0.0);                 
       rgpulse(pw,one,0.0,rof1);                 
       zgradpulse(1.5*gzlvl3, 0.5e-3);
       delay(5.0e-4);
       rgpulse(pw,zero,rof1,0.0);                 
       rgpulse(pw,one,0.0,rof1);                 
       zgradpulse(-gzlvl3, 0.5e-3);
     }
     
     delay(d1);
     rcvroff();
     
   status(B);

     rgpulse(pw, zero, rof1, rof1);
     
     zgradpulse(0.3*gzlvl3,gt3);
     txphase(zero);
     dec2phase(zero);
     delay(tauxh-gt3);               /* delay=1/4J(XH)   */

     sim3pulse(2*pw,0.0,2*pwN,t4,zero,zero,rof1,rof1);

     zgradpulse(0.3*gzlvl3,gt3);
     dec2phase(t2);
     delay(tauxh-gt3 );               /* delay=1/4J(XH)   */
  
     rgpulse(pw, t1, rof1, rof1);

     zgradpulse(0.5*gsign*ihh*gzlvl3,gt3);
     delay(200.0e-6); 
     decphase(zero);
            
     if (TROSY[A] == 'y')
     { 
       txphase(t3);       
       if ( phase1 == 2 ) 
         dec2rgpulse(pwN, t6, rof1, 0.0);
       else 
         dec2rgpulse(pwN, t2, rof1, 0.0);              
       if ( (C13refoc[A]=='y') && (d2 > 1.0e-3 + 2.0*WFG2_START_DELAY) )
       {
         delay(d2/2.0 - 0.5e-3 - WFG2_START_DELAY);     
         decshaped_pulse("stC200", 1.0e-3, zero, 0.0, 0.0);
         delay(d2/2.0 - 0.5e-3 - WFG2_STOP_DELAY);
       }
       else
         delay(d2);

       rgpulse(pw, t3, 0.0, rof1);         
       zgradpulse(0.55*gzlvl3,gt3);
       delay(tauxh-gt3 );
       
       sim3pulse(2*pw,0.0,2*pwN,zero,zero,zero,rof1,rof1);
       
       zgradpulse(0.55*gzlvl3,gt3);
       delay(tauxh-gt3 );       
       sim3pulse(pw,0.0,pwN,zero,zero,t3,rof1,rof1);
     }
     else
     {         
       txphase(t4);      
       dec2rgpulse(pwN, t2, rof1, 0.0);
        
       if ( (C13refoc[A]=='y') && (tau1 > 0.5e-3 + WFG2_START_DELAY) )
       {
         delay(tau1 - 0.5e-3 - WFG2_START_DELAY); 
         simshaped_pulse("", "stC200", 2.0*pw, 1.0e-3, t4, zero, 0.0, 0.0);  
         dec2phase(t3);  
         delay(tau1 - 0.5e-3 - WFG2_STOP_DELAY);
       }
       else 
       {
         delay(tau1);
         rgpulse(2.0*pw, t4, 0.0, 0.0);
         dec2phase(t3);
         delay(tau1);
       }
       
       dec2rgpulse(pwN, t3, 0.0, 0.0);
       
       zgradpulse(0.5*gzlvl3,gt3);
       delay(200.0e-6);
       rgpulse(pw, two, rof1, rof1);
     } 
     
     zgradpulse(gzlvl3,gt3);
     txphase(v7); dec2phase(zero);
     delay(tauxh-gt3-pwHs-rof1+5.0e-5);
     
     if(wtg3919[0] == 'y')
     {     	
       rgpulse(pw*0.231,v7,rof1,rof1);     
       delay(d3);
       rgpulse(pw*0.692,v7,rof1,rof1);
       delay(d3);
       rgpulse(pw*1.462,v7,rof1,rof1);

       delay(d3/2-pwN);
       dec2rgpulse(2*pwN, zero, rof1, rof1);
       txphase(v8);
       delay(d3/2-pwN);

       rgpulse(pw*1.462,v8,rof1,rof1);
       delay(d3);
       rgpulse(pw*0.692,v8,rof1,rof1);
       delay(d3);
       rgpulse(pw*0.231,v8,rof1,rof1); 
     }
     else
     {
      if (tpwrsf<4095.0)
      {
       obspower(tpwrs+6.0); obspwrf(tpwrsf);  
       shaped_pulse("H2Osinc", pwHs, v7, rof1, 0.0);
       obspower(tpwr); obspwrf(4095.0);
       sim3pulse(2.0*pw, 0.0, 2.0*pwN, v8, zero, zero, 0.0, 0.0);
       obspower(tpwrs); obspwrf(tpwrsf);
       shaped_pulse("H2Osinc", pwHs, v7, rof1, 0.0);
       obspower(tpwr); obspwrf(4095.0);
      }
     else
      {
       obspower(tpwrs);  
       shaped_pulse("H2Osinc", pwHs, v7, rof1, 0.0);
       obspower(tpwr);
       sim3pulse(2.0*pw, 0.0, 2.0*pwN, v8, zero, zero, 0.0, 0.0);
       obspower(tpwrs);
       shaped_pulse("H2Osinc", pwHs, v7, rof1, 0.0);
       obspower(tpwr);
      }
     } 
        
     zgradpulse(gzlvl3,gt3);   

     if(Cdecflg[0] == 'y')
     {
       delay(tauxh-gt3-pwHs-rof1-pwNt-3.0*POWER_DELAY-PRG_START_DELAY); 
       dec2rgpulse(pwNt, zero, rof1, rof1); 
       dec2power(dpwr2);
       rcvron();

     statusdelay(C,5.0e-5);
       setreceiver(t5);  
       pbox_decon(&Cdseq);
      
       if(Hdecflg[0] == 'y')
         homodec(&HHdseq); 
     }
     else
     {
       delay(tauxh-gt3-pwHs-rof1-pwNt-POWER_DELAY); 
       dec2rgpulse(pwNt, zero, rof1, rof1); 
       dec2power(dpwr2);
       rcvron();

     statusdelay(C,5.0e-5);
       setreceiver(t5);  
      
       if(Hdecflg[0] == 'y')
         homodec(&HHdseq); 
     }       
}



/* ------------------------------ UTILITY FUNCTIONS -------------------------------- */

void makeCdec()
{
  double  Cppm = getval("dfrq"),                 /*  pre-set C decoupling parameters */        
          cbw = 250.0*Cppm,                       /* C-decoupling bandwidth, 250 ppm */
          cpw = 0.006,                              /* C-decoupling pulsewidth, 6 ms */
	  pwC = getval("pwC"),
	  pwClvl = getval("pwClvl"),
          compC = getval("compC"),
          Cdecpwr;
  char    cmd[MAXSTR];

            /* create low power decoupling for long range J(CH) and do it only once */
            
    if((getval("arraydim") < 1.5) || (ix==1))  
    {
      sprintf(cmd, "Pbox Cdec_lp -w \"WURST-40 %.2f/%.6f\" -s 4.0 -p %.0f -l %.2f -0",
                    cbw, cpw, pwClvl, 1.0e6*pwC*compC);
      system(cmd);
      Cdseq = getDsh("Cdec_lp");
      Cdseq.pwr = Cdseq.pwr + 1.0;                   /* optional 1 dB power increase */
    }   
    if ((find("Cdecpwr") == 4) && (Cdecpwr = getval("Cdecpwr")))
      Cdseq.pwr = Cdecpwr;              /* if requested, set to user defined power */
    setlimit("Cdseq.pwr", Cdseq.pwr, 45.0);
}

void makeHHdec()
{
  double  ppm = getval("sfrq"),                  /* pre-set HH decoupling parameters */        
          hhbw = 3.0*ppm,                               /* homo-decoupling bandwidth */
          hhpw = 0.020,                                /* homo-decoupling pulsewidth */
          hhofs = -0.25*ppm,                      /* homo-decoupling offset from H2O */
          hhstp = 1e6/getval("sw"),                /* homo-decoupling shape stepsize */
          compH = getval("compH"),
          Hdecpwr;
  char    cmd[MAXSTR];
      
            /* create shape for HH homo-decoupling and do it only once */
                        
    if((getval("arraydim") < 1.5) || (ix==1))  
    {
      sprintf(cmd, "Pbox hhdec -w \"CAWURST-8 %.2f/%.6f %.2f\" -s %.2f ",
                    hhbw, hhpw, hhofs, hhstp);
      sprintf(cmd, "%s -dcyc 0.05 -p %.0f -l %.2f -0\n", 
                    cmd, tpwr, 1.0e6*pw*compH);
      system(cmd);
      HHdseq = getDsh("hhdec"); 
    }   
    if ((find("Hdecpwr") == 4) && (Hdecpwr = getval("Hdecpwr")))
      HHdseq.pwr = Hdecpwr;              /* if requested, set to user defined power */
    setlimit("HHdseq.pwr", HHdseq.pwr, 51.0);
}

