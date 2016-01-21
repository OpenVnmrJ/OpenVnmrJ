/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* Nfhsqc.c    "Fast N15 hsqc using 3919 watergate suppression"
               "with options for Saturation Transfer Difference"

   minimum step cycling is 2
   program written by Susumu Mori and Chitrananda Abeygunawardana,Johns Hopkins
   modified for 3rd Channel operation BKJ 970430 
   TROSY option is included. When TROSY is selected use nt=8 (NM, Palo Alto)
   
   Modified by Eriks Kupce :
   the TROSY phase cycle corrected to pick the correct component and to use the 
   sensitivity enhanced version. The minimum phase cycle for TROSY reduced to 4 steps.
   Corrected the d2 timing in the TROSY version. Added soft watergate option via
   wtg3919 flag.   
   
   Use f1coef = '1 0 -1 0 0 1 0 1' for TROSY
   E.K., 17.05.2002.
   
   tau1 timing corrected for regular experiment (4*pwN/PI correction added)
   tau1 timing corrected for TROSY experiment
   f1180 flag added for starting t1 at half-dwell
   C13shape flag added for chosing betweem adiabatic or composite 13C refocussing pulse in t1 
   (Marco Tonelli and Klaas Hallenga, NMRFAM, Univ. Wisconsin).

   STD options added by Mike Osbourne, U.Montreal.

    Options are 1. No STD
                2. Saturation using shaped decoupling
                3. Saturation using a shaped-pulse train
                4. Saturation using Pbox-enerated shaped pulse train.
                               
*/


#include <standard.h>
#include "bionmr.h"

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
  int       t1_counter;
  char	    C13refoc[MAXSTR],	      /* C13 refocussing pulse in middle of t1 */
	    C13shape[MAXSTR],   /* choose between sech/tanh or composite pulse */
            TROSY[MAXSTR],
            wtg3919[MAXSTR],
            NHsat[MAXSTR],               /* flag for saturation of NH band    */
	    NHsatauto[MAXSTR],               /* flag for autosaturation of NH band    */
      	    WUsat[MAXSTR],		/*flag for wurst decoupling saturation*/
       	WURSTSAT[MAXSTR],
      	NHsatshape[MAXSTR],
	    f1180[MAXSTR];   		       /* Flag to start t1 @ halfdwell */
  double    tauxh, tau1, ncyc,
            pwNt = 0.0,               /* pulse only active in the TROSY option */
            gsign = 1.0,
            gzlvl3=getval("gzlvl3"),
            gt3=getval("gt3"),
	    gstab=getval("gstab"),	      /* recovery delay after gradient */
            JNH = getval("JNH"),
            pwN = getval("pwN"),
            pwNlvl = getval("pwNlvl"),  
            pwHs, tpwrs=0.0, compH,      /* H1 90 degree pulse length at tpwrs */               
            sw1 = getval("sw1"),
            pwClvl = getval("pwClvl"), 	         /* coarse power for C13 pulse */
            pwC = getval("pwC"),       /* C13 90 degree pulse length at pwClvl */
            rfst = 4095.0,	            /* fine power for the stCall pulse */
       WUsatpwr= getval("WUsatpwr"),
       NHsatpwr= getval("NHsatpwr"),
        WUtof=getval("WUtof"),
        WUdmf=getval("WUdmf"),
       /* WURST90=getval("WURST90"), */
	WURSTRES=getval("WURSTRES"),
	WURSTTIME=getval("WURSTTIME"),
	

        pwNHsel= getval("pwNHsel"),
        NHoffset=getval("NHoffset"),
        NHsattime=getval("NHsattime"),
        satdelay=getval("satdelay"),   /* time between stauration pulses */
	
	
            compC = getval("compC");   /* adjustment for C13 amplifier compr-n */


