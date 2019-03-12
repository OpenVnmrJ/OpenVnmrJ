/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*ghca_co_canh.c, 

Use Semi-constant time in CO dimension so that more increments
can be used in the CO dimension.  Maximun ni is set to 64.

When sw1 = 2000, the value of ratio at different ni is:
ni    = 64,   48,   32,   24,   14 (minimum value)
ratio = 0.38, 0.50, 0.76, 1.01, 1.74

if the calculated value is larger than 1.0, it will be set to 1.0.
Semi-constant time evolution is used when ratio is smaller than 1.0
Constant time evolution is used when ratio is 1.0.

Recommended value: ratio = 0.5 to 1.0.

 3D experiment correlates HN(i), N(i) and CO (both i and i-1).
 Ref: Frank Lorh and Heinz Ruterjans,  JBNMR, 6 (1995) 189-197.

    Uses three(four) channels:
         1)  1H   (t3)       - carrier  4.7 ppm (tof)
         2) 13CO  (t1, ni)   - carrier  174 ppm  (dof)
                               SLP pulses are used for CA
         3) 15N   (t2, ni2)  - carrier  120 ppm (dof2) 
         4) 2H               - at lock frequency

    The waltz16 field strength is enterable (waltzB1).
    Typical values would be ~6-8ppm, (3500-5000 Hz at 600 MHz).
  
Weixing Zhang,
St.Jude Children's Hospital,
Memphis, Tenn.
 September 1, 1998.

Modified for BioPack, GG Palo Alto, Oct 1998
interchanged gzlvl1/gzlvl6, gzlvl2/gzlvl9, gt1/gt6 for consistency 10/02
 */

#include <standard.h> 

static int      
    
    phi1[4] = {0,0,2,2},
    phi2[2] = {0,2},   
    phi3[8] = {0,0,0,0,2,2,2,2},
    phi4[16] = {0,0,0,0,0,0,0,0,2,2,2,2,2,2,2,2},
    phi5[1]  ={0},
                
    rec[8] = {0,2,2,0,2,0,0,2};

extern int dps_flag;       
static double d2_init=0.0, d3_init=0.0;
            
