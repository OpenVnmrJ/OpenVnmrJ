/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*  NH_diffusion. 

  Diffusion of 13C,15N-edited protons.

  Published by Nesmelova I.V., Idiyatullin D., Mayo K.H.
  J.Magn.Res. 2004, vol.166(1), 129-133
 
  Changed from original code from Mayo's group by Mike
  Osborne, March 2006 for compatibility with Biopack

  array gzlvl2 for diffusion decay
  tpwrsf_u and tpwrsf_d allow fine power control of soft pulses
  phincr1 allows phase shifting of first watergate pulse

  Modified for BioPack to use standard flipback pulses GG, March 2006

  Modified sequence to comment out gradient and delay prior to first pulse
   (GG. Varian Palo Alto, Oct. 2006)

*/

#include <standard.h>

static int	ph1[16] = {0,0,0,0,0,0,0,0,2,2,2,2,2,2,2,2},
		ph2[16] = {2,2,2,2,2,2,2,2,0,0,0,0,0,0,0,0},
		ph3[16] = {0,1,2,3,0,1,2,3,0,1,2,3,0,1,2,3},
		ph4[16] = {2,3,0,1,2,3,0,1,2,3,0,1,2,3,0,1},
		ph5[16] = {0,1,2,3,2,3,0,1,0,1,2,3,2,3,0,1},
		ph6[16] = {2,3,0,1,0,1,2,3,2,3,0,1,0,1,2,3},                
		ph7[16] = {0,0,0,0,2,2,2,2,0,0,0,0,2,2,2,2},
		ph9[16] = {2,2,2,2,0,0,0,0,2,2,2,2,0,0,0,0},
		ph10[2] = {0,2},
		ph8[16] = {2,2,2,2,0,0,0,0,0,0,0,0,2,2,2,2}; 

/* array gzlvl2 for diffusion decay*/

