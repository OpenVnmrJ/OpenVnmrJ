/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*  gNhsqcTROSY_CCLS_U.c 

   1H,15N-TROSY pulse sequence. Sensitivity Enhanced.
   CCLS experiment on uniformly labeled samples 

   The CCLS sequences are useful for proteins where only few amino
   acids are labeled to figure out which pairs of labeled amino acids 
   are next to each other. The *_U.c should be used when the labeled 
   amino acids have C13 at both C' and Calfa positions, otherwise
   if only C' is labeled, gNhsqcTROSY_CCLS.c should be used.

   set CCLS='y' to turn on suppression of NH attached to 13CO
   set CCLS='n' to collect regular spectrum
   timeCT=0.016 will give maximum suppression of HN-CO signals

   recommend running the experiment interleaved:
   CCLS='n','y'
   ni=64 phase=1,2 f1180=y


if array='phase,CCLS' then
  wft2d(1,0,0,0,-1,0,0,0,0,-1,0,0,0,-1,0,0)  "for CCLS='n'"
  wft2d(0,0, 1,0, 0,0,-1,0,0,0,0,-1,0,0,0,-1)  "for CCLS='y'"
  wft2d(1,0,-1,0,-1,0,1,0,0,-1,0,1,0,-1,0,1) "for the difference"
 endif
if array='CCLS,phase' then
  wft2d(1,0,-1,0,0,0,0,0,0,-1,0,-1,0,0,0,0)  "for CCLS='n'"
  wft2d(0,0,0,0,1,0,-1,0,0,0,0,0,0,-1,0,-1)  "for CCLS='y'"
  wft2d(1,0,-1,0,-1,0,1,0,0,-1,0,-1,0,1,0,1)  "for difference"
if array='phase' then wft2da 


   dof is set to CO region (174ppm)


   f1coef='1 0 -1 0 0 1 0 1' 
   (in nmrPipe you need to process indirect dimension with "|nmrPipe -fn FT -neg" flag)


   Calibration of flipback pulses.

   There are three different water flipback pulses, the parameters that need to be 
   calibrated are the fine power and small angle phase for each flipback:
	-tpwrsf_t, phincr_t	(flipback following 1st INEPT transfer)
	-tpwrsf_a, phincr_a	(flipback following 1st 1H 90 pulse after 15N evolution)
	-tpwrsf_d, phincr_d	(flipback before last INEPT transfer)

    See "Carbonyl carbon label selective (CCLS) 1H-15N HSQC
    experiment for improved detection of backbone 13C-15N
    cross peaks in larger proteins", J. Biomol.NMR, 39, 177-185 (2007),
    Marco Tonelli, Larry R. Masterson, Klaas Hallenga, Gianiuigi Veglia
    and John Markley.
*/


#include <standard.h>
#include "Pbox_bio.h"               /* Pbox Bio Pack Pulse Shaping Utilities */
  

	     
static int   phx[1]={0},   ph_x[1]={2},    ph_y[1]={3},

	     phi3[2]  = {0,2},	
             recT[2]  = {1,3};

static double   d2_init=0.0;

static shape H2Osinc, offC3, offC6, offC8;

