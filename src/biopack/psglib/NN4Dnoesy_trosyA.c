/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*NN4Dnoesy_trosyA
       This pulse sequence will allow one to perform the following experiment:
	4D 15N 15N edited noesy, with TROSY detection in F3,F4. 
         (modified from N_N_noesy_4d_pfg_trosy_lek.c)


      Set N_flg to y to eliminate evolution during t2
      (during the running of some test 2D sets before running the 4D)

      Because of the large numbers of shaped water pulses do not use
       rectangular ones.(use seduce or sinc).

      Written by L.E.Kay on Nov 4, 2001 
      Modified by L.E.Kay on Nov. 8 to allow for diagonal peak suppression.
       Set diag_supp to y.

      Modified by L.E.Kay on Nov. 9 to use gd. coherence selection
      (Unlike the 4D N_N experiment which uses adiabatic pulses generated
       as .DEC files, the present version uses adiabatic pulses)

       The autocal and checkofs flags are generated automatically in Pbox_bio.h
       If these flags do not exist in the parameter set, they are automatically 
       set to 'y' - yes. In order to change their default values, create the  
       flag(s) in your parameter set and change them as required. 
       The available options for the checkofs flag are: 'y' (yes) and 'n' (no). 
       The offset (tof, dof, dof2 and dof3) checks can be switched off  
       individually by setting the corresponding argument to zero (0.0).
       For the autocal flag the available options are: 'y' (yes - by default), 
       'q' (quiet mode - suppress Pbox output), 'r' (read from file, no new  
       shapes are created), 's' (semi-automatic mode - allows access to user  
       defined parameters) and 'n' (no - use full manual setup, equivalent to 
       the original code).

       The shape "composite.RF" should be in shapelib. It is just a 90x-180y-90x

       0.0    1023.0  1.0
       90.0   1023.0  1.0
       90.0   1023.0  1.0
       0.0    1023.0  1.0

       Modified for BioPack by GG, Palo Alto, June 2004
*/

#include <standard.h>
#include "Pbox_bio.h"

static double d2_init = 0.0, d3_init=0.0, d4_init=0.0;
static double   H1ofs=4.7, C13ofs=19.6, N15ofs=120.0, H2ofs=3.2;
static shape H2Osel,C13adiab;

static int phi1[4] = {0,0,2,2},
           phi2[1] = {0},
           phi3[1] = {2}, 
           phi4[1] = {1}, 
           phi5[1] = {1},
           phi6[1] = {3},
           phi7[1] = {1},
           phi8[1] = {3},
           phi9[2] = {0,1},
           rec[4] =  {0,2,2,0};

