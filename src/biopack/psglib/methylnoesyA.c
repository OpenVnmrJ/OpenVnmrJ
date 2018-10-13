/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*  methylnoesyA.c - auto-calibrated version of the original sequence
                           methyl_3d_noesy_500.c
   
    This pulse sequence will allow one to perform the following
    experiment:

    3D C(methyl-CT) -  noesy - C(methyl-CT) - H (methyl)
 
    Uses three channels:
         1)  1H             - 4.73 ppm 
         2) 13C             - 19.6 ppm 
         3) 15N             - 120.0 ppm  

    Set dm = 'nny',  [13C decoupling during acquisition].
    Set dm2 = 'nnn'

    F1: 13C methyl 
    F2: 13C methyl 
    F3: 1H  methyl 

    carbon carrier is set to 19.6 ppm

    If CT_flg[A] == 'n' then the t1 period is CT and t2 is not
    If CT_flg[A] == 'y' then the t1 period is CT and t2 is CT

    Modified by L.E.K on Feb. 18, 98 to include CT option 


    Based on Val_4d_noesy_600.c

    Modified by L.E.K on March 26, 1998 to include simultaneous 15N acquistion
    during T2. Set n_shift == 'y' and be sure to include 15N decoupling during
    acquisition. Reduce the decoupling power since both 15N and 13C decoupling
    is present. 

    If CT_flg == 'n' then  make sure t1,max is < about 9.5ms; Note that c-c and
    N-ca evolution will proceed (if n_shift == 'y')

    At 600 MHz use a 1.2 ms iburp2 centered at 54.5 ppm to invert the Ca. Use
    a (600/y)*1.2 ms iburp2 at a field strength of y MHz.

    Zwahlen et.al.,JACS, 120, 7617(1998).

    Modified for autocal (E.Kupce), modified for BioPack by GG, PaloAlto, June 2004
    
*/

#include <standard.h>
#include "Pbox_bio.h"

static shape Preb_5p, Pib_1p5, Pib_35p, Pib_35pi, Psed_156p, Pdec_156p;
static double   H1ofs=4.7, C13ofs=19.6, N15ofs=120.0, H2ofs=3.2;

static int  phi1[4] = {0,0,2,2},
            phi2[8] = {0,0,0,0,2,2,2,2},
            phi7[2] = {0,2},
            phi8[2] = {0,2},
            rec[8]  = {0,2,2,0,2,0,0,2};
           
static double d2_init=0.0, d3_init=0.0;
            
