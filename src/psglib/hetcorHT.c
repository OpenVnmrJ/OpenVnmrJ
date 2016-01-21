// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
#ifndef LINT
#endif

/* hetcorHT - hadamard heteronuclear chemical shift correlation

 Includes:

   1.  presaturation option
   2.  composite 180 degree pulses
   3.  simultaneous pulses on transmitter and decoupler RF channels


 Parameters:

      pw  -  90 degree pulse on the observe nucleus
    tpwr  -  transmitter power level; only for systems with a linear
             amplifier on the transmitter channel
      pp  -  proton 90 degree pulse on the decoupler channel
   pplvl  -  decoupler power level; only for systems with a linear
             amplifier on the decoupler channel; otherwise decoupler
             is turned to to full-power for pulses on systems that
             have bilevel decoupling capability
   compH  -  H-1 amplifier compression factor
     dhp  -  decoupler power level during acquisition
    dpwr  -  decoupler power level during acquisition for systems with
	     linear amplifiers
  presat  -  'n': no presaturation of 13C signals
             'y': broadband presaturation of 13C signals
    j1xh  -  one-bond heternuclear coupling constant
      nt  -  multiple of 1 (minimum)


 References:

  a.d.bax and g.a.morris, j.magn. reson. 42:501-505 (1981)
  a.d.bax, j.magn. reson. 53:517 (1983)
  joyce a. wilde and philip h. bolton, j. magn. reson. 59:343-346 (1984) */



#include <standard.h>

pulsesequence()
{
   char         Cdseq[MAXSTR], refoc[MAXSTR], sspul[MAXSTR];
   int          mult = (0.5 + getval("mult"));
   double	rg1 = 2.0e-6,
                j1xh = getval("j1xh"),
         	gt0 = getval("gt0"),
         	gzlvl0 = getval("gzlvl0"),
         	pp = getval("pp"),
	 	pplvl = getval("pplvl"),
	 	compH = getval("compH"),
		Cdpwr = getval("Cdpwr"),
	 	Cdres = getval("Cdres"), 
	 	tau1 = 0.002,
	 	tau2 = 0.002,
	 	Cdmf = getval("Cdmf");
   
   shape   hdx;

/* Get new variables from parameter table */

   getstr("Cdseq", Cdseq);
   getstr("refoc", refoc);
   getstr("sspul", sspul);
   
   if (j1xh < 1.0) j1xh = 150.0;

   hdx = pboxHT_F1i("gaus180", pp*compH, pplvl); /* HADAMARD stuff */

   if (getval("htcal1") > 0.5)          /* Optional fine power calibration */
     hdx.pwr = getval("htpwr1");

   if(j1xh > 0.0)
     tau1 = 0.25/j1xh;
   tau2 = tau1;
   if (mult > 2) tau2=tau1/2.5;
   else if ((mult == 0) || (mult == 2)) tau2=tau1/2.0;

   dbl(ct, v1);                     /*  v1 = 02 */
   add(two,v1,oph);
   mod4(oph,oph);                   /* oph = 02 */


/* Calculate delays */

   if (dm[0] == 'y')
   {
     abort_message("decoupler must be set as dm=nny\n");
   }
   if(refoc[A]=='n' && dm[C]=='y')
   {
     abort_message("with refoc=n decoupler must be set as dm=nnn\n");
   }
 
/* Relaxation delay */

   status(A);

   delay(0.05);
   zgradpulse(gzlvl0,gt0);
   if (sspul[0] == 'y')
   {
     obspower(tpwr); decpower(pplvl);
     simpulse(pw, pp, zero, zero, rg1, rg1);  /* destroy H and C magnetization */
     zgradpulse(gzlvl0,gt0);
   }

   delay(d1);
 
   status(B);

     obspower(Cdpwr);
     obsunblank(); xmtron(); 
     obsprgon(Cdseq, 1.0/Cdmf, Cdres);  

     pbox_decpulse(&hdx, zero, rg1, rg1);

     obsprgoff(); xmtroff(); obsblank();  
     obspower(tpwr); decpower(pplvl);
          
     delay(2.0e-4);		
     
     decrgpulse(pp, v1, rg1, rg1);
     
     delay(tau1 - POWER_DELAY);
     simpulse(2.0*pw, 2.0*pp, zero, zero, rg1, rg1);
     txphase(one); decphase(one); 
     delay(tau1);
     simpulse(pw, pp, one, one, rg1, rg1);
     if(refoc[A]=='y')
     {
       txphase(zero); decphase(zero); 
       delay(tau2);
       simpulse(2.0*pw, 2.0*pp, zero, zero, rg1, rg1);
       delay(tau2 - rof2 - POWER_DELAY);
       decrgpulse(pp, zero, rg1, rof2);
     }
     decpower(dpwr);    

   status(C);
}
