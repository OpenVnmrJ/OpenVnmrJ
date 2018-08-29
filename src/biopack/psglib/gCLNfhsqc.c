/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* gCLNfhsqc.c    "Fast N15 CLEANEX-PM hsqc using 3919 watergate suppression"

   minimum step cycling is 2
   program written by Susumu Mori and Chitrananda Abeygunawardana,Johns Hopkins
   modified for 3rd Channel operation BKJ 970430 
   TROSY option is included. When TROSY is selected use nt=8 (NM, Palo Alto)
   CLEANEX-PM  included by Igor Goljer  09-08-2002
   mix   : mixing time for exchange
   pw180 : 180 degree pulse for CLEANEX-PM mixing time should be 85 us
   gztm  : gradient level used for mixing to suppress radiation dumping
   only the above three variables need to be created to do CLEANEX-PM
   H2Osinc pulse used for selective excitation of water
   
   Modified CLEANEX-FHSQC sequence is written by T.-L. Hwang, based on the original
FHSQC sequence by S. Mori, etc.

reference:

"Accurate quantitation of water-amide proton exchange rates using the
Phase-Modulated CLEAN chemical EXchange (CLEANEX-PM) approach
with a Fast-HSQC (FHSQC) detection scheme"
T.-L. Hwang, P. C. M. van Zijl, and S. Mori,
J. Biomol. NMR, v11, p221-226 (1998)


"Application of Phase-Modulated CLEAN chemical EXchange Spectroscopy
(CLEANEX-PM) to detect water-protein proton exchange and intermolecular NOEs"
T.-L. Hwang, S. Mori, A. J. Shaka, and P. C. M. van Zijl
J. Am. Chem. Soc. v114, p3157-3158 (1997)


"Multiple-pulse mixing sequences that selectively enhance chemical exchange
or cross relaxation peaks in high-resolution NMR spectra"
T.-L. Hwang and A. J. Shaka
J. Magn. Reson.  v.135, p280-287 (1998)


   the TROSY phase cycle corrected to pick the correct component and to use the 
   sensitivity enhanced version. The minimum phase cycle for TROSY reduced to 4 steps.
   Corrected the d2 timing in the TROSY version. Added soft watergate option via
   wtg3919 flag.   
   
   Use f1coef = '1 0 -1 0 0 1 0 1' for TROSY
   E.K., 17.05.2002.
*/


#include <standard.h>

static double d2_init = 0.0;

static int phi1[4] = {1,1,3,3},
           phi2[2] = {0,2},
           phi3[8] = {0,0,0,0,2,2,2,2}, 
           phi4[16] = {0,0,0,0,0,0,0,0,2,2,2,2,2,2,2,2},
           rec[8]  = {2,0,0,2,0,2,2,0},
                                
           phT1[1] = {1},                           /* phase cycling for TROSY */
           phT2[4] = {1,3,0,2},                     
           phT4[1] = {3},                     
           recT[4] = {1,3,2,0};                              /* minimum nt = 4 */

