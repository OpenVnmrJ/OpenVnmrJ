/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*   hbcbcaconnhA.c  Auto(Pbox)Calibrated version of hbcbcaconnh_3c_pfg1_sel_500.c

    This pulse sequence will allow one to perform the following experiment:

    3D cbcaconnh (enhanced sensitivity PFG and with minimal perturbation of
       water) correlating cb(i),ca(i) with n(i+1), nh(i+1).

                      F1       C_beta (i), C_alpha (i)
                      F2       N (i+1)
                      F3(acq)  NH (i+1)

    Uses three channels:
    1)  1H             - carrier (tof) @ 4.7ppm [H2O]
    2) 13C             - carrier (dof @ 58ppm [CA] and dofcacb @ 43ppm [CA,CB])
                                 (Note: centre of F1 is 43ppm (dofcacb))
    3) 15N             - carrier (dof2)@ 119ppm [centre of amide N]  


    Set dm = 'nnn', dmm = 'ccc' 
    Set dm2 = 'nny', dmm2 = 'ccp' [15N decoupling during acquisition]
                      dseq2 = 'waltz16' 

    Must set phase = 1,2 and phase2 = 1,2 for States-TPPI
    acquisition in t1 [carbon]  and t2 [N]. [The fids must be manipulated
    (add/subtract) with 'grad_sort_nd' program (or equivalent) before regular
    processing.]

    Flags
         fsat     'y' for presaturation of H2O
         fscuba   'y' to apply scuba pulse after presaturation of H2O
         f1180    'y' to get 180deg linear phase correction in F1,
                      otherwise linear phase correction is 0.
         f2180    'y' to get 180deg linear phase correction in F2,
                      otherwise linear phase correction is 0.

    Standard Settings: fsat='n', fscuba='n', f1180='n', f2180='n'.
    
    The flag f1180/f2180 should be set to 'y' if t1 is 
    to be started at halfdwell time. This will give -90, 180
    phasing in f1/f2. If it is set to 'n' the phasing will
    be 0,0 and will still give a perfect baseline.
    Set f1180 to n for (0,0) in C and f2180 = n for (0,0) in N

    Written by Lewis Kay 09/15/92 
    Modified by L.E.K 11/24/92 to include enhanced pfg and shaped C' pulses
    Modified by L.E.K. 01/03/93 to reduce phase cycle by adding gradients
             in the t1 evolution constant time domain. H decoupling now
             begins in the interval immediately following the t1 evolution
             domain.   
    Modified by G.G 10/14/93 to permit DPS and remove gate
    Modified by L. E. K May 25, 94 to minimize water saturation 

    Modified by L. E. Kay, Nov 2, 95 to ensure adequate delays between
         power statements and pulses (especially for Innova).

    Modified by L.E.Kay, Nov 24, 95 to include a complete exocycle on
         the N15 180 pulse in the center of the t2 evoln period. Can
         still use a 4 step cycle - 8 is preferred. 
    
    Added a user adjustable phase shift (sphase1) for one of the
    off-resonance CO 180b pulses - RM Mar 11, 1998.

    REF: D. R. Muhandiram and L. E. Kay  J. Magn. Reson. B 103, 203-216 (1994).
 
         L. E. Kay, P. Keifer and T. Saarinen 
                                     J. Am. Chem. Soc. 114, 10663-10665 (1992).
    
         Original non-gradient, unenhanced version
         S. Grzesiek and A. Bax, JACS 114 6291, (1992).



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


     Modified for Automatic Calibration (E.Kupce) feb03
     Modified sequence for current psg statements (no DEV's) and zgradpulses   GG apr03
*/

#include <standard.h>
/* #define PI 3.1416 */
#include "Pbox_bio.h"

#define CA90    "square90n 133p"      /* square 90 on Cab at 43 ppm, null at C', 133 ppm away */
#define CA180   "square180n 133p"    /* square 180 on Cab at 43 ppm, null at C', 133 ppm away */
#define CA180ps "-maxincr 2.0 -attn i"                         /* seduce 180 shape parameters */
#define CO180   "seduce 28p 133p"                /* seduce 180 on CO at 176 ppm, 133 ppm away */
#define CO180ps "-s 0.5 -attn i"                         /* seduce 180 shape parameters */
#define CO90b   "square90n 118p 118p" /* square 90 on CO at 176 ppm, null at Ca, 118 ppm away */
#define CO180b  "square180  28p 118p"                          /* square 180 on CO at 176 ppm */
#define CA180b  "square180n 118p"     /* square 180 on Ca at 58 ppm, null at C', 118 ppm away */
#define CADEC   "SEDUCE1 20p"                        /* on resonance SEDUCE1 on Ca, at 58 ppm */
#define CADECps "-dres 2.0 -maxincr 20.0 -attn i"

static shape ca90, ca180, co180, co90b, co180b, ca180b, cadec, w16;

static int  phi1[1]  = {0},
            phi2[1]  = {1},
            phi3[4]  = {0,0,2,2},
            phi4[1]  = {0},
            phi5[2]  = {0,2},
            phi6[2]  = {0,2},
            phi7[1]  = {0},
            phi8[1] = {0},
            phi9[8] = {0,0,2,2,1,1,3,3},
            rec[8] = {0,2,2,0,2,0,0,2},
            phi11[1] = {0},
            phi12[1] = {1},
            phi13[1] = {1};
           
static double d2_init=0.0, d3_init=0.0;
            
pulsesequence()
{
/* DECLARE VARIABLES */

 char       autocal[MAXSTR],  /* auto-calibration flag */
            fsat[MAXSTR],
	    fscuba[MAXSTR],
            f1180[MAXSTR],    /* Flag to start t1 @ halfdwell             */
            f2180[MAXSTR],    /* Flag to start t2 @ halfdwell             */
            spco180a[MAXSTR],
            spco90b[MAXSTR],
            spco180b[MAXSTR],
            cadecseq[MAXSTR];


 int         phase, phase2, ni, ni2, icosel,  /* used to get n and p type */
             t1_counter,   /* used for states tppi in t1           */ 
             t2_counter;   /* used for states tppi in t2           */ 

 double      tau1,         /*  t1 delay */
             tau2,         /*  t2 delay */
             taua,         /*  ~ 1/4JCH =  1.7 ms */
             tauc,         /* ~ 1/4JCAC' = 3.6 ms  */
             taud,         /* ~ 1/4JC'CA = 4.3, 4.4 ms */
             taue,         /* 1/4JC'N = 12.4 ms    */
             tauf,         /* 1/4JNH = 2.25 ms */
             BigTC,        /* carbon constant time period */
             BigTN,        /* nitrogen constant time period */
             pwn,          /* PW90 for 15N pulse              */
             pwca90a,      /* PW90 for 13C at dvhpwr    */
             pwca180a,     /* PW180 for 13C at dvhpwra  */
             pwco180a,
             pwca180b,      /* PW180 for ca nucleus @ dvhpwrb         */
             pwco180b,
             pwco90b,      /* PW90 for co nucleus @ dhpwrb         */
             tsatpwr,      /* low level 1H trans.power for presat  */
             tpwrml,       /* power level for h decoupling  */
             pwmlev,       /* h 90 pulse at tpwrml            */
             dhpwr,        /* power level for 13C pulses on dec1  
                              90 for part a of the sequence at 43 ppm */
             dvhpwra,        /* power level for 180 13C pulses at 43 ppm */
             dhpwrb,        /* power level for 13C pulses on dec1 - 54 ppm
                               90  for part b of the sequence*/
             dvhpwrb,        /* power level for 13C pulses on dec1 - 54 ppm
                               180 for part b of the sequence     */
             dhpwr2,       /* high dec2 pwr for 15N hard pulses    */
             sw1,          /* sweep width in f1                    */             
             sw2,          /* sweep width in f2                    */             
             dofcacb,      /* dof for dipsi part, 43  ppm            */      
             pwcadec,     /* seduce ca decoupling at dpwrsed        */
             dpwrsed,     /* power level for seduce ca decoupling   */
             dressed,     /* resoln for seduce decoupling  = 2      */
             dhpwrcoa,    /* power level for pwco180a, 180 shaped C'  */
             sphase1,     /* phase shift for off resonance C' 180  */
             pwN, pwNlvl,      /* N-15 RF calibration parameters */
             pwC, compC, pwClvl,      /* C-13 RF calibration parameters */
             compH,waltzB1,
             BigT1,     
             gt1,
             gt2,
             gt4,
             gt5,
             gt6,
             gt7,
             gt9,
             gt10,
             gstab,
             gzlvl1,
             gzlvl2,
             gzlvl4,
             gzlvl5,
             gzlvl6,
             gzlvl7,
             gzlvl9,
             gzlvl10;
           


/*  variables commented out are already defined by the system      */


/* LOAD VARIABLES */


  getstr("autocal",autocal);
  getstr("fsat",fsat);
  getstr("f1180",f1180);
  getstr("f2180",f2180);
  getstr("fscuba",fscuba);

  taua   = getval("taua"); 
  tauc   = getval("tauc"); 
  taud   = getval("taud");
  taue   = getval("taue");
  tauf   = getval("tauf");
  BigTC  = getval("BigTC");
  BigTN  = getval("BigTN");
  pwN = getval("pwN");
  pwNlvl = getval("pwNlvl");
  pwn = getval("pwn");
  tpwr = getval("tpwr");
  tsatpwr = getval("tsatpwr");
  tpwrml  = getval("tpwrml");
  dpwr = getval("dpwr");
  dhpwr2 = getval("dhpwr2");
  phase = (int) ( getval("phase") + 0.5);
  phase2 = (int) ( getval("phase2") + 0.5);
  sw1 = getval("sw1");
  sw2 = getval("sw2");
  dofcacb = getval("dofcacb");
  ni = getval("ni");
  ni2 = getval("ni2");
  BigT1 = getval("BigT1");
  sphase1 = getval("sphase1");
  gt1 = getval("gt1");
  gt2 = getval("gt2");
  gt4 = getval("gt4");
  gt5 = getval("gt5");
  gt6 = getval("gt6");
  gt7 = getval("gt7");
  gt9 = getval("gt9");
  gt10 = getval("gt10");
  gstab = getval("gstab");
  gzlvl1 = getval("gzlvl1");
  gzlvl2 = getval("gzlvl2");
  gzlvl4 = getval("gzlvl4");
  gzlvl5 = getval("gzlvl5");
  gzlvl6 = getval("gzlvl6");
  gzlvl7 = getval("gzlvl7");
  gzlvl9 = getval("gzlvl9");
  gzlvl10 = getval("gzlvl10");


  if(autocal[0]=='n')
  {
    getstr("spco180a",spco180a);
    getstr("spco90b",spco90b);
    getstr("spco180b",spco180b);
    getstr("cadecseq",cadecseq);    
    pwca90a = getval("pwca90a");
    pwca180a = getval("pwca180a");
    pwco180a = getval("pwco180a");
    pwca180b = getval("pwca180b");
    pwco90b = getval("pwco90b");
    pwco180b = getval("pwco180b"); 
    dhpwr = getval("dhpwr");
    dvhpwra = getval("dvhpwra");
    dhpwrb = getval("dhpwrb");
    dvhpwrb = getval("dvhpwrb");    
    dhpwrcoa = getval("dhpwrcoa");
    dpwrsed = getval("dpwrsed"); 
    dressed = getval("dressed");
    pwcadec = getval("pwcadec");    
    pwmlev = getval("pwmlev");
  }
  else
  {
    waltzB1=getval("waltzB1");
    pwmlev=1/(4.0*waltzB1);
    compH = getval("compH");
    tpwrml= tpwr - 20.0*log10(pwmlev/(compH*pw));
    tpwrml= (int) (tpwrml + 0.5);
    strcpy(spco180a,"Psed180_133p");
    strcpy(spco90b,"Phard90co_118p");
    strcpy(spco180b,"Phard180co_118p");
    strcpy(cadecseq,"Pseduce1_lek");
    if (FIRST_FID)
    {
      pwN = getval("pwN");
      pwNlvl = getval("pwNlvl");
      pwC = getval("pwC");
      pwClvl = getval("pwClvl");
      compC = getval("compC");
      ca90 = pbox("cal", CA90, "", dfrq, compC*pwC, pwClvl);
      ca180 = pbox("cal", CA180, "", dfrq, compC*pwC, pwClvl);      
      co180 = pbox(spco180a, CO180, CO180ps, dfrq, compC*pwC, pwClvl);
      co90b = pbox(spco90b, CO90b, CA180ps, dfrq, compC*pwC, pwClvl);
      co180b = pbox(spco180b, CO180b, CA180ps, dfrq, compC*pwC, pwClvl);
      ca180b = pbox("cal", CA180b, "", dfrq, compC*pwC, pwClvl);          
      cadec = pbox(cadecseq, CADEC, CADECps, dfrq, compC*pwC, pwClvl);
      w16 = pbox_dec("cal", "WALTZ16", tpwrml, sfrq, compH*pw, tpwr);
    }
    pwca90a = ca90.pw;       dhpwr = ca90.pwr;    
    pwca180a = ca180.pw;     dvhpwra = ca180.pwr;
    pwco180a = co180.pw;     dhpwrcoa = co180.pwr;
    pwco90b = co90b.pw;      dhpwrb = co90b.pwr;
    pwco180b = co180b.pw;    
    pwca180b = ca180b.pw;    dvhpwrb = ca180b.pwr;    
    pwcadec = 1.0/cadec.dmf; dpwrsed = cadec.pwr; dressed = cadec.dres;
    pwmlev = 1.0/w16.dmf;
    pwn=pwN; dhpwr2=pwNlvl;
  }   



/* LOAD PHASE TABLE */

  settable(t1,1,phi1);
  settable(t2,1,phi2);
  settable(t3,4,phi3);
  settable(t4,1,phi4);
  settable(t5,2,phi5);
  settable(t6,2,phi6);
  settable(t7,1,phi7);
  settable(t8,1,phi8);
  settable(t9,8,phi9);
  settable(t10,8,rec);
  settable(t11,1,phi11);
  settable(t12,1,phi12);
  settable(t13,1,phi13);

/* CHECK VALIDITY OF PARAMETER RANGES */

    if( 0.5*ni*1/(sw1) > BigTC - gt10 )
    {
        printf(" ni is too big\n");
        psg_abort(1);
    }

    if( ni2*1/(sw2) > 2.0*BigTN )
    {
        printf(" ni2 is too big\n");
        psg_abort(1);
    }

    if((dm[A] == 'y' || dm[B] == 'y' || dm[C] == 'y'))
    {
        printf("incorrect dec1 decoupler flags!  ");
        psg_abort(1);
    }

    if((dm2[A] == 'y' || dm2[B] == 'y' ))
    {
        printf("incorrect dec2 decoupler flags! Should be 'nny' ");
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

    if( dhpwr > 62 )
    {
        printf("don't fry the probe, DHPWR too large!  ");
        psg_abort(1);
    }

    if( dhpwrb > 62 )
    {
        printf("don't fry the probe, DHPWRB too large!  ");
        psg_abort(1);
    }

    if( dvhpwrb > 62 )   /* pwr level for dipsi  */
    {
        printf("don't fry the probe, DVHPWRB too large!  ");
        psg_abort(1);
    }

    if( dhpwr2 > 62 )
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
    if( pwn > 200.0e-6 )
    {
        printf("dont fry the probe, pwn too high ! ");
        psg_abort(1);
    } 


    if( pwcadec > 500.0e-6 || pwcadec < 200.0e-6 )
    {
        printf("pwcadec outside reasonable limits: < 500e-6 > 200e-6 \n");
        psg_abort(1);
    }

    if( dpwrsed > 45 )
    {
        printf("dpwrsed is too high\n");
        psg_abort(1);
    }

    if( gt1 > 15e-3 || gt2 > 15e-3 || gt4 >=15e-3 || gt5 > 15e-3 || gt6 >= 15e-3 || gt7 >= 15e-3 || gt9 >= 15e-3 || gt10 >= 15e-3) 
    {
        printf("all gti values must be < 15e-3\n");
        psg_abort(1);
    }

    if(gt10 > 250.0e-6) {
        printf("gt10 must be 250e-6\n");
        psg_abort(1);
    }


/*  Phase incrementation for hypercomplex 2D data */

    if (phase == 2)
      tsadd(t3,1,4);  
    if (phase2 == 2) {
      tsadd(t11,2,4);   
      icosel = 1;
    }
    else icosel = -1; 

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
        tau2 += ( 1.0 / (2.0*sw2) ); 
        if(tau2 < 0.2e-6) tau2 = 0.0;
    }
        tau2 = tau2/2.0;

/* Calculate modifications to phases for States-TPPI acquisition          */

   if( ix == 1) d2_init = d2 ;
   t1_counter = (int) ( (d2-d2_init)*sw1 + 0.5 );
   if(t1_counter % 2) {
      tsadd(t3,2,4);     
      tsadd(t10,2,4);    
    }

   if( ix == 1) d3_init = d3 ;
   t2_counter = (int) ( (d3-d3_init)*sw2 + 0.5 );
   if(t2_counter % 2) {
      tsadd(t8,2,4);  
      tsadd(t10,2,4);    
    }

/* BEGIN ACTUAL PULSE SEQUENCE */


status(A);
   decoffset(dofcacb);       /* initially pulse at 43 ppm */
   decpower(dhpwr);        /* Set Dec1 power for hard 13C pulses         */
   dec2power(dhpwr2);      /* Set Dec2 power for 15N hard pulses         */

/* Presaturation Period */
   if (fsat[A] == 'y')
   {
        obspower(tsatpwr);      /* Set transmitter power for 1H presaturation */
	delay(2.0e-5);
        rgpulse(d1,zero,rof1,rof1);
   	obspower(tpwr);      /* Set transmitter power for hard 1H pulses */
	delay(2.0e-5);
	if(fscuba[A] == 'y')
	{
		delay(2.2e-2);
		rgpulse(pw,zero,2.0e-6,0.0);
		rgpulse(2*pw,one,2.0e-6,0.0);
		rgpulse(pw,zero,2.0e-6,0.0);
		delay(2.2e-2);
	}
   }
   else
   obspower(tpwr);           /* Set transmitter power for hard 1H pulses */
   delay(d1);
   txphase(zero);
   delay(1.0e-5);

/* Begin Pulses */

status(B);

   rcvroff();
   delay(20.0e-6);


/* ensure that magnetization originates on H and not 13C  */
   decrgpulse(pwca90a,zero,0.0,0.0);

   delay(0.2e-6);
   zgradpulse(gzlvl6,gt6);
   delay(gstab);

   rgpulse(pw,zero,0.0,0.0);                    /* 90 deg 1H pulse */
   decphase(t1);

   delay(0.2e-6);
   zgradpulse(gzlvl9,gt9);
   delay(2.0e-6);

   decpower(dvhpwra);
   delay(taua - POWER_DELAY - gt2 - 2.2e-6);   /* taua <= 1/4JCH */                          
   simpulse(2*pw,pwca180a,zero,t1,0.0,0.0);
   decpower(dhpwr);

   delay(0.2e-6);
   zgradpulse(gzlvl9,gt9);
   delay(2.0e-6);

   txphase(t2); decphase(t3);
   delay(taua - POWER_DELAY - gt2 - 2.2e-6); 

   rgpulse(pw,t2,0.0,0.0);

   decrgpulse(pwca90a,t3,2.0e-6,0.0);

   delay(2.0e-6);

     delay(tau1);

     decpower(dhpwrcoa);
     decshaped_pulse(spco180a,pwco180a,zero,4.0e-6,0.0);

     dec2rgpulse(2*pwn,zero,0.0,0.0);

     delay(2.0e-6);
     zgradpulse(gzlvl10,gt10);
     delay(2.0e-6);

     decpower(dvhpwra);

     delay(0.80e-3 - gt10 - 4.0e-6 - 2*POWER_DELAY);
     delay(0.2e-6);
     
     rgpulse(2*pw,zero,0.0,0.0);

     decphase(t4);

     initval(1.0,v3);
     decstepsize(0.0);
     dcplrphase(v3);

     delay(BigTC - 0.80e-3);

     decrgpulse(pwca180a,t4,0.0,0.0);

     dcplrphase(zero);

     delay(2.0e-6);
     zgradpulse(gzlvl10,gt10);
     delay(2.0e-6);

     delay(BigTC - tau1 + 2*pwn + 2*pw - 2*POWER_DELAY - gt10 - 4.0e-6);
     delay(0.2e-6);

     decpower(dhpwrcoa);
     decshaped_pulse(spco180a,pwco180a,zero,4.0e-6,0.0); /* bloch seigert */
     decpower(dhpwr);

   decrgpulse(pwca90a,zero,2.0e-6,0.0);

     txphase(one); delay(2.0e-6);

     /* H decoupling on */
     obspower(tpwrml);
     obsprgon("waltz16",pwmlev,90.0);
     xmtron();    /* TURN ME OFF  DONT FORGET  */
     /* H decoupling on */

   decpower(dhpwrcoa);
   decshaped_pulse(spco180a,pwco180a,zero,4.0e-6,0.0); /* bloch seigert */
   decphase(t5);

   initval(1.0,v3);
   decstepsize(0.0);
   dcplrphase(v3);
 
   delay(tauc - 3*POWER_DELAY - PRG_START_DELAY);
   
   decpower(dvhpwra);
   decrgpulse(pwca180a,t5,0.0,0.0);

   dcplrphase(zero);

   decpower(dhpwrcoa);
   decshaped_pulse(spco180a,pwco180a,zero,4.0e-6,0.0); 
   decphase(zero);
   decpower(dhpwr);
   delay(tauc - 2*POWER_DELAY);

   decrgpulse(pwca90a,zero,0.0,0.0);

   /* H decoupling off */
   xmtroff();
   obsprgoff();
   /* H decoupling off */

   rgpulse(pwmlev,two,2.0e-6,0.0);

   delay(0.2e-6);
   decoffset(dof);
   decpower(dhpwrb);

   delay(0.2e-6);
   zgradpulse(gzlvl4,gt4);
   delay(gstab);

   rgpulse(pwmlev,zero,2.0e-6,0.0);
   txphase(one); delay(2.0e-6);

   /* H decoupling on */
   obspower(tpwrml);
   obsprgon("waltz16",pwmlev,90.0);
   xmtron();
   /* H decoupling on */

   decphase(t6);
   delay(2.0e-6);
   decshaped_pulse(spco90b,pwco90b,t6,0.0,0.0);

   decphase(zero);
   delay(taud - POWER_DELAY - 4.0e-6);

   decpower(dvhpwrb);
   decrgpulse(pwca180b,zero,4.0e-6,0.0);
   decpower(dhpwrb);
   delay(taue - taud - POWER_DELAY + 2*pwn);

   /* adjust phase */
   initval(1.0,v2);
   decstepsize(sphase1);
   dcplrphase(v2);
   /* adjust phase */
    
   decshaped_pulse(spco180b,pwco180b,zero,0.0,0.0);
   dcplrphase(zero);
   
   dec2rgpulse(2*pwn,zero,0.0,0.0);
   delay(taue - 2*POWER_DELAY - 4.0e-6 - 4.0e-6);    

   decpower(dvhpwrb);
   decrgpulse(pwca180b,zero,4.0e-6,0.0); /* bloch seigert */
   decpower(dhpwrb);

   decshaped_pulse(spco90b,pwco90b,t7,4.0e-6,0.0);

   /* H decoupling off */
   xmtroff();
   obsprgoff();
   /* H decoupling off */

   rgpulse(pwmlev,two,2.0e-6,0.0);

   delay(0.2e-6);
   zgradpulse(gzlvl5,gt5);
   delay(gstab);

   rgpulse(pwmlev,zero,2.0e-6,0.0);

   txphase(one); delay(2.0e-6);

   /* H decoupling on */
   obspower(tpwrml);
   obsprgon("waltz16",pwmlev,90.0);
   xmtron();
   /* H decoupling on */

   dec2rgpulse(pwn,t8,2.0e-6,0.0);
   dec2phase(t9); decphase(zero);

   /* seduce on */

   decpower(dpwrsed);
   decprgon(cadecseq,pwcadec,dressed);
   decon();
   /* seduce on */

   delay(BigTN - tau2 + WFG_START_DELAY + WFG_STOP_DELAY + pwco180b);


  /* seduce off */
  decoff();
  decprgoff();
  
  decpower(dhpwrb);
  /* seduce off */

  dec2rgpulse(2*pwn,t9,0.0,0.0);
  decshaped_pulse(spco180b,pwco180b,zero,0.0,0.0);

  dec2phase(t11);

   /* seduce on */

   decpower(dpwrsed);
   decprgon(cadecseq,pwcadec,dressed);
   decon();
   /* seduce on */

  delay(BigTN + tau2 - 5.5e-3 - POWER_DELAY - PRG_STOP_DELAY - pwmlev - 2.0e-6);

   /* H decoupling off */ 
   xmtroff();
   obsprgoff();
   /* H decoupling off */

   rgpulse(pwmlev,two,2.0e-6,0.0);    
   obspower(tpwr);

   delay(2.5e-3);

  /* seduce off */
  decoff();
  decprgoff();
  decpower(dhpwrb);
  /* seduce off */

   delay(0.2e-6);
   zgradpulse(gzlvl1,gt1);
   delay(2.0e-6);
   
   txphase(zero);
   dec2phase(t11);
   delay(3.0e-3 - gt1 - 2.2e-6 - 2.0*GRADIENT_DELAY);
  
   sim3pulse(pw,0.0,pwn,zero,zero,t11,0.0,0.0);

   delay(0.2e-6);
   zgradpulse(gzlvl7,gt7);
   delay(2.0e-6);

   dec2phase(zero);
   delay(tauf - gt7 - 2.2e-6);

   sim3pulse(2*pw,0.0,2*pwn,zero,zero,zero,0.0,0.0);

   delay(0.2e-6);
   zgradpulse(gzlvl7,gt7);
   delay(2.0e-6);
   
   txphase(t12);
   dec2phase(t13);
   delay(tauf - gt7 - 2.2e-6);

   sim3pulse(pw,0.0,pwn,t12,zero,t13,0.0,0.0);
   
   delay(0.2e-6);
   zgradpulse(gzlvl7,gt7);
   delay(2.0e-6);
 
   txphase(zero);
   dec2phase(zero);
   delay(tauf - gt7 - 2.2e-6);
   sim3pulse(2*pw,0.0,2*pwn,zero,zero,zero,0.0,0.0);

  
   delay(0.2e-6);
   zgradpulse(gzlvl7,gt7);
   delay(2.0e-6);

   txphase(zero);
   delay(tauf - gt7 - 2.2e-6);
   
   rgpulse(pw,zero,0.0,0.0);

   txphase(zero);

   delay(BigT1);

   rgpulse(2*pw,zero,0.0,0.0);

   delay(2.0e-6);
   zgradpulse(icosel*gzlvl2,gt1/10);
   delay(2.0e-6);

   delay(BigT1 - gt1/10 - POWER_DELAY - 4.0e-6 - 2*GRADIENT_DELAY);

   dec2power(dpwr2);  /* set power for 15N decoupling */
    

/* BEGIN ACQUISITION */

status(C);
         setreceiver(t10);

}
