// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
/* 
 */
/* hetcorps - heteronuclear chemical shift correlation
              (absolute value and phase sensitive)

 Parameters:

      pw  -  90 degree pulse on the observe nucleus
    tpwr  -  transmitter power level; only for systems with a linear
             amplifier on the transmitter channel
      pp  -  proton 90 degree pulse on the decoupler channel
   pplvl  -  decoupler power level; only for systems with a linear
             amplifier on the decoupler channel; otherwise decoupler
             is turned to to full-power for pulses on systems that
             have bilevel decoupling capability
     dhp  -  decoupler power level during acquisition
    dpwr  -  decoupler power level during acquisition for systems with
	     linear amplifiers
   hmult  -  'n': remove non-geminal proton-proton couplings in F1
             'y': preserve all proton-proton couplings in F1
   chonly -  'y': gives CH only spectrum
   oddeven-  'y': gives CH and CH3 positive and CH2 negative
             'n': gives all positive
             This parameter is irrelevant in av (phase=0) spectrum
              or if chonly = 'y' 
    j1xh  -  one-bond heternuclear coupling constant
   phase  -  1,2 - Hypercomplex
             0   - absolute value
      nt  -  1 (minimum for hypercomplex)
             2 (minimum for absolute value)
             multiple of 2 (recommended for hypercomplex)
                         4 (recommended for absolute value)

 References:

  a.d.bax and g.a.morris, j.magn. reson. 42:51 (1981)
  a.d.bax, j.magn. reson. 53:517 (1983)
  joyce a. wilde and philip h. bolton, j. magn. reson. 59:343-346 (1984) */



#include <standard.h>

static int   phs2[4] = {0,1,2,3},
	     phs8[4] = {0,0,2,2},
	     phs7[2] = {0,2},
	     phs4[1] = {0};

pulsesequence()
{
   char  	hmult[MAXSTR],
                chonly[MAXSTR],
                oddeven[MAXSTR];
   double	j1xh,
         	pp,
	 	pplvl,
         	dly3,
         	dly4,
         	phase;
   int          iphase;

/* Get new variables from parameter table */
   pp = getval("pp");
   j1xh = getval("j1xh");
   getstr("hmult", hmult);
   getstr("chonly",chonly);
   getstr("oddeven",oddeven);
   phase = getval("phase");
   iphase = (int)(phase + 0.5);
   pplvl = getval("pplvl");

/* Calculate delays */
      dly3 = 1.0 / (2.0 * j1xh);

   if (chonly[0] == 'y')
      dly4 = dly3;
   else
   {
    if (iphase == 0)
      dly4 = 1.0/(3.0*j1xh);
    else
    {
     if (oddeven[0] == 'y')
      dly4 = 3.0/(4.0*j1xh);
     else
      dly4 = 1.0/(3.0*j1xh);
    }
   }
/* PHASE CYCLING CALCULATION */

   settable(t2,4,phs2);
   settable(t8,4,phs8);
   settable(t7,2,phs7);
   settable(t4,1,phs4);


   assign(zero, v1);                  /* v1 = 0 */
   if (iphase == 2)
      incr(v1);                      /* hypercomplex phase increment */


   if (iphase == 0)
    hlv(ct,v5);
   else
    assign(ct,v5);
   getelem(t2,v5,v2);
   getelem(t8,v5,v8);

   add(v8,one,v9);

   add(v2,two,v3);

   getelem(t7,v5,v7);
   getelem(t4,v5,v4);

   add(v4,one,v4); 
   mod2(ct,v13);
   if (iphase == 0)
    add(v4,v13,v4);

   add(v4,v7,oph);

   add(v7,one,v6);

   initval(2.0*(double)(((int)(d2*getval("sw1")+0.5)%2)),v12);
   add(v1,v12,v1);
   add(oph,v12,oph);

   add(oph,two,oph);

/* ACTUAL PULSE-SEQUENCE BEGINS  */


   status(A);
      if (dm[0] == 'y')
      {
         fprintf(stdout, "decoupler must be set as dm=nny\n");
         psg_abort(1);
      }
 
      decpower(pplvl);
      hsdelay(d1);


   status(B);
      rcvroff();
      delay(2.0e-5);
      decpulse(pp, v1);

      if (hmult[0] == 'y')
      {	
         if (d2 > 0.0)
          delay(d2/2.0 - (2*pp/PI) - 2.33*pw - rof1);
         else
          delay(d2/2.0);
         rgpulse(pw, v8, rof1, 0.0);
         rgpulse(2.67*pw, v9, 0.0, 0.0);
         rgpulse(pw, v8, 0.0, rof1);
         if (d2 > 0.0) 
          delay(d2/2.0 - 2.33*pw - rof1); 
         else 
          delay(d2/2.0);
      }
      else
      {
         if (d2 > 0.0) 
          delay(d2/2.0 - (4*pp/PI)); 
         else 
          delay(d2/2.0);
         decpulse(pp, v2);
         delay(dly3 - rof1 - pw - 1.5 * pp);
         rgpulse(pw, v8, rof1, 0.0);
         simpulse(2.67*pw, 2.0*pp, v9, v2, 0.0, 0.0);
         rgpulse(pw, v8, 0.0, rof1);
         delay(dly3 - rof1 - pw - 1.5*pp);
         decpulse(pp, v3);
         if (d2 > 0.0)  
          delay(d2/2.0 - (2*pp/PI));  
         else  
          delay(d2/2.0);
      }

      if (iphase == 0)
         delay(dly3 - rof1);
      else
        {
         delay(dly3/2.0 - rof1 - 2.0*pp);
                                               /* composite C & H 180's */
         simpulse(pw, pp, zero, zero, rof1, 0.0);
         simpulse(2.67*pw, 2.0*pp, one, one, 0.0, 0.0);
         simpulse(pw, pp, zero, zero, 0.0, rof1);
         delay(dly3/2.0 - 2.0*rof1 - 2*pp - (2*pp/3.1416));
        }
         simpulse(pw, pp, v7, v4, rof1, rof2);
         delay(dly4/2.0 - 2.33*pp -(2*pp/PI) - rof2 - rof1);
       
       if (iphase != 0)  
         {
          simpulse(pw, pp, v7, zero, rof1, 0.0);
          simpulse(2.0*pw, 2.67*pp, v6, one, 0.0, 0.0);
          simpulse(pw, pp, v7, zero, 0.0, rof2);
         }
      rcvron();
	decpower(dpwr);
      delay(dly4/2.0 - POWER_DELAY - 2.33*pp);
 
/* Observe period */
   status(C);
}
