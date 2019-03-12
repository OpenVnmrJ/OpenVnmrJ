/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* gihca_con_cah.c

 Uses coherence selection and (optional) magic angle gradient pulses.

 3D gihca_con_cah  correlates Ha(i), CO(i-1) and N(i).
 Ref: Mäntylahti et al., J. Biomol. NMR, 49, 99-109 (2011).

    Uses three channels:
         1)  1H   (t3)       - carrier  4.7 ppm (tof)
         2) 13CO  (t1, ni)   - carrier  174 ppm  
            13Ca             - carrier  174 ppm (dof) 
         3) 15N   (t2, ni2)  - carrier  120 ppm (dof2) 

    Setting the delay bigTNCa to ~25 ms provides null 
    for Ha(i-1), CO(i-1), N(i) cross peaks, whereas both pathways
    (with opposite phases!) can be selected by setting 
    bigTNCa=bigTNC ~15ms.

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

  modified for BioPack, GG Palo Alto, Sept. 1998
 */

#include <standard.h> 

static int      
    
    phi1[4] = {0,0,2,2},
    phi2[2] = {0,2},   
    phi3[8] = {1,1,1,1,3,3,3,3},
    phi4[16] ={0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1},
    phi5[1]  ={0},
    rec[16] = {0,2,2,0,2,0,0,2,2,0,0,2,0,2,2,0};

extern int dps_flag;       
static double d2_init=0.0, d3_init=0.0;

void shaka6(double pwC90)
{
          /* Shaka composite (10.9333*pwC90)   */

        decrgpulse(pwC90*158.0/90.0, zero, 0.0, 0.0);
        decrgpulse(pwC90*171.2/90.0, two, 0.0, 0.0);
        decrgpulse(pwC90*342.8/90.0, zero, 0.0, 0.0); 
        decrgpulse(pwC90*145.5/90.0, two, 0.0, 0.0);
        decrgpulse(pwC90*81.2/90.0, zero, 0.0, 0.0);
        decrgpulse(pwC90*85.3/90.0, two, 0.0, 0.0);
}

void pulsesequence()
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
    bigTCN,       /*  ~ 16.6 ms      */
    bigTNC,       /*  ~ 14 ms       */
    bigTNCa,      /*  ~ 50 ms       */
    gstab,         /*  ~0.2 ms, gradient recovery time     */

    pwClvl,   /* High power level for carbon on channel 2 */
    pwC,      /* C13 90 degree pulse length at pwClvl     */
    phshift3,
    phi7cal = getval("phi7cal"),  /* phase in degrees of the last C13 90 pulse */

    compH,    /* Compression factor for H1 (at tpwr)      */
    compC,     /* Compression factor for C13 (at pwClvl)   */
    pwNlvl,   /* Power level for Nitrogen on channel 3    */
    pwN,      /* N15 90 degree pulse lenght at pwNlvl     */
    pwC90,    /* 90 degree pulse length at low power level       */
    tpwrHd,   /* Power level for proton decoupling on channel 1  */
    pwHd,     /* H1 90 degree pulse lenth at tpwrHd.             */
    waltzB1 = getval("waltzB1"),  /* waltz16 field strength (in Hz)     */
  
    /* 180 degree pulse at Ca (56ppm), first off-resonance null at CO(174ppm)     */
    pwC2,                                   /* 180 degree pulse length at rf2 */
    rf5,                      /* fine power for 10.5 kHz rf for 600MHz magnet */

    pwC8,     /* 180 sinc pulse on CO (on resonance)     */
    pwC3,
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
    rf3,
    rf4;
 
         
/* LOAD VARIABLES */
    gzcal = getval("gzcal");  /* calbration factor for Z gradient          */
    pwC8 = getval("pwC8");
                 /*180 degree pulse at CO(174ppm) with null at Ca. This is
                  automatically calculated by the macro "proteincal" and
                  by the setup macro "ghca_co_n". The SLP pulse "offC8 is called
                  directly from your shapelib.    */

     pwC3 = getval("pwC3");
                     /*180 degree pulse at Ca(56ppm) null at CO(174ppm) */
                 /* automatically calculated by the macro "proteincal" and
                  by the setup macro "ghcaco". The SLP pulse "offC3 is called
                  directly from your shapelib.    */



    getstr("f1180",f1180);
    getstr("f2180",f2180);
    getstr("mag_flg", mag_flg);

    taua   = getval("taua"); 
    taub   = getval("taub");
    bigTC = getval("bigTC");
    bigTCN = getval("bigTCN");
    bigTNC = getval("bigTNC");
    bigTNCa = getval("bigTNCa");
  
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
    phshift3 = getval("phshift3");

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

    if(0.5*(ni/sw1) > (bigTCN))
    {
      printf("ni is too big, should be < %f\n",sw1*(bigTCN - 2.0*pwN));
       psg_abort(1);
    }

    if(0.5*(ni2/sw2) > (bigTNCa))
    {
      printf("ni2 is too big, should be < %f\n",sw2*(bigTNCa - 2.0*pwN));
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

    if( dpwr > 50 )
    {
        printf("don't fry the probe, DPWR too large!  ");
        psg_abort(1);
    }

/*  Phase incrementation for hypercomplex 2D data */

    if (phase1 == 2) 
    {
       tsadd(t2, 1, 4);   
    }

    if (phase2 == 2)
    {
       tsadd(t1,1,4); 
       icosel = 1;
    }

    else icosel = -1;

   
/*  Set up f1180  tau1 = t1               */
      
    tau1 = d2;

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
       tsadd(t2,2,4);     
       tsadd(t16,2,4);    
    }

    if( ix == 1) d3_init = d3 ;
    t2_counter = (int)((d3-d3_init)*sw2 + 0.5);
    if((t2_counter % 2)) 
    {
       tsadd(t1,2,4);  
       tsadd(t16,2,4);    
    }

   decstepsize(1.0);
   initval(phshift3, v1);

