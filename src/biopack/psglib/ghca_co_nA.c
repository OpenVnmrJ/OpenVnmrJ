/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* ghca_co_nA.c

 Uses coherence selection and (optional) magic angle gradient pulses.

 3D hcacon  correlates Ha, Ca and N15(i+1).
 Ref: R.Powers and A.Bax,  JMR, 94, 209-213(1991).

    Uses three channels:
         1)  1H   (t3)       - carrier  4.7 ppm (tof)
         2) 13CA  (t2, ni2)  - carrier  56 ppm  (dof)
            13CO             - carrier  174 ppm 
         3) 15N   (t1, ni)   - carrier  120 ppm (dof2) 

    gzcal = gcal for z gradient (gauss/dac),   
    mag_flg = y,  using magic angle pulsed field gradient
                   uses gzcal (= 0.0022 gauss/dac, for example)   
             =n,  using z-axis gradient only
   Walter Zhang, 
   St. Jude Children's Research Hospital
   Memphis, TN 381
   3-13-98


    The waltz16 field strength is enterable (waltzB1).
    Typical values would be ~6-8ppm, (3500-5000 Hz at 600 MHz).
  
   The coherence-transfer gradients using power levels
   gzlvl6 and gzlvl9 may be either z or magic-angle gradients. For the
   latter, a proper /vnmr/imaging/gradtable entry must be present and
   syscoil must contain the value of this entry (name of gradtable). The
   amplitude of the gzlvl6 and gzlvl9 should be lower than for a z axis
   probe to have the x and y gradient levels within the 32k range. For
   any value, a dps display (using power display) shows the x,y and z
   dac values. These must be <=32k.

   The autocal and checkofs flags are generated automatically in Pbox_bio.h
   If these flags do not exist in the parameter set, they are automatically 
   set to 'y' - yes. In order to change their default values, create the flag(s) 
   in your parameter set and change them as required. 
   The available options for the checkofs flag are: 'y' (yes) and 'n' (no). 
   The offset (tof, dof, dof2 and dof3) checks can be switched off individually 
   by setting the corresponding argument to zero (0.0).
   For the autocal flag the available options are: 'y' (yes - by default), 
   'q' (quiet mode - suppress Pbox output), 'r' (read from file, no new shapes 
   are created), 's' (semi-automatic mode - allows access to user defined 
   parameters) and 'n' (no - use full manual setup, equivalent to the original 
   code).   

   modified for BioPack, GG Palo Alto, Sept. 1998
   Auto-calibrated version, E.Kupce, 27.08.2002.
 */

#include <standard.h> 
#include "Pbox_bio.h"               /* Pbox Bio Pack Pulse Shaping Utilities */

static int      
    
    phi1[4] = {0,0,2,2},
    phi2[2] = {0,2},   
    phi3[8] = {1,1,1,1,3,3,3,3},
    phi4[16] ={0,0,0,0,0,0,0,0,2,2,2,2,2,2,2,2},
    phi5[1]  ={0},
                
    rec[16] = {0,2,2,0,2,0,0,2,2,0,0,2,0,2,2,0};

extern int dps_flag;       
static double d2_init=0.0, d3_init=0.0;
static double   H1ofs=4.7, C13ofs=56.0, N15ofs=120.0, H2ofs=0.0;

static shape wz16, offC8;

shaka6(pwC90)
double pwC90;
{
          /* Shaka composite (10.9333*pwC90)   */

        decrgpulse(pwC90*158.0/90.0, zero, 0.0, 0.0);
        decrgpulse(pwC90*171.2/90.0, two, 0.0, 0.0);
        decrgpulse(pwC90*342.8/90.0, zero, 0.0, 0.0); 
        decrgpulse(pwC90*145.5/90.0, two, 0.0, 0.0);
        decrgpulse(pwC90*81.2/90.0, zero, 0.0, 0.0);
        decrgpulse(pwC90*85.3/90.0, two, 0.0, 0.0);
}

