/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* gNT1T2.c     Heteronuclear T1/T2 with TROSY option

	The pulse sequence is used to measure T1 or T2 of 15N with 
	TROSY selection of the slowest relaxing component.

	written by Youlin Xia, HKUST on Jan. 20, 1999.

        modified by Marco Tonelli @NMRFAM (October 2004) to include
	the possibility of measuring both T1 and T2 within the same
	sequence, the use of water flipback pulses throughout the 
	sequence instead of square selective pulses, option to use 
	watergate 3919 instead of soft-watergate during the last inept
	transfer, the possibility of turning on C13 refocusing in t1 
	and the f1180 flag for starting t1 at half dwell time.


                  CHOICE OF DECOUPLING AND 2D MODES

    Set dm = 'nnn', dmm = 'ccc'
    Set dm2 = 'nny', dmm2 = 'ccg' (or 'ccw', or 'ccp') for N15 decoupling.

    Must set phase = 1,2  for States-TPPI acquisition in t1 [N15].

    The flag f1180 should be set to 'y' if t1 is to be started at halfdwell
    time. This will give -90, 180 phasing in f1. If it is set to 'n' the
    phasing should be 0,0 and will still give a perfect baseline.  Thus,
    set f1180='n' for (0,0) in N15.  f1180='y' is ignored if ni=0.

    Must use f1coef = '0 1 0 1 1 0 -1 0' for processing T1 data and
    f1coef = '1 0 -1 0 0 -1 0 -1' for processing T2 data.
    (the same coefficients can be used for both T1 and T2 data, but then 
     the phase in f1 and/or f2 needs to be adjusted by +/-90 for T1 or T2 
     data depending on the coefficients used)


Note:   
        
    1. T1 MEASUREMENTS OF NH GROUPS:
       Implemented by setting the flag T1='y'.  An array of 1D
       spectra is obtained by arraying the relaxation time parameter,
       relaxT, to a multiple of 10 milliseconds.  relaxT corresponds
       exactly to the relaxation time of the N15 spins.  The method uses 180
       degree H1 pulses every 5ms during relaxT as according to Kay et al.
       Hard 180 1H pulses are used unless the HNshape flag is set to 'y' then 
       shaped pulses (shape_ss) are used to invert the NH resonances, without 
       disturbing H2O. 
       This shape is produced at installation of BioPack as "NH180", a cosine-
       modulated pulse with 180's +-4ppm from H2O. The pw_shpss is
       calculated by the setup macro "gNT1T2" to match the stored shape.
       Its power ,shss_pwr, is calculated in the pulse sequence if autosoft='y'. 

       Note: maxrelaxT as in point 2 is not used when T1='y'.

    2. T2 MEASUREMENTS OF NH GROUPS:
       Implemented by setting the flag T2='y' and arraying relaxT as above.
       relaxT corresponds exactly to the relaxation time of the N15 spins.
       Care should be taken for relaxT times greater than 0.25 seconds, and 
       times greater than 0.5 seconds are automatically aborted.
       The method is according to Kay et al, with N15 180's every 625 us, and
       H1 180's every 10 ms.  625us was used instead of approx 500us used by
       Kay et al to reduce sample heating.
       A dummy period of N15 180's is delivered at the beginning of each pulse 
       sequence to ensure constant average sample heating.

       If T2 measurements are to be made in different experiments with different 
       relaxTs as for 2D, including arrays of relaxT's, set the parameter 
       maxrelaxT to the maximum relaxT you have set in all experiments - this 
       will ensure constant average sample heating in all experiments.  
       maxrelaxT does not need to be set for a single array in a single exp.
        
    3. Splitting of resonances in the N15 dimension by C13 coupling in C13
       enriched samples can be removed by setting C13refoc='y'.

    4. H2O preservation is achieved according to Kay et al, except that a sinc
       one-lobe selective pulse is used to put H2O back along z.  
       H2O is cycled back to z as much as possible during t1, and during the 
       relaxation delays for the following relaxation time measurements.

       Watergate is used for solvent suppression during the last spin-echo.
       Hard 3919 watergate pulses can be used instead of the soft pulses by 
       setting the wtg3919 flag to 'y'

    5. Weak gradients during t1 evolution can be turned on by setting gt0, 
       gzlvl0 and gstab. Then, then the evolution time is long enough a
       pair of opposite sign gradients of lenght gt0 and strenght gzlvl0
       (+/-) are turned on with a gstab recovery delay after the last 
       gradient. To turn off these gradients either set gt0 or gzlvl0 equal
       to 0.0. (gradient strength below 500 are recommended)

       Note: gradients that are too short can not be executed by the
       spectrometer, resulting in :
	- the experiment will not run at all, with or without an error message
	  (depending on the software).
	- the experiment will run but the spectrometer will increase the 
	  short gradients to the minimum lenght without any warning, causing
	  a timing error in the first few points of the experiment (and baseline
	  distorsions in t1).