/* 180 degree sinc pulse on CO, null at Ca 118ppm away */
	rf2 = (compC*4095.0*pwC*2.0*1.65)/pwC8;
	rf2 = (int) (rf2 + 0.5);

/* 180 degree pulse on Ca, null at CO 118ppm away */
        rf4 = (compC*4095.0*pwC*2.0)/pwC3;
        rf4 = (int) (rf4 + 0.5);

/* 180 degree pulse on Ca, null at CO 118ppm away */
        pwC2 = sqrt(3.0)/(2.0*118.0*dfrq);
        rf5 = (4095.0*compC*pwC*2.0)/pwC2;
        rf5 = (int) (rf5 + 0.5);
        if( rf5 > 4095.0 )
         { printf("increase pwClvl so pwC<24us*600/sfrq"); psg_abort(1);}

/* power level and pulse time for WALTZ 1H decoupling */
     pwHd = 1/(4.0 * waltzB1) ;                           
     tpwrHd = tpwr - 20.0*log10(pwHd/(compH*pw));
     tpwrHd = (int) (tpwrHd + 0.5);
 

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

    decphase(t2);
    delay(10.0e-6);
    decrgpulse(pwC90, t2, 0.0, 0.0);
    decphase(zero);

    delay(tau1); 
    decpwrf(rf4);
    decshaped_pulse("offC3", pwC3, zero, 0.0, 0.0);

    decpwrf(rf2);
    
    delay(bigTCN);

    decshaped_pulse("offC8", pwC8, zero, 0.0, 0.0);
    dec2phase(zero);
    dec2rgpulse(2.0*pwN, zero, 0.0, 0.0);

    delay(bigTCN - 2.0*pwN - tau1);

    decpwrf(rf4);
    decshaped_pulse("offC3", pwC3, zero, 0.0, 0.0);
    decpwrf(rf3);
    decrgpulse(pwC90, one, 0.0, 0.0);

       xmtroff();
       obsprgoff();
       rgpulse(pwHd,three,2.0e-6,0.0);
       delay(2.0e-6);
       zgradpulse(gzlvl7, gt7);
       delay(150.0e-6);
       rgpulse(pwHd,one,2.0e-6,0.0);
       txphase(zero);
       delay(2.0e-6);
       obsprgon("waltz16", pwHd, 90.0);
       xmtron();
    
    dec2phase(t1);
    dec2rgpulse(pwN, t1, 0.0, 0.0);

    delay(tau2);

    delay(bigTCN);

    decpwrf(rf2);
    decshaped_pulse("offC8", pwC8, zero, 0.0, 0.0);

    delay(bigTNCa - bigTCN);

    dec2rgpulse(2.0*pwN, zero, 0.0, 0.0);
    decpwrf(rf4);
    decshaped_pulse("offC3", pwC3, zero, 0.0, 0.0);

    delay(bigTNCa - tau2);
    
    dec2rgpulse(pwN,one,0.0,0.0);

    decoffset(dof);

       xmtroff();
       obsprgoff();
       rgpulse(pwHd,three,2.0e-6,0.0);
       delay(2.0e-6);
       zgradpulse(gzlvl8,gt8);
       delay(150.0e-6);

    rgpulse(pwHd,one,2.0e-6,0.0);
    txphase(zero);
    delay(2.0e-6);
    obsprgon("waltz16", pwHd, 90.0);
    xmtron();
    txphase(zero);
    decphase(t4);   
    delay(10.0e-6);

    decpwrf(rf3);
    decrgpulse(pwC90, t4, 0.0, 0.0);
    decpwrf(rf5);
    delay(bigTNC);
    
    decrgpulse(pwC2, zero, 0.0, 0.0);
    dec2rgpulse(2.0*pwN, zero, 0.0, 0.0);

    delay(bigTNC - pwHd - PRG_STOP_DELAY - 2.0*pwN - taub);

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
       magradpulse(-1*icosel*gzcal*gzlvl9, gt6/4.0);
    }
    else
    {
       zgradpulse(-1*gzlvl9, gt6/4.0);  
    }           
        
    delay(gstab);
    rcvron();
statusdelay(C,1.0e-4 -rof1 );
     setreceiver(t16);
}
