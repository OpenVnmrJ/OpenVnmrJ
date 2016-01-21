/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* gNfhsqc_CCLS_U.c


   CCLS experiment on uniformly labeled samples for sequence  testing
   set CCLS='y' to turn on suppression of NH attached to 13CO
   set CCLS='n' to collect regular spectrum
   timeCT=0.016 will give maximum suppression of HN-CO signals
  
   The CCLS sequences are useful for proteins where only few amino acids
   are labeled to figure out which pairs of labeled amino acids are next 
   to each other. This sequence should be used when the labeled amino
   acids have C13 at both C' and Calfa positions, otherwise if only
   C' is labeled, gNfhsqc_CCLS.c should be used.

   recommend running the experiment interleaved:
   CCLS='n','y' bs=16 il='y'
   ni=64 phase=1,2 f1180=y

   wft2d(1,0,0,0, 0,0,0,0, 0,0,-1,0, 0,0,0,0)
     will give reference spectrum CCLS='n'
   wft2d(0,0,0,0, 1,0,0,0, 0,0,0,0, 0,0,-1,0) 
    will give CO suppressed spectrum CCLS='n'
   wft2d(1,0,0,0, -1,0,0,0, 0,0,-1,0, 0,0,1,0)
     will give difference spectrum
     (but S/N will be reduced)

   dof is set to CO region (174ppm)

    See "Carbonyl carbon label selective (CCLS) 1H-15N HSQC
    experiment for improved detection of backbone 13C-15N
    cross peaks in larger proteins", J. Biomol.NMR, 39, 177-185 (2007),
    Marco Tonelli, Larry R. Masterson, Klaas Hallenga, Gianiuigi Veglia
    and John Markley.

*/


#include <standard.h>
#include "Pbox_bio.h"               /* Pbox Bio Pack Pulse Shaping Utilities */

static int phi1[4] = {1,1,3,3},
           phi2[2] = {0,2},
           phi3[8] = {0,0,0,0,2,2,2,2}, 
           phi4[16] = {0,0,0,0,0,0,0,0,2,2,2,2,2,2,2,2},
           rec[8]  = {2,0,0,2,0,2,2,0},
                                
           recT[4] = {1,3,2,0};                              /* minimum nt = 4 */

static double d2_init = 0.0;

static shape H2Osinc, offC3, offC6, offC8;

