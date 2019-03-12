/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*  rna_gNhsqc.c

    HSQC gradient sensitivity enhanced version for N15, with options for
    TROSY on N15/H1 and for T1, T1rho, and T2 relaxation measurements of the
    N15 nuclei. Coherence transfer gradients may be z or magic-angle (using 
    triax probe).

                        NOTE: dof must be set at 110ppm always
                        NOTE: dof2 must be set at 200ppm always

    Standard features include optional 13C sech/tanh pulse to 
    refocus 13C coupling during t1; 1D checks of N15 and H1 pulse times using 
    calN and calH; one lobe sinc pulse to put H2O back along z (the sinc 
    one-lobe is significantly more selective than gaussian, square, or seduce
    90 pulses); preservation of H20 along z during t1 and the relaxation
    delays; option of obtaining spectrum of only NH2 groups; 2H decoupling 
    option for partially-deuterated (ie NHD) groups.
  

    pulse sequence: 	Kay, Keifer and Saarinen, JACS, 114, 10663 (1992)
    sech/tanh pulse: 	Silver, Joseph and Hoult, JMR, 59, 347 (1984)
			Bendall, JMR, A116, 46 (1995)
    relaxation times:   Kay et al, JMR, 97, 359 (1992)
			Farrow et al, Biochemistry, 33, 5984 (1994)
    TROSY:		Weigelt, JACS, 120, 10778 (1998)
     
    Written by MRB, December 1997, starting with ghn_co from BioPack.
    Revised and improved to a standard format by MRB, BKJ and GG for the 
    BioPack, January 1998, so as to include calculation of the above 
    standard features within the pulse sequence code and associated macro.
    Minor additions during 98, TROSY added Dec 98.   (Version Dec 1998).

    Modified to follow changes by Peter Lukavsky (Stanford) for RNA (May 1999)


        	  CHOICE OF DECOUPLING AND 2D MODES

    Set dm = 'nnn', dmm = 'ccc'
    Set dm2 = 'nny', dmm2 = 'ccg' (or 'ccw', or 'ccp') for N15 decoupling.
    Set dm3='nyn', dmm2='cwc' for 2H decoupling using 4th channel

    Must set phase = 1,2  for States-TPPI acquisition in t1 [N15].
    
    The flag f1180 should be set to 'y' if t1 is to be started at halfdwell
    time. This will give 90, -180 phasing in f1. If it is set to 'n' the 
    phasing should be 0,0 and will still give a perfect baseline.  Thus,
    set f1180='n' for (0,0) in N15.  f1180='y' is ignored if ni=0.



          	  DETAILED INSTRUCTIONS FOR USE OF rna_gNhsqc

         
    1. Obtain a printout of the Philosopy behind the BioPack development,
       and General Instructions using the macro: 
                                      "printon man('BioPack') printoff".
       These Detailed Instructions for rna_gNhsqc may be printed using:
                                      "printon man('rna_gNhsqc') printoff".
             
    2. Apply the setup macro "rna_gNhsqc".  This loads the relevant parameter set
       and also sets ni=0 and phase=1 ready for a 1D spectral check.

    3. Centre H1 frequency on H2O (4.7ppm), C13 frequency on 110ppm, and N15 
       frequency on the amide region (200 ppm).

    4. CALIBRATION OF pw AND pwN:
       calN and calH are multipliers of the first N15 and H1 90 degree pulses
       in the sequence.  They can be used to calibrate the pulses using a 1D
       spectrum.  Setting either calN or calH = 2.0 should give a S/N null (or 
       small dispersion signals) corresponding to the pulses being set at 180
       degrees.  Adjust pwN or pw, respectively, until this occurs.  An array
       of calH or calN = 1.8, 2.0, 2.2 is also convenient to judge the null at
       2.0.  calN and calH are automatically reset to 1 for 2D spectra if you   
       forget to do so.  

    5. CHOICE OF N15 REGION:
       amino='y' gives a spectrum of amino resonances centered on dof=85ppm.
       This is a common usage.                               Set sw1 to 40ppm.

       imino='y' gives a spectrum of imino groups.  dof is shifted
       automatically by the pulse sequence code to 155ppm.  Set sw1 to 25ppm.

    6. Splitting of resonances in the N15 dimension by C13 coupling in C13
       enriched samples can be removed by setting C13refoc='y'.

    7. H2O preservation is achieved according to Kay et al, except that a sinc
       one-lobe selective pulse is used to put H2O back along z.  This is much
       more selective than a hard, Seduce1, or gaussian pulse.  H2O is cycled
       back to z as much as possible during t1, and during the relaxation
       delays for the following relaxation time measurements.

    8. T1 MEASUREMENTS OF NH GROUPS:
       Implemented automatically by setting the flag T1='y'.  An array of 1D
       spectra is obtained by arraying the relaxation time parameter,
       relaxT, to a multiple of 10 milliseconds.  relaxT corresponds
       exactly to the relaxation time of the N15 spins.  The method uses 180
       degree H1 pulses every 5ms during relaxT as according to Kay et al.

       2D spectra should be acquired in a separate run for each of the desired 
       values of relaxT. (maxrelaxT as in point 10 is not used when T1='y'.)

   10. T1rho MEASUREMENTS OF NH GROUPS:
       Implemented by setting the flag T1rho='y' and arraying relaxT as above.
       relaxT corresponds exactly to the relaxation time of the N15 spins.
       Spin lock power is limited to 1.5 kHz for a 600 Mhz spectrometer as
       this delivers about the same sample heating as the T2 method below.
       Increasing this RF (by changing the number 1500 in the DECLARE AND LOAD  
       VARIABLES section of rna_gNhsqc.c) causes substantial sample or coil heating 
       as indicated by deterioration of the lock signal).  Care should be taken
       for relaxT times greater than 0.25 seconds, and times greater than
       0.5 seconds are automatically aborted.  Gives very similar results
       to the T2 method as determined using alphalytic protease.  A dummy
       period of spinlock RF is delivered at the beginning of each pulse
       sequence to ensure constant average sample heating - the code
       determines this from the maximum relaxT you have set in your array of 
       relaxT. The spectrum at relaxT=0 may be erroneous because of insufficient
       dephasing of unlocked spins.

       2D spectra should be acquired in a separate run for each of the desired 
       values of relaxT.  If T1rho measurements are to be made in different
       experiments with different relaxTs as for 2D, including arrays of 
       relaxT's, set the parameter maxrelaxT to the maximum relaxT you have
       set in all experiments - this will ensure constant average sample
       heating in all experiments.  maxrelaxT does not need to be set for a
       single array in a single exp.

   11. T2 MEASUREMENTS OF NH GROUPS:
       Implemented by setting the flag T2='y' and arraying relaxT as above.
       relaxT corresponds exactly to the relaxation time of the N15 spins.
       As for T1rho, care should be taken for relaxT times greater than 0.25
       seconds, and times greater than 0.5 seconds are automatically aborted.
       The method is according to Kay et al, with N15 180's every 625 us, and
       H1 180's every 10 ms.  625us was used instead of approx 500us used by
       Kay et al to reduce sample heating.  An alternative method using the
       same no. of N15 and H1 pulses for all relaxT times (by decreasing the
       625us delays to obtain short relaxT) to test whether large nos. of
       pulses decreases S/N because of pulse imperfection, gave exactly
       equivalent results for alphalytic protease, and was discarded.  Another
       alternative, with waltz H1 decoupling and a single N15 180 at the middle
       of relaxT (credited to Wagner et al by Kay et al), gave substantially
       shorter apparent T2 times, and was discarded.    A dummy period of N15
       180's is delivered at the beginning of each pulse sequence to ensure 
       constant average sample heating.

       2D spectra should be acquired in separate runs with maxrelaxT
       set as for T1rho='y' in point 10.       

   12. NH2 GROUPS:
       A spectrum of NH2 groups, with NH groups cancelled, can be obtained
       with flag NH2only='y'.  This utilises a 1/2J (J=94Hz) period of NH 
       J-coupling evolution added to t1 which cancels NH resonances and 
       inverts NH2 resonances (normal INEPT behaviour).  A 180 degree phase
       shift is added to a N15 90 pulse to provide positive NH2 signals.
       The NH2 resonances will be smaller (say 80%) than when NH2only='n'.

   13. RELAXATION MEASUREMENTS OF NH2 GROUPS:
       NH2 resonances are supressed for the methods described in points 9-11.
       To obtain relaxation times of NH2 groups, set the flag NH2only='y' and
       proceed as for points 9-11.  In these cases the 1/2J delays described
       in points 8 and 12 are changed to 1/4J delays by the pulse sequence
       code.  Although NH groups are completely cancelled in point 12, when 
       measuring relaxation times NH groups appear at 50% intensity. 

   14. The coherence-transfer gradients using power levels
       gzlvl1 and gzlvl2 may be either z or magic-angle gradients. For the
       latter, a proper /vnmr/imaging/gradtable entry must be present and
       syscoil must contain the value of this entry (name of gradtable). The
       amplitude of the gzlvl1 and gzlvl2 should be lower than for a z axis
       probe to have the x and y gradient levels within the 32k range. For
       any value, a dps display (using power display) shows the x,y and z
       dac values. These must be <=32k.

   15. 1/4J DELAY TIMES:
       These are are determined from the NH coupling constant, JNH, listed in 
       dg for possible change by the user. lambda is the 1/4J time for H1 NH 
       coupling evolution, tNH is the 1/4J time for N15 NH coupling evolution.  
       lambda is usually set a little lower than the theoretical time to 
       minimise relaxation, but tNH should be as close to the theoretical time 
       as possible.

   16. TROSY and COUPLED 2D SPECTRA:
       Set TROSY='y' and dm2='nnn' for a TROSY spectrum of the bottom right
       peak of the 2D coupled quartet (high-field H1, low-field N15).  For
       comparison, TROSY='n' and dm2='nnn' gives a 2D spectrum coupled in
       both dimensions.  The TROSY spectrum retains the N15 magnetization as
       well as the H1 magnetization and so gives a little more than 50% S/N
       than decoupled HSQC for a small peptide.  For TROSY, H2O flipback is
       maintained throughout t1 and t2 using two sinc one-lobe pulses (H2O is 
       inverted for half of t1 for normal HSQC, ie TROSY='n' as described in 
       point 7).  To select any of the other three peaks of the 2D coupled
       quartet, in a clockwise direction from bottom right, change t4/t10
       from x/y to x/-y to -x/-y to -x/y.  NOTE, the Varian phases are of
       opposite sense to those in the Weigelt paper, ie swap y and -y.  The
       same phases as written by Weigelt also work provided the +/- gzlvl1
       signs for phase=1,2 are swapped to -/+, and the phase of one of the
       first two H1 90 degree pulses is swapped to ensure that the N15
       magnetization adds rather than subtracts.  The phase list used for
       this sequence code is yet another equivalent which permits the least
       change from the normal SE train in the BioPack sequences.
       Relaxation time measurements have not yet been implemented for TROSY
       spectra (requires customer demand !).

   17. bottom and right FLAGS can be used to check for the sharpest peak:

       Type trace='f1' for display of 2D spectrum
       with bottom='y' and right='y' the bottom-right peak of the coupled
       2D gets selected
       with bottom='y' and right='n' the bottom-left peak of the coupled
       2D gets selected
       etc.
       Best to array in a clockwise direction
	bottom='y','y','n','n'
	right='y','n','n','y'
	array='(bottom,right)'


       if BioPack power limits are in effect (BPpwrlimits=1) the cpmg 15N 
       amplitude is decreased by 3dB and pulse width increased 40%
