/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* ghca_nA.c

 Uses coherence selection and (optional) magic angle gradient pulses.

 3D hcan  correlates Ha, Ca and N15(i) and N15(i+1).
 Ref: R.Powers and A.Bax,  JMR, 94, 209-213(1991).

    Uses three channels:
         1)  1H   (t3)       - carrier  4.7 ppm (tof)
         2) 13CA  (t2, ni2)  - carrier  56 ppm  (dof)
         3) 15N   (t1, ni)   - carrier  120 ppm (dof2) 

    gzcal = gcal for z gradient (gauss/dac),   
    mag_flg = y,  using magic angle pulsed field gradient
                   uses gzcal (= 0.0022 gauss/dac, for example)   
             =n,  using z-axis gradient only

  Coded by Marco Tonelli and Klaas Hallenga, NMRFAM, starting from ghn_caA.


   The waltz16 field strength is enterable (waltzB1).
   Typical values would be ~6-8ppm, (3500-5000 Hz at 600 MHz).
  
   The coherence-transfer gradients using power levels
   gzlvl1and gzlvl2 may be either z or magic-angle gradients. For the
   latter, a proper /vnmr/imaging/gradtable entry must be present and
   syscoil must contain the value of this entry (name of gradtable). The
   amplitude of the gzlvl1 and gzlvl2 should be lower than for a z axis
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

   Coded by Marco Tonelli and Klaas Hallenga, NMRFAM, starting from ghn_caA.
 */

#include <standard.h> 
#include "Pbox_bio.h"               /* Pbox Bio Pack Pulse Shaping Utilities */



static int   phx[1]={0},   phy[1]={1},

	     phi3[2]  = {0,2},
	     phi5[4]  = {0,0,2,2},
	     phi9[8]  = {0,0,0,0,2,2,2,2},
             rec[4]   = {0,2,2,0};


static double   d2_init=0.0, d3_init=0.0;
static double   H1ofs=4.7, C13ofs=56.0, N15ofs=120.0, H2ofs=0.0;

static shape wz16, offC1, offC2, offC9;  /* Cbd; */

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
    corr,
    taua,         /*  ~ 1/4JCH =  1.5 ms - 1.7 ms]        */
    taub,         /*  ~ 3.3 ms       */
    timeTN,       /*  ~ 18 ms       */

    pwClvl,   /* High power level for carbon on channel 2 */
    pwC,      /* C13 90 degree pulse length at pwClvl     */

    compH,    /* Compression factor for H1 (at tpwr)      */
    compC,     /* Compression factor for C13 (at pwClvl)   */
    pwNlvl,   /* Power level for Nitrogen on channel 3    */
    pwN,      /* N15 90 degree pulse lenght at pwNlvl     */
    tpwrd,   /* Power level for proton decoupling on channel 1  */
    pwHd,     /* H1 90 degree pulse lenth at tpwrd.             */
    waltzB1 = getval("waltzB1"),  /* waltz16 field strength (in Hz)     */

    bw, ofs, ppm,                            /* temporary Pbox parameters */

/* 90 degree pulse at Ca (56ppm), first off-resonance null at CO (174ppm)     */
    pwC1,		              /* 90 degree pulse length on C13 at rf1 */
    rf1,		       /* fine power for 4.7 kHz rf for 600MHz magnet */

/* 180 degree pulse at Ca (56ppm), first off-resonance null at CO(174ppm)     */
    pwC2,		                    /* 180 degree pulse length at rf2 */
    rf2,		      /* fine power for 10.5 kHz rf for 600MHz magnet */

/* the following pulse lengths for SLP pulses are automatically calculated    */
/* by the macro "BPcal".  SLP pulse shapes, "offC9" etc are called       */
/* directly from your shapelib.                    			      */
   pwC9 = getval("pwC9"),  /*180 degree pulse at CO(174ppm) null at Ca(56ppm) */
   pwC9a,                      /* pwC9a=pwC9, but not set to zero when pwC9=0 */
   rf9,	                       /* fine power for the pwC9 ("offC9") pulse */

   gzcal = getval("gzcal"),  /* calbration factor for Z gradient          */

   gt1 = getval("gt1"),
   gzlvl1 = getval("gzlvl1"),
   gzlvl2 = getval("gzlvl2"),

   gt0 = getval("gt0"),
   gt3 = getval("gt3"),
   gt4 = getval("gt4"),
   gt5 = getval("gt5"),
   gstab = getval("gstab"),
 
   gzlvl0 = getval("gzlvl0"),
   gzlvl3 = getval("gzlvl3"),
   gzlvl4 = getval("gzlvl4"),
   gzlvl5 = getval("gzlvl5"),

   rf0 = 4095.0;

         
