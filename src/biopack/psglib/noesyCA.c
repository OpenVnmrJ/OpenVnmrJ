/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*  noesyCA.c   auto-calibrated version of noesy_C_chirp_purge_lek_v3a.c 

    This pulse sequence will allow one to perform the following experiment:

    3D noesy from unlabeled to labeled (13C) in D2O. Note the corresponding
        expt. for water samples is noesy_NC_chirp_purge_lek_500.c
	
	purging and INEPT transfer is achieved by carbon CHIRP inversion pulses
	well-suited for D2O-SAMPLES!!

                       F1      1H
                       F2      13C
                       F3(acq) 1H

    NOTE: cnoesy_3c_sed_pfg(1).c 3D C13 edited NOESYs with separation via the
          carbon of the origination site. (D2O experiments)

    Uses three channels:
         1)  1H             - carrier @ water  
         2) 13C             - carrier @ 67 ppm 
         3) 15N             - carrier @ 119ppm [centre of amide N]


    Set dm = 'nny', dmm = 'ccp' [13C decoupling during acquisition].
    Set dm2 = 'nnn', dmm2 = 'ccc' 

    Must set phase = 1,2 and phase2 = 1,2 for States-TPPI acquisition in
    t1 [H]  and t2 [C and N].
  
    Flags
         fsat       'y' for presaturation of H2O
         fscuba     'y' to apply scuba pulse after presaturation of H2O
         mess_flg   'y' for 1H (H2O) purging pulse before relaxation delay
         f1180      'y' for 180deg linear phase correction in F1
                        otherwise 0deg linear phase correction
         f2180      'y' for 180deg linear phase correction in F2
         c180_flg   'y' when recording F1,F3 2D.

    Standard Settings:
       fsat='n', fscuba='n', mess_flg='y', f1180='y', f2180='y'
       c180_flg='n'
         

    Set f1180 = 'y' and f2180 = 'y' for (-90,180) in F1 and (-90,180) in  F2.   
    Note: Zero order phase correct may not be exactly -90 in F2 due to seduce.

    Modified by L.E. Kay on Aug. 15, 1994 from noesyc_pfg_h2o_NC+.c 

    Modified by L. E. Kay on March 6, 1995 to include 15N purging

    Modified by L.E.Kay on April 2, 1996 to include additional N purging

    Modified by R.Konrat on May, 1996 to include CHIRP pulses,
	 (improves INEPT transfer &  B1-sensitivity)

	NOTE: CHIRP-inversion is achieved using the decoupler (decprgon)

	PROGRAM: apod_chirp_dec creates the necessary .DEC-shape
                 (only for non-autocalibrated mode (autocal='n'))

  Please check with the paper for details concerning the construction
      of the pulses for your specific field

   Adiabatic pulses are used for both purging and the INEPT transfers
     from 1H ----> 13C and from 13C ----> 1H. 
 
    Example: For Purging for labeled protein/unlabeled peptide
       Carbon carrier is placed at 67 ppm throughout the expt. The center
       of the adiapatic pulse (purging) is at 0 ppm and covers a sweep of
       60 KHz (centered about 0 ppm). I recommend a taua delay of 2.1ms
       and a sweep rate of 2.980 *107 Hz/s and a adiabatic pulse width of
       2.013 ms. We have also optimized a taua of 2.0 ms and a sweep
       rate of 3.729*107Hz/s with a width of 1.609 ms. (600 MHz) Note that
       different rates are needed for different fields. 
 
       apod_chirp_dec 2013 1000 -40090.0 60000.0 20 0 1 
 
       will generate a pulse of 2.013 ms with 1000 steps starting
       -40090 Hz from 67 ppm (ie, 30 KHz downfield of 0 ppm) with a
       sweep of 60 KHz and the first 20% and last 20% of the pulse
       is multiplied by a sine funtion.
 
       For the adiabatic pulses used for the INEPT transfers we use pulses
       of duration 500 us and a 60 KHz sweep centered at 0 ppm for protein/
       peptide work.
 
       Note: Optmized taua/pw_chirp and rates must be employed.
 
       Use a 5 KHz field for chirp 

    Modified by L.E.Kay, based on noesy_NC+_chirp_purge_rk.c on Sept. 6, 96

    Modified by  L. E. Kay on Nov 27, 1996 to allow for fast chirps for the inept
     transfers

   Modified by C. Zwahlen & S. Vincent on March 18, 1997, to allow two
   two different chirps for purging


       The autocal flag is generated automatically in Pbox_bio.h
       If this flag does not exist in the parameter set, it is automatically 
       set to 'y' - yes. In order to change the default value, create the  
       flag in your parameter set and change as required. 
       For the autocal flag the available options are: 'y' (yes - by default), 
       and 'n' (no - use full manual setup, equivalent to 
       the original code).  E.Kupce, Varian

