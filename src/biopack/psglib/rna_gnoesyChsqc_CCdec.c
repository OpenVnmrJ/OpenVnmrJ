/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*  rna_gnoesyChsqc_CCdec.c

    3D C13 edited noesy with separation via the carbon of the destination site
         recorded on a water sample  and bandselective decoupling during t2

                      NOTE dof MUST BE SET AT 110ppm ALWAYS
                      NOTE dof2 MUST BE SET AT 200ppm ALWAYS

    Set dm = 'nnny', [13C decoupling during acquisition].
    Set dm2 = 'nyny', [15N dec during t1 and acq] 

    Must set phase = 1,2 and phase2 = 1,2 for States-TPPI
    acquisition in t1 [H]  and t2 [C].

    Set f1180 = 'y' and f2180 = 'y' for (-90,180) in F1 and (-90,180) in  F2.    
     
    dof is changed automatically to 85ppm. This folds aromatic carbons over.
    Set sw2=60ppm.
    dof2 is set to 200ppm (middle of aromatic N15 resonances).

    Use the VnmrJ button to run the macro BPrna_CCdec. This will create the 
    necessary waveforms and set their parameters (set all other parameters
    before doing this).

    For Carbon-Carbon filtering, run 2 experiments with the two different
    filters. Subtract data to obtain purine-pyrimidine separation.

    Always check the proper folding by running a quick CH plane by setting 
    ni=0,phase=1 and ni2=32,phase=1,2.

    Pulse Sequence: Kwaku Dayie, Cleveland Clinic, original: ktdrna_gnoesyChsqc_dec2sel_II.c
       See Dayie, J BioMol NMR, 2005, 32, 129-39.
       Modified for BioPack, GG, Varian, 1/2008
*/

#include <standard.h>

/* Chess - CHEmical Shift Selective Suppression */
void Chess(pulsepower,pulseshape,duration,phase,rx1,rx2,gzlvlw,gtw,gswet)
double pulsepower,duration,rx1,rx2,gzlvlw,gtw,gswet;
  codeint phase;
  char* pulseshape;
{
  obspwrf(pulsepower);
  shaped_pulse(pulseshape,duration,phase,rx1,rx2);
  zgradpulse(gzlvlw,gtw);
  delay(gswet);
}

/* Wet4 - Water Elimination */
void Wet4(phaseA,phaseB)
  codeint phaseA,phaseB;
{
  double finepwr,gzlvlw,gtw,gswet,gswet2,wetpwr,wetpw,dz;
  char   wetshape[MAXSTR];
  getstr("wetshape",wetshape);    /* Selective pulse shape (base)  */
  wetpwr=getval("wetpwr");        /* User enters power for 90 deg. */
  wetpw=getval("wetpw");        /* User enters power for 90 deg. */
  dz=getval("dz");
  finepwr=wetpwr-(int)wetpwr;     /* Adjust power to 152 deg. pulse*/
  wetpwr=(double)((int)wetpwr);
  if (finepwr==0.0) {wetpwr=wetpwr+5; finepwr=4095.0; }
  else {wetpwr=wetpwr+6; finepwr=4095.0*(1-((1.0-finepwr)*0.12)); }
  rcvroff();
  obspower(wetpwr);         /* Set to low power level        */
  gzlvlw=getval("gzlvlw");      /* Z-Gradient level              */
  gtw=getval("gtw");            /* Z-Gradient duration           */
  gswet=getval("gswet");        /* Post-gradient stability delay */
  gswet2=getval("gswet2");        /* Post-gradient stability delay */
  Chess(finepwr*0.5059,wetshape,wetpw,phaseA,20.0e-6,rof2,gzlvlw,gtw,gswet);
  Chess(finepwr*0.6298,wetshape,wetpw,phaseB,20.0e-6,rof2,gzlvlw/2.0,gtw,gswet);
  Chess(finepwr*0.4304,wetshape,wetpw,phaseB,20.0e-6,rof2,gzlvlw/4.0,gtw,gswet);
  Chess(finepwr*1.00,wetshape,wetpw,phaseB,20.0e-6,rof2,gzlvlw/8.0,gtw,gswet2);
  obspower(tpwr); obspwrf(tpwrf);     /* Reset to normal power level   */
  rcvron();
  delay(dz);
}
static int  phi1[8]  = {0,0,0,0,2,2,2,2},
            phi2[16] = {0,0,0,0,0,0,0,0,2,2,2,2,2,2,2,2},
            phi3[8]  = {0,0,0,0,2,2,2,2},
            rec[16]  = {0,3,2,1,2,1,0,3,2,1,0,3,0,3,2,1},
            phi5[4]  = {0,1,2,3},
            phi6[2]  = {0,2},
            phi7[4]  = {1,1,3,3};
                    
