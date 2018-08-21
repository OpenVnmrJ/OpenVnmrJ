/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*  gNhsqcS3.c
    
    HSQC gradient sensitivity enhanced version for N15.
    Set dm2='nnn'

    For NH:
      Set NH2='n'
      Set abfilter='a' or 'b', phase=1,2 for the subspectral editing i.e. to select
      a or b state of 1JNH doublet. Use wft2d(1,0,-1,0,0,-1,0,-1).
      Set abfilter='a','b' and phase=1,2 for interleaved collection. Set array='abfilter,phase'

    For NH2:
      Set NH2='y'
      Set abfilter='a' or 'b', phase=1,2 for the subspectral editing i.e. to select
      inner or outer lines of 1JNH2 doublet of doublets. Use wft2d(1,0,-1,0,0,-1,0,-1).
      Set abfilter='a','b' and phase=1,2 for interleaved collection. Set array='abfilter,phase' 

    For Interleaved collection:
      for one component use wft2d(1,0,-1,0,0,0,0,0,0,-1,0,-1,0,0,0,0) and
      for other component,  wft2d(0,0,0,0,1,0,-1,0,0,0,0,0,0,-1,0,-1) 

      for both components (s/n reduced by sqrt(2)):
       wft2d(1,0,-1,0,1,0,-1,0,0,-1,0,-1,0,-1,0,-1) 


    pulse sequence: 	P. Permi, J. Biomol. NMR 22, 27-35 (2002).  
    sech/tanh pulse: 	Silver, Joseph and Hoult, JMR, 59, 347 (1984)
			Bendall, JMR, A116, 46 (1995)
    
    Written by P.Permi, starting with gNhsqc.c from BioPack, as gNhsqc_jnh_pp.c
       and gNhsqc_jnh2_pp.c
    Modified for BioPack by G.Gray (Varian). Combined NH and NH2 options as gNhsqcS3.c


        	  CHOICE OF DECOUPLING AND 2D MODES

    Set dm = 'nnn', dmm = 'ccc'
    Set dm2 = 'nnn', dmm2 = 'ccc'
    Set dm3='nyn', dmm2='cwc' for 2H decoupling using 4th channel

    Must set phase = 1,2  for States-TPPI acquisition in t1 [N15].
    
    The flag f1180 should be set to 'y' if t1 is to be started at halfdwell
    time. This will give -90, 180 phasing in f1. If it is set to 'n' the 
    phasing should be 0,0 and will still give a perfect baseline.  Thus,
    set f1180='n' for (0,0) in N15.  f1180='y' is ignored if ni=0.


       Center H1 frequency on H2O (4.7ppm), C13 frequency on 100ppm, and N15 
       frequency on the amide region (120 ppm).

       Splitting of resonances in the N15 dimension by C13 coupling in C13
       enriched samples can be removed by setting C13refoc='y'.

       H2O preservation is achieved with sinc one-lobe selective pulse 
       used to put H2O back along z.

       The coherence-transfer gradients using power levels
       gzlvl1 and gzlvl2 may be either z or magic-angle gradients. For the
       latter, a proper /vnmr/imaging/gradtable entry must be present and
       syscoil must contain the value of this entry (name of gradtable). The
       amplitude of the gzlvl1 and gzlvl2 should be lower than for a z axis
       probe to have the x and y gradient levels within the 32k range. For
       any value, a dps display (using power display) shows the x,y and z
       dac values. These must be <=32k.

*/



#include <standard.h>
  
	     
static int   phx[1]={0},
	     phi3[2]  = {0,2},	
             phi9[8]  = {0,0,1,1,2,2,3,3},	
             rec[4]   = {0,2,2,0};

static double   d2_init=0.0;


