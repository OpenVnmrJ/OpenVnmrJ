/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*  JIT_ghn_ca_coP.c  
    
    A 'just-in-time' HN(CA)CO TROSY experiment for the backbone assignment
    of large proteins with high sensitivity. Optional 2H decoupling.

        * Werner-Allen JW,
        * Jiang L,
        * Zhou P.

    Department of Biochemistry, Duke University Medical Center, Durham, NC 27710, USA.
    J Magn Reson. 2006 Jul;181(1):177-80

    HNCACO presents an unresolved sensitivity limitation due to fast 13CO transverse 
    relaxation and passive 13Calpha-13Cbeta coupling. This is a high-sensitivity 
    'just-in-time' (JIT) HN(CA)CO pulse sequence that uniformly refocuses 
    13Calpha-13Cbeta coupling while collecting 13CO shifts in real time. 
    Sensitivity comparisons of the 3-D JIT HN(CA)CO, a CT-HMQC-based control, 
    and a HSQC-based control with selective 13Calpha inversion pulses were performed 
    using a 2H/13C/15N labeled sample of the 29 kDa HCA II protein at 15 degrees C. 
    The JIT experiment shows a 42% signal enhancement over the CT-HMQC-based experiment.
    Compared to the HSQC-based experiment, the JIT experiment is 16% less sensitive for 
    residues experiencing proper 13Calpha refocusing and 13Calpha-13Cbeta decoupling. 
    However, for the remaining residues, the JIT spectrum shows a 106% average sensitivity
    gain over the HSQC-based experiment. The high-sensitivity JIT HNCACO experiment 
    should be particularly beneficial for studies of large proteins to provide 13CO
    resonance information regardless of residue type.

   
   carrier on CO, with phshift for the last 90 ca pulse 
   phshift=27.0 is optmized empirically.  

   HMQC type of CaCO experiment while refocusing CaCb.
   CO chemical shift evolution is done in a Just-In-time fashion. 

               CHOICE OF DECOUPLING AND 2D MODES

         Set dm  = 'nnn', dmm =  'ccc' 
         Set dm2 = 'nnn', dmm2 = 'ccc' 
         Set dm3 = 'nnn' for no 2H decoupling, or
                   'nyn' and dmm3 = 'cwc' for 2H decoupling. 

   scale_CaCO:  This parameter is used to enlarge the c13 evolution time.
                When scale_CaCO is set to 1.0, full sensitivity is obtained;
                otherwise, set scale_CaCO to 0.8, so that 90% signals would be obtained,
                however, CO dimension gets additional 1.8 ms evolution time.
                This parameter is fixed in the pulse sequence code, but could be
                modified and re-compiled

   created by Jon Werner-Allen and Pei Zhou  (Duke University) on 07/06/05
   aliphatic refocusing pulses have been changed to reburp pulses 12/11/05
   180 Cab pulse during t1 removed 3/19/06

   Added to BioPack, GG Varian, February 2007
*/


#include <standard.h>
#include "bionmr.h"

static int  
       phx[1]   = {0},
       phi3[2]  = {0,2},
       phi5[4]  = {0,0,2,2},
       phi8[8]  = {1,1,1,1,3,3,3,3},
       phi9[16] = {0,0,0,0,0,0,0,0,2,2,2,2,2,2,2,2},
       rec[8]   = {0,2,2,0,2,0,0,2};