/* LOAD VARIABLES */

    getstr("f1180",f1180);
    getstr("f2180",f2180);
    getstr("mag_flg", mag_flg);

    taua   = getval("taua"); 
    taub   = getval("taub");
    timeTN = getval("timeTN");
  
    pwClvl = getval("pwClvl");
    pwC = getval("pwC");
    compH= getval("compH");
    compC = getval("compC");

    pwNlvl = getval("pwNlvl");
    pwN = getval("pwN");

    sw1 = getval("sw1");
    sw2 = getval("sw2");
    ni = getval("ni");
    ni2 = getval("ni2");
    

/*   LOAD PHASE TABLE    */

	settable(t3,2,phi3);
	settable(t4,1,phx);
	settable(t5,4,phi5);
        settable(t8,1,phx);
	settable(t9,8,phi9);
	settable(t10,1,phx);
	settable(t11,1,phy);
	settable(t12,4,rec);


/* CHECK VALIDITY OF PARAMETER RANGES */

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

    if( dpwr > 47 )
    {
        printf("don't fry the probe, DPWR too large!  ");
        psg_abort(1);
    }

      setautocal();                        /* activate auto-calibration flags */ 
        

      if (autocal[0] == 'n') 
      {
    /* 90 degree pulse on Ca, null at CO 118ppm away */
        pwC1 = sqrt(15.0)/(4.0*118.0*dfrq);
        rf1 = 4095.0*(compC*pwC)/pwC1;
        rf1 = (int) (rf1 + 0.5);

    /* 180 degree pulse on Ca, null at CO 118ppm away */
        pwC2 = sqrt(3.0)/(2.0*118.0*dfrq);
        rf2 = (4095.0*compC*pwC*2.0)/pwC2;
        rf2 = (int) (rf2 + 0.5);
        if( rf2 > 4095.0 )
         { printf("increase pwClvl so pwC<24us*600/sfrq"); psg_abort(1);}

    /* 180 degree one-lobe sinc pulse on CO, null at Ca 118ppm away */
        pwC9a = getval("pwC9a");
        rf9 = (compC*4095.0*pwC*2.0*1.65)/pwC9a; /* needs 1.65 times more     */
        rf9 = (int) (rf9 + 0.5);                 /* power than a square pulse */

    /* power level and pulse time for WALTZ 1H decoupling */
	pwHd = 1/(4.0 * waltzB1) ;                              /* 7.5 kHz rf   */
	tpwrd = tpwr - 20.0*log10(pwHd/(compH*pw));
	tpwrd = (int) (tpwrd + 0.5);
      }
      else  
      {
        if(FIRST_FID)                                         /* call Pbox */
        {
          ppm = getval("dfrq"); 
          bw = 118.0*ppm; ofs = -bw;           
          offC1 = pbox_Rcal("square90n", bw, compC*pwC, pwClvl);
          offC2 = pbox_Rcal("square180n", bw, compC*pwC, pwClvl);
          offC9 = pbox_make("offC9", "sinc180n", bw, -ofs, compC*pwC, pwClvl);
          wz16 = pbox_Dcal("WALTZ16", 2.8*waltzB1, 0.0, compH*pw, tpwr);
          if (dm3[B] == 'y') H2ofs = 3.2;     
          ofs_check(H1ofs, C13ofs, N15ofs, H2ofs);

        }
        pwC1 = offC1.pw; rf1 = offC1.pwrf;
        pwC2 = offC2.pw; rf2 = offC2.pwrf;
        pwC9a = offC9.pw; rf9 = offC9.pwrf;
        tpwrd = wz16.pwr; pwHd = 1.0/wz16.dmf;
 
      }	


/* PHASES AND INCREMENTED TIMES */

/*  Phase incrementation for hypercomplex 2D data, States-Haberkorn element */

    if (phase1 == 2)   tsadd(t3,1,4);  
    if (phase2 == 2)  {tsadd(t10,2,4); icosel = +1;}
      else 			       icosel = -1;    


/*  Set up f1180  */
   
    tau1 = d2;
    if((f1180[A] == 'y') && (ni > 1.0)) 
	{ tau1 += ( 1.0 / (2.0*sw1) ); if(tau1 < 0.2e-6) tau1 = 0.0; }
    tau1 = tau1/2.0;