pulsesequence()
{
 char    
    f1180[MAXSTR],    
    f2180[MAXSTR],
    mag_flg[MAXSTR];  /* y for magic angle, n for z-gradient only  */


 int        
    icosel,
    ni2,     
    t1_counter,   
    t2_counter;   

 double      
    tau1,       
    tau2,       
    taua,         /*  ~ 1/4JCH =  1.5 ms - 1.7 ms]        */
    taub,         /*  ~ 3.3 ms       */
    bigTC,        /*  ~ 3.5 ms       */
    bigTCN,       /*  ~ 18 ms       */
    gstab,         /*  ~0.2 ms, gradient recovery time     */

    pwClvl,   /* High power level for carbon on channel 2 */
    pwC,      /* C13 90 degree pulse length at pwClvl     */

    compH,    /* Compression factor for H1 (at tpwr)      */
    compC,     /* Compression factor for C13 (at pwClvl)   */
    pwNlvl,   /* Power level for Nitrogen on channel 3    */
    pwN,      /* N15 90 degree pulse lenght at pwNlvl     */
    pwC90,    /* 90 degree pulse length at low power level       */
    tpwrHd,   /* Power level for proton decoupling on channel 1  */
    pwHd,     /* H1 90 degree pulse lenth at tpwrHd.             */
    waltzB1 = getval("waltzB1"),  /* waltz16 field strength (in Hz)     */
    bw, ppm,  /* bandwidth, ppm - temporary Pbox parameters */

    pwC8,     /* 180 sinc pulse on CO (on resonance)     */
    gzcal,    /* z gradient calibration (gcal)           */
    gt1,
    gt0,
    gt3,
    gt4,
    gt5,
    gt6,
    gt7,
    gt8, 

    gzlvl1,
    gzlvl0,
    gzlvl3,
    gzlvl4,
    gzlvl5,
    gzlvl6,   /* C13 selection gradient level in DAC units */
    gzlvl7,
    gzlvl8,
    gzlvl9,   /* H1 gradient level in DAC units            */
    rf0,
    rf2,
    rf3;
 
         
/* LOAD VARIABLES */
    gzcal = getval("gzcal");  /* calbration factor for Z gradient          */
    pwC8 = getval("pwC8");
                 /*180 degree pulse at CO(174ppm) with null at Ca. This is
                  automatically calculated by the macro "biocal" and
                  by the setup macro "ghca_co_n". The SLP pulse "offC8 is called
                  directly from your shapelib.    */

    getstr("f1180",f1180);
    getstr("f2180",f2180);
    getstr("mag_flg", mag_flg);

    taua   = getval("taua"); 
    taub   = getval("taub");
    bigTC = getval("bigTC");
    bigTCN = getval("bigTCN");
  
    pwClvl = getval("pwClvl");
    pwC = getval("pwC");
    compH= getval("compH");
    compC = getval("compC");

    pwNlvl = getval("pwNlvl");
    pwN = getval("pwN");
    pwC90 = getval("pwC90");

    sw1 = getval("sw1");
    sw2 = getval("sw2");
    ni = getval("ni");
    ni2 = getval("ni2");
    rf0 = 4095.0;
    rf3  = (compC*4095.0*pwC)/pwC90;
  
    gt1 = getval("gt1");
    gt0 = getval("gt0");
    gt3 = getval("gt3");
    gt4 = getval("gt4");
    gt5 = getval("gt5");
    gt6 = getval("gt6");
    gt7 = getval("gt7");
    gt8 = getval("gt8");
    gstab = getval("gstab");
 
    gzlvl1 = getval("gzlvl1");
    gzlvl0 = getval("gzlvl0");
    gzlvl3 = getval("gzlvl3");
    gzlvl4 = getval("gzlvl4");
    gzlvl5 = getval("gzlvl5");
    gzlvl6 = getval("gzlvl6");
    gzlvl7 = getval("gzlvl7");
    gzlvl8 = getval("gzlvl8");
    gzlvl9 = getval("gzlvl9");

 
    
/* LOAD PHASE TABLE */

    settable(t1,4,phi1);
    settable(t2,2,phi2);
    settable(t3,8,phi3);
    settable(t4,16,phi4);
    settable(t5,1,phi5);
   
    settable(t16,16,rec); 

/* CHECK VALIDITY OF PARAMETER RANGES */

    if(ni2/sw2 > (2.0*bigTC - 11*pwC))
    {
       printf("ni2 is too big, should be < %f\n",sw2*(2.0*bigTC-11*pwC));
       psg_abort(1);
    }

    if((dm[A] == 'y' || dm[B] == 'y'))
    {
       printf("incorrect dec1 decoupler flags! Should be 'nny' ");
       psg_abort(1);
    }

    if((dm2[A] == 'y' || dm2[B] == 'y' || dm2[C] == 'y'))
    {
       printf("incorrect dec2 decoupler flags! Should be 'nnn' ");
       psg_abort(1);
    }

    if( dpwr > 50)
    {
        printf("don't fry the probe, DPWR too large!  ");
        psg_abort(1);
    }

/*  Phase incrementation for hypercomplex 2D data */

    if (phase1 == 2) 
    {
       tsadd(t1, 1, 4);   
    }

    if (phase2 == 2)
    {
       tsadd(t5,2,4);
       icosel = 1; 
    }
    else icosel = -1;

   
/*  Set up f1180  tau1 = t1               */
      
    tau1 = d2 - (4.0*pwN/PI + pwC8 + 2.0*POWER_DELAY);
    tau1 -= (WFG_START_DELAY + WFG_STOP_DELAY);

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
        if(tau2 < 0.2e-6) tau2 = 0.0;
    }
    tau2 = tau2/2.0;
 