References:
	   
	1.   Neil A.F., ...., Lewis E.K., Biochemistry 33, 5984-6003(1994)
	   
	2.   K. Pervushin,R.Riek,G.Wider,K.Wuthrich,
	     Proc.Natl.Acad.Sci.USA,94,12366-71,1997.
	
	3.   G. Zhu, Y. Xia, L.K. Nicholson, and K.H. Sze,
	     J. Magn. Reson. 143, 423-426(2000).
	
Please quote Reference 3 if you use this pulse sequence
Modified for BioPack by GG, Palo Alto, May 2002
Added Modified Tonelli version to BioPack, Sept 2006
		
*/

#include <standard.h>

static double d2_init = 0.0, relaxTmax;
 
static int phi1_T1[8]  = {0,0,0,0,2,2,2,2},		phi1_T2[4]  = {1,2,3,0},
           phi2_T1[4]  = {1,0,3,2},			phi2_T2[2]  = {1,0},
           phi3[1]     = {1}, 
           phi4_T1[1]  = {2}, 				phi4_T2[4]  = {0},
           phi10_T1[8] = {0,2,0,2,0,2,0,2},		phi10_T2[4] = {0,2,0,2},

           rec_T1[8]   = {0,3,2,1,2,1,0,3},		rec_T2[4]   = {0,3,2,1};
            
           