/*  Set up f2180  */

    tau2 = d3;
    if((f2180[A] == 'y') && (ni2 > 1.0)) 
	{ tau2 += ( 1.0 / (2.0*sw2) ); if(tau2 < 0.2e-6) tau2 = 0.0; }
    tau2 = tau2/2.0;



/* Calculate modifications to phases for States-TPPI acquisition          */

   if( ix == 1) d2_init = d2;
   t1_counter = (int) ( (d2-d2_init)*sw1 + 0.5 );
   if(t1_counter % 2) 
	{ tsadd(t3,2,4); tsadd(t12,2,4); }

   if( ix == 1) d3_init = d3;
   t2_counter = (int) ( (d3-d3_init)*sw2 + 0.5 );
   if(t2_counter % 2) 
	{ tsadd(t8,2,4); tsadd(t12,2,4); }




/* CHECK VALIDITY OF PARAMETER RANGES */

    if ( 0.5*ni2*1/(sw2) > (timeTN -SAPS_DELAY -PWRF_DELAY))
       { text_error(" ni2 is too big. Make ni2 equal to %d or less.\n",
         ((int)((timeTN -SAPS_DELAY -PWRF_DELAY) *2.0*sw2))); psg_abort(1);}

    if (gt1 > 0.5*(taub -2.0*gstab -pwC9a -WFG2_START_DELAY -2.0*GRADIENT_DELAY))
       { text_error(" Make gt1 equal or less than %.5f. Recalibrate gzlvl2.\n",
         (0.5*(taub -2.0*gstab -pwC9a -WFG2_START_DELAY -2.0*GRADIENT_DELAY)));    
	psg_abort(1);}

    if ( dm[A] == 'y' || dm[B] == 'y' )
       { text_error("incorrect dec1 decoupler flags! Should be 'nny' "); psg_abort(1);}

    if ( dm2[A] == 'y' || dm2[B] == 'y' || dm2[C] == 'y')
       { text_error("incorrect dec2 decoupler flags! Should be 'nnn' "); psg_abort(1);}

    if ( dm3[A] == 'y' || dm3[C] == 'y' )
       { text_error("incorrect dec3 decoupler flags! Should be 'nyn' or 'nnn' ");
                                                                     psg_abort(1);}
    if ( dpwr > 50 )
       { text_error("dpwr too large! recheck value  "); psg_abort(1);}

    if ( pw > 20.0e-6 )
       { text_error(" pw too long ! recheck value "); psg_abort(1);}

    if ( pwN > 100.0e-6 )
       { text_error(" pwN too long! recheck value "); psg_abort(1);}


/* BEGIN ACTUAL PULSE SEQUENCE */

status(A);
    delay(d1);
    if ( dm3[B] == 'y' )
      { lk_hold(); lk_sampling_off();}  /*freezes z0 correction, stops lock pulsing*/

    obsoffset(tof);
    decoffset(dof);	/* 13C OFFSET ON CA */
    obspower(tpwr);        
    decpower(pwClvl);
    decpwrf(rf0);
    dec2power(pwNlvl);
    txphase(zero);
    decphase(zero);
    dec2phase(zero);
    rcvroff();

    delay(10.0e-6);
    decrgpulse(pwC, zero, 0.0, 0.0);
    zgradpulse(gzlvl0, 5.0e-4);
    delay(5.0e-4);

    rgpulse(pw, zero, 0.0, 0.0);            /* 1H -> 13Calpha INEPT TRANSFER BEGINS */
    zgradpulse(gzlvl0,gt0);  
    delay(taua -gt0 -2.0*GRADIENT_DELAY);

    simpulse(2.0*pw, 2.0*pwC, zero, zero, 0.0, 0.0);

    zgradpulse(gzlvl0,gt0); 
    txphase(one);
    delay(taua -gt0 -2.0*GRADIENT_DELAY -SAPS_DELAY);

    rgpulse(pw, one, 0.0, 0.0);
  
    zgradpulse(gzlvl3,gt3);
    obspower(tpwrd);
    decpwrf(rf1);
    delay(gstab);

    decrgpulse(pwC1, zero, 0.0, 0.0);	/* 13Calpha -> 15N INEPT TRASFER BEGINS */

    decpwrf(rf2);
    delay(taub -PRG_START_DELAY);

    rgpulse(pwHd, one, 0.0, 0.0);
    txphase(zero);
    delay(2.0e-6);
    obsprgon("waltz16", pwHd, 90.0);
    xmtron();
    delay(timeTN -taub -0.5*pwC2);

    sim3pulse(0.0, pwC2, 2.0*pwN, zero, zero, zero, 0.0, 0.0);

    decpwrf(rf1);
    delay(timeTN -0.5*pwC2);

    decrgpulse(pwC1, zero, 0.0, 0.0);
   
    xmtroff();
    obsprgoff();
    rgpulse(pwHd, three, 0.0, 0.0);
    zgradpulse(gzlvl3, gt3);
    decpwrf(rf2);
    dec2phase(t3);
    delay(gstab);

    if ( dm3[B] == 'y' )     /* begins optional 2H decoupling */
      {
       dec3rgpulse(1/dmf3,one,10.0e-6,2.0e-6);
       dec3unblank();
       dec3phase(zero);
       delay(2.0e-6);
       setstatus(DEC3ch, TRUE, 'w', FALSE, dmf3);
      }

    rgpulse(pwHd, one, 0.0, 0.0);
    txphase(zero);
    delay(2.0e-6);
    obsprgon("waltz16", pwHd, 90.0);
    xmtron();
    
    dec2rgpulse(pwN, t3, 0.0, 0.0);
