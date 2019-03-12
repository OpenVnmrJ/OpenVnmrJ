// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
/* pansy_tocsy.c - PANSY TOCSY, homo/hetero-nuclear correlation spectroscopy,
   written by Eriks Kupce, Oxford, April 2006

  Processing for H-H tocsy:
  ------------------------
  axis='pp' sb=-at sbs=sb sb1=-ni/sw1 sbs1=sb1
  wft2d(1,0,0,0,0,0,0,0,0,0,0,0,-1,0,0,0)

  Processing for H-C COSY:
  ------------------------
  axis='dp' linear prediction may be used
  wft2d(0,0,1,0,0,0,0,0,0,0,0,0,0,0,-1,0)

  c1d - flag to switch to acquire C-13 in 1D mode. The spectrum is processed
        using the same wft2d command. Phase the spectrum in F1. 
        Use trace = 'f2' ds(fn1/4) to display the C-13 1D spectrum.

  Referencing:
  ------------
  the setref macro currently fails for the second receiver data sets. Hence the
  referencing should be done manually, by setting the reference using a cursor.

*/

#include <standard.h>

static shape  HHmix, CHdec;

void pulsesequence()
{
   char    c1d[MAXSTR];               /* option to record only 1D C13 spectrum */
   int     ncyc;
   double  tau1 = 0.002,         			          /*  t1 delay */
           post_del = 0.0,
           pwClvl = getval("pwClvl"), 	  /* coarse power for C13 pulse */
           pwC = getval("pwC"),     	  /* C13 90 degree pulse length at pwClvl */
	   compC = getval("compC"),
	   compH = getval("compH"),
	   mixpwr = getval("mixpwr"),
	   jCH = getval("jCH"),
	   gt0 = getval("gt0"),  		       
	   gt1 = getval("gt1"),  		       
	   gt2 = getval("gt2"),  		       
	   gzlvl0 = getval("gzlvl0"),
	   gzlvl1 = getval("gzlvl1"),
	   gzlvl2 = getval("gzlvl2"),
	   grec = getval("grec"),
	   phase = getval("phase");

           getstr("c1d",c1d);
	   
           ncyc=1;
           if(jCH > 0.0)
             tau1 = 0.25/jCH;

           dbl(ct, v1);                     /* v1 = 0 */
           mod4(v1,oph);
           hlv(ct,v2);
           add(v2,v1,v2);
           if (phase > 1.5)
             incr(v1);                      /* hypercomplex phase increment */           

           initval(2.0*(double)((int)(d2*getval("sw1")+0.5)%2),v10); 
           add(v1,v10,v1);
           add(oph,v10,oph);
           mod4(v1,v1);  mod4(v2,v2); mod4(oph,oph); 
           assign(zero,v3);

           if(FIRST_FID) 
	   {
	     HHmix = pbox_mix("HHmix", "DIPSI2", mixpwr, pw*compH, tpwr);  
	     if(c1d[A] == 'n')
	     {
	       opx("CHdec"); setwave("WURST2 30k/1.2m"); pbox_par("steps","600"); cpx(pwC*compC, pwClvl);
	       CHdec = getDsh("CHdec");
	     }
	   }
	   ncyc = (int) (at/HHmix.pw) + 1;
	   post_del = ncyc*HHmix.pw - at;
	            
             
/* BEGIN PULSE SEQUENCE */

      status(A);

	zgradpulse(gzlvl0, gt0);
	rgpulse(pw, zero, 0.0, 0.0);  /* destroy H-1 magnetization*/
	zgradpulse(gzlvl0, gt0);
	delay(1.0e-4);
	obspower(tpwr);
	txphase(v1);
        decphase(zero);
        dec2phase(zero);

        presat();
	obspower(tpwr);
        	
	delay(1.0e-5);

      status(B);

        if(c1d[A] == 'y')
	{
   	  rgpulse(pw,v1,0.0,0.0);                 /* 1H pulse excitation */
          delay(d2);
   	  rgpulse(pw,two,0.0,0.0);                 /* 1H pulse excitation */
          assign(oph,v3);
	}
	else
	{
          decunblank(); pbox_decon(&CHdec);

   	  rgpulse(pw,v1,0.0,0.0);                 /* 1H pulse excitation */
   	  txphase(zero);
   	
          delay(d2);

          pbox_decoff(); decblank();  
          decpower(pwClvl); decpwrf(4095.0);
   	  
	  delay(tau1 - POWER_DELAY);
          simpulse(2.0*pw, 2.0*pwC, zero, zero, 0.0, 0.0);
          txphase(one); decphase(one); dec2phase(one);
	  delay(tau1);
          simpulse(pw, pwC, one, one, 0.0, 0.0);
          txphase(zero); decphase(zero); dec2phase(zero);
	  delay(tau1);
          simpulse(2.0*pw, 2.0*pwC, zero, zero, 0.0, 0.0);
	  delay(tau1);
          simpulse(0.0, pwC, zero, zero, 0.0, 0.0);
        }
	zgradpulse(gzlvl1, gt1);
   	delay(grec);
        simpulse(0.0, pwC, zero, v3, 0.0, rof2);
        
        txphase(v2);
        obsunblank(); pbox_xmtron(&HHmix);  

      status(C);
      
        setactivercvrs("ny");
        startacq(alfa);
        acquire(np,1.0/sw);
        endacq();
        
	delay(post_del);
        pbox_xmtroff(); obsblank();
        zgradpulse(gzlvl2, gt2);
        obspower(tpwr);
   	delay(grec);
   	rgpulse(pw,zero,0.0,rof2);                 /* 1H pulse excitation */
   	        
      status(D);
      
        setactivercvrs("yn");
        startacq(alfa);
        acquire(np,1.0/sw);
        endacq();

}		 