/* Calculate modifications to phases for States-TPPI acquisition  */

    if( ix == 1) d2_init = d2 ;
    t1_counter = (int)((d2-d2_init)*sw1 + 0.5);
    
    if((t1_counter % 2)) 
    {
       tsadd(t1,2,4);     
       tsadd(t16,2,4);    
    }

    if( ix == 1) d3_init = d3 ;
    t2_counter = (int)((d3-d3_init)*sw2 + 0.5);
    if((t2_counter % 2)) 
    {
       tsadd(t2,2,4);  
       tsadd(t16,2,4);    
    }

    setautocal();                        /* activate auto-calibration flags */ 
        
    if (autocal[0] == 'n') 
    {
/* 180 degree sinc pulse on CO, null at Ca 118ppm away */
      rf2 = (compC*4095.0*pwC*2.0*1.65)/pwC8;
      rf2 = (int) (rf2 + 0.5);

/* power level and pulse time for WALTZ 1H decoupling */
      pwHd = 1/(4.0 * waltzB1) ;                          
      tpwrHd = tpwr - 20.0*log10(pwHd/(compH*pw));
      tpwrHd = (int) (tpwrHd + 0.5);
    }
    else        /* if autocal = 'y'(yes), 'q'(quiet), r(read), or 's'(semi) */
    {
      if(FIRST_FID)                                            /* call Pbox */
      {
        ppm = getval("dfrq"); bw = 118.0*ppm; 
        offC8 = pbox_make("offC8", "sinc180n", bw, 0.0, compC*pwC, pwClvl);
        bw = 2.8*7500.0;
        wz16 = pbox_Dcal("WALTZ16", 2.8*waltzB1, 0.0, compH*pw, tpwr);

        ofs_check(H1ofs, C13ofs, N15ofs, H2ofs);
      }
      rf2 = offC8.pwrf; pwC8 = offC8.pw;
      tpwrHd = wz16.pwr; pwHd = 1.0/wz16.dmf;
    }

/* BEGIN ACTUAL PULSE SEQUENCE */