/* N EVOLUTION BEGINS */
      dec2phase(zero);
      
      corr = 0.5*(4.0*pwN/PI +pwC2 +pwC9 +WFG2_START_DELAY +2.0*SAPS_DELAY +PWRF_DELAY);

      if (tau1 > corr) delay(tau1 -corr);

      decrgpulse(pwC2, zero, 0.0, 0.0);
      decpwrf(rf9);
      decshaped_pulse("offC9", pwC9, zero, 0.0, 0.0);  
      dec2phase(t5);

      if (tau1 > corr) delay(tau1 -corr);

/* N EVOLUTION ENDS */
    dec2rgpulse(pwN, t5, 0.0, 0.0);

    xmtroff();
    obsprgoff();
    rgpulse(pwHd, three, 0.0, 0.0);

    if ( dm3[B] == 'y' )   /* turns off 2H decoupling  */
      {
       setstatus(DEC3ch, FALSE, 'c', FALSE, dmf3);
       dec3rgpulse(1/dmf3,three,2.0e-6,2.0e-6);
       dec3blank();
       lk_autotrig();   /* resumes lock pulsing */
      }

    zgradpulse(gzlvl4, gt4);
    delay(gstab);

    rgpulse(pwHd,one,0.0,0.0);
    txphase(zero);
    delay(2.0e-6);
    obsprgon("waltz16", pwHd, 90.0);
    xmtron();

    decpwrf(rf1);
    delay(10.0e-6);

    decphase(t8);
    decrgpulse(pwC1, t8, 0.0, 0.0);
