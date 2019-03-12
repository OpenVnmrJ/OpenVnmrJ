/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*  gNhsqcHD.c

    HSQC gradient sensitivity enhanced version for N15, with options for
    HH-homodecoupling, TROSY and for T1, T1rho, and T2 relaxation measurements
    of the N15 nuclei. Coherence transfer gradients may be z or magic-angle 
    (using a triax probe).


    Standard features include optional 13C sech/tanh pulse on CO and Ca to 
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
    HH Homo-decoupling: Kupce and Wagner, JMR, B109 329 (1995)
     
    Written by MRB, December 1997, starting with ghn_co from BioPack.
    Revised and improved to a standard format by MRB, BKJ and GG for the 
    BioPack, January 1998, so as to include calculation of the above 
    standard features within the pulse sequence code and associated macro.
    Minor additions during 98, TROSY added Dec 98.   (Version Dec 1998).
    HH homo-decoupling option added. (EK, May '02)

    Modified the amplitude of the flipback pulse(s) (pwHs) to permit user adjustment around
    theoretical value (tpwrs). If tpwrsf < 4095.0 the value of tpwrs is increased 6db and
    values of tpwrsf of 2048 or so should give equivalent amplitude. In cases of severe
    radiation damping( which happens during pwHs) the needed flip angle may be much less than
    90 degrees, so tpwrsf should be lowered to a value to minimize the observed H2O signal in 
    single-scan experiments (with ssfilter='n').(GG jan01)

    F1 frequency axis reversed in order to make f1coef consistent with the standard settings.
    Use f1coef = '1 0 -1 0 0 1 0 1'. (EK, May '02).


        	  CHOICE OF DECOUPLING AND 2D MODES

    Set dm = 'nnn', dmm = 'ccc'
    Set dm2 = 'nny', dmm2 = 'ccg' (or 'ccw', or 'ccp') for N15 decoupling.
    Set dm3='nyn', dmm2='cwc' for 2H decoupling using 4th channel

    Must set phase = 1,2  for States-TPPI acquisition in t1 [N15].
    
    The flag f1180 should be set to 'y' if t1 is to be started at halfdwell
    time. This will give 90, -180 phasing in f1. If it is set to 'n' the 
    phasing should be 0,0 and will still give a perfect baseline.  Thus,
    set f1180='n' for (0,0) in N15.  f1180='y' is ignored if ni=0.



          	  DETAILED INSTRUCTIONS FOR USE OF gNhsqc

         
    1. Obtain a printout of the Philosopy behind the BioPack development,
       and General Instructions using the macro: 
                                      "printon man('BioPack') printoff".
       These Detailed Instructions for gNhsqc may be printed using:
                                      "printon man('gNhsqc') printoff".
             
    2. Apply the setup macro "gNhsqc".  This loads the relevant parameter set
       and also sets ni=0 and phase=1 ready for a 1D spectral check.

    3. Centre H1 frequency on H2O (4.7ppm), C13 frequency on 100ppm, and N15 
       frequency on the amide region (120 ppm).

    4. CALIBRATION OF pw AND pwN:
       calN and calH are multipliers of the first N15 and H1 90 degree pulses
       in the sequence.  They can be used to calibrate the pulses using a 1D
       spectrum.  Setting either calN or calH = 2.0 should give a S/N null (or 
       small dispersion signals) corresponding to the pulses being set at 180
       degrees.  Adjust pwN or pw, respectively, until this occurs.  An array
       of calH or calN = 1.8, 2.0, 2.2 is also convenient to judge the null at
       2.0.  calN and calH are automatically reset to 1 for 2D spectra if you   
       forget to do so.  (Note that compH and compN can be calibrated using
       calH and calN in an analogous way to compC as in point 6, gChsqc
       manual).

    5. CALIBRATION OF dof2:  
       Set ni=ni2=0 and phase=phase2=1 and obtain a well=phased absorption mod
       1D spectrum. Set d2=0.0001 and phase=1,2, sum the two resulting spectra
       and display in exp5 with a 90 degree phase shift:
       clradd select(1) spadd select(2) spadd jexp5 full rp=rp+90 ds   
       dof2 will be in the middle of the NH region when the signals between
       7.4 and 9ppm in the 1H spectrum are roughly balanced between +ve and
       -ve signals.  Note that the same method with d2=0 should give a zero 
       spectrum.  Don't forget to reset d2=0 when you have finished.

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
       An alternative method using waltz decoupling was tested and discarded.
       It gave exactly the same results using alphalytic protease, but has 
       the comparative disadvantage of delivering more H1 RF, so more heating.

       2D spectra should be acquired in a separate run for each of the desired 
       values of relaxT. (maxrelaxT as in point 10 is not used when T1='y'.)

   10. T1rho MEASUREMENTS OF NH GROUPS:
       Implemented by setting the flag T1rho='y' and arraying relaxT as above.
       relaxT corresponds exactly to the relaxation time of the N15 spins.
       Spin lock power is limited to 1.5 kHz for a 600 Mhz spectrometer as
       this delivers about the same sample heating as the T2 method below.
       Increasing this RF (by changing the number 1500 in the DECLARE AND LOAD  
       VARIABLES section of gNhsqc.c) causes substantial sample or coil heating 
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
       spectra.

   17. Radiation Damping:
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

   18. HH homo-decoupling:
       Activated by setting the Hdecflg='y'. The dsp flag should be set to 'n'.
       The decoupling waveform is created automatically at the 'go' (or 'dps') time.
       Note - complete de-phasing of water is the key requirement for obtaining good
       quality spectra with HH homo-decoupling !!! This may reduce the overall sensitivity 
       in proteins with fast exchanging NH groups. Note also that the selective 
       pulses in watergate are not affected by Radiation Damping.  
       The residual signal from water is eliminated by phase cycling and hence the
       spectra will improve with increasing ss and nt.                   
       The power level for HH decoupling can be changed by setting Hdecpwr
       to a non-zero value. If Hdecpwr=0 the Pbox-calculated value will be used. 

       Due to the Bloch-Siegert effects the alfa delay can be relatively large and 
       negative.

       The effect from HH homo-decoupling is best appreciated, if the HH coupling
       is resolved. This usually requires a fairly long acquisition time (at >= 0.1s)
       a good digital resolution (fn >= 4k) and a sensible window function. For the
       purpose of comparison the HH decoupling can be switched off by Hdecflg='off'.

   19. 13C decoupling during acquisition:
       Activated by setting the Cdecflg='y'.
       The decoupling waveform is created automatically at the 'go' (or 'dps') time.
       Recommended Cdecflg='y' - use low power WURST-40 decoupling for long range 
       C-H couplings. 
       The power level for CH decoupling can be changed by setting Cdecpwr
       to a non-zero value. If Cdecpwr=0, the Pbox-calculated value will be used.

       The resolution is improved by using C-13 decoupling during acquisition. 
       This is best achieved using low-power WURST-40 decoupling designed for long 
       range CH decoupling (set Cdecflg = 'y'). 



       if BioPack power limits are in effect (BPpwrlimits=1) the cpmg 15N 
       amplitude is decreased by 3dB and pulse width increased 40%
*/


#include <standard.h>
#include "Pbox_psg.h"

	     
static int   /*  T is for TROSY='y', phx etc also enable TROSY phase changes */
                  phx[1]={0},   phy[1]={1}, ph_x[1]={2},

	     phi3[2]  = {0,2},	
             phi9[8]  = {0,0,1,1,2,2,3,3},	
             rec[4]   = {0,2,2,0},		     recT[2]  = {1,3};


static double   d2_init=0.0, relaxTmax;

shape   HHdseq, Cdseq;

void pulsesequence()
{

/* DECLARE AND LOAD VARIABLES */

void        makeHHdec(), makeCdec(); 	                /* utility functions */

char        f1180[MAXSTR],   		      /* Flag to start t1 @ halfdwell */
            mag_flg[MAXSTR],      /* magic-angle coherence transfer gradients */
	    C13refoc[MAXSTR],		/* C13 sech/tanh pulse in middle of t1*/
	    NH2only[MAXSTR],		       /* spectrum of only NH2 groups */
	    T1[MAXSTR],				/* insert T1 relaxation delay */
	    T1rho[MAXSTR],		     /* insert T1rho relaxation delay */
	    T2[MAXSTR],				/* insert T2 relaxation delay */
	    TROSY[MAXSTR],			    /* do TROSY on N15 and H1 */
	    Hdecflg[MAXSTR],                       /* HH-homo decoupling flag */
	    Cdecflg[MAXSTR];                /* low power C-13 decoupling flag */
 
int         icosel,          			  /* used to get n and p type */
            ihh=1,       /* used in HH decouling to improve water suppression */
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
            pwr_dly,                 /* power delay */
        
/* the sech/tanh pulse is automatically calculated by the macro "proteincal", */  
/* and is called directly from your shapelib.                  		      */
   pwClvl = getval("pwClvl"), 	  	        /* coarse power for C13 pulse */
   pwC = getval("pwC"),     	      /* C13 90 degree pulse length at pwClvl */
   rf0,            	          /* maximum fine power when using pwC pulses */
   rfst,	                           /* fine power for the stCall pulse */

   compH = getval("compH"),        /* adjustment for H1 amplifier compression */
   compN = getval("compN"),       /* adjustment for N15 amplifier compression */
   compC = getval("compC"),       /* adjustment for C13 amplifier compression */

	calH = getval("calH"), /* multiplier on a pw pulse for H1 calibration */
   	tpwrsf = getval("tpwrsf"),    /* fine power adustment for soft pulse  */
   	pwHs = getval("pwHs"),	        /* H1 90 degree pulse length at tpwrs */
   	pwHH = 0.0,                     /* pwHH = pwHs for HH homo-decoupling */
   	tpwrs,	  	              /* power for the pwHs ("H2Osinc") pulse */

	pwNlvl = getval("pwNlvl"),	              /* power for N15 pulses */
        pwN = getval("pwN"),          /* N15 90 degree pulse length at pwNlvl */
	calN = getval("calN"),   /* multiplier on a pwN pulse for calibration */
	slNlvl,					   /* power for N15 spin lock */
        slNrf = 1500.0,        /* RF field in Hz for N15 spin lock at 600 MHz */

	sw1 = getval("sw1"),

	gt1 = getval("gt1"),  		       /* coherence pathway gradients */
        gzcal = getval("gzcal"),               /* dac to G/cm conversion      */
	gzlvl1 = getval("gzlvl1"),
	gzlvl2 = getval("gzlvl2"),
        BPpwrlimits,                        /*  =0 for no limit, =1 for limit */

	gt0 = getval("gt0"),				   /* other gradients */
	gt3 = getval("gt3"),
	gt4 = getval("gt4"),
	gt5 = getval("gt5"),
	gstab = getval("gstab"),
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
    getstr("TROSY",TROSY);
    getstr("Hdecflg", Hdecflg);
    getstr("Cdecflg", Cdecflg);

/*   LOAD PHASE TABLE    */
	
        settable(t3,2,phi3);
	settable(t4,1,phx);
   if (TROSY[A]=='y')
       {settable(t1,1,ph_x);
	settable(t9,1,phx);
 	settable(t10,1,phy);
	settable(t11,1,phx);
	settable(t12,2,recT);}
    else
       {settable(t1,1,phx);
	settable(t9,8,phi9);
 	settable(t10,1,phx);
	settable(t11,1,phy);
	settable(t12,4,rec);}



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
    if(pwHs > 1e-6)
      tpwrs = tpwr - 20.0*log10(pwHs/(compH*pw*1.69));  /* needs 1.69 times more */
    else                    	                    /* power than a square pulse */
      tpwrs = 0.0;
    tpwrs = (int) (tpwrs);    
    if (tpwrsf<4095.0) tpwrs = tpwrs + 6.0;
    if (tpwrsf < 4095.0) 
    {
      tpwrs = tpwrs + 6.0;   
      pwr_dly = POWER_DELAY + PWRF_DELAY;
    }
    else pwr_dly = POWER_DELAY;

/* power level for N15 spinlock (90 degree pulse length calculated first) */
	slNlvl = 1/(4.0*slNrf*sfrq/600.0) ;
	slNlvl = pwNlvl - 20.0*log10(slNlvl/(pwN*compN));
	slNlvl = (int) (slNlvl + 0.5);

/* use 1/8J times for relaxation measurements of NH2 groups */
  if ( (NH2only[A]=='y') && ((T1[A]=='y') || (T1rho[A]=='y') || (T2[A]=='y')) )	
     {  tNH = tNH/2.0;  }

/* reset calH and calN for 2D if inadvertently left at 2.0 */
  if (ni>1.0) {calH=1.0; calN=1.0;}

/* make shapes and set up parameters for HH homo-decoupling */
    if(Cdecflg[0] == 'y') makeCdec();
    if(Hdecflg[0] == 'y') makeHHdec();
    if(Hdecflg[0] != 'n')
    { 
      pwHH = pwHs; 
      pwHs = 0.0; 
    }


/* CHECK VALIDITY OF PARAMETER RANGES */

  if ((TROSY[A]=='y') && (gt1 < -2.0e-4 + pwHs + 1.0e-4 + 2.0*POWER_DELAY))
  { text_error( " gt1 is too small. Make gt1 equal to %f or more.\n",    
    (-2.0e-4 + pwHs + 1.0e-4 + 2.0*POWER_DELAY) ); psg_abort(1); }

  if((dm[A] == 'y' || dm[B] == 'y' || dm[C] == 'y' ))
  { text_error("incorrect dec1 decoupler flags! Should be 'nnn' "); psg_abort(1); }

  if((dm2[A] == 'y' || dm2[B] == 'y'))
  { text_error("incorrect dec2 decoupler flags! Should be 'nny' "); psg_abort(1); }

  if( dpwr2 > 50 )
  { text_error("don't fry the probe, DPWR2 too large!  ");   	    psg_abort(1); }

  if( pw > 50.0e-6 )
  { text_error("dont fry the probe, pw too high ! ");               psg_abort(1); } 
  
  if( pwN > 100.0e-6 )
  { text_error("dont fry the probe, pwN too high ! ");              psg_abort(1); }



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
{ text_error("Choose only one relaxation measurement ! ");          psg_abort(1); } 


if ( ((T1[A]=='y') || (T1rho[A]=='y')) && 
       ((relaxT*100.0 - (int)(relaxT*100.0+1.0e-4)) > 1.0e-6) )
 { text_error("Relaxation time, relaxT, must be zero or multiple of 10msec"); psg_abort(1);}
 

 if ( (T2[A]=='y') && 
           (((relaxT+0.01)*50.0 - (int)((relaxT+0.01)*50.0+1.0e-4)) > 1.0e-6) )
{ text_error("Relaxation time, relaxT, must be odd multiple of 10msec"); psg_abort(1);}

if ( ((T1rho[A]=='y') || (T2[A]=='y'))  &&  (relaxTmax > 0.25) && (ix==1) ) 
{ printf("WARNING, sample heating will result for relaxT>0.25sec"); }

if ( ((T1rho[A]=='y') ||  (T2[A]=='y'))  &&  (relaxTmax > 0.5) ) 
{ text_error("relaxT greater than 0.5 seconds will heat sample"); psg_abort(1);}


if ( ((NH2only[A]=='y') || (T1[A]=='y') || (T1rho[A]=='y') || (T2[A]=='y'))
   &&  (TROSY[A]=='y') ) 
{ text_error("TROSY not implemented with NH2 spectrum, or relaxation exps."); psg_abort(1);} 


if ((TROSY[A]=='y') && (dm2[C] == 'y'))
{ text_error("Choose either TROSY='n' or dm2='n' ! ");              psg_abort(1); }
/* PHASES AND INCREMENTED TIMES */

/*  Phase incrementation for hypercomplex 2D data, States-Haberkorn element */

    if (TROSY[A]=='y')
	 {  if (phase1 == 2)   				      icosel = -1;
            else 	  {  tsadd(t4,2,4);  tsadd(t10,2,4);  icosel = +1;  }
	 }
    else {  if (phase1 == 2)  {tsadd(t10,2,4); icosel = +1;}
            else 			       icosel = -1;    
	 }

    if(Hdecflg[0] != 'n') ihh = icosel;

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
	decpwrf(rf0);
 	dec2power(pwNlvl);
	txphase(zero);
        decphase(zero);
        dec2phase(zero);
        if(Hdecflg[0] != 'n')
        {
          delay(5.0e-5);
          rgpulse(pw,zero,rof1,0.0);                 
          rgpulse(pw,one,0.0,rof1);                 
          zgradpulse(1.5*gzlvl0, 0.5e-3);
          delay(5.0e-4);
          rgpulse(pw,zero,rof1,0.0);                 
          rgpulse(pw,one,0.0,rof1);                 
          zgradpulse(-gzlvl0, 0.5e-3);
        }
        
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
	if (TROSY[A]=='n')   
	dec2rgpulse(pwN, zero, 0.0, 0.0);   /*destroy N15 magnetization*/
	zgradpulse(gzlvl0, 0.5e-3);
	delay(1.0e-4);
	if (TROSY[A]=='n')    dec2rgpulse(pwN, one, 0.0, 0.0);
	zgradpulse(0.7*gzlvl0, 0.5e-3);
	decpwrf(rfst);
	txphase(t1);
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

   	rgpulse(calH*pw,t1,0.0,0.0);                 /* 1H pulse excitation */

	txphase(zero);
   	dec2phase(zero);
	zgradpulse(gzlvl0, gt0);
	delay(lambda - gt0 - pwHH);
	
	if(Hdecflg[0] != 'n')
	{
	  obspower(tpwrs);
          if (tpwrsf<4095.0) obspwrf(tpwrsf); 
	  shaped_pulse("H2Osinc", pwHH, two, 5.0e-5, 0.0);
	  obspower(tpwr);
          if (tpwrsf<4095.0) obspwrf(4095.0);
   	  sim3pulse(2.0*pw, 0.0, 2.0*pwN, zero, zero, zero, 0.0, 0.0);
   	  obspower(tpwrs);
          if (tpwrsf<4095.0) obspwrf(tpwrsf);
   	  shaped_pulse("H2Osinc", pwHH, two, 5.0e-5, 0.0);
   	  obspower(tpwr);
          if (tpwrsf<4095.0) obspwrf(4095.0); 
   	}
   	else 
   	  sim3pulse(2.0*pw, 0.0, 2.0*pwN, zero, zero, zero, 0.0, 0.0);
   	
   	txphase(one);
	zgradpulse(gzlvl0, gt0);
	delay(lambda - gt0 - pwHH);        
 	rgpulse(pw, one, 0.0, 0.0);
	txphase(two);
        obspower(tpwrs);
        if (tpwrsf<4095.0) obspwrf(tpwrsf);
        shaped_pulse("H2Osinc", pwHs, two, 5.0e-5, 0.0);
	obspower(tpwr);
	if (tpwrsf<4095.0) obspwrf(4095.0);

        if (TROSY[A]=='y')
	  zgradpulse(ihh*gzlvl3, gt3);           
	else
	  zgradpulse(-ihh*gzlvl3, gt3);
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
           {decshaped_pulse("stC200", 1.0e-3, zero, 0.0, 0.0);
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
            decshaped_pulse("stC200", 1.0e-3, zero, 0.0, 0.0);
            delay(tau1 - 0.5e-3);}
	else    delay(2.0*tau1);

        if (mag_flg[A] == 'y')  magradpulse(gzcal*gzlvl1, gt1);
        else  zgradpulse(gzlvl1, gt1);   	/* 2.0*GRADIENT_DELAY */
	delay(2.0e-4 - 2.0*GRADIENT_DELAY);

	dec2rgpulse(2.0*pwN, t9, 0.0, 0.0);

	txphase(three);

        delay(gt1 + 2.0e-4 - pwHs - 1.0e-4 - 2.0*pwr_dly);
        obspower(tpwrs);
	if (tpwrsf<4095.0) obspwrf(tpwrsf);
        shaped_pulse("H2Osinc", pwHs, three, 5.0e-5, 0.0);
        obspower(tpwr);
	if (tpwrsf<4095.0) obspwrf(4095.0);

	txphase(t4);
	delay(5.0e-5);
}

else
{					  	    /* fully-coupled spectrum */
        if (dm2[C]=='n')  {rgpulse(2.0*pw, zero, 0.0, 0.0);  pw=0.0;}		

  	if ( (C13refoc[A]=='y') && (tau1 > 0.5e-3 + WFG2_START_DELAY) )
           {delay(tau1 - 0.5e-3 - WFG2_START_DELAY);     /* WFG2_START_DELAY */
            simshaped_pulse("", "stC200", 2.0*pw, 1.0e-3, zero, zero, 0.0, 0.0);
            delay(tau1 - 0.5e-3);
            delay(gt1 + 2.0e-4);}
	else
           {delay(tau1);
            rgpulse(2.0*pw, zero, 0.0, 0.0);
            delay(gt1 + 2.0e-4 - 2.0*pw);
            delay(tau1);} 
 
	pw=getval("pw");
	dec2rgpulse(2.0*pwN, t9, 0.0, 0.0);

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

	delay((gt1/10.0) + 1.0e-4 +gstab - 0.65*pw + 2.0*GRADIENT_DELAY + POWER_DELAY);

        if ( dm3[B] == 'y' )   /* turns off 2H decoupling  */
           {
           setstatus(DEC3ch, FALSE, 'c', FALSE, dmf3);
           dec3rgpulse(1/dmf3,three,2.0e-6,2.0e-6);
           dec3blank();
           lk_autotrig();   /* resumes lock pulsing */
           }

	rgpulse(2.0*pw, zero, 0.0, 0.0);

	dec2power(dpwr2);				       /* POWER_DELAY */
        if (mag_flg[A] == 'y')	  magradpulse(icosel*gzcal*gzlvl2, 0.1*gt1);
        else   zgradpulse(icosel*gzlvl2, 0.1*gt1);		/* 2.0*GRADIENT_DELAY */
        

        if(Cdecflg[0] == 'y')
        {
          delay(gstab-2.0*POWER_DELAY-PRG_START_DELAY+rof2);
          rcvron();
                           
          statusdelay(C,1.0e-4);		

          if (dm3[B] == 'y') 
          {
            delay(1/dmf3); 
            lk_sample();
          }
	  setreceiver(t12);
          pbox_decon(&Cdseq);
          
          if(Hdecflg[0] == 'y')
            homodec(&HHdseq);  
        }
        else
        {
          delay(gstab+rof2);
          rcvron();
                             
          statusdelay(C,1.0e-4);		

          if (dm3[B] == 'y') 
          {
            delay(1/dmf3); 
            lk_sample();
          }
	  setreceiver(t12);

          if(Hdecflg[0] == 'y')
            homodec(&HHdseq);        
        }
}		 

/* ------------------------------ UTILITY FUNCTIONS -------------------------------- */

void makeCdec()
{
  double  Cppm = getval("dfrq"),                 /*  pre-set C decoupling parameters */        
          cbw = 250.0*Cppm,                       /* C-decoupling bandwidth, 250 ppm */
          cpw = 0.006,                              /* C-decoupling pulsewidth, 6 ms */
	  pwC = getval("pwC"),
	  pwClvl = getval("pwClvl"),
          compC = getval("compC"),
          Cdecpwr;
  char    cmd[MAXSTR];

            /* create low power decoupling for long range J(CH) and do it only once */
            
    if((getval("arraydim") < 1.5) || (ix==1))  
    {
      sprintf(cmd, "Pbox Cdec_lp -w \"WURST-40 %.2f/%.6f\" -s 4.0 -p %.0f -l %.2f -0",
                    cbw, cpw, pwClvl, 1.0e6*pwC*compC);
      system(cmd);
      Cdseq = getDsh("Cdec_lp");
      Cdseq.pwr = Cdseq.pwr + 1.0;                   /* optional 1 dB power increase */
    }   
    if ((find("Cdecpwr") == 4) && (Cdecpwr = getval("Cdecpwr")))
      Cdseq.pwr = Cdecpwr;              /* if requested, set to user defined power */
    setlimit("Cdseq.pwr", Cdseq.pwr, 45.0);
}

void makeHHdec()
{
  double  ppm = getval("sfrq"),                  /* pre-set HH decoupling parameters */        
          hhbw = 3.0*ppm,                               /* homo-decoupling bandwidth */
          hhpw = 0.020,                                /* homo-decoupling pulsewidth */
          hhofs = -0.25*ppm,                      /* homo-decoupling offset from H2O */
          hhstp = 1e6/getval("sw"),                /* homo-decoupling shape stepsize */
          compH = getval("compH"),
          Hdecpwr;
  char    cmd[MAXSTR];
      
            /* create shape for HH homo-decoupling and do it only once */
            
    if((getval("arraydim") < 1.5) || (ix==1))  
    {
      sprintf(cmd, "Pbox hhdec -w \"CAWURST-8 %.2f/%.6f %.2f\" -s %.2f ",
                    hhbw, hhpw, hhofs, hhstp);
      sprintf(cmd, "%s -dcyc 0.05 -p %.0f -l %.2f -0\n", 
                    cmd, tpwr, 1.0e6*pw*compH);
      system(cmd);
      HHdseq = getDsh("hhdec"); 
    }   
    if ((find("Hdecpwr") == 4) && (Hdecpwr = getval("Hdecpwr")))
      HHdseq.pwr = Hdecpwr;              /* if requested, set to user defined power */
    setlimit("HHdseq.pwr", HHdseq.pwr, 51.0);
}