pulsesequence()
{
  int       t1_counter;
  char	    CCLS[MAXSTR],	      /* C13 refocussing pulse in middle of t1 */
            wtg3919[MAXSTR],
	    f1180[MAXSTR];   		       /* Flag to start t1 @ halfdwell */

  double    timeCT=getval("timeCT"),
 	    tauxh, tau1,
            gzlvl3=getval("gzlvl3"),
            gzlvl4=getval("gzlvl4"),
            gt3=getval("gt3"),
            gt4=getval("gt4"),
            gstab=getval("gstab"),			/* gradient recovery delay */
            JNH = getval("JNH"),
            pwN = getval("pwN"),
            pwNlvl = getval("pwNlvl"),  
            pwHs, tpwrs=0.0, compH=1.0,          /* H1 90 degree pulse length at tpwrs */               
            sw1 = getval("sw1"),

                               /* temporary Pbox parameters */
            bw, ofs, ppm, /* bandwidth, pulsewidth, offset, ppm, # steps */

            pwC3=getval("pwC3"),		   /*180 degree pulse at Ca(56ppm) null at CO(174ppm) */
            pwC6=getval("pwC6"),                  /* 90 degree selective sinc pulse on CO(174ppm) */
            pwC8=getval("pwC8"),                  /* 180 degree selective sinc pulse on CO(174ppm) */
            rf3= 4095.0,	           /* fine power for the pwC3 ("offC3") pulse */
            rf6= 4095.0,	           /* fine power for the pwC6 ("offC6") pulse */
            rf8= 4095.0,	           /* fine power for the pwC8 ("offC8") pulse */
            pwClvl = getval("pwClvl"), 	         /* coarse power for C13 pulse */
            pwC = getval("pwC"),       /* C13 90 degree pulse length at pwClvl */
            compC = getval("compC");   /* adjustment for C13 amplifier compr-n */

    getstr("CCLS",CCLS);
    getstr("wtg3919",wtg3919);
    getstr("f1180",f1180);
    
/* check validity of parameter range */

    if((dm[A] == 'y' || dm[B] == 'y' || dm[C] == 'y' ))
    { text_error("incorrect Dec1 decoupler flags!  "); psg_abort(1); } 

    if((dm2[A] == 'y' || dm2[B] == 'y') )
    { text_error("incorrect Dec2 decoupler flags!  "); psg_abort(1); } 

    if( dpwr2 > 50 )
    { text_error("don't fry the probe, dpwr2 too large!  "); psg_abort(1); }

/* INITIALIZE VARIABLES */
    
    if(wtg3919[0] != 'y')      /* selective H20 one-lobe sinc pulse needs 1.69  */
    {                                   /* times more power than a square pulse */
      pwHs = getval("pwHs");            
      compH = getval("compH");
    }
    else 
      pwHs = pw*2.385+7.0*rof1+d3*2.5; 

    tauxh = ((JNH != 0.0) ? 1/(4*(JNH)) : 2.25e-3);

    setautocal();                        /* activate auto-calibration flags */ 
        
    if (autocal[0] == 'n') 
    {
    /* offC3 - 180 degree pulse on Ca, null at CO 118ppm away */
        rf3 = (compC*4095.0*pwC*2.0)/pwC3;
	rf3 = (int) (rf3 + 0.5);  
	
    /* 90 degree one-lobe sinc pulse on CO, null at Ca 118ppm away */	
	rf6 = (compC*4095.0*pwC*1.69)/pwC6;	/* needs 1.69 times more     */
	rf6 = (int) (rf6 + 0.5);		/* power than a square pulse */

    /* 180 degree one-lobe sinc pulse on CO, null at Ca 118ppm away */
	rf8 = (compC*4095.0*pwC*2.0*1.65)/pwC8;	/* needs 1.65 times more     */
	rf8 = (int) (rf8 + 0.5);		      /* power than a square pulse */

      if(wtg3919[0] != 'y')      /* selective H20 one-lobe sinc pulse needs 1.69  */
       {
        if (pwHs > 1e-6) 
         tpwrs = tpwr - 20.0*log10(pwHs/(compH*pw*1.69));  
        else 
         tpwrs = 0.0;
        tpwrs = (int) (tpwrs); 
       }	  
    }
    else        /* if autocal = 'y'(yes), 'q'(quiet), r(read), or 's'(semi) */
    {
      if(FIRST_FID)                                            /* call Pbox */
      {
        ppm = getval("dfrq"); 
        bw = 118.0*ppm; ofs = -bw; 
        offC3 = pbox_make("offC3", "square180n", bw, ofs, compC*pwC, pwClvl);
        offC6 = pbox_make("offC6", "sinc90n", bw, 0.0, compC*pwC, pwClvl);
        offC8 = pbox_make("offC8", "sinc180n", bw, 0.0, compC*pwC, pwClvl);

        if(wtg3919[0] != 'y')
          H2Osinc = pbox_Rsh("H2Osinc", "sinc90", pwHs, 0.0, compH*pw, tpwr);
      }
      pwC3 = offC3.pw; rf3 = offC3.pwrf;             
      pwC6 = offC6.pw; rf6 = offC6.pwrf; 
      pwC8 = offC8.pw; rf8 = offC8.pwrf;

      if (wtg3919[0] != 'y') 
        { pwHs = H2Osinc.pw; tpwrs = H2Osinc.pwr-1.0; } /* 1dB correction applied */ 
    }

/* LOAD VARIABLES */

    if(ix == 1) d2_init = d2;
    t1_counter = (int) ( (d2-d2_init)*sw1 + 0.5);
    
/*  Set up f1180  */
   
    tau1 = d2;
    if((f1180[A] == 'y') && (ni > 1.0)) 
	{ tau1 += ( 1.0 / (2.0*sw1) ); if(tau1 < 0.2e-6) tau1 = 0.0; }
    tau1 = tau1/2.0;


/* LOAD PHASE TABLES */

      settable(t6, 4, recT); 
      assign(one,v7); 
      assign(three,v8);
      settable(t1, 4, phi1);
      settable(t2, 2, phi2);
      settable(t3, 8, phi3);
      settable(t4, 16, phi4);
      settable(t5, 8, rec);  

      if ( phase1 == 2 ) tsadd(t2, 1, 4); 
                                   
    if(t1_counter %2)          /* calculate modification to phases based on */
    { tsadd(t2,2,4); tsadd(t5,2,4); tsadd(t6,2,4); }   /* current t1 values */

    if(wtg3919[0] != 'y') 
    { add(one,v7,v7); add(one,v8,v8); }
         
                           /* sequence starts!! */
   status(A);
     
     obspower(tpwr);
     dec2power(pwNlvl);
     decpower(pwClvl); decpwrf(4095.0);
     delay(d1);
     
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

     decphase(zero);
     txphase(t4);      
     zgradpulse(gzlvl3,gt3);
     delay(gstab); 

       dec2rgpulse(pwN, t2, rof1, rof1);
/* CT EVOLUTION BEGINS */
       dec2phase(t3);

       if (CCLS[A]=='y')
         {
          decpwrf(rf8);
          delay(timeCT -SAPS_DELAY -WFG3_START_DELAY -PWRF_DELAY -tau1);

          sim3shaped_pulse("","offC8","",0.0, pwC8, 2.0*pwN, zero, zero, zero, 0.0, 0.0);

          decpwrf(rf3);
          delay(timeCT -2.0*pw -pwC3 -WFG_START_DELAY -PWRF_DELAY);

          decshaped_pulse("offC3",pwC3, zero, 0.0, 0.0);
          rgpulse(2.0*pw, t4, 0.0, 0.0);
         }
        else
         {
          delay(timeCT -SAPS_DELAY -tau1);

          dec2rgpulse(2.0*pwN, zero, 0.0, 0.0);

          decpwrf(rf3);
          delay(timeCT -pwC8 -pwC3 -2.0*WFG_START_DELAY -2.0*PWRF_DELAY -rof1);

          decshaped_pulse("offC3",pwC3, zero, 0.0, 0.0);
          decpwrf(rf8);
          simshaped_pulse("","offC8", 2.0*pw, pwC8, t4, zero, rof1, 0.0);
         }
       delay(tau1);
/* CT EVOLUTION ENDS */
       dec2rgpulse(pwN, t3, rof1, rof1);
       
       decpwrf(rf6);
       zgradpulse(gzlvl3,gt3);
       delay(gstab);

       rgpulse(pw, two, rof1, rof1);
       decshaped_pulse("offC6",pwC6, zero, rof1, rof1);  

     zgradpulse(gzlvl4,gt4);
     txphase(v7); dec2phase(zero);
     delay(tauxh -gt4 -pwHs -rof1 -pwC6 -WFG_START_DELAY -2.0*rof1);
     
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
        
     zgradpulse(gzlvl4,gt4);   
     delay(tauxh -gt4 -pwHs -rof1 -POWER_DELAY); 
     dec2power(dpwr2);

   status(C);
     setreceiver(t5);   
}



