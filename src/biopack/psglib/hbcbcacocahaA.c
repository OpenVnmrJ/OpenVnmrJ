/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* hbcbcacocahaA: Autocalibrated (Automatic Pbox) version of the Kay sequence:
        hbcbcacocaha_3c_pfg1+_500a.c 


    This pulse sequence will allow one to perform the following experiment:

    3D correlation of cb, ca and ha and have separation with c'.
                        F1      CB, CA
                        F2      CA
                        F3(acq) HA

    Uses three channels:
         1)  1H        - carrier (tof) @ 4.7 ppm [H2O]
         2) 13C        - carrier (dofcacb @ 43ppm/ dof 54ppm) [CA/CB then CA]
                                 (The centre of F1 is 43ppm (dofcacb))
         3) 15N        - carrier (dof2) @ 119 ppm [centre of amide N] 


    Set dm = 'nny', dmm = 'ccp' [13C decoupling during acquisition].
    Set dm2 = 'nnn', dmm2 = 'ccc' [15N decoupling during t2 done using
                                       N 180 pulse]

    Must set phase = 1,2 and phase2 = 1,2 for States-TPPI acquisition in
    t1 [carbon]  and t2 [C'].
  
    Flags
         fsat      'y' for presaturation of H2O
         fscuba    'y' to apply scuba pulse after presaturation
         mess_flg  'y' for H2O purging in the middle of sequence
         f1180     'y' for 180deg linear phase correction in F1
                       otherwise 0deg linear phase correction in F1
         f2180     'y' for 180deg linear phase correction in F2
         c180_flg  'y' for c' 180 at t2/2 when recording F1,F3 2D

    Standard Settings
         fsat='n', fscuba='n', mess_flg='y', f1180='n', f2180='y', c180_flg='n'

    The flag f1180 should be set to 'y' if t1 is to be started at half
    dwell time. This will give -90, 180 phasing in f1. If it is set to 'n'
    the phasing will be 0,0 and will still give a perfect baseline.
    The flag f2180 should always be set to 'y' to give -90,180 in the C'
    dimension

    a pulses are all at 43 ppm; b pulses are at 54 ppm. The C' 180 shape
    is seduce for a pulses and rectangular for b pulses.

    To test the sequence set c180_flg='y', but set to n before you start 3D
    Set pwca90a to ~57 us
        pwca180a to ~ 51 us
        pwco180a = 254 us (seduce shape)
    
        pwca90b = 62 us
        pwca180b = 55.5 us

        decouple using a 100-120 us pulse

    Written by Lewis Kay 10/02/92 

    Modified by L.E.K on Aug 22/93 to improve the sequence.
    This is a three channel expt. Does not involve seduce decoupling

    To be compatible with the Inova system:
    - Added 2us delays between obspower and pulse statements (RM Jan 6/97)
    and added 4us delay on either side of the shaped C180 when c180_flg='y'
    (RM & LEK  Jan 7/97).

    REF: L. E. Kay    J. Am. Chem. Soc. 115, 2055-2057 (1993).

    Modified by L.E.Kay on Dec. 30, 1999 to add small angle phase shift
     and an additional phase cycling step

    Added parameter waltzB1 to permit user definition of 1H decoupling field (autocal!='n')



      The autocal flag is generated automatically in Pbox_bio.h
       If this flag does not exist in the parameter set, it is automatically 
       set to 'y' - yes. In order to change the default value, create the  
       flag in your parameter set and change as required. 
       For the autocal flag the available options are: 'y' (yes - by default), 
       and 'n' (no - use full manual setup, equivalent to 
       the original code). E. Kupce, Varian
    
*/

#include <standard.h>
/* #define PI 3.1416 */
#include "Pbox_bio.h"

#define CA90    "square90n 133p"      /* square 90 on Cab at 43 ppm, null at C', 133 ppm away */
#define CA180   "square180n 133p"    /* square 180 on Cab at 43 ppm, null at C', 133 ppm away */
#define CA180ps "-maxincr 2.0 -attn i"                         /* seduce 180 shape parameters */
#define CO180   "seduce 28p 133p"                /* seduce 180 on CO at 176 ppm, 133 ppm away */
#define CO180ps "-s 0.5 -attn i"                               /* seduce 180 shape parameters */
#define CA90b   "square90n 118p"      /* square 90 on Ca at 58 ppm, null at C', 133 ppm away  */
#define CA180b  "square180n 118p"     /* square 180 on Ca at 58 ppm, null at C', 118 ppm away */
#define CO90b   "square90n 118p 118p" /* square 90 on CO at 176 ppm, null at Ca, 118 ppm away */
#define CO180b  "square180  28p 118p"                          /* square 180 on CO at 176 ppm */

static shape ca90, ca180, co180, co90b, co180b, ca90b, ca180b, w16;

static int  phi1[1]  = {0},
            phi2[1]  = {1},
            phi3[2]  = {0,2},
            phi4[4]  = {1,1,3,3},
            phi5[8]  = {0,0,0,0,2,2,2,2},
            phi6[8]  = {0,0,0,0,2,2,2,2},
            phi7[16] = {0,0,0,0,0,0,0,0,
                        2,2,2,2,2,2,2,2},
            phi9[4] = {0,0,2,2}, 
            rec[16] = {0,2,0,2,2,0,2,0,
                       2,0,2,0,0,2,0,2};
           
static double d2_init=0.0, d3_init=0.0;
            
pulsesequence()
{
/* DECLARE VARIABLES */

 char       autocal[MAXSTR],  /* auto-calibration flag */
            fsat[MAXSTR],
	    fscuba[MAXSTR],
            f1180[MAXSTR],    /* Flag to start t1 @ halfdwell             */
            f2180[MAXSTR],    /* Flag to start t2 @ halfdwell             */
            mess_flg[MAXSTR], /* water purging */
            c180_flg[MAXSTR],
            spco90a[MAXSTR],
            spco180a[MAXSTR],
            spco90b[MAXSTR],
            spco180b[MAXSTR];

 int         phase, phase2, ni, 
             t1_counter,   /* used for states tppi in t1           */ 
             t2_counter;   /* used for states tppi in t2           */ 

 double      tau1,         /*  t1 delay */
             tau2,         /*  t2 delay */
             taua,         /*  ~ 1/4JCH =  1.7 ms */
             tauc1,         /* ca/cb refocus and ca/c' defocus = 4.5 ms */
             tauc2,         /* ca/cb stay and ca/c' refocus = 2.7 ms */
             taud,         /* ca-ha refocus; 1.8 ms   */
             BigTC,        /* carbon constant time period */
             dly_pg1,      /* delay for water purging */
             pwN,          /* PW90 for 15N pulse              */
             pwca90a,       /* PW90 for ca nucleus @ pwClvl         */
             pwca180a,      /* PW180 for ca at dvhpwra               */
             pwco180a,      /* PW180 for c' using seduce shape  */
             pwca90b,       /* PW90 for ca nucleus @ dhpwrb         */
             pwca180b,      /* PW180 for ca nucleus @ dvhpwrb         */
             pwco180b,      /* PW180 for c' using rectang. pulse     */
             pwco90b,      /* PW90 for co nucleus @ dhpwrb         */
             tsatpwr,      /* low level 1H trans.power for presat  */
             tpwrml,       /* power level for h decoupling  */
             tpwrmess,     /* power level for water purging */
             pwmlev,       /* h 90 pulse at tpwrml            */
             pwClvl,        /* power level for 13C pulses on dec1  
                              90 for part a of the sequence at 43 ppm */
             dvhpwra,        /* power level for 180 13C pulses at 43 ppm */
             dpwr_coa,      /* power level for C' 180 pulses at 43 ppm */
             dhpwrb,        /* power level for 13C pulses on dec1 - 54 ppm
                               90  for part b of the sequence */
             dvhpwrb,        /* power level for 13C pulses on dec1 - 54 ppm
                               180 for part b of the sequence     */
             pwNlvl,       /* high dec2 pwr for 15N hard pulses    */
             sw1,          /* sweep width in f1                    */             
             sw2,          /* sweep width in f2                    */             
             dofcacb,      /* dof for dipsi part, 43  ppm            */      
             cln_dly,    /* so that get rid of crap from hb etc with
                              zero tocsy transfer   */
             sphase,
             pwC, compC,      /* C-13 RF calibration parameters */
             compH,
             waltzB1,gstab,
             ni2=getval("ni2"),
             gt0,
             gt1,
             gt2,
             gt3,
             gt4,
             gt5,
             gt6,
             gt7,
             gzlvl0,
             gzlvl1,
             gzlvl2,
             gzlvl3,
             gzlvl4,
             gzlvl5,
             gzlvl6,
             gzlvl7;


/*  variables commented out are already defined by the system      */


/* LOAD VARIABLES */

  getstr("autocal",autocal);
  getstr("fsat",fsat);
  getstr("f1180",f1180);
  getstr("f2180",f2180);
  getstr("fscuba",fscuba);
  getstr("mess_flg",mess_flg);
  getstr("c180_flg",c180_flg);

  taua   = getval("taua"); 
  tauc1   = getval("tauc1"); 
  tauc2   = getval("tauc2"); 
  taud   = getval("taud"); 
  BigTC  = getval("BigTC");
  dly_pg1 = getval("dly_pg1");
  pwN = getval("pwN");
  tpwr = getval("tpwr");
  tsatpwr = getval("tsatpwr");
  tpwrml  = getval("tpwrml");
  tpwrmess = getval("tpwrmess");
  dpwr = getval("dpwr");
  pwNlvl = getval("pwNlvl");
  phase = (int) ( getval("phase") + 0.5);
  phase2 = (int) ( getval("phase2") + 0.5);
  sw1 = getval("sw1");
  sw2 = getval("sw2");
  dofcacb = getval("dofcacb");
  cln_dly = getval("cln_dly");
  ni = getval("ni");

  sphase = getval("sphase");

  gt0 = getval("gt0");
  gt1 = getval("gt1");
  gt2 = getval("gt2");
  gt3 = getval("gt3");
  gt4 = getval("gt4");
  gt5 = getval("gt5");
  gt6 = getval("gt6");
  gt7 = getval("gt7");
  gstab = getval("gstab");
  gzlvl0 = getval("gzlvl0");
  gzlvl1 = getval("gzlvl1");
  gzlvl2 = getval("gzlvl2");
  gzlvl3 = getval("gzlvl3");
  gzlvl4 = getval("gzlvl4");
  gzlvl5 = getval("gzlvl5");
  gzlvl6 = getval("gzlvl6");
  gzlvl7 = getval("gzlvl7");

  if(autocal[0]=='n')
  {     
    getstr("spco90a",spco90a);
    getstr("spco180a",spco180a);
    getstr("spco90b",spco90b);
    getstr("spco180b",spco180b);
    pwmlev = getval("pwmlev");
    pwca90a = getval("pwca90a");
    pwca180a = getval("pwca180a");
    pwco180a = getval("pwco180a");
    pwca90b = getval("pwca90b");
    pwca180b = getval("pwca180b");
    pwco90b = getval("pwco90b");
    pwco180b = getval("pwco180b"); 
    pwClvl = getval("pwClvl");
    dvhpwra = getval("dvhpwra");
    dpwr_coa = getval("dpwr_coa"); 
    dhpwrb = getval("dhpwrb");
    dvhpwrb = getval("dvhpwrb");
  }
  else
  {
    waltzB1=getval("waltzB1");
    pwmlev=1/(4.0*waltzB1);
    compH = getval("compH");
    tpwrml= tpwr - 20.0*log10(pwmlev/(compH*pw));
    tpwrml= (int) (tpwrml + 0.5);
    strcpy(spco90a,"Psed180_133p");
    strcpy(spco180a,"Psed180_133p");
    strcpy(spco90b,"Phard90co_118p");
    strcpy(spco180b,"Phard180co_118p");
    if (FIRST_FID)
    {
      pwC = getval("pwC");
      pwClvl = getval("pwClvl");
      compC = getval("compC");
      ca90 = pbox("cal", CA90, "", dfrq, compC*pwC, pwClvl);
      ca180 = pbox("cal", CA180, "", dfrq, compC*pwC, pwClvl);      
      co180 = pbox(spco180a, CO180, CO180ps, dfrq, compC*pwC, pwClvl);
      ca90b = pbox("cal", CA90b, "", dfrq, compC*pwC, pwClvl);
      ca180b = pbox("cal", CA180b, "", dfrq, compC*pwC, pwClvl);          
      co90b = pbox(spco90b, CO90b, CA180ps, dfrq, compC*pwC, pwClvl);
      co180b = pbox(spco180b, CO180b, CA180ps, dfrq, compC*pwC, pwClvl);
      w16 = pbox_dec("cal", "WALTZ16", tpwrml, sfrq, compH*pw, tpwr);
    }
    pwca90a = ca90.pw;       pwClvl = ca90.pwr;    
    pwca180a = ca180.pw;     dvhpwra = ca180.pwr;
    pwco180a = co180.pw;     dpwr_coa = co180.pwr;
    pwco90b = co90b.pw;      dhpwrb = co90b.pwr;
    pwco180b = co180b.pw;    
    pwca90b = ca90b.pw;          
    pwca180b = ca180b.pw;    dvhpwrb = ca180b.pwr;    
    pwmlev = 1.0/w16.dmf;
  }   

/* LOAD PHASE TABLE */

  settable(t1,1,phi1);
  settable(t2,1,phi2);
  settable(t3,2,phi3);
  settable(t4,4,phi4);
  settable(t5,8,phi5);
  settable(t6,8,phi6);
  settable(t7,16,phi7);
  settable(t9,4,phi9);
  settable(t8,16,rec);

/* CHECK VALIDITY OF PARAMETER RANGES */

    if( ni*1/(sw1) > 2.0*BigTC )
    {
        printf(" ni is too big\n");
        psg_abort(1);
    }

    if((c180_flg[A] == 'y') && (ni2>1))
    {
        printf("set c180_flg=n for C=O evolution");
        psg_abort(1);
    }

    if((dm[A] == 'y' || dm[B] == 'y'))
    {
        printf("incorrect dec1 decoupler flags!  ");
        psg_abort(1);
    }

    if((dm2[A] == 'y' || dm2[B] == 'y' || dm2[C] == 'y'))
    {
        printf("incorrect dec2 decoupler flags! Should be 'nnn' ");
        psg_abort(1);
    }

    if( pwmlev < 30.0e-6 ) 
    {
        printf("too much power during proton mlev sequence\n");
        psg_abort(1);
     }

    if( tpwrml > 53 )
     {
        printf("tpwrml is too high\n");
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

    if( dpwr2 > 50 )
    {
        printf("don't fry the probe, DPWR2 too large!  ");
        psg_abort(1);
    }

    if( pwClvl > 62 )
    {
        printf("don't fry the probe, DHPWR too large!  ");
        psg_abort(1);
    }

    if( dhpwrb > 62 )
    {
        printf("don't fry the probe, DHPWRB too large!  ");
        psg_abort(1);
    }

    if( dvhpwrb > 62 )  
    {
        printf("don't fry the probe, DVHPWRB too large!  ");
        psg_abort(1);
    }

    if( pwNlvl > 62 )
    {
        printf("don't fry the probe, DHPWR2 too large!  ");
        psg_abort(1);
    }

    if( pw > 200.0e-6 )
    {
        printf("dont fry the probe, pw too high ! ");
        psg_abort(1);
    } 
    if( pwmlev > 200.0e-6 )
    {
        printf("dont fry the probe, pwmlev too high ! ");
        psg_abort(1);
    } 
    if( pwN > 200.0e-6 )
    {
        printf("dont fry the probe, pwN too high ! ");
        psg_abort(1);
    } 
    if( pwca90a > 200.0e-6 )
    {
        printf("dont fry the probe, pwca90a too high ! ");
        psg_abort(1);
    } 
    if( pwca90b > 200.0e-6 )
    {
        printf("dont fry the probe, pwca90b too high ! ");
        psg_abort(1);
    } 
    if( pwca180b > 200.0e-6 )
    {
        printf("dont fry the probe, pwca180b too high ! ");
        psg_abort(1);
    } 
    if( pwco90b > 200.0e-6 )
    {
        printf("dont fry the probe, pwco180b too high ! ");
        psg_abort(1);
    } 

    if( gt0 > 15e-3 || gt1 > 15e-3 || gt2 > 15e-3 || gt3 > 15e-3 || gt4 > 15e-3 || gt5 > 15e-3 || gt6 > 15e-3 || gt7 > 15e-3 )
    {
        printf("gradients on for too long. Must be < 15e-3 \n");
        psg_abort(1);
    }


/*  Phase incrementation for hypercomplex 2D data */

    if (phase == 2) {
      tsadd(t3,1,4);  
    }
    if (phase2 == 2)
      tsadd(t7,1,4);

/*  Set up f1180  tau1 = t1               */
   
    tau1 = d2;
    if(f1180[A] == 'y') {
        tau1 += ( 1.0 / (2.0*sw1) );
        if(tau1 < 0.2e-6) tau1 = 0.0;
    }
        tau1 = tau1/2.0;

/*  Set up f2180  tau2 = t2               */

    tau2 = d3;
    if(f2180[A] == 'y') {
        tau2 += ( 1.0 / (2.0*sw2) - pwca180b - 2.0*pwN - (4.0/PI)*pwco90b - 2*POWER_DELAY - WFG_START_DELAY - WFG_STOP_DELAY - 2.0e-6 ); 
        if(tau2 < 0.2e-6) tau2 = 0.0;
    }
        tau2 = tau2/2.0;

/* Calculate modifications to phases for States-TPPI acquisition          */

   if( ix == 1) d2_init = d2 ;
   t1_counter = (int) ( (d2-d2_init)*sw1 + 0.5 );
   if(t1_counter % 2) {
      tsadd(t3,2,4);     
      tsadd(t8,2,4);    
    }

   if( ix == 1) d3_init = d3 ;
   t2_counter = (int) ( (d3-d3_init)*sw2 + 0.5 );
   if(t2_counter % 2) {
      tsadd(t7,2,4);  
      tsadd(t8,2,4);    
    }

/* BEGIN ACTUAL PULSE SEQUENCE */

/* Receiver off time */

status(A);
   decoffset(dofcacb);       /* initially pulse at 43 ppm */
   obspower(tsatpwr);      /* Set transmitter power for 1H presaturation */
   decpower(pwClvl);        /* Set Dec1 power for hard 13C pulses         */
   dec2power(pwNlvl);      /* Set Dec2 power for 15N hard pulses         */

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
   delay(20.0e-6);

/* first ensure that magnetization does infact start on H and not C */

   decrgpulse(pwca90a,zero,2.0e-6,2.0e-6);

   delay(2.0e-6);
   zgradpulse(gzlvl0,gt0);
   delay(gstab);

/* this is the real start */

   rgpulse(pw,zero,0.0,0.0);                    /* 90 deg 1H pulse */
   decphase(t1);
   decpower(dvhpwra);
 
   delay(2.0e-6);
   zgradpulse(gzlvl1,gt1);
   delay(2.0e-6);

   delay(taua - POWER_DELAY - gt1 - 4.0e-6);   /* taua <= 1/4JCH */                          
   simpulse(2*pw,pwca180a,zero,t1,0.0,0.0);

   decpower(pwClvl);
   txphase(t2); decphase(t3);

   delay(2.0e-6);
   zgradpulse(gzlvl1,gt1);
   delay(2.0e-6);

   delay(taua - POWER_DELAY - gt1 - 4.0e-6); 

   rgpulse(pw,t2,0.0,0.0);

   delay(2.0e-6);
   zgradpulse(gzlvl2,gt2);
   delay(gstab);

   decrgpulse(pwca90a,t3,0.0,2.0e-6);

   delay(tau1);

   decpower(dpwr_coa);
   decshaped_pulse(spco180a,pwco180a,zero,2.0e-6,0.0);

   dec2rgpulse(2*pwN,zero,0.0,0.0);

   delay(2.0e-6);
   zgradpulse(gzlvl7,gt7);
   delay(2.0e-6);

   decpower(dvhpwra);

   delay(0.8e-3 - gt7 - 4.0e-6 - 2*POWER_DELAY);
   delay(0.2e-6);
     
   rgpulse(2*pw,zero,0.0,0.0);

   decphase(t4);
   delay(BigTC - 0.8e-3);

   initval(1.0,v3);
   decstepsize(sphase);
   dcplrphase(v3);

   decrgpulse(pwca180a,t4,2.0e-6,2.0e-6);
   dcplrphase(zero);

   delay(2.0e-6);
   zgradpulse(gzlvl7,gt7);
   delay(2.0e-6);

   delay(BigTC - tau1 + 2*pwN + 2*pw - 2.0*POWER_DELAY - gt7 - 4.0e-6);
   delay(0.2e-6);

   decpower(dpwr_coa);
   decshaped_pulse(spco180a,pwco180a,zero,2.0e-6,0.0); /* bloch seigert */
   decpower(pwClvl);

   decrgpulse(pwca90a,t9,2.0e-6,0.0);

   /* H decoupling on */
   obspower(tpwrml);
   obsprgon("waltz16",pwmlev,90.0);
   xmtron();    /* TURN ME OFF  DONT FORGET  */
   /* H decoupling on */

   decpower(dpwr_coa);
   decshaped_pulse(spco180a,pwco180a,zero,2.0e-6,0.0);   /* bloch seigert */
   decpower(pwClvl);
   delay(tauc1 - 4.0*POWER_DELAY - PRG_START_DELAY - 2.0e-6);

   decpower(dvhpwra);
   decrgpulse(pwca180a,t5,2.0e-6,0.0);
   decpower(dpwr_coa);
   decshaped_pulse(spco180a,pwco180a,zero,2.0e-6,0.0);
   decpower(pwClvl);
   delay(tauc1 - 2.0*POWER_DELAY - 2.0e-6);
   decrgpulse(pwca90a,t6,2.0e-6,0.0);

   delay(2.0e-6);
   decphase(t7);

     /* H decoupling off */
     xmtroff();
     obsprgoff();
     /* H decoupling off */

   delay(2.0e-6);
   zgradpulse(gzlvl3,gt3);
   delay(gstab);

   decoffset(dof);
   hsdelay(cln_dly);
   decpower(dhpwrb);

     /* H decoupling on */
     obspower(tpwrml);
     obsprgon("waltz16",pwmlev,90.0);
     xmtron();    /* TURN ME OFF  DONT FORGET  */
     /* H decoupling on */

   delay(2.0e-6);

   decshaped_pulse(spco90b,pwco90b,t7,0.0,0.0);

   delay(tau2);
  
   if(c180_flg[A] == 'y') {
       delay(4.0e-6);
       decshaped_pulse(spco180b,pwco180b,zero,0.0,0.0);
       delay(4.0e-6);
   }
   else
   {
       decpower(dvhpwrb);
       decrgpulse(pwca180b,zero,2.0e-6,0.0);
       decpower(dhpwrb);
      
       dec2rgpulse(2*pwN,zero,0.0,0.0);
    }

   delay(tau2);

   decshaped_pulse(spco90b,pwco90b,zero,0.0,0.0);

   delay(0.2e-6);

     /* H decoupling off */
     xmtroff();
     obsprgoff();
     /* H decoupling off */

   if(mess_flg[A] == 'y') {

     obspower(tpwrmess);
     rgpulse(dly_pg1,zero,2.0e-6,2.0e-6);
     rgpulse(dly_pg1/1.62,one,2.0e-6,2.0e-6);

  }


   delay(2.0e-6);
   zgradpulse(gzlvl4,gt4);
   delay(gstab);

   obspower(tpwr);

   rgpulse(pw,zero,2.0e-6,0.0);

   delay(2.0e-6);
   zgradpulse(gzlvl4,gt4/1.73);
   delay(gstab);

  decrgpulse(pwca90b,zero,0.0,0.0);

   delay(2.0e-6);
   zgradpulse(gzlvl5,gt5);
   delay(2.0e-6);

  delay(taud - gt5 - 4.0e-6);

  rgpulse(2*pw,zero,0.0,0.0); 

  delay(tauc2 - taud - POWER_DELAY - 2*pw - 2.0e-6 - 2.0e-6);

  decshaped_pulse(spco180b,pwco180b,zero,2.0e-6,0.0);   
  decpower(dvhpwrb);
  decrgpulse(pwca180b,zero,2.0e-6,0.0);
  decpower(dhpwrb);

   delay(2.0e-6);
   zgradpulse(gzlvl5,gt5);
   delay(2.0e-6);

  delay(tauc2 - gt5 - 6.0e-6 - POWER_DELAY); 

  decshaped_pulse(spco180b,pwco180b,zero,0.0,0.0); /* bloch seigert */   
  simpulse(pw,pwca90b,zero,zero,2.0e-6,0.0);

   delay(2.0e-6);
   zgradpulse(gzlvl6,gt6);
   delay(2.0e-6);

  delay(taua - POWER_DELAY - gt6 - 4.0e-6 - 2.0e-6);

  decpower(dvhpwrb);
  simpulse(2*pw,pwca180b,zero,zero,2.0e-6,0.0);

   delay(2.0e-6);
   zgradpulse(gzlvl6,gt6);
   delay(2.0e-6);

   delay(taua - POWER_DELAY - gt6 - 4.0e-6);

   decpower(dpwr);  /* Set power for decoupling */

   rgpulse(pw,zero,0.0,0.0);  
    
/*   rcvron();  */          /* Turn on receiver to warm up before acq */ 

/* BEGIN ACQUISITION */

status(C);
setreceiver(t8);

}