/* INITIALIZE VARIABLES */

    getstr("C13refoc",C13refoc);
    getstr("C13shape",C13shape);
    getstr("TROSY",TROSY);
    getstr("wtg3919",wtg3919);
    getstr("f1180",f1180);
    getstr("NHsat", NHsat);
    getstr("NHsatauto", NHsatauto);
    getstr("WUsat", WUsat);
    getstr("WURSTSAT", WURSTSAT);
    getstr("NHsatshape", NHsatshape);

    
    tauxh = ((JNH != 0.0) ? 1/(4*(JNH)) : 2.25e-3);

    if ( (C13refoc[A]=='y') && (C13shape[A]=='y') )
    {
      /* 180 degree adiabatic C13 pulse from 0 to 200 ppm */
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

/* check validity of parameter range */

    if((dm[A] == 'y' || dm[B] == 'y' || dm[C] == 'y' ))
    { text_error("incorrect Dec1 decoupler flags!  "); psg_abort(1); } 

    if((dm2[A] == 'y' || dm2[B] == 'y') )
    { text_error("incorrect Dec2 decoupler flags!  "); psg_abort(1); } 

    if( dpwr > 0 )
    { text_error("don't fry the probe, dpwr too large!  "); psg_abort(1); }

    if( dpwr2 > 50 )
    { text_error("don't fry the probe, dpwr2 too large!  "); psg_abort(1); }

    if ((NHsat[A]=='y') && (NHsatauto[A] == 'y'))
    { text_error("Both NHsat and NHsatauto can not be == 'y'   ! "); psg_abort(1); }

    if ((NHsat[A]=='y') && (WUsat[A] == 'y'))
    { text_error("Both NHsat and WUsat can not be == 'y'   ! "); psg_abort(1); }

    if ((NHsatauto[A]=='y') && (WUsat[A] == 'y'))
    { text_error("Both NHsatauto and WUsat can not be == 'y'   ! "); psg_abort(1); }

    if ((TROSY[A]=='y') && (dm2[C] == 'y'))
    { text_error("Choose either TROSY='n' or dm2='n' ! "); psg_abort(1); }

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
     if (C13shape[A]=='y') 
	decpwrf(rfst);
     else
	decpwrf(4095.0);
     delay(d1);
 

     if ((NHsatauto[A] == 'y') && (NHsat[A] == 'n') && (WUsat[A] == 'n'))
   {
    ncyc=NHsattime/(pwNHsel+0.001);
    initval(ncyc,v1);
    starthardloop(v1);
      delay(0.0005);
      shiftedpulse("eburp2", pwNHsel, 90.0, NHoffset, two, 2.0e-6, 0.0);
      delay(0.0005);
      endhardloop();
    }


     if ((NHsat[A] == 'y') && (NHsatauto[A] == 'n') && (WUsat[A] == 'n'))
   {
   obspower(NHsatpwr);
   obsoffset(NHoffset);
    ncyc=NHsattime/(pwNHsel+satdelay+2.0e-6);
    
   
    initval(ncyc,v1);
    starthardloop(v1);
	delay(satdelay/2);
      shaped_pulse(NHsatshape, pwNHsel, two, 2.0e-6, 0.0);
/*      shaped_pulse(NHsatshape, pwNHsel, zero, 2.0e-6, 0.0); */
      delay(satdelay/2);
      endhardloop();
	
    }


    if ((WUsat[A] == 'y') && (NHsat[A] == 'n') && (NHsatauto[A] == 'n'))
   {
	rcvroff();
   obspower(WUsatpwr);
   obsoffset(WUtof);
   delay(10e-06);
   /* obsprgon(WURSTSAT,WURST90,WURSTRES); */
   obsprgon(WURSTSAT,1/WUdmf,WURSTRES);
   xmtron();
   delay(WURSTTIME);
   xmtroff();
   obsprgoff();
}


     obspower(tpwr);
     obsoffset(tof);
     delay(0.000001);

   
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

     zgradpulse(0.5*gsign*gzlvl3,gt3);
     delay(gstab); 
     decphase(zero);
            
     if (TROSY[A] == 'y')
     { 
       txphase(t3);       
       if ( phase1 == 2 ) 
         dec2rgpulse(pwN, t6, rof1, rof1);
       else 
         dec2rgpulse(pwN, t2, rof1, rof1);              

       if ( (C13shape[A]=='y') && (C13refoc[A]=='y') 
	   && (tau1 > 0.5e-3 +WFG2_START_DELAY +rof1) )
         {
         delay(tau1 -0.5e-3 -WFG2_START_DELAY -rof1);     
         decshaped_pulse("stC200", 1.0e-3, zero, 0.0, 0.0);
         delay(tau1 -0.5e-3 -WFG2_STOP_DELAY -rof1);
         }
       else if ( (C13shape[A]!='y') && (C13refoc[A]=='y') 
	        && (tau1 > 2.0*pwC +SAPS_DELAY +2.0*rof1) )
         {
         delay(tau1 -2.0*pwC -SAPS_DELAY -2.0*rof1); 
	 decrgpulse(pwC, zero, rof1, 0.0);
	 decphase(one);
         decrgpulse(2.0*pwC, one, 0.0, 0.0);  
	 decphase(zero);
	 decrgpulse(pwC, zero, 0.0, rof1);
         delay(tau1 -2.0*pwC -SAPS_DELAY -2.0*rof1); 
         }
       else if (tau1 > rof1)
         delay(2.0*tau1 -2.0*rof1);

       rgpulse(pw, t3, rof1, rof1);         
       zgradpulse(0.3*gzlvl3,gt3);
       delay(tauxh -gt3);
       
       sim3pulse(2*pw,0.0,2*pwN,zero,zero,zero,rof1,rof1);
       
       zgradpulse(0.3*gzlvl3,gt3);
       delay(tauxh -gt3);       
       sim3pulse(pw,0.0,pwN,zero,zero,t3,rof1,rof1);
     }
     else
     {         
       txphase(t4);      
       dec2rgpulse(pwN, t2, rof1, rof1);
       dec2phase(t3);

       if ( (C13shape[A]=='y') && (C13refoc[A]=='y') 
           && (tau1 > 0.5e-3 +WFG2_START_DELAY +2.0*pwN/PI +SAPS_DELAY +rof1) )
         {
         delay(tau1 -0.5e-3 -WFG2_START_DELAY -2.0*pwN/PI -SAPS_DELAY -rof1); 
         simshaped_pulse("", "stC200", 2.0*pw, 1.0e-3, t4, zero, 0.0, 0.0);  
         delay(tau1 -0.5e-3 -WFG2_STOP_DELAY -2.0*pwN/PI -SAPS_DELAY -rof1);
         }
       else if ( (C13shape[A]!='y') && (C13refoc[A]=='y') 
                && (tau1 > 2.0*pwN/PI +2.0*pwC +2.0*SAPS_DELAY +2.0*rof1) )
         {
         delay(tau1 -2.0*pwN/PI -2.0*pwC -2.0*SAPS_DELAY -2.0*rof1); 
	 decrgpulse(pwC, zero, rof1, 0.0);
	 decphase(one);
         simpulse(2.0*pw, 2.0*pwC, t4, one, 0.0, 0.0);  
	 decphase(zero);
	 decrgpulse(pwC, zero, 0.0, rof1);
         delay(tau1 -2.0*pwN/PI -2.0*pwC -SAPS_DELAY -2.0*rof1); 
         }
       else if (tau1 > pw +2.0*pwN/PI +SAPS_DELAY +2.0*rof1) 
         {
         delay(tau1 -pw -2.0*pwN/PI -SAPS_DELAY -2.0*rof1);
         rgpulse(2.0*pw, t4, rof1, rof1);
         delay(tau1 -2.0*pwN/PI -pw -2.0*rof1);
         }
       else
         rgpulse(2.0*pw, t4, rof1, rof1);
       
       dec2rgpulse(pwN, t3, rof1, rof1);
       
       zgradpulse(0.5*gzlvl3,gt3);
       delay(gstab);
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
     delay(tauxh-gt3-pwHs-pwNt-POWER_DELAY); 
     dec2rgpulse(pwNt, zero, rof1, rof1); 
     dec2power(dpwr2);
	rcvron();
   status(C);
     setreceiver(t5);   
}



