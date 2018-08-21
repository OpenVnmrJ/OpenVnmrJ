/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* gNhsqc_IPAPHD.c    "N15 IPAP-hsqc using watergate suppression"
                      "and optional HH homodecoupling in t2"

                      Ref: JMR v131, 373-378 (1998)

   With IPAP='n','y' phase=1,2 array='IPAP,phase'
   you can acquire both antiphase and in-phase spectra at the same time. To
   obtain both components with the same phase use
        wft2d(1,0,0,0,0,0,0,0,0,0,-1,0,0,0,0,0)     for in-phase signals
        wft2d(0,0,0,0,0,0,1,0,0,0,0,0,1,0,0,0)      for anti-phase signals

   To look at individual satellites you can also build sum and difference of
   the two data sets:
        wft2d(1,0,0,0,0,0,1.11,0,0,0,-1,0,1.11,0,0,0)       sum
        wft2d(1,0,0,0,0,0,-1.11,0,0,0,-1,0,-1.11,0,0,0)     difference

   Note the factor of 1.11. You can adjust this value to get good match.

   To measure exact signal positions in these component spectra, you need
   line fitting software (such as "fitspec" / deconvolution).

   program written by Nagarajan Murali starting from the gNfhsqc.c 
   Feb. 26, 2001
   
   Modified by Eriks Kupce on 17.05.2002 :
   Added homo-decoupling and soft watergate options; fixed phase cycling.   
   
   HH homo-decoupling:
   ===================
   Activated by setting the Hdecflg='y'. The dsp flag should be set to 'n'.
   The decoupling waveform is created automatically at the 'go' (or 'dps') time.
   Recommended Cdecflg='y' - use low power WURST-40 decoupling for long range 
   C-H couplings. The decoupling waveform is created automatically at the 'go' 
   (or 'dps') time.                
   Note - complete de-phasing of water is the key requirement for obtaining good
   quality spectra with HH homo-decoupling !!! This may reduce the overall 
   sensitivity in proteins with fast exchanging NH groups. Note also that the  
   selective pulses in watergate are not affected by radiation damping.  
   The residual signal from water is eliminated by phase cycling and hence the
   spectra will improve with increasing ss and nt.                   
   The power levels for HH and CH decoupling can be changed by setting Hdecpwr
   and Cdecpwr to non-zero values. If Hdecpwr=0 or Cdecpwr=0, the calculated 
   values will be used.
   With Hdecflg = 'y' the alfa delay can be relatively large and negative, 
   presumably due to phase gradients introduced by Bloch-Siegert effects.
   (Correct the phase in F2 and use calfa command to set the right alfa delay).
   The effect from HH homo-decoupling is best appreciated, if the HH coupling
   is resolved. This usually requires fairly long acquisition times (at >= 0.1s),
   a good digital resolution (fn >= 4k) and a sensible window function. For the
   purpose of comparison the HH decoupling can be switched off by Hdecflg='off'.
   The resolution is further improved by using C-13 decoupling during acquisition. 
   This is best achieved using low-power WURST-40 decoupling designed for long 
   range CH decoupling, which is also created automatically. Just set Cdecflg = 'y'. 

   Refs.: Kupce and Wagner, JMR B109, 329 (1995).
          M. Pellecchia et al, J. Biomol. NMR, v.15, p.335 (1999).
   
*/


#include <standard.h>
#include "Pbox_psg.h"

static double d2_init = 0.0;

static int phi1[2]  = {3,1},
           phi2[4]  = {0,0,2,2},	/* phase for IP */
	   phi2A[4] = {3,3,1,1},	/* phase for AP */
           phi3[16] = {0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3}, 
           phi4[16] = {0,0,0,0,0,0,0,0,2,2,2,2,2,2,2,2},
           rec[4]   = {0,2,2,0}, 	 /* receiver phase for IP */
	   recA[8]  = {0,2,2,0,2,0,0,2}; /* receiver phase for AP */

shape   HHdseq, Cdseq;