pulsesequence()
{

/* DECLARE AND LOAD VARIABLES */

char        abfilter[MAXSTR],  /* flag for selection of inner or outer pair of doublets */
            NH2[MAXSTR],      /* flag for selection of NH2 */
            f1180[MAXSTR],   		      /* Flag to start t1 @ halfdwell */
            mag_flg[MAXSTR],      /* magic-angle coherence transfer gradients */
	    C13refoc[MAXSTR];		/* C13 sech/tanh pulse in middle of t1*/
	    
 
int         icosel,          			  /* used to get n and p type */
            t1_counter;  		        /* used for states tppi in t1 */
	    
double      tau1,         				         /*  t1 delay */
	    lambda = 0.91/(4.0*getval("JNH")), 	   /* 1/4J H1 evolution delay */
	            
   pwClvl = getval("pwClvl"), 	  	        /* coarse power for C13 pulse */
   pwC = getval("pwC"),     	      /* C13 90 degree pulse length at pwClvl */
   rf0,            	          /* maximum fine power when using pwC pulses */
   rfst,	                           /* fine power for the stCall pulse */

   compH = getval("compH"),        /* adjustment for H1 amplifier compression */
   compC = getval("compC"),       /* adjustment for C13 amplifier compression */

	calH = getval("calH"), /* multiplier on a pw pulse for H1 calibration */
   	pwHs = getval("pwHs"),	        /* H1 90 degree pulse length at tpwrs */
        tpwrsf_u = getval("tpwrsf_u"),/* fine power correction for flipback(up)*/
        tpwrsf_d = getval("tpwrsf_d"),/* fine power correction for flipback(down)*/
   	tpwrs,	  	              /* power for the pwHs ("H2Osinc") pulse */

	pwNlvl = getval("pwNlvl"),	              /* power for N15 pulses */
        pwN = getval("pwN"),          /* N15 90 degree pulse length at pwNlvl */
	calN = getval("calN"),   /* multiplier on a pwN pulse for calibration */

	sw1 = getval("sw1"),

	gt1 = getval("gt1"),  		       /* coherence pathway gradients */
        gzcal = getval("gzcal"),               /* dac to G/cm conversion      */
	gzlvl1 = getval("gzlvl1"),
	gzlvl2 = getval("gzlvl2"),

	gstab = getval("gstab"),			   /* field recovery  */
	gt0 = getval("gt0"),				   /* other gradients */
	gt3 = getval("gt3"),
	gt5 = getval("gt5"),
	gzlvl0 = getval("gzlvl0"),
	gzlvl3 = getval("gzlvl3"),
	gzlvl5 = getval("gzlvl5");

    getstr("NH2",NH2);
    getstr("f1180",f1180);
    getstr("mag_flg",mag_flg);
    getstr("C13refoc",C13refoc);
    getstr("abfilter",abfilter);


/*   LOAD PHASE TABLE    */
	
        settable(t3,2,phi3);
	settable(t4,1,phx);
        settable(t1,1,phx);
	settable(t9,8,phi9);
 	settable(t10,1,phx);
        settable(t12,4,rec);


/*   INITIALIZE VARIABLES   */

/* maximum fine power for pwC pulses (and initialize rfst) */
	rf0 = 4095.0;    rfst=0.0;

/* 180 degree adiabatic C13 pulse from 0 to 200 ppm */
     if (C13refoc[A]=='y')
       {rfst = (compC*4095.0*pwC*4000.0*sqrt((30.0*sfrq/600.0+7.0)/0.35));   
	rfst = (int) (rfst + 0.5);
	if ( 1.0/(4000.0*sqrt((30.0*sfrq/600.0+7.0)/0.35)) < pwC )
           { text_error( " Not enough C13 RF. pwC must be %f usec or less.\n", 
	    (1.0e6/(4000.0*sqrt((30.0*sfrq/600.0+7.0)/0.35))) ); psg_abort(1); }}

/* selective H20 one-lobe sinc pulse */
    tpwrs = tpwr - 20.0*log10(pwHs/(compH*pw*1.69));   /*needs 1.69 times more*/
    tpwrs = (int) (tpwrs);                   	  /*power than a square pulse */
    if (tpwrsf_d<4095.0) tpwrs=tpwrs+6;   /* nominal tpwrsf_d ~ 2048 */
         /* tpwrsf_d,tpwrsf_u can be used to correct for radiation damping  */

/* reset calH and calN for 2D if inadvertently left at 2.0 */
  if (ni>1.0) {calH=1.0; calN=1.0;}



/* CHECK VALIDITY OF PARAMETER RANGES */

  if((dm[A] == 'y' || dm[B] == 'y' || dm[C] == 'y' ))
  { text_error("incorrect dec1 decoupler flags! Should be 'nnn' "); psg_abort(1); }

  if((dm2[A] == 'y' || dm2[B] == 'y' || dm2[C] == 'y' ))
  { text_error("incorrect dec2 decoupler flags! Should be 'nnn' "); psg_abort(1); }

  if((NH2[A] != 'y') && (NH2[A] != 'n'))
  { text_error("NH2 should be 'y' or 'n'!"); psg_abort(1); }

  if((abfilter[A] != 'a') && (abfilter[A] != 'b'))
  { text_error("abfilter should be 'a' or 'b'!"); psg_abort(1); }

  if( dpwr2 > 46 )
  { text_error("don't fry the probe, DPWR2 too large!  "); psg_abort(1); }

  if( pw > 50.0e-6 )
  { text_error("dont fry the probe, pw too high ! "); psg_abort(1); } 
  
  if( pwN > 100.0e-6 )
  { text_error("dont fry the probe, pwN too high ! "); psg_abort(1); }



/* PHASES AND INCREMENTED TIMES */

/*  Phase incrementation for hypercomplex 2D data, States-Haberkorn element */


    if (phase1 == 1)  {tsadd(t10,2,4); icosel = +1;}
            else 			       icosel = -1;


/*  Set up f1180  */
   
    tau1 = d2;
    if((f1180[A] == 'y') && (ni > 1.0)) 
	{ tau1 += ( 1.0 / (2.0*sw1) ); if(tau1 < 0.2e-6) tau1 = 0.0; }
    tau1 = tau1/2.0;



/* Calculate modifications to phases for States-TPPI acquisition          */

   if( ix == 1) d2_init = d2;
   t1_counter = (int) ( (d2-d2_init)*sw1 + 0.5 );
   if(t1_counter % 2) 
	{ tsadd(t3,2,4); tsadd(t12,2,4); }


/* BEGIN PULSE SEQUENCE */

status(A);

	obspower(tpwr);
	decpower(pwClvl);
	decpwrf(rf0);
 	dec2power(pwNlvl);
	txphase(zero);
        decphase(zero);
        dec2phase(zero);

	delay(d1);

 
/*  xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx  */
        rcvroff();
	dec2rgpulse(pwN, zero, 0.0, 0.0);  
	decrgpulse(pwC, zero, 0.0, 0.0);   /*destroy N15 and C13 magnetization*/
	zgradpulse(gzlvl0, 0.5e-3);
	delay(1.0e-4);
	dec2rgpulse(pwN, one, 0.0, 0.0);
	decrgpulse(pwC, one, 0.0, 0.0);
	zgradpulse(0.7*gzlvl0, 0.5e-3);
	decpwrf(rfst);
	txphase(t1);
	delay(5.0e-4);

   if(dm3[B] == 'y')				  /*optional 2H decoupling on */
         {lk_hold();
	  dec3unblank();
          dec3rgpulse(1/dmf3, one, 0.0, 0.0); 
          dec3unblank();
          setstatus(DEC3ch, TRUE, 'w', FALSE, dmf3);} 

   	rgpulse(calH*pw,t1,0.0,0.0);                 /* 1H pulse excitation */

	txphase(zero);
   	dec2phase(zero);
	zgradpulse(gzlvl0, gt0);
	delay(lambda - gt0);

   	sim3pulse(2.0*pw, 0.0, 2.0*pwN, zero, zero, zero, 0.0, 0.0);

   	txphase(one);
	zgradpulse(gzlvl0, gt0);
	delay(lambda - gt0);

 	rgpulse(pw, one, 0.0, 0.0);
	txphase(two);
   	obspower(tpwrs); obspwrf(tpwrsf_d);
   	shaped_pulse("H2Osinc_d", pwHs, two, 5.0e-5, 0.0);
	obspower(tpwr); obspwrf(4095.0);
	zgradpulse(gzlvl3, gt3);
	dec2phase(t3);
	delay(gstab);
   	dec2rgpulse(calN*pwN, t3, 0.0, 0.0);
	txphase(zero);
	decphase(zero);

/*  xxxxxxxxxxxxxxxxxx    OPTIONS FOR N15 EVOLUTION    xxxxxxxxxxxxxxxxxxxxx  */

	txphase(zero);
	dec2phase(t9);

  	if ( (C13refoc[A]=='y') && (tau1 > 0.5e-3 + WFG2_START_DELAY) )
           {delay(tau1 - 0.5e-3 - WFG2_START_DELAY);     /* WFG2_START_DELAY */
            simshaped_pulse("", "stC200", 2.0*pw, 1.0e-3, zero, zero, 0.0, 0.0);
            decphase(zero);
            delay(tau1 - 0.5e-3);
            zgradpulse(-1*gzlvl1, 0.5*gt1);
            delay(gstab - 2.0*GRADIENT_DELAY);}
	else
           {delay(tau1);
            rgpulse(2.0*pw, zero, 0.0, 0.0);
            delay(tau1);
            zgradpulse(-1*gzlvl1, 0.5*gt1);
            delay(gstab - 2.0*GRADIENT_DELAY);
            }  

	dec2rgpulse(2.0*pwN, t9, 0.0, 0.0);

        zgradpulse(gzlvl1, 0.5*gt1);   	/* 2.0*GRADIENT_DELAY */
	txphase(t4);
	dec2phase(t10);
	delay(gstab - 2.0*GRADIENT_DELAY);

/*  xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx  */

	sim3pulse(pw, 0.0, pwN, t4, zero, t10, 0.0, 0.0);

        obspower(tpwrs); obspwrf(tpwrsf_u);
   	shaped_pulse("H2Osinc_u", pwHs, two, 5.0e-5, 0.0);
	obspower(tpwr); obspwrf(4095.0);

	txphase(zero);
	dec2phase(zero);
	zgradpulse(gzlvl5, gt5);
	
	delay(lambda - 1.3*pwN - gt5 - pwHs - 5.0e-5 -2.0*POWER_DELAY);

	sim3pulse(2.0*pw, 0.0, 2.0*pwN, zero, zero, zero, 0.0, 0.0);

	zgradpulse(gzlvl5, gt5);
	txphase(one);
	delay(lambda - 1.3*pwN - gt5);

if(abfilter[A] == 'a')
        {
         if (NH2[A] == 'n') 
          {
           dec2rgpulse(pwN,one,0.0,0.0);
          } 
         else
          {
           dec2rgpulse(pwN,zero,0.0,0.0);
          } 
        }
       else
        {
         if (NH2[A] == 'n') 
          {
           dec2rgpulse(pwN,three,0.0,0.0);
          } 
         else
          {
           dec2rgpulse(pwN,two,0.0,0.0);
          } 
        }

	delay(gt1/10.0 -pwN +gstab + 2.0*GRADIENT_DELAY + POWER_DELAY );

	rgpulse(2.0*pw, zero, 0.0, 0.0);

	dec2power(dpwr2);				       /* POWER_DELAY */
        if (mag_flg[A] == 'y')	  magradpulse(gzcal*icosel*gzlvl2, 0.1*gt1);
        else   zgradpulse(icosel*gzlvl2, 0.1*gt1);		/* 2.0*GRADIENT_DELAY */
        delay(gstab);

  if (dm3[B] == 'y') {delay(1/dmf3); lk_sample();}

	setreceiver(t12);
}		 
