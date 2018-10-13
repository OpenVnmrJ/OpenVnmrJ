/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*  gc_tocsy_nch2A.c 

    This pulse sequence will allow one to correlate the side chain
    carbon resonances with the NZ amino group of lysines, and with the
    NE of arginines.

                F1      N
		F2	C
                F3(acq) H

    This sequence uses the standard three channel configuration
         1)  1H             - carrier (tof) @ H2O [4.77 ppm]
         2) 13C             - carrier (dof) @ aliphatic C [42 ppm]
         3) 15N             - carrier (dof2) @ center of NZ [33 ppm] for Lys only
                            or between Lys NZ and and Arg NE [59 ppm] for Lys and Arg
    
    Set dm = 'nnny', dmm = 'cccp' 
    Set dm2 = 'nnnn'

    Must set phase = 1,2 and phase2 = 1,2 for States-TPPI acquisition in t1 and t2
    
    
    Flags
        fsat            'y' for presaturation of H2O
        fscuba          'y' for apply scuba pulse after presaturation of H2O
        f1180           'y' for 180 deg linear phase correction in F1 (15N)
        f2180           'y' for 180 deg linear phase correction in F2 (13CA)
			'n' for phases (0,0) which also gives perfect baseline
        ch_plane            to avoid 15N evolution when recording 2D CH plane

	Standard Settings
        fsat='n',fscuba='n',f1180='y',f2180='n'

        Written by fm, 09 Mar 2004

        Improvements by fm, 03 May 2004
                added ch_plane flag for zero 15N evolution time when
                  recording HC plane
                added waltz 1H decoupling during 15N evolution

                set taub=1/4J (1.8 ms) to obtain
                100% transfer efficiency for CH2 (Hz -> Cx + 4CxHzHz)

         Written by fm, 19 Aug 2004
         Modified for BioPack, GG, Varian, Jan 2008 from Mulder's ccctocsy_h2can_RK_600_fm.c
    (See Andre, Linse and Mulder, JACS, 129,15805-15813 (2007))
  
*/

#include <standard.h>
#include "Pbox_bio.h"       /* Pbox Bio Pack Pulse Shaping Utilities */

static shape   spco180,spreb180;

static int  phi1[1]  = {1},	
            phi2[4]  = {0,0,2,2},
            phi3[2]  = {0,2},
            phi4[16]  = {0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3},
            phi5[1]  = {0},
            rec[8]  = {0,2,2,0,2,0,0,2};

static double d2_init=0.0,
	      d3_init=0.0;
            