pulsesequence()
{

/* DECLARE AND LOAD VARIABLES; parameters used in the last half of the */
/* sequence are declared and initialized as 0.0 in bionmr.h, and       */
/* reinitialized below  */

char        f1180[MAXSTR],                   /* Flag to start t1 @ halfdwell */
            f2180[MAXSTR],                   /* Flag to start t2 @ halfdwell */
            CaInvert[MAXSTR];            /* for reburp Ca/Cb refocusing pulses */
 
int   t1_counter, t2_counter, ni=getval("ni"), ni2 = getval("ni2");          

double 
   d2_init=0.0,      d3_init=0.0,                   
   tau1,                                                        /*  t1 delay */
   tauCaCO = getval("tauCaCO"),  /* J(CaCO)=55 Hz, delay for CO-Ca evolution */
   tauCaCb = getval("tauCaCb"),  /* J(CaCb)=35 Hz, delay for Ca-Cb evolution */
   scale_CaCO,           /* parameter used to increase CO digital resolution */
   timeTN = getval("timeTN"),             /* constant time for 15N evolution */
   lambda1,
   lambda2,
            
   pwClvl = getval("pwClvl"),                  /* coarse power for C13 pulse */
   pwC = getval("pwC"),              /* C13 90 degree pulse length at pwClvl */
   pw_CaInvert = getval("pw_CaInvert"),    /* pulse width of aliphatic pulse */
   pwlvl_CaInvert = getval("pwlvl_CaInvert"),             /* aliphatic pulse */

   pwNlvl = getval("pwNlvl"),                        /* power for N15 pulses */
   pwN = getval("pwN"),              /* N15 90 degree pulse length at pwNlvl */
   dpwr2 = getval("dpwr2"),                      /* power for N15 decoupling */

   pwCO90,                                        /* length of sinc 90 on CO */
   pwCa90,
   phshift = getval("phshift"),      /* phase shift on CO by 180 on Ca in t1 */
   pwCO180,                                      /* length of last C13 pulse */
   pwHs = getval("pwHs"),             /* H2O 90 degree pulse length at tpwrs */

   sw1 = getval("sw1"),   sw2 = getval("sw2"),
   gstab = getval("gstab"),

   gt1 = getval("gt1"),                gzlvl1 = getval("gzlvl1"),
                                       gzlvl2 = getval("gzlvl2"),
   gt3 = getval("gt3"),                gzlvl3 = getval("gzlvl3"),
   gt4 = getval("gt4"),                gzlvl4 = getval("gzlvl4"),
   gt5 = getval("gt5"),                gzlvl5 = getval("gzlvl5"),
   gt6 = getval("gt6"),                gzlvl6 = getval("gzlvl6"),
   gt7 = getval("gt7"),                gzlvl7 = getval("gzlvl7"),
   gt8 = getval("gt8"),                gzlvl8 = getval("gzlvl8"),
   gt9 = getval("gt9"),                gzlvl9 = getval("gzlvl9");

   getstr("f1180",f1180);   getstr("f2180",f2180);
   getstr("CaInvert", CaInvert);

/*   LOAD PHASE TABLE    */
     
   settable(t3,2,phi3);     settable(t4,1,phx);      settable(t5,4,phi5);
   settable(t8,8,phi8);     settable(t9,16,phi9);    settable(t10,1,phx);
   settable(t12,8,rec);

/*   INITIALIZE VARIABLES   */
   kappa = 5.4e-3;     lambda1 = 2.4e-3;  lambda2=2.75e-3;
   scale_CaCO=1.0;   /* Set scale_CaCO =1.0 for full sensitivity   */
   pwC=pwC*1.0;

/* selective H20 one-lobe sinc pulse */

/* get calculated pulse lengths of shaped C13 pulses */
   pwCO90 = c13pulsepw("co", "cab", "sinc", 90.0); 
   pwCa90 = c13pulsepw("ca", "co", "square", 90.0); 
   pwCO180 = c13pulsepw("co", "cab", "sinc", 180.0); 

/* CHECK VALIDITY OF PARAMETER RANGES */

   if ( 0.25*(ni-1)/sw1 > (tauCaCb - tauCaCO*scale_CaCO - WFG_START_DELAY - 4.0e-6
            -pwCO180/2.0 - 2.0*POWER_DELAY - 2.0*PWRF_DELAY - pw_CaInvert/2.0))
   {
      printf ("ni is too big. Set ni equal to %d or smaller.\n",
      ((int) ((tauCaCb - tauCaCO*scale_CaCO - WFG_START_DELAY - 4.0e-6
         -pwCO180/2.0 - 2.0*POWER_DELAY - 2.0*PWRF_DELAY - pw_CaInvert/2.0)*sw1*4.0 + 1.0 ) )); 
}

   if ( 0.5*(ni2-1)/(sw2) > timeTN - WFG3_START_DELAY -4.0e-6 - POWER_DELAY - PWRF_DELAY)
      { printf(" ni2 is too big. Make ni2 equal to %d or less.\n", 
        ((int)((timeTN - WFG3_START_DELAY -4.0e-6 - POWER_DELAY - PWRF_DELAY)*2.0*sw2 + 1.0))); 
        psg_abort(1);
      }

   if ( dm[A] == 'y' || dm[B] == 'y' || dm[C] == 'y' )
      { printf("incorrect dec1 decoupler flags! Should be 'nnn' "); psg_abort(1);}

   if ( dm2[A] == 'y' || dm2[B] == 'y' || dm2[C] == 'y' )
      { printf("incorrect dec2 decoupler flags! Should be 'nnn' "); psg_abort(1);}

   if ( dm3[A] == 'y' || dm3[C] == 'y' )
      { printf("incorrect dec3 decoupler flags! Should be 'nyn' or 'nnn' ");
                                                psg_abort(1);}     
   if ( dpwr2 > 46 )
      { printf("dpwr2 too large! recheck value  ");               psg_abort(1);}

   if ( pw > 20.0e-6 )
      { printf(" pw too long ! recheck value ");                  psg_abort(1);} 
  
   if ( pwN > 100.0e-6 )
      { printf(" pwN too long! recheck value ");                  psg_abort(1);} 

   if ( (0.25/sw1 - pwN - 4.0e-6 - pwCO90*2.0/PI
           - WFG_START_DELAY - POWER_DELAY - PWRF_DELAY) < 0)
   {
      printf("sw1 too wide.  Please set sw1 to %5d or narrower.\n", 
      (int)( 0.25/(pwN + 4.0e-6 + pwCO90*2.0/PI + WFG_START_DELAY + POWER_DELAY + PWRF_DELAY)));
      psg_abort(1);
   };
 
/* PHASES AND INCREMENTED TIMES */

/*  Phase incrementation for hypercomplex 2D data, States-Haberkorn element */

   if (phase1 == 2)   tsadd(t3,1,4);  
   if (phase2 == 2)  {tsadd(t10,2,4); icosel = +1;}
            else                       icosel = -1;    
   
/*  Set up f1180  */
   
   tau1 = d2;

   if ((f1180[A] == 'y') ) 
   { tau1 += ( 1.0 / (2.0*sw1) ); if(tau1 < 0.2e-6) tau1 = 0.0; }
   else { printf ("f1180 should be set to 'y'.\n"); psg_abort(1); }

   tau1 = tau1/2.0;

/*  Set up f2180  */

   tau2 = d3;
   if ((f2180[A] == 'y') ) 
   { tau2 += ( 1.0 / (2.0*sw2) ); if(tau2 < 0.2e-6) tau2 = 0.0; }
   tau2 = tau2/2.0;

/* Calculate modifications to phases for States-TPPI acquisition          */

   if( ix == 1) d2_init = d2;
   t1_counter = (int) ( (d2-d2_init)*sw1 + 0.5 );
   if (t1_counter % 2)   { tsadd(t3,2,4); tsadd(t12,2,4); }

   if( ix == 1) d3_init = d3;
   t2_counter = (int) ( (d3-d3_init)*sw2 + 0.5 );
   if(t2_counter % 2)    { tsadd(t8,2,4); tsadd(t12,2,4); }

/* BEGIN PULSE SEQUENCE */

status(A);
      delay(d1);

      if ( dm3[B] == 'y' ) { lk_hold(); lk_sampling_off(); }

      rcvroff();
      obsoffset(tof);        obspower(tpwr);       obspwrf(4095.0);
      set_c13offset("ca");   decpower(pwClvl);     decpwrf(4095.0);
      dec2offset(dof2);      dec2power(pwNlvl);

     txphase(one);
     shiftedpulse("sinc", pwHs, 90.0, 0.0, one, 10.0e-6, 2.0e-6);

     txphase(zero);  decphase(zero); dec2phase(zero);

/*  xxxxxxxxxxxxxxx HN to N to Ca transfer xxxxxxxxxxxxxxxxxxxx */

   rgpulse(pw, zero, 2.0e-6, 2.0e-6);                   /* 1H pulse excitation */

      zgradpulse(gzlvl3, gt3);
      dec2phase(zero);
      delay(lambda1 - gt3 -4.0e-6);

   sim3pulse(2.0*pw, 0.0, 2.0*pwN, zero, zero, zero, 2.0e-6, 2.0e-6);

      delay(lambda1 - gt3 -gstab -4.0e-6);
      zgradpulse(gzlvl3, gt3);
      txphase(three);
      delay(gstab);

   rgpulse(pw, three, 2.0e-6, 2.0e-6);

      zgradpulse(gzlvl4, gt4);
      delay(2.0e-4);

   dec2rgpulse(pwN, zero, 2.0e-6, 2.0e-6);

      zgradpulse(gzlvl5, gt5);
      delay(timeTN - WFG3_START_DELAY - gt5 - 4.0e-6);

   sim3_c13pulse("", "ca", "co", "square", "", 0.0, 180.0, 2.0*pwN,
                             zero, zero, zero, 2.0e-6, 2.0e-6);
      dec2phase(one);
      delay(timeTN - gt5 - gstab -4.0e-6);

      zgradpulse(gzlvl5, gt5);
      delay(gstab);

   dec2rgpulse(pwN, one, 2.0e-6, 2.0e-6);
/*  xxxxxxxxxxxxxxxxxxxxxxxx END of N to CA TRANSFER xxxxxxxxxxxxxxxxxxxx */

      zgradpulse(gzlvl6, gt6);
      delay(gstab);

      set_c13offset("co");

      if ( dm3[B] == 'y' )     /* begins optional 2H decoupling */
      {
         dec3rgpulse(1/dmf3,one,10.0e-6,2.0e-6);
         dec3unblank();
         dec3phase(zero);
         setstatus(DEC3ch, TRUE, 'w', FALSE, dmf3);
      }

/* =================  Ca to CO transfer ==================== */

   c13pulse("ca", "co", "square", 90.0, t5, 2.0e-6, 2.0e-6);

      delay(tauCaCO*scale_CaCO + tau1/2.0 - 4.0e-6 - pwCO180/2.0 - WFG_START_DELAY 
            - 2.0*POWER_DELAY - 2.0*PWRF_DELAY - 2.0/PI*pwCa90);

   c13pulse("co", "ca", "sinc", 180.0, zero, 2.0e-6, 2.0e-6);
    
      decpower(pwlvl_CaInvert);
      delay(tauCaCb - tau1/2.0 - tauCaCO*scale_CaCO - WFG_START_DELAY - 4.0e-6
             - 2.0*POWER_DELAY - PWRF_DELAY - pw_CaInvert/2.0 - pwCO180/2.0);
     
   decshaped_pulse(CaInvert, pw_CaInvert, zero, 2.0e-6, 2.0e-6);
      decpower(pwClvl);
    
      delay(tauCaCb - tau1 - 4.0e-6 - (1.0-2.0/PI)*pwCO90 - WFG_START_DELAY
                    - 2.0*POWER_DELAY - PWRF_DELAY - pw_CaInvert/2.0);

   c13pulse("co", "ca", "sinc", 90.0, t3, 2.0e-6, 2.0e-6);

      delay(tau1 - pwN - 4.0e-6 - pwCO90*2.0/PI - POWER_DELAY - PWRF_DELAY);

   dec2rgpulse(2.0*pwN, zero, 2.0e-6, 2.0e-6);

      delay(tau1 - pwN - 4.0e-6 - pwCO90*2.0/PI
           - WFG_START_DELAY - POWER_DELAY - PWRF_DELAY);

   c13pulse("co", "ca", "sinc", 90.0, two, 2.0e-6, 2.0e-6);
   
      decpower(pwlvl_CaInvert);
      delay(tauCaCb - tau1 - 4.0e-6 - (1.0-2.0/PI)*pwCO90 - WFG_START_DELAY
             - 2.0*POWER_DELAY - PWRF_DELAY - pw_CaInvert/2.0);

   decshaped_pulse(CaInvert, pw_CaInvert, zero, 2.0e-6, 2.0e-6);
      decpower(pwClvl);

      delay(tauCaCb - tau1/2.0 - tauCaCO*scale_CaCO - WFG_START_DELAY - 4.0e-6
            - 2.0*POWER_DELAY - PWRF_DELAY - pw_CaInvert/2.0 - pwCO180/2.0);

   c13pulse("co", "ca", "sinc", 180.0, zero, 2.0e-6, 2.0e-6);

      delay(tauCaCO*scale_CaCO +tau1/2.0- WFG_START_DELAY - 4.0e-6
            -pwCO180/2.0 - 2.0*POWER_DELAY - 2.0*PWRF_DELAY
            -SAPS_DELAY - 2.0/PI*pwCa90);

      initval(phshift, v4);
      decstepsize(1.0);
      dcplrphase(v4);
   c13pulse("ca", "co", "square", 90.0, zero, 2.0e-6, 2.0e-6);
      dcplrphase(zero); 
/* ============== End of CO to Ca transfer ========== */

      if ( dm3[B] == 'y' )   /* turns off 2H decoupling  */
      {
         setstatus(DEC3ch, FALSE, 'c', FALSE, dmf3);
         dec3rgpulse(1/dmf3,three,2.0e-6,2.0e-6);
         dec3blank();
         lk_autotrig();   /* resumes lock pulsing */
      }

      set_c13offset("ca");

      zgradpulse(gzlvl7, gt7);
      dec2phase(t8);
      delay(gstab);

/*  xxxxxxxxxxxxxxxxxxxx  N15 EVOLUTION & SE TRAIN   xxxxxxxxxxxxxxxxxxxxxxx  */     
   dec2rgpulse(pwN, t8, 2.0e-6, 2.0e-6);

      decphase(zero);     dec2phase(t9);
      delay(timeTN - tau2 - WFG3_START_DELAY -POWER_DELAY - PWRF_DELAY -4.0e-6);

   sim3_c13pulse("", "ca", "co", "square", "", 0.0, 180.0, 2.0*pwN, 
                              zero, zero, t9, 2.0e-6, 2.0e-6);

      delay (timeTN - pwCO180 - WFG_START_DELAY - 3.0*POWER_DELAY
             - 3.0*PWRF_DELAY - gt1 - 2.0*GRADIENT_DELAY - gstab -8.0e-6);

      zgradpulse(gzlvl1, gt1);                     /* 2.0*GRADIENT_DELAY */
      dec2phase(t10);
      delay(gstab);

      c13pulse("co", "ca", "sinc", 180.0, zero, 2.0e-6, 2.0e-6);          /*pwCO180*/
      delay(tau2);

   sim3pulse(pw, 0.0, pwN, zero, zero, t10, 2.0e-6, 2.0e-6);

      txphase(two);
      shiftedpulse("sinc", pwHs, 90.0, 0.0, two, 2.0e-6, 2.0e-6);

      zgradpulse(gzlvl8, gt8);
      txphase(zero);     dec2phase(zero);
      delay(lambda2 - 1.3*pwN - gt8 -8.0e-6 -pwHs
             -WFG_START_DELAY -2.0*POWER_DELAY -2.0*PWRF_DELAY);

   sim3pulse(2.0*pw, 0.0, 2.0*pwN, zero, zero, zero, 2.0e-6, 2.0e-6);

      delay(lambda2 - 1.3*pwN - gt8 -gstab -4.0e-6);
      zgradpulse(gzlvl8, gt8);
      txphase(one);     dec2phase(zero);
      delay(gstab);

   sim3pulse(pw, 0.0, pwN, one, zero, zero, 2.0e-6, 2.0e-6);

      zgradpulse(gzlvl9, gt9);
      txphase(zero);     dec2phase(zero);
      delay(lambda2 - 1.3*pwN - gt9 - 4.0e-6);

   sim3pulse(2.0*pw, 0.0, 2.0*pwN, zero, zero, zero, 2.0e-6, 2.0e-6);

      delay (lambda2 - 1.3*pwN - gt9 -gstab -4.0e-6);
      zgradpulse(gzlvl9, gt9);                                            /* G8 */
      txphase(three);   dec2phase(one);
      delay(gstab);

   sim3pulse(pw, 0.0, pwN, three, zero, one, 2.0e-6, 2.0e-6);

      delay ( (gt1/10.0) + gstab -2.0e-6 - 0.65*pw + 2.0*GRADIENT_DELAY + POWER_DELAY);

   rgpulse(2.0*pw, zero, 2.0e-6, 2.0e-6);

      dec2power(dpwr2);
      zgradpulse(icosel*gzlvl2, gt1/10.0);                       /* 2.0*GRADIENT_DELAY */
      delay(gstab);
      rcvron();

status(C);
   setreceiver(t12);
   if (dm3[B] == 'y') lk_sample();
}

