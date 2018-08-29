/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* PR42_gChmqcnoesyNhsqcA.c

 This methyl-NH NOESY sequence is only suitable for 2H/13C/15N, ILV-methyl labeled samples.

 4D 13C-HMQC ------- NOESY ------- 15N-HSQC

 DESCRIPTION and INSTRUCTIONS:
 This pulse sequence will allow one to perform the following experiment:
    (t1) -->  1H
    (t2) --> 13C
    (t3) --> 15N

 OFFSET POSITION:
    tof     =   ~4.75 ppm (1H on water).
    tofali  =   1H offset for methyl region 
    dof     =   13C offset for methyl region 
    dofcaco =  120 ppm (Center of Ca and CO).
    dof2    =  120 ppm (15N region).

 For CH3-NH experiment, set codec='n'
 CA/CO decoupling in t3 is achieved with a stC200 pulse

 Modified from Varian BioPack gChmqcnoesyNhsqcA.c 
   Ref: Filtered Backprojection for the Reconstruction of a High-Resolution (4,2)D
      CH3-NH NOESY Spectrum on a 29 kDa protein.
      Coggins BE, Venters RA and Zhou P. JACS 127, 11562-11563 (2005).
    Added to BioPack, Sept 2005, G.Gray, Varian Palo Alto

 To obtain reconstruction software package, please visit
 http://zhoulab.biochem.duke.edu/software/pr-calc
*/

#include <standard.h>
#include "Pbox_bio.h" 

extern int dps_flag;

static double   ofs, bw, pws, ppm, nst; 
static shape Pdec_154p;

static  int phi1[4] = {0,0,2,2},
            phi2[2] = {0,2},
            phi14[2]= {3,1},
            phi3[1] = {0},
            phi4[8] = {0,0,0,0,1,1,1,1},
            phi5[1] = {0},
            rec[8]  = {0,2,2,0,2,0,0,2};

static shape stC200;