pulsesequence()
{
  void      makeHHdec(), makeCdec(); 	                  /* utility functions */
  int       ihh=1,        /* used in HH decouling to improve water suppression */
            t1_counter;
  char	    C13refoc[MAXSTR],		/* C13 sech/tanh pulse in middle of t1 */
	    Hdecflg[MAXSTR],                        /* HH-homo decoupling flag */
	    Cdecflg[MAXSTR],                 /* low power C-13 decoupling flag */
	    IPAP[MAXSTR],		       /* Flag for anti-phase spectrum */
            wtg3919[MAXSTR];
  double    tauxh, tau1,
            pwNt = 0.0,               /* pulse only active in the TROSY option */
            gsign = 1.0,
            maxHpwr = 51.0,            /* maximum allowed H-H decoupling power */
            maxCpwr = 45.0,            /* maximum allowed C-H decoupling power */
            gzlvl0=getval("gzlvl0"),
            gzlvl1=getval("gzlvl1"),
            gzlvl2=getval("gzlvl2"),
            gzlvl3=getval("gzlvl3"),
            gt0=getval("gt0"),
            gt1=getval("gt1"),
            gt2=getval("gt2"),
            gt3=getval("gt3"),
            JNH = getval("JNH"),
            pwN = getval("pwN"),
            pwNlvl = getval("pwNlvl"),  
            pwHs, tpwrs=0.0, compH,      /* H1 90 degree pulse length at tpwrs */               
            sw1 = getval("sw1"),
            pwClvl = getval("pwClvl"), 	         /* coarse power for C13 pulse */
            pwC = getval("pwC"),       /* C13 90 degree pulse length at pwClvl */
            rfst = 4095.0,	            /* fine power for the stCall pulse */
            compC = getval("compC");   /* adjustment for C13 amplifier compr-n */


/* INITIALIZE VARIABLES */

    getstr("C13refoc",C13refoc);
    getstr("IPAP",IPAP);	       /* IPAP = 'y' for AP; IPAP = 'n' for IP */
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

    if(Cdecflg[0] == 'y') makeCdec(maxCpwr);     /* make shapes for HH homo-decoupling */
    if(Hdecflg[0] == 'y') makeHHdec(maxHpwr);
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

/* LOAD VARIABLES */

    if(ix == 1) d2_init = d2;
    t1_counter = (int) ( (d2-d2_init)*sw1 + 0.5);
    
    tau1 = d2/2.0;
    if(tau1 < 0.0) tau1 = 0.0;

/* LOAD PHASE TABLES */

    settable(t1, 2, phi1);
    if (IPAP[A]=='y') settable(t2,4,phi2A);
    else settable(t2, 4, phi2); 
    settable(t3, 16, phi3);
    settable(t4, 16, phi4);
    if (IPAP[A]=='y') settable(t5,8,recA);
    else settable(t5, 4, rec);

    assign(one,v7); 
    assign(three,v8);     

    if ( phase1 == 2 )         /* Hypercomplex in t1 */
    {
      if (IPAP[A] == 'y') 
        tsadd(t3,1,4); 
      tsadd(t2, 1, 4); 
    } 
                                   
    if(t1_counter %2)          /* calculate modification to phases based on */
    { 			       /* current t1 values */
      tsadd(t2,2,4); 
      tsadd(t5,2,4); 
      if (IPAP[A] == 'y') 
        tsadd(t3,2,4); 
    }

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
       zgradpulse(1.5*gzlvl0, 0.5e-3);
       delay(5.0e-4);
       rgpulse(pw,zero,rof1,0.0);                 
       rgpulse(pw,one,0.0,rof1);                 
       zgradpulse(-gzlvl0, 0.5e-3);
     }
     
     delay(d1);
     rcvroff();
     
   status(B);

     dec2rgpulse(pwN,zero,rof1,rof1);
     zgradpulse(gzlvl0,gt0);
     delay(1e-3); 
     
     rgpulse(pw, zero, rof1, rof1);
     
     zgradpulse(gzlvl1,gt1);
     txphase(zero);
     dec2phase(zero);
     delay(tauxh-gt1);               /* delay=1/4J(XH)   */

     sim3pulse(2*pw,0.0,2*pwN,zero,zero,zero,rof1,rof1);

     zgradpulse(gzlvl1,gt1);
     dec2phase(t2);
     delay(tauxh-gt1 );               /* delay=1/4J(XH)   */
  
     rgpulse(pw, t1, rof1, rof1);

     zgradpulse(0.5*gsign*ihh*gzlvl2,gt2);
     delay(200.0e-6); 
     decphase(zero);
            
     txphase(t4);      
     dec2rgpulse(pwN, t2, rof1, 0.0);

     if (IPAP[A] == 'y') 
     {
      delay(tauxh-pwN); 
      sim3pulse(2*pw,0.0,2*pwN,zero,zero,t3,rof1,rof1);
      delay(tauxh-pwN);
      rgpulse(pw,t4,rof1,rof1);
     }	
        
     if ( (C13refoc[A]=='y') && (tau1 > 0.5e-3 + WFG2_START_DELAY) )
     {
       delay(tau1 - 0.5e-3 - WFG2_START_DELAY); 
       simshaped_pulse("", "stC200", 0.0, 1.0e-3, zero, zero, 0.0, 0.0);  
       dec2phase(zero);  
       delay(tau1 - 0.5e-3 - WFG2_STOP_DELAY);
     }
     else 
       delay(2.0*tau1);
       
     dec2rgpulse(pwN, zero, 0.0, 0.0);
       
     zgradpulse(0.5*gzlvl2,gt2);
     delay(200.0e-6);
     rgpulse(pw, zero, rof1, rof1); 
     
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
       obspower(tpwrs);  
       shaped_pulse("H2Osinc", pwHs, v7, rof1, 0.0);
       obspower(tpwr);
       sim3pulse(2.0*pw, 0.0, 2.0*pwN, v8, zero, zero, 0.0, 0.0);
       obspower(tpwrs);
       shaped_pulse("H2Osinc", pwHs, v7, rof1, 0.0);
       obspower(tpwr);
     } 
        
     zgradpulse(gzlvl3,gt3);   

     if(Cdecflg[0] == 'y')
     {
       delay(tauxh-gt3-pwHs-rof1-pwNt-3.0*POWER_DELAY-PRG_START_DELAY); 
       dec2rgpulse(pwNt, zero, rof1, rof2); 
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
       dec2rgpulse(pwNt, zero, rof1, rof2); 
       dec2power(dpwr2);
       rcvron();

     statusdelay(C,5.0e-5);
       setreceiver(t5);  
      
       if(Hdecflg[0] == 'y')
         homodec(&HHdseq); 
     }       
}



