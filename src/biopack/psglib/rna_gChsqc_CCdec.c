/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
 /*  rna_gChsqc_CCdec

    HSQC with gradients for C13/H1 chemical shift correlation with optional
    N15 refocusing and numerous options for C13 decoupling, editing
    spectral regions, and measuring relaxation times.

    13C homonuclear decoupling in t1 and Carbon-Carbon Filter


    		      NOTE dof MUST BE SET AT 110ppm ALWAYS
                      NOTE dof2 MUST BE SET AT 200ppm ALWAYS

    
    THE ZZ OPTION [ZZ='y']:
    Suppresses H2O with gradients but does not use coherence gradients.  Must
    use ssfilter/ssntaps for good H2O suppression. It can be broadbanded to
    cover aromatic and ribose CHn groups by setting allC='y'. Alternatively,
    ribose='y' selects the dof for the pyrimidine H5 and ribose (H1' to H5'')
    region; and aromatic='y' selects the dof for the aromatic (H6/H8/H2) region.

    THE ALTERNATIVE SE OPTION [SE='y']:
    Uses coherence gradients and the Sensitivity Enhancement
    train for better H2O suppression for CH resonances very close to H2O. 
    A theoretical root2 CH 2D enhancement should be gained.
    CH2 groups lose up to 50% S/N relative to the ZZ option.

    The sequence will function correctly with ribose='y'/aromatic='y'/allC='y'.

    VNMR processing when SE='y':for VNMR5.3 or later, use f1coef='1 0 -1 0 0 -1 0 -1'
    and wft2da. For earlier versions of VNMR use these coefficients as arguments
    for wft2da. ( use f1coef='1 0 -1 0 0 1 0 1' if CT option is also used)

    THE CT OPTION: [CT='y']:
    This converts the t1 C13 shift evolution to Constant Time. Any combination 
    of ZZ='y'/SE='y' and ribose='y'/aromatic='y'/allC='y'can be used.
    The constant time delay, CTdelay(1/Jcc), can be set for optimum S/N for any type
    of groups, eg 28ms for ribose and 14ms for aromatics. For the allC option
    it is best to set the CTdelay to 1/Jcc(ribose) which equals 2/Jcc(aromatic).
    Note that in some options, CTdelays less than 8ms will generate an error
    message resulting from a negative delay.
  
    In all the above cases:
    * Selecting CH2only='y' will give a spectrum of only CH2 groups. 
    * A spectrum of only CH groups can be obtained as described in more detail 
    below.
    * N15 coupling in doubly-labelled samples is refocused by N15refoc='y'.
    * H2 coupling in triply-labelled samples is refocused by dm3='nyn'.
    * Efficient STUD+ decoupling is invoked with STUD='y' without need to 
    set any parameters.
    
    T1, T1rho, and T2 relaxation measurements of C13 nuclei can be made for any 
    combination of the above options, except for selecting CH2only='y'.

    1D checks of C13 and H1 pulse times can be made using calC and calH.  


    pulse sequence:	John, Plant and Hurd, JMR, A101, 113 (1992)
		 	Kay, Keifer and Saarinen, JACS, 114, 10663 (1992)
    relaxation times:   Kay et al, JMR, 97, 359 (1992)
			Farrow et al, Biochemistry, 33, 5984 (1994)
			Yamazaki et al, JACS, 116, 8266 (1994)
    STUD+ decoupling    Bendall & Skinner, JMR, A124, 474 (1997) and in press
     
    Written by MRB, January 1998, starting with ghn_co from BioPack.


	@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
	@						       @
	@   Rewritten for RnaPack by Peter Lukavsky (10/98).   @
        @   Modified by Kwaku Dayie, Cleveland Clinic          @
        @   Modified for BioPack, GG, Varian 1/2008            @
           (see Kwaku Dayie, J.Biomol.NMR, 32, 129-139(2005))
	@                                                      @
	@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@


        	  CHOICE OF DECOUPLING AND 2D MODES

    Set dm = 'nny', dmm = 'ccg' (or 'ccw', or 'ccp') for 13C decoupling.
    Set dm2 = 'nny', dmm2 = 'ccg' (or 'ccw', or 'ccp') for 15N decoupling.
    Not for STUD='y' and allC='y'
    Set dm3 = 'nnn' for no 2H decoupling, or
	      'nyn'  and dmm3 = 'w' for 2H decoupling. 

    Must set phase = 1,2  for States-TPPI acquisition in t1 [C13].
    
    The flag f1180 should be set to 'y' if t1 is to be started at halfdwell
    time. This will give 90, -180 phasing in f1. If it is set to 'n' the 
    phasing should be 0,0 and will still give a perfect baseline.  Thus,
    set f1180='n' for (0,0) in C13.  f1180='y' is ignored if ni=0.



          	  DETAILED INSTRUCTIONS FOR USE OF rna_gChsqc_CCdec

         
    1. Obtain a printout of the Philosopy behind the RnaPack development,
       and General Instructions using the macro: 
                                      "printon man('RnaPack') printoff".
       These Detailed Instructions for rna_gChsqc may be printed using:
                                      "printon man('rna_gChsqc') printoff".
             
    2. Apply the setup macro "rna_gChsqc".  This loads the relevant parameter set
       and also sets ni=0 and phase=1 ready for a 1D spectral check.

    3. Centre H1 frequency on H2O (4.7ppm), N15 frequency on the aromatic N
       region (200 ppm), and C13 frequency on 110ppm.

    4. CHOICE OF C13 REGION:
       ribose='y' gives a spectrum of ribose/C5 resonances centered on dof=85ppm.
       This is a common usage.                               Set sw1 to 60ppm.

       aromatic='y' gives a spectrum of aromatic C2/C6/C8 groups.  dof is shifted
       automatically by the pulse sequence code to 145ppm.  Set sw1 to 30ppm.

       allC='y' gives a spectrum of ribose and aromatic resonances. dof is
       set by the code to 110ppm.                        Set sw1 to 110ppm.
       C13 shift during the 90 degree C13 pulses is not refocused for the
       first increment when ZZ='y' and allC='y' (use linear prediction).

    5. CALIBRATION OF pwC AND pw:
       calC and calH are multipliers of the first C13 and H1 90 degree pulses
       in the sequence.  They can be used to calibrate the pulses using a 1D
       spectrum.  Setting either calC or calH = 2.0 should give a S/N null (or 
       small dispersion signals) corresponding to the pulses being set at 180
       degrees.  Adjust pwC or pw, respectively, until this occurs.  An array
       of calC or calH = 1.8, 2.0, 2.2 is also convenient to judge the null at
       2.0.  calC and calH are automatically reset to 1 for 2D spectra if you   
       forget to do so.  Because of the width of the RNA ribose/aromatic C13 
       region, only the central portion of the H1 1D spectrum will be nulled
       when ribose='y' and calC=2.

    6. AMPLIFIER COMPRESSION AND PHASE SHIFT AT LOWER POWER:
       For optimum usage of the sequences in RnaPack, the parameter compC
       should be calibrated for the selected pwClvl.  Measure pwC as in point 5.
       Lower pwClvl by 6 dB (half RF amplitude) and measure pwC again calling
       it pwC'.  Then compC=pwC'/(2*pwC). If you do these calibrations using an
       RNA, it is best to utilise the aromatic region since dof needs to be
       close to the C13 resonances.
       If compC is less than 0.9 it is probably best to use a lower pwClvl so that 
       compC > 0.9 and amplifier compression does not need to be compensated so 
       heavily.
       Same procedure is used for compH using the rna_water sequence and compN
       using the rna_gNhsqc sequence. All values have to be written in the 
       rna_gChsqc parameter set and will be updated in the adequate sequences by
       the macro rna_pack2.

    7. N15 COUPLING:
       Splitting of resonances in the C13 dimension by N15 coupling in N15
       enriched samples is removed by setting N15refoc='y'.  No N15 RF is
       delivered in the pulse sequence if N15refoc='n'.  N15 parameters are
       listed in dg2.

    8. 1/4J DELAY TIMES:
       These are listed in dg/dg2 for possible change by the user. JCH is used
       to calculate the 1/4J time (lambda=0.94*1/4J) for H1 CH coupling evolution,
       tCH is the 1/4J time for C13 CH coupling evolution.
       Lambda is calculated a little lower (0.94) than the theoretical time to
       minimise relaxation, but tCH should be as close
       to the theoretical time as possible.  So for:
         ribose CH/CH2: 	  JCH=145Hz         tCH = 0.00172
         aromatic CH:       	  JCH=180Hz         tCH = 0.00139
	 allC:			  JCH=160Hz         tCH = 0.00156

    9. SPECTRAL EDITING FOR DIFFERENT CHn GROUPS.
       CH2only='y' provides spectra of just CH2 groups, by inserting two 1/4J
       periods of CH coupling evolution, where generally the parameter
			 tCH = 1/4J = 0.0018.
       CH groups are more accurately nulled using tCH = 0.00173. ZZ='y' is best
       for CH2's. CH2only flag cannot be used with relaxation time measurements.

       By setting tCH = 0.0009 with CH2only='y', CH2's are nulled.
       SE='y is best for CH spectra.

   10. T1 MEASUREMENTS OF CH GROUPS.  SET tCH = 0.00172 AND T1='y':
       An array of 1D or 2D spectra is obtained by arraying the relaxation time 
       parameter, relaxT, to a multiple of 5 milliseconds including zero.  
       relaxT corresponds exactly to the relaxation time of the C13 spins.  The 
       method uses 180 degree H1 pulses every 5ms during relaxT and transverse 
       magnetization is dephased by a gradient as according to Kay et al.
       SE='y' is best for CH's.  For aromatic CH's use tCH = 0.00139.

   11. T1 MEASUREMENTS OF CH2:
       Proceed as for point 10 but set tCH = 0.0009 for CH2 groups
       Unfortunately the H1 irradiation removes 
       spin order and about 50% S/N is lost for CH2.  Also 50% of
       CH intensity is retained so the overall result at relaxT=0 is a 
       normal 1D spectrum at about half intensity.  However accurate CH2 
       T1 times can be determined from resolved peaks in a 2D array.  
       ZZ='y' is best for CH2's.  

   12. T1rho MEASUREMENTS OF CHn GROUPS:
       Implemented by setting the flag T1rho='y' and arraying relaxT as above
       for T1.  relaxT corresponds exactly to the relaxation time of the C13 
       spins.  Spin lock power is set at 2.0 kHz for a 600 Mhz spectrometer,
       and scaled in proportion to other field strengths.  Increasing this RF
       (by changing the number 2000 in the DECLARE AND LOAD VARIABLES section
       of gChsqc.c) causes substantial sample or coil heating as indicated by 
       deterioration of the lock signal).  Care should be taken for relaxT
       times greater than 0.25 seconds, and times greater than 0.5 seconds are 
       automatically aborted.  A dummy period of spinlock RF is delivered at
       the beginning of each pulse sequence to ensure constant average sample 
       heating - the code determines this from the maximum relaxT you have set
       in your array of relaxT.  If T1rho measurements are to be made in 
       different experiments with different relaxTs, including arrays of 
       relaxT's, set the parameter maxrelaxT to the maximum relaxT you have
       set in all experiments - this will ensure constant average sample
       heating in all experiments.  maxrelaxT does not need to be set for a
       single array in a single exp.  The spectrum at relaxT=0 may be erroneous
       because of insufficient dephasing of unlocked spins.

   13. T2 MEASUREMENTS OF CH GROUPS:
       Implemented by setting the flag T2='y', but not recommended because 
       homonuclear C13 coupling invalidates the estimated relaxation time.
       Like T1rho, other parameters and flags are as for T1='y'.  Also, T2='y'
       delivers more sample heating than T1rho='y' for the same relaxation
       time.

   14. STUD DECOUPLING.   SET STUD='y':
       Setting the flag STUD='y' overrides the decoupling parameters listed in
       dg2 and applies STUD+ decoupling instead.  In consequence it is easy
       to swap between the decoupling scheme you have been using to STUD+ for
       an easy comparison.  The STUD+ waveforms are calculated for your field
       strength at the time of RnaPack installation and RF power is 
       calculated within the pulse sequence.  The calculations are for the most 
       efficient conditions to cover 140ppm when allC='y' with all decoupled 
       peaks being greater than 85% of ideal; 80ppm/90% for ribose='y' and 
       aromatic='y'.  If you wish to compare different
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

15.    Band selective 13C decoupling is possible, along with Carbon-Carbon
       filtering. Waveforms are created by the macro BPrna_CCdec once all other
       parameters are set. Homonuclear C13 decoupling is done during t1, decoupling
       C2', C5 and aromatic carbonyls. Carbon-Carbon filtering is optional. If 
       CCfilter='y' and CCfilter='n' a filtering period is done prior to t1 evolution
       in which C2', C5 and aromatic carbonyls are decoupled for a period of 1/J(C5C6). 
       If CCfilter='n' and CCfilter2='y' only C2' and aromatics are decoupled.
       Data sets can be added or subtracted to obtain purine and pyrimidine spectra,
       respectively (see K.Dayie, J. Biomol.NMR, 32, 129-139(2005).
  
       Modified by Kwaku Dayie (Cleveland Clinic) from rna_gChsqc as rna_gChsqcdec_sel.c
       Modified by GG, Varian, for BioPack (1/2008) 
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
            mag_flg[MAXSTR],                  /* magic-angle gradient flag    */
	    ZZ[MAXSTR],				    /* no coherence gradients */
	    SE[MAXSTR],		 /* coherence gradients & sensitivity enhance */
	    CT[MAXSTR],				       /* constant time in t1 */
	    ribose[MAXSTR],	 		    /* ribose CHn groups only */
	    aromatic[MAXSTR], 			  /* aromatic CHn groups only */
	    allC[MAXSTR], 		        /* ribose and aromatic groups */
            rna_stCshape[MAXSTR],     /* calls sech/tanh pulses from shapelib */
	    rna_stCdec[MAXSTR],	       /* calls STUD+ waveforms from shapelib */

        CCdseq[MAXSTR],
        CCfdseq[MAXSTR],
        CCf2dseq[MAXSTR],
        CCfilter[MAXSTR],
        CCfilter2[MAXSTR],
        CChomodec[MAXSTR],                                /* Setup for C-imino - C-H6 */

	    CH2only[MAXSTR],		       /* spectrum of only CH2 groups */
	    N15refoc[MAXSTR],		 	 /* N15 pulse in middle of t1 */
	    T1[MAXSTR],				/* insert T1 relaxation delay */
	    T1rho[MAXSTR],		     /* insert T1rho relaxation delay */
	    T2[MAXSTR],				/* insert T2 relaxation delay */
	    STUD[MAXSTR];   /* apply automatically calculated STUD decoupling */
 
