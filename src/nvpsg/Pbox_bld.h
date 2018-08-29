/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#ifndef PBOX_BLD_H
#define PBOX_BLD_H

/* Pbox_bld.h - Bi-level decoupling include file 
                Ref. E. Kupce, R. Freeman, G. Wider and K. Wuethrich, 
                     J. Magn. Reson., Ser A, vol. 122, p. 81 (1996)  
                     
                Requires the following parameters:
                jBL - J-coupling to be decoupled by bi-level decoupling
                nBL - number of high power pulses, must be a power of 2. Larger 
                      nBL suppresses higher order sidebands and requires more
                      RF power. The default is 2.
                BLxpwr - extra power level for low power decouling. This is 
                         added to the internal setting of the low power
                         decoupling level to reduce the inner sidebands.
                BLorder - 1, suppresses only high order sidebands; requires 
                             the shortest duration of high power decoupling.
                             The sub-harmonic sidebands can be suppressed by 
                             using high peak amplitude decoupling sequences,
                             such as clean-WURST or sech. The inner sidebands 
                             can be suppressed by increasing the extra power 
                             level, BLxpwr for the low power  decoupling.
                          2, suppresses all outer sidebands including the 
			     sub-harmonic;
                             this is the DEFAULT option. The inner sidebands 
                             can be reduced by increasing the extra power
                             level, BLxpwr for low power  decoupling. 
                          3, suppresses all sidebands including the inner 
                             sidebands; requires the longest duration of
                             high power decoupling;
                             
               Note: This setup uses explicit acquisition (dsp must be set to 
                     dsp = 'n') and depends on the digitization rate. As a
                     result, the decoupling pulse duration depends on spectral
                     width, sw. This can have some limitations, particularly
                     for relatively latge J-couplings and relatively small sw. 
*/

static shape hidec, lodec;

void make_bilev(double BLorder, double pwhi, double pwlo, double dbw, double rpw90, double rpwr)
{
  char  cmd[MAXSTR];    
  double BLxpwr = getval("BLxpwr");

  if(BLxpwr > 6.0)
  {
    abort_message("Unsafe BLxpwr level. Aborting");
  }
    
  sprintf(cmd,"Pbox hilev.DEC -u %s -w \"wurst2i %.1f/%.7f\" -sucyc t5 -s 1.0 -p %.0f -l %.1f",
               userdir, dbw, pwhi, rpwr, rpw90*1.0e6);
  system(cmd);  
  if(BLorder == 16.0)             
    sprintf(cmd,"Pbox lolev.DEC -u %s -w \"WURST2 %.1f/%.7f\" -sucyc m16 -s 1.0 -p %.0f -l %.1f",
                 userdir, dbw, pwlo, rpwr, rpw90*1.0e6);
  else
    sprintf(cmd,"Pbox lolev.DEC -u %s -w \"WURST2 %.1f/%.7f\" -sucyc t5,m4 -s 1.0 -p %.0f -l %.1f",
                 userdir, dbw, pwlo, rpwr, rpw90*1.0e6);
  system(cmd);               
  hidec = getDsh("hilev");
  lodec = getDsh("lolev");
  lodec.pwr = lodec.pwr+BLxpwr;  
  if (hidec.pwr > 63)
  {
    abort_message("insufficient power for bi-level decoupling. Try reducing jBL");
  }

  return;
}

void acquireBL(double dbw, double rpw90, double rpwr)
{
   int             ihi=1;
   double          jBL = getval("jBL"),
                   npx = 2.0,
                   nptot = 2.0,
                   nBL = getval("nBL"),
                   BLorder = getval("BLorder"),
                   pwlo, pwhi,
                   acqdel = 2.0*getval("at")/np;  /* dwell time */   
   
   if(nBL < 1.0) nBL = 4.0;    
   if(BLorder > 2.0) BLorder = 16.0;
   if(jBL < 1.0) 
   {
     abort_message("J-coupling (jBL) required for Bi-level decoupling");
   }
   
   pwlo = 0.2/jBL;                        /* duration of the dec. pulse */  
   ihi = (int) (0.5 + pwlo/acqdel);       /* points in low power dec. pulse */ 
   ihi = (int) (0.5 + (double) ihi/nBL);  /* points in high power dec. pulse */ 
   npx = (double) ihi;                    /* points in high power dec. pulse */
   if(BLorder == 16.0)
     nptot = npx*(BLorder+1.0);
   else
     nptot = npx*(nBL*BLorder+1.0);
   pwhi = (double) npx*acqdel;
   pwlo = (double) pwhi*nBL;
   npx = npx*2.0;
   nptot = nptot*2.0;
   if (FIRST_FID)
     make_bilev(BLorder, pwhi, pwlo, dbw, rpw90, rpwr);
   
   if(BLorder == 16.0)    
     initval(BLorder,v14);
   else
     initval(nBL*BLorder,v14);
   modn(ct, v14, v11);    
   add(one, v11, v11);   /* v11 = 1, 2, ... N */
   if(BLorder == 16.0)    
     initval(BLorder+1.0, v12);
   else
     initval(BLorder*nBL+1.0, v12);
   sub(v12,v11,v10);     /* v10 = N, N-1, ... 1 */  

   decunblank();
   pbox_decon(&hidec);        /* high power decoupling */
    startacq(alfa);
       starthardloop(v11); 
         acquire(npx, acqdel);
       endhardloop();     
       pbox_decoff();
      
       pbox_decon(&lodec);      /* low power decoupling */  
       acquire(2.0, acqdel - PRG_START_DELAY - PRG_STOP_DELAY - POWER_DELAY);
     
       starthardloop(v10);     
         acquire(npx, acqdel);
       endhardloop();
       acquire(np-nptot-2.0, acqdel);
    endacq();
    pbox_decoff();
}

#endif