/* ------------------------------ UTILITY FUNCTIONS ------------------------------ */

void makeCdec(maxpwr)
double maxpwr;
{
  double  Cppm = getval("dfrq"),               /*  pre-set C decoupling parameters */        
          cbw = 250.0*Cppm,                     /* C-decoupling bandwidth, 250 ppm */
          cpw = 0.006,                            /* C-decoupling pulsewidth, 6 ms */
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
      Cdseq.pwr = Cdseq.pwr + 1.0;                 /* optional 1 dB power increase */
    }   
    if ((find("Cdecpwr") == 4) && (Cdecpwr = getval("Cdecpwr")))
      Cdseq.pwr = Cdecpwr;              /* if requested, set to user defined power */
    setlimit("Cdseq.pwr", Cdseq.pwr, maxpwr);
}

void makeHHdec(maxpwr)
double maxpwr;
{
  double  ppm = getval("sfrq"),                /* pre-set HH decoupling parameters */        
          hhbw = 3.0*ppm,                             /* homo-decoupling bandwidth */
          hhpw = 0.020,                              /* homo-decoupling pulsewidth */
          hhofs = -0.25*ppm,                    /* homo-decoupling offset from H2O */
          hhstp = 1e6/getval("sw"),              /* homo-decoupling shape stepsize */
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
      HHdseq.pwr = Hdecpwr;            /* if requested, set to user defined power */
    setlimit("HHdseq.pwr", HHdseq.pwr, maxpwr);
}