status(A);
     delay(d1);
    obsoffset(tof);
    decoffset(dof);
    obspower(tpwr);        
    decpower(pwClvl);
    decpwrf(rf0);
    dec2power(pwNlvl);
    txphase(zero);
    decphase(zero);
    dec2phase(zero);
    rcvroff();

    if(gt1 > 0.2e-6)
    {
       delay(10.0e-6);
       decrgpulse(pwC, zero, 1.0e-6, 1.0e-6);
       delay(0.2e-6);
       zgradpulse(gzlvl1, gt1);
    }   
    delay(1.0e-3);

    rgpulse(pw,zero,1.0e-6,1.0e-6);            
    delay(2.0e-6);
    zgradpulse(gzlvl0,gt0);  
    delay(taua - gt0 - 2.0e-6);

    simpulse(2.0*pw, 2.0*pwC, zero, zero, 1.0e-6, 1.0e-6);

    delay(taua - gt0 - 500.0e-6);
    zgradpulse(gzlvl0,gt0); 
    txphase(one);
    delay(500.0e-6);
    rgpulse(pw, one, 1.0e-6, 1.0e-6);
  
    delay(2.0e-6);
    zgradpulse(gzlvl3,gt3);
    obspower(tpwrHd);
    decpwrf(rf3);
    decphase(one);
    delay(200.0e-6);

    decrgpulse(pwC90, one, 0.0, 0.0);
    delay(taub - PRG_START_DELAY);


    rgpulse(pwHd,one,2.0e-6,0.0);
    txphase(zero);
    delay(2.0e-6);
    obsprgon("waltz16", pwHd, 90.0);
    xmtron();
    decpwrf(rf0);
    decphase(zero);
    delay(bigTC - taub - 0.5*10.9333*pwC);
    shaka6(pwC);   /*  Shaka composite 180 refocusing and inversion pulse  */
    decpwrf(rf3);
    decphase(t3);
    delay(bigTC - 0.5*10.9333*pwC);
    decrgpulse(pwC90, t3, 0.0, 0.0);
   
    decoffset(dof+(174-56)*dfrq);

    if (gt4 > 0.2e-6)
    {
       xmtroff();
       obsprgoff();
       rgpulse(pwHd,three,2.0e-6,0.0);
       delay(2.0e-6);
       zgradpulse(gzlvl4, gt4);
       delay(150.0e-6);
       rgpulse(pwHd,one,2.0e-6,0.0);
       txphase(zero);
       delay(2.0e-6);
       obsprgon("waltz16", pwHd, 90.0);
       xmtron();
    }
    decphase(t4);
    delay(10.0e-6);
    decrgpulse(pwC90, t4, 0.0, 0.0);
    decphase(zero);
    dec2phase(t1);
    delay(bigTCN - POWER_DELAY);
    decpwrf(rf2);
    dec2rgpulse(pwN, t1, 0.0, 0.0);
      delay(tau1);
      decshaped_pulse("offC8", pwC8, zero, 0.0, 0.0);  
      dec2phase(zero);
      delay(tau1);
    dec2rgpulse(pwN, zero, 0.0, 0.0);
    decpwrf(rf3);
    delay(bigTCN - POWER_DELAY);
    decrgpulse(pwC90, zero, 0.0, 0.0); 
    decoffset(dof);
    decphase(t2);

    if (gt8 > 0.2e-6)
    {
       xmtroff();
       obsprgoff();
       rgpulse(pwHd,three,2.0e-6,0.0);
       delay(2.0e-6);
       zgradpulse(gzlvl8,gt8);
       delay(150.0e-6);
    }
    rgpulse(pwHd,one,2.0e-6,0.0);
    txphase(zero);
    delay(2.0e-6);
    obsprgon("waltz16", pwHd, 90.0);
    xmtron();
    txphase(zero);
    decphase(t2);   
    delay(10.0e-6);

    decrgpulse(pwC90, t2, 0.0, 0.0);
    decpwrf(rf0);
    decphase(t4);
    delay(bigTC - tau2 - 0.5*10.9333*pwC -2.0*pwC90/PI);
    shaka6(pwC);
       delay(bigTC+tau2-taub-0.5*10.9333*pwC-PRG_STOP_DELAY-pwHd-2.0*pwC90/PI);
       
    xmtroff();
    obsprgoff();
    rgpulse(pwHd,three,2.0e-6,0.0);
  
    delay(2.0e-6);
    obspower(tpwr);
    if (mag_flg[A] == 'y')
    {
       magradpulse(gzcal*gzlvl6, gt6);
    }
    else
    {
       zgradpulse(gzlvl6, gt6);
    }
    decphase(t5);
    txphase(zero);
    decpwrf(rf0);
    delay(taub - gt6 - POWER_DELAY - 2.0*GRADIENT_DELAY - 4.0e-6);
    /*delay(pwC90 - pwC);     To account for different pulse length */
  
    simpulse(pw, pwC, zero, t5, 0.0, 0.0);
    decphase(zero);
    delay(2.0e-6);
    zgradpulse(0.8*gzlvl5, gt5);
    delay(taua - pwC - gt5 - 2.0e-6);
    simpulse(2.0*pw, 2.0*pwC, zero, zero, 0.0, 0.0);
    delay(taua - pwC - gt5 - 500.0e-6);
    zgradpulse(0.8*gzlvl5, gt5);
    txphase(one);
    decphase(one);
    delay(500.0e-6);

    simpulse(pw, pwC, one, one, 0.0, 0.0);
    
    delay(2.0e-6);
    txphase(zero);
    decphase(zero);
    zgradpulse(gzlvl5, gt5);
    delay(taua - gt5 - 2.0e-6);

    simpulse(2.0*pw, 2.0*pwC, zero, zero, 0.0, 0.0);
    delay(taua - gt5 - 2.0*POWER_DELAY - 500.0e-6);
    zgradpulse(gzlvl5, gt5);
    decpower(dpwr);
    dec2power(dpwr2);
    delay(500.0e-6);
	
    rgpulse(pw, zero, 0.0, 0.0);

    delay(1.0e-4 +gstab + gt6/4.0 + 2.0*GRADIENT_DELAY);
    rgpulse(2.0*pw, zero, 0.0, rof1);
    if(mag_flg[A] == 'y')
    {
       magradpulse(icosel*gzcal*gzlvl9, gt6/4.0);
    }
    else
    {
       zgradpulse(icosel*gzlvl9, gt6/4.0);  
    }           
        
    delay(gstab);
    rcvron();
statusdelay(C,1.0e-4 -rof1 );
     setreceiver(t16);
}
