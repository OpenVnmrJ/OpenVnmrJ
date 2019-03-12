/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*  gcbca_nh.c

    3D CBCANH gradient sensitivity enhanced version.


    Correlates Cb(i),Ca(i) with N(i), NH(i) and N(i+1), NH(i+1).  Uses constant
    time evolution for the CaCb shifts and for the 15N shifts.
    
    Standard features include maintaining the 13C carrier in the CaCb region
    throughout using off-res SLP pulses; full power square pulses on 13C 
    initially when 13CO excitation is irrelevant; square pulses on the Ca and
    CaCb with first null at 13CO; one lobe sinc pulses on 13CO with first null
    at Ca; optional 2H decoupling when CaCb magnetization is transverse for 4 
    channel spectrometers.

    Magic-angle option for coherence transfer gradients.  TROSY option for
    N15/H1 evolution/detection.

    pulse sequence: 	J Magn. Reson. 99, 201 (1992).
    SLP pulses:     	J Magn. Reson. 96, 94-102 (1992)
    shaka6 composite: 	Chem. Phys. Lett. 120, 201 (1985)
    TROSY:		 JACS, 120, 10778 (1998)
 
    (comments such as "point a" refer to pulse sequence diagram in refernce)

    This sequence is modified from the sequence gcbca_co_nh.c (MRB) by BKJ
    12/6/96, corrected BKJ 12/11/96.   Revised and improved to a standard
    format by MRB, BKJ and GG for the BioPack, January 1997. 
    TROSY added Dec 98, based on similar addition to gNhsqc. (Version Dec 1998).
    Semi-Constant Time option -  Eriks Kupce, Oxford, 26.01.2006, based on  
                                 example provided by Marco Tonelli at NMRFAM

        	  CHOICE OF DECOUPLING AND 2D MODES

    	Set dm = 'nnn', dmm = 'ccc' 
    	Set dm2 = 'nny', dmm2 = 'ccg' (or 'ccw', or 'ccp') for 15N decoupling.
	Set dm3 = 'nnn' for no 2H decoupling, or
		  'nyn'  and dmm3 = 'cwc' for 2H decoupling. 
  
    Must set = 1,2 and phase2 = 1,2 for States-TPPI acquisition in
    t1 [13C]  and t2 [15N].

    The flag f1180/f2180 should be set to 'y' if t1/t2 is to be started at
    halfdwell time. This will give 90, -180 phasing in f1/f2. If it is set to
    'n' the phasing should be 0,0 and will still give a perfect baseline.  Thus,
    set f1180='n' for (0,0) in 13C and f2180='n' for (0,0) in 15N.  f1180='y' is
    ignored if ni=0, and f2180='y' is ignored if ni2=0.



          	  DETAILED INSTRUCTIONS FOR USE OF gcbca_nh


    1. Obtain a printout of the Philosopy behind the BioPack development,
       and General Instructions using the macro: 
                                      "printon man('BioPack') printoff".
       These Detailed Instructions for gcbca_nh may be printed using:
                                      "printon man('gcbca_nh') printoff".
             
    2. Apply the setup macro "gcbca_nh".  This loads the relevant parameter set
       and also sets ni=ni2=0 and phase=phase2=1 ready for a 1D spectral check.

    3. The delay zeta, between points c and d in the pulse sequence, is set
       to 6 ms by the pulse sequence code for the 1D check, or for 2D/15N 
       spectra, to provide a spectrum of large positive signals.
       The normal value for 2D/13C nmr, or 3D nmr, is 0.011 (11 ms), but at this
       value positive and negative signals approximately cancel in the 1D
       spectrum (see JMR 99, 201 for an explanation).  zeta is automatically 
       reset to the value in the dg2 parameter set (normally 11 ms) in the pulse
       sequence code for 2D/13C and 3D work (ie when ni>1).  

    4. Centre H1 frequency on H2O (4.7ppm), C13 frequency on 46ppm, and N15 
       frequency on the amide region (120 ppm).  The C13 frequency remains at 
       46ppm, ie at CaCb throughout the sequence.

    5. The H1 frequency is NOT shifted to the amide region during the sequence.
       The H1 DIPSI2 decoupling is controlled by the waltzB1 parameter.

    6. tauCH (1.7 ms) and timeTN (14 ms) were determined for alphalytic protease
       and are listed in dg2 for possible readjustment by the user.

    7. The coherence-transfer gradients using power levels
       gzlvl1 and gzlvl2 may be either z or magic-angle gradients. For the
       latter, a proper /vnmr/imaging/gradtable entry must be present and
       syscoil must contain the value of this entry (name of gradtable). The
       amplitude of the gzlvl1 and gzlvl2 should be lower than for a z axis
       probe to have the x and y gradient levels within the 32k range. For
       any value, a dps display (using power display) shows the x,y and z
       dac values. These must be <=32k.

    8. TROSY:
       Set TROSY='y' and dm2='nnn' for a TROSY spectrum of the bottom right
       peak of the 2D coupled NH quartet (high-field H1, low-field N15).  The 
       TROSY spectrum gives 50% S/N compared to the decoupled spectrum for a 
       small peptide.  To select any of the other three peaks of the 2D coupled
       quartet, in a clockwise direction from bottom right, change t4/t10
       from x/y to x/-y to -x/-y to -x/y.  NOTE, the phases of the SE train
       are almost the same as those determined for the gNhsqc sequence.  The
       major difference is that kappa is eliminated compared to normal
       gcbca_nh so the N15 magnetization has not evolved with respect to the 
       attached H's.  I.e. the N15 state would be Ix rather than IySz if there 
       was no coherence gradient - this imparts a 90 degree shift so t8 is 
       changed to y (from x in the normal spectrum).  Also gzlvl1 is after the
       180 N15 pulse rather than before as in gNhsqc, so the sign of icosel 
       and the t4/t10 phase2 increments are also swapped compared to gNhsqc.     

    9. PROJECTION-RECONSTRUCTION experiments:  
       Projection-Reconstruction experiments are enabled by setting the projection 
       angle, pra to values between 0 and 90 degrees (0 < pra < 90). Note, that for 
       these experiments axis='ph', ni>1, ni2=0, phase=1,2 and phase2=1,2 must be used. 
       Processing: use wft2dx macro for positive tilt angles and wft2dy for negative 
       tilt angles. 
       wft2dx = wft2d(1,0,-1,0,0,1,0,1,0,1,0,1,-1,0,1,0)
       wft2dy = wft2d(1,0,-1,0,0,-1,0,-1,0,1,0,1,1,0,-1,0)
       The following relationships can be used to inter-convert the frequencies (in Hz) 
       between the tilted, F1(+)F3, F1(-)F3 and the orthogonal, F1F3, F2F3 planes:       
         F1(+) = F1*cos(pra) + F2*sin(pra)  
         F1(-) = F1*cos(pra) - F2*sin(pra)
         F1 = 0.5*[F1(+) + F1(-)]/cos(pra)
         F2 = 0.5*[F1(+) - F1(-)]/sin(pra)
       References: 
       E.Kupce and R.Freeman, J. Amer. Chem. Soc., vol. 125, pp. 13958-13959 (2003).
       E.Kupce and R.Freeman, J. Amer. Chem. Soc., vol. 126, pp. 6429-6440 (2004).
       Related:
       S.Kim and T.Szyperski, J. Amer. Chem. Soc., vol. 125, pp. 1385-1393 (2003).
       Eriks Kupce, Oxford, 26.08.2004.       
