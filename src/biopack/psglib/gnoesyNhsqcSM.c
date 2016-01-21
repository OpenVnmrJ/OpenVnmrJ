/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*  gnoesyNhsqcSM.c - BioPack pulse sequence adapted for Small Molecules 
                      Eriks Kupce, Oxford, 2005 

    This pulse sequence will allow one to perform the following experiment:

    3D NOESY-HSQC gradient sensitivity enhanced version for 15N, with
    H1-H1 NOESY in the first dimension, and N15 shifts in the second.

    optional magic-angle coherence transfer gradients

    Standard features include optional 13C sech/tanh pulse on CO and Cab to 
    refocus 13C coupling during t1 and t2; one lobe sinc pulse to put H2O back 
    along z (the sinc one-lobe is significantly more selective than gaussian, 
    square, or seduce 90 pulses); preservation of H20 along z during t1 and the 
    relaxation delays; option of obtaining spectrum of only NH2 groups;

    pulse sequence: 	Kay, Keifer and Saarinen, JACS, 114, 10663 (1992)
    			Zhang et al, J Biol NMR, 4, 845 (1994)
    sech/tanh pulse: 	Silver, Joseph and Hoult, JMR, 59, 347 (1984)
			Bendall, JMR, A116, 46 (1995)
         
    Written by MRB, January 1998, starting with gNhsqc from BioPack.
    Revised and improved to a standard format by MRB, BKJ and GG for the 
    BioPack, January 1998, so as to include calculation of the above 
    standard features within the pulse sequence code and associated macro.
    Auto-calibrated version, E.Kupce, 27.08.2002.


        	  CHOICE OF DECOUPLING AND 2D MODES

    Set dm = 'nnn', dmm = 'ccc'
    Set dm2 = 'nny', dmm2 = 'ccg' (or 'ccw', or 'ccp') for N15 decoupling.

    Must set phase = 1,2 and phase2 = 1,2 for States-TPPI acquisition in
    t1 [H1]  and t2 [N15].
   
    The flag f1180/f2180 should be set to 'y' if t1/t2 is to be started at
    halfdwell time. This will give 90, -180 phasing in f1/f2. If it is set to
    'n' the phasing should be 0,0 and will still give a perfect baseline.  Thus,
    set f1180='n' for (0,0) in H1 and f2180='n' for (0,0) in N15.  f1180='y' is
    ignored if ni=0, and f2180='y' is ignored if ni2=0.



          	  DETAILED INSTRUCTIONS FOR USE OF gnoesyNhsqcSM
         
    1. Centre H1, C13 and N15 frequencies as required for your sample.

    2. Ensure that H1, C13 and N15 pulse widths (pw, pwC, pwN), power levels 
       (tpwr, pwClvl, pwNlvl) and compression factors (compH, compC, compN) are
       correct. All waveforms are generated and calibrated automatically based 
       on these numbers. 
       
    3. Make sure the gzlvl2 coherence gradient is fine tuned for maximum signal.

    4. Splitting of resonances in the 1st or 2nd dimension by C13 coupling in 
       C13-enriched samples can be removed by setting C13refoc='y'.

    5. In aqueous solutions the signal loss due to exchange is achieved by 
       preserving H2O magnetization according to Kay et al. This option is 
       enabled by setting pwHs to 1500 us. The tpwrsf is then fine tuned for
       minimum residual water signal. 

    6. NH2 GROUPS:
       A spectrum of NH2 groups, with NH groups cancelled, can be obtained
       with flag NH2only='y'.  This utilises a 1/2J (J=94Hz) period of NH 
       J-coupling evolution added to t1 which cancels NH resonances and 
       inverts NH2 resonances (normal INEPT behaviour).  A 180 degree phase
       shift is added to a N15 90 pulse to provide positive NH2 signals.  The 
       NH2 resonances will be smaller (say 80%) than when NH2only='n'.

    7. Radiation Damping:
       At fields 600MHz and higher with high-Q probes, radiation damping is a
       factor to be considered. Its primary effect is in the flipback pulse
       calibration. Radiation damping causes a rotation back to the +Z axis
       even without a flipback pulse. Hence, the pwHs pulse often needs to 
       be reduced in its flip-angle. This can be accomplished by using the
       parameter tpwrsf. If this value is less than 4095.0 the value of tpwrs
       (calculated in the psg code) is increased by 6dB, thereby permitting
       the value of tpwrsf to be optimized to obtain minimum H2O in the 
       spectrum. The value of tpwrsf is typically lower than 2048 (half-maximum
       to compensate for the extra 6dB in tpwrs). 

    8. Low power long-range C-13 WURST decoupling.   
       Setting the flag wudec='y' activates low power C-13 WURST decoupling to
       remove long-range C-H couplings in C-13 labelled samples. The decoupling 
       waveform is generated "on-the-fly" from within the pulse sequence using 
       Pbox.  
       
    9. PROJECTION-RECONSTRUCTION and TILT experiments:  
       PR and TILT experiments are enabled by setting the projection angle, pra 
       to values between 0 and 90 degrees (0 < pra < 90). Note, that for these 
       experiments axis='ph', ni>1, ni2=0, phase=1,2 and phase2=1,2 must be used. 
       Processing: use wft2dx macro to obtain positive tilt angles and wft2dy for  
       negative tilt angles. For array='phase,phase2' the macros correspond to:
       wft2dx = wft2d(1,0,-1,0,0,-1,0,-1,0,-1,0,-1,-1,0,1,0)
       wft2dy = wft2d(1,0,-1,0,0,1,0,1,0,1,0,1,-1,0,1,0)
       The following relationships can be used to inter-convert the frequencies (in Hz) 
       between the tilted, F1(+)F3, F1(-)F3 and the orthogonal, F1F3, F2F3 planes:       
         F1(+) = F1*cos(pra) + F2*sin(pra)  
         F1(-) = F1*cos(pra) - F2*sin(pra)
         F1 = 0.5*[F1(+) + F1(-)]/cos(pra)
         F2 = 0.5*[F1(+) - F1(-)]/sin(pra)
       References: 
       PROJECTION-RECONSTRUCTION:
       E.Kupce and R.Freeman, J. Amer. Chem. Soc., vol. 125, pp. 13958-13959 (2003).
       E.Kupce and R.Freeman, J. Amer. Chem. Soc., vol. 126, pp. 6429-6440 (2004).
       TILT:
       E.Kupce and R.Freeman, J. Magn. Reson., vol. 172, pp. 329-332 (2005).
       E.Kupce, et al, Magn. Reson. Chem., vol. 43, pp. 791-794 (2005).
       
       Eriks Kupce, Oxford, 26.08.2004.       
