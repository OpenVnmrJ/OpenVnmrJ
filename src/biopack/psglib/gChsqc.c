/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* gChsqc.c

    HSQC with gradients for C13/H1 chemical shift correlation and numerous
    options for refocusing/decoupling J couplings, editing spectral regions,
    and measuring relaxation times.

    		      NOTE dof MUST BE SET AT 35ppm ALWAYS
    

    THE ZZ (phase-cycled) OPTION [ZZ='y']:

    Suppresses H2O with gradients but does not use coherence gradients.  Must
    use ssfilter/ssntaps for good H2O suppression.  The ZZ sequence employs
    sech/tanh inversion pulses.  It can be broadbanded to cover aromatic and
    aliphatic CHn groups by setting allC='y'.  Alternatively, aliph='y' selects
    the aliphatic region; alphaC='y' also selects the aliphatic region but
    with optimized S/N on Ca's (peptide alpha carbons); and arom='y' selects
    the aromatic region. 


    THE ALTERNATIVE SE OPTION [SE='y']:

    Uses    coherence gradients     and the     Sensitivity Enhancement
    train for better H2O suppression for CH resonaces very close to H2O. 
    Although a theoretical root2 CH 2D enhancement should be gained, there
    was some loss of S/N from relaxation and C-C coupling using the protein 
    alphalytic protease, so only a modest gain in S/N overall.  CH2 and
    CH3 groups lose up to 50% S/N relative to the ZZ option.  Overall we 
    suggest that SE='y' has advantages for Ca's, especially as it permits
    the Crefoc_l and Crefoc_r options described below. However the sequence will
    function correctly with aliph='y'/arom='y'/allC='y', but the 180's in the
    SE train and gradient echo lose S/N for allC='y'.  (The WET method applied 
    to the ZZ option was tried as an alternative for H2O suppression but gave
    no better suppression than SE and saturated CH resonances close to H2O).

    Coherence transfer gradients may be z or magic-angle(using triax probe).

    VNMR processing when SE='y':
    for VNMR5.3 or later, use f1coef='1 0 -1 0 0 -1 0 -1'
    ( use f1coef='1 0 -1 0 0 1 0 1' if CT option is also used)
    and wft2da. For earlier versions of VNMR use these coefficients as arguments
    for wft2da.


    THE CT OPTION: [CT='y']:

    This converts the t1 C13 shift evolution to Constant Time. Any combination 
    of ZZ='y'/SE='y' and aliph='y'/alpha='y'/arom='y'/allC='y'can be used.
    The constant time delay, CTdelay, can be set for optimum S/N for any type
    of groups, eg 27ms for Ca's and 16ms for aromatics.  Note that in some 
    options, CTdelays less than 8ms will generate an error message resulting 
    from a negative delay.  It is recommended that CT='n' is generally more 
    useful.   
  

    OTHER FEATURES AND OPTIONS:

    C13 sech/tanh 1ms inversion pulses have been used wherever there is an
    advantage.  (Although two adiabatic pulses [and two echos] can be used for 
    refocusing, these were not implemented in the SE train because the large 
    angle precessions around the effective field are subject to significant 
    variation with RF power in terms of absolute angles in degrees).  The 
    standard BioPack one-lobe sinc inversion pulse on CO was retained to 
    refocus 13C coupling during t1 when aliph='y'or alphaC='y', because it is 
    much shorter than a sech/tanh pulse and equally effective.  This is
    optional and is set when COrefoc='y'.  When SE='y'; alphaC='y'; & 
    COrefoc='y', the options Crefoc_r='y' and Crefoc_l='y' may be selected.  
    Each of these inserts two 3ms sech/tanh inversion pulses on the Cb's 
    (peptide beta carbons) refocusing the Cb-Ca coupling for all Cb's to the 
    right of 46ppm (Crefoc_r='y') and to the left of 67ppm (Crefoc_l='y').
 
    In all the above cases:
    * Selecting CH2only='y' will give a spectrum of only CH2 groups. 
    * A spectrum of only CH groups can be obtained as described in more detail 
    below.
    * N15 coupling in doubly-labelled samples is refocused by N15refoc='y'.
    * H2 coupling in triply-labelled samples is refocused by dm3='nyn'.
    * Efficient STUD+ decoupling is invoked with STUD='y' without need to 
    set any parameters.
    
    T1, T1rho, and T2 relaxation measurements of C13 nuclei can be made for any 
    combination of the above options.

    1D checks of C13 and H1 pulse times can be made using calC and calH.  


    pulse sequence:	John, Plant and Hurd, JMR, A101, 113 (1992)
		 	Kay, Keifer and Saarinen, JACS, 114, 10663 (1992)
    sech/tanh pulse: 	Silver, Joseph and Hoult, JMR, 59, 347 (1984)
			Bendall, JMR, A116, 46 (1995)
    relaxation times:   Kay et al, JMR, 97, 359 (1992)
			Farrow et al, Biochemistry, 33, 5984 (1994)
			Yamazaki et al, JACS, 116, 8266 (1994)
    STUD+ decoupling    Bendall & Skinner, JMR, A124, 474 (1997) and in press
    improved water suppression:  F.A.A.A. Mulder, J. Biom. NMR, 8, 223(1996). (gzlvl5)
     
    Written by MRB, January 1998, starting with ghn_co from BioPack.
    Revised and improved to a standard format by MRB, BKJ and GG for the 
    BioPack, January 1998, so as to include calculation of the above 
    standard features within the pulse sequence code and associated macros.
    Minor modifications by GG and MRB during 1998 (version Nov. 98).
    Automatically reduced gzlvl3 by factor of 10 for SE='y', resulting in
    improved water suppression (E.Kupce, Varian, Aug 2008).



        	  CHOICE OF DECOUPLING AND 2D MODES

    Set dm = 'nny', dmm = 'ccg' (or 'ccw', or 'ccp') for 13C decoupling.
    Set dm2 = 'nnn', dmm2 = 'ccc'.
    Set dm3 = 'nnn' for no 2H decoupling, or
	      'nyn'  and dmm3 = 'w' for 2H decoupling. 

    Must set phase = 1,2  for States-TPPI acquisition in t1 [C13].
    
    The flag f1180 should be set to 'y' if t1 is to be started at halfdwell
    time. This will give 90, -180 phasing in f1. If it is set to 'n' the 
    phasing should be 0,0 and will still give a perfect baseline.  Thus,
    set f1180='n' for (0,0) in C13.  f1180='y' is ignored if ni=0.



          	  DETAILED INSTRUCTIONS FOR USE OF gChsqc

         
    1. Obtain a printout of the Philosopy behind the BioPack development,
       and General Instructions using the macro: 
                                      "printon man('BioPack') printoff".
       These Detailed Instructions for gChsqc may be printed using:
                                      "printon man('gChsqc') printoff".
             
    2. Apply the setup macro "gChsqc".  This loads the relevant parameter set
       and also sets ni=0 and phase=1 ready for a 1D spectral check.

    3. Centre H1 frequency on H2O (4.7ppm), N15 frequency on the amide
       region (120 ppm), and C13 frequency on 35ppm.

    4. CHOICE OF C13 REGION:
       aliph='y' gives a spectrum of aliphatic resonances centered on dof=35ppm.
       This is a common usage.                              Set sw1 to 80ppm.

       alphaC='y' also gives a spectrum of aliphatic resonances, but dof is
       automatically shifted by the pulse sequence code to 56ppm, and all
       pulses are optimized for Ca resonances.		    Set sw1 to 80ppm.

       arom='y' gives a spectrum of aromatic groups.  dof is shifted 
       automatically by the pulse sequence code to 125ppm.  Set sw1 to 30 ppm.

       allC='y' gives a spectrum of aliphatic and aromatic resonances. dof is
       shifted by the code to 70ppm.                        Set sw1 to 140 ppm.
       C13 shift during the 90 degree C13 pulses is not refocused for the
       first increment when ZZ='y' and allC='y' (use linear prediction).

    5. CALIBRATION OF pwC AND pw:
       calC and calH are multipliers of the first C13 and H1 90 degree
       pulses in the sequence, and are listed in dg.    They can be used to
       calibrate the pulses using a 1D spectrum.      Setting either calC or
       calH = 2.0 should give a S/N null (or small dispersion signals)
       corresponding to the pulses being set at 180 degrees.     Adjust pwC
       or pw, respectively, until this occurs.      An array of calC or
       calH = 1.8, 2.0, 2.2 is also convenient to judge the null at 2.0.
       calC and calH are automatically reset to 1 for 2D spectra if you   
       forget to do so.     Because of the width of the protein aliphatic C13 
       region, only the central portion of the H1 1D spectrum will be nulled
       when aliph='y' and calC=2.      It is less ambiguous to do the C13
       calibration on the narrower aromatic region using arom='y'.  (Note that
       compH can be calibrated using calH in an analogous way to compC as in
       point 6).

    6. C13 AMPLIFIER COMPRESSION AND PHASE SHIFT AT LOWER POWER:
       For optimum usage of the sequences in BioPack, the parameter compC
       should be calibrated for the selected pwClvl.  Measure pwC as in point 5.
       Lower pwClvl by 6 dB (half RF amplitude) and measure pwC again calling
       it pwC'.  Then compC=pwC'/(2*pwC).  As under point 5, if you do these 
       calibrations using a protein, it is best to utilise the narrower
       aromatic region since dof needs to be close to the C13 resonances.  If
       compC is less than 0.9 it is probably best to use a lower pwClvl so that 
       compC > 0.9 and amplifier compression does not need to be compensated so 
       heavily.

       For all BioPack sequences, slightly higher S/N may be
       obtained by using a higher value for compC than determined by this
       absolute method because of the gradual curvature of the compression.
       For max S/N do a quick compC array to higher values for 1D spectra.

       When amplifier compression is evident, there may also be a small RF
       phase shift when changing from pwClvl to a lower power.  To check this,
       first ensure the phshift10 calibration in point 7 is accurate.  Then
       set ni=0; SE='y'; alphaC='y'; and array COrefoc='n','y'. The two 1D 
       spectra should have the same phase and almost the same intensities. Any
       difference in the value of rp for correctly phased spectra arises from 
       the different phase of a 180 refocusing pulse which is at lower power
       in the 2nd spectrum - the actual RF phase difference is half the 
       difference in rp, since the refocusing pulse doubles the error.  The
       change in rp will appear as rp1 in a 2D spectrum.  Although this is a
       trivial correction for normal 2D, there will be a loss of S/N for T1rho
       measurements as in point 16 given by 1-cos(rp1/2.0). 
            
    7. CO COUPLING REFOCUSED WHEN aliph='y'.  SET COrefoc='y':
       The normal C13 180 degree pulse on CO at the middle of t1 induces a
       phase shift, which should be field-invariant, and so this phase shift 
       has been calibrated and compensated in the pulse sequence.  If not 
       adequately compensated, the first increment may be out of phase with
       all succeeding increments and a zero-order phase-shift will be necessary
       in F1, which is easily done after the 2D/3D transform.  This phase
       shift can be checked by setting ni=0; SE='y'; aliph='y'; and array 
       COrefoc='n','y'.  The two 1D spectra should have the same phase and so
       look identical. If not, change the calibration by changing the phshift10 
       parameter in the INITIALIZE VARIABLES section of the code (+ve values 
       only allowed and a phase errors of 5 degrees are quite obvious). 

    8. CO COUPLING REFOCUSED WHEN alphaC='y'.  SET COrefoc='y':
       This uses a different SLP pulse, than in point 7, ie offC9 rather than
       offC10 but uses all the same parameter names and calibrations,
       including phshift10.

    9. Cb COUPLING REFOCUSED WHEN SE='y'; alphaC='y'.
       SET  Crefoc_r='y'  AND/OR   Crefoc_l='y':
       Crefoc_r refocuses Cb's to the right of Ca's, and Crefoc_l to the left.
       Each flag uses a pair of 5ms sech/tanh pulses (two stC50_5r pulses or
       two stC50_5l pulses), and so T2 losses of S/N corresponding to 10ms or 
       20ms delays are incurred, but a 100% 2D gain from refocusing the CaCb 
       coupling is potentially gained.  The selectivity of EACH pulse changes 
       from 10% inversion to 90% inversion in 490 Hz for a 600 MHz spectrometer 
       [increasing in absolute terms to 510 Hz (but decreasing markedly in ppm) 
       or a 1 GHz spectrometer], or about 20% to 80% for BOTH pulses overall.  
       These stC50 pulses have been convoluted (SLP) off resonance placing the 
       overall 3% inversion point for each pair of pulses at 50ppm for the 
       right-hand option, and at 63ppm for the left-hand option.  Although this 
       worked well for alphalytic protease, as of Jan '98 this aspect had not 
       been exhaustively tested, and it may be advantageous to move the 3% 
       inversion points further to the right and left of the C13 spectrum, eg
       to 48ppm and to 65ppm.  To do so change the nos. 50.0 and 63.0 in the 
       equation for $freq in the macros "makestC50_5r" and "makestC50_5l"    
       respectively.  Then run these two macros (or the macro "proteincal") 
       again. When both left and right options are selected the total delay
       during pulses and gradients is 25ms - this is convenient, because any
       Cb's that fall between 50 and 63ppm, and their coupled Ca's, are
       inverted by coupling and appear as negative peaks in a 1D spectrum or
       2D spectrum. 

   10. Ca COUPLING REFOCUSED WHEN 'ZZ'='y'; aliph='y'.  SET Crefoc_l='y':
       Uses a pair of 10 ms stC12_10 sech/tanh pulses, with similar attributes
       to the discussion in point 9.  Inverts from 50 ppm to 62.5 ppm, with
       overall inversion being 90% at those two points. The points for overall
       10% inversion are 310 Hz outside of 50/62.5.  As for point 9, the pulse 
       is more selective at higher field as the 310 difference becomes smaller
       in ppm.  Change the bandwidth value of 1.875kHz in the macro
       "makestC12_10" and in the pulse sequence code to change this selectivity. 

   11. N15 COUPLING:
       Splitting of resonances in the C13 dimension by N15 coupling in N15
       enriched samples is removed by setting N15refoc='y'.  No N15 RF is
       delivered in the pulse sequence if N15refoc='n'.  N15 parameters are
       listed in dg2.

   12. 1/4J DELAY TIMES:
       These are listed in dg/dg2 for possible change by the user. lambda is
       the 1/4J time for H1 CH coupling evolution, tCH is the 1/4J time for
       C13 CH coupling evolution.  lambda is usually set a little lower than
       the theoretical time to minimise relaxation, but tCH should be as close
       to the theoretical time as possible.  So for:
         aliphatic CH/CH2  (J~140Hz):   lambda = 0.0016         tCH = 0.00175
         aliphatic CH3 (J~128Hz):			        tCH = 0.00195
           aromatics   (J~165Hz):       lambda = 0.0013         tCH = 0.00145

   13. SPECTRAL EDITING FOR DIFFERENT CHn GROUPS.
       CH2only='y' provides spectra of just CH2 groups, by inserting two 1/4J
       periods of CH coupling evolution, where generally the parameter
			 tCH = 1/4J = 0.0018.
       CH groups are more accurately nulled using tCH = 0.00173 and CH3 groups
       are nulled better at tCH = 0.00195.  ZZ='y' is best for CH2's.  The
       CH2only flag cannot be used with relaxation time measurements.

       By setting tCH = 0.0009 with CH2only='y', CH2's are nulled and CH3's
       are partially negative giving a CH spectrum at about 70% S/N.  CH3's 
       are nulled at tCH = 0.00075 if required for the study of overlapping 
       CH's at low H1 ppm values with CH2's being partially positive.  SE='y'
       is best for CH spectra.  

       Probably a better method to get a CH spectrum is to set T1='y'; 
       tCH=0.00175 and relaxT=0 as in point 14 below. This also eliminates CH2 
       groups, leaves CH3 groups partially positive and gives better S/N for CH 
       groups. For example, if combined with alphaC='y'; COrefoc='y'; 
       Crefoc_r='y'; Crefoc_l='y', all C13 resonances are eliminated except
       Ca's and a small number of Cb's which are CH's and occur between 
       50-63ppm.  Maximum S/N for CH's is obtained at tCH = 0.0016, but 0.0018 
       gives a better null for most CH2's. CH3's are minimum at tCH = 0.0014
       but cannot generally be nulled.  SE='y' is best for CH spectra and must 
       be used with Crefoc_r and Crefoc_l='y' flags.
       
   14. T1 MEASUREMENTS OF CH GROUPS.  SET tCH = 0.00175 AND T1='y':
       An array of 1D spectra is obtained by arraying the relaxation time 
       parameter, relaxT, to a multiple of 5 milliseconds including zero.  
       relaxT corresponds exactly to the relaxation time of the C13 spins.  The 
       method uses 180 degree H1 pulses every 5ms during relaxT and transverse 
       magnetization is dephased by a gradient as according to Kay et al.
       CH2 resonances are dephased and CH3 resonances are greatly reduced by 
       the method, so with relaxT set to a small value it is a convenient
       method to get a CH only spectrum as described in point 13.  SE='y' is 
       best for CH's.  For aromatic CH's use tCH = 0.00145.

       2D spectra should be acquired in a separate run for each of the desired 
       values of relaxT. (maxrelaxT as in point 16 is not used when T1='y'.)

   15. T1 MEASUREMENTS OF CH2 and CH3 GROUPS:
       Proceed as for point 14 but set tCH = 0.0009 for CH2 groups and
       tCH = 0.0007 for CH3 groups.  Unfortunately the H1 irradiation removes 
       spin order and about 50% S/N is lost for CH2 and CH3.  Also 50% of
       CH intensity is retained so the overall result at relaxT=0 is a 
       normal 1D spectrum at about half intensity.  However accurate CH2 
       and CH3 T1 times can be determined from resolved peaks in 2D spectra.  
       ZZ='y' is best for CH2's and CH3's.  
	
   16. T1rho MEASUREMENTS OF CHn GROUPS:
       Implemented by setting the flag T1rho='y' and arraying relaxT as above
       for T1.  relaxT corresponds exactly to the relaxation time of the C13 
       spins.  Spin lock power is set at 2.0 kHz for a 600 MHz spectrometer,
       and scaled in proportion to other field strengths.  Increasing this RF
       (by changing the number 2000 in the DECLARE AND LOAD VARIABLES section
       of gChsqc.c) causes substantial sample or coil heating as indicated by 
       deterioration of the lock signal).  Care should be taken for relaxT
       times greater than 0.25 seconds, and times greater than 0.5 seconds are 
       automatically aborted.  A dummy period of spinlock RF is delivered at
       the beginning of each pulse sequence to ensure constant average sample 
       heating - the code determines this from the maximum relaxT you have set
       in your array of relaxT.  The spectrum at relaxT=0 may be erroneous
       because of insufficient dephasing of unlocked spins.  The setting of
       all other parameters and flags are as for T1='y' in points 13 to 15.

       2D spectra should be acquired in a separate run for each of the desired 
       values of relaxT.  If T1rho measurements are to be made in different
       experiments with different relaxTs as for 2D, including arrays of 
       relaxT's, set the parameter maxrelaxT to the maximum relaxT you have
       set in all experiments - this will ensure constant average sample
       heating in all experiments.  maxrelaxT does not need to be set for a
       single array in a single exp.

   17. T2 MEASUREMENTS OF CH GROUPS:
       Implemented by setting the flag T2='y', but not recommended because 
       homonuclear C13 coupling invalidates the estimated relaxation time.
       Like T1rho, other parameters and flags are as for T1='y'.  Also, T2='y'
       delivers more sample heating than T1rho='y' for the same relaxation
       time.   2D spectra should be acquired in separate runs with maxrelaxT
       set as for T1rho='y' in point 16.       
       if BioPack power limits are in effect (BPpwrlimits=1) the cpmg 
       amplitude is decreased by 3dB and pulse width increased 40%

   18. STUD DECOUPLING.   SET STUD='y':
       Setting the flag STUD='y' overrides the decoupling parameters listed in
       dg2 and applies STUD+ decoupling instead.  In consequence it is easy
       to swap between the decoupling scheme you have been using to STUD+ for
       an easy comparison.  The STUD+ waveforms are calculated for your field
       strength at the time of BioPack installation and RF power is 
       calculated within the pulse sequence.  The calculations are for the most 
       efficient conditions to cover 140ppm when allC='y' with all decoupled 
       peaks being greater than 85% of ideal; 80ppm/90% for aliph='y'; and 
       30ppm/95% for alphaC='y' or arom='y'.  If you wish to compare different
       decoupling schemes, the power level used for STUD+ can be obtained from 
       dps - subtract 3dB from that level to compare to decoupling schemes at
       a continuous RF level such as GARP.  The values of 85, 90, and 95% have
       been set to limit sample heating as much as possible.  If you wish to 
       change STUD parameters, for example to increase the quality of decoupling
       (and RF heating) for 30ppm decoupling say, change the 95% level for the 
       centerband, by changing the relevant number in the macro makeSTUDpp and 
       rerun the macro (don't use 100% !!).  (At the time of writing STUD has
       been coded to use the coarse attenuator, because the fine attenuator
       is not presently included in the fail-safe calculation of decoupling 
       power which aborts an experiment if the power is too high - this may
       lower STUD efficiency a little).

  19.  When SE='y', the coherence-transfer gradients using power levels
       gzlvl1 and gzlvl2 may be either z or magic-angle gradients. For the
       latter, a proper /vnmr/imaging/gradtable entry must be present and
       syscoil must contain the value of this entry (name of gradtable). The
       amplitude of the gzlvl1 and gzlvl2 should be lower than for a z axis
       probe to have the x and y gradient levels within the 32k range. For
       any value, a dps display (using power display) shows the x,y and z
       dac values. These must be <=32k.
  