pulsesequence()
{
/* DECLARE VARIABLES */

 char       fsat[MAXSTR],
	    fscuba[MAXSTR],
            f1180[MAXSTR],    /* Flag to start t1 @ halfdwell             */
            f2180[MAXSTR],    /* Flag to start t2 @ halfdwell             */
            ch_plane[MAXSTR];    /* Flag to start t2 @ halfdwell             */

 int         phase, phase2, ni, ni2, ncyc,
             t1_counter,   /* used for states tppi in t1           */ 
             t2_counter;   /* used for states tppi in t2           */ 

 double      tau1,         /*  t1 delay */
 	     tau2,         /*  t2 delay */
             taua,         /*  ~ 1/4JCH =  1.7 ms */
             taub,         /*  = 1/4JCH or 1/8JCH for editing */
             TC,           /*  ~ 1/2JCaCo =  9 ms */
             pw_ml,        /* PW90 for mlev 1H decoupling */
             pwN,          /* PW90 for 15N pulse  */
             pwC,          /* PW90 hard 13C pulse */
             pwc90,        /* PW90 for Ca or Co nucleus */
             pw_dip,       /* PW90 for Ca or Co nucleus */
             pwreb180,     /* PW180 for reburp */
             pwcon180,     /* PW for Ca or Co on-res 180 */
             pwcoff180,    /* PW for Ca or Co off-res 180 */
             satpwr,      /* low level 1H trans.power for presat  */
             tpwrml,       /* power for 1H decoupling */
             pwClvl,        /* power level for 13C pulses on dec1 - 64 us 
                              90 for part a of the sequence  */
             compH,        /* compression factor */
             compC,        /* compression factor */
             waltzB1,      /* power level for proton decoupling */
             d_c90,        /* power level for Ca or Co 90 pulse */
             d_c180,       /* power level for Ca or Co 180 pulse */
             d_coff180,    /* power level for shifted 180 */
             d_reb,        /* power level for reburp 180 pulse */
             dpwr_dip,     /* power level for reburp 180 pulse */
             pwNlvl,       /* high dec2 pwr for 15N hard pulses    */
             sw1,          /* sweep width in f1                    */             
             sw2,          /* sweep width in f2                    */             
             dof_coca,     /* offset between Co and Ca for comp180 */
             bw,ofs,ppm,   /* temporary Pbox parameters */



             zeta,
             zeta1,

             gt0,
             gt1,
             gt2,
             gt3,
             gt4,
             gt5,
             gt6,
             gt7,
             gt8,
             gstab,
             gzlvl0,
             gzlvl1,
             gzlvl2,
             gzlvl3,
             gzlvl4,
             gzlvl5,
             gzlvl6,
             gzlvl7,
             gzlvl8; 

/* LOAD VARIABLES */


  getstr("fsat",fsat);
  getstr("fscuba",fscuba);
  getstr("f1180",f1180);
  getstr("f2180",f2180);
  getstr("ch_plane",ch_plane);

  taua   = getval("taua"); 
  taub   = getval("taub"); 
  TC   = getval("TC"); 
  pwc90 = getval("pwc90");
  pwN = getval("pwN");
  pwC = getval("pwC");
  pwClvl = getval("pwClvl");
  compC = getval("compC");
  compH = getval("compH");
  waltzB1 = getval("waltzB1");
  tpwr = getval("tpwr");
  satpwr = getval("satpwr");
  d_c180 = getval("d_c180");
  d_reb = getval("d_reb");
  dpwr = getval("dpwr");
  pwNlvl = getval("pwNlvl");
  phase = (int) ( getval("phase") + 0.5);
  phase2 = (int) ( getval("phase2") + 0.5);
  sw1 = getval("sw1");
  sw2 = getval("sw2");
  ni  = getval("ni");
  ni2  = getval("ni2");
  ncyc = getval("ncyc");

  gt0 = getval("gt0");
  gt1 = getval("gt1");
  gt2 = getval("gt2");
  gt3 = getval("gt3");
  gt4 = getval("gt4");
  gt5 = getval("gt5");
  gt6 = getval("gt6");
  gt7 = getval("gt7");
  gt8 = getval("gt8");
  gstab = getval("gstab");

  gzlvl0 = getval("gzlvl0");
  gzlvl1 = getval("gzlvl1");
  gzlvl2 = getval("gzlvl2");
  gzlvl3 = getval("gzlvl3");
  gzlvl4 = getval("gzlvl4");
  gzlvl5 = getval("gzlvl5");
  gzlvl6 = getval("gzlvl6");
  gzlvl7 = getval("gzlvl7");
  gzlvl8 = getval("gzlvl8");

/* LOAD PHASE TABLE */

  settable(t1,1,phi1);
  settable(t2,4,phi2);
  settable(t3,2,phi3);
  settable(t4,16,phi4);
  settable(t5,1,phi5);
  settable(t6,8,rec);

/* CHECK VALIDITY OF PARAMETER RANGES */

    if( satpwr > 6 )
    {
        printf("TSATPWR too large !!!  ");
        psg_abort(1);
    }

    if( dpwr > 46 )
    {
        printf("don't fry the probe, DPWR too large!  ");
        psg_abort(1);
    }

    if( dpwr2 > 46 )
    {
        printf("don't fry the probe, DPWR2 too large!  ");
        psg_abort(1);
    }

    if( pwNlvl > 63 )
    {
        printf("don't fry the probe, DHPWR2 too large!  ");
        psg_abort(1);
    }

    if( pw > 200.0e-6 )
    {
        printf("dont fry the probe, pw too high ! ");
        psg_abort(1);
    } 

    if( pwN > 200.0e-6 )
    {
        printf("dont fry the probe, pwN too high ! ");
        psg_abort(1);
    } 

    if( pwC > 200.0e-6 )
    {
        printf("dont fry the probe, pwC too high ! ");
        psg_abort(1);
    } 

    if( ncyc > 9 )
    {
        printf("dont fry the probe, ncyc must be <= 9\n");
        psg_abort(1);
    }


    if( gt0 > 10.0e-3 || gt1 > 10.0e-3 || gt2 > 10.0e-3 || gt3 >10.0e-3 ||
       gt4 > 10.0e-3 || gt5 > 10.0e-3 || gt6 > 10.0e-3 || gt7 > 10.0e-3 )
    {
        printf("gt values are too long. Must be < 10.0e-3\n");
        psg_abort(1);
    } 

/*  Phase incrementation for hypercomplex 2D & 3D data */

    if (phase == 2)
      tsadd(t3,1,4);

    if (phase2 == 2)
      tsadd(t5,1,4);

/*  Set up f1180  tau1 = t1               */
   
    tau1 = d2;
    if(f1180[A] == 'y') {
        tau1 += 1.0 / (2.0*sw1) - 4.0*pwC - 4.0*2.0e-6 - 2.0*(2.0/PI)*pwN ;
        if(tau1 < 0.4e-6) {
           tau1 = 0.4e-6;
           printf("tau1 is negative; decrease sw1 for proper phasing \n");
        }
    }
        tau1 = tau1/2.0;

/*  Set up f2180  tau2 = t2               */

    tau2 = d3;
    if(f2180[A] == 'y') {
        tau2 += ( 1.0 / (2.0*sw2));
    }

    tau2 = tau2 / 2.0;


/* Calculate modifications to phases for States-TPPI acquisition in t1 & t2  */

   if( ix == 1) d2_init = d2 ;
   t1_counter = (int) ( (d2-d2_init)*sw1 + 0.5 );
   if(t1_counter % 2) {
      tsadd(t3,2,4);     
      tsadd(t6,2,4);    
    }

   if( ix == 1) d3_init = d3 ;
   t2_counter = (int) ( (d3-d3_init)*sw2 + 0.5 );
   if(t2_counter % 2) {
      tsadd(t5,2,4);     
      tsadd(t6,2,4);    
    }
   pwc90 = 1/(4.0 * (4700*(dfrq/150))) ;  /*  4.7kHz at 150 MHz 13C */
   d_c90 = pwClvl - 20.0*log10(pwc90/(compC*pwC));
   d_c90 = (int) (d_c90 + 0.5);

   pwcon180 = 1/(4.0 * (10900*(dfrq/150))) ;  /*  10.9kHz at 150 MHz 13C */   d_c180 = pwClvl - 20.0*log10(pwcon180/(compC*pwC));
   d_c180 = (int) (d_c180 + 0.5);


/* Now include the Bax/Logan trick (shared time evolution in 13C) */

    if(ni2 != 1)
      zeta = (taub/2.0 + 2*pw) / ( (double)(ni2-1) );
    else
      zeta = 0.0;

    zeta1 = zeta*( (double)t2_counter );

 if (FIRST_FID)                                      /* call Pbox */
  {
          ppm = getval("dfrq");
          bw = 132.0*ppm; ofs = bw;   /* carrier at 42ppm, inversion at 174ppm */
          spco180 = pbox_make("spco180","square180n", bw, ofs, compC*pwC, pwClvl);
          spreb180 = pbox_make("spreb180","reburp", 20*dfrq, -4*dfrq, compC*pwC, pwClvl);
  }
  pwcoff180=spco180.pw; d_coff180=spco180.pwrf;
  pwreb180=spreb180.pw; d_reb=spreb180.pwrf;

    if (0.5*TC - 0.5*(ni2-1)/sw2 - pwcon180/2.0 - 2.0e-6 - pwcoff180 -2.0*2.0e-6 \
           - WFG_START_DELAY - WFG_STOP_DELAY - POWER_DELAY < 0.4e-6) {
	printf("ni2 too large !\n");
	psg_abort(1);
    }

/* BEGIN ACTUAL PULSE SEQUENCE */

status(A);
   obsoffset(tof);
   decoffset(dof);
   dec2offset(dof2);
   obspower(satpwr);      /* Set transmitter power for 1H presaturation */
   decpower(pwClvl);        /* Set Dec1 power for hard 13C pulses         */
   dec2power(pwNlvl);      /* Set Dec2 power for 15N hard pulses         */

/* Presaturation Period */

status(B);
   if (fsat[0] == 'y')
   {
	delay(2.0e-5);
        rgpulse(d1,zero,2.0e-6,2.0e-6); /* presaturation */
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
   txphase(zero);
   dec2phase(zero);
   delay(1.0e-5);

/* Begin Pulses */

status(B);

   rcvroff();
   delay(20.0e-6);

   decrgpulse(pwC,zero,0.0,0.0);

   delay(0.2e-6);
   zgradpulse(gzlvl0,gt0);
   delay(gstab);

   rgpulse(pw,zero,0.0,0.0);                    /* 90 deg 1H pulse */

   zgradpulse(gzlvl1,gt1);
   delay(taua - gt1);                /* taua <= 1/4JCH */ 
   simpulse(2*pw,2*pwC,zero,zero,0.0,0.0);
   zgradpulse(gzlvl1,gt1);
   delay(taua - gt1 );                /* taua <= 1/4JCH */ 

   rgpulse(pw,t1,0.0,0.0);

   zgradpulse(gzlvl2,gt2);
   delay(gstab);

   decrgpulse(pwC,t5,2.0e-6,0.0);

   txphase(zero); decphase(zero);

   delay(tau2);

   dec2rgpulse(2*pwN,zero,0.0,0.0);

   delay(taub/2.0 - 2*pwN);

   rgpulse(2*pw,zero,0.0,0.0);

   zgradpulse(gzlvl3,gt3);
   delay(tau2 - zeta1);

   decrgpulse(2.0*pwC,zero,0.0,0.0);
   zgradpulse(gzlvl3,gt3);

   delay(taub/2.0 - zeta1 + 2*pw);

   decrgpulse(pwC,one,0.0,2.0e-6);


   zgradpulse(gzlvl6,gt6/1.7);
   delay(gstab);

   pw_dip = 1/(4.0 * (6700*(dfrq/150))) ;  /*  6.7kHz at 150 MHz 13C */
   dpwr_dip = pwClvl - 20.0*log10(pw_dip/(compC*pwC));
   dpwr_dip = (int) (dpwr_dip + 0.5);
   decpower(dpwr_dip);

   if( (float) ncyc > 0.1) {
      decspinlock("FLOPSY8",pw_dip,1.0,zero,ncyc);
   }

   decphase(one); 
   decpower(d_c90);
   zgradpulse(gzlvl6,gt6);
   delay(gstab);

   pw_ml = 1/(4.0 * waltzB1) ;
   tpwrml = tpwr - 20.0*log10(pw_ml/(compH*pw));
   tpwrml = (int) (tpwrml + 0.5);

   obspower(tpwrml);

/* H decoupling on */
     rgpulse(pw_ml,one,2.0e-6,0.0);
     txphase(zero);
     delay(2.0e-6);
     obsprgon("waltz16",pw_ml,90.0);
     xmtron();
    
/* Cz */
   decrgpulse(pwc90,one,0.0,2.0e-6);

   decpower(d_c180);
   delay(0.5*TC - 2.0e-6 - POWER_DELAY  - 2.0*pwN - 2.0e-6);
   dec2rgpulse(2.0*pwN,zero,0.0,2.0e-6);
   decrgpulse(pwcon180,zero,0.0,0.0);
   delay(0.5*TC - POWER_DELAY );

   decphase(t2);
   decpower(d_c90);
   decrgpulse(pwc90,t2,0.0,0.0);

/* CzNz */

/* H decoupling off */
     xmtroff();
     obsprgoff();
     rgpulse(pw_ml,three,2.0e-6,2.0e-6);
     zgradpulse(gzlvl4,gt4);
     dec2phase(t3);
     delay(gstab);

     decpower(pwClvl);
     dof_coca=dof+(110-42)*dfrq;
     decoffset(dof_coca);	/* move C carrier for comp180 */
     decphase(one);

/* H decoupling on */
     obspower(tpwrml);
     rgpulse(pw_ml,one,2.0e-6,0.0);
     txphase(zero);
     delay(2.0e-6);
     obsprgon("waltz16",pw_ml,90.0);
     xmtron();
    
/* N evolution */

   dec2rgpulse(pwN,t3, 0.0, 2.0e-6);
   if(ch_plane[0] == 'y')
   {
     dec2phase(zero);
     delay(0.2e-6);
   }
   else {
       delay(tau1);
     decrgpulse(pwC, one, 0.0, 0.0);
     decrgpulse(2.0*pwC, zero, 2.0e-6, 2.0e-6);
     decrgpulse(pwC, one, 0.0, 0.0);
       delay(tau1);
   }
   dec2rgpulse(pwN, zero, 2.0e-6, 0.0);

/* H decoupling off */
     xmtroff();
     obsprgoff();
     rgpulse(pw_ml,three,2.0e-6,2.0e-6);
     obspower(tpwr);

     decpower(d_c90);
     decoffset(dof);		/* move back C carrier */
     decphase(t5);

/* clean-up before proceeding */

     zgradpulse(gzlvl5,gt5);
     delay(gstab);

   decrgpulse(pwc90,zero,0.0,0.0);

/* refocus Ca-N and Ca-H couplings */

     delay(0.5*TC - pwcon180/2.0 - pwcoff180  \
           - WFG_START_DELAY - WFG_STOP_DELAY - POWER_DELAY );

   decphase(zero);
   decpower(pwClvl); decpwrf(d_coff180);
   simshaped_pulse("","spco180",2.0*pw,pwcoff180,zero,zero,0.0e-6,0.0e-6);	/* Bloch siegert correction */
   decphase(zero); decpower(d_c180); decpwrf(4095.0);
   decrgpulse(pwcon180,t4,0.0e-6,0.0e-6);
   decpwrf(d_coff180); decpower(pwClvl);
   dec2rgpulse(2.0*pwN,zero,0.0,0.0);
   decshaped_pulse("spco180", pwcoff180, zero, 0.0e-6, 0.0e-6);
     delay(taub/2.0);
   rgpulse(2.0*pw,zero,0.0,0.0);
     decpower(d_c90); decpwrf(4095.0);
     decphase(zero);
     delay(0.5*TC - taub/2.0 - 2.0*pw  -pwcon180/2 - 2.0*pwN - pwcoff180 \
          - WFG_START_DELAY - WFG_STOP_DELAY \
          - 2.0*POWER_DELAY);

   decrgpulse(pwc90,zero,0.0,0.0);
 
   zgradpulse(gzlvl7,gt7);
   delay(gstab);
   rgpulse(pw,zero,0.0,0.0);
   zgradpulse(gzlvl8,gt8);
   txphase(zero);
   dec2phase(zero);
   decpwrf(d_reb); decpower(pwClvl);
   delay(taua - 2.0*POWER_DELAY - gt8  - 0.5*pwreb180 );
   simshaped_pulse("","spreb180",2.0*pw,pwreb180,zero,zero,0.0e-6,0.0e-6);	/* Purge all C outside 26-46 ppm */
   zgradpulse(gzlvl8,gt8);
   decphase(zero);
   dec2power(dpwr2);    /* set power for 15N decoupling */
   decpower(dpwr);      /* set power for 13C decoupling */
   decpwrf(4095.0);
   delay(taua - 0.5*pwreb180 - gt8 - 3.0*POWER_DELAY);

   rgpulse(pw,zero,0.0,0.0);



/* BEGIN ACQUISITION */

status(C);
   setreceiver(t6);

}