int         icosel,          			  /* used to get n and p type */
            t1_counter,  		        /* used for states tppi in t1 */
	    rTnum,			/* number of relaxation times, relaxT */
	    rTcounter;		    /* to obtain maximum relaxT, ie relaxTmax */

double      tau1,         				          /* t1 delay */

        CCdpwr = getval("CCdpwr"),    /*   power level for CC decoupling */
        CCdres = getval("CCdres"),    /*   dres for CC decoupling */
        CCdmf = getval("CCdmf"),      /*   dmf for CC decoupling */
/* parameters for CC filtering including C5 */
        CCfdpwr = getval("CCfdpwr"),    /*   power level for CC filtering */
        CCfdres = getval("CCfdres"),    /*   dres for CC filtering */
        CCfdmf = getval("CCfdmf"),      /*   dmf for CC filtering */
/* parameters for CC filtering not including C5 */
        CCf2dpwr = getval("CCf2dpwr"),    /*   power level for CC filtering */
        CCf2dres = getval("CCf2dres"),    /*   dres for CC filtering */
        CCf2dmf = getval("CCf2dmf"),      /*   dmf for CC filtering */

	    lambda = 0.94/(4*getval("JCH")),	   /* 1/4J H1 evolution delay */
	    tCH = getval("tCH"),		  /* 1/4J C13 evolution delay */
	    CTdelay = getval("CTdelay"),     /* total constant time evolution */
	    fdelay = 1.0/(2*getval("JC5C6")),     /* filter delay 1/2JCC */
	    relaxT = getval("relaxT"),		     /* total relaxation time */
	    rTarray[1000], 	    /* to obtain maximum relaxT, ie relaxTmax */
            maxrelaxT = getval("maxrelaxT"),    /* maximum relaxT in all exps */
            stdmf,                                 /* dmf for STUD decoupling */

   pwClvl = getval("pwClvl"), 	  	        /* coarse power for C13 pulse */
   pwC = getval("pwC"),     	      /* C13 90 degree pulse length at pwClvl */
   rfC,            	          /* maximum fine power when using pwC pulses */
   calC = getval("calC"),        /* multiplier on a pwC pulse for calibration */