pulsesequence()
{
 
char      N15edit[MAXSTR], C13edit[MAXSTR];	      /* C13 editing*/
	   
 double tpwrs,pwC,d2,tau,d3,d4,d5,d6,
 gt2,gt3,gt0,gzlvl0,gzlvl2,gzlvl3,phincr1,tpwrsf_u,tpwrsf_d,pwHs,compH,
 pwN,pwNlvl,ref_pwr,ref_pw90,pwZa,pwClvl,JXH;     
  


     pwC=getval("pwC");
     pwClvl=getval("pwClvl");
     ref_pw90=getval("ref_pw90"); 
     ref_pwr=getval("ref_pwr"); 
     pwHs=getval("pwHs");
     gt2=getval("gt2");
     gt3=getval("gt3");
     gt0=getval("gt0");
 /*    tau=getval("tau"); */
     d2=getval("d2");
     d3=getval("d3");
     d4=getval("d4");
     d5=getval("d5");
     gzlvl2=getval("gzlvl2");
     gzlvl3=getval("gzlvl3");
     gzlvl0=getval("gzlvl0");
     phincr1 = getval("phincr1");
     d6=getval("d6");
     JXH = getval("JXH");

 
    tpwrsf_u = getval("tpwrsf_u"); /* fine power adjustment           */
    tpwrsf_d = getval("tpwrsf_d"); /* fine power adjustment           */
    pwHs = getval("pwHs");       /* H1 90 degree pulse length at tpwrs2 */
    compH = getval("compH");
    pwNlvl = getval("pwNlvl");                    /* power for N15 pulses */
    pwN = getval("pwN");          /* N15 90 degree pulse length at pwNlvl */
    
	 
 getstr("N15edit",N15edit); 
 getstr("C13edit",C13edit); 
 pwZa=pw;                         /* initialize variable */

/* optional editing for C13 enriched samples */
  if ((N15edit[A]=='y') && (C13edit[A]=='n')) 
     {
      pwC = 0.0;  
      if (2.0*pw > 2.0*pwN) pwZa = pw; 
      else pwZa = pwN; 
     }      
  if ((C13edit[A]=='y')&& (N15edit[A]=='n')) 
     {
      pwN = 0.0;  
      if (2.0*pw > 2.0*pwC) pwZa = pw; 
      else pwZa = pwC; 
     }      
  if ((C13edit[A]=='y') && (N15edit[A]=='y'))
     {
      
      if (2.0*pw > 2.0*pwN) pwZa = pw; /*pwN always longer than pwC*/
      else pwZa = pwN; 
     }      

tau = 1/(2*(JXH));

printf("tau is %f\n",tau);
printf("pwZa is %f\n",pwZa);

/* set pwZa to either pw or pwX depending on which is the largest (for calculating delays) */


/*calculate phase cycle for WATERGATE*/
   hlv(ct,v1);
   hlv(v1,v2);
   mod2(v2,v3);
   dbl(v3,v4);
   assign(two,v5);
   add(v4,v5,v6);

   obsstepsize(1.0);
   if (phincr1 < 0.0) phincr1=360+phincr1;
   initval(phincr1,v7);

   settable(t1,16,ph1);
   settable(t2,16,ph2);
   settable(t3,16,ph3);
   settable(t4,16,ph4);      
   settable(t5,16,ph5);
   settable(t6,16,ph6);
   settable(t7,16,ph7); 
   settable(t8,16,ph8);
   settable(t9,16,ph9);
   settable(t10,2,ph10); 



   tpwrs=tpwr-20.0*log10(pwHs/(compH*pw*1.69)); /* sinc pulse */
   tpwrs = (int) (tpwrs) +6.0;  /* to permit finepower ~2048 */ 




/* START THE PULSE SEQUENCE */

     status(A);
     decpower(pwClvl);
     delay(d1);
     obsoffset(tof);
     /*zgradpulse(gzlvl2,gt2);
     delay(d3+d5-pwHs);*/
     obspower(tpwrs); obspwrf(tpwrsf_d);
     shaped_pulse("H2Osinc_d",pwHs,t2,rof1,rof1);

     status(B);
     obspower(tpwr); obspwrf(4095.0);
     rgpulse(pw,t1,3.0e-6,0.0);
     delay(d2);
      zgradpulse(gzlvl2,gt2);
     delay(d5);
     rgpulse(pw,t3,3.0e-6,3.0e-6);
     obspower(tpwrs); obspwrf(tpwrsf_u);
     shaped_pulse("H2Osinc_u",pwHs,t4,rof1,rof1);

     zgradpulse(gzlvl3,gt3);
     delay(d3); obspwrf(tpwrsf_d);
     shaped_pulse("H2Osinc_d",pwHs,t6,rof1,rof1);
     obspower(tpwr); obspwrf(4095.0);
     rgpulse(pw,t5,3.0e-6,3.0e-6);
     delay(d2);
     zgradpulse(gzlvl2,gt2);     
     delay(d5);
     delay(d6);

   
     zgradpulse(gzlvl0,gt0);
     obspower(tpwrs);
     obspwrf(tpwrsf_d);
     xmtrphase(v7);
     delay(tau-pwHs-pwZa-gt0-d6-pwN);
     shaped_pulse("H2Osinc_d",pwHs,v6,rof1,rof1);
     obspower(tpwr);
     obspwrf(4095.0);
     xmtrphase(zero); 
     dec2power(pwNlvl);

     dec2rgpulse(pwN,zero,1.0e-6,1.0e-6);
     decrgpulse(pwC,zero,1.0e-6,1.0e-6);
     rgpulse(2*pw,t7,1.0e-6, 1.0e-6);
     decrgpulse(pwC,t10,1.0e-6,1.0e-6);
     dec2rgpulse(pwN,t10,1.0e-6,1.0e-6);  
     obspower(tpwrs);
     obspwrf(tpwrsf_u); 
     shaped_pulse("H2Osinc_u",pwHs,t9,rof1,rof1);
     delay(tau-pwHs-pwZa-gt0-d6-pwN);
     zgradpulse(gzlvl0,gt0);
     dec2power(dpwr2);
     decpower(dpwr);
     delay(d6); 

     setreceiver(t8);
   status(C);
}
