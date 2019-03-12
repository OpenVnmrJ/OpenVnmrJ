/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* ghnha.c,  

  3D HNHA experiments for measuring HN-HA coupling constants

    Ref:
      1  Vuister and Bax, JACS, 115, 7772-7777 (1993)
         (Original pulse sequence)
      2  Hitoshi Kuboniwa, Stephan Grzesiek, Frank Delaglio and Ad Bax,
         J. Biol. NMR, 4, 871-878 (1994)
         (Using Jump-Return for water suppression)
      3  Weixing Zhang, Thomas E. Smithgall, and William H. Gmeiner,
         J. Biol. NMR, 10, 263-272 (1997)
         (Using Water-Gate for water suppression)
      4  Stephan Grzesiek and Ad Bax,  JACS, 117, 5312-5315 (1995)
         (Better use of gradient pulses and delay arrangement)

  The (HN-HA) coupling constant (J) is related to the intensities of cross peak
  (Ic) and diagonal peak (Id) according to the following equation (Ref 4):

     Ic/Id ~ -tan[PI*J*(del1+del2)]*tan[PI*J(del1+del2)]


  Written by Weixing Zhang,  September 23, 1998
     (updated for 13C decoupling, Feb 1999)
  St. Jude Children's Research Hospital.
  Memphis, TN 38105
  (901)495-3169

  Uses three channels:
	1)  1H  (t1, HA), (t3, HN) 	-carrier at 4.7 ppm (tof)
	2) 13C                      	-carrier at 174 ppm (dof)
 	3) 15N  (t2, 15N)   		-carrier at 118 ppm (dof2)


  JR_flg  = y     using jump-return water suppression (ref 2).
                   uses tau_JR interpulse delay with adjustable width
                   of second pulse (pwJR).
          = n     using watergate for water suppression (ref 3).

  mag_flg = y     using magic angle pulsed field gradient for watergate.
            n     using z-axis gradient only.

  flg_3919='y'    uses 3919 watergate with tau_3919 delay.
  flg_3919= n     uses soft-pulse watergate (when JR_flg='n'). Small-angle
                   phase correction (phasestep) in 0.25deg steps optional.
                   BioPack "H2Osinc.RF" shape(1418 steps)
                   is used as default. Use pwHs=1418*n for no waveform
                   error messages. Since only NH is detected, 1.418 msec is
                   a reasonably selected pulse.
  c13_flg='y'     for 13C-labelled protein (decouples Ca and CO)
                   uses CaCO.DEC created by Pbox

        (modified for BioPack 22mar1999 by GG, Varian Palo Alto)


    Modified the amplitude of the flipback pulse(s) (pwHs) to permit user adjustment around
    theoretical value (tpwrs). If tpwrsf < 4095.0 the value of tpwrs is increased 6db and
    values of tpwrsf of 2048 or so should give equivalent amplitude. In cases of severe
    radiation damping( which happens during pwHs) the needed flip angle may be much less than
    90 degrees, so tpwrsf should be lowered to a value to minimize the observed H2O signal in 
    single-scan experiments (with ssfilter='n').(GG jan01)


    Radiation Damping:
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


*/

#include <standard.h> 
static int

            phi1[2] = {0,2},
            phi2[4]=  {0,0,2,2},
            phi3[16] ={0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3},
            phi4[4] = {0,0,2,2},
            phi6[1] = {0},
            phi7[4] = {0,2,2,0}, 
            phi8[4] = {0,0,1,1},
            phi9[4] = {2,2,3,3},

            rec[8] = {0,2,2,0,2,0,0,2},
            recjr[8] ={0,0,1,1,2,2,3,3};

extern int dps_flag;       
static double d2_init=0.0, d3_init=0.0;
            