void pulsesequence()
{
/* DECLARE VARIABLES */

 char        fscuba[MAXSTR],f1180[MAXSTR],f2180[MAXSTR],fsat[MAXSTR],
             shape[MAXSTR],sh_ad[MAXSTR],f3180[MAXSTR],
             N_flg[MAXSTR],diag_supp[MAXSTR];

 int	     phase,
             phase2,
             phase3,
             t1_counter,
             t2_counter,
             t3_counter, icosel;

 double      hscuba,                /* length of 1/2 scuba delay */
             pwx2,                  /* PW90 for X-nuc            */
             tsatpwr,               /* low power level for presat*/
             dhpwr2,                /* power level for X hard pulses */
             jxh,                   /* coupling for XH           */
             tauxh,                 /* delay = 1/(2jxh)          */
	     tau1,	      	    /* t1/2  H */
	     tau2,	      	    /* t2/2 N */
	     tau3,	      	    /* t3/2 N */
	     sw1,                  /* spectral width in 1H dimension */
             sw2,                  /* spectral width in 15N dimension */
             sw3,                  /* spectral width in 15N dimension */
             MIX,                  /* Total Mixing time for noesy portion */
             pw_sl,                /* selective 2ms pulse on water */
             tpwrsl,               /* power level for pw_sl   */ 
             ppm,nst,pws,bw,ofs,   /* used by Pbox */
             pwN,pwNlvl,compH,compC,pwC,pwClvl,
             d_ad,                 /* C high power for adiabatic pulses */
             pwc_ad,               /* C 90 pulse width   */

             zeta,                /* Bax-Logan trick */
             zeta1,               /* Bax-Logan trick */

             BigT,
             BigT1,

             gzlvl0,
             gzlvl1,
             gzlvl2,
             gzlvl3,
             gzlvl4,
             gzlvl5,
             gzlvl6,
             gzlvl7,
             gzlvl8,
             gzlvl9,
             gzlvl10,
             gzlvl11,
             gzlvl12,

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
             gt11,
             gt12;

/* LOAD VARIABLES */
          pwNlvl=getval("pwNlvl");
          pwN=getval("pwN");
          compC=getval("compC");
          pwC=getval("pwC");
          pwClvl=getval("pwClvl");
          compH=getval("compH");

  jxh = getval("jxh");
  dhpwr2 = getval("dhpwr2"); 
  pwx2 = getval("pwx2");
  tsatpwr = getval("tsatpwr");
  hscuba = getval("hscuba");
  phase = (int) (getval("phase") + 0.5);
  phase2 = (int) (getval("phase2") + 0.5);
  phase3 = (int) (getval("phase3") + 0.5);
  sw1 = getval("sw1");
  sw2 = getval("sw2");
  sw3 = getval("sw3");
  MIX  = getval("MIX");
  pw_sl = getval("pw_sl");
  tpwrsl = getval("tpwrsl");

  pwc_ad = getval("pwc_ad");
  d_ad = getval("d_ad");

  ni = getval("ni"); 

  BigT = getval("BigT");
  BigT1 = getval("BigT1");

  gstab  = getval("gstab");
  gzlvl0 = getval("gzlvl0");
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
  gzlvl11 = getval("gzlvl11");
  gzlvl12 = getval("gzlvl12");

  gt1 = getval("gt1");
  gt2 = getval("gt2");
  gt3 = getval("gt3");
  gt4 = getval("gt4");
  gt5 = getval("gt5");
  gt6 = getval("gt6");
  gt7 = getval("gt7");
  gt8 = getval("gt8");
  gt9 = getval("gt9");
  gt10 = getval("gt10");
  gt11 = getval("gt11");
  gt12 = getval("gt12");

  getstr("fscuba",fscuba); 
  getstr("fsat",fsat); 
  getstr("f1180",f1180); 
  getstr("f2180",f2180); 
  getstr("f3180",f3180); 

  getstr("N_flg",N_flg);

  getstr("diag_supp",diag_supp);
  getstr("sh_ad",sh_ad);
  getstr("shape",shape);
  if(d_ad > 62) {
   printf("chirp power is too high \n");
   psg_abort(1);
  }
  if(pwc_ad > 1.2e-3) {
    printf("adiabatic pulse is too long; set < 0.5 ms\n");
    psg_abort(1);
  } 

   setautocal();                        /* activate auto-calibration flags */ 
        
      if (autocal[0] != 'n') 
        /* if autocal = 'y'(yes), 'q'(quiet), r(read), or 's'(semi) */
      {
        strcpy(shape,"H2Osel");
        strcpy(sh_ad,"C13adiab");
        if(FIRST_FID)                                            /* call Pbox */
        {
          ppm = getval("dfrq"); ofs = 0.0;   pws = 0.0005; /*0.5 ms pulse */
          bw = 200.0*ppm;       nst = 1000;          /* nst - number of steps */
          C13adiab = pbox_makeA("C13adiab", "sech", bw, pws, ofs, compC*pwC, pwClvl, nst);
          H2Osel = pbox_Rsh("H2Osel", "sinc90", pw_sl, 0.0, compH*pw, tpwr);
          ofs_check(H1ofs, C13ofs, N15ofs, H2ofs);
        }
        pw_sl = H2Osel.pw; tpwrsl = H2Osel.pwr-1.0;  /* 1dB correction applied */
        d_ad = C13adiab.pwr; pwc_ad = C13adiab.pw;
        pwx2=pwN; dhpwr2=pwNlvl;
      }

/* check validity of parameter range */

    if((dm[A] == 'y' || dm[B] == 'y' || dm[C] == 'y'))
	{
	printf("incorrect Dec1 decoupler flags!  ");
	psg_abort(1);
    } 

    if((dm2[A] == 'y' || dm2[B] == 'y' || dm2[C] == 'y'))
	{
	printf("incorrect Dec2 decoupler flags!  ");
	psg_abort(1);
    } 

    if( tsatpwr > 8 )
    {
	printf("tsatpwr too large !!!  ");
	psg_abort(1);
    }

    if( dpwr > -16 )
    {
	printf("No C decoupling used in this expt. ");
	psg_abort(1);
    }

    if( dpwr2 > -16 )
    {
	printf("No N decoupling used in this expt. ");
	psg_abort(1);
    }

    if(gzlvl0 > 500) {
        printf("gzlvl0_max is 500\n");
        psg_abort(1);
    } 

    if( gt1 > 3e-3 || gt2 > 3e-3 || gt3 > 3e-3 || gt4 > 3e-3 
        || gt5 > 3e-3 || gt6 > 3e-3 || gt7 > 3e-3 
        || gt8 > 3e-3 || gt9 > 3e-3 || gt10 > 3e-3 || gt11 > 3e-3  
        || gt12 > 3e-3)
    {
        printf("gradients are on for too long !!! ");
        psg_abort(1);
    } 


    if(ix==1) {
    if(f1180[A] != 'n' || f2180[A] !='y' || f3180[A] != 'y') {
        printf("f1180 should be n, f2180 y, and f3180 should be y\n");
    }
   }


/* LOAD VARIABLES */

  settable(t1, 4,  phi1);
  settable(t2, 1,  phi2);
  settable(t3, 1,  phi3);
  settable(t4, 1,  phi4);
  settable(t5, 1,  phi5);
  settable(t6, 1,  phi6);
  settable(t7, 1,  phi7);
  settable(t8, 1,  phi8);
  settable(t9, 2,  phi9);
  settable(t10, 4,  rec);

/* INITIALIZE VARIABLES */

  tauxh = 1/(4*jxh);

/* Phase incrementation for hypercomplex data */

  if( phase == 2 ) {
        tsadd(t2, 1, 4);
        tsadd(t3, 1, 4);
   }

   if ( phase2 == 2 ) {    /* Hypercomplex in t2 */
        tsadd(t1, 1, 4);
   }

   if ( phase3 == 2 )     /* Hypercomplex in t3 */
   {
        tsadd(t6, 2, 4);
        tsadd(t7, 2, 4);
        tsadd(t8, 2, 4);
        tsadd(t10, 2, 4);
        icosel = -1;
   }
   else
       icosel = 1;

/* calculate modifications to phases based on current t2/t3 values
   to achieve States-TPPI acquisition */

   if(ix==1)
     d4_init = d4;
     t3_counter = (int) ( (d4-d4_init)*sw3 + 0.5);
     if(t3_counter %2) {
       tsadd(t4,2,4);
       tsadd(t5,2,4);
       tsadd(t10,2,4);
     }

   if(ix==1)
     d3_init = d3;
     t2_counter = (int) ( (d3-d3_init)*sw2 + 0.5);
     if(t2_counter %2) {
       tsadd(t1,2,4);
       tsadd(t10,2,4); 
     }

   if(ix==1)
     d2_init = d2;
     t1_counter = (int) ( (d2-d2_init)*sw1 + 0.5);
     if(t1_counter %2) {
       tsadd(t2,2,4);
       tsadd(t3,2,4);
       tsadd(t10,2,4); 
     }

/* set up so that get (-90,180) phase corrects in F1 if f1180 flag is y */

     tau1 = d2;
     if(f1180[A] == 'y') {
            tau1 += 1/(2.0*sw1);
     }

     tau1 = tau1/2.0;
     if(tau1 < 0.2e-6) tau1 = 0.2e-6;
     

/* set up so that get (-90,180) phase corrects in F2 if f2180 flag is y */

   tau2 = d3;
   if(f2180[A] == 'y')  
      tau2 += ( 1.0/(2.0*sw2) - (4.0/PI)*pwx2
                - 2.0*(2.0*GRADIENT_DELAY + 50.0e-6 + 5.0e-6) );
   if(f2180[A] == 'n')  
      tau2 = ( tau2 - (4.0/PI)*pwx2
                - 2.0*(2.0*GRADIENT_DELAY + 50.0e-6 + 5.0e-6) );

   tau2 = tau2/2.0;

   if(ix==1)
     if((tau2 + 5.0e-6 - 0.5*(WFG_START_DELAY + 4.0*pw + WFG_STOP_DELAY)
         < 5.0e-6) && N_flg[A] == 'n')
      printf("tau2 is negative; set f1180 to y\n");

/* set up so that get (-90,180) phase corrects in F3 if f3180 flag is y */

   tau3 = d4;
   if(f3180[A] == 'y')  tau3 += ( 1.0/(2.0*sw3) );

   tau3 = tau3/2.0;
   if( tau3 < 0.2e-6) tau3 = 2.0e-7; 

/* Now include Bax/Logan trick */

   if(ni != 1) {
     if(diag_supp[A] == 'n')
        zeta = (tauxh - gt5 - 102.0e-6 + 2.0*pwx2 - 2.0e-6);
     else 
        zeta = (1.0/(8.0*93.39) - gt5 - 102.0e-6 + 2.0*pwx2 - 2.0e-6);
     zeta = zeta / ( (double) (ni-1) );

     if(zeta < 0.0) {
        printf("problem with zeta\n");
        psg_abort(1);
     }

   }

   else
      zeta = 0.0;

   if( ix == 1) d2_init = d2;
   t1_counter = (int) ( (d2 - d2_init)*sw1 + 0.5 );

   zeta1 = zeta*( (double)t1_counter );
   tau1 = tau1 - zeta1; 



/* BEGIN ACTUAL PULSE SEQUENCE */

status(A);
   obspower(tsatpwr);            /* Set power for presaturation  */
   decpower(d_ad);               /* Set decoupler1 power to d_ad */
   dec2power(dhpwr2);            /* Set decoupler2 power to dhpwr2 */

/* Presaturation Period */


 if(fsat[0] == 'y')
{
  txphase(zero);
  rgpulse(d1,zero,2.0e-6,2.0e-6);  /* presat */
  obspower(tpwr);                /* Set power for hard pulses  */

    if (fscuba[0] == 'y')            /* Scuba pulse sequence */
    {  
      hsdelay(hscuba);

      rgpulse(pw,zero,1.0e-6,0.0);	/* 90x180y90x */
      rgpulse(2*pw,one,1.0e-6,0.0);
      rgpulse(pw,zero,1.0e-6,0.0);
 
      txphase(zero);
      delay(hscuba);        
    }
}

else  {
 obspower(tpwr);                /* Set power for hard pulses  */
 delay(d1);
}

status(B);

   obsoffset(tof);

   rcvroff();
   delay(20.0e-6);

  /* eliminate all magnetization originating from 15N */

  dec2rgpulse(pwx2,zero,0.0,0.0);

  delay(2.0e-6);
  zgradpulse(gzlvl1,gt1);
  delay(gstab);

  rgpulse(pw,zero,0.0,0.0);

  delay(2.0e-6);
  zgradpulse(gzlvl2,gt2);
  delay(2.0e-6);

  delay(tauxh - gt2 - 4.0e-6);               /* delay=1/4J(XH)   */

  sim3pulse(2*pw,0.0e-6,2*pwx2,zero,zero,zero,0.0,0.0);

  delay(tauxh - gt2 - 202.0e-6);             /* delay=1/4J(XH)   */

  txphase(one); dec2phase(t1);

  delay(2.0e-6);
  zgradpulse(gzlvl2,gt2);
  delay(gstab);

  rgpulse(pw,one,0.0,0.0);       

  /* shaped pulse */
  obspower(tpwrsl);
  shaped_pulse(shape,pw_sl,two,2.0e-6,0.0);
  delay(2.0e-6);
  obspower(tpwr);
  /* shaped pulse */

  delay(2.0e-6);
  zgradpulse(gzlvl3,gt3);
  delay(gstab);

  txphase(zero);

  dec2rgpulse(pwx2,t1,0.0,0.0);
  dec2phase(zero);

  if(N_flg[A] == 'n') 
  {
    if(tau2 + 5.0e-6 - 0.5*(WFG2_START_DELAY
                 + pwc_ad + WFG2_STOP_DELAY) < 0.2e-6)
     {
      rgradient('z',gzlvl0); /* use rgradient since shaping takes more time */
      delay(tau2 + 5.0e-6 - 0.5*(WFG_START_DELAY + 4.0*pw + WFG_STOP_DELAY));
      rgradient('z',0.0);
      delay(50.0e-6);

      shaped_pulse("composite",4.0*pw,zero,0.0,0.0);  /* 90x-180y-90x  */

      rgradient('z',-1.0*gzlvl0);
      delay(tau2 + 5.0e-6 - 0.5*(WFG_START_DELAY + 4.0*pw + WFG_STOP_DELAY));
      rgradient('z',0.0);
      delay(50.0e-6);

     }

     else

     {
      rgradient('z',gzlvl0); /* use rgradient since shaping takes more time */
      delay(tau2 + 5.0e-6 - 0.5*(WFG2_START_DELAY
              + pwc_ad + WFG2_STOP_DELAY));
      rgradient('z',0.0);
      delay(50.0e-6);

      simshaped_pulse("composite",sh_ad,4.0*pw,pwc_ad,zero,zero,0.0,0.0);

      rgradient('z',-1.0*gzlvl0); /* use rgradient since shaping takes more time */
      delay(tau2 + 5.0e-6 - 0.5*(WFG2_START_DELAY
              + pwc_ad + WFG2_STOP_DELAY));
      rgradient('z',0.0);
      delay(50.0e-6);

     }
  }   /* N_flg[A] == y */

  else
    sim3pulse(2.0*pw,0.0,2.0*pwx2,zero,zero,zero,4.0e-6,4.0e-6);

  dec2rgpulse(pwx2,zero,0.0,0.0);

  delay(2.0e-6);
  zgradpulse(gzlvl4,gt4);
  delay(gstab);

  if(diag_supp[A] == 'n') {

  /* shaped pulse */
  obspower(tpwrsl);
  shaped_pulse(shape,pw_sl,t3,2.0e-6,0.0);
  delay(2.0e-6);
  obspower(tpwr);
  /* shaped pulse */

  rgpulse(pw,t2,2.0e-6,0.0);
  txphase(t9);

  delay(tau1 + tauxh + zeta1 - gt5 - 102.0e-6);

  delay(2.0e-6);
  zgradpulse(gzlvl5,gt5);
  delay(100.0e-6);

  dec2rgpulse(2.0*pwx2,zero,0.0,0.0);

  delay(tau1);

  rgpulse(2.0*pw,t9,0.0,0.0); txphase(one);

  delay(tauxh - zeta1 - gt5 - 102.0e-6 + 2.0*pwx2);

  delay(2.0e-6);
  zgradpulse(gzlvl5,gt5);
  delay(100.0e-6);

  rgpulse(pw,one,0.0,0.0);

  /* shaped pulse */
  obspower(tpwrsl);
  shaped_pulse(shape,pw_sl,one,2.0e-6,0.0);
  delay(2.0e-6);
  obspower(tpwr);
  /* shaped pulse */

  delay(MIX - gt6 - gstab -2.0e-6); 

    delay(2.0e-6);
    zgradpulse(gzlvl6,gt6);
    delay(gt6);
    delay(gstab);

  }

  else {

  initval(1.0,v4);
  obsstepsize(45.0);
  xmtrphase(v4);

  /* shaped pulse */
  obspower(tpwrsl);
  shaped_pulse(shape,pw_sl,t3,2.0e-6,0.0);
  delay(2.0e-6);
  obspower(tpwr);
  /* shaped pulse */
/*
  xmtrphase(zero);
  delay(2.0e-6);

  initval(1.0,v4);
  obsstepsize(45.0);
  xmtrphase(v4);
*/
  
  rgpulse(pw,t2,2.0e-6,0.0);

  xmtrphase(zero);
  txphase(t9);

  delay(tau1 + 1.0/(8.0*93.39) + zeta1 - gt5 - 102.0e-6 - SAPS_DELAY);

  delay(2.0e-6);
  zgradpulse(gzlvl5,gt5);
  delay(100.0e-6);

  dec2rgpulse(2.0*pwx2,zero,0.0,0.0);

  delay(tau1);

  rgpulse(2.0*pw,t9,0.0,0.0); txphase(one);

  delay(1.0/(8.0*93.39) - zeta1 - gt5 - 102.0e-6 + 2.0*pwx2);

  delay(2.0e-6);
  zgradpulse(gzlvl5,gt5);
  delay(100.0e-6);

  rgpulse(pw,one,0.0,0.0);

  /* shaped pulse */
  obspower(tpwrsl);
  shaped_pulse(shape,pw_sl,one,2.0e-6,0.0);
  delay(2.0e-6);
  obspower(tpwr);
  /* shaped pulse */

    delay(2.0e-6);
    zgradpulse(gzlvl6,gt6/2.0);
    delay(gstab);

    delay(MIX - 1.5*gt6 - 2.0*(gstab+2.0e-6) - 2.0*pwx2); 

    dec2rgpulse(2.0*pwx2,zero,0.0,0.0); 

    delay(2.0e-6);
    zgradpulse(gzlvl6,gt6);
    delay(gstab);

  }

   rgpulse(pw,two,2.0e-6,0.0);

  /* shaped pulse */
  obspower(tpwrsl);
  shaped_pulse(shape,pw_sl,zero,2.0e-6,0.0);
  delay(2.0e-6); txphase(zero);
  obspower(tpwr);
  /* shaped pulse */

  delay(2.0e-6);
  zgradpulse(gzlvl7,gt7);
  delay(2.0e-6);

  delay(tauxh  
      - POWER_DELAY - 2.0e-6 - WFG_START_DELAY
      - pw_sl - WFG_STOP_DELAY - 2.0e-6 - POWER_DELAY 
      - gt7 - 4.0e-6);               /* delay=1/4J(XH)   */

  sim3pulse(2*pw,0.0e-6,2*pwx2,zero,zero,zero,0.0,0.0);

  delay(tauxh 
      - gt7 - gstab -2.0e-6
      - POWER_DELAY - 2.0e-6 - WFG_START_DELAY
      - pw_sl - WFG_STOP_DELAY - 2.0e-6 - POWER_DELAY); 

  delay(2.0e-6);
  zgradpulse(gzlvl7,gt7);
  delay(gt7);
  delay(gstab);

  /* shaped pulse */
  obspower(tpwrsl);
  shaped_pulse(shape,pw_sl,one,2.0e-6,0.0);
  obspower(tpwr);
  delay(2.0e-6); txphase(zero);
  /* shaped pulse */

  rgpulse(pw,one,0.0,0.0);

  delay(2.0e-6);
  zgradpulse(gzlvl8,gt8);
  delay(gstab);

  txphase(t6);

  if(phase3 == 1)
    dec2rgpulse(pwx2,t4,4.0e-6,0.0);
  if(phase3 == 2)
    dec2rgpulse(pwx2,t5,4.0e-6,0.0);

  decphase(zero); 

  if(tau3 - 0.5*(WFG_START_DELAY
                 + pwc_ad + WFG_STOP_DELAY)
          < 0.2e-6) {

  delay(tau3);
  delay(tau3);

  }

  else {

  delay(tau3 - 0.5*(WFG_START_DELAY
               + pwc_ad + WFG_STOP_DELAY));

  decshaped_pulse(sh_ad,pwc_ad,zero,0.0,0.0); 

  delay(tau3 - 0.5*(WFG_START_DELAY
               + pwc_ad + WFG_STOP_DELAY));

  }

  delay(2.0e-6);
  zgradpulse(-1.0*gzlvl11,gt11);
  delay(100.0e-6);

  delay(BigT - 4.0/PI*pwx2 + pw - 2.0*GRADIENT_DELAY - gt11
        - 102.0e-6);

  dec2rgpulse(2.0*pwx2,zero,0.0,0.0);

  delay(2.0e-6);
  zgradpulse(gzlvl11,gt11);
  delay(100.0e-6);

  delay(BigT - 2.0*GRADIENT_DELAY - gt11
        - 102.0e-6);

  rgpulse(pw,t6,0.0,0.0);

  /* shaped pulse */
  obspower(tpwrsl);
  shaped_pulse(shape,pw_sl,t7,2.0e-6,0.0);
  delay(2.0e-6); txphase(zero);
  obspower(tpwr);
  /* shaped pulse */

  delay(2.0e-6);
  zgradpulse(gzlvl9,gt9);
  delay(100.0e-6);

  delay(tauxh 
      - POWER_DELAY - 2.0e-6 - WFG_START_DELAY
      - pw_sl - WFG_STOP_DELAY - 2.0e-6 - POWER_DELAY 
      - gt9 - 102.0e-6);

  sim3pulse(2.0*pw,0.0,2.0*pwx2,zero,zero,zero,0.0,0.0);
  dec2phase(t8);

  delay(2.0e-6);
  zgradpulse(gzlvl9,gt9);
  delay(100.0e-6);

  delay(tauxh 
      - gt9 - 102.0e-6
      - POWER_DELAY - 2.0e-6 - WFG_START_DELAY
      - pw_sl - WFG_STOP_DELAY - 2.0e-6 - POWER_DELAY); 

  /* shaped pulse */
  obspower(tpwrsl);
  shaped_pulse(shape,pw_sl,zero,2.0e-6,0.0);
  obspower(tpwr);
  delay(2.0e-6); txphase(zero);
  /* shaped pulse */

  sim3pulse(pw,0.0,pwx2,zero,zero,t8,0.0,0.0);

  dec2phase(zero);

  delay(2.0e-6);
  zgradpulse(gzlvl10,gt10);
  delay(2.0e-6);

  delay(tauxh - gt10 - 4.0e-6);

  sim3pulse(2*pw,0.0e-6,2*pwx2,zero,zero,zero,2.0e-6,2.0e-6);  

  delay(2.0e-6);
  zgradpulse(gzlvl10,gt10);
  delay(100.0e-6);

  delay(tauxh
        - gt10 - 102.0e-6
        + 2.0/PI*pw - pwx2 + 0.5*(pwx2-pw));

  dec2rgpulse(pwx2,zero,0.0,0.0);

  dec2power(dpwr2);  /* Very Important */
  decpower(dpwr);

  delay(BigT1 - 2.0*POWER_DELAY);

  rgpulse(2.0*pw,zero,0.0,0.0); 

  delay(2.0e-6);
  zgradpulse(-1.0*icosel*gzlvl12,gt12);
  delay(50.0e-6);

  delay(BigT1 - 2.0*GRADIENT_DELAY - 52.0e-6 - gt12);

/* acquire data */

status(C);
     setreceiver(t10);
}