static double d2_init=0.0, d3_init=0.0;

void pulsesequence()
{
/* DECLARE VARIABLES */

 char     
            f1180[MAXSTR],    /* Flag to start t1 @ halfdwell             */
            Aromatic[MAXSTR], /* Aromatic region    */
            Ribose[MAXSTR], /* Ribose region    */
            Both[MAXSTR], /* Ribose and aromatic region    */
	    CCdseq[MAXSTR],
            C1pC5pSel[MAXSTR],
            pwC1p_c5p_shape[MAXSTR],
            CCfdseq[MAXSTR],
            CCf2dseq[MAXSTR],
            CCfilter[MAXSTR],
            CCfilter2[MAXSTR],
            mag_flg[MAXSTR],  /* magic angle gradient                     */
            f2180[MAXSTR],    /* Flag to start t2 @ halfdwell             */
            wet[MAXSTR],      /* Flag to select optional WET water suppression */
            STUD[MAXSTR],     /* Flag to select adiabatic decoupling      */
            rna_stCdec[MAXSTR];   /* contains name of adiabatic decoupling shape */

 int         
             t1_counter,   /* used for states tppi in t1           */ 
             t2_counter;   /* used for states tppi in t2           */ 

double    
        CCdpwr = getval("CCdpwr"),    /*   power level for CC decoupling */
        CCdres = getval("CCdres"),    /*   dres for CC decoupling */
        CCdmf = getval("CCdmf"),      /*   dmf for CC decoupling */
        CCfdpwr = getval("CCfdpwr"),    /*   power level for CC filtering */
        CCfdres = getval("CCfdres"),    /*   dres for CC filtering */
        CCfdmf = getval("CCfdmf"),      /*   dmf for CC filtering */
        CCf2dpwr = getval("CCf2dpwr"),    /*   power level for CC filtering (no C5) */
        CCf2dres = getval("CCf2dres"),    /*   dres for CC filtering (no C5) */
        CCf2dmf = getval("CCf2dmf"),      /*   dmf for CC filtering (no C5) */

        pwC1p_c5p_pwr = getval("pwC1p_c5p_pwr"),  /*power level for C1'C5' selective pulse */
        pwC1p_c5p_pw = getval("pwC1p_c5p_pw"),    /*length for C1'C5' selective pulse */

            stdmf = getval("dmf140"),     /* dmf for 140 ppm of STUD decoupling */
            rf140 = getval("rf140"),       	  /* rf in Hz for 140ppm STUD+ */
            rfst,                        /* fine power level for adiabatic pulse */
            studlvl,	                         /* coarse power for STUD+ decoupling */
            rfC,                        /* full fine power */
            compC = getval("compC"),         /* adjustment for C13 amplifier compression */
             tau1,         /*  t1 delay */
             tau2,         /*  t2 delay */
             corr,         /*  correction for t2  */
             JCH,          /*  CH coupling constant */

            fdelay = 1.0/(2*getval("JC5C6")),     /* filter delay 1/2JCC */

             pwN,          /* PW90 for 15N pulse              */
             pwC,          /* PW90 for c nucleus @ pwClvl         */
             pwC180,       /* PW180 for c nucleus in INEPT transfers */
             pwClvl,        /* power level for 13C pulses on dec1  */
             pwNlvl,       /* high dec2 pwr for 15N hard pulses    */
             mix,          /* noesy mix time                       */
             sw1,          /* spectral width in t1 (H)             */
             sw2,          /* spectral width in t2 (C) (3D only)   */
             gt0,
             gt3,
             dofa,
             tofa,
             gt4,
             gt5,
             gt7,
             gt8,
             gzcal,        /* dac to G/cm conversion factor */
	     gstab,
             gzlvl0, 
             gzlvl3, 
             gzlvl4, 
             gzlvl5,
             gzlvl7, 
             gzlvl8;


/* LOAD VARIABLES */

  getstr("Ribose",Ribose);
  getstr("Both",Both);
  getstr("Aromatic",Aromatic);
  getstr("pwC1p_c5p_shape",pwC1p_c5p_shape);
  getstr("C1pC5pSel",C1pC5pSel);
  getstr("wet",wet);
  getstr("mag_flg",mag_flg);
  getstr("f1180",f1180);
  getstr("f2180",f2180);
  getstr("CCdseq",CCdseq);
  getstr("CCfdseq",CCfdseq);
  getstr("CCf2dseq",CCf2dseq);
  getstr("CCfilter",CCfilter);
  getstr("CCfilter2",CCfilter2);

  getstr("STUD",STUD);

    mix  = getval("mix");
    sw1  = getval("sw1");
    sw2  = getval("sw2");
  JCH = getval("JCH"); 
  pwC = getval("pwC");
  pwC180 = getval("pwC180");  
  pwN = getval("pwN");
  pwClvl = getval("pwClvl");
  pwNlvl = getval("pwNlvl");
  gstab = getval("gstab");
  gzcal = getval("gzcal");
  gt0 = getval("gt0");
  gt3 = getval("gt3");
  gt4 = getval("gt4");
  gt5 = getval("gt5");
  gt7 = getval("gt7");
  gt8 = getval("gt8");
  gzlvl0 = getval("gzlvl0");
  gzlvl3 = getval("gzlvl3");
  gzlvl4 = getval("gzlvl4");
  gzlvl5 = getval("gzlvl5");
  gzlvl7 = getval("gzlvl7");
  gzlvl8 = getval("gzlvl8");

/* LOAD PHASE TABLE */
  settable(t1,8,phi1);
  settable(t2,16,phi2);
  settable(t3,8,phi3);
  settable(t4,16,rec);
  settable(t5,4,phi5);
  settable(t6,2,phi6);
  settable(t7,4,phi7);

/* CHECK VALIDITY OF PARAMETER RANGES */

    if((dm[A] == 'y' || dm[C] == 'y' ))
    {
        printf("incorrect 13C decoupler flags! dm='nnnn' or 'nnny' only  ");
        psg_abort(1);
    }

    if((Aromatic[A] == 'y' && Ribose[A] == 'y' && Both[A] == 'y' ))
    {
        printf("incorrect  flags! Choose either Aromatic or Ribose ");
        psg_abort(1);
    }

    if((Aromatic[A] == 'y' && Ribose[A] == 'y' ))
    {
        printf("incorrect  flags! Choose either Aromatic or Ribose ");
        psg_abort(1);
    }

    if((Aromatic[A] == 'y' && Both[A] == 'y' ))
    {
        printf("incorrect  flags! Choose either Aromatic or Ribose ");
        psg_abort(1);
    }

    if((Ribose[A] == 'y' && Both[A] == 'y' ))
    {
        printf("incorrect  flags! Choose either Aromatic or Ribose ");
        psg_abort(1);
    }

    if((dm2[A] == 'y' || dm2[C] == 'y' ))
    {
        printf("incorrect 15N decoupler flags! No decoupling in relax or mix periods  ");
        psg_abort(1);
    }

    if( dpwr > 49 )
    {
        printf("don't fry the probe, DPWR too large!  ");
        psg_abort(1);
    }

    if( dpwr2 > 49 )
    {
        printf("don't fry the probe, DPWR2 too large!  ");
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

    if( gt0 > 15e-3 || gt7 > 15e-3 || gt8 > 15e-3 || gt3 > 15e-3 || gt4 > 15e-3 || gt5 > 15e-3 || gt7 > 15e-3 ) 
    {
        printf("gti values < 15e-3\n");
        psg_abort(1);
    } 

   if( gzlvl3*gzlvl4 > 0.0 ) 
    {
        printf("gt3 and gt4 must be of opposite sign for optimal water suppression\n");
        psg_abort(1);
     }

/*  Phase incrementation for hypercomplex 2D data */

    if (phase1 == 2)
      tsadd(t1,1,4);
    if (phase2 == 2)
      tsadd(t2,1,4);

/*  Set up f1180  tau1 = t1               */
   
    tau1 = d2;
    if(f1180[A] == 'y') {
        tau1 += ( 1.0 / (2.0*sw1) - 4.0/PI*pw - 4.0*pwC - 6*rof1);
    }

    else tau1 = tau1 - (4.0/PI*pw + 4.0*pwC + 6*rof1);

    if(tau1 < 0.2e-6) tau1 = 4.0e-7;

    tau1 = tau1/2.0;

/*  Set up f2180  tau2 = t2               */
    corr = (4.0/PI)*pwC + WFG2_START_DELAY +4*rof1;

    tau2 = d3;
    if(f2180[A] == 'y')
     {
      tau2 += ( 1.0 / (2.0*sw2) -corr); 
     }

    else
     tau2 = tau2 - corr; 
     tau2 = tau2/2.0;

/* Calculate modifications to phases for States-TPPI acquisition          */

   if( ix == 1) d2_init = d2 ;
   t1_counter = (int) ( (d2-d2_init)*sw1 + 0.5 );
   if(t1_counter % 2) {
      tsadd(t1,2,4);     
      tsadd(t4,2,4);    
    }

   if( ix == 1) d3_init = d3 ;
   t2_counter = (int) ( (d3-d3_init)*sw2 + 0.5 );
   if(t2_counter % 2) {
      tsadd(t2,2,4);  
      tsadd(t4,2,4);    
    }

   dofa=dof; 

if (Ribose[A]=='y')
   dofa=dof-(30*dfrq); /*first noesy was ribose to base*/ 

if (Aromatic[A]=='y')
   dofa=dof+(35*dfrq);

if (Both[A]=='y')
   dofa=dof+(6*dfrq);

   tofa=tof+(1.32*sfrq); /*1.72 or 1.62 first noesy was ribose to base*/ 

   /* 140 ppm STUD+ decoupling */
	strcpy(rna_stCdec, "rna_stCdec140");
	studlvl = pwClvl + 20.0*log10(compC*pwC*4.0*rf140);
	studlvl = (int) (studlvl + 0.5);

        if (pwC180>3.0*pwC)   /*140ppm sech/tanh "rna_stC140" inversion pulse*/
         {
        rfst = (compC*4095.0*pwC*4000.0*sqrt((21.0*sfrq/600.0+7.0)/0.35));
        rfst = (int) (rfst + 0.5);
         }
        else rfst=4095.0;

    if( pwC > (20.0e-6*600.0/sfrq) )
	{ printf("Increase pwClvl so that pwC < 20*600/sfrq");
	  psg_abort(1); }

    /* maximum fine power for pwC pulses */
	rfC = 4095.0;

/* BEGIN ACTUAL PULSE SEQUENCE */


status(A);
   decoffset(dofa);
   decpower(pwClvl);  /* Set Dec1 power for hard 13C pulses         */
   dec2power(dpwr2);       /* Set Dec2 to low power for decoupling */
   delay(d1);

  if (wet[A] == 'y') {
     Wet4(zero,one);
  }

   obspower(tpwr);           /* Set transmitter power for hard 1H pulses */
   obsoffset(tofa);
   rcvroff();
status(B);
   rgpulse(pw,t1,1*rof1,0.0);                  /* 90 deg 1H pulse */
if (d2>0.0)
 {
   delay(tau1);
   decrgpulse(pwC,zero,0.0,0.0);            /* composite 180 on 13C */ 
   decrgpulse(2.0*pwC,one,0.0,0.0);
   decrgpulse(pwC,zero,0.0,0.0);
   delay(tau1);
 }
status(C);
   rgpulse(pw,zero,2*rof1,0.0);             /*  2nd 1H 90 pulse   */
   obsoffset(tof);
   decoffset(dofa);
   delay(mix - 10.0e-3);                    /*  mixing time     */
   zgradpulse(gzlvl0, gt0);
   decrgpulse(pwC,zero,gstab,2*rof1); 
   decpwrf(rfst);                           /* power for inversion pulse */
   dec2power(pwNlvl);
   zgradpulse(gzlvl7, gt7);
   delay(10.0e-3 - gt7 - gt0 - 8*rof1);
   rgpulse(pw,zero,0.0,2*rof1);
   zgradpulse(gzlvl8, gt8);       /* g3 in paper   */
   delay(1/(4.0*JCH) - gt8 - 2*rof1 -pwC180/2.0);
   if (pwC180>3.0*pwC)
    simshaped_pulse("","rna_stC140",2*pw,pwC180,zero,zero,0.0,2*rof1);
   else 
    simpulse(2.0*pw,2.0*pwC,zero,zero,0.0,2*rof1);
   decphase(zero);
   zgradpulse(gzlvl8, gt8);       /* g4 in paper  */
   decpwrf(rfC);
   delay(1/(4.0*JCH) - gt8 - 2*rof1 -pwC180/2.0);
   rgpulse(pw,one,1*rof1,2*rof1);

if (CCfilter[A]=='y')
{
       decrgpulse(pwC, zero, 0.0, 0.0);

        decpower(CCfdpwr); decphase(zero);
        decprgon(CCfdseq,1.0/CCfdmf,CCfdres);
        decon();  /* CC decoupling on */

        delay(fdelay);

        decoff(); decprgoff();        /* CC decoupling off */
        decpower(pwClvl);

        decrgpulse(2.0*pwC, zero, 0.0, 0.0);

        decpower(CCfdpwr); decphase(zero);
        decprgon(CCfdseq,1.0/CCfdmf,CCfdres);
        decon();  /* CC decoupling on */

        delay(fdelay);

        decoff(); decprgoff();        /* CC decoupling off */
        decpower(pwClvl);

        decrgpulse(pwC, zero, 0.0, 0.0);
}
if (CCfilter2[A]=='y')
{
        decrgpulse(pwC, zero, 0.0, 0.0);

        decpower(CCf2dpwr); decphase(zero);
        decprgon(CCf2dseq,1.0/CCf2dmf,CCfdres);
        decon();  /* CC decoupling on */

        delay(fdelay);

        decoff(); decprgoff();        /* CC decoupling off */
        decpower(pwClvl);

        decrgpulse(2.0*pwC, zero, 0.0, 0.0);

        decpower(CCf2dpwr); decphase(zero);
        decprgon(CCf2dseq,1.0/CCf2dmf,CCf2dres);
        decon();  /* CC decoupling on */

        delay(fdelay);

        decoff(); decprgoff();        /* CC decoupling off */
        decpower(pwClvl);

        decrgpulse(pwC, zero, 0.0, 0.0);
}

                if (mag_flg[A] == 'y')
                {
                   magradpulse(gzcal*gzlvl3, gt3);
                }
                else
                {
                   zgradpulse(gzlvl3, gt3);
                }
   dofa=dof-(17.5*dfrq); /*noesy ribose ribose to base*/ 
   decoffset(dofa);
   decrgpulse(pwC,t2,gstab,2*rof1);   


if (tau2 > (pwN + WFG2_START_DELAY + POWER_DELAY) )
           {decpower(CCdpwr); decphase(zero);
           decprgon(CCdseq,1.0/CCdmf,CCdres); decon();  /* CC decoupling on */
 
           delay(tau2- pwN - WFG2_START_DELAY - POWER_DELAY);
        sim3pulse(2.0*pw, 0.0, 2*pwN, zero, zero, zero, 0.0, 0.0);
            delay(tau2 - pwN - WFG2_START_DELAY - POWER_DELAY);
 
           decoff(); decprgoff();                      /* CC decoupling off */
           decpower(pwClvl);
           }
       else    {decpower(CCdpwr); decphase(zero);
                decprgon(CCdseq,1.0/CCdmf,CCdres); decon();  /* CC decoupling on */ 
		delay(2.0*tau2);
                decoff(); decprgoff();                      /* CC decoupling off */
                decpower(pwClvl);}
 


   decrgpulse(pwC,zero,2*rof1,2*rof1);
                if (mag_flg[A] == 'y')
                {
                   magradpulse(gzcal*gzlvl4, gt4);
                }
                else
                {
                   zgradpulse(gzlvl4, gt4);
                }
   txphase(t5);
   delay(gstab);
   rgpulse(pw,t5,0.0,0.0);
   decpwrf(rfst);
   dec2power(dpwr2);
   zgradpulse(gzlvl5, gt5);

if (C1pC5pSel[A]=='y')
        {
        decpower(pwC1p_c5p_pwr);
     	delay(1/(4.0*JCH) - gt5 -pwC1p_c5p_pw/2.0);
        simshaped_pulse("", pwC1p_c5p_shape, 2.0*pw,pwC1p_c5p_pw, zero, zero, 0.0, 0.0);
   decphase(zero);
        decpower(pwClvl);
     	zgradpulse(gzlvl5, gt5);
        txphase(t5);
     	delay(1/(4.0*JCH) - gt5 -pwC1p_c5p_pw/2.0 -2.0*pwC);

        }
else
{

   if (pwC180>3.0*pwC)
    {
     delay(1/(4.0*JCH) - gt5 -pwC180/2.0);
     simshaped_pulse("","rna_stC140",2*pw,pwC180,zero,zero,0.0,0.0);
   decphase(zero);
     decpwrf(rfC);
     zgradpulse(gzlvl5, gt5);
     txphase(t5);
     delay(1/(4.0*JCH) -  gt5 -pwC180/2.0 - 2.0*pwC);
    }
   else
    {
     delay(1/(4.0*JCH) - gt5 -pwC);
     simpulse(2.0*pw,2.0*pwC,zero,zero,0.0,0.0);
     decpwrf(rfC);
     zgradpulse(gzlvl5, gt5);
     txphase(t5);
     delay(1/(4.0*JCH) -  gt5 -pwC - 2.0*pwC);
    }
}
   decrgpulse(pwC,zero,0.0,0.0);     
   decrgpulse(pwC,t3,0.0,0.0);     
   rgpulse(pw,t5,0.0,rof2);                        /* flip-back pulse  */
   rcvron();
   setreceiver(t4);
   if ((STUD[A]=='y') && (dm[D] == 'y'))
    {
     decpower(studlvl);
     decprgon(rna_stCdec, 1.0/stdmf, 1.0);
     decon();
    }
   else	
    { 
     decpower(dpwr);
     status(D);
    }
}