/* Sech/tanh inversion pulses automatically calculated by macro "rna_cal"     */
/* and string parameter rna_stCshape calls them from your shapelib.           */
   rfst = 0.0,          /* fine power for the rna_stCshape pulse, initialised */
   dofa,       /* dof shifted to 85 or 145ppm for ribose and aromatic spectra */

/* STUD+ waveforms automatically calculated by macro "rna_cal" */
/* and string parameter rna_stCdec calls them from your shapelib. */
   studlvl,	                         /* coarse power for STUD+ decoupling */
   rf140 = getval("rf140"), 			 /* rf in Hz for 140ppm STUD+ */
   dmf140 = getval("dmf140"), 			      /* dmf for 140ppm STUD+ */
   rf80 = getval("rf80"), 			  /* rf in Hz for 80ppm STUD+ */
   dmf80 = getval("dmf80"), 			       /* dmf for 80ppm STUD+ */
   rf30 = getval("rf30"),                         /* rf in Hz for 30ppm STUD+ */
   dmf30 = getval("dmf30"),                            /* dmf for 30ppm STUD+ */


 pwZa,					 /* the largest of 2.0*pw and 2.0*pwN */

 compC = getval("compC"),         /* adjustment for C13 amplifier compression */
 slClvl,					   /* power for C13 spin lock */
 slCrf = 2000.0, 	       /* RF field in Hz for C13 spin lock at 600 MHz */

	calH = getval("calH"), /* multiplier on a pw pulse for H1 calibration */
 compH = getval("compH"),          /* adjustment for H1 amplifier compression */

	pwNlvl = getval("pwNlvl"),	              /* power for N15 pulses */
        pwN = getval("pwN"),          /* N15 90 degree pulse length at pwNlvl */
 compN = getval("compN"),         /* adjustment for N15 amplifier compression */


	sw1 = getval("sw1"),

 gstab = getval("gstab"),   /* Gradient recovery delay, typically 150-200us */

	gt1 = getval("gt1"),  		       /* coherence pathway gradients */
	gzlvl1 = getval("gzlvl1"),
	gzlvl2 = getval("gzlvl2"),

	gt3 = getval("gt3"),
	gt4 = getval("gt4"),
	gt5 = getval("gt5"),
	gzlvl0 = getval("gzlvl0"),
	gzlvl3 = getval("gzlvl3"),
	gzlvl5 = getval("gzlvl5"),
	gzlvl6 = getval("gzlvl6"),
        gzcal  = getval("gzcal");

    getstr("f1180",f1180);
    getstr("mag_flg",mag_flg);
    getstr("ZZ",ZZ);

  getstr("CCdseq",CCdseq);
  getstr("CCfdseq",CCfdseq);
  getstr("CCfilter",CCfilter);
  getstr("CCf2dseq",CCf2dseq);
  getstr("CCfilter2",CCfilter2);
  getstr("CChomodec",CChomodec);

    getstr("SE",SE);
    getstr("CT",CT);
    getstr("ribose",ribose);
    getstr("aromatic",aromatic);
    getstr("allC",allC);
    getstr("CH2only",CH2only);
    getstr("N15refoc",N15refoc);
    getstr("T1",T1);
    getstr("T1rho",T1rho);
    getstr("T2",T2);
    getstr("STUD",STUD);