/* Calpha CT evolution begins here */

    decphase(t9);
    decpwrf(rf2);
    delay(timeTN -tau2 -PWRF_DELAY -SAPS_DELAY);

    sim3pulse(0.0, pwC2, 2.0*pwN, zero, t9, zero, 0.0, 0.0);

    decpwrf(rf9);
    if (tau2 > taub)
	{
          delay(timeTN -pwC9a -WFG_START_DELAY);     	   /* WFG_START_DELAY */
          decshaped_pulse("offC9", pwC9a, zero, 0.0, 0.0);
          delay(tau2 -taub -PRG_STOP_DELAY -pwHd -2.0e-6);
          xmtroff();
          obsprgoff();					    /* PRG_STOP_DELAY */
	  rgpulse(pwHd,three,2.0e-6,0.0);
	  txphase(t4);
          if (mag_flg[A]=='y')    magradpulse(gzcal*gzlvl1, gt1);
          else    zgradpulse(gzlvl1, gt1);   	/* 2.0*GRADIENT_DELAY */
	  obspower(tpwr);				       /* POWER_DELAY */
          decpwrf(rf0);
	  decphase(t10);
          delay(taub -gt1 -2.0*GRADIENT_DELAY -2.0*POWER_DELAY -SAPS_DELAY);
	}
    else if (tau2 > (taub -pwC9a -WFG_START_DELAY))
	{
          delay(timeTN +tau2 -taub -PRG_STOP_DELAY -pwHd -2.0e-6);
          xmtroff();
          obsprgoff();					    /* PRG_STOP_DELAY */
	  rgpulse(pwHd,three,2.0e-6,0.0);
	  txphase(t4);                                     /* WFG_START_DELAY */
          decshaped_pulse("offC9", pwC9a, zero, 0.0, 0.0);
          if (mag_flg[A]=='y')    magradpulse(gzcal*gzlvl1, gt1);
          else    zgradpulse(gzlvl1, gt1);   	/* 2.0*GRADIENT_DELAY */
	  obspower(tpwr);				       /* POWER_DELAY */
          decpwrf(rf0);
	  decphase(t10);
          delay(taub -pwC9a -WFG_START_DELAY -gt1 -2.0*GRADIENT_DELAY -2.0*POWER_DELAY -SAPS_DELAY);
	}
    else if (tau2 > (gt1 +gstab))
	{
          delay(timeTN +tau2 -taub -PRG_STOP_DELAY -pwHd -2.0e-6);
          xmtroff();
          obsprgoff();					    /* PRG_STOP_DELAY */
	  rgpulse(pwHd,three,2.0e-6,0.0);
	  txphase(t4);
          delay(taub -tau2 -pwC9a -WFG_START_DELAY);   /* WFG_START_DELAY */
          decshaped_pulse("offC9", pwC9a, zero, 0.0, 0.0);
          if (mag_flg[A]=='y')    magradpulse(gzcal*gzlvl1, gt1);
          else    zgradpulse(gzlvl1, gt1);   	/* 2.0*GRADIENT_DELAY */
	  obspower(tpwr);				       /* POWER_DELAY */
          decpwrf(rf0);
	  decphase(t10);
          delay(tau2 -gt1 -2.0*GRADIENT_DELAY -2.0*POWER_DELAY -SAPS_DELAY);
	}
    else
	{
          delay(timeTN +tau2 -taub -PRG_STOP_DELAY -pwHd -2.0e-6);
          xmtroff();
	  obsprgoff();					    /* PRG_STOP_DELAY */
	  rgpulse(pwHd,three,2.0e-6,0.0);
	  txphase(t4);
    	  delay(taub -tau2 -pwC9a -WFG2_START_DELAY -gt1 -2.0*GRADIENT_DELAY -gstab);
          if (mag_flg[A]=='y')    magradpulse(gzcal*gzlvl1, gt1);
          else    zgradpulse(gzlvl1, gt1);   	/* 2.0*GRADIENT_DELAY */
	  obspower(tpwr);				       /* POWER_DELAY */
	  delay(gstab -POWER_DELAY);                    /* WFG_START_DELAY */
          decshaped_pulse("offC9", pwC9a, zero, 0.0, 0.0);
          decpwrf(rf0);
	  decphase(t10);
          if (tau2 > POWER_DELAY -SAPS_DELAY) delay(tau2 -POWER_DELAY -SAPS_DELAY);
	   else delay(tau2);
	}


/*  xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx  */


/* Calpha CT evolution ends here */

    simpulse(pw, pwC, t4, t10, 0.0, 0.0);
    zgradpulse(0.8*gzlvl5, gt5);
    decphase(zero);
    delay(taua -pwC -gt5 -2.0*GRADIENT_DELAY);

    simpulse(2.0*pw, 2.0*pwC, zero, zero, 0.0, 0.0);

    zgradpulse(0.8*gzlvl5, gt5);
    txphase(one);
    decphase(t11);
    delay(taua - pwC - gt5 -2.0*GRADIENT_DELAY);

    simpulse(pw, pwC, one, t11, 0.0, 0.0);
    
    zgradpulse(gzlvl5, gt5);
    txphase(zero);
    decphase(zero);
    delay(taua -gt5 -2.0*GRADIENT_DELAY);

    simpulse(2.0*pw, 2.0*pwC, zero, zero, 0.0, 0.0);

    zgradpulse(gzlvl5, gt5);
    decpower(dpwr);
    dec2power(dpwr2);
    delay(taua -gt5 -2.0*GRADIENT_DELAY -2.0*POWER_DELAY);
	
    rgpulse(pw, zero, 0.0, 0.0);

    delay(1.0e-4 +gstab + gt1/4.0 + 2.0*GRADIENT_DELAY);
    rgpulse(2.0*pw, zero, 0.0, 0.0);
    if(mag_flg[A] == 'y') magradpulse(icosel*gzcal*gzlvl2, gt1/4.0);
      else zgradpulse(icosel*gzlvl2, gt1/4.0);  
        
    delay(gstab);
    rcvron();
statusdelay(C, 1.0e-4);
    if (dm3[B]=='y') lk_sample();

    setreceiver(t12);
}
