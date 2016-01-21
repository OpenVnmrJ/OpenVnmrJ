// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
/* 
 */

/* xpolar1.c - cp/mas pulse sequence - 

   MERCURY Version 02/02/01
   D. Rice 04/21/01

*/

#include <standard.h>

static int table1[4] = {0, 1, 2, 3};
static int table2[4] = {1, 1, 3, 3};
static int table3[4] = {3, 0, 3, 0};
static int table4[4] = {1, 2, 1, 2};

pulsesequence()

{

/* declare new variables */

   double          dutycycle,
                   timeoff,
                   cntct,
                   pcrho,
                   pdpd2,
		   p180,
                   pwx,
                   srate,
                   crossp,
                   dipolr,
                   tpwrm,
                   rof2init,
                   d2init,
                   qpshft,
                   lvlshft,
                   ltoss,
                   lpdp,
                   lrof2;

   char            pdp[MAXSTR],
                   toss[MAXSTR],
                   xpol[MAXSTR];

/* set variables */

   cntct   = getval("cntct");
   p180    = getval("p180");
   pcrho = getval("pcrho");
   at      = getval("at");
   crossp  = getval("crossp");
   dipolr  = getval("dipolr");
   tpwrm = getval("tpwrm");
   pwx = getval("pwx");
   srate   = getval("srate");
   rof2init = getval("rof2");
   d2init = getval("d2");
   pdpd2 = getval("pdpd2");

   getstr("xpol",   xpol);
   getstr("pdp", pdp);
   getstr("toss", toss);

/*adjust for propagation delays in the sequence*/

   qpshft = 1.0e-6;
   lvlshft = 2.4e-6;
  
   ltoss = 0.0;
   lpdp = 0.0;
   lrof2 = 0.0;

   if (pdp[0]=='y') 
   {
      lpdp = lpdp + lvlshft;
   }
   else
   {
      if (toss[0]=='y')
      {
         ltoss = ltoss + lvlshft;
      }  
      else  
      {
         lrof2 = lrof2 + lvlshft;
      }
   }

/*adjust the rof2 delay and the d2 delay*/ 

   rof2 = rof2init - lrof2;
   if (rof2 < 0.0) rof2 = 0.0;
   
   if  (p180 > 0.0) 
   {
      d2 = d2init - rof1;
      if (d2 < 0.0) d2 = 0.0;
   }

/*set spin rate abort conditions for the sequence*/

   if (toss[0]=='y')
   {
      if (((0.0773 / srate) - 2.0 * pwx - qpshft) < 0.0)
      {
         fprintf(stdout, "spin rate is too fast for TOSS!\n");
         psg_abort(1);
      }
   } 

   if (pdp[0]=='y')
   {
      if (((1.0 / srate) - pwx - pdpd2 - qpshft - lpdp) < 0.0)
      {
         fprintf(stdout, "pdpd2 is too long for spin rate!\n");
         psg_abort(1);
      }
   }

   if (((toss[0] == 'y') || (pdp[0] == 'y')) && (srate < 500.0))
   {
      fprintf(stdout, "spin rate is too low for toss or dipolar dephasing!\n");
      psg_abort(1);
   }

/*set abort conditions for high dutycycle and dm='y'*/

   dutycycle =  cntct + pcrho + pw + d1 + at;
   timeoff = d1;
   if (dm[2] != 'y') timeoff = timeoff + at;
   if (p180 > 0.0) 
   {
      dutycycle = dutycycle + d2 + p180;
      timeoff = timeoff + d2;
   } 
   if (toss[0] == 'y')
   {
      dutycycle = dutycycle + 2.142/srate;
      if (dm[2] != 'y') timeoff = timeoff + 2.142/srate - 8*pwx; 
   }
   if (pdp[0] == 'y') 
   {
      dutycycle = dutycycle + 2.0/srate;
      timeoff = timeoff + pdpd2; 
   }
   dutycycle = timeoff/dutycycle;

   if ((dutycycle < 0.8) || (dm[0] == 'y'))
   {
      fprintf(stdout, "Duty cycle is %5.2f%%.\n", (1.0 - dutycycle) * 100.0);
      fprintf(stdout, "ABORT! The duty cycle must be less than 20%%.\n");
      psg_abort(1);
   }
  
/*begin pulse sequence*/

   if (xpol[0] == 'n') 
   {			

      settable(t1,4,table1);
      settable(t3,4,table3);
      settable(t4,4,table4);

      status(A);
      setreceiver(t1);
      obs_pw_ovr(TRUE);
      dec_pw_ovr(TRUE);       
      declvloff();
      decpwrf(dipolr);
      obspwrf(tpwrm);
      delay(d1);
 
      if(p180 > 0.0)               
      {
         decoff();
         if (dm[1] == 'y') decon();
	 rgpulse(p180, zero, rof1, 0.0);
	 delay(d2);
      }
      rcvroff();  
      rgpulse(pw, t1,rof1, 0.0);
      decoff();
      if (dm[2] == 'y') decon();
   }
    
   else
   {

      settable(t1,4,table1);
      settable(t2,4,table2);
      settable(t3,4,table3);
      settable(t4,4,table4);

      status(A);
      setreceiver(t1);
      obs_pw_ovr(TRUE);
      dec_pw_ovr(TRUE);
      declvloff();
      decpwrf(crossp);
      obspwrf(tpwrm);
      txphase(t3);
      delay(d1);

      if(p180 > 0.0)             
      {
         decrgpulse(p180, zero, rof1, 0.0);
	 delay(d2);
      }

      rcvroff();
      decphase(t2);
      delay(rof1);
      decon();
      delay(pw - 0.8e-6);
      decphase(zero);
      delay(0.8e-6);
      xmtron();
      delay(cntct);
      xmtroff();

/*optional spin lock for 13C T1rho*/
    
      if (pcrho > 0.0)
      {
         decoff();
         if (dm[1] == 'y') decon();
         xmtron();
         delay(pcrho);
         xmtroff();          
      }
      decpwrf(dipolr);   
      decoff();
      if (dm[2] == 'y')
      {
         decon();
      }
   }
        
/*optional interrupted decoupling for protonated carbon dephasing*/

   if (pdp[0] == 'y')
   {
      decoff();
      delay(pdpd2);
      decon();
      delay((1.0 / srate) - pwx - pdpd2 - qpshft - lpdp);
      txphase(t3);
      delay(qpshft);
      xmtron();
      delay(2.0 * pwx);
      xmtroff();
      delay((1.0 / srate) - pwx);
   }

/*optional pi pulses for suppression of sidebands - TOSS*/

   if (toss[0] == 'y')
   {			
      delay((0.1226 / srate) - pwx - qpshft - ltoss);
      txphase(t3);
      delay(qpshft);
      xmtron();
      delay(2.0*pwx);
      xmtroff(); 
      delay((0.0773 / srate) - 2.0 * pwx - qpshft);
      txphase(t4);
      delay(qpshft);
      xmtron();
      delay(2.0 * pwx);
      xmtroff();
      delay((0.2236 / srate) - 2.0 * pwx - qpshft);
      txphase(t3);
      delay(qpshft);
      xmtron();
      delay(2.0 * pwx);
      xmtroff();
      delay((1.0433 / srate) - 2.0 * pwx - qpshft);
      txphase(t4);
      delay(qpshft);
      xmtron();
      delay(2.0 * pwx);
      xmtroff();
      delay((0.7744 / srate) - pwx);
   }
   
/*begin acquisition*/

   delay(rof2);
   rcvron();
   delay(alfa+1.0/(2.0*fb));
   acquire(np,1/sw);
   decoff();
   declvlon(); 
}