/*   LOAD PHASE TABLE    */

	settable(t3,2,phi3);
   if (ZZ[A]=='y')
       {settable(t1,8,phi1);
	settable(t4,4,phi4);
	settable(t11,8,rec1);}
   if (SE[A]=='y')
       {settable(t1,1,phi2);
	settable(t9,8,phi9);
	settable(t10,1,phi10);
	settable(t11,4,rec2);}



/*   INITIALIZE VARIABLES   */

/* optional refocusing of N15 coupling for N15 enriched samples */
  if (N15refoc[A]=='n')  pwN = 0.0;
  if (2.0*pw > 2.0*pwN) pwZa = 2.0*pw;
  else pwZa = 2.0*pwN;

/* power level for C13 spinlock (90 degree pulse length calculated first) */
	slClvl = 1/(4.0*slCrf*sfrq/600.0);  	    
	slClvl = pwClvl - 20.0*log10(slClvl/pwC*compC);
	slClvl = (int) (slClvl + 0.5);

/* reset calH and calC if the user forgets */
  if (ni>1.0)
   {
	calH=1.0; calC=1.0;
   }

/* maximum fine power for pwC pulses */
	rfC = 4095.0; 

/* compH and compN not used in this sequence */
	compH = compH;
	compN = compN;

stdmf=dmf80; dofa=dof; studlvl=0.0;

