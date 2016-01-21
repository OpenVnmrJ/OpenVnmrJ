/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*  ghncaco_trosy_3DA.c - auto-calibrated version of the original sequence
 
    This pulse sequence will allow one to perform the following
    experiment:

    3D hncaco with deuterium decoupling
	F1 	CO
	F2 	N + JNH/2
	F-acq	HN- JNH/2

    This sequence uses the four channel configuration
         1)  1H             - carrier @ 4.7 ppm [H2O]
         2) 13C             - carrier @ 56 ppm (CA) and 174 ppm(Co)
         3) 15N             - carrier @ 120 ppm  
         4) 2H		    - carrier @ 4.5 ppm 

    Set dm = 'nnn', dmm = 'ccc' 
    Set dm2 = 'nnn', dmm2 = 'ccc' 

    Must set phase = 1,2, phase2=1,2 for States-TPPI
    acquisition in t1[CO] and t2 [N]. [The fids must be manipulated
    (add/subtract) with 'grad_sort_nd' program (or equivalent) before regular
    processing for non-VNMR processing.]
    
    Flags
	fsat		'y' for presaturation of H2O
	fscuba		'y' for apply scuba pulse after presaturation of H2O
	f1180		'y' for 180 deg linear phase correction in F1 
		        	otherwise 0 deg linear phase correction
	f2180		'y' for 180 deg linear phase correction in F2
			    otherwise 0 deg linear phase correction
	fc180   	'y' for CO refocusing in t1
        sel_flg         'y' for active suppression of the anti-TROSY peak
        sel_flg         'n' for relaxation suppression of the anti-TROSY peak
        cacb_dec        'y' decoupling of cb during transfer from Ca to CO
        nietl_flg       'y' for active suppression of the anti-trosy with 
                            no loss in s/n
                        (D.Nietlispach,J.Biomol.NMR, 31,161(2005))

	Standard Settings
   fsat='n',fscuba='n',mess_flg='n',f1180='y',f2180='n',fc180='n'

   Use ampmode statement = 'dddp'
   Note the final coherence transfer gradients have been split
   about the last 180 (H)

   Calibration of carbon pulses
	
        pwc90     delta/sqrt(15) selective pulse applied at d_c90
        pwca180   delta/sqrt(3) selective pulse applied at d_c180
        pwca180dec pwca180+pad
        pwcareb   reburp 180 pulse (about 300 us at 600MHz) applied at d_creb
	pwcosed    delta/sqrt(3) pulse applied at d_sed
                 USE delta/sqrt(3) and not seduce pulse

   Calibration of small angle phase shift (set ni=1, ni2=1 phase=1,
   phase2=1)
     sphase  set fc180='y' and change sphase until get a null(no signal).
                The right sphase is the value at the null plus 45 degrees

     sphase1 about zero. Calibration is the same as that for sphase

    Ref:  Daiwen Yang and Lewis E. Kay, J.Am.Chem.Soc., 121, 2571(1999)
          Diawen Yang and Lewis E. Kay, J.Biomol.NMR, 13, 3(1999)

 
Written by L.E.Kay on Nov. 16, 2001 from hncaco_trosy_4D_ydw.c

    Modified by L.E.Kay to include the nietl_flg to suppress the
     anti-trosy component so that there is not a need for the sel_flg
    Modified by E.Kupce, Jan 2005, for autocalibration
       from hncaco_trosy_3d_lek_500a.c
    Modified by G.Gray for BioPack, Feb 2005
*/

#include <standard.h>
#include "Pbox_bio.h"
#define DELAY_BLANK 0.0e-6

#define CREB180   "reburp 80p -13p"            /* RE-BURP 180 on Cab at 43 ppm, -13 ppm away */
#define CAB180ps  "-s 1.0 -attn i"                           /* RE-BURP 180 shape parameters */
#define CO180     "square180n 118p 118p"          /* hard 180 on C', at 174 ppm 118 ppm away */
#define CA180ps   "-s 0.2 -attn d"                           /* RE-BURP 180 shape parameters */
#define CA180     "square180n 118p -118p"         /* hard 180 on CA, at 56 ppm 118 ppm away  */
#define C90       "square90n 118p"                            /* hard 90 on C, on resonance  */
#define CBDEC     "WURST2 7p/4m 18.5p\" \"WURST2 13p/4m 34.5p"         /* C-beta decoupling  */
#define CBDECps   "-refofs 56p -s 4.0 -attn e"               /* RE-BURP 180 shape parameters */