*/



#include <standard.h>


static int  /*  T is for TROSY='y', phx etc also enable TROSY phase changes */
                      phx[1]={0},   phy[1]={1},

	     phi8[2]  = {0,2},			    phi8T[2] = {1,3},
             phi9[8]  = {0,0,1,1,2,2,3,3},
             rec[4]   = {0,2,2,0},		     recT[2] = {3,1};

static double   d2_init=0.0, d3_init=0.0;
            
void pulsesequence()
{



/* DECLARE AND LOAD VARIABLES */

char        f1180[MAXSTR],   		      /* Flag to start t1 @ halfdwell */
            f2180[MAXSTR],    		      /* Flag to start t2 @ halfdwell */
            SCT[MAXSTR],    /* Semi-constant time flag for N-15 evolution */
 	    mag_flg[MAXSTR],      /* magic-angle coherence transfer gradients */
 	    TROSY[MAXSTR];			    /* do TROSY on N15 and H1 */
 
int         icosel,          			  /* used to get n and p type */
            t1_counter,  		        /* used for states tppi in t1 */
            t2_counter,  	 	        /* used for states tppi in t2 */
            PRexp,                          /* projection-reconstruction flag */
	    ni = getval("ni"),
	    ni2 = getval("ni2");

double      tau1,         				         /*  t1 delay */
            tau2,        				         /*  t2 delay */
	    tauCH = getval("tauCH"), 		         /* 1/4J delay for CH */
	    zeta = getval("zeta"),   /* zeta delay, 0.006 for 1D, 0.011 for 2D*/
            timeTN = getval("timeTN"),     /* constant time for 15N evolution */
	    t2a=0.0, t2b=0.0, halfT2=0.0, CTdelay=0.0,
	    timeCH = 1.1e-3,				      /* other delays */
	    timeAB = 3.3e-3,
	    kappa = 5.4e-3,
	    lambda = 2.4e-3,
            csa, sna,
            pra = M_PI*getval("pra")/180.0,
            
	pwClvl = getval("pwClvl"), 	        /* coarse power for C13 pulse */
        pwC = getval("pwC"),          /* C13 90 degree pulse length at pwClvl */
	rf0,            	  /* maximum fine power when using pwC pulses */

/* 90 degree pulse at Cab(46ppm), first off-resonance null at CO (174ppm)     */
        pwC1,		              /* 90 degree pulse length on C13 at rf1 */
        rf1,		       /* fine power for 5.1 kHz rf for 600MHz magnet */

/* 180 degree pulse at Cab(46ppm), first off-resonance null at CO(174ppm)     */
        pwC2,		                    /* 180 degree pulse length at rf2 */
        rf2,		      /* fine power for 11.4 kHz rf for 600MHz magnet */

/* the following pulse lengths for SLP pulses are automatically calculated    */
/* by the macro "proteincal".  SLP pulse shapes, "offC7" etc are called       */
/* directly from your shapelib.                    			      */
   pwC7 = getval("pwC7"),    /* 180 degree selective sinc pulse on CO(174ppm) */
   rf7,	                           /* fine power for the pwC7 ("offC7") pulse */

   compH = getval("compH"),       /* adjustment for C13 amplifier compression */
   compC = getval("compC"),       /* adjustment for C13 amplifier compression */

   	pwH,	    		        /* H1 90 degree pulse length at tpwr1 */
   	tpwr1,	  	                     /* 9.2 kHz rf magnet for DIPSI-2 */
   	DIPSI2time,     	        /* total length of DIPSI-2 decoupling */
        ncyc_dec,
        waltzB1=getval("waltzB1"),

	pwNlvl = getval("pwNlvl"),	              /* power for N15 pulses */
        pwN = getval("pwN"),          /* N15 90 degree pulse length at pwNlvl */

	sw1 = getval("sw1"),
	sw2 = getval("sw2"),

	gt1 = getval("gt1"),  		       /* coherence pathway gradients */
        gzcal  = getval("gzcal"),            /* g/cm to DAC conversion factor */
	gzlvl1 = getval("gzlvl1"),
	gzlvl2 = getval("gzlvl2"),

	gt0 = getval("gt0"),				   /* other gradients */
	gt3 = getval("gt3"),
	gt4 = getval("gt4"),
	gt5 = getval("gt5"),
	gstab = getval("gstab"),
	gzlvl0 = getval("gzlvl0"),
	gzlvl3 = getval("gzlvl3"),
	gzlvl4 = getval("gzlvl4"),
	gzlvl5 = getval("gzlvl5"),
	gzlvl6 = getval("gzlvl6");

    getstr("f1180",f1180);
    getstr("f2180",f2180);
    getstr("SCT",SCT);
    getstr("mag_flg",mag_flg);
    getstr("TROSY",TROSY);
 
/*   LOAD PHASE TABLE    */

	settable(t3,1,phx);
	settable(t4,1,phx);
   if (TROSY[A]=='y')
       {settable(t8,2,phi8T);
	settable(t9,1,phx);
 	settable(t10,1,phy);
	settable(t11,1,phx);
	settable(t12,2,recT);}
    else
       {settable(t8,2,phi8);
	settable(t9,8,phi9);
	settable(t10,1,phx);
	settable(t11,1,phy);
	settable(t12,4,rec);}




/*   INITIALIZE VARIABLES   */

    if( dpwrf < 4095 )
	{ printf("reset dpwrf=4095 and recalibrate C13 90 degree pulse");
	  psg_abort(1); }
 
    /* set zeta to 6ms for 1D spectral check, otherwise it will be the    */
    /* value in the dg2 parameter set (about 11ms) for 2D/13C and 3D work */
        if (ni>1)  zeta = zeta;
	else  zeta = 0.006;
	
    /* maximum fine power for pwC pulses */
	rf0 = 4095.0;

    /* 90 degree pulse on Cab, null at CO 128ppm away */
	pwC1 = sqrt(15.0)/(4.0*128.0*dfrq);
        rf1 = 4095.0*(compC*pwC/pwC1);
	rf1 = (int) (rf1 + 0.5);
	
    /* 180 degree pulse on Cab, null at CO 128ppm away */
        pwC2 = sqrt(3.0)/(2.0*128.0*dfrq);
	rf2 = (4095.0*compC*pwC*2.0)/pwC2;
	rf2 = (int) (rf2 + 0.5);	
	if( rf2 > 4095 )
       { printf("increase pwClvl so that C13 90 < 21.9us*(600/sfrq)"); psg_abort(1);}
	
    /* 180 degree one-lobe sinc pulse on CO, null at Ca 118m away */
	rf7 = (compC*4095.0*pwC*2.0*1.65)/pwC7;	/* needs 1.65 times more     */
	rf7 = (int) (rf7 + 0.5);		/* power than a square pulse */

    /* power level and pulse times for DIPSI 1H decoupling */
	DIPSI2time = 2.0*zeta + 2.0*timeTN - 5.4e-3 + pwC1 + 5.0*pwN + gt3 + 5.0e-5 + 2.0*GRADIENT_DELAY + 3.0*POWER_DELAY;
        pwH = 1.0/(4.0*waltzB1);
     ncyc_dec = (DIPSI2time*90.0)/(pwH*2590.0*4.0);  
     ncyc_dec = (int) (ncyc_dec +0.5);
	pwH = (DIPSI2time*90.0)/(ncyc_dec*2590.0*4.0);  /* fine correction of pwH */
	tpwr1 = 4095*(compH*pw/pwH);
	tpwr1 = (int) (2.0*tpwr1 + 0.5);  /* x2 because obs atten will be reduced by 6dB */

/* set up Projection-Reconstruction experiment */
 
    tau1 = d2; tau2 = d3; 
    PRexp=0; csa = 1.0; sna = 0.0;   
    if((pra > 0.0) && (pra < 90.0)) /* PR experiments */
    {
      PRexp = 1;
      csa = cos(pra); 
      sna = sin(pra);
      tau1 = d2*csa;  
      tau2 = d2*sna;
    }

/* CHECK VALIDITY OF PARAMETER RANGES */

    if(SCT[A] == 'n')
    {
      if (PRexp) 
      {
        if( 0.5*ni*sna/sw1 > timeTN - WFG3_START_DELAY)
          { printf(" ni is too big. Make ni equal to %d or less.\n",
          ((int)((timeTN - WFG3_START_DELAY)*2.0*sw1/sna)));         psg_abort(1);}
      }
      else 
      {
        if ( 0.5*ni2*1/(sw2) > timeTN - WFG3_START_DELAY)
         { printf(" ni2 is too big. Make ni2 equal to %d or less.\n", 
           ((int)((timeTN - WFG3_START_DELAY)*2.0*sw2)));              psg_abort(1);}
      }
    }
    
    if ( 0.5*ni*1/(sw1) > timeAB - gt4 - WFG_START_DELAY - pwC7 )
       { printf(" ni is too big. Make ni equal to %d or less.\n",
         ((int)((timeAB - gt4 - WFG_START_DELAY - pwC7)*2.0*sw1))); psg_abort(1);}

    if ( dm[A] == 'y' || dm[B] == 'y' || dm[C] == 'y' )
       { printf("incorrect dec1 decoupler flags! Should be 'nnn' "); psg_abort(1);}

    if ( dm2[A] == 'y' || dm2[B] == 'y' )
       { printf("incorrect dec2 decoupler flags! Should be 'nny' "); psg_abort(1);}

    if ( dm3[A] == 'y' || dm3[C] == 'y' )
       { printf("incorrect dec3 decoupler flags! Should be 'nyn' or 'nnn' ");
							             psg_abort(1);}
    if ( dpwr2 > 50 )
       { printf("dpwr2 too large! recheck value  ");		     psg_abort(1);}

    if ( pw > 20.0e-6 )
       { printf(" pw too long ! recheck value ");	             psg_abort(1);} 
  
    if ( pwN > 100.0e-6 )
       { printf(" pwN too long! recheck value ");	             psg_abort(1);} 
 
    if ( TROSY[A]=='y' && dm2[C] == 'y')
       { text_error("Choose either TROSY='n' or dm2='n' ! ");        psg_abort(1);}


/* PHASES AND INCREMENTED TIMES */

/*  Phase incrementation for hypercomplex 2D data, States-Haberkorn element */

    if (phase1 == 2)   tsadd(t3,1,4);  
    if (TROSY[A]=='y')
	 {  if (phase2 == 2)   				      icosel = +1;
            else 	    {tsadd(t4,2,4);  tsadd(t10,2,4);  icosel = -1;}
	 }
    else {  if (phase2 == 2)  {tsadd(t10,2,4); icosel = +1;}
            else 			       icosel = -1;    
	 }

/* Calculate modifications to phases for States-TPPI acquisition          */

   if( ix == 1) d2_init = d2;
   t1_counter = (int) ( (d2-d2_init)*sw1 + 0.5 );
   if(t1_counter % 2) 
	{ tsadd(t3,2,4); tsadd(t12,2,4); }

   if( ix == 1) d3_init = d3;
   t2_counter = (int) ( (d3-d3_init)*sw2 + 0.5 );
   if(t2_counter % 2) 
	{ tsadd(t8,2,4); tsadd(t12,2,4); }

/* Set up CONSTANT/SEMI-CONSTANT time evolution in N15 */

    halfT2 = 0.0;  
    CTdelay = timeTN + pwC2 + WFG_START_DELAY - SAPS_DELAY;

    if(ni>1)                
    {
      if(f1180[A] == 'y')     /*  Set up f1180 */
        tau1 += 0.5*csa/sw1;  /* if not PRexp then csa = 1.0 */
      if(PRexp)
      {
        halfT2 = 0.5*(ni-1)/sw1;  /* ni2 is not defined */
        if(f1180[A] == 'y') 
        { tau2 += 0.5*sna/sw1; halfT2 += 0.25*sna/sw1; }
        t2b = (double) t1_counter*((halfT2 - CTdelay)/((double)(ni-1)));
      }
    }
    if (ni2>1)
    {
      halfT2 = 0.5*(ni2-1)/sw2;
      if(f2180[A] == 'y')        /*  Set up f2180  */
      { tau2 += 0.5/sw2; halfT2 += 0.25/sw2; }
      t2b = (double) t2_counter*((halfT2 - CTdelay)/((double)(ni2-1)));
    }
    tau1 = tau1/2.0;
    tau2 = tau2/2.0;
    if(tau1 < 0.2e-6) tau1 = 0.0; 
    if(tau2 < 0.2e-6) tau2 = 0.0; 

    if(t2b < 0.0) t2b = 0.0;
    t2a = CTdelay - tau2 + t2b;
    if(t2a < 0.2e-6)  t2a = 0.0;

/* uncomment these lines to check t2a and t2b 
    printf("%d: t2a = %.12f", t2_counter,t2a);
    printf(" ; t2b = %.12f\n", t2b);
*/

/* BEGIN ACTUAL PULSE SEQUENCE */

status(A);
   	delay(d1);
        if ( dm3[B] == 'y' )
          { lk_hold(); lk_sampling_off();}  /*freezes z0 correction, stops lock pulsing*/

	rcvroff();
	obspower(tpwr);
	decpower(pwClvl);
 	dec2power(pwNlvl);
	decpwrf(rf0);
	obsoffset(tof);
	txphase(zero);
   	delay(1.0e-5);
        if (TROSY[A] == 'n')
	dec2rgpulse(pwN, zero, 0.0, 0.0);  /*destroy N15 and C13 magnetization*/
	decrgpulse(pwC, zero, 0.0, 0.0);
	zgradpulse(gzlvl0, 0.5e-3);
	delay(1.0e-4);
        if (TROSY[A] == 'n')
	dec2rgpulse(pwN, one, 0.0, 0.0);
	decrgpulse(pwC, zero, 0.0, 0.0);
	zgradpulse(0.7*gzlvl0, 0.5e-3);
	delay(5.0e-4);

   	rgpulse(pw,zero,0.0,0.0);                      /* 1H pulse excitation */

   	decphase(zero);
	zgradpulse(gzlvl0, gt0);
	delay(tauCH - gt0);

   	simpulse(2.0*pw, 2.0*pwC, zero, zero, 0.0, 0.0);

   	txphase(one);
	decphase(t3);
	zgradpulse(gzlvl0, gt0);
	delay(tauCH - gt0);

   	rgpulse(pw, one, 0.0, 0.0);
	zgradpulse(gzlvl3, gt3);
	delay(2.0e-4);
      if ( dm3[B] == 'y' )     /* begins optional 2H decoupling */
        {
          gt4=0.0;             /* no gradients during 2H decoupling */
          dec3rgpulse(1/dmf3,one,10.0e-6,2.0e-6);
          dec3unblank();
          dec3phase(zero);
          delay(2.0e-6);
          setstatus(DEC3ch, TRUE, 'w', FALSE, dmf3);
        }
   	decrgpulse(pwC, t3, 0.0, 0.0);                         	
								/* point a */	
	txphase(zero);
	decphase(zero);
        decpwrf(rf7);
     	delay(tau1);

						        /*  WFG3_START_DELAY  */
	sim3shaped_pulse("", "offC7", "", 0.0, pwC7, 2.0*pwN, zero, zero, zero, 
								     0.0, 0.0);

	zgradpulse(gzlvl4, gt4);
        decpwrf(rf2);	
        if ( pwC7 > 2.0*pwN)
     	   {delay(timeCH - pwC7 - gt4 - WFG3_START_DELAY - 2.0*pw);}
        else
     	   {delay(timeCH - 2.0*pwN - gt4 - WFG3_START_DELAY - 2.0*pw);}

     	rgpulse(2.0*pw,zero,0.0,0.0);

     	delay(timeAB - timeCH);

     	decrgpulse(pwC2, zero, 0.0, 0.0);

	zgradpulse(gzlvl4, gt4);
        decpwrf(rf7);
     	delay(timeAB - tau1 - gt4 - WFG_START_DELAY - pwC7 - 2.0e-6);

 							/*  WFG_START_DELAY   */
     	decshaped_pulse("offC7", pwC7, zero, 0.0, 0.0);
                                                              
	decpwrf(rf1);					       	/* point b */
   	decrgpulse(pwC1, zero, 2.0e-6, 0.0);                   		
	obspwrf(tpwr1); obspower(tpwr-6);				       /* POWER_DELAY */
	obsprgon("dipsi2", pwH, 5.0);		           /* PRG_START_DELAY */
	xmtron();
								/* point c */
	dec2phase(zero);
	decpwrf(rf2);
	delay(zeta - 2.0*POWER_DELAY - PRG_START_DELAY);

	sim3pulse(0.0, pwC2, 2.0*pwN, zero, zero, zero, 0.0, 0.0); 

	decpwrf(rf1);
	dec2phase(t8);
	delay(zeta);
								/* point d */
	decrgpulse(pwC1, zero, 0.0, 0.0);                        
        if ( dm3[B] == 'y' )   /* turns off 2H decoupling  */
           {
           setstatus(DEC3ch, FALSE, 'c', FALSE, dmf3);
           dec3rgpulse(1/dmf3,three,2.0e-6,2.0e-6);
           dec3blank();
           lk_autotrig();   /* resumes lock pulsing */
           }

/*  xxxxxxxxxxxxxxxxxx    OPTIONS FOR N15 EVOLUTION    xxxxxxxxxxxxxxxxxxxxx  */

	zgradpulse(gzlvl3, gt3);
        if (TROSY[A]=='y') { xmtroff(); obsprgoff(); }
 	delay(2.0e-4);
	dec2rgpulse(pwN, t8, 0.0, 0.0);       /* N15 EVOLUTION BEGINS HERE */
								/* point e */
	decpwrf(rf2);
	decphase(zero);
	dec2phase(t9);

        if(SCT[A] == 'y')
        {
	  delay(t2a);
          dec2rgpulse(2.0*pwN, t9, 0.0, 0.0);
	  delay(t2b);
          decrgpulse(pwC2, zero, 0.0, 0.0); /* WFG_START_DELAY  */	
        }
        else
        {	
          delay(timeTN - tau2);
	  sim3pulse(0.0, pwC2, 2.0*pwN, zero, zero, t9, 0.0, 0.0);
        }
	
	dec2phase(t10);
        decpwrf(rf7);

if (TROSY[A]=='y')
{
    if (tau2 > gt1 + 2.0*GRADIENT_DELAY + 1.0e-4)
	{
	  txphase(t4);
          delay(timeTN - pwC7 - WFG_START_DELAY);          /* WFG_START_DELAY */
          decshaped_pulse("offC7", pwC7, zero, 0.0, 0.0);
          delay(tau2 - gt1 - 2.0*GRADIENT_DELAY - 1.0e-4);
          if (mag_flg[A]=='y')  magradpulse(gzcal*gzlvl1, gt1);
          else  zgradpulse(gzlvl1, gt1);   	/* 2.0*GRADIENT_DELAY */
	  obspwrf(4095.0); obspower(tpwr);				       /* POWER_DELAY */
	  delay(1.0e-4 - 2.0*POWER_DELAY);
	}
    else
	{
	  txphase(t4);
          delay(timeTN -pwC7 -WFG_START_DELAY -gt1 -2.0*GRADIENT_DELAY -1.0e-4);
          if (mag_flg[A]=='y')    magradpulse(gzcal*gzlvl1, gt1);
          else  zgradpulse(gzlvl1, gt1);	   	/* 2.0*GRADIENT_DELAY */
	  obspwrf(4095.0); obspower(tpwr);				       /* POWER_DELAY */
	  delay(1.0e-4 - 2.0*POWER_DELAY);                    /* WFG_START_DELAY */
          decshaped_pulse("offC7", pwC7, zero, 0.0, 0.0);
          delay(tau2);
	}
}
else
{
    if (tau2 > kappa)
	{
          delay(timeTN - pwC7 - WFG_START_DELAY);     	   /* WFG_START_DELAY */
          decshaped_pulse("offC7", pwC7, zero, 0.0, 0.0);
          delay(tau2 - kappa - PRG_STOP_DELAY);
          xmtroff();
          obsprgoff();					    /* PRG_STOP_DELAY */
	  txphase(t4);
          delay(kappa - gt1 - 2.0*GRADIENT_DELAY - 1.0e-4);
          if (mag_flg[A]=='y')    magradpulse(gzcal*gzlvl1, gt1);
          else    zgradpulse(gzlvl1, gt1);   	/* 2.0*GRADIENT_DELAY */
	  obspwrf(4095.0); obspower(tpwr);				       /* POWER_DELAY */
	  delay(1.0e-4 - 2.0*POWER_DELAY);
	}
    else if (tau2 > (kappa - pwC7 - WFG_START_DELAY))
	{
          delay(timeTN + tau2 - kappa - PRG_STOP_DELAY);
          xmtroff();
          obsprgoff();					    /* PRG_STOP_DELAY */
	  txphase(t4);                                     /* WFG_START_DELAY */
          decshaped_pulse("offC7", pwC7, zero, 0.0, 0.0);
          delay(kappa -pwC7 -WFG_START_DELAY -gt1 -2.0*GRADIENT_DELAY -1.0e-4);
          if (mag_flg[A]=='y')    magradpulse(gzcal*gzlvl1, gt1);
          else    zgradpulse(gzlvl1, gt1);   	/* 2.0*GRADIENT_DELAY */
	  obspwrf(4095.0); obspower(tpwr);				       /* POWER_DELAY */
	  delay(1.0e-4 - 2.0*POWER_DELAY);
	}
    else if (tau2 > gt1 + 2.0*GRADIENT_DELAY + 1.0e-4)
	{
          delay(timeTN + tau2 - kappa - PRG_STOP_DELAY);
          xmtroff();
          obsprgoff();					    /* PRG_STOP_DELAY */
	  txphase(t4);
          delay(kappa - tau2 - pwC7 - WFG_START_DELAY);    /* WFG_START_DELAY */
          decshaped_pulse("offC7", pwC7, zero, 0.0, 0.0);
          delay(tau2 - gt1 - 2.0*GRADIENT_DELAY - 1.0e-4);
          if (mag_flg[A]=='y')    magradpulse(gzcal*gzlvl1, gt1);
          else    zgradpulse(gzlvl1, gt1);   	/* 2.0*GRADIENT_DELAY */
	  obspwrf(4095.0); obspower(tpwr);				       /* POWER_DELAY */
	  delay(1.0e-4 - 2.0*POWER_DELAY);
	}
    else
	{
          delay(timeTN + tau2 - kappa - PRG_STOP_DELAY);
          xmtroff();
	  obsprgoff();					    /* PRG_STOP_DELAY */
	  txphase(t4);
    	  delay(kappa-tau2-pwC7-WFG_START_DELAY-gt1-2.0*GRADIENT_DELAY-1.0e-4);
          if (mag_flg[A]=='y')    magradpulse(gzcal*gzlvl1, gt1);
          else    zgradpulse(gzlvl1, gt1);   	/* 2.0*GRADIENT_DELAY */
	  obspwrf(4095.0); obspower(tpwr);				       /* POWER_DELAY */
	  delay(1.0e-4 - 2.0*POWER_DELAY);                    /* WFG_START_DELAY */
          decshaped_pulse("offC7", pwC7, zero, 0.0, 0.0);
          delay(tau2);
	}
}                                                            	/* point f */
/*  xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx  */
	if (TROSY[A]=='y')  rgpulse(pw, t4, 0.0, 0.0);
	else                sim3pulse(pw, 0.0, pwN, t4, zero, t10, 0.0, 0.0);

	txphase(zero);
	dec2phase(zero);
	zgradpulse(gzlvl5, gt5);
	if (TROSY[A]=='y')   delay(lambda - 0.65*(pw + pwN) - gt5);
	else   delay(lambda - 1.3*pwN - gt5);

	sim3pulse(2.0*pw, 0.0, 2.0*pwN, zero, zero, zero, 0.0, 0.0);

	zgradpulse(gzlvl5, gt5);
	txphase(one);
	dec2phase(t11);
	delay(lambda - 1.3*pwN - gt5);

	sim3pulse(pw, 0.0, pwN, one, zero, t11, 0.0, 0.0);

	txphase(zero);
	dec2phase(zero);
	zgradpulse(gzlvl6, gt5);
	delay(lambda - 1.3*pwN - gt5);

	sim3pulse(2.0*pw, 0.0, 2.0*pwN, zero, zero, zero, 0.0, 0.0);

	dec2phase(t10);
	zgradpulse(gzlvl6, gt5);
	if (TROSY[A]=='y')   delay(lambda - 1.6*pwN - gt5);
	else   delay(lambda - 0.65*pwN - gt5);

	if (TROSY[A]=='y')   dec2rgpulse(pwN, t10, 0.0, 0.0); 
	else    	     rgpulse(pw, zero, 0.0, 0.0); 

	delay((gt1/10.0) + 1.0e-4 +gstab - 0.5*pw + 2.0*GRADIENT_DELAY + POWER_DELAY);

	rgpulse(2.0*pw, zero, 0.0, rof1);
	dec2power(dpwr2);				       /* POWER_DELAY */
        if (mag_flg[A] == 'y')    magradpulse(icosel*gzcal*gzlvl2, gt1/10.0);
        else   zgradpulse(icosel*gzlvl2, gt1/10.0);            /* 2.0*GRADIENT_DELAY */
        delay(gstab);
        rcvron();
statusdelay(C,1.0e-4 - rof1);
   if (dm3[B]=='y') lk_sample();

	setreceiver(t12);
}		 