/*  RIBOSE spectrum only, centered on 85ppm. */
  if (ribose[A]=='y')
   {
        dofa = dof - 25.0*dfrq;

        /* 80 ppm STUD+ decoupling */
        strcpy(rna_stCdec, "stCdec80");
        stdmf = dmf80;
        studlvl = pwClvl + 20.0*log10(compC*pwC*4.0*rf80);
        studlvl = (int) (studlvl + 0.5);

        /* 50ppm sech/tanh inversion */
        rfst = (compC*4095.0*pwC*4000.0*sqrt((7.5*sfrq/600+3.85)/0.41));
        rfst = (int) (rfst + 0.5);
        strcpy(rna_stCshape, "rna_stC50");
   }

/*  AROMATIC spectrum only, centered on 145ppm */
  if (aromatic[A]=='y')
   {
        dofa = dof + 35*dfrq;

        /* 30 ppm STUD+ decoupling */
        strcpy(rna_stCdec, "stCdec30");
        stdmf = dmf30;
        studlvl = pwClvl + 20.0*log10(compC*pwC*4.0*rf30);
        studlvl = (int) (studlvl + 0.5);

        /* 30ppm sech/tanh inversion */
        rfst = (compC*4095.0*pwC*4000.0*sqrt((4.5*sfrq/600.0+3.85)/0.41));
        rfst = (int) (rfst + 0.5);
        strcpy(rna_stCshape, "rna_stC30");
   }

/*  TOTAL CARBON spectrum, centered on 110ppm */
  if (allC[A]=='y')
   {
	dofa = dof;

	/* 140 ppm STUD+ decoupling */
	strcpy(rna_stCdec, "stCdec140");
        stdmf = dmf140;
        studlvl = pwClvl + 20.0*log10(compC*pwC*4.0*rf140);
        studlvl = (int) (studlvl + 0.5);

        /* 140ppm sech/tanh inversion */
        rfst = (compC*4095.0*pwC*4000.0*sqrt((21.0*sfrq/600.0+7.0)/0.35));
        rfst = (int) (rfst + 0.5);
        strcpy(rna_stCshape, "rna_stC140");
        if ( 1.0/(4000.0*sqrt((21.0*sfrq/600.0+7.0)/0.35)) < pwC )
           { text_error( " Not enough C13 RF. pwC must be %f usec or less.\n",
            (1.0e6/(4000.0*sqrt((21.0*sfrq/600.0+7.0)/0.35))) );    psg_abort(1); }
   }