*/



#include <standard.h>
  

/* Left-hand phase list for ZZ='y'; right-hand list for SE='y'; phi3 common */

static int   phi1[8]  = {1,1,1,1,3,3,3,3},	phi2[1]  = {1},
	     phi3[2]  = {0,2},	      		phi9[8]  = {0,0,1,1,2,2,3,3},
	     phi4[4]  = {0,0,2,2},		phi10[1] = {0}, 
             rec1[8]  = {0,2,2,0,2,0,0,2},	rec2[4]  = {0,2,2,0};

static double   d2_init=0.0,
	        relaxTmax;


void pulsesequence()
{



/* DECLARE AND LOAD VARIABLES */

char        f1180[MAXSTR],   		      /* Flag to start t1 @ halfdwell */
	    ZZ[MAXSTR],				    /* no coherence gradients */
	    SE[MAXSTR],		 /* coherence gradients & sensitivity enhance */
	    CT[MAXSTR],				       /* constant time in t1 */
	    aliph[MAXSTR],	 		 /* aliphatic CHn groups only */
	    alphaC[MAXSTR],		 	       /* Ca groups optimized */
	    arom[MAXSTR], 			  /* aromatic CHn groups only */
	    allC[MAXSTR], 		     /* aliphatic and aromatic groups */
	    stCshape[MAXSTR],	      /* calls sech/tanh pulses from shapelib */
	    stCdec[MAXSTR],	       /* calls STUD+ waveforms from shapelib */
	    COrefoc[MAXSTR],   /* no CO coupling when aliph='y' or alphaC='y' */
	    Crefoc_r[MAXSTR],  	        /* no rhs Cb coupling when alphaC='y' */
	    Crefoc_l[MAXSTR],           /* no lhs Cb coupling when alphaC='y' */
	    CH2only[MAXSTR],		       /* spectrum of only CH2 groups */
	    offCshape[MAXSTR],	  	    /* calls SLP pulses from shapelib */
	    N15refoc[MAXSTR],		 	  /* N15 pulse in middle of t1*/
	    T1[MAXSTR],				/* insert T1 relaxation delay */
	    T1rho[MAXSTR],		     /* insert T1rho relaxation delay */
	    T2[MAXSTR],				/* insert T2 relaxation delay */
            mag_flg[MAXSTR],           /* flag to select magic-angle gradient */
	    STUD[MAXSTR];   /* apply automatically calculated STUD decoupling */
 
int         icosel=1,          			  /* used to get n and p type */
            t1_counter,  		        /* used for states tppi in t1 */
	    rTnum,			/* number of relaxation times, relaxT */
	    rTcounter;		    /* to obtain maximum relaxT, ie relaxTmax */

double      tau1,         				         /*  t1 delay */
	    lambda = getval("lambda"),		   /* 1/4J H1 evolution delay */
	    tCH = getval("tCH"),		  /* 1/4J C13 evolution delay */
	    CTdelay = getval("CTdelay"),     /* total constant time evolution */
	    relaxT = getval("relaxT"),		     /* total relaxation time */
	    rTarray[1000], 	    /* to obtain maximum relaxT, ie relaxTmax */
            maxrelaxT = getval("maxrelaxT"),    /* maximum relaxT in all exps */
	    ncyc,			 /* number of pulsed cycles in relaxT */
            calG = 1.0,
   pwClvl = getval("pwClvl"), 	  	        /* coarse power for C13 pulse */
   pwC = getval("pwC"),     	      /* C13 90 degree pulse length at pwClvl */
   rf0,            	          /* maximum fine power when using pwC pulses */
   calC = getval("calC"),        /* multiplier on a pwC pulse for calibration */

/* 180 degree pulse at Cab (35ppm), first off-resonance null at CO(174ppm)    */
        pwC2 = 0.0,            /* 180 degree pulse length at rf2, initialised */
        rf2 = 0.0,/* fine power for 12.1 kHz rf for 600MHz magnet, initialised*/

/* Sech/tanh inversion pulses automatically calculated by macro "proteincal"  */
/* and string parameter stCshape calls them from your shapelib. 	      */
   rfst = 0.0,	            /* fine power for the stCshape pulse, initialised */
   rfst1 = 0.0,	         /* fine power for the stC12 & 50 pulses, initialised */
   dofa = 0.0,   /* dof shifted to 56, 70, 125ppm for Ca's, allC and arom 
						       spectra, initialised   */

/* STUD+ waveforms automatically calculated by macro "proteincal"  	      */
/* and string parameter stCdec calls them from your shapelib. 	              */
   studlvl = 0.0,	    /* coarse power for STUD+ decoupling, initialised */
   stdmf = 1.0,        		      /* dmf for STUD decoupling, initialised */
   rf140 = getval("rf140"), 			 /* rf in Hz for 140ppm STUD+ */
   dmf140 = getval("dmf140"), 			      /* dmf for 140ppm STUD+ */
   rf80 = getval("rf80"), 			  /* rf in Hz for 80ppm STUD+ */
   dmf80 = getval("dmf80"), 			       /* dmf for 80ppm STUD+ */
   rf30 = getval("rf30"), 			  /* rf in Hz for 30ppm STUD+ */
   dmf30 = getval("dmf30"), 			       /* dmf for 30ppm STUD+ */

/* the following pulse lengths for SLP pulses are automatically calculated    */
/* by the macro "proteincal".  SLP pulse shapes, "offC10"/"offC9" are called  */
/* directly from your shapelib.  offC9 uses the same parameters as offC10.    */
 pwC10 = getval("pwC10"),  /*180 degree pulse at CO(174ppm) null at Ca(56ppm) */
 phshift10,           /* phase shift induced on Cab by pwC10 ("offC10") pulse */
 pwZa,					 /* the largest of 2.0*pw and 2.0*pwN */
 pwZ,					     /* the largest of pwC10 and pwZa */
 rf10 = 0.0,	    /* fine power for the pwC10 ("offC10") pulse, initialised */

 compC = getval("compC"),         /* adjustment for C13 amplifier compression */
 slClvl,					   /* power for C13 spin lock */
 slCrf = 2000.0, 	       /* RF field in Hz for C13 spin lock at 600 MHz */

	calH = getval("calH"), /* multiplier on a pw pulse for H1 calibration */

	pwNlvl = getval("pwNlvl"),	              /* power for N15 pulses */
        pwN = getval("pwN"),          /* N15 90 degree pulse length at pwNlvl */

	sw1 = getval("sw1"),

        BPpwrlimits,                    /*  =0 for no limit, =1 for limit */

	gt1 = getval("gt1"),  		       /* coherence pathway gradients */
        gzcal = getval("gzcal"),               /* dac->G/cm conversion factor */
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
	gzlvl5 = getval("gzlvl5");

   P_getreal(GLOBAL,"BPpwrlimits",&BPpwrlimits,1);

    getstr("f1180",f1180);
    getstr("ZZ",ZZ);
    getstr("SE",SE);
    getstr("CT",CT);
    getstr("aliph",aliph);
    getstr("alphaC",alphaC);
    getstr("arom",arom);
    getstr("allC",allC);
    getstr("COrefoc",COrefoc);
    getstr("Crefoc_r",Crefoc_r);
    getstr("Crefoc_l",Crefoc_l);
    getstr("CH2only",CH2only);
    getstr("N15refoc",N15refoc);
    getstr("T1",T1);
    getstr("T1rho",T1rho);
    getstr("T2",T2);
    getstr("mag_flg",mag_flg);
    getstr("STUD",STUD);



/*   LOAD PHASE TABLE    */

	settable(t3,2,phi3);
   if (ZZ[A]=='y')
       {settable(t1,8,phi1);
	settable(t4,4,phi4);
	settable(t11,8,rec1);}
   if (SE[A]=='y')
       { settable(t1,1,phi2);
	 settable(t9,8,phi9);
	 settable(t10,1,phi10);
	 settable(t11,4,rec2);	
         if(CT[A] == 'y') 
	 {tsadd(t10,2,4); 
	  tsadd(t11,2,4); icosel = -1; }
	 else icosel = 1;	
       }


/*   INITIALIZE VARIABLES   */

    /* optional refocusing of N15 coupling for N15 enriched samples */
	if (N15refoc[A]=='n')  pwN = 0.0;
        if (2.0*pw > 2.0*pwN) pwZa = 2.0*pw; else pwZa = 2.0*pwN;

    /* power level for C13 spinlock (90 degree pulse length calculated first) */
	slClvl = 1/(4.0*slCrf*sfrq/600.0) ;  	    
	slClvl = pwClvl - 20.0*log10(slClvl/pwC*compC);
	slClvl = (int) (slClvl + 0.5);

    /* reset calH and calC if the user forgets */
        if (ni>1.0) {calH=1.0; calC=1.0;}

    /* maximum fine power for pwC pulses */
	rf0 = 4095.0; 

			   /********************/

/*  ALIPHATIC spectrum only, centered on 35ppm. */
if (aliph[A]=='y')
   {/* 80 ppm STUD+ decoupling */
	strcpy(stCdec, "stCdec80");
 	stdmf = dmf80;
	studlvl = pwClvl + 20.0*log10(compC*pwC*4.0*rf80);
	studlvl = (int) (studlvl + 0.5);

    /* 80ppm sech/tanh inversion */
	rfst = (compC*4095.0*pwC*4000.0*sqrt((12.07*sfrq/600+3.85)/0.35));
	rfst = (int) (rfst + 0.5);
	dofa = dof;
	strcpy(stCshape, "stC80");

    /* 180 degree one-lobe sinc pulse on CO, null at Ca 118ppm away, dof at   */
    /*     35ppm, to refocus CO coupling during t1 for aliphatic spectrum     */
       rf10 = (compC*4095.0*pwC*2.0*1.65)/pwC10; /* needs 1.65 times more     */
       rf10 = (int) (rf10 + 0.5);                /* power than a square pulse */
       strcpy(offCshape, "offC10");

    /* 180 degree pulse on Cab at 35ppm, null at CO 139ppm away, for CT='y'   */
        pwC2 = sqrt(3.0)/(2.0*139.0*dfrq);
        rf2 = (compC*4095.0*pwC*2.0)/pwC2;
        rf2 = (int) (rf2 + 0.5);
        if( rf2 > 4095.0 ) rf2=4095.0;

    /*  12.5 ppm bandwidth sech/tanh inversion on Ca's with 90% inversion     */
    /*  points at 50 and 62.5ppm so as not to invert most Cb's 		      */
	rfst1 = (compC*4095.0*pwC*4000.0*sqrt((1.875*sfrq/600.0+0.31)/3.47));
	rfst1 = (int) (rfst1 + 0.5);
   }


/*  ALPHA C spectrum, centered on 56ppm */
if (alphaC[A]=='y')
   {/* 30 ppm STUD+ decoupling */
	strcpy(stCdec, "stCdec30");
 	stdmf = dmf30;
	studlvl = pwClvl + 20.0*log10(compC*pwC*4.0*rf30);
	studlvl = (int) (studlvl + 0.5);

    /* 30ppm sech/tanh inversion */
        rfst = (compC*4095.0*pwC*4000.0*sqrt((4.5*sfrq/600.0+3.85)/0.41));
	rfst = (int) (rfst + 0.5);
	dofa = dof + 21.0*dfrq;
	strcpy(stCshape, "stC30");

    /* 180 degree one-lobe sinc pulse on CO, null at Ca 118ppm away, dof at   */
    /*     56ppm, to refocus CO coupling during t1 for aliphatic spectrum     */
       rf10 = (compC*4095.0*pwC*2.0*1.65)/pwC10; /* needs 1.65 times more     */
       rf10 = (int) (rf10 + 0.5);                /* power than a square pulse */
       strcpy(offCshape, "offC9");

    /* 180 degree pulse on Ca at 56ppm, null at CO 118ppm away, for CT='y'    */
        pwC2 = sqrt(3.0)/(2.0*118.0*dfrq);
        rf2 = (compC*4095.0*pwC*2.0)/pwC2;
        rf2 = (int) (rf2 + 0.5);
        if( rf2 > 4095.0 ) rf2=4095.0;

    /*  50 ppm bandwidth sech/tanh inversion on Cb's with 3% inversion       */
    /*  cutoff at 50ppm so as not to invert Ca's.  Also used for a 2nd pulse  */
    /*  which is used to the left of the Ca's with a cutoff at 63ppm	      */
	rfst1 = (compC*4095.0*pwC*4000.0*sqrt((7.5*sfrq/600.0+0.47)/1.65));
	rfst1 = (int) (rfst1 + 0.5);
   }
 
   /* phase shift etc for the offC10/offC9 pulses at the middle of t1  */
        if(pwC10 > pwZa) pwZ = pwC10; else pwZ = pwZa;
	phshift10 = 325.0;


/*  AROMATIC spectrum only, centered on 125ppm */
if (arom[A]=='y')
   {/* 30 ppm STUD+ decoupling */
	strcpy(stCdec, "stCdec30");
 	stdmf = dmf30;
	studlvl = pwClvl + 20.0*log10(compC*pwC*4.0*rf30);
	studlvl = (int) (studlvl + 0.5);

    /* 30ppm sech/tanh inversion */
        rfst = (compC*4095.0*pwC*4000.0*sqrt((4.5*sfrq/600.0+3.85)/0.41));   
	rfst = (int) (rfst + 0.5);
	dofa = dof + 90.0*dfrq;
	strcpy(stCshape, "stC30");
    }


/*  TOTAL CARBON spectrum, centered on 70ppm */
if (allC[A]=='y')
   {/* 140 ppm STUD+ decoupling */
	strcpy(stCdec, "stCdec140");
 	stdmf = dmf140;
	studlvl = pwClvl + 20.0*log10(compC*pwC*4.0*rf140);
	studlvl = (int) (studlvl + 0.5);

    /* 200ppm sech/tanh inversion */
        rfst = (compC*4095.0*pwC*4000.0*sqrt((30.0*sfrq/600.0+7.0)/0.35));   
	rfst = (int) (rfst + 0.5);
	dofa = dof + 35.0*dfrq;
	strcpy(stCshape, "stC200");
	if ( 1.0/(4000.0*sqrt((30.0*sfrq/600.0+7.0)/0.35)) < pwC )
           { text_error( " Not enough C13 RF. pwC must be %f usec or less.\n", 
	    (1.0e6/(4000.0*sqrt((30.0*sfrq/600.0+7.0)/0.35))) ); psg_abort(1); }
   }



/* CHECK VALIDITY OF PARAMETER RANGES */

  if ((CT[A]=='y') && (ni/sw1 > CTdelay))
  { text_error( " ni is too big. Make ni equal to %d or less.\n",    
      ((int)(CTdelay*sw1)) ); psg_abort(1); }

  if (tCH < 2.0*pw + pwZ + WFG3_START_DELAY + SAPS_DELAY)
  { text_error( " tCH is too small. Make tCH equal to %f or more.\n",    
    (2.0*pw + pwZ + WFG3_START_DELAY + SAPS_DELAY) ); psg_abort(1); }

  if ( (tCH < gt4 + 2.0*GRADIENT_DELAY) && (((CH2only[A]=='y') && (ZZ[A]=='y')
         && (CT[A]=='n')) || (T1[A]=='y') || (T1rho[A]=='y') || (T2[A]=='y')) )
{ text_error( " tCH is smaller than gt4. Make tCH equal to %f or more OR make gt4 smaller than tCH.\n", (gt4+2.0*GRADIENT_DELAY)); psg_abort(1);}
			/***************/

  if((dm2[A] == 'y' || dm2[B] == 'y' || dm2[C] == 'y' ))
  { text_error("incorrect dec2 decoupler flags! Should be 'nnn' "); psg_abort(1); }

  if((dm[A] == 'y' || dm[B] == 'y'))
  { text_error("incorrect dec1 decoupler flags! Should be 'nny' "); psg_abort(1); }

  if( (dpwr > 52) && (dm[C]=='y'))
  { text_error("don't fry the probe, DPWR too large!  "); psg_abort(1); }

  if( pw > 50.0e-6 )
  { text_error("don't fry the probe, pw too high ! "); psg_abort(1); }
 
  if( pwN > 100.0e-6 )
  { text_error("don't fry the probe, pwN too high ! "); psg_abort(1); }



/*  CHOICE OF PULSE SEQUENCE */

  if ( ((ZZ[A]=='n') && (SE[A]=='n')) || ((ZZ[A]=='y') && (SE[A]=='y')) )
  { text_error("Choose EITHER no coherence gradients (ZZ='y') OR coherence gradients with SE='y'"); psg_abort(1);}


if(((aliph[A]=='y') && (alphaC[A]=='y')) || ((aliph[A]=='y') && (arom[A]=='y'))
|| ((aliph[A]=='y') && (allC[A]=='y'))  || ((alphaC[A]=='y') && (arom[A]=='y'))
|| ((alphaC[A]=='y') && (allC[A]=='y'))  || ((arom[A]=='y')  && (allC[A]=='y'))
|| ((aliph[A]=='n') && (alphaC[A]=='n')  &&  (arom[A]=='n')  && (allC[A]=='n')))
{text_error ("Choose  ONE  of  aliph='y'  OR  alphaC='y'  OR  arom='y'  OR  allC='y' ! "); psg_abort(1);}

if ( ((arom[A]=='y') || (allC[A]=='y')) && (COrefoc[A]=='y') )
{text_error("Refocusing of CO coupling not available for  arom='y'  or  allC='y'.Set COrefoc='n' or aliph='y' or alphaC='y'"); psg_abort(1);}


if ( ((SE[A]=='n') || (alphaC[A]=='n')) && (Crefoc_r[A]=='y') )
{text_error ("Refocusing of Cb coupling only available if SE='y' & alphaC='y' & CT='n'! "); psg_abort(1);}

if ( ((SE[A]=='y') && (Crefoc_l[A]=='y')) &&
         ((aliph[A]=='y') || (allC[A]=='y') || (arom[A]=='y') || (CT[A]=='y')) )
{text_error ("Refocusing of Cb coupling only available if SE='y' & alphaC='y' & CT='n' ! "); psg_abort(1);}

if ( ((ZZ[A]=='y') && (Crefoc_l[A]=='y')) && ((COrefoc[A]=='y') ||
         (alphaC[A]=='y') || (allC[A]=='y') || (arom[A]=='y') || (CT[A]=='y')) )
{text_error ("Refocusing of Ca coupling only if ZZ='y' &  aliph='y' & COrefoc-'n' & CT='n'"); psg_abort(1);}

if ( (CH2only[A]=='y') && ((arom[A]=='y') || (allC[A]=='y')) ) 
{ text_error ("Set  CH2only='n'  OR  aliph='y'  OR  alphaC='y' ! "); psg_abort(1); }


if ( (CH2only[A]=='y') && ((T1[A]=='y') || (T1rho[A]=='y') || (T2[A]=='y')) )
{ text_error ("CH2 only spectra not available with relaxation exps."); psg_abort(1);}



/*  RELAXATION TIMES AND FLAGS */  

/* evaluate maximum relaxT, relaxTmax chosen by the user */
  rTnum = getarray("relaxT", rTarray);
  relaxTmax = rTarray[0];
  for (rTcounter=1; rTcounter<rTnum; rTcounter++)
      if (relaxTmax < rTarray[rTcounter]) relaxTmax = rTarray[rTcounter];


/* compare relaxTmax with maxrelaxT */
  if (maxrelaxT > relaxTmax)  relaxTmax = maxrelaxT; 


if ( ((T1rho[A]=='y') || (T2[A]=='y')) && (relaxTmax > d1) )
{ text_error("Maximum relaxation time, relaxT, is greater than d1 ! ");
						 		    psg_abort(1); }
 
if ( ((T1[A]=='y') && (T1rho[A]=='y'))   ||   ((T1[A]=='y') && (T2[A]=='y')) ||
    ((T1rho[A]=='y') && (T2[A]=='y')) )
{ text_error("Choose only one relaxation measurement ! "); psg_abort(1); } 


if ( ((T1[A]=='y') || (T1rho[A]=='y') || (T2[A]=='y')) && 
           ((relaxT*200.0 - (int)(relaxT*200.0+gstab)) > 1.0e-6) )
 { text_error("Relaxation time, relaxT, must be zero or multiple of 5msec"); psg_abort(1);}
 

if ( ((T1rho[A]=='y') || (T2[A]=='y'))  &&  (relaxTmax > 0.25) && (ix==1) ) 
{ printf("WARNING, sample heating will result for relaxT > 0.25sec"); }


if ( ((T1rho[A]=='y') ||  (T2[A]=='y'))  &&  (relaxTmax > 0.5) ) 
{ text_error("relaxT greater than 0.5 seconds will heat sample"); psg_abort(1); }



/* PHASES AND INCREMENTED TIMES */

/*  Phase incrementation for hypercomplex 2D data, States-Haberkorn element */
    
    if(SE[A]=='y')
    {
      if (phase1 == 1) tsadd(t10,2,4); 
      else icosel = -icosel;   
      calG = 0.1;    
    }
    else if (phase1 == 2) tssub(t4,1,4); 


/*  Set up f1180  */
       tau1 = d2;
    if((f1180[A] == 'y') && (ni > 1.0)) 
	{ tau1 += ( 1.0 / (2.0*sw1) ); if(tau1 < 0.2e-6) tau1 = 0.0; }
    tau1 = tau1/2.0;


/* Calculate modifications to phases for States-TPPI acquisition          */
   if( ix == 1) d2_init = d2;
   t1_counter = (int) ( (d2-d2_init)*sw1 + 0.5 );
   if(t1_counter % 2) 
	{ tsadd(t3,2,4); tsadd(t11,2,4); }


/*  Correct inverted signals for CH2 only spectra, but leave CH only spectra */
/*  positive and uncorrected when tCH=0.0009				     */
   if ((CH2only[A]=='y') && (tCH > 0.00095))
      { tsadd(t3,2,4); }    


/* BEGIN PULSE SEQUENCE */

status(A);
    if (dm3[B]=='y') lk_sample();  			 /* for H2 decoupling */

	obspower(tpwr);
	decpower(pwClvl);
	decpwrf(rf0);
 	dec2power(pwNlvl);
	decoffset(dofa);
	txphase(zero);
        decphase(zero);
        dec2phase(zero);

	delay(d1);


/*  xxxxxxxxxxxxxxxxx  CONSTANT SAMPLE HEATING FROM C13 RF xxxxxxxxxxxxxxxxx  */

 if  (T1rho[A]=='y')
      { decpower(slClvl);
    	decrgpulse(relaxTmax-relaxT, zero, 0.0, 0.0);
    	decpower(pwClvl); }
	
 if  (T2[A]=='y')      
 	{ncyc = 8.0*100.0*(relaxTmax - relaxT);
         if (BPpwrlimits > 0.5)
          {
           decpower(pwClvl-3.0);    /* reduce for probe protection */
           pwC=pwC*compC*1.4;
          }
    	 if (ncyc > 0)
       	    {initval(ncyc,v1);
             loop(v1,v2);
       	     delay(0.3125e-3 - pwC);
      	     decrgpulse(2*pwC, zero, 0.0, 0.0);
      	     delay(0.3125e-3 - pwC);
            endloop(v2);}
         if (BPpwrlimits > 0.5)
          {
           decpower(pwClvl);         /* restore normal value */
           pwC=getval("pwC");
          }
 	}
/*  xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx  */
        rcvroff();
	decrgpulse(pwC, zero, 0.0, 0.0);  /*destroy C13 magnetization*/
	decrgpulse(pwC, zero, 0.0, 0.0);
	zgradpulse(gzlvl0, 0.5e-3);
	delay(gstab);
	decrgpulse(pwC, one, 0.0, 0.0);
	zgradpulse(0.7*gzlvl0, 0.5e-3);
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

   	rgpulse(calH*pw,zero,0.0,0.0);                 /* 1H pulse excitation */

   	dec2phase(zero);
	zgradpulse(1.4*gzlvl5, gt5);
	decpwrf(rfst);
	delay(lambda - gt5 - WFG2_START_DELAY - 0.5e-3 + 70.0e-6);

     /* coupling evol reduced by 140us using stC pulses. Also WFG2_START_DELAY*/
   	simshaped_pulse("", stCshape, 2.0*pw, 1.0e-3, zero, zero, 0.0, 0.0);

   	txphase(t1);
	zgradpulse(1.4*gzlvl5, gt5);
	decpwrf(rf0);
	delay(lambda - gt5 - 0.5e-3 + 70.0e-6);

 	rgpulse(pw, t1, 0.0, 0.0);
        zgradpulse(calG*gzlvl3, gt3);
	decphase(t3);
	delay(gstab);
   	decrgpulse(calC*pwC, t3, 0.0, 0.0);
	txphase(zero);
	decphase(zero);


/*  xxxxxxxxxxxxxxxxxx    OPTIONS FOR C13 RELAXATION    xxxxxxxxxxxxxxxxxxxx  */

if ( (T1[A]=='y') || (T1rho[A]=='y') || (T2[A]=='y') )
   {
    decphase(one);
    zgradpulse(0.7*gzlvl4, gt4);			/* 2.0*GRADIENT_DELAY */
    delay(tCH - gt4 - 2.0*GRADIENT_DELAY);

    simpulse(2.0*pw, 2.0*pwC, zero, one, 0.0, 0.0);

    zgradpulse(0.7*gzlvl4, gt4);			/* 2.0*GRADIENT_DELAY */
    delay(tCH - gt4 - 2.0*GRADIENT_DELAY);
    decrgpulse(pwC, one, 0.0, 0.0);

    obspower(tpwr-6.0);		
    rgpulse(0.7e-3, zero, 0.0, 0.0);
    zgradpulse(0.6*gzlvl0, gt0);
    obspower(tpwr);				       
    decphase(three);
    delay(gstab);
   }

		/*   xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx    */

if  (T1[A]=='y')
   {
    ncyc = (200.0*relaxT);
    initval(ncyc,v4);
    if (ncyc > 0)
	  {loop(v4,v5);
	   delay(2.5e-3 - pw);
    	   rgpulse(2.0*pw, two, 0.0, 0.0);
   	   delay(2.5e-3 - pw);
           endloop(v5);} 

    decrgpulse(pwC, three, 0.0, 0.0);
   }

		/*   xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx    */
			     /* Theory suggests 8.0 is better than 2PI as RF  */
			     /* field multiplier and experiment confirms this.*/
if  (T1rho[A]=='y')          /* Shift evolution of 2.0*pwC/PI for one pulse   */
   {		             /* at end left unrefocused as for normal sequence*/
    decrgpulse(pwC, three, 0.0, 0.0);
    delay(1.0/(8.0*slCrf) - 2.0*pwC/PI - pwC - 0.5e-6);
    decphase(zero);
    decrgpulse(pwC, zero, 0.5e-6, 0.0);
    decpower(slClvl);

    ncyc = (200.0*relaxT + 1.0);           /* minimum 5ms spinlock to dephase */
    initval(ncyc,v4);		           /*  spins not locked		      */
    if (ncyc > 0)
	  {loop(v4,v5);
           decrgpulse((2.5e-3-pw), zero, 0.0, 0.0);
           simpulse(2.0*pw, 2.0*pw, zero, zero, 0.0, 0.0);
           decrgpulse((2.5e-3-pw), zero, 0.0, 0.0);
           endloop(v5);} 

    decpower(pwClvl);	
    decrgpulse(pwC, zero, 0.0, 0.0);
    delay(1.0/(8.0*slCrf) + 2.0*pwC/PI - pwC);
   }

		/*   xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx    */

if  (T2[A]=='y')
   {
    if (BPpwrlimits > 0.5)
     {
      decpower(pwClvl-3.0);    /* reduce for probe protection */
      pwC=pwC*compC*1.4;
     }
    decrgpulse(pwC, three, 0.0, 0.0);
    decphase(zero);

    ncyc = 200.0*relaxT;
    initval(ncyc,v5);

    loop(v5,v6);

      initval(3.0,v7);
      loop(v7,v8);
       	delay(0.3125e-3 - pwC);
      	decrgpulse(2.0*pwC, zero, 0.0, 0.0);
      	delay(0.3125e-3 - pwC);
      endloop(v8);

      delay(0.3125e-3 - pwC);
      decrgpulse(2.0*pwC, zero, 0.0, 0.0);
      delay(0.3125e-3 - pwC - pw);

      rgpulse(2.0*pw, zero, 0.0, 0.0);

      delay(0.3125e-3 - pwC - pw );
      decrgpulse(2.0*pwC, zero, 0.0, 0.0);
      delay(0.3125e-3 - pwC);
  
      initval(3.0,v9);
      loop(v9,v10);
      	delay(0.3125e-3 - pwC);
      	decrgpulse(2.0*pwC, zero, 0.0, 0.0);
      	delay(0.3125e-3 - pwC);
      endloop(v10);

    endloop(v6);
    if (BPpwrlimits > 0.5)
     {
      decpower(pwClvl);    /* restore value */
      pwC=getval("pwC");
     }
   }



/*  ffffffffffffffffff   BEGIN NO COHERENCE GRADIENTS   ffffffffffffffffffff  */
     if (ZZ[A]=='y') {
/*  ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff  */



/*  xxxxxxxxxxxxxxxxxxx    OPTIONS FOR C13 EVOLUTION    xxxxxxxxxxxxxxxxxxxx  */
       	txphase(zero);
	decphase(zero);



     /*****************     CONSTANT TIME EVOLUTION      *****************/
      if (CT[A]=='y') {
     /***************/

    delay(CTdelay/2.0 - tau1);

if ( ((aliph[A]=='y') || (alphaC[A]=='y')) && (COrefoc[A]=='y') )
{   decpwrf(rf2);					       /* dummy delay */
    decpwrf(rf2);
    decrgpulse(pwC2, zero, 2.0e-6, 0.0);
    decpwrf(rf10);

    if ( (CH2only[A]=='y') || (T1[A]=='y') || (T1rho[A]=='y') || (T2[A]=='y') )
	  {delay(CTdelay/2.0 - tCH);
	   rgpulse(2.0*pw, zero, 0.0, 0.0);
	   delay(tCH - 2.0*pw - pwZ - WFG3_START_DELAY - SAPS_DELAY);
	   sim3shaped_pulse("", offCshape, "", 0.0, pwC10, 2.0*pwN, zero, 
           zero, zero, 0.0, 0.0);}			  /* WFG3_START_DELAY */

    else  {delay(CTdelay/2.0 - pwZ - WFG3_START_DELAY - SAPS_DELAY);      
           sim3shaped_pulse("", offCshape, "", 2.0*pw, pwC10, 2.0*pwN, zero, 
           zero, zero, 0.0, 0.0);}			  /* WFG3_START_DELAY */

    decpwrf(rf0);
    initval(phshift10, v10);
    decstepsize(1.0);
    dcplrphase(v10);  					        /* SAPS_DELAY */
}

else
{   decrgpulse(2.0*pwC, zero, 2.0e-6, 0.0);

    if ( (CH2only[A]=='y') || (T1[A]=='y') || (T1rho[A]=='y') || (T2[A]=='y') )
	  {delay(CTdelay/2.0 - tCH);
	   rgpulse(2.0*pw, zero, 0.0, 0.0);
	   delay(tCH - 2.0*pw - 2.0*pwN);
	   dec2rgpulse(2.0*pwN, zero, 0.0, 0.0);}

    else  {delay(CTdelay/2.0 - pwZa);
           sim3pulse(2.0*pw, 0.0, 2.0*pwN, zero, zero, zero, 0.0, 0.0);}
}
    delay(tau1);

     /***************/
		      }
     /********************************************************************/




     /*****************         NORMAL EVOLUTION         *****************/
      else            {
     /***************/

/*  Extra 1/2J evolution for non Constant Time option.  This is added to C13  */
/*  shift evolution in all other cases by shifting a 2pw pulse by tCH or 1/4J */

if ( (Crefoc_l[A]=='n') &&   
        ((CH2only[A]=='y') || (T1[A]=='y') || (T1rho[A]=='y') || (T2[A]=='y')) )
{
    decphase(zero);
    zgradpulse(gzlvl4, gt4);				/* 2.0*GRADIENT_DELAY */
    delay(tCH - gt4 - 2.0*GRADIENT_DELAY);

    simpulse(2.0*pw, 2.0*pwC, zero, zero, 0.0, 0.0);

    zgradpulse(gzlvl4, gt4);				/* 2.0*GRADIENT_DELAY */
    delay(tCH - gt4 - 2.0*GRADIENT_DELAY + 4.0*pwC/PI);
}           /* the 4.0*pwC/PI evolution subtracted below is refocused by 2pwC */


		   /***************************************/

if ( ((aliph[A]=='y') || (alphaC[A]=='y')) && (COrefoc[A]=='y') )
{
    if (tau1 > 0.0)      /* total 13C evolution equals d2 exactly */
       {      /* 2.0*pwC/PI compensates for shift evolution during pwC pulses */
	decpwrf(rf10);

        if(tau1 - 2.0*pwC/PI - WFG3_START_DELAY - 0.5*pwZ > 0.0)
	   {delay(tau1 - 2.0*pwC/PI - WFG3_START_DELAY - 0.5*pwZ);
	    sim3shaped_pulse("", offCshape, "", 2.0*pw, pwC10, 2.0*pwN, zero, 
            zero, zero, 0.0, 0.0);			  /* WFG3_START_DELAY */
	    initval(phshift10, v10);
	    decstepsize(1.0);
	    dcplrphase(v10);  				        /* SAPS_DELAY */
	    delay(tau1 - 2.0*pwC/PI  - SAPS_DELAY - 0.5*pwZ - 2.0e-6);}

        else
	   {initval(180.0, v10);
	    decstepsize(1.0);
	    dcplrphase(v10);  				        /* SAPS_DELAY */
            delay(2.0*tau1 - 4.0*pwC/PI - SAPS_DELAY - 2.0e-6);}

	decpwrf(rf0);
       }

    else      /* 13C evolution during pwC pulses refocused for 1st increment  */
       {decrgpulse(2.0*pwC, zero, 2.0e-6, 0.0);}
}

		   /***************************************/

else
{
  if ( ((aliph[A]=='y') && (Crefoc_l[A]=='y')) &&
        ((CH2only[A]=='y') || (T1[A]=='y') || (T1rho[A]=='y') || (T2[A]=='y')) )
       {
        delay(tau1);

  	dec2rgpulse(2.0*pwN, zero, 0.0, 0.0); 
        delay(tCH - 2.0*pwN - 2.0*pw);
        decpwrf(rfst1);
  	rgpulse(2.0*pw, zero, 0.0, 0.0);
    	if (tCH < 0.5*gt1 + 1.99e-4)  delay(0.5*gt1 + 1.99e-4 - tCH);
        decshaped_pulse("stC12_10", 10.0e-3, zero, 0.0, 0.0);
	decphase(one);
	decpwrf(rf0);
	delay(0.5*gt1 + 1.99e-4);

        delay(tau1);

	decrgpulse(2.0*pwC, one, 0.0, 0.0);
  
        decphase(zero);
	zgradpulse(gzlvl1, 0.5*gt1);    		/* 2.0*GRADIENT_DELAY */
        decpwrf(rfst1);
   	if (tCH > 0.5*gt1 + 1.99e-4)
               delay(tCH - 0.5*gt1 - 2.0*GRADIENT_DELAY);
   	else   delay(1.99e-4 - 2.0*GRADIENT_DELAY);
        decshaped_pulse("stC12_10", 10.0e-3, zero, 0.0, 0.00);
        decpwrf(rf0);
	zgradpulse(-gzlvl1, 0.5*gt1);      		/* 2.0*GRADIENT_DELAY */
        delay(1.99e-4 - 2.0*GRADIENT_DELAY - 2.0e-6);
       }


  else if ((aliph[A]=='y') && (Crefoc_l[A]=='y'))
       {
        delay(tau1);

    	sim3pulse(2.0*pw, 0.0, 2.0*pwN, zero, zero, zero, 0.0, 0.0);
	delay(0.5*gt1 + gstab - pwZa);
        decpwrf(rfst1);
 	decshaped_pulse("stC12_10", 10.0e-3, zero, 0.0, 0.0);
	decphase(one);
	decpwrf(rf0);
	delay(0.5*gt1 + gstab);

	delay(tau1);

	decrgpulse(2.0*pwC, one, 0.0, 0.0);

	decphase(zero);
    	zgradpulse(gzlvl1, 0.5*gt1);    		/* 2.0*GRADIENT_DELAY */
        decpwrf(rfst1);
        delay(gstab - 2.0*GRADIENT_DELAY);
   	decshaped_pulse("stC12_10", 10.0e-3, zero, 0.0, 0.0);
        decpwrf(rf0);
    	zgradpulse(-gzlvl1, 0.5*gt1);    		/* 2.0*GRADIENT_DELAY */
	delay(gstab - 2.0*GRADIENT_DELAY - 2.0e-6);
       }
  else
       { 
	      /* 2.0*pwC/PI compensates for shift evolution during pwC pulses */
      	if (tau1 > (0.5*pwZa + 2.0*pwC/PI + 2.0e-6))
           {delay(tau1 - 0.5*pwZa - 2.0*pwC/PI);
    	    sim3pulse(2.0*pw, 0.0, 2.0*pwN, zero, zero, zero, 0.0, 0.0);
    	    delay(tau1 - 0.5*pwZa - 2.0*pwC/PI - 2.0e-6);}

        else if (2.0*tau1 > (4.0*pwC/PI + 2.0e-6 + SAPS_DELAY))
           {initval(180.0, v10);
	    decstepsize(1.0);
	    dcplrphase(v10);  				        /* SAPS_DELAY */
            delay(2.0*tau1 - 4.0*pwC/PI - 2.0e-6 - SAPS_DELAY);}

    	else if ((arom[A]=='y') || (aliph[A]=='y') || (alphaC[A]=='y'))
	      /* 13C evolution during pwC pulses refocused for 1st increment  */
            {decrgpulse(2.0*pwC, zero, 2.0e-6, 0.0);}

    	else   			  /* 13C evolution can't be refocused across  */
            {initval(180.0, v10); /* whole spectrum for allC='y' for 1st inc. */
	     decstepsize(1.0);
	     dcplrphase(v10);}
       }
      
}
     /***************/
		      }
     /********************************************************************/




/*  xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx  */

        decphase(t4);
    	decrgpulse(pwC, t4, 2.0e-6, 0.0);
	dcplrphase(zero);
	zgradpulse(0.7*gzlvl3, gt3);
        if ( dm3[B] == 'y' )   /* turns off 2H decoupling  */
           {
           setstatus(DEC3ch, FALSE, 'c', FALSE, dmf3);
           dec3rgpulse(1/dmf3,three,2.0e-6,2.0e-6);
           dec3blank();
           lk_autotrig();   /* resumes lock pulsing */
           }
	delay(gstab);
	rgpulse(pw, zero, 0.0, 0.0);

	decphase(zero);
	zgradpulse(3.75*gzlvl5, gt5);
	decpwrf(rfst);
	delay(lambda - gt5 - WFG2_START_DELAY - 0.5e-3 + 70.0e-6);

     /* coupling evol reduced by 140us using stC pulses. Also WFG2_START_DELAY*/
   	simshaped_pulse("", stCshape, 2.0*pw, 1.0e-3, zero, zero, 0.0, 0.0);

	zgradpulse(3.75*gzlvl5, gt5);
	txphase(one);
        decphase(zero);
	decpwrf(rf0);
	delay(lambda - gt5 - 0.5e-3 + 70.0e-6);

	rgpulse(pw, one, 0.0, 0.0);
	zgradpulse(gzlvl0, gt0);
	txphase(zero);
	delay(gstab);
	rgpulse(pw, zero, 0.0, rof2);

if ((STUD[A]=='y') && (dm[C] == 'y'))
       {decpower(studlvl);
        decprgon(stCdec,1/stdmf, 1.0);
        decon();}
else	{decpower(dpwr);
	 status(C);}

/*  fffffffff   END NO COHERENCE GRADIENTS PULSE SEQUENCE    fffffffffffffff  */
     		     }
/*  ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff  */
 









/*  ffffffffffffffffffff   BEGIN SENSITIVITY ENHANCE   fffffffffffffffffffff  */
     if (SE[A]=='y') {
/*  ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff  */

/*  xxxxxxxxxxxxxxxxxxx    OPTIONS FOR C13 EVOLUTION    xxxxxxxxxxxxxxxxxxxx  */

        txphase(zero);
 

 /*****************     CONSTANT TIME EVOLUTION      *****************/
      if (CT[A]=='y') {
     /***************/

	initval(90.0, v9);
	decstepsize(1.0);
	dcplrphase(v9);
	decphase(t9);
	delay(CTdelay/2.0 - tau1);

if ( ((aliph[A]=='y') || (alphaC[A]=='y')) && (COrefoc[A]=='y') )
{   decpwrf(rf2);					       /* dummy delay */
    decpwrf(rf2);
    decrgpulse(pwC2, t9, 0.0, 0.0);
    dcplrphase(zero);
    decphase(zero);
    decpwrf(rf10);

    if ( (CH2only[A]=='y') || (T1[A]=='y') || (T1rho[A]=='y') || (T2[A]=='y') )
	 {
	  if ((tau1 < gt1 + gstab)
          && (tCH < gt1 + gstab + 2.0*pw + pwZ + WFG3_START_DELAY +SAPS_DELAY))
	       {delay(CTdelay/2.0  - gt1 - gstab - tCH);
                if (mag_flg[A] == 'y')    	   	/* 2.0*GRADIENT_DELAY */
                   {magradpulse(gzcal*gzlvl1, gt1);}
                else
                   {zgradpulse(gzlvl1, gt1);}
	        delay(gstab - 2.0*GRADIENT_DELAY);
	        rgpulse(2.0*pw, zero, 0.0, 0.0);
	        delay(tCH - 2.0*pw - pwZ - WFG3_START_DELAY - SAPS_DELAY);
	    	sim3shaped_pulse("", offCshape, "", 0.0, pwC10, 2.0*pwN, 
		zero, zero, zero, 0.0, 0.0);		  /* WFG3_START_DELAY */
	    	decpwrf(rf0);
            	decphase(t10);
	    	initval(phshift10, v10);  
	    	decstepsize(1.0);
	    	dcplrphase(v10);  			        /* SAPS_DELAY */
	        delay(tau1);}

          else if (tau1 < gt1 + gstab)
	       {delay(CTdelay/2.0 - tCH);
	        rgpulse(2.0*pw, zero, 0.0, 0.0);
                if (mag_flg[A] == 'y')    	   	/* 2.0*GRADIENT_DELAY */
                   {magradpulse(gzcal*gzlvl1, gt1);}
                else
                   {zgradpulse(gzlvl1, gt1);}
	        delay(gstab - 2.0*GRADIENT_DELAY);
	        delay(tCH - 2.0*pw-pwZ-WFG3_START_DELAY-SAPS_DELAY-gt1-gstab);
	    	sim3shaped_pulse("", offCshape, "", 0.0, pwC10, 2.0*pwN, 
		zero, zero, zero, 0.0, 0.0);		  /* WFG3_START_DELAY */
	    	decpwrf(rf0);
            	decphase(t10);
	    	initval(phshift10, v10); 
	    	decstepsize(1.0);
	    	dcplrphase(v10);  			        /* SAPS_DELAY */
	        delay(tau1);}

	  else {delay(CTdelay/2.0 - tCH);
	        rgpulse(2.0*pw, zero, 0.0, 0.0);
	        delay(tCH - 2.0*pw - pwZ - WFG3_START_DELAY - SAPS_DELAY);
	    	sim3shaped_pulse("", offCshape, "", 0.0, pwC10, 2.0*pwN, 
		zero, zero, zero, 0.0, 0.0);		  /* WFG3_START_DELAY */
	    	decpwrf(rf0);
            	decphase(t10);
	    	initval(phshift10, v10); 
	    	decstepsize(1.0);
	    	dcplrphase(v10);  			        /* SAPS_DELAY */
                delay(tau1 - gt1 - gstab);
                if (mag_flg[A] == 'y')    	   	/* 2.0*GRADIENT_DELAY */
                   {magradpulse(gzcal*gzlvl1, gt1);}
                else
                   {zgradpulse(gzlvl1, gt1);}
	        delay(gstab - 2.0*GRADIENT_DELAY);}
	 }
    else {
    	  if (tau1 < gt1 + gstab)
	       {delay(CTdelay/2.0-pwZ-WFG3_START_DELAY-SAPS_DELAY -gt1 -gstab);
                if (mag_flg[A] == 'y')    	   	/* 2.0*GRADIENT_DELAY */
                   {magradpulse(gzcal*gzlvl1, gt1);}
                else
                   {zgradpulse(gzlvl1, gt1);}
	    	delay(gstab - 2.0*GRADIENT_DELAY); 
	    	sim3shaped_pulse("", offCshape, "", 2.0*pw, pwC10, 2.0*pwN, 
		zero, zero, zero, 0.0, 0.0);		  /* WFG3_START_DELAY */
	    	decpwrf(rf0);
            	decphase(t10);
	    	initval(phshift10, v10);  
	    	decstepsize(1.0);
	    	dcplrphase(v10);  			        /* SAPS_DELAY */
	    	delay(tau1);}

    	  else {delay(CTdelay/2.0 - pwZ -WFG3_START_DELAY -SAPS_DELAY); 
	   	sim3shaped_pulse("", offCshape, "", 2.0*pw, pwC10, 2.0*pwN, 
		zero, zero, zero, 0.0, 0.0);		  /* WFG3_START_DELAY */
	    	decpwrf(rf0);
            	decphase(t10);
	    	initval(phshift10, v10);  
	    	decstepsize(1.0);
	    	dcplrphase(v10);  			        /* SAPS_DELAY */
            	delay(tau1 - gt1 - gstab);
                if (mag_flg[A] == 'y')    	   	/* 2.0*GRADIENT_DELAY */
                   {magradpulse(gzcal*gzlvl1, gt1);}
                else
                   {zgradpulse(gzlvl1, gt1);}
	    	delay(gstab - 2.0*GRADIENT_DELAY);}
	  }
}

else
{   decrgpulse(2.0*pwC, t9, 0.0, 0.0);
    dcplrphase(zero);
    decphase(t10);
		
    if ( (CH2only[A]=='y') || (T1[A]=='y') || (T1rho[A]=='y') || (T2[A]=='y') )
	 {
	  if ((tau1 < gt1 + gstab) && (tCH < gt1 + gstab + 2.0*pw + 2.0*pwN))
	       {delay(CTdelay/2.0  - gt1 - gstab - tCH);
                if (mag_flg[A] == 'y')    	   	/* 2.0*GRADIENT_DELAY */
                   {magradpulse(gzcal*gzlvl1, gt1);}
                else
                   {zgradpulse(gzlvl1, gt1);}
	        delay(gstab - 2.0*GRADIENT_DELAY);  
	        rgpulse(2.0*pw, zero, 0.0, 0.0);
	        delay(tCH - 2.0*pw - 2.0*pwN);
	        dec2rgpulse(2.0*pwN, zero, 0.0, 0.0);
	        delay(tau1);}

          else if (tau1 < gt1 + gstab)
	       {delay(CTdelay/2.0 - tCH);
	        rgpulse(2.0*pw, zero, 0.0, 0.0);
                if (mag_flg[A] == 'y')    	   	/* 2.0*GRADIENT_DELAY */
                   {magradpulse(gzcal*gzlvl1, gt1);}
                else
                   {zgradpulse(gzlvl1, gt1);}
	        delay(gstab - 2.0*GRADIENT_DELAY);
	        delay(tCH - 2.0*pw - 2.0*pwN - gt1 - gstab);  
	        dec2rgpulse(2.0*pwN, zero, 0.0, 0.0);
	        delay(tau1);}

	  else {delay(CTdelay/2.0 - tCH);
	        rgpulse(2.0*pw, zero, 0.0, 0.0);
	        delay(tCH - 2.0*pw - 2.0*pwN);
	        dec2rgpulse(2.0*pwN, zero, 0.0, 0.0); 
                delay(tau1 - gt1 - gstab);
                if (mag_flg[A] == 'y')    	   	/* 2.0*GRADIENT_DELAY */
                   {magradpulse(gzcal*gzlvl1, gt1);}
                else
                   {zgradpulse(gzlvl1, gt1);}
	        delay(gstab - 2.0*GRADIENT_DELAY);}
	 }
    else {
          if (tau1 < gt1 + gstab) 
               {delay(CTdelay/2.0 - pwZa - gt1 - gstab);
                if (mag_flg[A] == 'y')    	   	/* 2.0*GRADIENT_DELAY */
                   {magradpulse(gzcal*gzlvl1, gt1);}
                else
                   {zgradpulse(gzlvl1, gt1);}
	        delay(gstab - 2.0*GRADIENT_DELAY); 
	        sim3pulse(2.0*pw, 0.0, 2.0*pwN, zero, zero, zero, 0.0, 0.0);
	        delay(tau1);
	       } 
          else {delay(CTdelay/2.0 - pwZa);
	        sim3pulse(2.0*pw, 0.0, 2.0*pwN, zero, zero, zero, 0.0, 0.0);
                delay(tau1 - gt1 - gstab);
                if (mag_flg[A] == 'y')    	   	/* 2.0*GRADIENT_DELAY */
                   {magradpulse(gzcal*gzlvl1, gt1);}
                else
                   {zgradpulse(gzlvl1, gt1);}
	        delay(gstab - 2.0*GRADIENT_DELAY);}
	 }            
}


     /***************/
		      }
     /********************************************************************/




     /*****************         NORMAL EVOLUTION         *****************/
      else            {
     /***************/

decphase(zero);
delay(tau1);

if ( (CH2only[A]=='y') || (T1[A]=='y') || (T1rho[A]=='y') || (T2[A]=='y') )	
{
    if ( ((aliph[A]=='y') || (alphaC[A]=='y')) )
        {
         if (COrefoc[A]=='y')
	      {decpwrf(rf10); 				  /* WFG3_START_DELAY */
               sim3shaped_pulse("", offCshape, "", 0.0, pwC10, 2.0*pwN, zero, 
               zero, zero, 0.0, 0.0);
 	       initval(phshift10, v10);
	       decstepsize(1.0);
	       dcplrphase(v10);  				/* SAPS_DELAY */
	       delay(tCH - pwZ - 2.0*pw - WFG3_START_DELAY - SAPS_DELAY);}

	 else {decpwrf(rf10);  		        	       /* dummy delay */
               dec2rgpulse(2.0*pwN, zero, 0.0, 0.0); 
               delay(tCH - 2.0*pwN - 2.0*pw);}
          
	 if ((Crefoc_r[A]=='y') || (Crefoc_l[A]=='y'))
	      {decpwrf(rfst1);
  	       rgpulse(2.0*pw, zero, 0.0, 0.0);
    	       if (tCH < 0.5*gt1 + 1.99e-4)  delay(0.5*gt1 + 1.99e-4 - tCH);

	       if (Crefoc_l[A]=='y')
                   decshaped_pulse("stC50_5l", 5.0e-3, zero, 0.0, 0.0);
	       if (Crefoc_r[A]=='y')
                   decshaped_pulse("stC50_5r", 5.0e-3, zero, 0.0, 0.0);
	       delay(0.5*gt1 + 1.99e-4);}

         else {rgpulse(2.0*pw, zero, 0.0, 0.0);
    	       if (tCH < gt1 + 1.99e-4)  delay(gt1 + 1.99e-4 - tCH);}

         decphase(t9);
         delay(tau1);

	 if (alphaC[A]=='y')
              {decpwrf(rf2);
	       decrgpulse(pwC2, t9, 2.0e-6, 0.0);
               decpwrf(rf0);}
	 else {decpwrf(rf0);
	       decrgpulse(2.0*pwC, t9, 2.0e-6, 0.0);
               decpwrf(rf0);}   		               /* dummy delay */

	 if ((Crefoc_r[A]=='y') || (Crefoc_l[A]=='y'))
    	      {if (mag_flg[A] == 'y')    	   	/* 2.0*GRADIENT_DELAY */
                   {magradpulse(gzcal*gzlvl1, 0.5*gt1);}
               else
                   {zgradpulse(gzlvl1, 0.5*gt1);}
               decphase(zero);
               decpwrf(rfst1);
   	       if (tCH > 0.5*gt1 + 1.99e-4)
                      delay(tCH - 0.5*gt1 - 2.0*GRADIENT_DELAY);
   	       else   delay(1.99e-4 - 2.0*GRADIENT_DELAY);

	       if (Crefoc_l[A]=='y')
                   decshaped_pulse("stC50_5l", 5.0e-3, zero, 0.0, 0.0);
	       if (Crefoc_r[A]=='y')
                   decshaped_pulse("stC50_5r", 5.0e-3, zero, 0.0, 0.0);
               if (mag_flg[A] == 'y')    	   	/* 2.0*GRADIENT_DELAY */
                   {magradpulse(gzcal*gzlvl1, 0.5*gt1);}
               else
                   {zgradpulse(gzlvl1, 0.5*gt1);}
               delay(1.99e-4 - 2.0*GRADIENT_DELAY);}

	 else {if (mag_flg[A] == 'y')    	   	/* 2.0*GRADIENT_DELAY */
                   {magradpulse(gzcal*gzlvl1, gt1);}
               else
                   {zgradpulse(gzlvl1, gt1);}
   	       if (tCH > gt1 + 1.99e-4)
                      delay(tCH - gt1 - 2.0*GRADIENT_DELAY);
   	       else   delay(1.99e-4 - 2.0*GRADIENT_DELAY);}

   	 decphase(t10);
         decpwrf(rf0);   				       /* dummy delay */
	 delay(2.0e-6);
        }
    else
        {      		
   	 dec2rgpulse(2.0*pwN, zero, 0.0, 0.0);  	
   	 delay(tCH - 2.0*pwN - 2.0*pw);		  /* shifting 2pw by tCH means*/
   	 rgpulse(2.0*pw, zero, 0.0, 0.0);	  /* CH J evolution for 2*tCH */
   	 decphase(t9);
    	 if (tCH < gt1 + 1.99e-4)  delay(gt1 + 1.99e-4 - tCH);

   	 delay(tau1);

   	 decrgpulse(2.0*pwC, t9, 0.0, 0.0);

         if (mag_flg[A] == 'y')    	 	  	/* 2.0*GRADIENT_DELAY */
             {magradpulse(gzcal*gzlvl1, gt1);}
         else
             {zgradpulse(gzlvl1, gt1);}
   	 decphase(t10);
   	 if (tCH > gt1 + 1.99e-4)
	        delay(tCH - gt1 - 2.0*GRADIENT_DELAY);
   	 else   delay(1.99e-4 - 2.0*GRADIENT_DELAY);
   	}
}
			/****************************/
else
{
    if ( ((aliph[A]=='y') || (alphaC[A]=='y')) )
        {
         if (COrefoc[A]=='y')
	      {decpwrf(rf10); 				  /* WFG3_START_DELAY */
               sim3shaped_pulse("", offCshape, "", 2.0*pw, pwC10, 2.0*pwN, zero, 
               zero, zero, 0.0, 0.0);
 	       initval(phshift10, v10);
	       decstepsize(1.0);
	       dcplrphase(v10);  			 	/* SAPS_DELAY */
	       pwZ = pwZ - WFG3_START_DELAY - SAPS_DELAY;}

	 else {decpwrf(rf10);  		        	       /* dummy delay */
               sim3pulse(2.0*pw, 0.0, 2.0*pwN, zero, zero, zero, 0.0, 0.0); 
               pwZ = pwZa;}

	 if ((Crefoc_r[A]=='y') || (Crefoc_l[A]=='y'))
	     {delay(0.5*gt1 + gstab - pwZ);
              decpwrf(rfst1);
	       if (Crefoc_l[A]=='y')
                   decshaped_pulse("stC50_5l", 5.0e-3, zero, 0.0, 0.0);
	       if (Crefoc_r[A]=='y')
                   decshaped_pulse("stC50_5r", 5.0e-3, zero, 0.0, 0.0);
	      delay(0.5*gt1 + gstab);}

         else delay(gt1 + gstab - pwZ);
	
	 decphase(t9);
         delay(tau1);

	 if (alphaC[A]=='y')
              {decpwrf(rf2);
	       decrgpulse(pwC2, t9, 2.0e-6, 0.0);
               decpwrf(rf0);}
	 else {decpwrf(rf0);
	       decrgpulse(2.0*pwC, t9, 2.0e-6, 0.0);
               decpwrf(rf0);}   		               /* dummy delay */

	 if ((Crefoc_r[A]=='y') || (Crefoc_l[A]=='y'))
    	      {if (mag_flg[A] == 'y')    	   	/* 2.0*GRADIENT_DELAY */
                   {magradpulse(gzcal*gzlvl1, 0.5*gt1);}
               else
                   {zgradpulse(gzlvl1, 0.5*gt1);}
               decphase(zero);
               decpwrf(rfst1);
               delay(gstab - 2.0*GRADIENT_DELAY);
	       if (Crefoc_l[A]=='y')
                   decshaped_pulse("stC50_5l", 5.0e-3, zero, 0.0, 0.0);
	       if (Crefoc_r[A]=='y')
                   decshaped_pulse("stC50_5r", 5.0e-3, zero, 0.0, 0.0);
               if (mag_flg[A] == 'y')    	   	/* 2.0*GRADIENT_DELAY */
                   {magradpulse(gzcal*gzlvl1, 0.5*gt1);}
               else
                   {zgradpulse(gzlvl1, 0.5*gt1);}
               delay(gstab - 2.0*GRADIENT_DELAY);}

	 else {if (mag_flg[A] == 'y')    	   	/* 2.0*GRADIENT_DELAY */
                   {magradpulse(gzcal*gzlvl1, gt1);}
               else
                   {zgradpulse(gzlvl1, gt1);}
               delay(gstab - 2.0*GRADIENT_DELAY);}

   	 decphase(t10);
         decpwrf(rf0);   				       /* dummy delay */
	 delay(2.0e-6);
        }
    else
        {
	 sim3pulse(2.0*pw, 0.0, 2.0*pwN, zero, zero, zero, 0.0, 0.0);
 
	 decphase(t9);
         delay(gt1 + gstab - pwZa);
         delay(tau1);

	 decrgpulse(2.0*pwC, t9, 0.0, 0.0);

         if (mag_flg[A] == 'y')    	  	 	/* 2.0*GRADIENT_DELAY */
             {magradpulse(gzcal*gzlvl1, gt1);}
         else
             {zgradpulse(gzlvl1, gt1);}
    	 decphase(t10);
    	 delay(gstab - 2.0*GRADIENT_DELAY);
	}
}


     /***************/
		      }
     /********************************************************************/




/*  xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx  */

	simpulse(pw, pwC, zero, t10, 0.0, 0.0);

	decphase(zero);
	zgradpulse(3.75*gzlvl5, gt5);
	delay(lambda - 1.5*pwC - gt5);

	simpulse(2.0*pw, 2.0*pwC, zero, zero, 0.0, 0.0);

	zgradpulse(3.75*gzlvl5, gt5);
	txphase(one);
	decphase(one);
	delay(lambda  - 1.5*pwC - gt5);

	simpulse(pw, pwC, one, one, 0.0, 0.0);

	txphase(zero);
	decphase(zero);
	zgradpulse(gzlvl5, gt5);
	delay(lambda - 1.5*pwC - gt5);

	simpulse(2.0*pw, 2.0*pwC, zero, zero, 0.0, 0.0);

	zgradpulse(gzlvl5, gt5);
	delay(lambda - pwC - 0.5*pw - gt5);

	rgpulse(pw, zero, 0.0, 0.0);

if ((STUD[A]=='y') && (dm[C] == 'y'))
{	delay((gt1/4.0) + 9.0e-4 - 0.5*pw +6.0*GRADIENT_DELAY +SAPS_DELAY +pwC);

        if ( dm3[B] == 'y' )   /* turns off 2H decoupling  */
           {
           setstatus(DEC3ch, FALSE, 'c', FALSE, dmf3);
           dec3rgpulse(1/dmf3,three,2.0e-6,2.0e-6);
           dec3blank();
           lk_autotrig();   /* resumes lock pulsing */
           }

	rgpulse(2.0*pw, zero, 0.0, 0.0);
	dcplrphase(zero);  				        /* SAPS_DELAY */
        if (mag_flg[A] == 'y')				/* 2.0*GRADIENT_DELAY */
            {magradpulse(icosel*gzcal*gzlvl2, gt1/4.0);}
        else
            {zgradpulse(icosel*gzlvl2, gt1/4.0);}
        delay(gstab - rof1);
	zgradpulse(gzlvl0, 3.0e-4);      		/* 2.0*GRADIENT_DELAY */
	delay(gstab);		
	decrgpulse(pwC, zero, 0.0, 0.0);
	zgradpulse(-gzlvl0, 3.0e-4);     		/* 2.0*GRADIENT_DELAY */
        rcvron();
	delay(gstab);		
  if (dm3[B] == 'y') delay(1/dmf3);
        decpower(studlvl);
        decprgon(stCdec,1/stdmf, 1.0);
        decon();
}
else
{	delay((gt1/4.0) + 1.0e-4 +gstab - 0.5*pw + 2.0*GRADIENT_DELAY + POWER_DELAY
        + SAPS_DELAY);

        if ( dm3[B] == 'y' )   /* turns off 2H decoupling  */
           {
           setstatus(DEC3ch, FALSE, 'c', FALSE, dmf3);
           dec3rgpulse(1/dmf3,three,2.0e-6,2.0e-6);
           dec3blank();
           lk_autotrig();   /* resumes lock pulsing */
           }

	rgpulse(2.0*pw, zero, 0.0, 0.0);
	dcplrphase(zero);  				        /* SAPS_DELAY */
	decpower(dpwr);					       /* POWER_DELAY */
        if (mag_flg[A] == 'y')				/* 2.0*GRADIENT_DELAY */
            {magradpulse(icosel*gzcal*gzlvl2, gt1/4.0);}
        else
            {zgradpulse(icosel*gzlvl2, gt1/4.0);}
        rcvron();
        delay(gstab);
statusdelay(C,1.0e-4);		
  if (dm3[B] == 'y') delay(1/dmf3);

}
		 
/*  fffffffffffffffffffff    END SENSITIVITY ENHANCE   fffffffffffffffffffff  */
     		}
/*  ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff  */




	setreceiver(t11);
}		 