pulsesequence()
{
/* DECLARE VARIABLES */

 char       autocal[MAXSTR],  /* auto-calibration flag */
            fsat[MAXSTR],
            fscuba[MAXSTR],
            f1180[MAXSTR],    /* Flag to start t1 @ halfdwell         */
            f2180[MAXSTR],    /* Flag to start t2 @ halfdwell         */
            shib[MAXSTR],     /* iburp for inversion during first inept     */
            Hshp[MAXSTR],     /* proton inversion during chirp        */
            ddseq[MAXSTR],
            shreb[MAXSTR],    /* reburb hard during t2                */
            co_shp[MAXSTR],   /* shape of co 180 at 176 ppm */
            CT_flg[MAXSTR],
            codecseq[MAXSTR],
            c180_flg[MAXSTR],

            n_shift[MAXSTR],
            shibca[MAXSTR],
            shibcai[MAXSTR];

 int         phase, phase2, t2_counter, ni2, ni,
             t1_counter;   /* used for states tppi in t2,t1        */ 

 double      tau1,         /*  t1 delay */
             tau2,         /*  t2 delay */
             taua,         /*  ~ 1/4JCH =  1.7 ms; first inept */
             mix,	   /* noesy mixing time */
             TC,           /* Variable CT period during t1 1/2JCC */
             TC2,          /* Variable CT period during t3  1/2JCC */
             pwc,          /* 90 c pulse at dhpwr            */
             tsatpwr,      /* low level 1H trans.power for presat  */
             dhpwr,        /* power level for high power 13C pulses on dec1 */
             sw1,          /* sweep width in f1                    */ 
             sw2,          /* sweep width in f2                    */ 
             pwC,pwClvl,compC,pwN,pwNlvl,ppm,ofs,bw,  /*used by Pbox */
             d_ib,
             pwib,

             pwhshp,

             pwd1,        /* 2H flip back pulses   */

             d_reb,
             pwreb,

             ph_reb,
             ph_reb1,    /* only used if CT_flg=='y' and n_shift=='y'  */

             pwco180,

             dhpwr2,
             pwn,

             d_co180,

             pwcodec,    /* carbon pw90 for seduce decoupling */
             dpwrsed,
             dressed,

             d_ibca,     /* power level for selective 13Ca pulse during CT-t2 */
             pwibca,     /* selective 13Ca pulse width  */

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
             gt12,
             gstab,
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
             gzlvl12;

/*  variables commented out are already defined by the system      */


/* LOAD VARIABLES */

  getstr("autocal",autocal);
  getstr("fsat",fsat);
  getstr("f1180",f1180);
  getstr("f2180",f2180);
  getstr("fscuba",fscuba);
  getstr("ddseq",ddseq);
  getstr("n_shift",n_shift); 
  getstr("Hshp",Hshp);
  getstr("CT_flg",CT_flg);
  getstr("c180_flg",c180_flg);

  compC = getval("compC"); pwN=getval("pwN"); pwNlvl=getval("pwNlvl");
  pwC = getval("pwC"); pwClvl=getval("pwClvl");
  
  pwhshp = getval("pwhshp");
  taua   = getval("taua"); 
  mix   = getval("mix"); 
  TC = getval("TC");
  pwc = getval("pwc");
  tpwr = getval("tpwr");
  tsatpwr = getval("tsatpwr");
  dhpwr = getval("dhpwr");
  dpwr = getval("dpwr");
  phase = (int) ( getval("phase") + 0.5);
  phase2 = (int) ( getval("phase2") + 0.5);
  sw1 = getval("sw1");
  sw2 = getval("sw2");
  ni2 = getval("ni2");
  ni  = getval("ni");

  pwd1 = getval("pwd1");

  ph_reb = getval("ph_reb");
  ph_reb1 = getval("ph_reb1");

  TC2 = getval("TC2");

  dhpwr2 = getval("dhpwr2");
  pwn = getval("pwn");

  setautocal();

  if(autocal[0]=='n')
  {     
    getstr("shreb",shreb);
    getstr("shib",shib);
    getstr("shibca",shibca);
    getstr("shibcai",shibcai);
    getstr("co_shp",co_shp);
    getstr("codecseq",codecseq);

    d_reb = getval("d_reb");
    pwreb = getval("pwreb");
    d_ib = getval("d_ib");
    pwib = getval("pwib");
    d_ibca = getval("d_ibca");
    pwibca = getval("pwibca"); 
    d_co180 = getval("d_co180");
    pwco180 = getval("pwco180");
    pwcodec = getval("pwcodec");
    dpwrsed = getval("dpwrsed");
    dressed = getval("dressed");
  }
  else
  {    
  /*strcpy(Hshp,"hard");  former declarations using TNMR.h syntax
    strcpy(shreb,"Preb_5p");
    strcpy(shib,"Pib_1p5");
    strcpy(shibca,"Pib_35p");
    strcpy(shibcai,"Pib_35pi");    
    strcpy(co_shp,"Psed_156p");
    strcpy(codecseq,"Pdec_156p");*/

    strcpy(Hshp,"hard");
    strcpy(shreb,"Preb_5p");
    strcpy(shib,"Pib_1p5");
    strcpy(shibca,"Pib_35p");
    strcpy(shibcai,"Pib_35pi");    
    strcpy(co_shp,"Psed_156p");
    strcpy(codecseq,"Pdec_156p");
    if (FIRST_FID)
    {
      ppm = getval("dfrq");

/*       These are former declarations (at top) using TNMR.h syntax                     */
/*REB180   "reburp 110p 5p"*/             /* RE-BURP 180 on Cab at 24.6 ppm, 5 ppm away */
/*IB180    "iburp2 24.4p 1.5p"*/          /* I-BURP 180 on  Me at 21.1 ppm, 1.5 ppm away */
/*IBCA     "iburp2 24.4p 35p"*/           /* I-BURP 180 on Cab at 54.6 ppm, 35 ppm away */
/*IBCAI    "iburp2 24.4p 35p"*/           /* I-BURP 180 on Cab at 54.6 ppm, 35 ppm away */
/*CO180    "seduce 30p 156p"*/            /* SEDUCE 180 on C' at 175.6 ppm 156 ppm away */
/*CODEC    "WURST2 20p/4m 156p"*/  /* WURST2 decoupling on C' at 175.6 ppm 156 ppm away */
/*REB180ps "-stepsize 0.5 -attn i"*/                     /* seduce 180 shape parameters */
/*CODECps  "-dres 1.0 -maxincr 20.0 -attn i"*/
    /*co180 = pbox(co_shp, CO180, REB180ps, dfrq, compC*pwc, dhpwr);*/
    /*ibcai = pbox(shibcai, IBCAI, REB180ps, dfrq, compC*pwc, dhpwr);*/      
    /*ibca = pbox(shibca, IBCA, REB180ps, dfrq, compC*pwc, dhpwr);*/
    /*ib180 = pbox(shib, IB180, REB180ps, dfrq, compC*pwc, dhpwr);*/          
    /*reb = pbox(shreb, REB180, REB180ps, dfrq, compC*pwc, dhpwr);*/
    /*COdec = pbox(codecseq, CODEC, CODECps, dfrq, compC*pwc, dhpwr);*/

      bw = 110.0*ppm; ofs = 5.0*ppm;
      Preb_5p = pbox_Rsh("Preb_5p", "reburp", bw , ofs, compC*pwC, pwClvl);
      bw = 24.4*ppm; ofs = 1.5*ppm;
      Pib_1p5 = pbox_Rsh("Pib_1p5", "iburp2", bw , ofs, compC*pwC, pwClvl);
      bw = 24.4*ppm; ofs = 35*ppm;
      Pib_35p = pbox_Rsh("Pib_35p", "iburp2", bw , ofs, compC*pwC, pwClvl);
      bw = 24.4*ppm; ofs = 35*ppm;
      Pib_35pi = pbox_Rsh("Pib_35pi", "iburp2", bw , ofs, compC*pwC, pwClvl);
      bw = 30.0*ppm; ofs = 156*ppm;
      Psed_156p = pbox_Rsh("Psed_156p", "seduce", bw , ofs, compC*pwC, pwClvl);
      bw = 20.0*ppm; ofs = 156*ppm;
      Pdec_156p = pbox_Dsh("Pdec_156p", "WURST2", bw , ofs, compC*pwC, pwClvl);


      ofs_check(H1ofs, C13ofs, N15ofs, H2ofs);
    }
    d_reb = Preb_5p.pwr;        pwreb = Preb_5p.pw;
    d_ib = Pib_1p5.pwr;         pwib = Pib_1p5.pw;
    d_ibca = Pib_35p.pwr;       pwibca = Pib_35p.pw;       
    d_co180 = Psed_156p.pwr;    pwco180 = Psed_156p.pw;  
    dpwrsed = Pdec_156p.pwr;    pwcodec = 1.0/Pdec_156p.dmf;  dressed = Pdec_156p.dres;

    pwc=pwC; dhpwr=pwClvl; pwn=pwN; dhpwr2=pwNlvl; pwhshp=2.0*pw;
    pwd1=1/dmf3; pwhshp=2.0*pw;
  }   
   
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

  gstab  = getval("gstab");
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

/* LOAD PHASE TABLE */

  settable(t1,4,phi1);
  settable(t2,8,phi2);
  settable(t7,2,phi7);
  settable(t8,2,phi8);
  settable(t9,8,rec);

/* CHECK VALIDITY OF PARAMETER RANGES */

   if(TC/2.0 - 0.5*(ni-1)*1/(sw1) - POWER_DELAY - 4.0e-6 
         - WFG3_START_DELAY - pwco180
         - WFG3_STOP_DELAY - pwd1 
         - gt4 - gstab -2.0e-6 - POWER_DELAY
         - 4.0e-6 - WFG_START_DELAY < 0.2e-6)
    {
        printf(" ni is too big\n");
        psg_abort(1);
    }

  if(CT_flg[A] == 'y' && n_shift[A] == 'n') {

   if(TC2/2.0 - 0.5*(ni2-1)*1/(sw2) - POWER_DELAY - 4.0e-6 
         - WFG3_START_DELAY - pwco180
         - WFG3_STOP_DELAY - pwd1 
         - gt10 - gstab -2.0e-6 - POWER_DELAY
         - 4.0e-6 - WFG_START_DELAY < 0.2e-6)
    {
        printf(" ni2 is too big\n");
        psg_abort(1);
    }

   }

  if(CT_flg[A] == 'y' && n_shift[A] == 'y') {

   if(TC2/2.0 - 0.5*(ni2-1)/sw2 - POWER_DELAY 
         - 4.0e-6 - WFG3_START_DELAY - pwco180
         - WFG3_STOP_DELAY - 2.0e-6 - POWER_DELAY - WFG_START_DELAY
         - 4.0e-6 - pwibca - WFG_STOP_DELAY - pwd1 
         - gt10 - gstab -2.0e-6 - POWER_DELAY
         - 4.0e-6 - WFG3_START_DELAY < 0.2e-6)
    {
        printf(" ni2 is too big\n");
        psg_abort(1);
    }

   }

    if((dm[A] == 'y' || dm[B] == 'y' ))
    {
        printf("incorrect dec1 decoupler flags!  ");
        psg_abort(1);
    }

    if((dm2[A] == 'y' || dm2[B] == 'y'))
    {
        printf("incorrect dec2 decoupler flags!  ");
        psg_abort(1);
    }

    if((dm3[A] == 'y' || dm3[B] == 'y' || dm3[C] == 'y'))
    {
        printf("incorrect dec3 decoupler flags!  ");
        psg_abort(1);
    }

    if( tsatpwr > 6 )
    {
        printf("TSATPWR too large !!!  ");
        psg_abort(1);
    }

    if( dpwr > 48 )
    {
        printf("don't fry the probe, DPWR too large!  ");
        psg_abort(1);
    }

    if( d_ib > 54 )
    {
        printf("don't fry the probe, d_ib too large!  ");
        psg_abort(1);
    }

    if( dpwr2 > 49 )
    {
        printf("don't fry the probe, DPWR2 too large!  ");
        psg_abort(1);
    }

    if( dpwr3 > 51 )
    {
        printf("don't fry the probe, DPWR3 too large!  ");
        psg_abort(1);
    }

    if( dhpwr > 63 )
    {
        printf("don't fry the probe, DHPWR too large!  ");
        psg_abort(1);
    }

    if( pw > 200.0e-6 )
    {
        printf("dont fry the probe, pw too high ! ");
        psg_abort(1);
    } 

    if( pwd1 < 100.0e-6 && pwd1 != 0.0)
    {
        printf("dont fry the probe, pwd1 too short and dpwr3 too high! ");
        psg_abort(1);
    } 

    if(d_co180 > 50)
    {
        printf("dont fry the probe, d_co180 is too high\n ");
        psg_abort(1);
    } 

    if(((pwco180 > 250e-6) || (pwco180 < 200e-6)) && (autocal[A] == 'n'))
     {
        printf("pwco180 is misset < 250 us > 200 us\n");
        psg_abort(1);
     }

    if(dpwrsed > 45) 
     {
        printf("dpwrsed is misset < 46\n");
        psg_abort(1);
     }

    if(gt1 > 15e-3 || gt2 > 15e-3 || gt3 > 15e-3 || 
        gt4 > 15e-3 || gt5 > 15e-3 || gt6 > 15e-3 ||
        gt7 > 15e-3 || gt8 > 15e-3 || gt9 > 15e-3 || 
        gt10 > 15e-3 || gt11 > 15e-3 || gt12 > 15e-3) 
    {
        printf("gradients on for too long. Must be < 15e-3 \n");
        psg_abort(1);
    }

/*  Phase incrementation for hypercomplex 2D data */

    if (phase2 == 2) {
      tsadd(t2,1,4);  
    }

    if (phase == 2) 
      tsadd(t1,1,4);

/*  Set up f2180  tau2 = t2               */
   
    tau2 = d3;

    if(CT_flg[A] == 'y') {

    if(f2180[A] == 'y') {
        tau2 += ( 1.0 / (2.0*sw2) );
    }

    }

    if(CT_flg[A] == 'n' && n_shift[A] == 'n') {

         if(f2180[A] == 'y') {
            tau2 += ( 1.0 / (2.0*sw2) - 4.0/PI*pwc - POWER_DELAY
                      - PRG_START_DELAY - 4.0*pw - 4.0e-6 - 2.0*pwn
                      - PRG_STOP_DELAY - POWER_DELAY - 4.0e-6);

            if(tau2 < 0.0 && ix == 1) 
               printf("tau2 start2 negative; decrease sw2\n");
          }

         if(f2180[A] == 'n') {
            tau2 = ( tau2 - 4.0/PI*pwc - POWER_DELAY
                      - PRG_START_DELAY - 4.0*pw - 4.0e-6 - 2.0*pwn
                      - PRG_STOP_DELAY - POWER_DELAY - 4.0e-6);
         }

     } 

    if(CT_flg[A] == 'n' && n_shift[A] == 'y') {

         if(f2180[A] == 'y') {
            tau2 += ( 1.0 / (2.0*sw2) - 4.0/PI*pwn - POWER_DELAY
                      - PRG_START_DELAY - 4.0*pw - 4.0e-6 
                      - PRG_STOP_DELAY - POWER_DELAY - 4.0e-6);

            if(tau2 < 0.0 && ix == 1) 
               printf("tau2 start2 negative; decrease sw2\n");
          }

         if(f2180[A] == 'n') {
            tau2 = ( tau2 - 4.0/PI*pwn - POWER_DELAY
                      - PRG_START_DELAY - 4.0*pw - 4.0e-6
                      - PRG_STOP_DELAY - POWER_DELAY - 4.0e-6);
         }

     } 

    if(tau2 < 0.4e-6) tau2 = 0.4e-6;
    tau2 = tau2/2.0;

/*  Set up f1180  tau1 = t1               */
   
    tau1 = d2;
    if(f1180[A] == 'y') {
        tau1 += ( 1.0 / (2.0*sw1) );
    }
    if(tau1 < 0.4e-6) tau1 = 0.4e-6;
    tau1 = tau1/2.0;

/* Calculate modifications to phases for States-TPPI acquisition          */

   if( ix == 1) d2_init = d2 ;
   t1_counter = (int) ( (d2-d2_init)*sw1 + 0.5 );
   if(t1_counter % 2) {
      tsadd(t1,2,4);     
      tsadd(t9,2,4);     
    }

   if( ix == 1) d3_init = d3 ;
   t2_counter = (int) ( (d3-d3_init)*sw2 + 0.5 );
   if(t2_counter % 2) {
      tsadd(t2,2,4);     
      tsadd(t9,2,4);    
    }

/* BEGIN ACTUAL PULSE SEQUENCE */

status(A);
   obspower(tsatpwr);     /* Set transmitter power for 1H presaturation */
   decpower(dhpwr);       /* Set Dec1 power for hard 13C pulses         */
   dec2power(dhpwr2);    /* Set Dec2 power for hard 15N pulses         */
   dec3power(dpwr3);     /* Set Dec3 power for 2H pulses        */

/* Presaturation Period */


   if (fsat[0] == 'y')
   {
        delay(2.0e-5);
        rgpulse(d1,zero,2.0e-6,2.0e-6); /* presat */
        obspower(tpwr);    /* Set transmitter power for hard 1H pulses */
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

   obspower(tpwr);          /* Set transmitter power for hard 1H pulses */
   txphase(zero);
   dec2phase(zero);
   decphase(zero);
   delay(1.0e-5);

/* Begin Pulses */

status(B);

   rcvroff();
   lk_hold();
   delay(20.0e-6);

/* first ensure that magnetization does infact start on H and not C */

   decrgpulse(pwc,zero,2.0e-6,2.0e-6);

   delay(2.0e-6);
   zgradpulse(gzlvl1,gt1);
   delay(gstab);

   decpower(d_ib);			/* set power for chirp during inept */
   delay(4e-6);

/* this is the real start */

   rgpulse(pw,zero,0.0,0.0);                    /* 90 deg 1H pulse */

   delay(2.0e-6);
   zgradpulse(gzlvl2,gt2);
   delay(2.0e-6);

   delay(taua - gt2 - 4.0e-6 - WFG2_START_DELAY);   /* taua <= 1/4JCH */                          

   simshaped_pulse(Hshp,shib,pwhshp,pwib,zero,zero,0.0,0.0);
   decphase(zero);

   txphase(one); decphase(t1);
   decpower(dhpwr);

   delay(2.0e-6);
   zgradpulse(gzlvl2,gt2);
   delay(2.0e-6);

   delay(taua - gt2 - 4.0e-6 - WFG2_STOP_DELAY - POWER_DELAY); 

   rgpulse(pw,one,0.0,0.0);
   txphase(zero);

   delay(2.0e-6);
   zgradpulse(gzlvl3,gt3);
   delay(gstab);

   decrgpulse(pwc,t1,0.0,0.0);
   decphase(zero);

   delay(tau1);

   decpower(d_co180);
   sim3shaped_pulse(Hshp,co_shp,"hard",pwhshp,pwco180,2.0*pwn,zero,zero,zero,4.0e-6,0.0);

   delay(TC/2.0 - tau1 - POWER_DELAY - 4.0e-6 - WFG3_START_DELAY - pwco180
         - WFG3_STOP_DELAY - pwd1 
         - gt4 - gstab -2.0e-6 - POWER_DELAY
         - 4.0e-6 - WFG_START_DELAY);

   dec3rgpulse(pwd1,zero,0.0,0.0);

   delay(tau1);

   delay(2.0e-6);
   zgradpulse(gzlvl4,gt4);
   delay(gstab);

   initval(1.0,v3);
   decstepsize(ph_reb);
   dcplrphase(v3);

   decpower(d_reb);

   decshaped_pulse(shreb,pwreb,zero,4.0e-6,0.0);
   dcplrphase(zero);

   decphase(zero); decpower(dhpwr);
  
   delay(2.0e-6);
   zgradpulse(gzlvl4,gt4);
   delay(gstab);

   delay(TC/2.0 - tau1 - WFG_STOP_DELAY - POWER_DELAY
         - gt4 - gstab -2.0e-6);

   decrgpulse(pwc,zero,0.0,0.0);

   dec3rgpulse(pwd1,two,4.0e-6,0.0);


   delay(2.0e-6);
   zgradpulse(gzlvl5,gt5);
   delay(gstab);

   rgpulse(pw,zero,0.0,0.0);

   delay(2.0e-6);
   zgradpulse(gzlvl6,gt6);
   delay(gstab);

   decpower(d_ib);
   delay(taua - gt6 - 4.0e-6 - WFG2_START_DELAY - POWER_DELAY);

   simshaped_pulse(Hshp,shib,pwhshp,pwib,zero,zero,0.0,0.0);
   decphase(zero);

   delay(2.0e-6);
   zgradpulse(gzlvl6,gt6);
   delay(gstab);


   decpower(dhpwr);
   txphase(one);
   delay(taua - gt6 - gstab -2.0e-6 - POWER_DELAY - WFG2_STOP_DELAY);

   rgpulse(pw,one,0.0,0.0);
   txphase(zero);

   delay(mix - gt7 - 352.0e-6);

   decrgpulse(pwc,zero,0.0,0.0);

   delay(2.0e-6);
   zgradpulse(gzlvl7,gt7);
   delay(gstab);

   decpower(d_ib); /* set power level for iburp */

   rgpulse(pw,zero,0.0,0.0);                    /* 90 deg 1H pulse */

   delay(2.0e-6);
   zgradpulse(gzlvl8,gt8);
   delay(2.0e-6);

   if(n_shift[A] == 'n') {

   delay(taua - gt8 - 4.0e-6 - WFG2_START_DELAY);   /* taua <= 1/4JCH */                          
   simshaped_pulse(Hshp,shib,pwhshp,pwib,zero,zero,0.0,0.0);
   decphase(zero);

   }

   else {
 delay(taua - gt8 - 4.0e-6 - WFG3_START_DELAY);                 
 sim3shaped_pulse(Hshp,shib,"hard",pwhshp,pwib,2.0*pwn,zero,zero,zero,0.0,0.0);
    }


   txphase(one); decphase(t2);
   decpower(dhpwr);

   delay(2.0e-6);
   zgradpulse(gzlvl8,gt8);
   delay(2.0e-6);

   if(n_shift[A] == 'n') 
      delay(taua - gt8 - 4.0e-6 - WFG2_STOP_DELAY - POWER_DELAY); 
   else
      delay(taua - gt8 - 4.0e-6 - WFG3_STOP_DELAY - POWER_DELAY); 

   rgpulse(pw,one,0.0,0.0);
   txphase(zero);

   delay(2.0e-6);
   zgradpulse(gzlvl9,gt9);
   delay(gstab);

  if(CT_flg[A] == 'y' && n_shift[A] == 'n') {

   decrgpulse(pwc,t2,0.0,0.0);
   decphase(zero);

   delay(tau2);

   decpower(d_co180);
   sim3shaped_pulse(Hshp,co_shp,"hard",pwhshp,pwco180,2.0*pwn,zero,zero,zero,4.0e-6,0.0);

   delay(TC2/2.0 - tau2 - POWER_DELAY - 4.0e-6 - WFG3_START_DELAY - pwco180
         - WFG3_STOP_DELAY - pwd1 
         - gt10 - gstab -2.0e-6 - POWER_DELAY
         - 4.0e-6 - WFG_START_DELAY);

   dec3rgpulse(pwd1,zero,0.0,0.0);

   delay(tau2);

   delay(2.0e-6);
   zgradpulse(gzlvl10,gt10);
   delay(gstab);

   initval(1.0,v4);
   decstepsize(ph_reb);
   dcplrphase(v4);

   decpower(d_reb);

   decshaped_pulse(shreb,pwreb,zero,4.0e-6,0.0);
   dcplrphase(zero);

   decphase(zero); decpower(dhpwr);
   
   delay(2.0e-6);
   zgradpulse(gzlvl10,gt10);
   delay(gstab);

   delay(TC2/2.0 - tau2 - WFG_STOP_DELAY - POWER_DELAY 
         - gt10 - gstab -2.0e-6); 

   decrgpulse(pwc,zero,0.0,0.0);

   dec3rgpulse(pwd1,two,4.0e-6,0.0);

   }

  if(CT_flg[A] == 'y' && n_shift[A] == 'y') {

   dec2phase(t2); delay(2.0e-6);

   dec2rgpulse((pwn-pwc)/2.0,t2,0.0,0.0);
   sim3pulse(0.0e-6,pwc,pwc,zero,t2,t2,0.0,0.0);
   dec2rgpulse((pwn-pwc)/2.0,t2,0.0,0.0);

   decphase(zero);

   delay(tau2);

   decphase(zero);
   decpower(d_co180);
   sim3shaped_pulse(Hshp,co_shp,"hard",pwhshp,pwco180,0.0e-6,zero,zero,zero,4.0e-6,2.0e-6);

   decpower(d_ibca);
   decshaped_pulse(shibca,pwibca,zero,4.0e-6,0.0);
   decphase(zero);

   delay(TC2/2.0 - tau2 - POWER_DELAY - 4.0e-6 - WFG3_START_DELAY - pwco180
         - WFG3_STOP_DELAY - 2.0e-6 - POWER_DELAY - WFG_START_DELAY
         - 4.0e-6 - pwibca - WFG_STOP_DELAY - pwd1 
         - gt10 - gstab -2.0e-6 - POWER_DELAY
         - 4.0e-6 - WFG3_START_DELAY);

   dec3rgpulse(pwd1,zero,0.0,0.0);

   delay(tau2);

   delay(2.0e-6);
   zgradpulse(gzlvl10,gt10);
   delay(gstab);

   initval(1.0,v4);
   decstepsize(ph_reb1);
   dcplrphase(v4);

   decpower(d_reb);

   sim3shaped_pulse("hard",shreb,"hard",0.0e-6,pwreb,2.0*pwn,zero,zero,zero,4.0e-6,0.0);
   dcplrphase(zero);

   decphase(t7); decpower(d_ibca);
   decshaped_pulse(shibcai,pwibca,t7,4.0e-6,0.0);
   decpower(dhpwr); decphase(zero);
   
   delay(2.0e-6);
   zgradpulse(gzlvl10,gt10);
   delay(gstab);

   delay(TC2/2.0 - tau2 - WFG3_STOP_DELAY - POWER_DELAY 
         - 4.0e-6 - WFG_START_DELAY - pwibca - WFG_STOP_DELAY 
         - POWER_DELAY
         - gt10 - gstab -2.0e-6); 

   dec2rgpulse((pwn-pwc)/2.0,zero,0.0,0.0);
   sim3pulse(0.0e-6,pwc,pwc,zero,zero,zero,0.0,0.0);
   dec2rgpulse((pwn-pwc)/2.0,zero,0.0,0.0);

   dec3rgpulse(pwd1,two,4.0e-6,0.0);

   }

   if(CT_flg[A] == 'n' && n_shift[A] == 'n') {

      txphase(one);
      decrgpulse(pwc,t2,0.0,0.0);

      if(c180_flg[A] == 'n')
    {

      decphase(zero);

      /* seduce on */
      decpower(dpwrsed);
      decprgon(codecseq,pwcodec,dressed);
      decon();
      /* seduce on */

      delay(tau2);

      rgpulse(pw,one,0.0,0.0);
      rgpulse(2.0*pw,zero,2.0e-6,0.0);
      rgpulse(pw,one,2.0e-6,0.0);

      dec2rgpulse(2.0*pwn,zero,0.0,0.0);

      delay(tau2); 

      /* seduce off */
      decoff();
      decprgoff();
      decpower(dhpwr);
      /* seduce off */

    }

      else
         decrgpulse(2.0*pwc,zero,4.0e-6,0.0);

      decrgpulse(pwc,zero,4.0e-6,0.0);

   }

   if(CT_flg[A] == 'n' && n_shift[A] == 'y') {

      txphase(one); dec2phase(t2);

      dec2rgpulse((PI-2.0)/PI*(pwn-pwc),t2,2.0e-6,0.0);
      sim3pulse(0.0e-6,pwc,pwc,zero,t2,t2,0.0,0.0);
      dec2rgpulse((2.0/PI)*(pwn-pwc),t2,0.0,0.0);

      if(c180_flg[A] == 'n')
    {

      decphase(zero);

      /* seduce on */
      decpower(dpwrsed);
      decprgon(codecseq,pwcodec,dressed);
      decon();
      /* seduce on */

      delay(tau2);

      rgpulse(pw,one,0.0,0.0);
      rgpulse(2.0*pw,zero,2.0e-6,0.0);
      rgpulse(pw,one,2.0e-6,0.0);

      delay(tau2); 

      /* seduce off */
      decoff();
      decprgoff();      /* note that ca-n evolves ; keep t2,max <= 9.5ms */
      decpower(dhpwr);
      /* seduce off */

    }

      else
         sim3pulse(0.0,2.0*pwc,2.0*pwn,zero,zero,zero,4.0e-6,0.0);

      dec2rgpulse((2.0/PI)*(pwn-pwc),zero,4.0e-6,0.0);
      sim3pulse(0.0e-6,pwc,pwc,zero,zero,zero,0.0,0.0);
      dec2rgpulse((PI-2.0)/PI*(pwn-pwc),zero,0.0,0.0);

   }

   delay(2.0e-6);
   zgradpulse(gzlvl11,gt11);
   delay(gstab);

   lk_sample();

   rgpulse(pw,t8,4.0e-6,0.0);                    /* 90 deg 1H pulse */

   delay(2.0e-6);
   zgradpulse(gzlvl12,gt12);
   delay(2.0e-6);

   decpower(d_ib);

   if(n_shift[A] == 'n') {

   delay(taua - gt12 - 4.0e-6 - WFG2_START_DELAY - POWER_DELAY);
   simshaped_pulse(Hshp,shib,pwhshp,pwib,t8,zero,0.0,0.0);
   decphase(zero);
   }

   else {
 delay(taua - gt12 - 4.0e-6 - WFG3_START_DELAY - POWER_DELAY);                 
 sim3shaped_pulse(Hshp,shib,"hard",pwhshp,pwib,2.0*pwn,t8,zero,zero,0.0,0.0);
    }

   delay(2.0e-6);
   zgradpulse(gzlvl12,gt12);
   delay(2.0e-6);

   if(n_shift[A] == 'n')
     delay(taua - gt12 - 4.0e-6 - WFG2_STOP_DELAY - 2.0*POWER_DELAY);
   else
     delay(taua - gt12 - 4.0e-6 - WFG3_STOP_DELAY - 2.0*POWER_DELAY);

   decpower(dpwr);  /* Set power for decoupling */
   dec2power(dpwr2);  /* Set power for decoupling */

   rgpulse(pw,t8,0.0,rof2);

/* BEGIN ACQUISITION */

status(C);
setreceiver(t9);

}