/* CHECK VALIDITY OF PARAMETER RANGES */

  if ((CT[A]=='y') && (ni/sw1 > CTdelay))
  { text_error( " ni is too big. Make ni equal to %d or less.\n",    
      ((int)(CTdelay*sw1)) );					    psg_abort(1); }

  if ( (tCH < gt4 + 2.0*GRADIENT_DELAY) && (((CH2only[A]=='y') && (ZZ[A]=='y')
         && (CT[A]=='n')) || (T1[A]=='y') || (T1rho[A]=='y') || (T2[A]=='y')) )
  { text_error( " tCH is smaller than gt4. Make tCH equal to %f or more,OR make gt4 smaller than tCH.\n", (gt4+2.0*GRADIENT_DELAY)); psg_abort(1);}
			/***************/

  if((dm3[A] == 'y' || dm3[C] == 'y' ))
  { text_error("incorrect dec1 decoupler flags! Should be 'nyn' or 'nnn' "); psg_abort(1); }

  if((dm2[A] == 'y' || dm2[B] == 'y'))
  { text_error("incorrect dec2 decoupler flags! Should be 'nnn' or 'nny' "); psg_abort(1); }

  if((dm[A] == 'y' || dm[B] == 'y'))
  { text_error("incorrect dec1 decoupler flags! Should be 'nny' "); psg_abort(1); }

  if( (((dm[C] == 'y') && (dm2[C] == 'y')) && (STUD[A] == 'y')) )
  { text_error("incorrect dec2 decoupler flags! Should be 'nnn' if STUD='y'"); psg_abort(1); }

  if( (((dm[C] == 'y') && (dm2[C] == 'y')) && (allC[A] == 'y')) )
  { text_error("incorrect dec2 decoupler flags! Should be 'nnn' if allC='y'"); psg_abort(1); }

  if( dpwr > 50 )
  { text_error("don't fry the probe, DPWR too large!  ");   	    psg_abort(1); }

  if( dpwr2 > 50 )
  { text_error("don't fry the probe, DPWR2 too large!  ");           psg_abort(1); }

  if( (pw > 20.0e-6) && (tpwr > 56) )
  { text_error("don't fry the probe, pw too high ! ");              psg_abort(1); }
 
  if( (pwC > 40.0e-6) && (pwClvl > 56) )
  { text_error("don't fry the probe, pwN too high ! ");             psg_abort(1); }

  if( (pwN > 100.0e-6) && (pwNlvl > 56) )
  { text_error("don't fry the probe, pwN too high ! ");             psg_abort(1); }


/*  CHOICE OF PULSE SEQUENCE */

  if ( ((ZZ[A]=='n') && (SE[A]=='n')) || ((ZZ[A]=='y') && (SE[A]=='y')) )
   { text_error("Choose EITHER no coherence gradients (ZZ='y'), OR coherence gradients (SE='y')"); psg_abort(1);}

  if ( ((ribose[A]=='y') && (allC[A]=='y')) || ((ribose[A]=='y') && (aromatic[A]=='y'))
       || ((aromatic[A]=='y') && (allC[A]=='y')) ) 
   { text_error("Choose  ONE  of  ribose='y'  OR  aromatic='y'  OR  allC='y' ! "); psg_abort(1);}

  if ( (CH2only[A]=='y') && ((aromatic[A]=='y') || (allC[A]=='y')) )
   { text_error (" Set  CH2only='n'  OR  ribose='y' ! ");	    psg_abort(1); }


  if ( (CH2only[A]=='y') && ((T1[A]=='y') || (T1rho[A]=='y') || (T2[A]=='y')) )
   { text_error ("CH2-only spectra not available with relaxation exps."); psg_abort(1);} 


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
   { text_error("Choose only one relaxation measurement ! "); psg_abort(1);}

  if ( ((T1[A]=='y') || (T1rho[A]=='y') || (T2[A]=='y')) && 
           ((relaxT*200.0 - (int)(relaxT*200.0+gstab/2)) > 1.0e-6) )
   { text_error("Relaxation time, relaxT, must be zero or multiple of 5msec"); psg_abort(1);}

  if ( ((T1rho[A]=='y') || (T2[A]=='y'))  &&  (relaxTmax > 0.25) && (ix==1) ) 
   { printf("WARNING, sample heating can produce a reduced lock level");} 

  if ( ((T1rho[A]=='y') ||  (T2[A]=='y'))  &&  (relaxTmax > 0.5) ) 
   { text_error("relaxT greater than 0.5 seconds will heat sample"); psg_abort(1);}


/* PHASES AND INCREMENTED TIMES */

/*  Phase incrementation for hypercomplex 2D data, States-Haberkorn element */
  if (ZZ[A]=='y')
   {
	if (phase1 == 2) 
	tssub(t4,1,4);
   }

	icosel=1;

  if (SE[A]=='y')
   {
	if (phase1 == 1) 
	{
		tsadd(t10,2,4);
		icosel = 1;
	}
	else icosel = -1;
   }

