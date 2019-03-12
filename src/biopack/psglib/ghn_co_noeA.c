/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*  ghn_co_noeA.c

 DESCRIPTION and INSTRUCTION:

    This pulse sequence will allow one to perform the following experiment:
    3D CO-edited NOE-H(N)CO which correlates  HN-NOE with CO(i-1). 
    ni(t1)  ---> 1H
    ni2(t2) ---> 13CO

    REFERENCE:
    Weixing Zhang, Thomas E Smithgall and William H Gmeiner, JMR, B111, 305-309 (1996)	

    OFFSET POSITION:
        tof =   ~4.75 ppm (1H on water).
        dof =   174 ppm (13C on CO).
        dof2 =  120 ppm (15N region).

    CHOICE OF FLAGS:
    mag_flg = y --> Using magic angle gradients (Triax PFG probe is required).
    wgate == 'y'--> Using 3-9-19 watergate for water suppresion.
    wgate == 'n'--> Using gradient selection for water suppression.
    (for non-vnmr processing, gradsort should not be used since there is no 15N evolution.)
     
    flipback:  first character 'y' for primary flipback pulse
               second character 'y' for flipback within watergate 
 
    Modified the amplitude of the flipback pulse(s) (pwHs) to permit user adjustment
    around theoretical value (tpwrs). If tpwrsf < 4095.0 the value of tpwrs is increased 
    6db and values of tpwrsf of 2048 or so should give equivalent amplitude. In cases of 
    severe radiation damping( which happens during pwHs) the needed flip angle may be 
    much less than 90 degrees, so tpwrsf should be lowered to a value to minimize the 
    observed H2O signal in single-scan experiments (with ssfilter='n').(GG jan01)

    The autocal and checkofs flags are generated automatically in Pbox_bio.h
    If these flags do not exist in the parameter set, they are automatically 
    set to 'y' - yes. In order to change their default values, create the flag(s) 
    in your parameter set and change them as required. 
    The available options for the checkofs flag are: 'y' (yes) and 'n' (no). 
    The offset (tof, dof, dof2 and dof3) checks can be switched off individually 
    by setting the corresponding argument to zero (0.0).
    For the autocal flag the available options are: 'y' (yes - by default), 
    'q' (quiet mode - suppress Pbox output), 'r' (read from file, no new shapes 
    are created), 's' (semi-automatic mode - allows access to user defined 
    parameters) and 'n' (no - use full manual setup, equivalent to the original code).

   FOR DETAILS OF PARAMETERS AND SEQUENCE SEE  ghn_co.c and gnoesyNhsqc.c
     NOTE: TROSY is not implemented for this sequence.

    The waltz16 field strength is enterable (waltzB1).
    Typical values would be ~6-8ppm, (3500-5000 Hz at 600 MHz).
  
    Modified from ghn_co.c and gnoesyNhsqc.c by
    Written by Weixing Zhang, April 9, 2002
    St. Jude Children's Research Hospital
    Memphis, TN 38105
    USA
    (901)495-3169
    Modified on April 26, 2002 for submission to BioPack.
    Auto-calibrated version, E.Kupce, 27.08.2002.

*/

#include <standard.h>
#include "Pbox_bio.h"               /* Pbox Bio Pack Pulse Shaping Utilities */  

static int   
     phi1[4]  = {0,0,2,2},
     phi2[2]  = {0,2},
     phi3[8]  = {0,0,0,0,2,2,2,2},
     rec[8]   = {0,2,2,0,2,0,0,2};	    

static double   d2_init=0.0, d3_init=0.0;
static double   H1ofs=4.75, C13ofs=174.0, N15ofs=120.0, H2ofs=0.0;

static shape H2Osinc, wz16, offC3, offC6, offC8;