*/

#include <standard.h>
#include "Pbox_bio.h"               /* Pbox Bio Pack Pulse Shaping Utilities */  

static int   phi1[2]  = {0,2},
	     phi3[4]  = {0,0,2,2},
	      
             phi9[16] = {0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3},
	     phi10[1] = {0}, 
             rec[8]   = {0,2,2,0,2,0,0,2};

static double   d2_init=0.0, d3_init=0.0;

static shape    adC180, wuCdec_lr, H2Osinc;

pulsesequence()
{
/* DECLARE AND LOAD VARIABLES */

  char        f1180[MAXSTR],   		      /* Flag to start t1 @ halfdwell */
              mag_flg[MAXSTR],    /* magic-angle coherence transfer gradients */
              f2180[MAXSTR],                  /* Flag to start t2 @ halfdwell */
              wudec[MAXSTR],	 /* automatic low power C-13 WURST decoupling */
              C13refoc[MAXSTR],	       /* adiabatic C13  pulse in middle of t1*/
	      NH2only[MAXSTR];		       /* spectrum of only NH2 groups */

 
  int         icosel,          		          /* used to get n and p type */
              t1_counter,  		        /* used for states tppi in t1 */
              t2_counter,  	 	        /* used for states tppi in t2 */
              PRexp,                        /* projection-reconstruction flag */
	      ni2 = getval("ni2");

  double      csa, sna, tau1, tau2,	                 /*  t1 and t2 delays */ 
              bw, ofs, ppm, nst,        /* bandwidth, offset, ppm, # of steps */
	      mix = getval("mix"),		 	    /* NOESY mix time */
	      tNH = 1.0/(4.0*getval("JNH")),	  /* 1/4J N15 evolution delay */
              pra = M_PI*getval("pra")/180.0,             /* projection angle */
              pwClvl = getval("pwClvl"), 	/* coarse power for C13 pulse */
              pwC = getval("pwC"),    /* C13 90 degree pulse length at pwClvl */
              compC = getval("compC"), /* adjust for C13 amplifier compression */
              pwC180 = 0.001,   /* duration of C13 180 degree adiabatic pulse */ 
              compH = getval("compH"), /* adjust for H1 amplifier compression */

   	tpwrsf = getval("tpwrsf"), /* fine power adjustment for flipback pulse */
   	pwHs = getval("pwHs"),	         /* H1 90 degree pulse length at tpwrs */
   	tpwrs = 0.0,	  	       /* power for the pwHs ("H2Osinc") pulse */
   	xdel = 2.0*GRADIENT_DELAY + POWER_DELAY,                /* xtra delay */

	pwNlvl = getval("pwNlvl"),	               /* power for N15 pulses */
        pwN = getval("pwN"),           /* N15 90 degree pulse length at pwNlvl */

	sw1 = getval("sw1"),
	sw2 = getval("sw2"),

        gzcal=getval("gzcal"),
	gt1 = getval("gt1"),  		        /* coherence pathway gradients */
	gzlvl1 = getval("gzlvl1"),
	gzlvl2 = getval("gzlvl2"),

	gt0 = getval("gt0"),				    /* other gradients */
	gt3 = getval("gt3"), 
	gt4 = getval("gt4"),
	gt5 = getval("gt5"),
	gstab = getval("gstab"),
	gzlvl0 = getval("gzlvl0"),
	gzlvl3 = getval("gzlvl3"),
	gzlvl6 = getval("gzlvl6"),
	gzlvl4 = getval("gzlvl4"),
	gzlvl5 = getval("gzlvl5");

    getstr("f1180",f1180);
    getstr("mag_flg",mag_flg);
    getstr("f2180",f2180);
    getstr("C13refoc",C13refoc);
    getstr("NH2only",NH2only);
    getstr("wudec",wudec);

/*   LOAD PHASE TABLE    */

	settable(t1,2,phi1);
	settable(t3,4,phi3);
	settable(t9,16,phi9);
	settable(t10,1,phi10);
	settable(t11,8,rec);

/*   MAKE PBOX SHAPES   */

   if((FIRST_FID) && ((C13refoc[A]=='y') || (wudec[A]=='y')))   /* call Pbox */
   {
     ppm = getval("dfrq"); ofs = 0.0; nst = 1000;   /* nst - number of steps */ 
     bw = pwC*compC;
     if(bw > 0.0) 
     {
       bw = 0.1/bw;                                     /* maximum bandwidth */
       bw = pwC180*bw*bw; 
     }
     else
       bw = 200.0*ppm; 
       
     if(C13refoc[A]=='y')
       adC180 = pbox_makeA("adC180", "wurst2i", bw, pwC180, ofs, compC*pwC, pwClvl, nst);
     if(wudec[A]=='y') 
       wuCdec_lr = pbox_Adec("wurstC_lr", "CAWURST", bw, 0.01, ofs, compC*pwC, pwClvl);
   }      
    
   if(pwHs > 1.0e-5)                /* selective H20 one-lobe sinc pulse */
   {
     if(FIRST_FID) 
       H2Osinc = pbox_Rsh("H2Osinc", "sinc90", pwHs, 0.0, compH*pw, tpwr);
     tpwrs = H2Osinc.pwr;
     pwHs = H2Osinc.pw;
   }

/* CHECK VALIDITY OF PARAMETER RANGES */

  if ((mix - gt4 - gt5) < 0.0 )
  { text_error("mix is too small. Make mix equal to %f or more.\n",(gt4 + gt5));
						   		    psg_abort(1); }

  if((dm[A] == 'y' || dm[B] == 'y' || dm[C] == 'y' ))
  { text_error("incorrect dec1 decoupler flags! Should be 'nnn' "); psg_abort(1); }

  if((dm2[A] == 'y' || dm2[B] == 'y'))
  { text_error("incorrect dec2 decoupler flags! Should be 'nny' "); psg_abort(1); }

  if( dpwr2 > 50 )
  { text_error("don't fry the probe, DPWR2 too large!  ");   	    psg_abort(1); }

  if( pw > 20.0e-6 )
  { text_error("dont fry the probe, pw too high ! ");               psg_abort(1); } 
  
  if( pwN > 100.0e-6 )
  { text_error("dont fry the probe, pwN too high ! ");              psg_abort(1); }

/* PHASES AND INCREMENTED TIMES */

/*  Phase incrementation for hypercomplex 2D data, States-Haberkorn element */

    if (phase1 == 2) 
      tsadd(t1,1,4);  
    if (phase2 == 1) 
    { tsadd(t10,2,4); icosel = 1; }
    else              icosel = -1; 

/* set up Projection-Reconstruction experiment */
   
    PRexp = 0;      
    if((pra > 0.0) && (pra < 90.0)) PRexp = 1;

    csa = cos(pra);
    sna = sin(pra);
    
    if(PRexp)
    {
      tau1 = d2*csa;
      tau2 = d2*sna;
    }
    else
    {
      tau1 = d2;
      tau2 = d3;
    }

    if((f1180[A] == 'y') && (ni > 1.0))    /*  Set up f1180, tau1 = t1 */
      tau1 += 1.0/(2.0*sw1);
    tau1 = tau1/2.0;

    if((PRexp == 0) && (f2180[A] == 'y') && (ni2 > 1.0)) /*  Set up f2180  tau2 = t2 */
      tau2 += 1.0/(2.0*sw2);     
    tau2 = tau2/2.0;

    if(tau1 < 0.2e-6) tau1 = 0.0;
    if(tau2 < 0.2e-6) tau2 = 0.0;

/* Calculate modifications to phases for States-TPPI acquisition          */

   if( ix == 1) d2_init = d2;
   t1_counter = (int) ( (d2-d2_init)*sw1 + 0.5 );
   if(t1_counter % 2) { tsadd(t1,2,4); tsadd(t11,2,4); }

   if( ix == 1) d3_init = d3;
   t2_counter = (int) ( (d3-d3_init)*sw2 + 0.5 );
   if(t2_counter % 2) { tsadd(t3,2,4); tsadd(t11,2,4); }

/*  Correct inverted signals for NH2 only spectra  */

   if(NH2only[A]=='y') { tsadd(t3,2,4); }

   if(wudec[A]=='y') xdel = xdel + POWER_DELAY + PWRF_DELAY + PRG_START_DELAY;

/* BEGIN PULSE SEQUENCE */

status(A);

	obspower(tpwr);
	decpower(pwClvl);
 	dec2power(pwNlvl);
	decpwrf(4095.0);
	txphase(zero);
        dec2phase(zero);

	delay(d1);

	dec2rgpulse(pwN, zero, 0.0, 0.0);  /* destroy N15 and C13 magnetization */
	decrgpulse(pwC, zero, 0.0, 0.0);
	zgradpulse(gzlvl0, 0.5e-3);
	delay(1.0e-4);
	dec2rgpulse(pwN, one, 0.0, 0.0);
	decrgpulse(pwC, one, 0.0, 0.0);
	zgradpulse(0.7*gzlvl0, 0.5e-3);

   	txphase(t1);
   	decphase(zero);
   	dec2phase(zero);
	delay(5.0e-4);
	rcvroff();

   	rgpulse(pw, t1, 50.0e-6, 0.0);                     /* 1H pulse excitation */

   	txphase(zero);

   if (tau1 > (2.0*GRADIENT_DELAY + pwN + 0.64*pw + 5.0*SAPS_DELAY))  
   {
     if (tau1>0.002)
     {
       zgradpulse(gzlvl6, 0.8*(tau1 - 2.0*GRADIENT_DELAY - pwN - 0.64*pw));
       delay(0.2*(tau1 - 2.0*GRADIENT_DELAY - pwN - 0.64*pw) - SAPS_DELAY);
     }
     else
     {
       delay(tau1-pwN-0.64*pw);
     }
     
     if (C13refoc[A]=='y')
       sim3pulse(0.0, 2.0*pwC, 2.0*pwN, zero, zero, zero, 0.0, 0.0);
     else
       dec2rgpulse(2.0*pwN, zero, 0.0, 0.0);
       
     if (tau1>0.002)
     {
       zgradpulse(-1.0*gzlvl6, 0.8*(tau1 - 2.0*GRADIENT_DELAY - pwN - 0.64*pw));
       delay(0.2*(tau1 - 2.0*GRADIENT_DELAY - pwN - 0.64*pw) - SAPS_DELAY);
     }
     else
     {
       delay(tau1-pwN-0.64*pw);
     }
   }
   else if (tau1 > (0.64*pw + 0.5*SAPS_DELAY))
     delay(2.0*tau1 - 2.0*0.64*pw - SAPS_DELAY );

   	rgpulse(pw, zero, 0.0, 0.0);

	delay(mix - gt4 - gt5 -gstab -200.0e-6);
	dec2rgpulse(pwN, zero, 0.0, 0.0);
	zgradpulse(gzlvl4, gt4);
        delay(gstab);

   	rgpulse(pw, zero, 200.0e-6,0.0);			       /* HSQC begins */

   	dec2phase(zero);
	zgradpulse(gzlvl0, gt0);
	delay(tNH - gt0);

   	sim3pulse(2.0*pw, 0.0, 2.0*pwN, zero, zero, zero, 0.0, 0.0);

   	txphase(one);
	zgradpulse(gzlvl0, gt0);
	delay(tNH - gt0);

 	rgpulse(pw, one, 0.0, 0.0);
	txphase(two);
        if (tpwrsf<4095.0)
        {
          obspower(tpwrs+6.0); obspwrf(tpwrsf);
   	  shaped_pulse("H2Osinc", pwHs, two, 5.0e-4, 0.0);
	  obspower(tpwr); obspwrf(4095.0);
        }
        else
        {
          obspower(tpwrs);
   	  shaped_pulse("H2Osinc", pwHs, two, 5.0e-4, 0.0);
	  obspower(tpwr);
        }
	zgradpulse(gzlvl3, gt3);
	dec2phase(t3);
	decpwrf(adC180.pwrf);
	delay(2.0e-4);
   	dec2rgpulse(pwN, t3, 0.0, 0.0);
	decphase(zero);


/*  xxxxxxxxxxxxxxxxxx    OPTIONS FOR N15 EVOLUTION    xxxxxxxxxxxxxxxxxxxxx  */

        txphase(zero);
	dec2phase(t9);

     if (NH2only[A]=='y')	
     {      
    	delay(tau2);
         			  /* optional sech/tanh pulse in middle of t2 */
    	if (C13refoc[A]=='y') 				   /* WFG_START_DELAY */
           { decshaped_pulse("adC180", pwC180, zero, 0.0, 0.0);
             delay(tNH - 1.0e-3 - WFG_START_DELAY - 2.0*pw); }
    	else
           { delay(tNH - 2.0*pw);}
    	rgpulse(2.0*pw, zero, 0.0, 0.0);
    	if (tNH < gt1 + 1.99e-4)  delay(gt1 + 1.99e-4 - tNH);

    	delay(tau2);

    	dec2rgpulse(2.0*pwN, t9, 0.0, 0.0);
        if (mag_flg[A] == 'y')
          magradpulse(gzcal*gzlvl1, gt1);
        else
          zgradpulse(gzlvl1, gt1);
    	dec2phase(t10);
   	if (tNH > gt1 + 1.99e-4)  delay(tNH - gt1 - 2.0*GRADIENT_DELAY);
   	else   delay(1.99e-4 - 2.0*GRADIENT_DELAY);
     }
     else
     {
        if ( (C13refoc[A]=='y') && (tau2 > 0.5e-3 + WFG2_START_DELAY) )
           {  delay(tau2 - 0.5e-3 - WFG2_START_DELAY); /* WFG2_START_DELAY */
            simshaped_pulse("", "adC180", 2.0*pw, pwC180, zero, zero, 0.0, 0.0);
            delay(tau2 - 0.5e-3);
            delay(gt1 + 2.0e-4);}
	else
           { delay(tau2);
            rgpulse(2.0*pw, zero, 0.0, 0.0);
            delay(gt1 + 2.0e-4 - 2.0*pw);
            delay(tau2); }

	dec2rgpulse(2.0*pwN, t9, 0.0, 0.0);

        if(mag_flg[A] == 'y')
          magradpulse(gzcal*gzlvl1, gt1);
        else
          zgradpulse(gzlvl1, gt1);
          
	dec2phase(t10);
	delay(2.0e-4 - 2.0*GRADIENT_DELAY);
     } 

/*  xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx  */

	sim3pulse(pw, 0.0, pwN, zero, zero, t10, 0.0, 0.0);

	dec2phase(zero);
	zgradpulse(gzlvl5, gt5);
	delay(tNH - 1.5*pwN - gt5);

	sim3pulse(2.0*pw, 0.0, 2.0*pwN, zero, zero, zero, 0.0, 0.0);

	zgradpulse(gzlvl5, gt5);
	txphase(one);
	dec2phase(one);
	delay(tNH  - 1.5*pwN - gt5);

	sim3pulse(pw, 0.0, pwN, one, zero, one, 0.0, 0.0);

	txphase(zero);
	dec2phase(zero);
	zgradpulse(1.5*gzlvl5, gt5);
	delay(tNH - 1.5*pwN - gt5);

	sim3pulse(2.0*pw, 0.0, 2.0*pwN, zero, zero, zero, 0.0, 0.0);

	zgradpulse(1.5*gzlvl5, gt5);
	delay(tNH - pwN - 0.5*pw - gt5);

	rgpulse(pw, zero, 0.0, 0.0);

	delay((gt1/10.0) + 1.0e-4+ gstab - 0.5*pw + xdel);

	rgpulse(2.0*pw, zero, 0.0, rof1);
	dec2power(dpwr2);				       /* POWER_DELAY */
	
        if (mag_flg[A] == 'y')
          magradpulse(icosel*gzcal*gzlvl2, gt1/10.0);
        else
          zgradpulse(icosel*gzlvl2, gt1/10.0);
        delay(gstab + rof2);
        
	setreceiver(t11);
        rcvron();
        statusdelay(C,1.0e-4-rof1);		
        if(wudec[A]=='y') 
        {
          decpwrf(4095.0);
          decpower(wuCdec_lr.pwr+3.0);
          decprgon("wurstC_lr", 1.0/wuCdec_lr.dmf, wuCdec_lr.dres);
          decon();
        }	
}		 