/*  Set up f1180  */
       tau1 = d2;
  if((f1180[A] == 'y') && (ni > 1.0)) 
   {
	tau1 += ( 1.0 / (2.0*sw1) );
	if(tau1 < 0.2e-6) tau1 = 0.0;
   }

	tau1 = tau1/2.0;

/* Calculate modifications to phases for States-TPPI acquisition          */
  if( ix == 1) d2_init = d2;

	t1_counter = (int) ( (d2-d2_init)*sw1 + 0.5 );

  if(t1_counter % 2) 
   {
	tsadd(t3,2,4);
	tsadd(t11,2,4);
   }

/*  Correct inverted signals for CH2 only spectra, but leave CH only spectra */
/*  positive and uncorrected when tCH=0.0009				     */
  if ((CH2only[A]=='y') && (tCH > 0.00095))
   {
	tsadd(t3,2,4);
   }

/* BEGIN PULSE SEQUENCE */

status(A);
 if (dm3[B]=='y') lk_sample();  			 /* for H2 decoupling */

	obspower(tpwr);
	decpower(pwClvl);
	decpwrf(rfC);
 	dec2power(pwNlvl);
	obsoffset(tof);
	decoffset(dofa);
	dec2offset(dof2);
	txphase(zero);
	decphase(zero);
        dec2phase(zero);

 if  ((T1rho[A]=='n') || (T2[A]=='n'))

	delay(d1);
        rcvroff();

status(B);

/*  xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx  */

  if (dm3[B]=='y') lk_hold();  				 /* for H2 decoupling */

	dec2rgpulse(pwN, zero, 0.0, 0.0);  /*destroy N15 and C13 magnetization*/
	decrgpulse(pwC, zero, 0.0, 0.0);
	zgradpulse(gzlvl0, 0.5e-3);
	delay(gstab/2);
	dec2rgpulse(pwN, one, 0.0, 0.0);
	decrgpulse(pwC, one, 0.0, 0.0);
	zgradpulse(0.7*gzlvl0, 0.5e-3);
	delay(5.0e-4);

   	rgpulse(calH*pw,zero,0.0,0.0);                 /* 1H pulse excitation */
   	decphase(zero);
	zgradpulse(gzlvl5, gt5);
        decpwrf(rfst);
        delay(lambda - gt5 - WFG2_START_DELAY - 0.5e-3 + 70.0e-6);

     /* coupling evol reduced by 140us using rna_stC pulses. Also WFG2_START_DELAY*/
        simshaped_pulse("", rna_stCshape, 2.0*pw, 1.0e-3, zero, zero, 0.0, 0.0);
        decphase(zero);

        txphase(t1);
        zgradpulse(gzlvl5, gt5);
	decpwrf(rfC);
        delay(lambda - gt5 - 0.5e-3 + 70.0e-6);

        rgpulse(pw, t1, 0.0, 0.0);

if (CCfilter[A]=='y')
{
	decrgpulse(pwC, zero, 0.0, 0.0);

	decpower(CCfdpwr); decphase(zero);
	decprgon(CCfdseq,1.0/CCfdmf,CCfdres);
	decon();  /* CC decoupling on */

	delay(fdelay);

	decoff(); decprgoff();        /* CC decoupling off */
	decpower(pwClvl);

	decrgpulse(2.0*pwC, zero, 0.0, 0.0);

	decpower(CCfdpwr); decphase(zero);
	decprgon(CCfdseq,1.0/CCfdmf,CCfdres);
	decon();  /* CC decoupling on */

	delay(fdelay);

	decoff(); decprgoff();        /* CC decoupling off */
	decpower(pwClvl);
 
	decrgpulse(pwC, zero, 0.0, 0.0);
}
if (CCfilter2[A]=='y')
{
	decrgpulse(pwC, zero, 0.0, 0.0);

	decpower(CCf2dpwr); decphase(zero);
	decprgon(CCf2dseq,1.0/CCf2dmf,CCf2dres);
	decon();  /* CC decoupling on */

	delay(fdelay);

	decoff(); decprgoff();        /* CC decoupling off */
	decpower(pwClvl);

	decrgpulse(2.0*pwC, zero, 0.0, 0.0);

	decpower(CCf2dpwr); decphase(zero);
	decprgon(CCf2dseq,1.0/CCf2dmf,CCf2dres);
	decon();  /* CC decoupling on */

	delay(fdelay);

	decoff(); decprgoff();        /* CC decoupling off */
	decpower(pwClvl);
 
	decrgpulse(pwC, zero, 0.0, 0.0);
}

        zgradpulse(gzlvl3, gt3);
        decphase(t3);
        delay(gstab);
        decrgpulse(calC*pwC, t3, 0.0, 0.0);
        txphase(zero);
        decphase(zero);