void pulsesequence()
{
/* DECLARE VARIABLES */

 char        shape_ss[MAXSTR],
             autosoft[MAXSTR],          /* Flag for autocalculation of shape_ss power */
                                        /* (Only for the case of default "sinc" pulse)*/
             HNshape[MAXSTR],
             f1180[MAXSTR],   		      /* Flag to start t1 @ halfdwell */
             wtg3919[MAXSTR],		/* Flag for 3919 watergate */
	     T1[MAXSTR],		/* Flag for T1 relax measurement */
	     T2[MAXSTR],		/* Flag for T2 relax measurement */
	     C13refoc[MAXSTR];		/* Flag C13 decoupling during t1 */
             
 int	     t1_counter,
	     rTnum,			/* number of relaxation times, relaxT */
	     rTcounter;		    /* to obtain maximum relaxT, ie relaxTmax */

 double   
	tau1,				/* t1/2  */
	taua = getval("taua"),		/* 2.25ms  */
	taub = getval("taub"),		/* 2.75ms  */
	relaxT = getval("relaxT"),	/* relaxation delay must be multple of 10ms */
	rTarray[1000],			/* to obtain maximum relaxT, ie relaxTmax */
        maxrelaxT = getval("maxrelaxT"),/* maximum relaxT in all exps */
	ncyc,

        pwClvl = getval("pwClvl"),      /* coarse power for C13 pulse */
        pwC = getval("pwC"),            /* C13 90 degree pulse length at pwClvl */
	pwNlvl = getval("pwNlvl"),	/* coarse power for N15 pulses */
	pwN = getval("pwN"),		/* N15 90 degree pulse length at pwNlvl */
	compH= getval("compH"),
	pwHs = getval("pwHs"),		/* H1 90 degree pulse length at tpwrs */
	tpwrs,				/* power for the pwHs ("H2Osinc") pulse */
	tpwrsf_t = getval("tpwrsf_t"),	/* fine power for the pwHs ("H2Osinc") pulse */
	tpwrsf_a = getval("tpwrsf_a"),	/* fine power for the pwHs ("H2Osinc") pulse */
	tpwrsf_u = getval("tpwrsf_u"),	/* fine power for the pwHs ("H2Osinc") pulse */
	tpwrsf_d = getval("tpwrsf_d"),	/* fine power for the pwHs ("H2Osinc") pulse */
	phincr_t = getval("phincr_t"),	/* fine power for the pwHs ("H2Osinc") pulse */
	phincr_a = getval("phincr_a"),	/* fine power for the pwHs ("H2Osinc") pulse */
	phincr_u = getval("phincr_u"),	/* fine power for the pwHs ("H2Osinc") pulse */
	phincr_d = getval("phincr_d"),	/* fine power for the pwHs ("H2Osinc") pulse */
	pwHs_dly,
	wtg_dly,
	d3919 = getval("d3919"),

        shss_pwr=getval("shss_pwr"),             /* power for cos modulated NH pulses */
        pw_shpss=getval("pw_shpss"),

	gstab = getval("gstab"),
	gt0 = getval("gt0"),
	gt1 = getval("gt1"),
	gt2 = getval("gt2"),
	gt3 = getval("gt3"),
	gt4 = getval("gt4"),
	gt5 = getval("gt5"),
	gt6 = getval("gt6"),
	gt7 = getval("gt7"),
	gzlvl0 = getval("gzlvl0"),
	gzlvl1 = getval("gzlvl1"),
	gzlvl2 = getval("gzlvl2"),
	gzlvl3 = getval("gzlvl3"),
	gzlvl4 = getval("gzlvl4"),
	gzlvl5 = getval("gzlvl5"),
	gzlvl6 = getval("gzlvl6"),
	gzlvl7 = getval("gzlvl7");

/* LOAD VARIABLES */

  getstr("T1",T1);
  getstr("T2",T2);
  getstr("f1180",f1180);
  getstr("wtg3919",wtg3919);
  getstr("shape_ss",shape_ss);
  getstr("HNshape",HNshape);
  getstr("C13refoc",C13refoc);
  getstr("autosoft",autosoft);

  
/* selective H20 one-lobe sinc pulse */
    tpwrs = tpwr - 20.0*log10(pwHs/(compH*pw*1.69));   /* needs 1.69 times more */
    tpwrs = (int) (tpwrs);                   /* power than a square pulse */

    pwHs_dly = pwHs +WFG_START_DELAY +2.0e-6 +2.0*POWER_DELAY +2.0*SAPS_DELAY;
    if (tpwrsf_t < 4095.0) pwHs_dly = pwHs_dly +2.0*PWRF_DELAY;	/* if one flipback is calibrated */
    if (phincr_t != 0.0) pwHs_dly = pwHs_dly +2.0*SAPS_DELAY;	/* so will probably be the others */

    if (wtg3919[0] != 'y')
	wtg_dly = pwHs_dly;
    else
	wtg_dly = pw*2.385 +7.0*rof1 +d3919*2.5 +SAPS_DELAY;

/* selective cos modulated NH 180 degree pulse */
    if (autosoft[A] == 'y')
     {
      shss_pwr = tpwr - 20.0*log10(pw_shpss/((compH*2*pw)*2));	/* needs 2 times more */
      shss_pwr = (int) (shss_pwr);				/* power than a square pulse */
     }


/* evaluate maximum relaxT, relaxTmax chosen by the user */
  rTnum = getarray("relaxT", rTarray);
  relaxTmax = rTarray[0];
  for (rTcounter=1; rTcounter<rTnum; rTcounter++)
      if (relaxTmax < rTarray[rTcounter]) relaxTmax = rTarray[rTcounter];

/* compare relaxTmax with maxrelaxT */
  if (maxrelaxT > relaxTmax)  relaxTmax = maxrelaxT; 


/* check validity of parameter range */

    if ( (wtg3919[0] != 'y') && (d3919 == 0.0) )
      { d3919=0.00011; text_error("d3919 delay for 3919 watergate pulse set to 110us"); }

    if ( (T1[A]=='y') && (T2[A]=='y') ) 
      { text_error("Choose only one relaxation measurement ! "); psg_abort(1); } 

    if ( (T1[A]=='y') && ((relaxT*100.0 - (int)(relaxT*100.0+1.0e-4)) > 1.0e-6) )
      { text_error("Relaxation time, relaxT, must be zero or multiple of 5msec"); psg_abort(1);}

    if ( (T2[A]=='y') && (((relaxT+0.01)*50.0 - (int)((relaxT+0.01)*50.0+1.0e-4)) > 1.0e-6) )
      { text_error("Relaxation time, relaxT, must be odd multiple of 10msec"); psg_abort(1);}

    if ( (T2[A]=='y')  &&  (relaxTmax > 0.5) )
      { text_error("relaxT> 0.5 seconds will have undue sample heating from N15 irradiation  !"); psg_abort(1);}

    if ((dm[A] == 'y' || dm[B] == 'y' ))
      { text_error("incorrect Dec1 decoupler flags!  "); psg_abort(1); } 

    if (dm2[A] == 'y' || dm2[B] == 'y' || dm2[C] == 'y')
      { text_error("incorrect Dec2 decoupler flag! dm2 should be 'nnn' "); psg_abort(1); } 

    if (dpwr > 50)
      { text_error("don't fry the probe, dpwr too large!  "); psg_abort(1); }

    if (dpwr2 > 50)
      { text_error("don't fry the probe, dpwr2 too large!  "); psg_abort(1); }

    if (gt1 > 15.0e-3 || gt2 > 15.0e-3 || gt3 > 15.0e-3 || gt4 > 15.0e-3)
      { text_error("gti must be less than 15 ms \n"); psg_abort(1); }


/* LOAD VARIABLES */

   assign(one,v11);
   assign(three,v12);

   settable(t3, 1, phi3);

   if (T1[A] == 'y')
     {
      settable(t1, 8, phi1_T1);
      settable(t2, 4, phi2_T1);
      settable(t4, 1, phi4_T1);
      settable(t10, 8, phi10_T1);

      settable(t14, 8, rec_T1);
     }
   else if (T2[A] == 'y')
     {
      settable(t1, 4, phi1_T2);
      settable(t2, 2, phi2_T2);
      settable(t4, 1, phi4_T2);
      settable(t10, 4, phi10_T2);

      settable(t14, 4, rec_T2);
     }

  
/* Phase incrementation for hypercomplex data */

   if ( phase1 == 2 )     /* Hypercomplex in t1 */
      { ttadd(t14,t10,4); tsadd(t3,2,4); }

       
/* Calculate modification to phases based on current d2 values
   to achieve States-TPPI acquisition */
 
   if (ix == 1) d2_init = d2;
   t1_counter = (int) ( (d2-d2_init)*sw1 + 0.5);

   if (t1_counter %2) 
     { 
      if (T1[A]=='y') tsadd(t2,2,4); 
      if (T2[A]=='y') tsadd(t1,2,4); 
      tsadd(t14,2,4); 
     }

      
/* Set up f1180  */

   tau1 = d2;
   if((f1180[A] == 'y') && (ni > 1.0))
       { tau1 += ( 1.0 / (2.0*sw1) ); if(tau1 < 0.2e-6) tau1 = 0.0; }
   tau1 = tau1/2.0;
      
/* Set up phincr corrections  */
   if (phincr_t < 0)  phincr_t=phincr_t+360;
   if (phincr_a < 0)  phincr_a=phincr_a+360;
   if (phincr_u < 0)  phincr_u=phincr_u+360;
   if (phincr_d < 0)  phincr_d=phincr_d+360;
 
/* BEGIN ACTUAL PULSE SEQUENCE */

status(A);
   obspower(tpwr);               /* Set power for pulses  */
   decpower(pwClvl);             /* Set decoupler power to pwClvl */
   dec2power(pwNlvl);            /* Set decoupler2 power to pwNlvl */

   txphase(zero);
   decphase(zero);
   dec2phase(zero);

   obsstepsize(1.0);  


   delay(d1);
 
/*  xxxxxxxxxxxxxxxxx  CONSTANT SAMPLE HEATING FROM N15 RF xxxxxxxxxxxxxxxxx  */

 if  (T2[A]=='y')
        {ncyc = 8.0*100.0*(relaxTmax - relaxT);
         if (ncyc > 0)
            {initval(ncyc,v1);
             loop(v1,v2);
             delay(0.625e-3 - pwN);
             dec2rgpulse(2*pwN, zero, 0.0, 0.0);
             delay(0.625e-3 - pwN);
            endloop(v2);}
        }

/*  xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx  */


status(B);
   rcvroff();

/*destroy N15  magnetization*/	

   dec2rgpulse(pwN, zero, 0.0, 0.0);
   zgradpulse(gzlvl1, gt1);
   delay(9.0e-5);

/*  1H-15N INEPT  */

   rgpulse(pw, zero, 0.0, 0.0);    

   txphase(zero);  dec2phase(zero);
   zgradpulse(gzlvl2,gt2);
   delay(taua -pwN -0.5*pw -gt2 -2.0*GRADIENT_DELAY);	/* delay=1/4J(NH)   */

   sim3pulse(2.0*pw, 0.0, 2.0*pwN, zero, zero, zero, 0.0, 0.0);

   txphase(one);  dec2phase(t1);
   zgradpulse(gzlvl2,gt2);
   delay(taua -1.5*pwN -gt2 -2.0*GRADIENT_DELAY);	/* delay=1/4J(NH)   */

   rgpulse(pw, one, 0.0, 0.0);

   initval(phincr_t,v14);
   txphase(t4); if(phincr_t != 0) xmtrphase(v14);
   if (tpwrsf_t < 4095.0) { obspwrf(tpwrsf_t); obspower(tpwrs+6.0); }
     else obspower(tpwrs); 
   shaped_pulse("H2Osinc", pwHs, t4, 2.0e-6, 2.0e-6);   
   obspower(tpwr); 
   if (tpwrsf_t < 4095.0) obspwrf(4095.0);
   txphase(zero);   if(phincr_t != 0) xmtrphase(zero);

   zgradpulse(gzlvl7,gt7);
   delay(gstab);

   dec2rgpulse(pwN, t1, 0.0, 0.0);

   dec2phase(zero);
   zgradpulse(gzlvl3,gt3);
   delay(taub -1.5*pwN -gt3 -2.0*GRADIENT_DELAY);	/* delay=1/4J(NH)   */

   sim3pulse(2.0*pw, 0.0, 2.0*pwN, zero, zero, zero, 0.0, 0.0);

   zgradpulse(gzlvl3,gt3);
   delay(taub -1.5*pwN -gt3 -2.0*GRADIENT_DELAY);	/* delay=1/4J(NH)   */


/*  xxxxxxxxxxxxxxxxxx    OPTIONS FOR N15 RELAXATION    xxxxxxxxxxxxxxxxxxxx  */

if (T1[A] == 'y') 
  {
   dec2phase(one);
   dec2rgpulse(pwN,one,0.0,0.0);
  
   ncyc = (100.0*relaxT);
   initval(ncyc,v4);
   if (ncyc > 0)
     {
      if (HNshape[A]=='y') obspower(shss_pwr); 
      loop(v4,v5);
	 if (HNshape[A]=='n')
           {
	    txphase(two);
            delay(2.5e-3 -pw -SAPS_DELAY);
            rgpulse(2.0*pw, two, 0.0, 0.0);
            delay(2.5e-3 -pw);

	    txphase(zero);
            delay(2.5e-3 -pw -SAPS_DELAY);
            rgpulse(2.0*pw, zero, 0.0, 0.0);
            delay(2.5e-3 -pw);
	   }
	 else
	   {
	    txphase(two);
	    delay(2.5e-3 -0.5*pw_shpss -WFG_START_DELAY -SAPS_DELAY);
	    shaped_pulse(shape_ss, pw_shpss, two, 0.0, 0.0);
	    delay(2.5e-3 -0.5*pw_shpss);
      
	    txphase(zero);
	    delay(2.5e-3 -0.5*pw_shpss -WFG_START_DELAY -SAPS_DELAY);
	    shaped_pulse(shape_ss, pw_shpss, zero, 0.0, 0.0);
	    delay(2.5e-3 -0.5*pw_shpss);
           }
      endloop(v5);
      if (HNshape[A]=='y') obspower(tpwr);
     }
   zgradpulse(gzlvl6,gt6);
   dec2phase(t2);
   delay(gstab);

   dec2rgpulse(pwN,t2,0.0,0.0);
  }
else if (T2[A] == 'y')
  {
   dec2phase(t2);
   initval(0.0,v3);   initval(180.0,v4);

   ncyc = 100.0*relaxT;
   initval(ncyc,v5);

   loop(v5,v6);

     initval(3.0,v7);
     loop(v7,v8);
       delay(0.625e-3 - pwN);
       dec2rgpulse(2.0*pwN, t2, 0.0, 0.0);
       delay(0.625e-3 - pwN);
     endloop(v8);

     delay(0.625e-3 - pwN - SAPS_DELAY);
     add(v4,v3,v3);  xmtrphase(v3);         /* SAPS_DELAY */
     dec2rgpulse(2.0*pwN, t2, 0.0, 0.0);
     delay(0.625e-3 - pwN - pw);

     rgpulse(2*pw, zero, 0.0, 0.0);

     delay(0.625e-3 - pwN - pw );
     dec2rgpulse(2.0*pwN, t2, 0.0, 0.0);
     xmtrphase(zero);                                          /* SAPS_DELAY */
     delay(0.625e-3 - pwN - SAPS_DELAY);

     initval(3.0,v9);
     loop(v9,v10);
       delay(0.625e-3 - pwN);
       dec2rgpulse(2.0*pwN, t2, 0.0, 0.0);
       delay(0.625e-3 - pwN);
     endloop(v10);

   endloop(v6);
  }

/* T1 EVOLUTION BEGINS */
 
  txphase(t3); dec2phase(zero);

/* to turn off gradients during t1 evolution, set gt0 or gzlvl0 to 0.0 */
  if ( (C13refoc[A] != 'y') && (gt0*gzlvl0 > 0.0) && 
       ((2.0*tau1 -2.0*gt0 -2.0*GRADIENT_DELAY -2.0*SAPS_DELAY) > 0.0001) )
    {  
     zgradpulse(gzlvl0,gt0);
     delay(2.0*tau1 -2.0*gt0 -2.0*GRADIENT_DELAY -2.0*SAPS_DELAY);
     zgradpulse(-gzlvl0,gt0);
     delay(gstab -2.0*GRADIENT_DELAY);
    }
  else if ( (C13refoc[A] == 'y') && (gt0*gzlvl0 > 0.0) &&
            ((tau1 -2.0*pwC -3.0*SAPS_DELAY -gt0 -gstab) > 0.0) )
    {
     delay(tau1 -2.0*pwC -3.0*SAPS_DELAY -gt0 -gstab);
     zgradpulse(gzlvl0, gt0);
     delay(gstab -2.0*GRADIENT_DELAY);

     decrgpulse(pwC, zero, 0.0, 0.0);
     decphase(one);
     decrgpulse(2.0*pwC, one, 0.0, 0.0);
     decphase(zero);
     decrgpulse(pwC, zero, 0.0, 0.0);

     delay(tau1 -2.0*pwC -SAPS_DELAY -gt0 -gstab);
     zgradpulse(gzlvl0, gt0);
     delay(gstab -2.0*GRADIENT_DELAY);
    }
  else if ( (C13refoc[A] == 'y') && ((tau1 -2.0*pwC -3.0*SAPS_DELAY) > 0.0) )
    {
     delay(tau1 -2.0*pwC -3.0*SAPS_DELAY);

     decrgpulse(pwC, zero, 0.0, 0.0);
     decphase(one);
     decrgpulse(2.0*pwC, one, 0.0, 0.0);
     decphase(zero);
     decrgpulse(pwC, zero, 0.0, 0.0);

     delay(tau1 -2.0*pwC -SAPS_DELAY);
    }
  else if (tau1 > SAPS_DELAY)
     delay(2.0*tau1 -2.0*SAPS_DELAY);
  
/* T1 EVOLUTION ENDS */
  rgpulse(pw,t3,0.0,0.0);

  initval(phincr_a,v14);
  if (phincr_a != 0) xmtrphase(v14);
  if (tpwrsf_a < 4095.0) { obspwrf(tpwrsf_a); obspower(tpwrs+6.0); }
    else obspower(tpwrs); 
  shaped_pulse("H2Osinc", pwHs, t3, 2.0e-6, 0.0);   
  obspower(tpwr); 
  if (tpwrsf_a < 4095.0) obspwrf(4095.0);
  txphase(zero); if (phincr_a != 0) xmtrphase(zero);

  zgradpulse(gzlvl4,gt4);
  delay(taua -pwN -0.5*pw -gt4 -2.0*GRADIENT_DELAY -pwHs_dly);           

  sim3pulse(2.0*pw,0.0e-6,2*pwN,zero,zero,zero,0.0,0.0);

  dec2phase(t3);
  zgradpulse(gzlvl4,gt4);
  delay(taua -1.5*pwN -gt4 -2.0*GRADIENT_DELAY -pwHs_dly);

  initval(phincr_d,v14);
  txphase(two); if (phincr_d != 0) xmtrphase(v14);
  if (tpwrsf_d < 4095.0) { obspwrf(tpwrsf_d); obspower(tpwrs+6.0); }
    else obspower(tpwrs); 
  shaped_pulse("H2Osinc", pwHs, two, 2.0e-6, 0.0);   
  obspower(tpwr); 
  if (tpwrsf_d < 4095.0) obspwrf(4095.0);
  txphase(zero); if (phincr_d != 0) xmtrphase(zero);

  sim3pulse(pw, 0.0, pwN, zero, zero, t3, 0.0, 0.0);

  zgradpulse(gzlvl5,gt5);
  delay(taua -1.5*pwN -wtg_dly -gt5 -2.0*GRADIENT_DELAY);

     if(wtg3919[0] == 'y')		/*  3919 watergate   */
     {
       txphase(v11);
       rgpulse(pw*0.231,v11,rof1,rof1);
       delay(d3919);
       rgpulse(pw*0.692,v11,rof1,rof1);
       delay(d3919);
       rgpulse(pw*1.462,v11,rof1,rof1);

       delay(d3919/2.0-pwN);
       dec2rgpulse(2.0*pwN, zero, rof1, rof1);
       txphase(v12);
       delay(d3919/2.0-pwN);

       rgpulse(pw*1.462,v12,rof1,rof1);
       delay(d3919);
       rgpulse(pw*0.692,v12,rof1,rof1);
       delay(d3919);
       rgpulse(pw*0.231,v12,rof1,rof1);
     }
     else				/*  soft pulse watergate   */
     {
       initval(phincr_d,v14);
       txphase(two); if (phincr_d != 0) xmtrphase(v14);
       if (tpwrsf_d < 4095.0) { obspwrf(tpwrsf_d); obspower(tpwrs+6.0); }
         else obspower(tpwrs); 
       shaped_pulse("H2Osinc", pwHs, two, 2.0e-6, 0.0);   
       obspower(tpwr); 
       if (tpwrsf_d < 4095.0) obspwrf(4095.0);
       txphase(zero);  if (phincr_d != 0) xmtrphase(zero);
 
       sim3pulse(2.0*pw, 0.0, 2.0*pwN, zero, zero, zero, 0.0, 0.0);

       initval(phincr_u,v14);
       txphase(two); if (phincr_u != 0) xmtrphase(v14);
       if (tpwrsf_u < 4095.0) { obspwrf(tpwrsf_u); obspower(tpwrs+6.0); }
         else obspower(tpwrs); 
       shaped_pulse("H2Osinc", pwHs, two, 2.0e-6, 0.0);   
       obspower(tpwr); 
       if (tpwrsf_u < 4095.0) obspwrf(4095.0);
       txphase(zero); if (phincr_u != 0) xmtrphase(zero);
     }

  zgradpulse(gzlvl5,gt5);
  delay(taua -1.5*pwN -wtg_dly -gt5 -2.0*GRADIENT_DELAY);       

  dec2rgpulse(pwN,zero,0.0,rof2);

status(C);			/* acquire data */
     setreceiver(t14);
}