pulsesequence()
{
/* DECLARE AND LOAD VARIABLES */

char        CCLS[MAXSTR],
	    f1180[MAXSTR],   		      /* Flag to start t1 @ halfdwell */
            mag_flg[MAXSTR];      /* magic-angle coherence transfer gradients */
 
int         icosel,          			  /* used to get n and p type */
            t1_counter,  		        /* used for states tppi in t1 */
	    TROSYselect;             /* choose which component of the coupled */
                                     /* quartet to select (default=1)         */

double      timeCT = getval("timeCT"), tmp,
	    tau1,         				         /*  t1 delay */
	    lambda = 0.91/(4.0*getval("JNH")), 	   /* 1/4J H1 evolution delay */
        
/* the sech/tanh pulse is automatically calculated by the macro "proteincal", */  
/* and is called directly from your shapelib.                  		      */
   pwClvl = getval("pwClvl"), 	  	        /* coarse power for C13 pulse */
   pwC = getval("pwC"),     	      /* C13 90 degree pulse length at pwClvl */
   compC = getval("compC"),       /* adjustment for C13 amplifier compression */
   rf0,            	          /* maximum fine power when using pwC pulses */

   bw, ofs, ppm,   /* bandwidth, pulsewidth, offset, ppm, # steps */

   pwC3,		   /*180 degree pulse at Ca(56ppm) null at CO(174ppm) */
   pwC6,                      /* 90 degree selective sinc pulse on CO(174ppm) */
   pwC8,                     /* 180 degree selective sinc pulse on CO(174ppm) */
   rf3,	                           /* fine power for the pwC3 ("offC3") pulse */
   rf6,	                           /* fine power for the pwC6 ("offC6") pulse */
   rf8,	                           /* fine power for the pwC8 ("offC8") pulse */

   compH = getval("compH"),        /* adjustment for H1 amplifier compression */

   	pwHs = getval("pwHs"),	        /* H1 90 degree pulse length at tpwrs */

        tpwrs = 0.0,
        tpwrsf_t,           /* fine power for the pwHs ("H2Osinc") pulse */
        tpwrsf_a,           /* fine power for the pwHs ("H2Osinc") pulse */
        tpwrsf_d,           /* fine power for the pwHs ("H2Osinc") pulse */
        phincr_t,           /* small angle phase for the pwHs ("H2Osinc") pulse */
        phincr_a,           /* small angle phase for the pwHs ("H2Osinc") pulse */
        phincr_d,           /* small angle phase for the pwHs ("H2Osinc") pulse */

	pwNlvl = getval("pwNlvl"),	              /* power for N15 pulses */
        pwN = getval("pwN"),          /* N15 90 degree pulse length at pwNlvl */

	sw1 = getval("sw1"),

	gt1 = getval("gt1"),  		       /* coherence pathway gradients */
	gt2 = getval("gt2"),  		       /* coherence pathway gradients */
        gzcal = getval("gzcal"),               /* dac to G/cm conversion      */
	gzlvl1 = getval("gzlvl1"),
	gzlvl2 = getval("gzlvl2"),

	gt0 = getval("gt0"),				   /* other gradients */
	gt3 = getval("gt3"),
	gt5 = getval("gt5"),
	gstab = getval("gstab"),
	gzlvl0 = getval("gzlvl0"),
	gzlvl3 = getval("gzlvl3"),
	gzlvl5 = getval("gzlvl5");

    getstr("CCLS",CCLS);
    getstr("f1180",f1180);
    getstr("mag_flg",mag_flg);

    TROSYselect = getval("TROSYselect");
    if ((TROSYselect < 1) || (TROSYselect > 4)) TROSYselect=1;

/*   LOAD PHASE TABLE    */
	
        settable(t3,2,phi3);
        settable(t1,1,ph_x);
	settable(t9,1,phx);
	settable(t4,1,ph_x); 
	settable(t10,1,ph_y);
	settable(t11,1,phx);
	settable(t12,2,recT);

/*   INITIALIZE VARIABLES   */

/* maximum fine power for pwC pulses */
    rf0 = 4095.0;

    setautocal();                        /* activate auto-calibration flags */

    if (autocal[0] == 'n')
    {
    /* offC3 - 180 degree pulse on Ca, null at CO 118ppm away */
        pwC3 = getval("pwC3");    
        rf3 = (compC*4095.0*pwC*2.0)/pwC3;
	rf3 = (int) (rf3 + 0.5);  
	
    /* 90 degree one-lobe sinc pulse on CO, null at Ca 118ppm away */	
        pwC6 = getval("pwC6");    
	rf6 = (compC*4095.0*pwC*1.69)/pwC6;	/* needs 1.69 times more     */
	rf6 = (int) (rf6 + 0.5);		/* power than a square pulse */

    /* 180 degree one-lobe sinc pulse on CO, null at Ca 118ppm away */
        pwC8 = getval("pwC8");
	rf8 = (compC*4095.0*pwC*2.0*1.65)/pwC8;	/* needs 1.65 times more     */
	rf8 = (int) (rf8 + 0.5);		      /* power than a square pulse */

    /* selective H20 one-lobe sinc pulse needs 1.69  */
    /* times more power than a square pulse */
        if (pwHs > 1e-6) tpwrs = tpwr - 20.0*log10(pwHs/(compH*pw*1.69));
          else tpwrs = 0.0;
        tpwrs = (int) (tpwrs);
    }
    else        /* if autocal = 'y'(yes), 'q'(quiet), r(read), or 's'(semi) */
    {
      if (FIRST_FID)                                            /* call Pbox */
	{
         ppm = getval("dfrq"); 
         bw = 118.0*ppm; ofs = -bw; 
         offC3 = pbox_make("offC3", "square180n", bw, ofs, compC*pwC, pwClvl);
         offC6 = pbox_make("offC6", "sinc90n", bw, 0.0, compC*pwC, pwClvl);
         offC8 = pbox_make("offC8", "sinc180n", bw, 0.0, compC*pwC, pwClvl);

         H2Osinc = pbox_Rsh("H2Osinc", "sinc90", pwHs, 0.0, compH*pw, tpwr);
	}

      pwHs = H2Osinc.pw; tpwrs = H2Osinc.pwr-1.0;   /* 1dB correction applied */

      pwC3 = offC3.pw; rf3 = offC3.pwrf;             
      pwC6 = offC6.pw; rf6 = offC6.pwrf; 
      pwC8 = offC8.pw; rf8 = offC8.pwrf;
    }

/* CHECK VALIDITY OF PARAMETER RANGES */

  if (f1180[A]=='y') tmp=0.5; else tmp=1.0;

  if ((timeCT -0.5*(ni-tmp)/sw1 -SAPS_DELAY) < 0.2e-6)
    { text_error(" ni is too big. Make ni equal to %d or less.\n", 
		 (int)(2.0*sw1*(timeCT -SAPS_DELAY) +tmp)); psg_abort(1);}

  if((dm[A] == 'y' || dm[B] == 'y' || dm[C] == 'y' ))
  { text_error("incorrect dec1 decoupler flags! Should be 'nnn' "); psg_abort(1); }

  if((dm2[A] == 'y' || dm2[B] == 'y' || dm2[C] == 'y'))
  { text_error("incorrect dec2 decoupler flags! Should be 'nny' "); psg_abort(1); }

  if( dpwr2 > 46 )
  { text_error("don't fry the probe, DPWR2 too large!  ");   	    psg_abort(1); }

  if( pw > 50.0e-6 )
  { text_error("dont fry the probe, pw too high ! ");               psg_abort(1); } 
  

/* PHASES AND INCREMENTED TIMES */
/*  Phase incrementation for hypercomplex 2D data, States-Haberkorn element */

    if (phase1 == 1) icosel = -1;
      else {tsadd(t4,2,4); tsadd(t10,2,4); icosel = +1;}

/*  Set up f1180  */
   
    tau1 = d2;
    if((f1180[A] == 'y') && (ni > 1.0)) 
	{ tau1 += ( 1.0 / (2.0*sw1) ); if(tau1 < 0.2e-6) tau1 = 0.0; }
    tau1 = tau1/2.0;

/* Change t4 and t10 phases to collect different components of the coupled quartet */

   if (TROSYselect == 2) {tsadd(t10,2,4);}
     else if (TROSYselect == 3) {tsadd(t4,2,4); tsadd(t10,2,4);}
     else if (TROSYselect == 4) {tsadd(t4,2,4);}

/* Calculate modifications to phases for States-TPPI acquisition          */

   if( ix == 1) d2_init = d2;
   t1_counter = (int) ( (d2-d2_init)*sw1 + 0.5 );
   if(t1_counter % 2) 
	{ tsadd(t3,2,4); tsadd(t12,2,4); }

/* if they exist read fine power and phase correction parameters from the experiment */
    if (find("tpwrsf_t") > 0) tpwrsf_t=getval("tpwrsf_t");
      else tpwrsf_t=4095.0;
    if (find("tpwrsf_a") > 0) tpwrsf_a=getval("tpwrsf_a");
      else tpwrsf_a=4095.0;
    if (find("tpwrsf_d") > 0) tpwrsf_d=getval("tpwrsf_d");
      else tpwrsf_d=4095.0;
    if (find("phincr_t") > 0) phincr_t=getval("phincr_t");
      else phincr_t=0.0;
    if (find("phincr_a") > 0) phincr_a=getval("phincr_a");
      else phincr_a=0.0;
    if (find("phincr_d") > 0) phincr_d=getval("phincr_d");
      else phincr_d=0.0;

/* set all "undefined" fine powers to 2048 and double coarse power (+6db) */
    if (tpwrsf_t==0.0 || tpwrsf_t==4095.0) tpwrsf_t=2048.0;
    if (tpwrsf_a==0.0 || tpwrsf_a==4095.0) tpwrsf_a=2048.0;
    if (tpwrsf_d==0.0 || tpwrsf_d==4095.0) tpwrsf_d=2048.0;
    tpwrs = tpwrs +6.0;

/* make sure all phincr values are > 0.0 */
    if (phincr_t<0.0) phincr_t=phincr_t+360.0;
    if (phincr_a<0.0) phincr_a=phincr_a+360.0;
    if (phincr_d<0.0) phincr_d=phincr_d+360.0;


/* BEGIN PULSE SEQUENCE */

status(A);

	obsstepsize(1.0);

	obspower(tpwr);
	obspwrf(4095.0);
	decpower(pwClvl);
	decpwrf(rf0);
 	dec2power(pwNlvl);
	txphase(t1);
        decphase(zero);
        dec2phase(zero);

	delay(d1);
        rcvroff();
	delay(5.0e-4);

      if ( dm3[B] == 'y' )     /* begins optional 2H decoupling */
        {
          lk_hold();
          dec3rgpulse(1/dmf3,one,10.0e-6,2.0e-6);
          dec3unblank();
          dec3phase(zero);
          delay(2.0e-6);
          setstatus(DEC3ch, TRUE, 'w', FALSE, dmf3);
        }

   	rgpulse(pw,t1,0.0,0.0);                 /* 1H pulse excitation */

	txphase(zero);
   	dec2phase(zero);
	zgradpulse(gzlvl0, gt0);
	delay(lambda -gt0);

   	sim3pulse(2.0*pw, 0.0, 2.0*pwN, zero, zero, zero, 0.0, 0.0);

   	txphase(one);
	zgradpulse(gzlvl0, gt0);
	delay(lambda -gt0);

 	rgpulse(pw, one, 0.0, 0.0);
/* H2O flipback pulse */
        initval(phincr_t,v11);
        txphase(two); xmtrphase(v11);
        obspwrf(tpwrsf_t); obspower(tpwrs); 
        shaped_pulse("H2Osinc", pwHs, two, 2.0e-6, 2.0e-6);   
        obspower(tpwr); obspwrf(4095.0);
        txphase(zero); xmtrphase(zero);
/* H2O flipback pulse */
	zgradpulse(gzlvl3, gt3);

	dec2phase(t3);
	delay(gstab);

   	dec2rgpulse(pwN, t3, 0.0, 0.0);
/*  xxxxxxxxxxxxxxxxxx    OPTIONS FOR N15 EVOLUTION    xxxxxxxxxxxxxxxxxxxxx  */
	dec2phase(t9);

        if (CCLS[A]=='y')
          {
	   decpwrf(rf8);
           delay(timeCT -tau1 -SAPS_DELAY -WFG3_START_DELAY -PWRF_DELAY);

           sim3shaped_pulse("","offC8","",0.0, pwC8, 2.0*pwN, zero, zero, t9, 0.0, 0.0);

           if (mag_flg[A] == 'y')  magradpulse(gzcal*gzlvl1, gt1);
             else  zgradpulse(gzlvl1, gt1);   	/* 2.0*GRADIENT_DELAY */

	   decpwrf(rf3);
	   txphase(t4); 
	   dec2phase(zero);
           delay(timeCT -gt1 -2.0*GRADIENT_DELAY -2.0*SAPS_DELAY
		  -PWRF_DELAY -pwC3 -WFG_START_DELAY -2.0e-6); 
           decshaped_pulse("offC3",pwC3, zero, 2.0e-6, 0.0);
           delay(tau1);
          }
	 else if (CCLS[A]=='n')
          {
           decpwrf(rf3);
           delay(timeCT -tau1 -SAPS_DELAY -PWRF_DELAY);

           dec2rgpulse(2.0*pwN, t9, 0.0, 0.0);

           if (mag_flg[A] == 'y')  magradpulse(gzcal*gzlvl1, gt1);
             else  zgradpulse(gzlvl1, gt1);   	/* 2.0*GRADIENT_DELAY */

	   txphase(t4); 
	   dec2phase(zero);
           delay(timeCT -gt1 -2.0*GRADIENT_DELAY -2.0*SAPS_DELAY
		  -pwC3 -WFG_START_DELAY -PWRF_DELAY 
		  -pwC8 -WFG_START_DELAY -2.0e-6); 

           decshaped_pulse("offC3",pwC3, zero, 0.0, 0.0);
           decpwrf(rf8);
           decshaped_pulse("offC8", pwC8, zero, 2.0e-6, 0.0);

           delay(tau1);
          }
/*  xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx  */
	rgpulse(pw, t4, 0.0, 0.0);

/* H2O flipback pulse */
        initval(phincr_a,v11);
        xmtrphase(v11);
        obspower(tpwrs); obspwrf(tpwrsf_a); 
        shaped_pulse("H2Osinc", pwHs, t4, 2.0e-6, 2.0e-6);   
        obspower(tpwr); obspwrf(4095.0);
        txphase(zero); xmtrphase(zero);
/* H2O flipback pulse */

        decpwrf(rf6);
        decshaped_pulse("offC6",pwC6, zero, 2.0e-6, 0.0);  

	zgradpulse(gzlvl5, gt5);
	delay(lambda -0.65*(pw +pwN) -gt5 -PWRF_DELAY -WFG_START_DELAY -pwC6 -2.0e-6
		-pwHs -WFG_START_DELAY -3.0*SAPS_DELAY -2.0*POWER_DELAY -2.0*PWRF_DELAY -4.0e-6);

	sim3pulse(2.0*pw, 0.0, 2.0*pwN, zero, zero, zero, 0.0, 0.0);

	zgradpulse(gzlvl5, gt5);
	txphase(one);
	dec2phase(t11);
	delay(lambda -1.3*pwN -gt5
		-pwHs -WFG_START_DELAY -4.0*SAPS_DELAY -2.0*POWER_DELAY -2.0*PWRF_DELAY -6.0e-6);

/* H2O flipback pulse */
        initval(phincr_d,v11);
        txphase(three); xmtrphase(v11);
        obspower(tpwrs); obspwrf(tpwrsf_d); 
        shaped_pulse("H2Osinc", pwHs, three, 2.0e-6, 2.0e-6);   
        obspower(tpwr); obspwrf(4095.0);
        txphase(zero); xmtrphase(zero);
/* H2O flipback pulse */
	sim3pulse(pw, 0.0, pwN, one, zero, t11, 2.0e-6, 0.0);

	txphase(zero);
	dec2phase(zero);
	zgradpulse(1.5*gzlvl5, gt5);
	delay(lambda - 1.3*pwN - gt5);

	sim3pulse(2.0*pw, 0.0, 2.0*pwN, zero, zero, zero, 0.0, 0.0);

	dec2phase(t10);
	zgradpulse(1.5*gzlvl5, gt5);
	delay(lambda - 1.6*pwN - gt5);

	dec2rgpulse(pwN, t10, 0.0, 0.0); 

	delay(gt2 +gstab - 0.65*pw + 2.0*GRADIENT_DELAY + POWER_DELAY);

        if ( dm3[B] == 'y' )   /* turns off 2H decoupling  */
           {
           setstatus(DEC3ch, FALSE, 'c', FALSE, dmf3);
           dec3rgpulse(1/dmf3,three,2.0e-6,2.0e-6);
           dec3blank();
           lk_autotrig();   /* resumes lock pulsing */
           }

	rgpulse(2.0*pw, zero, 0.0, 0.0);

	dec2power(dpwr2);				       /* POWER_DELAY */
        if (mag_flg[A] == 'y')	  magradpulse(icosel*gzcal*gzlvl2, gt2);
        else   zgradpulse(icosel*gzlvl2, gt2);		/* 2.0*GRADIENT_DELAY */
        delay(0.5*gstab);
        rcvron();
statusdelay(C,0.5*gstab);		

  if (dm3[B] == 'y') {delay(1/dmf3); lk_sample();}

	setreceiver(t12);
}		 
