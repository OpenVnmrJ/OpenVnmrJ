// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
/*  HMBCRELAY - From T. Spang & P. Bigler, MRC 2004; 42 p 55-60 */
/* R Crouch  10 June 2004 */
/* B Heise  07 April 2010 - include multi-freq. PRESAT */

/* Process as sum [ wft2d(1,0,1,0,0,1,0,1) ] for nJch or as
    difference [ wft2d(1,0,-1,0,0,1,0,-1) ] for 2Jch

  Typical Parameter Values (Performa II or Triax Scale down for performa I))
  gzlvl1 = 6000
  gzlvl3 = 3000

  Transfer Delays
  j1xh - 140 
  jnxh - 8
  jhh - 6  Calculated delay is 1/(16*jhh) NOT 1/(4*jhh).
  fad ='y' - Provided as a convenience only.
  shapedpulse = 'y' for on the fly adiabatic 13C 180s
  shapedpulse = 'n' used simple hard 180s
  phase=1,2  - Must be acquired with with this phase array to afford 2Jch selective editing.

  2Jch dependent more upon jhh than jnxh because of the pseudo-cosy relay step.
  Use sqsinebell window function in both dimensions.
*/

#include <standard.h>
#include <chempack.h>

static double d2_init = 0.0;

static int	ph1[2] = {0,2},
                ph2[2] = {2,0},
                ph5[1] = {0},
                ph6[2] = {0,2};
		
pulsesequence()

{
   double   pwxlvl, pwx, gtE,EDratio, gzlvlE, gstab,
                sw1, j1xh, jnxh, jhh, tau4, taumb, tauhh,
                 phase, hsgt, hsglvl, satdly, satpwr, satfrq, shapedpower;

  int          iphase, t1_counter;

  char       sspul[MAXSTR], fad[MAXSTR],  satmode[MAXSTR],
                shapedpulse[MAXSTR], comm[MAXSTR];

  gtE = getval("gtE");
  gzlvlE = getval("gzlvlE");
  EDratio = getval("EDratio");
  pwxlvl = getval("pwxlvl");
  pwx    = getval("pwx");
  shapedpower = getval("shapedpower");
  sw1 = getval("sw1");
  hsglvl = getval("hsglvl");
  hsgt = getval("hsgt");
  gstab = getval("gstab");
  j1xh = getval("j1xh");
  jnxh = getval("jnxh");
  jhh = getval("jhh");
  phase = getval("phase");
  satdly = getval("satdly");
  satpwr = getval("satpwr");
  satfrq = getval("satfrq");

  getstr("sspul",sspul);
  getstr("satmode",satmode);
  getstr("fad",fad);
  getstr("shapedpulse",shapedpulse);

//synchronize gradients to srate for probetype='nano'
//   Preserve gradient "area"
        gtE = syncGradTime("gtE","gzlvlE",0.5);
        gzlvlE = syncGradLvl("gtE","gzlvlE",0.5);

  /* construct a command line for Pbox */

  if (shapedpulse[0] == 'y')
  {
    if((getval("arraydim") < 1.5) || (ix==1))        /* execute only once */
     { sprintf(comm, "Pbox ad180 -w \"cawurst-10 %.1f/%.6f\" -s 1.0 -0\n",
             1.0/pwx, 30*pwx);
             system(comm);                         /* create adiabatic 180 pulse */
     }
   }

   if (ix == 1)
     d2_init = d2;
   t1_counter = (int)((d2-d2_init)*sw1 + 0.5);

  tau4 = 1/(4*j1xh);
  taumb = 1/(2*jnxh);
  tauhh = 1/(16*jhh);  /* enter jhh as typical number .. say 6 */

  iphase = (int) (phase + 0.5);

  settable(t1,2,ph1);
  settable(t2,2,ph2);
  settable(t5,1,ph5);
  settable(t6,2,ph6);

  if (iphase == 2)
      tsadd(t1,2,4);

  if (fad[0] == 'y')
   {
     if (t1_counter %2)
      {
        tsadd(t1,2,4);
        tsadd(t2,2,4);
        tsadd(t6,2,4);
      }
   }

  status(A);
  rcvroff();
  decpower(pwxlvl);
  obspower(tpwr);

   if (getflag("sspul"))
        steadystate();

   if (satmode[0] == 'y')
     {
        if ((d1-satdly) > 0.02)
                delay(d1-satdly);
        else
                delay(0.02);
        if (getflag("slpsat"))
                shaped_satpulse("relaxD",satdly,v4);
        else
                satpulse(satdly,v4,rof1,rof1);
     }
   else
        delay(d1);

   if (getflag("wet")) 
      wet4(zero,one);

  decpower(pwxlvl);
  obspower(tpwr);

  txphase(zero);
  decphase(zero);
  
  rgpulse(pw, zero, rof1, rof1);
  if (shapedpulse[0] == 'y')
     decpower(shapedpower);
  delay(tau4);
  if (shapedpulse[0] == 'y')
    { 
     simshaped_pulse("","ad180",2*pw,30*pwx,zero, zero, 2e-6, 2e-6); 
     decpower(pwxlvl);
   }
  else
    { simpulse(2*pw, 2*pwx, zero, zero, 2e-6, 2e-6); }
  delay(tau4);
  simpulse(pw, pwx, zero, t1, rof1, rof1);
  decphase(zero);

  delay(tauhh);
  
  zgradpulse(gzlvlE,gtE/2.0); 
  delay(d2/2);
  rgpulse(pw, zero, gstab - pw, 2e-6);
  rgpulse(2*pw, one, 2e-6, 2e-6);
  rgpulse(pw, zero, 2e-6, gstab - pw);
  delay(d2/2);
  zgradpulse(gzlvlE,gtE/2.0);
  delay(tauhh);

  decpulse(pwx, zero);
  if (shapedpulse[0] == 'y')
     decpower(shapedpower);
  delay(tau4);
  if (shapedpulse[0] == 'y')
    { simshaped_pulse("","ad180",2*pw,30*pwx,zero, zero, 2e-6, 2e-6); }
  else
    { simpulse(2*pw, 2*pwx, zero, zero, 2e-6, 2e-6); }
  txphase(one);
  delay(tau4);
  rgpulse(pw, one, 2e-6, 2e-6);
  txphase(zero);
  delay(tau4);
  if (shapedpulse[0] == 'y')
    { 
     simshaped_pulse("","ad180",2*pw,30*pwx,zero, zero, 2e-6, 2e-6); 
     decpower(pwxlvl);
   }
  else
    { simpulse(2*pw, 2*pwx, zero, zero, 2e-6, 2e-6); }
  delay(tau4);
  decpulse(pwx, zero);

  delay(taumb);

  decrgpulse(pwx, t2, 0.0, gstab);
  zgradpulse(gzlvlE,gtE/2.0);
  delay(d2/2);
  rgpulse(2*pw, zero, gstab, gstab);
  delay(d2/2);
  zgradpulse(gzlvlE,gtE/2.0);
  decrgpulse(pwx, t5, gstab, 3e-6);
  zgradpulse(2*gzlvlE/EDratio,gtE/2.0);
  delay(gstab);
  decpower(dpwr);
  setreceiver(t6);
  rcvron();
  status(B);
}