*/

#include <standard.h>
  

	     
static int   /*  T is for TROSY='y', phx etc also enable TROSY phase changes */
                      phx[1]={0},   phy[1]={1},   ph_x[1]={2},	ph_y[1]={3},

	     phi3[2]  = {0,2},	
             phi9[8]  = {0,0,1,1,2,2,3,3},	
             rec[4]   = {0,2,2,0},		     recT[2]  = {1,3};


static double   d2_init=0.0, relaxTmax;


void pulsesequence()
{

/* DECLARE AND LOAD VARIABLES */

char        f1180[MAXSTR],   		      /* Flag to start t1 @ halfdwell */
            mag_flg[MAXSTR],      /* magic-angle coherence transfer gradients */
	    C13refoc[MAXSTR],		/* C13 sech/tanh pulse in middle of t1*/
	    NH2only[MAXSTR],		       /* spectrum of only NH2 groups */
            amino[MAXSTR],                     /* select amino nitrogens      */
            imino[MAXSTR],                     /* select imino nitrogens      */
	    T1[MAXSTR],				/* insert T1 relaxation delay */
	    T1rho[MAXSTR],		     /* insert T1rho relaxation delay */
	    T2[MAXSTR],				/* insert T2 relaxation delay */
	    bottom[MAXSTR],
	    right[MAXSTR],
	    TROSY[MAXSTR];			    /* do TROSY on N15 and H1 */
 
int         icosel,          			  /* used to get n and p type */
            t1_counter,  		        /* used for states tppi in t1 */
	    rTnum,			/* number of relaxation times, relaxT */
	    rTcounter;		    /* to obtain maximum relaxT, ie relaxTmax */

double      tau1,         				         /*  t1 delay */
	    lambda = 0.91/(4.0*getval("JNH")), 	   /* 1/4J H1 evolution delay */
	    tNH = 1.0/(4.0*getval("JNH")),	  /* 1/4J N15 evolution delay */
	    relaxT = getval("relaxT"),		     /* total relaxation time */
	    rTarray[1000], 	    /* to obtain maximum relaxT, ie relaxTmax */
            maxrelaxT = getval("maxrelaxT"),    /* maximum relaxT in all exps */
	    ncyc,			 /* number of pulsed cycles in relaxT */
        
/* the sech/tanh pulse is automatically calculated by the macro "rna_cal", */  /* and is called directly from your shapelib.                  		      */
   pwClvl = getval("pwClvl"), 	  	        /* coarse power for C13 pulse */
   pwC = getval("pwC"),     	      /* C13 90 degree pulse length at pwClvl */
   rfC,            	          /* maximum fine power when using pwC pulses */
   rfst,	                       /* fine power for the rna_stC140 pulse */

   compH = getval("compH"),        /* adjustment for H1 amplifier compression */
   compN = getval("compN"),       /* adjustment for N15 amplifier compression */
   compC = getval("compC"),       /* adjustment for C13 amplifier compression */

	calH = getval("calH"), /* multiplier on a pw pulse for H1 calibration */
   	pwHs = getval("pwHs"),	        /* H1 90 degree pulse length at tpwrs */
   	tpwrs,	  	              /* power for the pwHs ("H2Osinc") pulse */

	pwNlvl = getval("pwNlvl"),	              /* power for N15 pulses */
        pwN = getval("pwN"),          /* N15 90 degree pulse length at pwNlvl */
	calN = getval("calN"),   /* multiplier on a pwN pulse for calibration */
	slNlvl,					   /* power for N15 spin lock */
        slNrf = 1500.0,        /* RF field in Hz for N15 spin lock at 600 MHz */
        dof2a,                                      /* offset for imino/amino */

	sw1 = getval("sw1"),

	gt1 = getval("gt1"),  		       /* coherence pathway gradients */
        gzcal = getval("gzcal"),               /* dac to G/cm conversion      */
	gzlvl1 = getval("gzlvl1"),
	gzlvl2 = getval("gzlvl2"),

            BPpwrlimits,                    /*  =0 for no limit, =1 for limit */

	gt0 = getval("gt0"),				   /* other gradients */
	gt3 = getval("gt3"),
	gt4 = getval("gt4"),
	gt5 = getval("gt5"),
	gzlvl0 = getval("gzlvl0"),
	gzlvl3 = getval("gzlvl3"),
	gzlvl4 = getval("gzlvl4"),
	gzlvl5 = getval("gzlvl5");
   P_getreal(GLOBAL,"BPpwrlimits",&BPpwrlimits,1);

    getstr("f1180",f1180);
    getstr("mag_flg",mag_flg);
    getstr("C13refoc",C13refoc);
    getstr("NH2only",NH2only);
    getstr("T1",T1);
    getstr("T1rho",T1rho);
    getstr("T2",T2);
    getstr("bottom",bottom);
    getstr("right",right);
    getstr("TROSY",TROSY);
    getstr("imino",imino);
    getstr("amino",amino);



/*   LOAD PHASE TABLE    */
	
        settable(t3,2,phi3);
   if (TROSY[A]=='y')
       {settable(t1,1,ph_x);

	if (bottom[A]=='y')
	settable(t4,1,phx);
	else
	settable(t4,1,ph_x);
        if (right[A]=='y')
 	settable(t10,1,phy);
	else
        settable(t10,1,ph_y);

	settable(t9,1,phx);
	settable(t11,1,phx);
	settable(t12,2,recT);}
    else
       {settable(t1,1,phx);
	settable(t4,1,phx);
	settable(t9,8,phi9);
 	settable(t10,1,phx);
	settable(t11,1,phy);
	settable(t12,4,rec);}



/*   INITIALIZE VARIABLES   */
dof2a=dof2;
/* IMINO-region setting of dof2 */
      if (imino[A] == 'y') dof2a=dof2-45*dfrq2;
      if (amino[A] == 'y') dof2a=dof2-115*dfrq2;
  if ((imino[A] == 'n') && (amino[A] == 'n')) dof2a=dof2;

/* maximum fine power for pwC pulses (and initialize rfst) */
	rfC = 4095.0;    rfst=0.0;

/* 180 degree adiabatic C13 pulse covers 140 ppm */
     if (C13refoc[A]=='y')
       {rfst = (compC*4095.0*pwC*4000.0*sqrt((21.0*sfrq/600.0+7.0)/0.35));   
	rfst = (int) (rfst + 0.5);
	if ( 1.0/(4000.0*sqrt((21.0*sfrq/600.0+7.0)/0.35)) < pwC )
           { text_error( " Not enough C13 RF. pwC must be %f usec or less.\n", 
	    (1.0e6/(4000.0*sqrt((21.0*sfrq/600.0+7.0)/0.35))) ); psg_abort(1); }}

/* selective H20 one-lobe sinc pulse */
    tpwrs = tpwr - 20.0*log10(pwHs/(compH*pw*1.69));   /*needs 1.69 times more*/
    tpwrs = (int) (tpwrs);                   	  /*power than a square pulse */

/* power level for N15 spinlock (90 degree pulse length calculated first) */
	slNlvl = 1/(4.0*slNrf*sfrq/600.0) ;
	slNlvl = pwNlvl - 20.0*log10(slNlvl/(pwN*compN));
	slNlvl = (int) (slNlvl + 0.5);

/* use 1/8J times for relaxation measurements of NH2 groups */
  if ( (NH2only[A]=='y') && ((T1[A]=='y') || (T1rho[A]=='y') || (T2[A]=='y')) )	
     {  tNH = tNH/2.0;  }

/* reset calH and calN for 2D if inadvertently left at 2.0 */
  if (ni>1.0) {calH=1.0; calN=1.0;}



/* CHECK VALIDITY OF PARAMETER RANGES */

  if ( ((imino[A] == 'y') && (amino[A] == 'y')) )
   {
    printf(" Choose ONE of the cases: imino='y' OR amino='y' ");
    psg_abort(1);
   }

  if ( ((imino[A] == 'y') && (NH2only[A] == 'y')) )
   {
    printf(" NH2only='y' only valide for  amino='y' ");
    psg_abort(1);
   }

  if ((TROSY[A]=='y') && (gt1 < -2.0e-4 + pwHs + 1.0e-4 + 2.0*POWER_DELAY))
  { text_error( " gt1 is too small. Make gt1 equal to %f or more.\n",    
    (-2.0e-4 + pwHs + 1.0e-4 + 2.0*POWER_DELAY) ); psg_abort(1); }

  if((dm[A] == 'y' || dm[B] == 'y' || dm[C] == 'y' ))
  { text_error("incorrect dec1 decoupler flags! Should be 'nnn' "); psg_abort(1); }

  if((dm2[A] == 'y' || dm2[B] == 'y'))
  { text_error("incorrect dec2 decoupler flags! Should be 'nny' "); psg_abort(1); }

  if( dpwr2 > 50 )
  { text_error("don't fry the probe, DPWR2 too large!  "); psg_abort(1); }

  if( pw > 50.0e-6 )
  { text_error("dont fry the probe, pw too high ! "); psg_abort(1); } 
  
  if( (pwN > 100.0e-6) && (pwNlvl > 54) )
  { text_error("dont fry the probe, pwN too high ! "); psg_abort(1); }



/*  RELAXATION TIMES AND FLAGS */  

/* evaluate maximum relaxT, relaxTmax chosen by the user */
  rTnum = getarray("relaxT", rTarray);
  relaxTmax = rTarray[0];
  for (rTcounter=1; rTcounter<rTnum; rTcounter++)
      if (relaxTmax < rTarray[rTcounter]) relaxTmax = rTarray[rTcounter];


/* compare relaxTmax with maxrelaxT */
  if (maxrelaxT > relaxTmax)  relaxTmax = maxrelaxT; 




if ( ((T1rho[A]=='y') || (T2[A]=='y')) && (relaxTmax > d1) )
{ text_error("Maximum relaxation time, relaxT, is greater than d1 ! "); psg_abort(1);}

if ( ((T1[A]=='y') && (T1rho[A]=='y'))   ||   ((T1[A]=='y') && (T2[A]=='y')) ||
    ((T1rho[A]=='y') && (T2[A]=='y')) )
{ text_error("Choose only one relaxation measurement ! "); psg_abort(1); } 


if ( ((T1[A]=='y') || (T1rho[A]=='y')) && 
       ((relaxT*100.0 - (int)(relaxT*100.0+1.0e-4)) > 1.0e-6) )
 { text_error("Relaxation time, relaxT, must be zero or multiple of 10msec"); psg_abort(1);}
 

 if ( (T2[A]=='y') && 
           (((relaxT+0.01)*50.0 - (int)((relaxT+0.01)*50.0+1.0e-4)) > 1.0e-6) )
{ text_error("Relaxation time, relaxT, must be odd multiple of 10msec"); psg_abort(1);}

if ( ((T1rho[A]=='y') || (T2[A]=='y'))  &&  (relaxTmax > 0.25) && (ix==1) ) 
{ printf("WARNING, sample heating may result in a reduced lock level for relaxT>0.25sec");}


if ( ((T1rho[A]=='y') ||  (T2[A]=='y'))  &&  (relaxTmax > 0.5) ) 
{ text_error("relaxT greater than 0.5 seconds will heat sample"); psg_abort(1);}


if ( ((NH2only[A]=='y') || (T1[A]=='y') || (T1rho[A]=='y') || (T2[A]=='y'))
   &&  (TROSY[A]=='y') ) 
{ text_error("TROSY not implemented with NH2 spectrum, or relaxation exps."); psg_abort(1);} 


if ((TROSY[A]=='y') && (dm2[C] == 'y'))
{ text_error("Choose either TROSY='n' or dm2='nnn' ! "); psg_abort(1); }

 

/* PHASES AND INCREMENTED TIMES */

/*  Phase incrementation for hypercomplex 2D data, States-Haberkorn element */

    if (TROSY[A]=='y')
	 {  if (phase1 == 1)   				      icosel = -1;
            else 	  {  tsadd(t4,2,4);  tsadd(t10,2,4);  icosel = +1;  }
	 }
    else {  if (phase1 == 1)  {tsadd(t10,2,4); icosel = +1;}
            else 			       icosel = -1;    
	 }


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



/*  Correct inverted signals for NH2 only spectra  */

   if ((NH2only[A]=='y') && (T1[A]=='n')  &&  (T1rho[A]=='n')  && (T2[A]=='n'))
      { tsadd(t3,2,4); }



/* BEGIN PULSE SEQUENCE */

status(A);

	obspower(tpwr);
	decpower(pwClvl);
	decpwrf(rfC);
 	dec2power(pwNlvl);
        dec2offset(dof2a);
	txphase(zero);
        decphase(zero);
        dec2phase(zero);

	delay(d1);

 
/*  xxxxxxxxxxxxxxxxx  CONSTANT SAMPLE HEATING FROM N15 RF xxxxxxxxxxxxxxxxx  */

 if  (T1rho[A]=='y')
 	{dec2power(slNlvl);
         dec2rgpulse(relaxTmax-relaxT, zero, 0.0, 0.0);
    	 dec2power(pwNlvl);}


	
 if  (T2[A]=='y')      
 	{ncyc = 8.0*100.0*(relaxTmax - relaxT);
         if (BPpwrlimits > 0.5)
          {
           dec2power(pwNlvl-3.0);    /* reduce for probe protection */
           pwN=pwN*compN*1.4;
          }
    	 if (ncyc > 0)
       	    {initval(ncyc,v1);
             loop(v1,v2);
       	     delay(0.625e-3 - pwN);
      	     dec2rgpulse(2*pwN, zero, 0.0, 0.0);
      	     delay(0.625e-3 - pwN);
            endloop(v2);}
         if (BPpwrlimits > 0.5)
          {
           dec2power(pwNlvl);         /* restore normal value */
           pwN=getval("pwN");
          }
 	}
/*  xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx  */
        rcvroff();
	if (TROSY[A]=='n')    dec2rgpulse(pwN, zero, 0.0, 0.0);  
	decrgpulse(pwC, zero, 0.0, 0.0);   /*destroy N15 and C13 magnetization*/
	zgradpulse(gzlvl0, 0.5e-3);
	delay(1.0e-4);
	if (TROSY[A]=='n')    dec2rgpulse(pwN, one, 0.0, 0.0);
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
   	obspower(tpwrs);
   	shaped_pulse("rna_H2Osinc", pwHs, two, 5.0e-5, 0.0);
	obspower(tpwr);
	zgradpulse(gzlvl3, gt3);
	dec2phase(t3);
	delay(2.0e-4);
   	dec2rgpulse(calN*pwN, t3, 0.0, 0.0);
	txphase(zero);
	decphase(zero);

/*  xxxxxxxxxxxxxxxxxx    OPTIONS FOR N15 RELAXATION    xxxxxxxxxxxxxxxxxxxx  */

if ( (T1[A]=='y') || (T1rho[A]=='y') || (T2[A]=='y') )
   {
    dec2phase(one);
    zgradpulse(gzlvl4, gt4);				/* 2.0*GRADIENT_DELAY */
    delay(tNH - gt4 - 2.0*GRADIENT_DELAY);

    sim3pulse(2.0*pw, 0.0, 2.0*pwN, zero, zero, one, 0.0, 0.0);

    zgradpulse(gzlvl4, gt4);				/* 2.0*GRADIENT_DELAY */
    delay(tNH - gt4 - 2.0*GRADIENT_DELAY);
   }

		/*   xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx    */

if  (T1[A]=='y')
   {
    dec2rgpulse(pwN, one, 0.0, 0.0);
    dec2phase(three);

    zgradpulse(gzlvl0, gt0);				/* 2.0*GRADIENT_DELAY */
    delay(2.5e-3 - gt0 - 2.0*GRADIENT_DELAY - pw);
    rgpulse(2.0*pw, zero, 0.0, 0.0);
    delay(2.5e-3 - pw);

    ncyc = (100.0*relaxT);
    initval(ncyc,v4);
    if (ncyc > 0)
	{loop(v4,v5);

	 delay(2.5e-3 - pw);
    	 rgpulse(2.0*pw, two, 0.0, 0.0);
   	 delay(2.5e-3 - pw);

	 delay(2.5e-3 - pw);
    	 rgpulse(2.0*pw, zero, 0.0, 0.0);
   	 delay(2.5e-3 - pw);

	 endloop(v5);}

    dec2rgpulse(pwN, three, 0.0, 0.0);
   }

		/*   xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx    */

			     /* Theory suggests 8.0 is better than 2PI as RF  */
			     /* field multiplier and experiment confirms this.*/
if  (T1rho[A]=='y')          /* Shift evolution of 2.0*pwN/PI for one pulse   */
   {		             /* at end left unrefocused as for normal sequence*/
    delay(1.0/(8.0*slNrf) - pwN);
    decrgpulse(pwN, zero, 0.0, 0.0);
    dec2power(slNlvl);
           				   /* minimum 5ms spinlock to dephase */
    dec2rgpulse((2.5e-3-pw), zero, 0.0, 0.0);	         /*  spins not locked */
    sim3pulse(2.0*pw, 0.0, 2.0*pw, zero, zero, zero, 0.0, 0.0);
    dec2rgpulse((2.5e-3-pw), zero, 0.0, 0.0);

    ncyc = 100.0*relaxT;
    initval(ncyc,v4);	    if (ncyc > 0)
	  {loop(v4,v5);
           dec2rgpulse((2.5e-3-pw), zero, 0.0, 0.0);
   	   sim3pulse(2.0*pw, 0.0, 2.0*pw, two, zero, zero, 0.0, 0.0);
           dec2rgpulse((2.5e-3-pw), zero, 0.0, 0.0);
           dec2rgpulse((2.5e-3-pw), zero, 0.0, 0.0);
   	   sim3pulse(2.0*pw, 0.0, 2.0*pw, zero, zero, zero, 0.0, 0.0);
           dec2rgpulse((2.5e-3-pw), zero, 0.0, 0.0);
           endloop(v5);} 

    dec2power(pwNlvl);	
    decrgpulse(pwN, zero, 0.0, 0.0);
    delay(1.0/(8.0*slNrf) + 2.0*pwN/PI - pwN);
   }
		/*   xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx    */

if  (T2[A]=='y')
   {
    dec2phase(zero);
    initval(0.0,v3);   initval(180.0,v4);
    if (BPpwrlimits > 0.5)
     {
      dec2power(pwNlvl-3.0);    /* reduce for probe protection */
      pwN=pwN*compN*1.4;
     }

    ncyc = 100.0*relaxT;
    initval(ncyc,v5);

    loop(v5,v6);

      initval(3.0,v7);
      loop(v7,v8);
       	delay(0.625e-3 - pwN);
      	dec2rgpulse(2.0*pwN, zero, 0.0, 0.0);
      	delay(0.625e-3 - pwN);
      endloop(v8);

      delay(0.625e-3 - pwN - SAPS_DELAY);
      add(v4,v3,v3);  obsstepsize(1.0);  xmtrphase(v3);	   	/* SAPS_DELAY */
      dec2rgpulse(2.0*pwN, zero, 0.0, 0.0);
      delay(0.625e-3 - pwN - pw);

      rgpulse(2*pw, zero, 0.0, 0.0);

      delay(0.625e-3 - pwN - pw );
      dec2rgpulse(2.0*pwN, zero, 0.0, 0.0);
      xmtrphase(zero);						/* SAPS_DELAY */
      delay(0.625e-3 - pwN - SAPS_DELAY);
  
      initval(3.0,v9);
      loop(v9,v10);
      	delay(0.625e-3 - pwN);
      	dec2rgpulse(2.0*pwN, zero, 0.0, 0.0);
      	delay(0.625e-3 - pwN);
      endloop(v10);

    endloop(v6);
    if (BPpwrlimits > 0.5)
     {
      dec2power(pwNlvl);    /* restore normal value */
      pwN=getval("pwN");
     }
   }

/*  xxxxxxxxxxxxxxxxxx    OPTIONS FOR N15 EVOLUTION    xxxxxxxxxxxxxxxxxxxxx  */
	txphase(zero);
	dec2phase(t9);

if ( (NH2only[A]=='y') || (T1[A]=='y') || (T1rho[A]=='y') || (T2[A]=='y') )	
{      
    	delay(tau1);
         			  /* optional sech/tanh pulse in middle of t1 */
    	if (C13refoc[A]=='y') 				   /* WFG_START_DELAY */
           {decshaped_pulse("rna_stC140", 1.0e-3, zero, 0.0, 0.0);
            delay(tNH - 1.0e-3 - WFG_START_DELAY - 2.0*pw);}
    	else
           {delay(tNH - 2.0*pw);}
    	rgpulse(2.0*pw, zero, 0.0, 0.0);
    	if (tNH < gt1 + 1.99e-4)  delay(gt1 + 1.99e-4 - tNH);

    	delay(tau1);

    	dec2rgpulse(2.0*pwN, t9, 0.0, 0.0);

        if (mag_flg[A] == 'y')  magradpulse(gzcal*gzlvl1, gt1);
        else  zgradpulse(gzlvl1, gt1);   	/* 2.0*GRADIENT_DELAY */
	txphase(t4);
    	dec2phase(t10);
   	if (tNH > gt1 + 1.99e-4)  delay(tNH - gt1 - 2.0*GRADIENT_DELAY);
   	else   delay(1.99e-4 - 2.0*GRADIENT_DELAY);
}

else if (TROSY[A]=='y')
{
  	if ( (C13refoc[A]=='y') && (tau1 > 0.5e-3 + WFG2_START_DELAY) )
           {delay(tau1 - 0.5e-3 - WFG2_START_DELAY);     /* WFG2_START_DELAY */
            decshaped_pulse("rna_stC140", 1.0e-3, zero, 0.0, 0.0);
            delay(tau1 - 0.5e-3);}
	else    delay(2.0*tau1);

        if (mag_flg[A] == 'y')  magradpulse(gzcal*gzlvl1, gt1);
        else  zgradpulse(gzlvl1, gt1);   	/* 2.0*GRADIENT_DELAY */
	delay(2.0e-4 - 2.0*GRADIENT_DELAY);

	dec2rgpulse(2.0*pwN, t9, 0.0, 0.0);

        delay(gt1 + 2.0e-4 - pwHs - 1.0e-4 - 2.0*POWER_DELAY);
	txphase(three);
   	obspower(tpwrs);				       /* POWER_DELAY */
   	shaped_pulse("H2Osinc", pwHs, three, 5.0e-5, 0.0);
	txphase(t4);
	obspower(tpwr);					       /* POWER_DELAY */
	delay(5.0e-5);
}

else
{					  	    /* fully-coupled spectrum */
        if (dm2[C]=='n')  {rgpulse(2.0*pw, zero, 0.0, 0.0);  pw=0.0;}		

  	if ( (C13refoc[A]=='y') && (tau1 > 0.5e-3 + WFG2_START_DELAY) )
           {delay(tau1 - 0.5e-3 - WFG2_START_DELAY);     /* WFG2_START_DELAY */
            simshaped_pulse("", "rna_stC140", 2.0*pw, 1.0e-3, zero, zero, 0.0, 0.0);
            delay(tau1 - 0.5e-3);
            delay(gt1 + 2.0e-4);}
	else
           {delay(tau1);
            rgpulse(2.0*pw, zero, 0.0, 0.0);
            delay(gt1 + 2.0e-4 - 2.0*pw);
            delay(tau1);} 
 
	pw=getval("pw");
	dec2rgpulse(2.0*pwN, t9, 0.0, 0.0);
        decphase(zero);

        if (mag_flg[A] == 'y')  magradpulse(gzcal*gzlvl1, gt1);
        else  zgradpulse(gzlvl1, gt1);   	/* 2.0*GRADIENT_DELAY */
	txphase(t4);
	dec2phase(t10);
	delay(2.0e-4 - 2.0*GRADIENT_DELAY);
}

	if  (T1rho[A]=='y')   delay(POWER_DELAY); 


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
	zgradpulse(1.5*gzlvl5, gt5);
	delay(lambda - 1.3*pwN - gt5);

	sim3pulse(2.0*pw, 0.0, 2.0*pwN, zero, zero, zero, 0.0, 0.0);

	dec2phase(t10);
	zgradpulse(1.5*gzlvl5, gt5);
	if (TROSY[A]=='y')   delay(lambda - 1.6*pwN - gt5);
	else   delay(lambda - 0.65*pwN - gt5);

	if (TROSY[A]=='y')   dec2rgpulse(pwN, t10, 0.0, 0.0); 
	else    	     rgpulse(pw, zero, 0.0, 0.0); 

	delay((gt1/10.0) + 1.0e-4 - 0.65*pw + 2.0*GRADIENT_DELAY + POWER_DELAY);

  if (dm3[B] == 'y')			         /*optional 2H decoupling off */
        { dec3rgpulse(1/dmf3, three, 0.0, 0.0); dec3blank();
          setstatus(DEC3ch, FALSE, 'w', FALSE, dmf3);
          dec3blank(); }

	rgpulse(2.0*pw, zero, 0.0, rof1);

	dec2power(dpwr2);				       /* POWER_DELAY */
        if (mag_flg[A] == 'y')	  magradpulse(icosel*gzcal*gzlvl2, 0.1*gt1);
        else   zgradpulse(icosel*gzlvl2, 0.1*gt1);		/* 2.0*GRADIENT_DELAY */
        rcvron();
statusdelay(C,1.0e-4-rof1);		

  if (dm3[B] == 'y') {delay(1/dmf3); lk_sample();}

	setreceiver(t12);
}		 