static shape   cbdec, c90, creb, ca180, co180;

static int  phi1[2]  = {0,2},
            phi2[4]  = {0,0,2,2}, 
	    phi3[8]  = {1,1,1,1,3,3,3,3},
	    phi4[2]  = {0,2},
	    phi5[1]  = {0},
            rec[8]   = {0,2,2,0,2,0,0,2};

static double d2_init=0.0, d3_init=0.0;


pulsesequence()

{

/* DECLARE VARIABLES */

 char       autocal[MAXSTR],  /* auto-calibration flag */
            fsat[MAXSTR],
	    fscuba[MAXSTR],
            f1180[MAXSTR],    /* Flag to start t1 @ halfdwell             */
            f2180[MAXSTR],    /* Flag to start t2 @ halfdwell             */
            fc180[MAXSTR],    /* Flag for checking sequence               */
            ddseq[MAXSTR],    /* deuterium decoupling sequence */
            spcosed[MAXSTR],  /* waveform Co seduce 180 */
            spcareb[MAXSTR],  /* waveform Ca reburp 180 */
            spca180[MAXSTR],  /* waveform Ca hard 180   */
            sel_flg[MAXSTR],
            shp_sl[MAXSTR],
            cacb_dec[MAXSTR],
            cacbdecseq[MAXSTR],
            nietl_flg[MAXSTR];

 int         phase, phase2, ni, icosel, 
             t1_counter,   /* used for states tppi in t1           */ 
             t2_counter;   /* used for states tppi in t2           */ 

 double      tau1,         /*  t1 delay */
             tau2,         /*  t2 delay */
             taua,         /*  ~ 1/4JNH =  2.25 ms */
             taub,         /*  ~ 1/4JNH =  2.25 ms */
             tauc,         /*  ~ 1/4JNCa =  ~13 ms */
             taud,         /*  ~ 1/4JCaC' =  3~4.5 ms ms */
             bigTN,        /* nitrogen T period */
             pwc90,       /* PW90 for ca nucleus @ d_c90         */
             pwca180,      /* PW180 for ca nucleus @ d_c180         */
             pwca180dec,   /* pwca180+pad         */
             pwcareb,      /* pw180 at d_creb  ~ 1.6 ms at 600 MHz */ 
             pwcosed,      /* PW180 at d_csed  ~ 200us at 600 MHz  */
             tsatpwr,      /* low level 1H trans.power for presat  */
             d_c90,        /* power level for 13C pulses(pwc90=sqrt(15)/4delta
			      delta is the separation between Ca and Co */ 
             d_c180,	   /* power level for pwca180(sqrt(3)/2delta) */
             d_creb,	   /* power level for pwcareb */
	     d_csed,       /* power level for pwcosed */
             sw1,          /* sweep width in f1                    */             
             sw2,          /* sweep width in f2                    */             
             pw_sl,        /* selective pulse on water      */
             tpwrsl,       /* power for pw_sl               */
             at,
             sphase,	   /* small angle phase shift  */
             sphase1,
             phase_sl,

             d_cacbdec,
             pwcacbdec,
             dres_dec,

             pwD,          /* PW90 for higher power (pwDlvl) deut 90 */
             pwDlvl,       /* high power for deut 90 hard pulse */

             compC,       /* C-13 RF calibration parameters */
             pwC,
             pwClvl,

             pwN,          /* PW90 for 15N pulse              */
	     pwNlvl,       /* high dec2 pwr for 15N hard pulses    */

             gstab,

             gt1,
             gt2,
             gt3,
             gt4,
             gt5,
             gt6,
             gt7,
             gt8,
             gt9,
             gt10,

             gzlvl1,
             gzlvl2,
             gzlvl3,
             gzlvl4,
             gzlvl5,
             gzlvl6,
             gzlvl7,
             gzlvl8,
             gzlvl9,
             gzlvl10;
            
/* LOAD VARIABLES */

  getstr("autocal",autocal);
  getstr("fsat",fsat);
  getstr("f1180",f1180);
  getstr("f2180",f2180);
  getstr("fc180",fc180);
  getstr("fscuba",fscuba);
  getstr("ddseq",ddseq);
  getstr("shp_sl",shp_sl);
  getstr("sel_flg",sel_flg);
  getstr("cacb_dec",cacb_dec);

  getstr("nietl_flg",nietl_flg);

  taua   = getval("taua"); 
  taub   = getval("taub"); 
  tauc   = getval("tauc"); 
  taud   = getval("taud"); 
  bigTN = getval("bigTN");
  pwN = getval("pwN");
  tpwr = getval("tpwr");
  tsatpwr = getval("tsatpwr");
  dpwr = getval("dpwr");
  pwNlvl = getval("pwNlvl");
  pwD = getval("pwD");
  pwDlvl = getval("pwDlvl");
  phase = (int) ( getval("phase") + 0.5);
  phase2 = (int) ( getval("phase2") + 0.5);
  sw1 = getval("sw1");
  sw2 = getval("sw2");
  ni = getval("ni");
  pw_sl = getval("pw_sl");
  tpwrsl = getval("tpwrsl");
  at = getval("at");
  sphase = getval("sphase");
  sphase1 = getval("sphase1");
  phase_sl = getval("phase_sl");

  gstab = getval("gstab");

  gt1 = getval("gt1");
  if (getval("gt2") > 0) gt2=getval("gt2");
    else gt2=gt1*0.1;
  gt3 = getval("gt3");
  gt4 = getval("gt4");
  gt5 = getval("gt5");
  gt6 = getval("gt6");
  gt7 = getval("gt7");
  gt8 = getval("gt8");
  gt9 = getval("gt9");
  gt10 = getval("gt10");

  gzlvl1 = getval("gzlvl1");
  gzlvl2 = getval("gzlvl2");
  gzlvl3 = getval("gzlvl3");
  gzlvl4 = getval("gzlvl4");
  gzlvl5 = getval("gzlvl5");
  gzlvl6 = getval("gzlvl6");
  gzlvl7 = getval("gzlvl7");
  gzlvl8 = getval("gzlvl8");
  gzlvl9 = getval("gzlvl9");
  gzlvl10 = getval("gzlvl10");


  if(autocal[0]=='n')
  {     
    getstr("spcosed",spcosed);
    getstr("spcareb",spcareb);
    getstr("spca180",spca180);
    getstr("cacbdecseq",cacbdecseq);

    d_c90 = getval("d_c90");
    d_c180 = getval("d_c180");
    d_creb = getval("d_creb");
    d_csed = getval("d_csed");

    pwc90 = getval("pwc90");
    pwca180 = getval("pwca180");
    pwca180dec = getval("pwca180dec");
    pwcareb = getval("pwcareb");
    pwcosed = getval("pwcosed");

    d_cacbdec = getval("d_cacbdec");
    pwcacbdec = getval("pwcacbdec");
    dres_dec = getval("dres_dec");
  }
  else
  {        
    strcpy(spcosed,"Phard_118p");    
    strcpy(spcareb,"Preburp_-15p");    
    strcpy(spca180,"Phard_-118p");    
    strcpy(cacbdecseq,"Pcb_dec");        
    if (FIRST_FID)  
    {
      compC = getval("compC");
      pwC = getval("pwC");
      pwClvl = getval("pwClvl");
      co180 = pbox(spcosed, CO180, CA180ps, dfrq, compC*pwC, pwClvl);
      creb = pbox(spcareb, CREB180, CAB180ps, dfrq, compC*pwC, pwClvl); 
      ca180 = pbox(spca180, CA180, CA180ps, dfrq, compC*pwC, pwClvl);
      cbdec = pbox(cacbdecseq, CBDEC,CBDECps, dfrq, compC*pwC, pwClvl);  
      c90 = pbox("Phard90", C90, CA180ps, dfrq, compC*pwC, pwClvl);        
    }    
    d_c90 = c90.pwr;
    d_c180 = ca180.pwr;
    d_creb = creb.pwr;
    d_csed = co180.pwr;
    pwc90 = c90.pw;
    pwca180 = ca180.pw;
    pwca180dec = ca180.pw;
    pwcareb = creb.pw;
    pwcosed = co180.pw;
    
    d_cacbdec = cbdec.pwr;
    pwcacbdec = 1.0/cbdec.dmf;
    dres_dec = cbdec.dres;
  }   

/* LOAD PHASE TABLE */

  settable(t1,2,phi1);
  settable(t2,4,phi2);
  settable(t3,8,phi3);
  settable(t4,2,phi4);
  settable(t5,1,phi5);
  settable(t6,8,rec);

/* CHECK VALIDITY OF PARAMETER RANGES */

  if(ix==1) 
   printf("Uses shared AT in the N dimension. Choose ni2 as desired\n");
        

 if((dm[A] == 'y' || dm[B] == 'y' || dm[C] == 'y' ))
    {
        printf("incorrect dec1 decoupler flags!  ");
        psg_abort(1);
    }

    if((dm2[A] == 'y' || dm2[B] == 'y' || dm2[C] == 'y'))
    {
        printf("incorrect dec2 decoupler flags! Should be 'nnn' ");
        psg_abort(1);
    }


    if( tsatpwr > 6 )
    {
        printf("TSATPWR too large !!!  ");
        psg_abort(1);
    }

    if( dpwr > -16 )
    {
        printf("DPWR too large!  ");
        psg_abort(1);
    }

    if( dpwr2 > -16 )
    {
        printf("DPWR2 too large!  ");
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

    if( gt1 > 3e-3 || gt2 > 3e-3 || gt3 > 3e-3 
	|| gt4 > 3e-3 || gt5 > 3e-3 || gt6 > 3e-3 
	|| gt7 > 3e-3 || gt8 > 3e-3 || gt9 > 3e-3 || gt10 > 3e-3) 
    {
       printf("gti values must be < 3e-3\n");
       psg_abort(1);
    } 

    if(tpwrsl > 30) {
       printf("tpwrsl must be less than 25\n");
       psg_abort(1);
    }

    if( pwDlvl > 59) {
       printf("pwDlvl too high\n");
       psg_abort(1);
    }

    if( dpwr3 > 50) {
       printf("dpwr3 too high\n");
       psg_abort(1);
    }

    if( pw_sl > 10e-3) {
       printf("too long pw_sl\n");
       psg_abort(1);
    }

    if(d_cacbdec > 40) {
       printf("d_cacbdec is too high; < 41\n");
       psg_abort(1);
    }

    if(nietl_flg[A] == 'y' && sel_flg[A] == 'y') {
       printf("nietl_flg and sel_flg cannot both be y\n");
       psg_abort(1);
    }

    if (fc180[A] =='y' && ni > 1.0) {
       text_error("must set fc180='n' to allow C' evolution (ni>1)\n");
       psg_abort(1);
   }


/*  Phase incrementation for hypercomplex 2D data */

    if (phase == 2) tsadd(t2,1,4);

    if (phase2 == 2) { tsadd(t5,2,4); icosel = 1; }
      else icosel = -1;

    if (nietl_flg[A] == 'y') icosel = -1*icosel;

/*  Set up f1180  tau2 = t1               */

    tau1 = d2;
    if(f1180[A] == 'y') {
        tau1 += ( 1.0 / (2.0*sw1) 
              - 4.0/PI*pwc90 - POWER_DELAY - 4.0e-6 - WFG_START_DELAY
              - pwca180dec - WFG_STOP_DELAY - 2.0*pwN - POWER_DELAY
              - 4.0e-6);
    }

    if(f1180[A] == 'n') 
        tau1 = ( tau1 
              - 4.0/PI*pwc90 - POWER_DELAY - 4.0e-6 - WFG_START_DELAY
              - pwca180dec - WFG_STOP_DELAY - 2.0*pwN - POWER_DELAY
              - 4.0e-6);

        if(tau1 < 0.2e-6) tau1 = 0.2e-6;
    tau1 = tau1/2.0;

/*  Set up f2180  tau2 = t2               */
 
    tau2 = d3;
    if(f2180[A] == 'y') {
        tau2 += ( 1.0 / (2.0*sw2) );
        if(tau2 < 0.2e-6) tau2 = 0.2e-6;
    }
        tau2 = tau2/2.0;

/* Calculate modifications to phases for States-TPPI acquisition          */

   if( ix == 1) d2_init = d2 ;
   t1_counter = (int) ( (d2-d2_init)*sw1 + 0.5 );
   if(t1_counter % 2) {
      tsadd(t2,2,4);     
      tsadd(t6,2,4);    
    }

   if( ix == 1) d3_init = d3 ;
   t2_counter = (int) ( (d3-d3_init)*sw2 + 0.5 );
   if(t2_counter % 2) {
      tsadd(t3,2,4);  
      tsadd(t6,2,4);    
    }

/* BEGIN ACTUAL PULSE SEQUENCE */

status(A);
   obspower(tsatpwr);     /* Set transmitter power for 1H presaturation */
   decpower(d_c180);       /* Set Dec1 power to high power          */
   dec2power(pwNlvl);     /* Set Dec2 power for 15N hard pulses         */
   decoffset(dof);


/* Presaturation Period */

   if (fsat[0] == 'y')
   {
	delay(2.0e-5);
    	rgpulse(d1,zero,2.0e-6,2.0e-6);
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
   lk_hold();
   delay(20.0e-6);

   initval(1.0,v2);
   obsstepsize(phase_sl);
   xmtrphase(v2);
 
   /* shaped pulse */
   obspower(tpwrsl);
   shaped_pulse(shp_sl,pw_sl,one,4.0e-6,0.0);
   xmtrphase(zero);
   obspower(tpwr);  txphase(zero);
   delay(4.0e-6);
   /* shaped pulse */

   rgpulse(pw,zero,0.0,0.0);                    /* 90 deg 1H pulse */

   delay(0.2e-6);
   zgradpulse(gzlvl5,gt5);
   delay(2.0e-6);

   delay(taua - gt5 - 2.2e-6);   /* taua <= 1/4JNH */ 

   sim3pulse(2*pw,0.0,2*pwN,zero,zero,zero,0.0,0.0);

   txphase(three); dec2phase(zero); decphase(zero); 

   delay(taua - gt5 - 200.2e-6 - 2.0e-6); 

   delay(0.2e-6);
   zgradpulse(gzlvl5,gt5);
   delay(200.0e-6);

   if (sel_flg[A] == 'n') 
     {
      rgpulse(pw,three,2.0e-6,0.0);

      decpower(d_c180);

      delay(0.2e-6);
      zgradpulse(gzlvl3,gt3);
      delay(200.0e-6);

      dec2rgpulse(pwN,zero,0.0,0.0);

      delay(tauc);
 
      dec2rgpulse(2*pwN,zero,0.0,0.0);
      decrgpulse(pwca180,zero,0.0,0.0);
      dec2phase(one); 

      delay(tauc - pwca180);

      dec2rgpulse(pwN,one,0.0,0.0);
     }
    else 
     {
      rgpulse(pw,one,2.0e-6,0.0);

      decpower(d_c180);

      initval(1.0,v5);
      dec2stepsize(45.0);
      dcplr2phase(v5);

      delay(0.2e-6);
      zgradpulse(gzlvl3,gt3);
      delay(200.0e-6);

      dec2rgpulse(pwN,zero,0.0,0.0);
      dcplr2phase(zero);

      delay(1.34e-3 - SAPS_DELAY - 2.0*pw);

      rgpulse(pw,one,0.0,0.0);
      rgpulse(2.0*pw,zero,0.0,0.0);
      rgpulse(pw,one,0.0,0.0);

      delay(tauc - 1.34e-3 - 2.0*pw);

      dec2rgpulse(2*pwN,zero,0.0,0.0);
      decrgpulse(pwca180,zero,0.0,0.0);
      dec2phase(one); 

      delay(tauc - pwca180);

      dec2rgpulse(pwN,one,0.0,0.0);
     }
/* END sel_flg */

   decphase(t1);

   decpower(d_c90);

   delay(0.2e-6);
   zgradpulse(gzlvl8,gt8);
   delay(200.0e-6);

/* Cay to CaxC'z  */
   dec2phase(zero);  txphase(zero);

   /* Turn on D decoupling using the third decoupler */
   dec3phase(one);
   dec3power(pwDlvl);
   dec3rgpulse(pwD,one,4.0e-6,0.0);
   dec3phase(zero);
   dec3power(dpwr3);
   dec3unblank();
   setstatus(DEC3ch, TRUE, 'w', FALSE, dmf3);
   /* Turn on D decoupling */
 
   if (cacb_dec[A] == 'n') 
     {
      decrgpulse(pwc90,t1,2.0e-6,0.0);

      delay(taud -POWER_DELAY -4.0e-6 -WFG_START_DELAY);

      initval(1.0,v3);
      decstepsize(sphase);
      dcplrphase(v3);
     
      decpower(d_creb); 
      decshaped_pulse(spcareb,pwcareb,zero,4.0e-6,0.0);
      dcplrphase(zero);

      decpower(d_csed);
      decshaped_pulse(spcosed,pwcosed,zero,4.0e-6,0.0);

      delay(taud - WFG_STOP_DELAY 
         	- POWER_DELAY - 4.0e-6 - WFG_START_DELAY - pwcosed - WFG_STOP_DELAY  
                - POWER_DELAY - 2.0e-6);

      decpower(d_c90); 
      decrgpulse(pwc90,one,2.0e-6,0.0);
     }
    else 
     {
      decrgpulse(pwc90,t1,2.0e-6,0.0);

      /* CaCb dec on */
      decpower(d_cacbdec);
      decprgon(cacbdecseq,pwcacbdec,dres_dec);
      decon();
      /* CaCb dec on */

      delay(taud - POWER_DELAY - PRG_START_DELAY
            - PRG_STOP_DELAY
 	    - POWER_DELAY - 4.0e-6 - WFG_START_DELAY);

      /* CaCb dec off */
      decoff();
      decprgoff();
      /* CaCb dec off */

      initval(1.0,v3);
      decstepsize(sphase);
      dcplrphase(v3);
     
      decpower(d_creb); 
      decshaped_pulse(spcareb,pwcareb,zero,4.0e-6,0.0);
      dcplrphase(zero);

      decpower(d_csed);
      decshaped_pulse(spcosed,pwcosed,zero,4.0e-6,0.0);

      /* CaCb dec on */
      decpower(d_cacbdec);
      decprgon(cacbdecseq,pwcacbdec,dres_dec);
      decon();
      /* CaCb dec on */

      delay(taud - WFG_STOP_DELAY 
            - POWER_DELAY - 4.0e-6 - WFG_START_DELAY - pwcosed - WFG_STOP_DELAY  
            - POWER_DELAY - PRG_START_DELAY
            - PRG_STOP_DELAY
            - POWER_DELAY - 2.0e-6);

      /* CaCb dec off */
      decoff();
      decprgoff();
      /* CaCb dec off */
  
      decpower(d_c90); 
      decrgpulse(pwc90,one,2.0e-6,0.0);
     }
/* END cacb_dec */

   /* Turn off D decoupling */
   setstatus(DEC3ch, FALSE, 'c', FALSE, dmf3);
   dec3blank(); 
   dec3phase(three);
   dec3power(pwDlvl);
   dec3rgpulse(pwD,three,4.0e-6,0.0);
   /* Turn off D decoupling */
 
   decoffset(dof+(174-56)*dfrq);   /* change Dec1 carrier to Co  */
 
   delay(2.0e-7);
   zgradpulse(gzlvl4,gt4);
   delay(100.0e-6);

/*  t1 period for C' chemical shift evolution; Ca 180 and N 180 are used
    to decouple  */
 
   decrgpulse(pwc90,t2,2.0e-6,0.0);
   if (fc180[A]=='n')
     {
      decpower(d_c180);
      delay(tau1);
      decshaped_pulse(spca180,pwca180dec,zero,4.0e-6,0.0);
      dec2rgpulse(2*pwN,zero,0.0,0.0);
      delay(tau1);
      decpower(d_c90);
     }
    else
      decrgpulse(2*pwc90,zero,0.0,0.0);
 
   decrgpulse(pwc90,zero,4.0e-6,0.0);
 
   decoffset(dof);  /* set carrier to Ca */

 
   delay(2.0e-7);
   zgradpulse(gzlvl9,gt9);
   delay(100.0e-6);

/*  Refocusing  CayC'z to Cax  */

   /* Turn on D decoupling using the third decoupler */
   dec3phase(one);
   dec3power(pwDlvl);
   dec3rgpulse(pwD,one,4.0e-6,0.0);
   dec3phase(zero);
   dec3power(dpwr3);
   dec3unblank();
   setstatus(DEC3ch, TRUE, 'w', FALSE, dmf3);
   /* Turn on D decoupling */

   if (cacb_dec[A] == 'n') 
     {
      decrgpulse(pwc90,zero,0.0e-6,0.0);

      delay(taud - POWER_DELAY 
	      - 4.0e-6 - WFG_START_DELAY - pwcosed - WFG_STOP_DELAY
	      - POWER_DELAY - 4.0e-6 - WFG_START_DELAY); 
 
      decpower(d_csed);
      decshaped_pulse(spcosed,pwcosed,zero,4.0e-6,0.0);

      decpower(d_creb);
      initval(1.0,v4);
      decstepsize(sphase1);
      dcplrphase(v4);

      decshaped_pulse(spcareb,pwcareb,zero,4.0e-6,0.0);
      dcplrphase(zero);
 
      delay(taud - WFG_STOP_DELAY
	      - POWER_DELAY 
	      - 4.0e-6);

      decpower(d_c90);
      decrgpulse(pwc90,one,4.0e-6,0.0);
     }
    else
     {
      decrgpulse(pwc90,zero,0.0e-6,0.0);

      /* CaCb dec on */
      decpower(d_cacbdec);
      decprgon(cacbdecseq,pwcacbdec,dres_dec);
      decon();
      /* CaCb dec on */
  
      delay(taud 
              - POWER_DELAY - PRG_START_DELAY
              - PRG_STOP_DELAY
              - POWER_DELAY 
	      - 4.0e-6 - WFG_START_DELAY - pwcosed - WFG_STOP_DELAY
	      - POWER_DELAY - 4.0e-6 - WFG_START_DELAY); 

      /* CaCb dec off */
      decoff();
      decprgoff();
      /* CaCb dec off */
 
      decpower(d_csed);
      decshaped_pulse(spcosed,pwcosed,zero,4.0e-6,0.0);

      decpower(d_creb);
      initval(1.0,v4);
      decstepsize(sphase1);
      dcplrphase(v4);

      decshaped_pulse(spcareb,pwcareb,zero,4.0e-6,0.0);
      dcplrphase(zero);

      /* CaCb dec on */
      decpower(d_cacbdec);
      decprgon(cacbdecseq,pwcacbdec,dres_dec);
      decon();
      /* CaCb dec on */
 
      delay(taud - WFG_STOP_DELAY
              - POWER_DELAY - PRG_START_DELAY
              - PRG_STOP_DELAY
	      - POWER_DELAY 
	      - 4.0e-6);

      /* CaCb dec off */
      decoff();
      decprgoff();
      /* CaCb dec off */
  
      decpower(d_c90);
      decrgpulse(pwc90,one,4.0e-6,0.0);
     }
/* END cacb_dec */

   /* Turn off D decoupling */
   setstatus(DEC3ch, FALSE, 'c', FALSE, dmf3);
   dec3blank(); 
   dec3phase(three);
   dec3power(pwDlvl);
   dec3rgpulse(pwD,three,4.0e-6,0.0);
   /* Turn off D decoupling */

   decpower(d_c180);
   txphase(zero);

   delay(2.0e-7);
   zgradpulse(gzlvl10,gt10);
   delay(100.0e-6);

/* Constant t2 period  */

   if (bigTN - tau2 >= 0.2e-6) 
     {
      dec2rgpulse(pwN,t3,2.0e-6,0.0);

      dec2phase(t4);

      delay(bigTN - tau2 + pwca180);

      dec2rgpulse(2*pwN,t4,0.0,0.0);
      decrgpulse(pwca180,zero,0.0,0.0);
      dec2phase(t5);

      decpower(d_csed);

      delay(bigTN - gt1 - 502.0e-6 - 2.0*GRADIENT_DELAY - POWER_DELAY
            - WFG_START_DELAY - pwcosed - WFG_STOP_DELAY);

      delay(2.0e-6);
      zgradpulse(gzlvl1,gt1);
      delay(500.0e-6);

      decshaped_pulse(spcosed,pwcosed,zero,0.0,0.0); 

      delay(tau2);

      sim3pulse(pw,0.0e-6,pwN,zero,zero,t5,0.0,0.0);
     }
    else 
     {
      dec2rgpulse(pwN,t3,2.0e-6,0.0);

      dec2rgpulse(2.0*pwN,t4,2.0e-6,2.0e-6);
      dec2phase(t5);

      delay(tau2 - bigTN);
      decrgpulse(pwca180,zero,0.0,0.0);

      decpower(d_csed);
   
      delay(bigTN - pwca180 - POWER_DELAY
            - gt1 - 502.0e-6 - 2.0*GRADIENT_DELAY
            - WFG_START_DELAY - pwcosed - WFG_STOP_DELAY);

      delay(2.0e-6);
      zgradpulse(gzlvl1,gt1);
      delay(500.0e-6);

      decshaped_pulse(spcosed,pwcosed,zero,0.0,0.0); 

      delay(tau2);

      sim3pulse(pw,0.0e-6,pwN,zero,zero,t5,0.0,0.0);
     }

   if (nietl_flg[A] == 'n') 
     {
      delay(0.2e-6);
      zgradpulse(gzlvl6,gt6);
      delay(2.0e-6);
  
      dec2phase(zero);
      delay(taub - gt6 - 2.2e-6);
  
      sim3pulse(2*pw,0.0e-6,2*pwN,zero,zero,zero,0.0,0.0);
  
      delay(0.2e-6);
      zgradpulse(gzlvl6,gt6);
      delay(200.0e-6);
      
      delay(taub - gt6 - 200.2e-6);
      txphase(one);
      dec2phase(one);
     
      sim3pulse(pw,0.0e-6,pwN,one,zero,one,0.0,0.0);
    
      delay(0.2e-6);
      zgradpulse(gzlvl7,gt7);
      delay(2.0e-6);
    
      txphase(zero);
      dec2phase(zero);
    
      delay(taub - gt7 - 2.2e-6);
    
      sim3pulse(2*pw,0.0e-6,2*pwN,zero,zero,zero,0.0,0.0);
  
      delay(0.2e-6);
      zgradpulse(gzlvl7,gt7);
      delay(200.0e-6);
  
      delay(taub - gt7 - 200.2e-6);

      sim3pulse(pw,0.0e-6,pwN,zero,zero,zero,0.0,0.0);

     }
   else 
     {

      /* shaped pulse */
      obspower(tpwrsl);
      shaped_pulse(shp_sl,pw_sl,zero,4.0e-6,0.0);
      obspower(tpwr);  txphase(zero);
      delay(4.0e-6);
      /* shaped pulse */
 
      delay(0.2e-6);
      zgradpulse(gzlvl6,gt6);
      delay(2.0e-6);
  
      dec2phase(zero);
      delay(taub - POWER_DELAY - 4.0e-6 - WFG_START_DELAY - pw_sl
                 - WFG_STOP_DELAY - POWER_DELAY - 4.0e-6
                 - gt6 - 2.2e-6);
  
      sim3pulse(2*pw,0.0e-6,2*pwN,zero,zero,zero,0.0,0.0);
      txphase(one);
      dec2phase(zero);
    
      delay(0.2e-6);
      zgradpulse(gzlvl6,gt6);
      delay(200.0e-6);
      
      delay(taub - gt6 - 200.2e-6);
   
      sim3pulse(pw,0.0e-6,pwN,one,zero,zero,0.0,0.0);
   
      delay(0.2e-6);
      zgradpulse(gzlvl7,gt7);
      delay(2.0e-6);
    
      txphase(zero);
      dec2phase(zero);
  
      delay(taub - gt7 - 2.2e-6);
   
      sim3pulse(2*pw,0.0e-6,2*pwN,zero,zero,zero,0.0,0.0);
      txphase(one);
      dec2phase(one);
   
      delay(0.2e-6);
      zgradpulse(gzlvl7,gt7);
      delay(200.0e-6);
    
      delay(taub - gt7 - 200.2e-6);
  
      sim3pulse(pw,0.0e-6,pwN,one,zero,one,0.0,0.0);
      txphase(zero);
    }
 
   delay(gt2 +gstab -0.5*(pwN -pw) -2.0*pw/PI);
 
   rgpulse(2*pw,zero,0.0,0.0);
 
   delay(2.0e-6);
   zgradpulse(icosel*gzlvl2, gt2);
   decpower(dpwr);
   dec2power(dpwr2);
   delay(gstab -2.0e-6 -2.0*GRADIENT_DELAY -2.0*POWER_DELAY);

   lk_sample();
status(C);
   setreceiver(t6);

}