pulsesequence()
{
 char   codec[MAXSTR], codecseq[MAXSTR];

 int	icosel,  t1_counter;

 double	d2_init = 0.0, 
        rf0 = 4095, rf200, pw200,
        copwr, codmf, cores, copwrf,

        tpwrs,   
        pwHs = getval("pwHs"), 
        compH = getval("compH"),
	pwClvl = getval("pwClvl"), 
        pwC = getval("pwC"), 
        compC = getval("compC"), 
        pwNlvl = getval("pwNlvl"), 
        pwN = getval("pwN"),

        sw1 = getval("sw1"), 
        swH = getval("swH"),
        swC = getval("swC"), 
        swN = getval("swN"), 
        swTilt,
        angle_H = getval("angle_H"), 
        angle_C = getval("angle_C"),
        angle_N, 
        cos_H, 
        cos_C,
        cos_N,

        mix = getval("mix"),
        tauCH = getval("tauCH"),                                                /* 1/4JCH  */
        tauNH = getval("tauNH"),                                                /* 1/4JNH  */
	tau1, tau2, tau3,
        tofali =getval("tofali"),                             /* aliphatic protons offset  */
        dofcaco =getval("dofcaco"),               /* offset for caco decoupling, ~120 ppm  */
        dof = getval("dof"),

        gstab = getval("gstab"),

        gt0 = getval("gt0"),  gzlvl0 = getval("gzlvl0"),
        gt1 = getval("gt1"),  gzlvl1 = getval("gzlvl1"),
                              gzlvl2 = getval("gzlvl2"),
        gt3 = getval("gt3"),  gzlvl3 = getval("gzlvl3"),
        gt4 = getval("gt4"),  gzlvl4 = getval("gzlvl4"),
        gt5 = getval("gt5"),  gzlvl5 = getval("gzlvl5"),
        gt6 = getval("gt6"),  gzlvl6 = getval("gzlvl6"),
        gt7 = getval("gt7"),  gzlvl7 = getval("gzlvl7"),
        gt8 = getval("gt8"),  gzlvl8 = getval("gzlvl8"),
        gt9 = getval("gt9"),  gzlvl9 = getval("gzlvl9");

/* LOAD VARIABLES */
   copwr = getval("copwr");        copwrf = getval("copwrf");
   codmf = getval("codmf");        cores = getval("cores");
   getstr("codecseq", codecseq);   getstr("codec", codec);

/* Load phase cycling variables */

   settable(t1, 4, phi1);   settable(t2, 2, phi2);   settable(t3, 1, phi3);
   settable(t4, 8, phi4);   settable(t5, 1, phi5);   settable(t14, 2, phi14);
   settable(t11, 8, rec);

   angle_N=0.0; cos_N=0.0;

/* activate auto-calibration flags */
setautocal();
  if (autocal[0] == 'n')
  {
    /* 180 degree adiabatic C13 pulse from 0 to 200 ppm */
    pw200 = getval("pw200");
    rf200 = (compC*4095.0*pwC*4000.0*sqrt((30.0*sfrq/600.0+7.0)/0.35));
    rf200 = (int) (rf200 + 0.5);
                             
    if (1.0/(4000.0*sqrt((30.0*sfrq/600.0+7.0)/0.35)) < pwC )
    { text_error( " Not enough C13 RF. pwC must be %f usec or less.\n",
                    (1.0e6/(4000.0*sqrt((30.0*sfrq/600.0+7.0)/0.35))) );
      psg_abort(1); }
  }
  else        /* if autocal = 'y'(yes), 'q'(quiet), r(read), or 's'(semi) */
  {
    strcpy(codecseq,"Pdec_154p");
    if(FIRST_FID)                                            /* call Pbox */
    {
      ppm = getval("dfrq");
      bw = 200.0*ppm; pws = 0.001; ofs = 0.0; nst = 1000; 
                                 /* 1 ms long pulse, nst: number of steps */
      stC200 = pbox_makeA("stC200", "sech", bw, pws, ofs, compC*pwC, pwClvl, nst);

      bw=20.0*ppm; ofs=154*ppm;
      Pdec_154p = pbox_Dsh("Pdec_154p", "WURST2", bw, ofs, compC*pwC, pwClvl);
    }
    rf200 = stC200.pwrf;  pw200 = stC200.pw;
  }
/* selective H20 one-lobe sinc pulse */
   tpwrs = tpwr - 20.0*log10(pwHs/(compH*pw*1.69));
   tpwrs = (int)(tpwrs+0.5);

/* check validity of parameter range */

   if((dm[A] == 'y'  || dm[B] == 'y' || dm[C] == 'y'))
   { printf("incorrect Dec1 decoupler flags!  ");      psg_abort(1);   }

   if((dm2[A] == 'y' || dm2[B] == 'y'))
   { printf("incorrect Dec2 decoupler flags!  ");      psg_abort(1);   }
  
   if ((dpwr > 48) || (dpwr2 > 48))
   { printf("don't fry the probe, dpwr too high!  ");  psg_abort(1);   }

/* set up angles for PR42 experiments */

   /* sw1 is used as symbolic index */
   if ( sw1 < 1000 ) { printf ("Please set sw1 to some value larger than 1000.\n"); psg_abort(1); }

   if (angle_H < 0 || angle_C < 0 || angle_H > 90 || angle_C > 90 )
   { printf("angles must be set between 0 and 90 degree.\n"); psg_abort(1); }

   cos_H = cos (PI*angle_H/180);  cos_C = cos (PI*angle_C/180);
   if ( (cos_H*cos_H + cos_C*cos_C) > 1.0) { printf ("Impossible angle combinations.\n"); psg_abort(1); }
   else { cos_N = sqrt(1 - (cos_H*cos_H + cos_C*cos_C) );  angle_N = acos(cos_N)*180/PI;  }

   if (ix == 1) d2_init = d2;
   t1_counter = (int)((d2-d2_init)*sw1 + 0.5);

   if(t1_counter % 2)   { tsadd(t3,2,4);  tsadd(t11,2,4);  }

   swTilt = swH*cos_H + swC*cos_C + swN*cos_N;


   if (phase1 == 1)  {;}                                                              /* CC */
   else if (phase1 == 2)  { tsadd(t1, 1, 4); }                                        /* SC */
   else if (phase1 == 3)  { tsadd(t2, 1, 4); tsadd(t14,1,4); }                        /* CS */
   else if (phase1 == 4)  { tsadd(t1, 1, 4); tsadd(t2,1,4); tsadd(t14,1,4); }         /* SS */

   if ( phase2 == 1 )    { tsadd(t5,2,4);   icosel = 1; }
   else                    icosel = -1; 

   tau1 = 1.0 * t1_counter * cos_H / swTilt;
   tau2 = 1.0 * t1_counter * cos_C / swTilt;
   tau3 = 1.0 * t1_counter * cos_N / swTilt;

   tau1 = tau1/2.0;  tau2 = tau2/2.0;  tau3 =tau3/2.0;


   if (ix ==1 )
   {
      printf ("Current Spectral Width:\t\t%5.2f\n", swTilt);
      printf ("Angle_H: %5.2f \n", angle_H);
      printf ("Angle_C: %5.2f \n", angle_C);
      printf ("Angle_N: %5.2f \n", angle_N);
      printf ("\n\n\n\n\n");
   }


/* BEGIN ACTUAL PULSE SEQUENCE */

status(A);

      delay(d1);

      rcvroff();
      obsoffset(tofali);  obspower(tpwr);     obspwrf(4095.0);
      decoffset(dof);     decpower(pwClvl);   decpwrf(rf0);
      dec2offset(dof2);   dec2power(pwNlvl);  dec2pwrf(4095.0);

      if (gt0 > 0.2e-6)
      {
         decrgpulse(pwC,zero,10.0e-6,0.0);
         dec2rgpulse(pwN,zero,2.0e-6,2.0e-6);
         zgradpulse(gzlvl0,gt0); 
         delay(gstab);
      }

      txphase(t1);   decphase(t2);   dec2phase(zero);

status(B);

   rgpulse(pw,t1,4.0e-6,2.0e-6);                     

      zgradpulse(gzlvl3,gt3);
      delay(2.0*tauCH - gt3 - 2.0*GRADIENT_DELAY -4.0e-6);

   decrgpulse(pwC,t2,2.0e-6,2.0e-6);	                        
      decphase(zero);   

   /*=======  Start of c13 evolution ==========*/

      if ( ((tau2 -PRG_START_DELAY - POWER_DELAY -pwN - 2.0*pwC/PI -2.0e-6)> 0) 
           && ((tau2 -PRG_STOP_DELAY - POWER_DELAY - pwN - 2.0*pwC/PI -2.0e-6)>0)
           && (codec[A] == 'y') )
      {
         decpower(copwr);  decpwrf(copwrf);
         decprgon(codecseq,1/codmf,cores);
         decon();

         delay(tau2 -PRG_START_DELAY - POWER_DELAY -pwN - 2.0*pwC/PI -2.0e-6);
   sim3pulse(2.0*pw, 0.0, 2.0*pwN, t1, zero, zero, 0.0, 0.0);
         delay(tau2 -PRG_STOP_DELAY - POWER_DELAY - pwN - 2.0*pwC/PI -2.0e-6);

         decoff();
         decprgoff();
      }
      else if ( (tau2 -pwN - 2.0*pwC/PI -2.0e-6) > 0)
      {
         delay(tau2 -pwN - 2.0*pwC/PI -2.0e-6);
   sim3pulse(2.0*pw, 0.0, 2.0*pwN, t1, zero, zero, 0.0, 0.0);
         delay(tau2 -pwN - 2.0*pwC/PI -2.0e-6);
      }
      else
      {
         delay(2.0*tau2);
         decphase(t14);  
         delay(4.0e-6);
   sim3pulse(2.0*pw, 2.0*pwC, 2.0*pwN, t1, t14, zero, 0.0, 0.0);
         delay(4.0e-6);
      }

      decpower(pwClvl);
      decpwrf(rf0);
      decphase(zero);
   /*======= End of  c13 evolution ==========*/
   decrgpulse(pwC,zero, 2.0e-6,2.0e-6);                       

      txphase(zero);
      delay(2.0*tauCH + tau1 - gt3 - 4.0*pwC
                    - gstab -4.0e-6 - 2.0*GRADIENT_DELAY);	
      zgradpulse(gzlvl3,gt3);
      delay(gstab);

   decrgpulse(pwC,zero,0.0, 0.0);
      decphase(one);
   decrgpulse(2.0*pwC, one, 0.2e-6, 0.0);
      decphase(zero);
   decrgpulse(pwC, zero, 0.2e-6, 0.0);

      delay(tau1);

   rgpulse(pw,zero,2.0e-6,0.0);                             

  /* ===========Beginning of NOE transfer ======= */

      delay(mix - gt4 - gt5 - pwN -pwC - 2.0*gstab);		
      obsoffset(tof);

      zgradpulse(gzlvl4,gt4);
      delay(gstab);

   decrgpulse(pwC, zero, 2.0e-6, 2.0e-6);
   dec2rgpulse(pwN, zero, 2.0e-6, 2.0e-6);

      zgradpulse(gzlvl5,gt5);
      delay(gstab);	
                                            /* H2O relaxes back to +z */

  /* ===========   End of NOE transfer ======= */

   rgpulse(pw, zero, 2.0e-6, 2.0e-6);                           

      zgradpulse(gzlvl6,gt6);
      delay(tauNH - gt6 - 4.0e-6 - 2.0*GRADIENT_DELAY);

   sim3pulse(2.0*pw, 0.0, 2*pwN, zero, zero, zero, 2.0e-6, 2.0e-6);

      delay(tauNH - gt6 - gstab -4.0e-6 - 2.0*GRADIENT_DELAY);
      zgradpulse(gzlvl6,gt6);
      txphase(one);
      delay(gstab);

   rgpulse(pw,one,2.0e-6,2.0e-6);	                    

      txphase(two);
      obspower(tpwrs);

      shaped_pulse("H2Osinc", pwHs, two, 2.0e-6, 2.0e-6);

      obspower(tpwr);
      zgradpulse(gzlvl7,gt7);
      dec2phase(t3);
      decoffset(dofcaco);          /* offset on 120ppm for CaCO decoupling */
      decpwrf(rf200);                             /* fine power for stC200 */
      delay(gstab);			

   dec2rgpulse(pwN,t3,2.0e-6,2.0e-6);

      dec2phase(t4);
      delay(tau3); 

   rgpulse(2.0*pw, zero, 2.0e-6, 2.0e-6);
   decshaped_pulse("stC200", 1.0e-3, zero, 2.0e-6, 2.0e-6);

      delay(tau3);
      delay(gt1 +gstab +8.0e-6 - 1.0e-3 - 2.0*pw);

   dec2rgpulse(2.0*pwN, t4, 2.0e-6, 2.0e-6);

      dec2phase(t5);
      zgradpulse(gzlvl1, gt1);       
      delay(gstab + WFG_START_DELAY + WFG_STOP_DELAY - 2.0*GRADIENT_DELAY);

   sim3pulse(pw, 0.0, pwN, zero, zero, t5, 2.0e-6, 2.0e-6);

      dec2phase(zero);
      zgradpulse(gzlvl8, gt8);
      delay(tauNH - gt8 - 2.0*GRADIENT_DELAY -4.0e-6);

   sim3pulse(2.0*pw, 0.0, 2.0*pwN, zero, zero, zero, 2.0e-6, 2.0e-6);

      delay(tauNH  - gt8 - gstab -4.0e-6 - 2.0*GRADIENT_DELAY);
      zgradpulse(gzlvl8, gt8);
      txphase(one);    dec2phase(one);
      delay(gstab);

   sim3pulse(pw, 0.0, pwN, one, zero, one, 2.0e-6, 2.0e-6);

      txphase(zero);   dec2phase(zero);
      zgradpulse(gzlvl9, gt9);
      delay(tauNH - gt9 - 2.0*GRADIENT_DELAY -4.0e-6);

   sim3pulse(2.0*pw, 0.0, 2.0*pwN, zero, zero, zero, 2.0e-6, 2.0e-6);

      delay(tauNH - gt9 - 2.0*GRADIENT_DELAY -gstab -4.0e-6);
      zgradpulse(gzlvl9, gt9);
      delay(gstab);

   rgpulse(pw, zero, 2.0e-6, 2.0e-6);

      delay((gt1/10.0) +gstab -2.0e-6 - 0.5*pw + 2.0*GRADIENT_DELAY + POWER_DELAY);

   rgpulse(2.0*pw, zero, 2.0e-6, 2.0e-6);

      zgradpulse(icosel*gzlvl2, gt1/10.0);    

      dec2power(dpwr2);	
      delay(gstab);

status(C);
      setreceiver(t11);
} 

