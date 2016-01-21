/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#ifndef LINT
#endif

/*
Juan A. Aguilar. Mathias Nilsson. Gareth A. Morris. Manchester. 13-06-2011
NB development version, minimum phase cycle, subject to change
http://nmr.chemistry.manchester.ac.uk

Convection compensated.

fn2D      - Fourier number to up the 2D display in F2

*/


#include <standard.h>
#include <chempack.h>

pulsesequence()
{

static int 	ph1[4] = {0, 2, 0, 2},
		ph2[4] = {1, 1, 1, 1},
		ph3[4] = {1, 1, 3, 3},
		ph4[4] = {0, 2, 0, 2};
		
double	gzlvl1 = getval("gzlvl1"), /* Diffusion-encoding pulse strength */
	gt1 = getval("gt1"),   /* Diffusion-encoding pulse width */
	gstab = getval("gstab"), /* Gradient stabilization delay (~0.0002s) */
	hsglvl = getval("hsglvl"),
	hsgt = getval("hsgt"),
        cycles = getval("cycles"), /* 1 cycle = 2 spin echoes = 2 diffusion 
                                   encoding periods. Keep in mind that each 
                                   cycle uses 4 gradients, so keep it to a 
                                   minumum. It also helps keeping signal. */
        cycle_time = getval("cycle_time"), /* cycle_time= time duration for i
                the whole cycle (2 spin echoes). 20-30, even 40 ms can be used
                depending on the actual coupling constants. Not the whole 
                cycle_time is used for diffusion encoding, as gstabs are used */
	Dtau,Ddelta,Deltac,dosytimecubed, dosyfrq;
		
char	satmode[MAXSTR],flg_45[MAXSTR],alt_grd[MAXSTR];

int     prgcycle = (int)(getval("prgcycle")+0.5);

	getstr("satmode",satmode);
        getstr("flg_45",flg_45);
        getstr("alt_grd",alt_grd);

       //synchronize gradients to srate for probetype='nano'
       //   Preserve gradient "area"
          gt1 = syncGradTime("gt1","gzlvl1",0.5);
          gzlvl1 = syncGradLvl("gt1","gzlvl1",0.5);

        sub(ct,ssctr,v12);
	settable(t1,4,ph1);      getelem(t1,v12,v1);
	settable(t2,4,ph2);      getelem(t2,v12,v2);
	settable(t3,4,ph3);      getelem(t3,v12,v3);
	settable(t4,4,ph4);      getelem(t4,v12,oph);
			
        assign(v12,v17);
        assign(zero,v18);
        assign(zero,v19);

  if (getflag("prgflg") && (satmode[0] == 'y') && (prgcycle > 1.5))
    {
        hlv(v12,v17);
        mod2(v12,v18); dbl(v18,v18);
        if (prgcycle > 2.5)
           {
                hlv(v17,v17);
                hlv(v12,v19); mod2(v19,v19); dbl(v19,v19);
           }
     }
   
   add(oph,v18,oph);
   add(oph,v19,oph);

   Ddelta=gt1;
   Deltac=cycle_time*0.5-gt1-gstab-rof1+pw*2.0-gt1/3.0;
   dosytimecubed=(gt1*gt1*2.0*(cycles-1.0)*Deltac);   

   dosyfrq = sfrq;
   cycles=cycles-2;
   initval(cycles,v5);		   

   putCmd("makedosyparams(%e,%e)\n",dosytimecubed,dosyfrq);

status(A);

   delay(5.0e-5);
   if (getflag("sspul"))
        steadystate();

   if (satmode[0] == 'y')
     {
        if ((d1-satdly) > 0.02)
                delay(d1-satdly);
        else
                delay(0.02);
        if (getflag("slpsat"))
             {
                shaped_satpulse("relaxD",satdly,zero);
                if (getflag("prgflg"))
                   shaped_purge(v1,zero,v18,v19);
             }

        else
             {
                satpulse(satdly,zero,rof1,rof1);
                if (getflag("prgflg"))
                   purge(v1,zero,v18,v19);
             }
     }
   else
     delay(d1);

   if (getflag("wet")) 
     wet4(zero,one);

status(B);
      if (getflag("lkgate_flg"))
              lk_hold(); /* turn lock sampling off */

	rgpulse(pw, v1, rof1, rof1);
                                                
  /*  ************** 1st cycle START *********************/
         
      delay(cycle_time*0.25-2.0*rof1);
   	  rgpulse(pw*2.0,v2,rof1,rof1);            /* 180d */   
      delay(cycle_time*0.25-2.0*rof1);
      
      rgpulse(pw,v3,rof1,rof1);               /* 90d refocusing pulse */
      
      if (alt_grd[0] == 'y')
      {  ifzero(v10);
           zgradpulse(gzlvl1,gt1);
         elsenz(v10);
           zgradpulse(-1.0*gzlvl1,gt1);
         endif(v10);
      }
      else
         zgradpulse(gzlvl1,gt1);

      delay(cycle_time*0.25-gt1-2.0*rof1);
   	  rgpulse(pw*2.0,v2,rof1,rof1);       	   /* 180d */   
      delay(cycle_time*0.25-gstab-gt1-rof1);

      if (alt_grd[0] == 'y')
      {  ifzero(v10);
           zgradpulse(gzlvl1,gt1);
         elsenz(v10);
           zgradpulse(-1.0*gzlvl1,gt1);
         endif(v10);
      }
      else
         zgradpulse(gzlvl1,gt1);
      delay(gstab);
      
/*  *********** 1st cycle END  -  Core cycles START **************/

   starthardloop(v5); 

      if (alt_grd[0] == 'y')
      {  ifzero(v10);
           zgradpulse(gzlvl1,gt1);
         elsenz(v10);
           zgradpulse(-1.0*gzlvl1,gt1);
         endif(v10);
      }
      else
         zgradpulse(gzlvl1,gt1);

      delay(cycle_time*0.25-gt1-rof1);
   	  rgpulse(pw*2.0,v2,rof1,rof1);          /* 180d */   
      delay(cycle_time*0.25-gstab-gt1-2.0*rof1);

      if (alt_grd[0] == 'y')
      {  ifzero(v10);
           zgradpulse(gzlvl1,gt1);
         elsenz(v10);
           zgradpulse(-1.0*gzlvl1,gt1);
         endif(v10);
      }
      else
         zgradpulse(gzlvl1,gt1);
      delay(gstab);
      
          rgpulse(pw,v3,rof1,rof1);            	/* 90d refocusing pulse */
      
      if (alt_grd[0] == 'y')
      {  ifzero(v10);
           zgradpulse(gzlvl1,gt1);
         elsenz(v10);
           zgradpulse(-1.0*gzlvl1,gt1);
         endif(v10);
      }
      else
         zgradpulse(gzlvl1,gt1);
     
      delay(cycle_time*0.25-gt1-2.0*rof1);
   	  rgpulse(pw*2.0,v2,rof1,rof1);         /* 180d */   
      delay(cycle_time*0.25-gstab-gt1-rof1);

      if (alt_grd[0] == 'y')
      {  ifzero(v10);
           zgradpulse(gzlvl1,gt1);
         elsenz(v10);
           zgradpulse(-1.0*gzlvl1,gt1);
         endif(v10);
      }
      else
         zgradpulse(gzlvl1,gt1);
      delay(gstab);

   endhardloop();      

/*  ***** Core cycles END - Last cycle START  *********************/

      if (alt_grd[0] == 'y')
      {  ifzero(v10);
           zgradpulse(gzlvl1,gt1);
         elsenz(v10);
           zgradpulse(-1.0*gzlvl1,gt1);
         endif(v10);
      }
      else
         zgradpulse(gzlvl1,gt1);

     delay(cycle_time*0.25-gt1-rof1);
   	 rgpulse(pw*2.0,v2,rof1,rof1);          /* 180d */   
     delay(cycle_time*0.25-gstab-gt1-2.0*rof1);

      if (alt_grd[0] == 'y')
      {  ifzero(v10);
           zgradpulse(gzlvl1,gt1);
         elsenz(v10);
           zgradpulse(-1.0*gzlvl1,gt1);
         endif(v10);
      }
      else
         zgradpulse(gzlvl1,gt1);
     delay(gstab);
      
         rgpulse(pw,v3,rof1,rof1);  	        /* 90d refocusing pulse */
    
     if (getflag("lkgate_flg"))
              lk_sample(); /* turn lock sampling back on */

     delay(cycle_time*0.25-2.0*rof1);
     if (flg_45[0] == 'y')
        {
         rgpulse(pw*2.0,v2,rof1,rof1);          /* 180d */
         delay(cycle_time*0.25-2.0*rof1);
         rgpulse(0.5*pw,v3,rof1,rof2);   /* 45-degree pulse, orthogonal to the last 90 */
        }
     else
        {
   	 rgpulse(pw*2.0,v2,rof1,rof2);          /* 180d */   
         delay(cycle_time*0.25-rof2);
        }
      
/*  ************** Last cycle END *********************/
   	
status(C);
}

/* Code version: JA_Dproject_04_d. 03 August 2011. Variation: 45d purging pulse not used. gzlvl2 dropped. p3 is now pw. Deltac and dosytimecubed recalculated*/