*/

#include <standard.h>
#ifndef PI
#define PI 3.14159265358979323846 
#endif

#include "Pbox_bio.h"

#define CHIRP1   "wurst180-20 -54k/1.793m -67p"                    /* WURST 180 on resonance  */
#define CHIRP2   "wurst180-20 -54k/2.75m -67p"                     /* WURST 180 on resonance  */
#define CHIRPi   "wurst180-20 -54k/0.5m"                           /* WURST 180 on resonance  */
#define CHIRPps "-stepsize 0.5 -dres 9.0 -type d -attn i"
#define CODEC   "SEDUCE1 20p 108p"                  /* off resonance SEDUCE1 on C' at 176 ppm */
#define CODECps "-dres 1.0 -stepsize 5.0 -attn i"

static shape chirp1, chirp2, chirpi, codec;


static int  phi1[8]  = {0,0,0,0,2,2,2,2},
            phi2[16] = {0,0,0,0,0,0,0,0,2,2,2,2,2,2,2,2},
            rec[16]  = {0,2,1,3,2,0,3,1,2,0,3,1,0,2,1,3},
            phi5[4]  = {0,0,1,1},
            phi6[2]  = {0,2},
            phi7[4]  = {0,0,2,2};
                    
static double d2_init=0.0, d3_init=0.0;
            