pulsesequence()
{
  int       phase, t1_counter;

  char	    C13refoc[MAXSTR],		/* C13 sech/tanh pulse in middle of t1 */
            TROSY[MAXSTR],
            wtg3919[MAXSTR];
  double    tauxh, tau1,  gt2, gt1, 
            gztm, mix,  pw180, pw135, pw120, pw110, p1lvl, 
            gzlvl1,  cycles,  
            pwNt = 0.0,               /* pulse only active in the TROSY option */
            gsign = 1.0,
            gzlvl3=getval("gzlvl3"),
            gt3=getval("gt3"),
            JNH = getval("JNH"),
            pwN = getval("pwN"),
            pwNlvl = getval("pwNlvl"),  
            pwHs, tpwrs=0.0,           /* H1 90 degree pulse length at tpwrs */               
            compH = getval("compH"),
            sw1 = getval("sw1"),
            pwClvl = getval("pwClvl"), 	         /* coarse power for C13 pulse */
            pwC = getval("pwC"),       /* C13 90 degree pulse length at pwClvl */
            rfst = 4095.0,	            /* fine power for the stCall pulse */
            compC = getval("compC");   /* adjustment for C13 amplifier compr-n */

   gztm=getval("gztm");
   gt2=getval("gt2");
   gt1= getval("gt1");
   mix=getval("mix");
   phase = (int) (getval("phase") + 0.5);
   sw1 = getval("sw1");
   pw180 = getval("pw180");
   gzlvl1 = getval("gzlvl1");



/* INITIALIZE VARIABLES */

        pw135 = pw180 / 180.0 * 135.0 ;
        pw120 = pw180 / 180.0 * 120.0 ;
        pw110 = pw180 / 180.0 * 110.0 ;

        p1lvl = tpwr -20*log10(pw180/(compH*2.0*pw));
        p1lvl = (int)(p1lvl + 0.5);
         cycles = mix / (730.0/180.0 * pw180) - 8.0;

         initval(cycles, v10);        /* mixing time cycles */


    getstr("C13refoc",C13refoc);
    getstr("TROSY",TROSY);
    getstr("wtg3919",wtg3919);
    
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

                                    /* selective H20 one-lobe sinc pulse needs 1.69  */
     pwHs = getval("pwHs");            /* times more power than a square pulse */
     if (pwHs > 1e-6) tpwrs = tpwr - 20.0*log10(pwHs/(compH*pw*1.69));  
     else tpwrs = 0.0;
     tpwrs = (int) (tpwrs);
    

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
    
    tau1 = d2/2.0;

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
      settable(t4, 16, phi4);
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
     delay(d1);
     
   status(B);

/* slective excitation of water */

     rgpulse(pw, zero, rof1, rof1);
     

     zgradpulse(gzlvl1,gt1);

     obspower(tpwrs+6);   /*make it a 180 degree inversion pulse*/
     shaped_pulse("H2Osinc", pwHs, zero, rof1, 0.0);
     obspower(tpwr);

     zgradpulse(gzlvl1,gt1);

/*  CLEANEX-PM spin-lock   */

      if (cycles > 1.5000)
      {

       obspower(p1lvl);
       txphase(zero);

            rgpulse(pw135, zero, 0.0, 0.0);
            rgpulse(pw120, two,  0.0, 0.0);
            rgpulse(pw110, zero, 0.0, 0.0);
            rgpulse(pw110, two,  0.0, 0.0);
            rgpulse(pw120, zero, 0.0, 0.0);
            rgpulse(pw135, two,  0.0, 0.0);

       rgradient('z',gztm/4.0);

            rgpulse(pw135, zero, 0.0, 0.0);
            rgpulse(pw120, two,  0.0, 0.0);
            rgpulse(pw110, zero, 0.0, 0.0);
            rgpulse(pw110, two,  0.0, 0.0);
            rgpulse(pw120, zero, 0.0, 0.0);
            rgpulse(pw135, two,  0.0, 0.0);

       rgradient('z',gztm/2.0);

            rgpulse(pw135, zero, 0.0, 0.0);
            rgpulse(pw120, two,  0.0, 0.0);
            rgpulse(pw110, zero, 0.0, 0.0);
            rgpulse(pw110, two,  0.0, 0.0);
            rgpulse(pw120, zero, 0.0, 0.0);
            rgpulse(pw135, two,  0.0, 0.0);


            rgpulse(pw135, zero, 0.0, 0.0);
            rgpulse(pw120, two,  0.0, 0.0);
            rgpulse(pw110, zero, 0.0, 0.0);
            rgpulse(pw110, two,  0.0, 0.0);
            rgpulse(pw120, zero, 0.0, 0.0);
            rgpulse(pw135, two,  0.0, 0.0);

       rgradient('z',gztm/4.0*3.0);

            rgpulse(pw135, zero, 0.0, 0.0);
            rgpulse(pw120, two,  0.0, 0.0);
            rgpulse(pw110, zero, 0.0, 0.0);
            rgpulse(pw110, two,  0.0, 0.0);
            rgpulse(pw120, zero, 0.0, 0.0);
            rgpulse(pw135, two,  0.0, 0.0);

       rgradient('z',gztm);

         starthardloop(v10);
            rgpulse(pw135, zero, 0.0, 0.0);
            rgpulse(pw120, two,  0.0, 0.0);
            rgpulse(pw110, zero, 0.0, 0.0);
            rgpulse(pw110, two,  0.0, 0.0);
            rgpulse(pw120, zero, 0.0, 0.0);
            rgpulse(pw135, two,  0.0, 0.0);
         endhardloop();

       rgradient('z',gztm/4.0*3.0);

            rgpulse(pw135, zero, 0.0, 0.0);
            rgpulse(pw120, two,  0.0, 0.0);
            rgpulse(pw110, zero, 0.0, 0.0);
            rgpulse(pw110, two,  0.0, 0.0);
            rgpulse(pw120, zero, 0.0, 0.0);
            rgpulse(pw135, two,  0.0, 0.0);

       rgradient('z',gztm/2.0);

            rgpulse(pw135, zero, 0.0, 0.0);
            rgpulse(pw120, two,  0.0, 0.0);
            rgpulse(pw110, zero, 0.0, 0.0);
            rgpulse(pw110, two,  0.0, 0.0);
            rgpulse(pw120, zero, 0.0, 0.0);
            rgpulse(pw135, two,  0.0, 0.0);

       rgradient('z',gztm/4.0);

            rgpulse(pw135, zero, 0.0, 0.0);
            rgpulse(pw120, two,  0.0, 0.0);
            rgpulse(pw110, zero, 0.0, 0.0);
            rgpulse(pw110, two,  0.0, 0.0);
            rgpulse(pw120, zero, 0.0, 0.0);
            rgpulse(pw135, two,  0.0, 0.0);

       rgradient('z', 0.0);

            rgpulse(pw135, zero, 0.0, 0.0);
            rgpulse(pw120, two,  0.0, 0.0);
            rgpulse(pw110, zero, 0.0, 0.0);
            rgpulse(pw110, two,  0.0, 0.0);
            rgpulse(pw120, zero, 0.0, 0.0);
            rgpulse(pw135, two,  0.0, 0.0);


       obspower(tpwr);

      }


/* .......................................  */

     zgradpulse(0.3*gzlvl3,gt3);
     txphase(zero);
     dec2phase(zero);
     delay(tauxh-gt3);               /* delay=1/4J(XH)   */

     sim3pulse(2*pw,0.0,2*pwN,t4,zero,zero,rof1,rof1);

     zgradpulse(0.3*gzlvl3,gt3);
     dec2phase(t2);
     delay(tauxh-gt3 );               /* delay=1/4J(XH)   */
  
     rgpulse(pw, t1, rof1, rof1);

     zgradpulse(0.5*gsign*gzlvl3,gt3);
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
       zgradpulse(0.3*gzlvl3,gt3);
       delay(tauxh-gt3 );
       
       sim3pulse(2*pw,0.0,2*pwN,zero,zero,zero,rof1,rof1);
       
       zgradpulse(0.3*gzlvl3,gt3);
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
	 tau1 -= pw;
	 if (tau1 < 0.0) tau1 = 0.0;
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
     delay(tauxh-gt3-pwHs-rof1);
     
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
     delay(tauxh-gt3-pwHs-rof1-pwNt-POWER_DELAY); 
     dec2rgpulse(pwNt, zero, rof1, rof1); 
     dec2power(dpwr2);

   status(C);
     setreceiver(t5);   
}