void pulsesequence()
{

   char     f1180[MAXSTR],   		     
            f2180[MAXSTR],
            wgate[MAXSTR], 
            flipback[MAXSTR],   		     
            mag_flg[MAXSTR];     
 	    
 
   int      icosel = -1,
            t1_counter,  		       
            t2_counter,  	 	       
	    ni2 = getval("ni2");

   double   tau1,         				        
            tau2,        				        
            timeTN = getval("timeTN"),    
	    kappa = 5.4e-3,
	    lambda = 2.4e-3,
            mix = getval("mix"),        /* NOE mixing period    */
            ratio=0.080,
            taud3 = getval("taud3"),

	    pwClvl = getval("pwClvl"), 	      
            pwC = getval("pwC"),  
	    rf0 = 4095,            	  
          bw, ofs, ppm,  /* bandwidth, offset, ppm - temporary Pbox parameters */
  
            compH = getval("compH"),      
            compC = getval("compC"),      
            pwHs = getval("pwHs"),	       
            tpwrsf = getval("tpwrsf"),      
            tpwrs,	  	             

            pwHd,	    		        
            tpwrd,	  	                   /* rf for WALTZ decoupling */

            waltzB1 = getval("waltzB1"),  /* waltz16 field strength (in Hz)     */
            pwNlvl = getval("pwNlvl"),	             
            pwN = getval("pwN"),         

	    sw1 = getval("sw1"),
	    sw2 = getval("sw2"),

	    gt1 = getval("gt1"),  		       /* coherence pathway gradients */
            gzcal  = getval("gzcal"),            /* g/cm to DAC conversion factor */
	    gzlvl1 = getval("gzlvl1"),
	    gzlvl2 = getval("gzlvl2"),

	    gt0 = getval("gt0"),				  
	    gt3 = getval("gt3"),
	    gt4 = getval("gt4"),
	    gt5 = getval("gt5"),
            gt7 = getval("gt7"),
            gt8 = getval("gt8"),
            gt9 = getval("gt9"),
            gstab = getval("gstab"),
	    gzlvl0 = getval("gzlvl0"),
	    gzlvl3 = getval("gzlvl3"),
	    gzlvl4 = getval("gzlvl4"),
	    gzlvl5 = getval("gzlvl5"),
            gzlvl7 = getval("gzlvl7"),
            gzlvl8 = getval("gzlvl8"),
            gzlvl9 = getval("gzlvl9"),

    pwC3a,                          /*180 degree pulse at Ca(56ppm) null at CO(174ppm) */
    phshift3 = 48.0,             /* phase shift induced on CO by pwC3a ("offC3") pulse */
    pwZ,					   /* the largest of pwC3a and 2.0*pwN */
    pwC6,                      /* 90 degree selective sinc pulse on CO(174ppm) */
    pwC8,                     /* 180 degree selective sinc pulse on CO(174ppm) */
    rf3,	                           /* fine power for the pwC3a ("offC3") pulse */
    rf6,	                           /* fine power for the pwC6 ("offC6") pulse */
    rf8;                           /* fine power for the pwC8 ("offC8") pulse */

    getstr("f1180",f1180);
    getstr("f2180",f2180);
    getstr("wgate", wgate);
    getstr("flipback", flipback);
    getstr("mag_flg",mag_flg);
   

/*   LOAD PHASE TABLE    */

	settable(t1,4,phi1);
	settable(t2,2,phi2);
	settable(t3,8,phi3);
       	settable(t12,8,rec);


/*   INITIALIZE VARIABLES   */

    if( dpwrf < 4095 )
    { 
       printf("reset dpwrf=4095 and recalibrate C13 90 degree pulse");
       psg_abort(1); 
    }

      setautocal();                      /* activate auto-calibration */   

      if (autocal[0] == 'n') 
      {
    /* offC3 - 180 degree pulse on Ca, null at CO 118ppm away */
        pwC3a = getval("pwC3a");    
        rf3 = (compC*4095.0*pwC*2.0)/pwC3a;
	  rf3 = (int) (rf3 + 0.5);  
	
    /* 90 degree one-lobe sinc pulse on CO, null at Ca 118ppm away */	
        pwC6 = getval("pwC6");    
	  rf6 = (compC*4095.0*pwC*1.69)/pwC6;	/* needs 1.69 times more     */
	  rf6 = (int) (rf6 + 0.5);		/* power than a square pulse */

    /* 180 degree one-lobe sinc pulse on CO, null at Ca 118ppm away */
        pwC8 = getval("pwC8");
	  rf8 = (compC*4095.0*pwC*2.0*1.65)/pwC8;	/* needs 1.65 times more     */
	  rf8 = (int) (rf8 + 0.5);		      /* power than a square pulse */

    /* selective H20 one-lobe sinc pulse */
        tpwrs = tpwr - 20.0*log10(pwHs/(compH*pw*1.69)); /* needs 1.69 times more */
        tpwrs = (int) (tpwrs);                       /* power than a square pulse */

    /* power level and pulse time for WALTZ 1H decoupling */
	  pwHd = 1/(4.0 * waltzB1) ;    
	  tpwrd = tpwr - 20.0*log10(pwHd/(compH*pw));
	  tpwrd = (int) (tpwrd + 0.5);
      }
      else      /* if autocal = 'y'(yes), 'q'(quiet), 'r'(read) or 's'(semi) */
      {
        if(FIRST_FID)                                         /* make shapes */
        {
          ppm = getval("dfrq"); 
          bw = 118.0*ppm; ofs = -bw; 
          offC3 = pbox_make("offC3", "square180n", bw, ofs, compC*pwC, pwClvl);
          offC6 = pbox_make("offC6", "sinc90n", bw, 0.0, compC*pwC, pwClvl);
          offC8 = pbox_make("offC8", "sinc180n", bw, 0.0, compC*pwC, pwClvl);
          H2Osinc = pbox_Rsh("H2Osinc", "sinc90", pwHs, 0.0, compH*pw, tpwr);
          bw = 2.8*7500.0;
          wz16 = pbox_Dcal("WALTZ16", bw, 0.0, compH*pw, tpwr);
          ofs_check(H1ofs, C13ofs, N15ofs, H2ofs);
        }
        pwC3a = offC3.pw; rf3 = offC3.pwrf;             /* set up parameters */
        pwC6 = offC6.pw; rf6 = offC6.pwrf; 
        pwC8 = offC8.pw; rf8 = offC8.pwrf;
        pwHs = H2Osinc.pw; tpwrs = H2Osinc.pwr-1.0;  /* 1dB correction applied */
        tpwrd = wz16.pwr; pwHd = 1.0/wz16.dmf;  
      }

      if (tpwrsf < 4095.0) tpwrs = tpwrs + 6.0;

    if (pwC3a > 2.0*pwN) pwZ = pwC3a; else pwZ = 2.0*pwN; 

/* CHECK VALIDITY OF PARAMETER RANGES */

    
    if ( dm[A] == 'y' || dm[B] == 'y' || dm[C] == 'y' )
    { 
        printf("incorrect dec1 decoupler flags! Should be 'nnn' "); 
        psg_abort(1);
    }

    if ( dm2[A] == 'y' || dm2[B] == 'y' )
    { 
        printf("incorrect dec2 decoupler flags! Should be 'nny' "); 
        psg_abort(1);
    }

    if ( dpwr2 > 50 )
    { 
        printf("dpwr2 too large! recheck value  ");		     
        psg_abort(1);
    }
    

/* PHASES AND INCREMENTED TIMES */

/*  Phase incrementation for hypercomplex 2D data, States-Haberkorn element */

    if (phase1 == 2)   
    {
        tsadd(t1,1,4);  
    }
    if (phase2 == 2)   	
    {
        tsadd(t2,1,4);
    }


/*  Set up f1180  */
   
    tau1 = d2 - 4*pw/PI;
    if((f1180[A] == 'y') && (ni > 1.0)) 
    { 
        tau1 += ( 1.0 / (2.0*sw1) ); 
    }
    if(tau1 < 0.2e-6) tau1 = 0.0; 
    tau1 = tau1/2.0;


/*  Set up f2180  */

    tau2 = d3 - pwZ - 4*pwC6/PI - 2*POWER_DELAY - WFG3_START_DELAY - WFG3_STOP_DELAY;
    if((f2180[A] == 'y') && (ni2 > 1.0)) 
    { 
        tau2 += ( 1.0 / (2.0*sw2) ); 
    }
    if(tau2 < 0.2e-6) tau2 = 0.0; 
    tau2 = tau2/2.0;


/* Calculate modifications to phases for States-TPPI acquisition          */

    if( ix == 1) d2_init = d2;
    t1_counter = (int) ( (d2-d2_init)*sw1 + 0.5 );
    if(t1_counter % 2) 
    { 
        tsadd(t1,2,4); 
        tsadd(t12,2,4); 
    }

    if( ix == 1) d3_init = d3;
    t2_counter = (int) ( (d3-d3_init)*sw2 + 0.5 );
    if(t2_counter % 2) 
	
    { 
        tsadd(t2,2,4); 
        tsadd(t12,2,4); 
    }

/* BEGIN PULSE SEQUENCE */

status(A);
    delay(d1);
    rcvroff();

    obspower(tpwr);
    decpower(pwClvl);
    decpwrf(rf0);
    obsoffset(tof);
    decoffset(dof-((174-35)*dfrq));  /* move 13C offset to 35ppm for decoupling */
    dec2power(pwNlvl);

    decphase(zero);
    dec2phase(zero);
    dec2rgpulse(pwN, zero, 0.0, 0.0);  
    decrgpulse(pwC, zero, 0.0, 0.0);
    zgradpulse(gzlvl0, gt0);
    dec2rgpulse(pwN, one, 0.0, 0.0);
    decrgpulse(pwC, one, 0.0, 0.0);
    zgradpulse(0.7*gzlvl0, gt0);

    txphase(t1);
    initval(135.0,v1);
    obsstepsize(1.0);
    xmtrphase(v1);

    delay(1.0e-3);

    rgpulse(pw, t1, 5.0e-6, 0.0);                    
    xmtrphase(zero);                                       
    txphase(zero);

    if (tau1 > (pwN + SAPS_DELAY))  
    {
        delay(tau1 - pwN - SAPS_DELAY);
        sim3pulse(0.0, 2.0*pwC, 2.0*pwN, zero, zero, zero, 0.0, 0.0);
        delay(tau1 - pwN);
    }
    else if (tau1 > (pwC + SAPS_DELAY))
    {
        delay(tau1 - pwC - SAPS_DELAY);
        decrgpulse(2.0*pwC, zero, 0.0, 0.0);
        delay(tau1 - pwC);
    }
    rgpulse(pw, zero, 1.0e-6, 0.0);

    delay(mix - gt4 - gt9 - 1.0e-3);
    zgradpulse(gzlvl4, gt4);
    delay(500.0e-6);
    dec2rgpulse(pwN, zero, 0.0, 0.0);
    zgradpulse(gzlvl9, gt9);
    delay(500.0e-6);

    rgpulse(pw, zero, 0.0,1.0e-6);                    

    zgradpulse(gzlvl0, gt0);
    delay(lambda - gt0);

    sim3pulse(2.0*pw, 0.0, 2.0*pwN, zero, zero, zero, 0.0, 1.0e-6);

    txphase(one);
    zgradpulse(gzlvl0, gt0);
    delay(lambda - gt0);

    rgpulse(pw, one, 0.0, 0.0);
    if(flipback[A] == 'y')
    {
       if (tpwrsf < 4095.0) obspwrf(tpwrsf); 
       obspower(tpwrs);
       txphase(zero);
       shaped_pulse("H2Osinc", pwHs, zero, 5.0e-6, 1.0e-6);
       if (tpwrsf < 4095.0) obspwrf(4095.0);
    }
    obspower(tpwr); 
    zgradpulse(gzlvl3, gt3);
    decphase(zero);
    dec2phase(zero);
    decoffset(dof);      /* move 13C offset back to C=O region */
    delay(2.0e-4);

    dec2rgpulse(pwN, zero, 0.0, 0.0);

    txphase(one);
    delay(kappa - pw - 2.0e-6 - PRG_START_DELAY - POWER_DELAY);
    rgpulse(pw,one,0.0,0.0);
    txphase(zero);
    obspower(tpwrd);
    delay(2.0e-6);
    obsprgon("waltz16", pwHd, 90.0);	          /* PRG_START_DELAY */
    xmtron();
    decpwrf(rf8);
    delay(timeTN - kappa);
   						   
    dec2rgpulse(2.0*pwN, zero, 0.0, 0.0);
    decshaped_pulse("offC8", pwC8, zero, 0.0, 0.0);
    delay(timeTN - pwC8 - WFG_START_DELAY - WFG_STOP_DELAY);
    dec2rgpulse(pwN, zero, 0.0, 1.0e-6);
    xmtroff();
    obsprgoff();
    obspower(tpwr);
    txphase(three);
    delay(2.0e-6);
    rgpulse(pw, three, 0.0, 2.0e-6);

    zgradpulse(gzlvl7, gt7);
    decphase(t2);
    decpwrf(rf6);
    delay(2.0e-4);
/* xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx*/
    decshaped_pulse("offC6", pwC6, t2, 0.0, 0.0);
    decphase(zero);	
    decpwrf(rf3);
    delay(tau2);
    sim3shaped_pulse("", "offC3", "",0.0, pwC3a, 2.0*pwN, zero, zero, zero, 0.0, 0.0);
    initval(phshift3, v3);
    decstepsize(1.0);
    dcplrphase(v3);  				        /* SAPS_DELAY */
    decphase(t3);
    decpwrf(rf6);
    delay(tau2);
    decshaped_pulse("offC6", pwC6, t3, 1.0e-6, 2.0e-6);

/*  xxxxxxxxxxxxxxxxxx    NO N15 EVOLUTION    xxxxxxxxxxxxxxxxxxxxx  */
    
    zgradpulse(gzlvl8, gt8);
    dcplrphase(zero);
    decphase(zero);
    delay(200.0e-6);
    rgpulse(pw,one,0.0,0.0);
    obspower(tpwrd);
    txphase(zero);
    delay(2.0e-6);
    obsprgon("waltz16", pwHd, 90.0);	          /* PRG_START_DELAY */
    xmtron();
    dec2rgpulse(pwN, zero, 0.0, 0.0);
    decpwrf(rf8);
    delay(timeTN - pwC8 - WFG_START_DELAY - WFG_STOP_DELAY);
    decshaped_pulse("offC8", pwC8, zero, 0.0, 0.0);
    dec2rgpulse(2.0*pwN, zero, 0.0, 0.0);
    delay(timeTN - kappa - pw - PRG_STOP_DELAY - 2.0e-6 - POWER_DELAY);
    xmtroff();
    obsprgoff();
    obspower(tpwr);
    txphase(three);
    delay(2.0e-6);	         
    rgpulse(pw,three,0.0,0.0);

    if (wgate[A] == 'y')
    {
        delay(kappa); 
        dec2rgpulse(pwN, zero, 0.0, 1.0e-6);
        zgradpulse(gzlvl3, gt3);
        delay(200.0e-6);
        if(flipback[B] == 'y')
        {
           if (tpwrsf < 4095.0) obspwrf(tpwrsf); 
           obspower(tpwrs);
           txphase(two);
           shaped_pulse("H2Osinc", pwHs, two, 2.0e-6, 0.0);
           if (tpwrsf < 4095.0) obspwrf(4095.0);
           obspower(tpwr);
        }

        rgpulse(pw, zero, 2.0e-6, 1.0e-6);
        if (mag_flg[A] == 'y')
        {
            magradpulse(gzcal*gzlvl5, gt5);
        }
        else
        {
            zgradpulse(gzlvl5, gt5);
        }
        
        delay(lambda  -2.5*taud3 - 31*pw*ratio - gt5);

        rgpulse(pw*ratio*3.0, zero, 0.0, 0.0);
        delay(taud3);
        rgpulse(pw*ratio*9.0, zero, 0.0, 0.0);
        delay(taud3);
        rgpulse(pw*ratio*19.0, zero, 0.0, 0.0);
        txphase(two);
        delay(taud3/2.0 - pwN);
        dec2rgpulse(2.0*pwN, zero, 0.0, 0.0);
        delay(taud3/2.0 - pwN);
        rgpulse(pw*ratio*19.0, two, 0.0, 0.0);
        delay(taud3);
        rgpulse(pw*ratio*9.0, two, 0.0, 0.0);
        delay(taud3);
        rgpulse(pw*ratio*3.0,two, 0.0, 0.0);

        if (mag_flg[A] == 'y')
        {
            magradpulse(gzcal*gzlvl5, gt5);
        }
        else
        {
            zgradpulse(gzlvl5, gt5);
        }
         delay(lambda - 2.5*taud3 - 31*pw*ratio - gt5);
    }
    else
    {       
        if (mag_flg[A]=='y')   
        {
           magradpulse(gzcal*gzlvl1, gt1);
        }
        else
        {
           zgradpulse(gzlvl1, gt1);  
           delay(4.0*GRADIENT_DELAY);
        }
        delay(kappa - gt1 - 6.0*GRADIENT_DELAY);     

        sim3pulse(pw, 0.0, pwN, zero, zero, zero, 0.0, 1.0e-6);

        zgradpulse(gzlvl5, gt5);
        delay(lambda - 1.3*pwN - gt5);

        sim3pulse(2.0*pw, 0.0, 2.0*pwN, zero, zero, zero, 0.0, 1.0e-6);

        zgradpulse(gzlvl5, gt5);
        txphase(one);
        dec2phase(one);
        delay(lambda - 1.3*pwN - gt5);

        sim3pulse(pw, 0.0, pwN, one, zero, one, 0.0, 1.0e-6);

        txphase(zero);
        dec2phase(zero);
        zgradpulse(0.7*gzlvl5, gt5);
        delay(lambda - 1.3*pwN - gt5);

        sim3pulse(2.0*pw, 0.0, 2.0*pwN, zero, zero, zero, 0.0, 1.0e-6);

        zgradpulse(0.7*gzlvl5, gt5);
        delay(lambda - 0.65*pwN - gt5);

        rgpulse(pw, zero, 0.0, 0.0); 

        delay((gt1/10.0) +gstab + 1.0e-4 - 0.5*pw + 2.0*GRADIENT_DELAY + POWER_DELAY);

        rgpulse(2.0*pw, zero, 0.0,1.0e-6);
        if (mag_flg[A] == 'y')    magradpulse(icosel*gzcal*gzlvl2, gt1/10.0);
        else   zgradpulse(icosel*gzlvl2, gt1/10.0);        
        delay(gstab);
    }
    dec2power(dpwr2);

statusdelay(C,1.0e-4);

        setreceiver(t12);
}		 