void pulsesequence()
{
/* DECLARE VARIABLES */

 char       autocal[MAXSTR],  /* auto-calibration flag */
            fsat[MAXSTR],
	    fscuba[MAXSTR],
            f1180[MAXSTR],    /* Flag to start t1 @ halfdwell             */
            f2180[MAXSTR],    /* Flag to start t2 @ halfdwell             */
            c180_flg[MAXSTR],
            codecseq[MAXSTR],
            mess_flg[MAXSTR],
            ch_shp1[MAXSTR], /* shape for the 1st purge CHIRP */
            ch_shp2[MAXSTR], /* shape for the 2nd purge CHIRP */
            chshpi[MAXSTR]; /* shape for the INEPT CHIRPs */

 int         phase, phase2, 
             t1_counter,   /* used for states tppi in t1           */ 
             t2_counter;   /* used for states tppi in t2           */ 

 double      tau1,         /*  t1 delay */
             tau2,         /*  t2 delay */
             ni2,
             mix,         /* mixing time in seconds */
             pwC,          /* PW90 for c nucleus @ pwClvl         */
             pwcodec,      /* PW for C' nucleus @ dpwrco seduce dec  */
             tsatpwr,      /* low level 1H trans.power for presat  */
             pwClvl,        /* power level for 13C pulses on dec1  */
             dpwrco,       /* power level for C' seduce decoupling  */
             sw1,          /* sweep width in f1                    */
             sw2,          /* sweep width in f2                    */
             tofps,        /* tof for presat                       */ 
             dressed,      /* decoupler resolution for seduce decoupling */
             tpwrmess,    /* power level for Messerlie purge */
             dly_pg1,     /* duration of first part of purge */
             dly_wt,
             taua1,       /* Delay for the first purge CHIRP */
             taua2,       /* Delay for the  second purge CHIRP */
                 
             pwchirp1,	/* duration of the 1st purge CHIRP */
             pwchirp2,	/* duration of the 2nd purge CHIRP */
             d_me1,     /* time difference between 
			start of the sweep and the 
			excitation of the methyl region
			automatically calculated by the program
		 	necessary parameter diff (see below) */
             d_me2,     /* time difference between 
			start of the sweep and the 
			excitation of the methyl region
			automatically calculated by the program
		 	necessary parameter diff (see below) */
             dchrp1,	/* power for the 1st purge CHIRP pulse, only lower
			limit is important (see above!) */
             dchrp2,	/* power for the 2nd purge CHIRP pulse, only lower
			limit is important (see above!) */
             dmfchp1,	/* dmf (1/90) for the 1st purge CHIRP pulse
			dmfchp1 = 1/time_step of chirp-pulse
			[time_step = pwchirp1/no. of points in
			the .DEC-shape] */
             dmfchp2,	/* dmf (1/90) for the 1st purge CHIRP pulse
			dmfchp2 = 1/time_step of chirp-pulse
			[time_step = pwchirp2/no. of points in
			the .DEC-shape] */
             dres_chp,	/* dres for the chirp pulse 
			(must be set to 90, otherwise
			timing errors! ) */
             diff1,	/* shift differences between 
			methyl region and start of sweep */
             diff2,	/* shift differences between 
			methyl region and start of sweep */
             rate1,	/* sweep rate of the 1st purge CHIRP pulse
			frequency sweep/pwchirp1   */
             rate2,	/* sweep rate of the 2nd purge CHIRP pulse
			frequency sweep/pwchirp2   */
             dchrpi,
             dmfchpi,
             pwchirpi,    /* INEPT CHIRP duration */
             ratei,
             diffi,
             tauf,
             d_mei,  
             compC,        /* C-13 RF calibration parameters */

             gt0,
             gt1,
             gt2,
             gt3,
             gt4,
             gt5,
             gt8,
             gt9,
             gt10,
             gt11,
             gstab,
             gzlvl0, 
             gzlvl1, 
             gzlvl2, 
             gzlvl3, 
             gzlvl4, 
             gzlvl5,
             gzlvl8, 
             gzlvl9,
             gzlvl10,
             gzlvl11;

/*  variables commented out are already defined by the system      */


/* LOAD VARIABLES */


  getstr("autocal",autocal);
  getstr("fsat",fsat);
  getstr("f1180",f1180);
  getstr("f2180",f2180);
  getstr("fscuba",fscuba);
  getstr("c180_flg",c180_flg);
  getstr("mess_flg",mess_flg);


  tofps  = getval("tofps");
  mix = getval("mix");
  pwC = getval("pwC");
  tpwr = getval("tpwr");
  tsatpwr = getval("tsatpwr");
  pwClvl = getval("pwClvl");
  dpwr = getval("dpwr");
  dpwr2 = getval("dpwr2");
  phase = (int) ( getval("phase") + 0.5);
  phase2 = (int) ( getval("phase2") + 0.5);
  sw1 = getval("sw1");
  sw2 = getval("sw2");
  ni2 = getval("ni2");
  tpwrmess = getval("tpwrmess");
  dly_pg1 = getval("dly_pg1");
  dly_wt = getval("dly_wt");
  taua1 = getval("taua1");
  taua2 = getval("taua2");
  
  rate1 = getval("rate1");
  rate2 = getval("rate2");
  diff1 = getval("diff1");
  diff2 = getval("diff2");
  diffi = getval("diffi");
  ratei = getval("ratei");

  tauf = getval("tauf");

  gt0 = getval("gt0");
  gt1 = getval("gt1");
  gt2 = getval("gt2");
  gt3 = getval("gt3");
  gt4 = getval("gt4");
  gt5 = getval("gt5");
  gt8 = getval("gt8");
  gt9 = getval("gt9");
  gt10 = getval("gt10");
  gt11 = getval("gt11");
  gstab = getval("gstab");
  gzlvl0 = getval("gzlvl0");
  gzlvl1 = getval("gzlvl1");
  gzlvl2 = getval("gzlvl2");
  gzlvl3 = getval("gzlvl3");
  gzlvl4 = getval("gzlvl4");
  gzlvl5 = getval("gzlvl5");
  gzlvl8 = getval("gzlvl8");
  gzlvl9 = getval("gzlvl9");
  gzlvl10 = getval("gzlvl10");
  gzlvl11 = getval("gzlvl11");

  if(autocal[0]=='n')
  {     
    getstr("codecseq",codecseq);
    dressed = getval("dressed");
    pwcodec = getval("pwcodec");
    dpwrco = getval("dpwrco");
    getstr("ch_shp1",ch_shp1);
    getstr("ch_shp2",ch_shp2);
    pwchirp1 = getval("pwchirp1");
    pwchirp2 = getval("pwchirp2");
    dchrp1 = getval("dchrp1");
    dchrp2 = getval("dchrp2");
    dmfchp1 = getval("dmfchp1");
    dmfchp2 = getval("dmfchp2");
    dres_chp = getval("dres_chp");
    getstr("chshpi",chshpi);
    dchrpi = getval("dchrpi");
    dmfchpi = getval("dmfchpi");
    pwchirpi = getval("pwchirpi");
  }
  else
  {    
    strcpy(codecseq,"Psed_108p");
    strcpy(ch_shp1,"Pwurst180_1");
    strcpy(ch_shp2,"Pwurst180_2");
    strcpy(chshpi,"Pwurst180i");
    if (FIRST_FID)
    {
      compC = getval("compC"); 
      codec = pbox(codecseq, CODEC, CODECps, dfrq, compC*pwC, pwClvl);
      chirp1 = pbox(ch_shp1, CHIRP1, CHIRPps, dfrq, compC*pwC, pwClvl);
      chirp2 = pbox(ch_shp2, CHIRP2, CHIRPps, dfrq, compC*pwC, pwClvl);
      chirpi = pbox(chshpi, CHIRPi, CHIRPps, dfrq, compC*pwC, pwClvl);
    }
    dpwrco = codec.pwr;      pwcodec = 1.0/codec.dmf;  dressed = codec.dres;
    dchrp1 = chirp1.pwr;     dmfchp1 = chirp1.dmf;
    pwchirp1 = chirp1.pw;    dres_chp = chirp1.dres;       
    dchrp2 = chirp1.pwr;     dmfchp2 = chirp2.dmf;
    pwchirp2 = chirp2.pw;     
    dchrpi = chirpi.pwr;     dmfchpi = chirpi.dmf;
    pwchirpi = chirpi.pw;     
  }   
  
/* LOAD PHASE TABLE */

  settable(t1,8,phi1);
  settable(t2,16,phi2);
  settable(t4,16,rec);
  settable(t5,4,phi5);
  settable(t6,2,phi6);
  settable(t7,4,phi7);

/* CHECK VALIDITY OF PARAMETER RANGES */


    if((dm[A] == 'y' || dm[B] == 'y' ))
    {
        printf("incorrect dec1 decoupler flags!  ");
        psg_abort(1);
    }

    if((dm2[A] == 'y' || dm2[B] == 'y'))
    {
        printf("incorrect dec2 decoupler flags! Should be 'nnn' ");
        psg_abort(1);
    }

    if( tsatpwr > 6 )
    {
        printf("TSATPWR too large !!!  ");
        psg_abort(1);
    }

    if( dpwr > 50 )
    {
        printf("don't fry the probe, DPWR too large!  ");
        psg_abort(1);
    }

    if( dpwrco > 50 )
    {
        printf("don't fry the probe, dpwrco too large!  ");
        psg_abort(1);
    }

    if( dpwr2 > 46 )
    {
        printf("don't fry the probe, DPWR2 too large!  ");
        psg_abort(1);
    }

    if( pw > 200.0e-6 )
    {
        printf("dont fry the probe, pw too high ! ");
        psg_abort(1);
    } 

    if( pwC > 200.0e-6 )
    {
        printf("dont fry the probe, pwC too high ! ");
        psg_abort(1);
    } 

    if( pwcodec < 300.0e-6 )
    {
        printf("dont fry the probe, pwcodec too high ! ");
        psg_abort(1);
    } 
    if ( tpwrmess > 56 )
    {
        printf("dont fry the probe, tpwrmess too high ! ");
        psg_abort(1);
    }
    if ( dly_pg1 > 0.010)
    {
        printf("dont fry the probe, dly_pg1 too long ! ");
        psg_abort(1);
    }

    if( gt0 > 15e-3 || gt1 > 15e-3 || gt2 > 15e-3 
           || gt3 > 15e-3 || gt4 > 15e-3  
           || gt5 > 15e-3 
           || gt8 > 15e-3 
           || gt9 > 15e-3 || gt10 > 15e-3 || gt11 > 15e-3  ) 
    {
        printf("gti values < 15e-3\n");
        psg_abort(1);
    } 

   if( gzlvl3*gzlvl4 > 0.0 ) 
    {
        printf("gt3 and gt4 must be of opposite sign \n");
        printf("for optimal water suppression\n");
        psg_abort(1);
     }

    if( dchrp1 > 60 )
    {
        printf("don't fry the probe, dchrp1 too large!  ");
        psg_abort(1);
    }

    if( dchrp2 > 60 )
    {
        printf("don't fry the probe, dchrp2 too large!  ");
        psg_abort(1);
    }

    if( pwchirp1 > 10.e-03 )
    {
        printf("don't fry the probe, pwchirp1 too large!  ");
        psg_abort(1);
    }

    if( pwchirp2 > 10.e-03 )
    {
        printf("don't fry the probe, pwchirp2 too large!  ");
        psg_abort(1);
    }

	d_me1 = diff1/rate1 ;
	d_me2 = diff2/rate2 ;

    if( d_me1 > 10.e-03 )
    {
        printf("don't fry the probe, d_me1 too large \n");
	printf("	(must be less than 10 msec)!  ");
        psg_abort(1);
    }
    if( d_me2 > 10.e-03 )
    {
        printf("don't fry the probe, d_me2 too large \n");
	printf("	(must be less than 10 msec)!  ");
        psg_abort(1);
    }

    if( d_me1 > pwchirp1 )
    {
        printf("impossible; d_me1 > pwchirp1 !  ");
        psg_abort(1);
    }

    if( d_me2 > pwchirp2 )
    {
        printf("impossible; d_me2 > pwchirp2 !  ");
        psg_abort(1);
    }


    if( dchrpi > 60 )
    {
       printf("dont fry the probe, dchrpi too large\n");
       psg_abort(1);
    }

    if(pwchirpi > 10.0e-3)
    {
        printf("don't fry the probe, pwchirpi too large!  ");
        psg_abort(1);
    }

    d_mei = diffi/ratei;

/*  Phase incrementation for hypercomplex 2D data */

    if (phase == 2)
      tsadd(t1,1,4);
    if (phase2 == 2)
      tsadd(t2,1,4);

/*  Set up f1180  tau1 = t1               */
   
    tau1 = d2;
    if(f1180[A] == 'y') {
        tau1 += ( 1.0 / (2.0*sw1) - 4.0/PI*pw - 2.0e-6 );
    }

    else
        tau1 = tau1 - 4.0/PI*pw - 2.0e-6;

    if(tau1 < 0.2e-6) tau1 = 2.0e-7;

/*  Set up f2180  tau2 = t2               */

    tau2 = d3;
    if(f2180[A] == 'y') {
        tau2 += ( 1.0 / (2.0*sw2) - (4.0/PI)*pwC 
             - 2.0*pw - PRG_START_DELAY - PRG_STOP_DELAY 
             - 2.0*POWER_DELAY - 4.0e-6); 
    }

    else tau2 = tau2 - ((4.0/PI)*pwC + 2.0*pw 
               + PRG_START_DELAY + PRG_STOP_DELAY 
               + 2.0*POWER_DELAY + 4.0e-6); 

        if(tau2 < 0.2e-6)  tau2 = 4.0e-7;
        tau2 = tau2/2.0;

/* Calculate modifications to phases for States-TPPI acquisition          */

   if( ix == 1) d2_init = d2 ;
   t1_counter = (int) ( (d2-d2_init)*sw1 + 0.5 );
   if(t1_counter % 2) {
      tsadd(t1,2,4);     
      tsadd(t4,2,4);    
    }

   if( ix == 1) d3_init = d3 ;
   t2_counter = (int) ( (d3-d3_init)*sw2 + 0.5 );
   if(t2_counter % 2) {
      tsadd(t2,2,4);  
      tsadd(t4,2,4);    
    }

   
   

/* BEGIN ACTUAL PULSE SEQUENCE */

status(A);
   delay(5.0e-6);
   obspower(tsatpwr);      /* Set transmitter power for 1H presaturation */
   decpower(pwClvl);        /* Set Dec1 power for hard 13C pulses         */
   delay(5.0e-6);

/* Presaturation Period */

   if (mess_flg[A] == 'y') {

      obsoffset(tofps);
      obspower(tpwrmess);
      txphase(zero);
      rgpulse(dly_pg1,zero,2.0e-6,2.0e-6);
      txphase(one);
      rgpulse(dly_pg1/1.62,one,2.0e-6,2.0e-6);

      obspower(tsatpwr);
   }

   if (fsat[0] == 'y')
   {
        obsoffset(tofps);
	delay(2.0e-5);
    	rgpulse(d1,zero,2.0e-6,2.0e-6); /* presat */
   	obspower(tpwr);      /* Set transmitter power for hard 1H pulses */
	delay(2.0e-5);
	if(fscuba[0] == 'y')
	{
		delay(2.2e-2);
		rgpulse(pw,zero,2.0e-6,0.0);
		rgpulse(2*pw,one,2.0e-6,0.0);
		rgpulse(pw,zero,2.0e-6,0.0);
		delay(2.2e-2);
	}
   }

   else
   {
    delay(d1);
   }

   obspower(tpwr);           /* Set transmitter power for hard 1H pulses */
   obsoffset(tof);
   decphase(zero);

/* Begin Pulses */

status(B);

   rcvroff();
   delay(10.0e-6);

   rgpulse(pw,t6,4.0e-6,0.0);            /* 90 deg 1H pulse */

   txphase(zero); decphase(zero);
 
   delay(2.0e-6);
   zgradpulse(gzlvl8,gt8);
   delay(gstab);

   delay(taua1 - gt8 - gstab -2.0e-6 - POWER_DELAY - 4.0e-6 
         - PRG_START_DELAY - d_me1); 

         
   /* 1st purge CHIRP inversion  on */

   decpower(dchrp1);  /* Set power for 1st purge CHIRP inversion */
   delay(4.0e-6);

   decprgon(ch_shp1,1.0/dmfchp1,dres_chp);
   decon();

   delay(d_me1);
   
   rgpulse(2*pw,zero,0.0,0.0);       /* 1H  inversion pulse */

   delay(pwchirp1 - d_me1 - 2*pw);
   decoff();
   decprgoff();

   /* chirp inversion  off */

   delay(2.0e-6);
   zgradpulse(gzlvl8,gt8);
   delay(gstab);

   delay(taua1 + 2*pw - (pwchirp1 - d_me1) 
          - PRG_STOP_DELAY  - gt8 - gstab -2.0e-6); 
 
   rgpulse(pw,zero,0.0,0.0);

   txphase(t7);

   delay(2.0e-6);
   zgradpulse(gzlvl9,gt9);
   delay(2.0*gstab); 
  
   rgpulse(pw,t7,0.0,0.0); 		/* PHASE t7 = 2(x),2(-x)*/

   delay(2.0e-6);
   zgradpulse(gzlvl11,gt11);
   delay(gstab);

   decphase(zero); txphase(zero);

   delay(taua2 - gt11 - gstab -2.0e-6 - POWER_DELAY 
        - 4.0e-6 - PRG_START_DELAY - d_me2);

   /* Second chirp inversion  on */

   decpower(dchrp2);  /* Set power for chirp inversion */
   delay(4.0e-6);
   decprgon(ch_shp2,1.0/dmfchp2,dres_chp);
   decon();

   delay(d_me2);

   rgpulse(2*pw,zero,0.0,0.0);        /* 1H inversion pulse */
   
   delay(pwchirp2 - d_me2 - 2*pw);
   decoff();
   decprgoff();

   /* Second purge CHIRP off */

   delay(2.0e-6);
   zgradpulse(gzlvl11,gt11);
   delay(gstab);

   txphase(zero);

   delay(taua2 + 2*pw - (pwchirp2 - d_me2) 
         - PRG_STOP_DELAY - gt11 - gstab -2.0e-6 );

   rgpulse(pw,zero,0.0,0.0);  

   delay(2.0e-6);
   zgradpulse(gzlvl10,gt10);
   delay(2.0*gstab); 

   rgpulse(pw,t1,4.0e-6,0.0);

   delay(tau1);

   rgpulse(pw,zero,2.0e-6,0.0);

   delay(mix - 10.0e-3);

   delay(2.0e-6);
   zgradpulse(gzlvl0,gt0);

   decpower(pwClvl);  /* Set power for hard pulses */

   delay(4.0e-6);

   decrgpulse(pwC,zero,0.0,0.0); 

   delay(2.0e-6);
   zgradpulse(gzlvl1,gt1);
   delay(2.0e-6);
   decphase(zero);

   delay(10.0e-3 - gt1 - gt0 - 8.0e-6);
   
   rgpulse(pw,zero,0.0,0.0);

   delay(2.0e-6);
   zgradpulse(gzlvl2,gt2);
   delay(gstab);

  decphase(zero);
  delay(tauf - gt2 - gstab -2.0e-6 - POWER_DELAY - 4.0e-6 
       - PRG_START_DELAY - d_mei);

   /* INEPT CHIRP inversion  on */

   decpower(dchrpi);  /* Set power for chirp inversion */
   delay(4.0e-6);
   decprgon(chshpi,1.0/dmfchpi,dres_chp);
   decon();

   delay(d_mei);

   rgpulse(2*pw,zero,0.0,0.0);  /* 1H inversion pulse */

   delay(pwchirpi - d_mei - 2*pw);
   decoff();
   decprgoff();

   /* chirp inversion  off */

   delay(2.0e-6);
   zgradpulse(gzlvl2,gt2);
   delay(gstab);

   txphase(one); 

   delay(tauf + 2*pw - (pwchirpi - d_mei) 
       - PRG_STOP_DELAY - gt2 - gstab -2.0e-6 );

  rgpulse(pw,one,0.0,0.0);

  txphase(zero); decphase(t2);

   decpower(pwClvl);  /* Set power for C13 hard pulse */
  

   delay(2.0e-6);
   zgradpulse(gzlvl3,gt3);
   delay(200.0e-6);

  decrgpulse(pwC,t2,0.0,0.0);     
  decphase(zero);

   if( c180_flg[A] == 'n' ) {

   delay(2.0e-6);

   /* CO decoupling on */
   decpower(dpwrco);
   decprgon(codecseq,pwcodec,dressed);
   decon();
   /* CO decoupling on */

   delay(tau2);
 
   rgpulse(2*pw,zero,0.0,0.0);

   delay(tau2);

   /* CO decoupling off */
   decoff();
   decprgoff();
   decpower(pwClvl);
   /* CO decoupling off */

   delay(2.0e-6);

  }

  else
   simpulse(2*pw,2*pwC,zero,zero,2.0e-6,2.0e-6);

  decrgpulse(pwC,zero,0.0,0.0);  

   delay(2.0e-6);
   zgradpulse(gzlvl4,gt4);
   delay(200.0e-6);

   rgpulse(pw,t5,2.0e-6,0.0);

   txphase(zero);

   delay(2.0e-6);
   zgradpulse(gzlvl5,gt5);
   delay(gstab);

  decphase(zero);
  delay(tauf - gt5 - gstab -2.0e-6 - POWER_DELAY 
       - 4.0e-6 - PRG_START_DELAY - d_mei);

   /* 2nd INEPT CHIRP inversion  on */

   decpower(dchrpi);  /* Set power for chirp inversion */
   delay(4.0e-6);
   decprgon(chshpi,1.0/dmfchpi,dres_chp);
   decon();

   delay(d_mei);

   rgpulse(2*pw,zero,0.0,0.0);  /* 1H inversion pulse */

   delay(pwchirpi - d_mei - 2*pw);
   decoff();
   decprgoff();

   /* chirp inversion  off */

   delay(2.0e-6);
   zgradpulse(gzlvl5,gt5);
   delay(gstab);

   decpower(dpwr);  /* Set power for decoupling */

   txphase(t5); 

   delay(tauf + 2*pw - (pwchirpi - d_mei) - PRG_STOP_DELAY 
          - gt5 - gstab -2.0e-6 - 2*POWER_DELAY);

   rgpulse(pw,t5,0.0,0.0);
    

/* BEGIN ACQUISITION */

status(C);
   setreceiver(t4);

}