/*  ffffffffffffffffffff   BEGIN SENSITIVITY ENHANCE   fffffffffffffffffffff  */
     if (SE[A]=='y') {
/*  ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff  */

/*  xxxxxxxxxxxxxxxxxxx    OPTIONS FOR C13 EVOLUTION    xxxxxxxxxxxxxxxxxxxx  */

        txphase(zero);
 
if (CChomodec[A]=='y')
{

decpower(CCdpwr); decphase(zero);
decprgon(CCdseq,1.0/CCdmf,CCdres);
decon();  /* CC decoupling on */
}
decphase(zero);
delay(tau1);

	 sim3pulse(2.0*pw, 0.0, 2.0*pwN, zero, zero, zero, 0.0, 0.0);
 
	 decphase(t9);
         delay(gt1 + gstab - pwZa);
         delay(tau1);

if (CChomodec[A]=='y')
{
decoff(); decprgoff();        /* CC decoupling off */
decpower(pwClvl);
}
	 decrgpulse(2.0*pwC, t9, 0.0, 0.0);

         if (mag_flg[A] == 'y')
            magradpulse(icosel*gzcal*gzlvl1,gt1);
         else
    	  zgradpulse(icosel*gzlvl1, gt1);         	/* 2.0*GRADIENT_DELAY */
    	 decphase(t10);
    	 delay(gstab - 2.0*GRADIENT_DELAY);

		      }
     /********************************************************************/




/*  xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx  */

	simpulse(pw, pwC, zero, t10, 0.0, 0.0);

	decphase(zero);
	zgradpulse(gzlvl5, gt5);
	delay(lambda - 1.5*pwC - gt5);

	simpulse(2.0*pw, 2.0*pwC, zero, zero, 0.0, 0.0);

	zgradpulse(gzlvl5, gt5);
	txphase(one);
	decphase(one);
	delay(lambda  - 1.5*pwC - gt5);

	simpulse(pw, pwC, one, one, 0.0, 0.0);

	txphase(zero);
	decphase(zero);
	zgradpulse(gzlvl6, gt5);
	delay(lambda - 1.5*pwC - gt5);

	simpulse(2.0*pw, 2.0*pwC, zero, zero, 0.0, 0.0);

	zgradpulse(gzlvl6, gt5);
	delay(lambda - pwC - 0.5*pw - gt5);

	rgpulse(pw, zero, 0.0, 0.0);

if ((STUD[A]=='y') && (dm[C] == 'y'))
{	delay((gt1/4.0) + ((9*gstab)/2) - 0.5*pw +6.0*GRADIENT_DELAY +SAPS_DELAY +pwC);

   if(dm3[B] == 'y')			         /*optional 2H decoupling off */
        { dec3rgpulse(1/dmf3, three, 0.0, 0.0); dec3blank();
          setstatus(DEC3ch, FALSE, 'w', FALSE, dmf3);
          dec3blank(); }

	rgpulse(2.0*pw, zero, 0.0, 0.0);

	dcplrphase(zero);  				        /* SAPS_DELAY */
        if (mag_flg[A] == 'y')
          magradpulse(icosel*gzcal*gzlvl2,gt1/4.0);
        else
	zgradpulse(gzlvl2, gt1/4.0);      		/* 2.0*GRADIENT_DELAY */
	delay(gstab/2);		
	zgradpulse(gzlvl0, ((gstab*3)/2));      		/* 2.0*GRADIENT_DELAY */
	delay(gstab/2);		
	decrgpulse(pwC, zero, 0.0, 0.0);
	zgradpulse(-gzlvl0, ((gstab*3)/2));     		/* 2.0*GRADIENT_DELAY */
	delay(gstab/2);		
   if(dm3[B] == 'y') delay(1/dmf3);
	decpower(studlvl);
	decprgon(rna_stCdec, 1.0/stdmf, 1.0);
       	decon();
}
else
{	delay((gt1/4.0) + gstab/2 - 0.5*pw + 2.0*GRADIENT_DELAY + 2*POWER_DELAY
        + SAPS_DELAY);

   if(dm3[B] == 'y')			         /*optional 2H decoupling off */
        { dec3rgpulse(1/dmf3, three, 0.0, 0.0); dec3blank();
          setstatus(DEC3ch, FALSE, 'w', FALSE, dmf3);
          dec3blank(); }

	rgpulse(2.0*pw, zero, 0.0, 0.0);

	dcplrphase(zero);  				        /* SAPS_DELAY */
	decpower(dpwr);					       /* POWER_DELAY */
	dec2power(dpwr2);				      /* POWER_DELAY */
        if (mag_flg[A] == 'y')
          magradpulse(icosel*gzcal*gzlvl2,gt1/4.0);
        else
         zgradpulse(gzlvl2, gt1/4.0);         		/* 2.0*GRADIENT_DELAY */
	delay(gstab/2);		
   if(dm3[B] == 'y') delay(1/dmf3);
status(C);
}
		 
/*  fffffffffffffffffffff    END SENSITIVITY ENHANCE   fffffffffffffffffffff  */

	setreceiver(t11);
}		 