void pulsesequence()
{

/* DECLARE VARIABLES */

 char       f1180[MAXSTR],    
            f2180[MAXSTR],   
            mag_flg[MAXSTR],
            c13_flg[MAXSTR],
            cdecshp[MAXSTR],
            JR_flg[MAXSTR],
            flg_3919[MAXSTR];

 int            
            ni2,
            t1_counter,   
            t2_counter;   

double  gzcal = getval("gzcal"),
        dfrq = getval("dfrq"),
        factor = 0.08, /* used for 3-9-19 water gate */
        pwJR = getval("pwJR"),    /* 2nd JR pulse  */
        tauJR = getval("tauJR"),  /* 90 us, Jump-Return delay  */
        tau_3919 = getval("tau_3919"),  /*  150 us, delay for 3919 water gate  */
            tau1,       
            tau2,       
            tau2a,
            tau2b,

            bigT1,        /*  ~ 7.5 ms,  INPUT      */
            bigT2,        /*  ~ 12.5 ms,  INPUT     */
            ratio,        /* bigT2/(bigT1 + bigT2)  */

            taua,         /*  ~ 1/2JNH =  5.0 ms , calculate */
            taub,         /*  same as taua, calculate  */
            del1,         /* BigT2 + 2.0*pwN      */
            del2,         /* BigT2 - 2.0*pw         */
            tauc,         /* 2.5 ms                 */

            pwClvl = getval("pwClvl"),
            pwC = getval("pwC"),
            cdecpwr = getval("cdecpwr"),
            cdecdmf = getval("cdecdmf"),
            cdecdres = getval("cdecdres"),

            pwNlvl,         /* power level for 15N hard pulse  */
            pwN,

  compH = getval("compH"),         /* adjustment for C13 amplifier compression */
  pwHs = getval("pwHs"),	        /* H1 90 degree pulse length at tpwrs */
  tpwrsf = getval("tpwrsf"),  /* fine power adjustment of flipback pulse      */
  tpwrs,	  	              /* power for the pwHs ("H2Osinc") pulse */
  phasestep = getval("phasestep"), /* # of 0.25deg phase correction for H2Osinc */
	
            gt0,
            gt1,
            gt2,
            gt3,
            gt4,
            gt5,

            gzlvl0,
            gzlvl1,
            gzlvl2,
            gzlvl3,
            gzlvl4,
            gzlvl5;
 /* selective H20 one-lobe sinc pulse */
 tpwrs = tpwr - 20.0*log10(pwHs/((compH*pw)*1.69));   /* needs 1.69 times more */
 tpwrs = (int) (tpwrs + 0.5);                           /* power than a square pulse */
           
/* LOAD VARIABLES */

  getstr("f1180",f1180);
  getstr("f2180",f2180);
  getstr("mag_flg", mag_flg);
  getstr("c13_flg", c13_flg);
  getstr("cdecshp", cdecshp);
  getstr("JR_flg", JR_flg);
  getstr("flg_3919",flg_3919);

  bigT1 = getval("bigT1");
  bigT2 = getval("bigT2");
  tauc = getval("tauc");

  pwNlvl = getval("pwNlvl");
  pwN = getval("pwN");

  sw1 = getval("sw1");
  sw2 = getval("sw2");
  ni = getval("ni");
  ni2 = getval("ni2");
  
  gt0 = getval("gt0");
  gt1 = getval("gt1");
  gt2 = getval("gt2");
  gt3 = getval("gt3");
  gt4 = getval("gt4");
  gt5 = getval("gt5");

  gzlvl0 = getval("gzlvl0");
  gzlvl1 = getval("gzlvl1");
  gzlvl2 = getval("gzlvl2");
  gzlvl3 = getval("gzlvl3");
  gzlvl4 = getval("gzlvl4");
  gzlvl5 = getval("gzlvl5");

/* LOAD PHASE TABLE */


  settable(t1, 2, phi1);
  settable(t2, 4, phi2);
  settable(t3, 16, phi3);
  settable(t4, 4, phi4);
  settable(t6, 1, phi6);
  settable(t7, 4, phi7);
  settable(t8, 4, phi8);
  settable(t9, 4, phi9);

  if (JR_flg[A] == 'y')
  {
     settable(t16,8,recjr);
  }
  else
  {
     settable(t16,8,rec);
  }
  
/*  CALCULATE DELAYS  */

  ratio = bigT2/(bigT1 + bigT2);
  del1 = bigT2 + 2.0*pwN;
  taua = del1 + 2.0*pw - bigT1 - pwN;

  del2 = bigT2 - 2.0*pw;
  taub = del2 - bigT1 -  3.0*pwN;

  if(dps_flag)
  {
     printf("H-H coupling period (del1+del2):  %f + %f = %f sec \n",del1,del2,(del1+del2));
  }

/* CHECK VALIDITY OF PARAMETER RANGES */

  if(ratio*ni2/sw2 > 2.0*(bigT2 - gt1 - gt2 - 200.0e-6))
  {
     printf("ni2 is too big, should be < %f\n",2.0*sw2*(bigT2-gt1-gt2-200e-6)/ratio);
     psg_abort(1);
  }

  if (gt1 < -0.2e-6 || gt2 < -0.2e-6 || gt3 < -0.2e-6)
  {
     printf("Incorrect range of gt1, or gt2, or gt3\n");
     psg_abort(1);
  }

  if((dm[A] == 'y' || dm[B] == 'y'  || dm[C] == 'y'))
  {
      printf("incorrect dec1 decoupler flags! Should be 'nnn' ");
      psg_abort(1);
  }

  if((dm2[A] == 'y' || dm2[B] == 'y'))
  {
      printf("incorrect dec2 decoupler flags! Should be 'nny' or 'nnn' ");
      psg_abort(1);
  }

  if( dpwr2 > 50 )
  {
      printf("don't fry the probe, DPWR2 too large!  ");
      psg_abort(1);
  }

/*  Phase incrementation for hypercomplex 2D data */

  if (phase1 == 2)
  {
     tsadd(t1,1,4); 
     tsadd(t6, 1, 4);
  }

  if (phase2 == 2) 
  {
     tsadd(t2, 1, 4);   
  }

/*  Set up f1180  tau1 = t1               */
   
  tau1 = d2 - 2.0*pwN - 4.0*pw/PI;
  if(f1180[A] == 'y') 
  {
     tau1 += (1.0/(2.0*sw1));
  }
  if(tau1 < 0.2e-6) tau1 = 0.0;
  tau1 = tau1/2.0;

/*  Set up f2180  tau2 = t2               */

  tau2 = d3;

  if(f2180[A] == 'y') 
  {
     tau2 += (1.0/(2.0*sw2));
  }
  if(tau2 < 0.2e-6) tau2 = 0.0;
  tau2a = ratio*tau2/2.0;
  tau2b = (1.0 - ratio)*tau2/2.0;

/* Calculate modifications to phases for States-TPPI acquisition          */

  if( ix == 1) d2_init = d2 ;
  t1_counter = (int)((d2-d2_init)*sw1 + 0.5);
    
  if((t1_counter % 2)) 
  {
     tsadd(t1,2,4);     
     tsadd(t6, 2, 4);
     tsadd(t16,2,4);    
  }

  if( ix == 1) d3_init = d3 ;
  t2_counter = (int)((d3-d3_init)*sw2 + 0.5);
  if((t2_counter % 2)) 
  {
     tsadd(t2,2,4);  
     tsadd(t16,2,4);    
  }

  obsstepsize(0.25);
  if (phasestep < 0.0) phasestep = phasestep + 4*360.0;
  initval(phasestep, v1);

/* BEGIN ACTUAL PULSE SEQUENCE */

status(A);
  delay(2.0e-6);
  obspower(tpwr);        
  decpower(pwClvl);
  dec2power(pwNlvl);
  decoffset(dof-59*dfrq);
  dec2offset(dof2);
  txphase(t1);
  decphase(zero);
  dec2phase(zero);
  delay(d1);
  rcvroff();

  dec2rgpulse(pwN, zero, 1.0e-6, 1.0e-6);
  if (c13_flg[A] == 'y') decrgpulse(pwC, zero, 1.0e-6, 1.0e-6);
  decoffset(dof);
  zgradpulse(gzlvl0,gt0);
  decpower(cdecpwr);
  delay(1.0e-3);
   
status(B);
  dec2phase(t2);
  rgpulse(pw,t1,rof1,0.0);            
  delay(2.0e-6);
  if (gt1 > 0.2e-6)
  {
     zgradpulse(gzlvl1,gt1);
  }
  else
  {
     delay(2.0*GRADIENT_DELAY);
  }
  delay(taua - gt1 - 2.0*GRADIENT_DELAY - 2.0e-6 - PRG_START_DELAY);
  if (c13_flg[A] == 'y')
  {
     decprgon(cdecshp, 1.0/cdecdmf, cdecdres);
     decon();
  }
  else
  {
     delay(PRG_START_DELAY);
  }

  dec2rgpulse(pwN, t2, 0.0, 0.0);
  delay(2.0e-6);
  if(gt2 > 0.2e-6)
  {
     zgradpulse(gzlvl2,gt2);
  }
  txphase(t6);
  delay(bigT1 - gt2 - 2.0*pw -  2.0e-6);
  rgpulse(2.0*pw, t6, 0.0, 0.0);
  delay(tau2a);
  dec2rgpulse(2.0*pwN, zero, 0.0, 0.0);
  delay(2.0e-6);
  if (gt1 > 0.2e-6)
  {
     zgradpulse(gzlvl1,gt1);
  }
  else
  {
     delay(2.0*GRADIENT_DELAY);
  }
  delay(100.0e-6);            /* added on 1-8-98  */
  if (gt2 > 0.2e-6)
  {
     zgradpulse(gzlvl2,gt2);
  }
  dec2phase(t3);
  delay(bigT2 - tau2a - 100.0e-6 - gt1 - gt2 - 2.0*GRADIENT_DELAY - 2.0e-6);
  rgpulse(pw, t6, 0.0, 0.0);
  txphase(t4);
  delay(tau1);

  dec2rgpulse(2.0*pwN, t3, 0.0, 0.0);
  
  delay(tau1);
  rgpulse(pw, t4, 0.0, 0.0);
  delay(2.0e-6);
  if (gt1 > 0.2e-6)
  {
     zgradpulse(gzlvl1,gt1);
  }
  else
  {
     delay(2.0*GRADIENT_DELAY);
  }
  delay(1.0e-3);  /* added on 1-8-98  */
  if (gt3 > 0.2e-6)
  {
     zgradpulse(gzlvl3,gt3);
  }
  txphase(zero);
  dec2phase(zero);
  delay(del2 - 1.0e-3 - gt1 - gt3 - 2.0*GRADIENT_DELAY - 2.0e-6);
  rgpulse(2.0*pw, zero, 0.0, 0.0);
  delay(tau2b);
  dec2rgpulse(2.0*pwN, zero, 0.0, 0.0);
  delay(2.0e-6);
  if(gt3 > 0.2e-6)
  {
     zgradpulse(gzlvl3,gt3);
  }
  delay(bigT1 - tau2b - gt3 - 2.0e-6);
  dec2rgpulse(pwN, zero, 0.0, 0.0);
  delay(2.0e-6);
  if (c13_flg[A] == 'y')
  {
     decprgoff();
     decoff();
  }
  else
  {
     delay(PRG_STOP_DELAY);
  }
  if (gt1 > 0.2e-6)
  {
     zgradpulse(gzlvl1,gt1);
  }
  else
  {
     delay(2.0*GRADIENT_DELAY);
  }
  txphase(t7);

  if (JR_flg[A] == 'y')
  {
     delay(taub - gt1 - 2.0*GRADIENT_DELAY - PRG_STOP_DELAY);
     rgpulse(pw, t7, 0.0, 2.0e-6);
     if (mag_flg[A] == 'y')
     {
        magradpulse(gzcal*gzlvl5, gt5);
     }
     else
     {
        zgradpulse(gzlvl5,gt5);
     } 
     txphase(t8);
     dec2power(dpwr2);
     rcvron();
     delay(500.0e-6);
     rgpulse(pw, t8, rof1, rof1);
     txphase(t9);
     delay(tauJR);
     rgpulse(pwJR, t9, rof1, 0.0);
     statusdelay(C,rof1);
  }
  else
  {
     delay(taub/2.0 - pwN - gt1 - 2.0*GRADIENT_DELAY - 2.0e-6 - PRG_STOP_DELAY);
     dec2rgpulse(2.0*pwN, zero, 0.0, 0.0);
     delay(taub/2.0 - pwN);
     rgpulse(pw, t7, 0.0, 2.0e-6);
     if (mag_flg[A] == 'y')
     {
        magradpulse(gzcal*gzlvl5, gt5);
     }
     else
     {
        zgradpulse(gzlvl5,gt5);
     }

     if (flg_3919[A] == 'y')
     {
        delay(tauc - 31.0*factor*pw - 2.5*tau_3919 - gt5 - 2.0e-6);
        rgpulse(pw*factor*3.0, two, 0.0, 0.0);
        delay(tau_3919);
        rgpulse(pw*factor*9.0, two, 0.0, 0.0);
        delay(tau_3919);
        rgpulse(pw*factor*19.0, two, 0.0, 0.0);
        delay(tau_3919/2.0 - pwN);
        dec2rgpulse(2.0*pwN, zero, 0.0, 0.0);
        delay(tau_3919/2.0 - pwN);
        rgpulse(pw*factor*19.0, zero, 0.0, 0.0);
        delay(tau_3919);
        rgpulse(pw*factor*9.0, zero, 0.0, 0.0);
        delay(tau_3919);
        rgpulse(pw*factor*3.0, zero, 0.0, 0.0);
        delay(100.0e-6);
        if (mag_flg[A] == 'y')
        {
           magradpulse(gzcal*gzlvl5, gt5);
        }
        else
        {
           zgradpulse(gzlvl5,gt5);
        }
        dec2power(dpwr2);
        delay(tauc - 31.0*factor*pw - 2.5*tau_3919 - gt5 - 200.0e-6);
        rcvron();
        statusdelay(C,100.0e-6);
     }
     else
     {
        if (tpwrsf<4095.0){obspower(tpwrs+6.0); obspwrf(tpwrsf);}
        else obspower(tpwrs);
        txphase(two);
        xmtrphase(v1);
        delay(tauc - pwHs - gt5 );
        shaped_pulse("H2Osinc", pwHs, two, rof1, 0.0);
        obspower(tpwr); obspwrf(4095.0);
        xmtrphase(zero);
        txphase(zero);
        sim3pulse(2.0*pw, 0.0, 2.0*pwN, zero, zero, zero, rof1, rof1);
        if (tpwrsf<4095.0){obspower(tpwrs+6.0); obspwrf(tpwrsf);}
        else obspower(tpwrs);
        txphase(two);
        xmtrphase(v1);
        shaped_pulse("H2Osinc", pwHs, two, rof1, rof1);
        xmtrphase(zero);
        obspower(tpwr); obspwrf(4095.0);
        dec2power(dpwr2);
        delay(100.0e-6-rof1);
        if (mag_flg[A] == 'y')
        {
           magradpulse(gzcal*gzlvl5, gt5);
        }
        else
        {
           zgradpulse(gzlvl5,gt5);
        }
        delay(tauc - pwHs  - gt5 - 200.0e-6);
        rcvron();
        statusdelay(C,100.0e-6);
     }
  }

   setreceiver(t16);
}