void pulsesequence()
{
 char    
    f1180[MAXSTR],    
    f2180[MAXSTR],
    mag_flg[MAXSTR];  /* y for magic angle, n for z-gradient only  */


 int        
    icosel,
    t1_counter,   
    t2_counter;   

 double      
    ni2,  
    ratio,        /* used to adjust t1 semi-constant time increment */
    tau1,       
    tau2,       
    taua,         /*  ~ 1/4JCH =  1.5 ms - 1.7 ms]        */
    taub,         /*  ~ 3.3 ms          */
    bigTC,        /*  ~ 8 ms            */
    bigTCO,       /*  ~ 6 ms           */
    bigTN,        /*  ~ 12 ms           */
    tauc,         /*  ~ 5.4 ms          */
    taud,         /*  ~ 2.3 ms          */
    gstab,         /*  ~0.2 ms, gradient recovery time     */

    pwClvl,   /* High power level for carbon on channel 2 */
    pwC,      /* C13 90 degree pulse length at pwClvl     */

    compH,     /* Compression factor for H1 on channel 1  */
    compC,     /* Compression factor for C13 on channel 2  */
    pwNlvl,   /* Power level for Nitrogen on channel 3    */
    pwN,      /* N15 90 degree pulse lenght at pwNlvl     */
    maxpwCN,

    pwCa90,   /*90 "offC13" pulse at Ca(56ppm) xmtr at CO(174ppm) */
    pwCa180,  /*180 "offC17" pulse at Ca(56ppm) xmtr at CO(174ppm) */
    pwCO90,   /* 90 "offC6" pulse at CO(174ppm) xmtr at CO(174ppm)*/
    pwCO180,  /* 180 "offC8" pulse at CO(174ppm) xmtr at CO(174ppm)*/
    pwCab180, /* 180 "offC27" pulse at Cab(46ppm) xmtr at CO(174ppm)*/

    tpwrHd,   /* Power level for proton decoupling on channel 1  */
    pwHd,     /* H1 90 degree pulse lenth at tpwrHd.             */
        waltzB1 = getval("waltzB1"),  /* waltz16 field strength (in Hz)     */
  



    phi_CO,   /* phase correction for Bloch-Siegert effect on CO */
    phi_Ca,   /* phase correction for Bloch-Siegert effect on Ca */

    gt1,
    gt0,
    gt3,
 
    gt5,
    gt6,
    gt7,
    gt4, 

    gzlvl6,  
    gzlvl0,   
    gzlvl3,
 
    gzlvl5,
    gzlvl1,   /* N15 selection gradient level in DAC units */
    gzlvl7,
    gzlvl4,
    gzlvl2,   /* H1 gradient level in DAC units            */
    gzcal,    /* gradient calibration (gcal)               */
    dfCa180,
    dfCab180,
    dfC90,
    dfCa90,
    dfCO180;
 
         
/* LOAD VARIABLES */

    getstr("f1180",f1180);
    getstr("f2180",f2180);
    getstr("mag_flg", mag_flg);

    gzcal  = getval("gzcal");
      ni2  = getval("ni2");
    taua   = getval("taua"); 
    taub   = getval("taub");
    tauc = getval("tauc");
    bigTC = getval("bigTC");
    bigTCO = getval("bigTCO");
    bigTN = getval("bigTN");
    taud = getval("taud");
    gstab = getval("gstab");
  
    pwClvl = getval("pwClvl");
    pwC = getval("pwC");
    compH = getval("compH");
    compC = getval("compC");

    pwNlvl = getval("pwNlvl");
    pwN = getval("pwN");

    pwCa90 = getval("pwCa90");
    pwCa180 = getval("pwCa180");
    pwCab180 = getval("pwCab180");
    pwCO90 = getval("pwCO90");
    pwCO180 = getval("pwCO180");

    maxpwCN = 2.0*pwN;
    if (pwCab180 > pwN) maxpwCN = pwCab180;

    phi_CO = getval("phi_CO");
    phi_Ca = getval("phi_Ca");

    gt1 = getval("gt1");
    gt0 = getval("gt0");
    gt3 = getval("gt3");
    gt5 = getval("gt5");
    gt6 = getval("gt6");
    gt7 = getval("gt7");
    gt4 = getval("gt4");
 
    gzlvl6 = getval("gzlvl6");
    gzlvl0 = getval("gzlvl0");
    gzlvl3 = getval("gzlvl3");
    gzlvl5 = getval("gzlvl5");
    gzlvl1 = getval("gzlvl1");
    gzlvl7 = getval("gzlvl7");
    gzlvl4 = getval("gzlvl4");
    gzlvl2 = getval("gzlvl2");

 
    dfCa180 = (compC*4095.0*pwC*2.0*1.69)/pwCa180;           /*power for "offC17" pulse*/
    dfCab180 = (compC*4095.0*pwC*2.0*1.69)/pwCab180;       /*power for "offC27" pulse*/
    dfC90  = (compC*4095.0*pwC*1.69)/pwCO90;                  /*power for "offC6" pulse */
    dfCa90  = (compC*4095.0*pwC)/pwCa90;                     /*power for "offC13" pulse*/
    dfCO180  = (compC*4095.0*pwC*2.0*1.65)/pwCO180;          /*power for "offC8" pulse */


	dfCa90 = (int) (dfCa90 + 0.5);
	dfCa180 = (int) (dfCa180 + 0.5);
	dfC90 = (int) (dfC90 + 0.5);
	dfCO180 = (int) (dfCO180 + 0.5);	
        dfCab180 = (int) (dfCab180 +0.5);


    /* power level and pulse time for WALTZ 1H decoupling */
	pwHd = 1/(4.0 * waltzB1) ;                         
	tpwrHd = tpwr - 20.0*log10(pwHd/(compH*pw));
	tpwrHd = (int) (tpwrHd + 0.5);
 
 
/* LOAD PHASE TABLE */

    settable(t1,4,phi1);
    settable(t2,2,phi2);
    settable(t3,8,phi3);
    settable(t4,16,phi4);
    settable(t5,1,phi5);
   
    settable(t16,8,rec); 

/* CHECK VALIDITY OF PARAMETER RANGES */
   
    if(ni > 64)
    {
       printf("ni is out of range. Should be: 14 to 64 ! \n");
       psg_abort(1);
    }
/*
    if(ni/sw1 > 2.0*(bigTCO))
    {
       printf("ni is too big, should be < %f\n", sw1*2.0*(bigTCO));
       psg_abort(1);
    }
*/

    if(ni2/sw2 > 2.0*(bigTN - pwCO180))
    {
       printf("ni2 is too big, should be < %f\n",2.0*sw2*(bigTN-pwCO180));
       psg_abort(1);
    }

    if((dm[A] == 'y' || dm[B] == 'y' ))
    {
       printf("incorrect dec1 decoupler flags! Should be 'nnn' ");
       psg_abort(1);
    }

    if((dm2[A] == 'y' || dm2[B] == 'y'))
    {
       printf("incorrect dec2 decoupler flags! Should be 'nny' ");
       psg_abort(1);
    }


    if( dpwr > 50 )
    {
        printf("don't fry the probe, DPWR too large!  ");
        psg_abort(1);
    }

/*  Phase incrementation for hypercomplex 2D data */

    if (phase1 == 1) 
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
      
    tau1 = d2;
    if(f1180[A] == 'y') 
    {
        tau1 += (1.0/(2.0*sw1));
    }
    if(tau1 < 0.2e-6) tau1 = 0.0;
    tau1 = tau1/4.0;
    ratio = 2.0*bigTCO*sw1/((double) ni);
    ratio = (double)((int)(ratio*100.0))/100.0;
    if (ratio > 1.0) ratio = 1.0;
    if((dps_flag) && (ni > 1)) 
        printf("ratio = %f => %f\n",2.0*bigTCO*sw1/((double) ni), ratio);

/*  Set up f2180  tau2 = t2               */
    tau2 = d3;
    if(f2180[A] == 'y') 
    {
        tau2 += (1.0/(2.0*sw2));
    }
    if(tau2 < 0.2e-6) tau2 = 0.0;
    tau2 = tau2/4.0;
 

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

    decstepsize(1.0);
    initval(phi_CO, v1);
    initval(phi_Ca, v2);

/* BEGIN ACTUAL PULSE SEQUENCE */

status(A);
   	delay(d1-1.0e-3);
    obsoffset(tof);
    decoffset(dof);
    obspower(tpwr);        
    decpower(pwClvl);
    decpwrf(4095.0);
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
       zgradpulse(gzlvl6, gt6);
    }  
    decpwrf(dfCa180); 
    delay(1.0e-3);

    rgpulse(pw,zero,1.0e-6,1.0e-6);            
    delay(2.0e-6);
    zgradpulse(gzlvl0,gt0);  
    delay(taua - gt0 - 2.0e-6 - WFG_START_DELAY);

    simshaped_pulse("","offC17",2.0*pw,pwCa180,zero,zero,1.0e-6,1.0e-6);
    decphase(zero);
      /* c13 offset on CO, slp 180 on Ca */
    delay(taua - gt0 - 500.0e-6 - WFG_STOP_DELAY);
    zgradpulse(gzlvl0,gt0); 
    txphase(one);
    delay(500.0e-6);
    rgpulse(pw, one, 1.0e-6, 1.0e-6);
  
    delay(2.0e-6);
    zgradpulse(gzlvl3,gt3);

    obspower(tpwrHd);
    decpwrf(dfCa90);
    delay(200.0e-6);
   
    decshaped_pulse("offC13", pwCa90, zero, 0.0, 0.0);
      /* c13 offset on CO, slp 90 on Ca */
    delay(taub - PRG_START_DELAY);

    obsprgon("waltz16", pwHd,180.0);
    xmtron();

    decpwrf(dfC90);
    decphase(t1);

    delay(bigTC - taub);

    decshaped_pulse("offC6", pwCO90, t1, 0.0, 0.0);

       /* c13 offset on CO, on-res 90 on CO */
    decpwrf(dfCO180);
    decphase(zero);

    delay(bigTCO/2.0 + maxpwCN/2.0 + WFG_STOP_DELAY - 2.0*pwCO90/PI - ratio*tau1);
    decshaped_pulse("offC8", pwCO180, zero, 0.0, 0.0);
       /* c13 offset on CO, on-res 180 on CO */
    decpwrf(dfCab180);
    delay(bigTCO/2.0 + (2.0 - ratio)*tau1 - PRG_STOP_DELAY);
    xmtroff();
    obsprgoff();

    sim3shaped_pulse("","offC27","",0.0,pwCab180,2.0*pwN,zero,zero,zero,0.0,0.0);
      /* c13 offset on CO, slp 180 at Cab */

    obsprgon("waltz16", pwHd,180.0);
    xmtron();
    decpwrf(dfCO180);
    delay(bigTCO/2.0 + (2.0 - ratio)*tau1 - PRG_START_DELAY);
    decshaped_pulse("offC8", pwCO180, zero, 0.0, 0.0);
       /* c13 offset on CO, on-res 180 on CO */
    decpwrf(dfC90);
    dcplrphase(v1);  
    delay(bigTCO/2.0+maxpwCN/2.0+WFG_STOP_DELAY-2.0*pwCO90/PI-ratio*tau1-SAPS_DELAY);
    decshaped_pulse("offC6", pwCO90, zero, 0.0, 0.0);
       /* c13 offset on CO, on-res 90 on CO */

    decpwrf(dfCa90);
    dcplrphase(v2);
    decphase(t3);  

         delay(bigTC - SAPS_DELAY);
    decshaped_pulse("offC13", pwCa90, t3, 0.0, 0.0);
      /* c13 offset on CO, slp 90 at Ca */
    xmtroff();
    obsprgoff();
    dcplrphase(zero);
 
    delay(2.0e-5);
    zgradpulse(gzlvl4,gt4);
    delay(2.0e-6);
    obsprgon("waltz16", pwHd, 180.0);
    xmtron();
    txphase(zero);
    decphase(zero);
    dec2phase(t2);   
    delay(150.0e-6);

    dec2rgpulse(pwN, t2, 0.0, 0.0);
    decpwrf(dfCO180);

         delay(bigTN/2.0 - tau2);
    decshaped_pulse("offC8", pwCO180, zero, 0.0, 0.0);
       /* c13 offset on CO, on-res 180 on CO */
    decpwrf(dfCa180);
    dec2phase(t4);
    delay(bigTN/2.0 - tau2);
 
    dec2rgpulse(2.0*pwN, t4, 0.0, 0.0);
    decshaped_pulse("offC17", pwCa180, zero, 0.0, 0.0);
      /* c13 offset on CO, slp 180 at Ca */
    decpwrf(dfCO180);
    delay(bigTN/2.0 + tau2 - pwCa180 - WFG_START_DELAY - WFG_STOP_DELAY);
    decshaped_pulse("offC8", pwCO180, zero, 0.0, 0.0);
       /* c13 offset on CO, on-res 180 on CO */
    delay(bigTN/2.0 + tau2 - tauc - PRG_STOP_DELAY);
    dec2phase(t5);

    xmtroff();
    obsprgoff();
    obspower(tpwr);
        delay(tauc - gt1 - 1.0e-3 - 2.0*GRADIENT_DELAY);

    if (mag_flg[A] == 'y')
    {
       magradpulse(gzcal*gzlvl1, gt1);
    }
    else
    {
       zgradpulse(gzlvl1, gt1);
    }
    delay(1.0e-3);
  
    sim3pulse(pw,0.0, pwN, zero,zero, t5, 0.0, 0.0);
    dec2phase(zero);
    delay(2.0e-6);
    zgradpulse(0.8*gzlvl5, gt5);
    delay(taud - gt5 - 2.0e-6);
    sim3pulse(2.0*pw,0.0, 2.0*pwN, zero,zero, zero, 0.0, 0.0);
    delay(taud - gt5 - 500.0e-6);
    zgradpulse(0.8*gzlvl5, gt5);
    txphase(one);
    decphase(one);
    delay(500.0e-6);

    sim3pulse(pw,0.0, pwN, one,zero, one, 0.0, 0.0);
    
    delay(2.0e-6);
    txphase(zero);
    decphase(zero);
    zgradpulse(gzlvl5, gt5);
    delay(taud - gt5 - 2.0e-6);
    sim3pulse(2.0*pw,0.0, 2.0*pwN, zero,zero, zero, 0.0, 0.0);
  
    delay(taud - gt5 - 2.0*POWER_DELAY - 500.0e-6);
    zgradpulse(gzlvl5, gt5);
    decpower(dpwr);
    dec2power(dpwr2);
    delay(500.0e-6);

    rgpulse(pw, zero, 0.0, 0.0);

    delay(1.0e-4 +gstab + gt1/10.0 + 2.0*GRADIENT_DELAY);
    rgpulse(2.0*pw, zero, 0.0, rof1);
    if(mag_flg[A] == 'y')
    {
       magradpulse(icosel*gzcal*gzlvl2, gt1/10.0);
    }
    else
    {
       zgradpulse(icosel*gzlvl2, gt1/10.0);  
    }           
    delay(gstab);    
    rcvron();
statusdelay(C,1.0e-4  - rof1);
	setreceiver(t16);
}
