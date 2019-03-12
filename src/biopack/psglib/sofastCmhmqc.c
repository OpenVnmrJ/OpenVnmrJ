/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*************************************************/
/*   SOFAST-HMQC methyl 1H-13C experiment        */
/*************************************************/
/*   P. Schanda, E. Kupce and B. Brutscher       */
/*    JBiomolNMR (2005) 33:199-211               */
/*************************************************/
/* optional flags
    constant-time evolution    (CT_flg)
    homonuclear 13C decoupling (Cdecflg,Cdecseq)

 Note: decoupling of carbons attached to methyls
 can induce Bloch-Siegert shifts. These can be
 somewhat compensated for by placing an identical
 decoupling band upfield from the observed methyl
 carbon region at the same distance away in Hz.
 Observed 13C methyl shifts will be displaced from
 the true shift by the magnitude of the BS shift
 and this can be significant if too high a power
 or bandwidth is used for the decoupling in 
 evolution.

 Decoupling can also perturb the methyl evolution
 if the bandwidth of the decoupling is too large.
 Some carbons coupled to methyl carbons have shifts
 close to the methyl carbon region so these may not
 be possible to decouple.

 The setup macro creates Pbox waveforms, including 
 the decoupling waveform used in evolution. The
 Pbox parameter values can be changed for enhancing
 specific performance.

*/

#include <standard.h>



static int 

   phi1[2] = {0,2},
   phi2[2] = {0,2};


void pulsesequence()
{
   char   CT_flg[MAXSTR],	/* Constant time flag */
          shname1[MAXSTR],
	  shname2[MAXSTR],
	  shname3[MAXSTR],
	  f1180[MAXSTR],
          Cdecflg[MAXSTR],
          Cdecseq[MAXSTR],
          grad_flg[MAXSTR];     /*gradient flag */

   int    t1_counter,
          phase;


   double d2_init=0.0,
          adjust = getval("adjust"),
          gzlvl1 = getval("gzlvl1"),
          gzlvl2 = getval("gzlvl2"), 
          gt1 = getval("gt1"),
          gt2 = getval("gt2"),
          shlvl1 = getval("shlvl1"),
          shlvl2 = getval("shlvl2"),
          shlvl3 = getval("shlvl3"),
          shdmf2 = getval("shdmf2"),
          shpw1 = getval("shpw1"),
          shpw2 = getval("shpw2"),
          shpw3 = getval("shpw3"),
	  pwClvl = getval("pwClvl"),
          pwC = getval("pwC"),
          dpwr = getval("dpwr"),
          CT_delay = getval("CT_delay"),
          d2 = getval("d2"),
          tau1 = getval("tau1"),
          tauch = getval("tauch");


   getstr("shname1", shname1);
   getstr("shname2", shname2);
   getstr("shname3", shname3);
   getstr("CT_flg", CT_flg);
   getstr("grad_flg",grad_flg);
   getstr("f1180",f1180);
   getstr("Cdecflg",Cdecflg);
   getstr("Cdecseq",Cdecseq);


  phase = (int) (getval("phase") + 0.5);
   
   settable(t1,2,phi1);
   settable(t2,2,phi2);


  if (phase == 1) ;
  if (phase == 2) tsadd(t1,1,4);

    tau1 = d2;
    if((f1180[A] == 'y') && (ni > 1.0))
        { tau1 += ( 1.0 / (2.0*sw1) ); if(tau1 < 0.2e-6) tau1 = 0.0; }
    tau1 = tau1;
  
    if (f1180[0] == 'y')  tau1 = tau1-pwC*4.0/3.0;

if(CT_flg[0] == 'y')
 {
     if ( (ni/sw1) > (CT_delay-pwC*8.0/3.0))
  { text_error( " ni is too big. Make ni equal to %d or less.\n",
      ((int)((CT_delay-pwC*8.0/3.0)*sw1)) );                psg_abort(1); }
 } 


   if( ix == 1) d2_init = d2;
   t1_counter = (int) ( (d2-d2_init)*sw1 + 0.5 );
   if(t1_counter % 2)
        { tsadd(t1,2,4); tsadd(t2,2,4); }


   status(A);

   decpower(pwClvl);
   decoffset(dof);
   obsoffset(tof);

   zgradpulse(gzlvl2, gt2);
   lk_sample();
   delay(1.0e-4);
   delay(d1-gt2);
   if (ix < (int)(2.0*ni)) lk_hold(); /*force lock sampling at end of experiment*/

   obspower(shlvl1);
   shaped_pulse(shname1,shpw1,zero,2.0e-4,2.0e-6);
   zgradpulse(gzlvl1, gt1);
   delay(1.0e-4);

  if (ni == 0)
  {
       delay(tauch-gt1-1.0e-4-WFG_START_DELAY+pwC*4.0-adjust);
       obspower(shlvl2);
     shaped_pulse(shname2,shpw2,zero,2.0e-6,2.0e-6);
       obspower(shlvl1);
     decrgpulse(pwC,t1,0.0,0.0);
     decrgpulse(pwC*2.0,zero,0.0,0.0);
     decrgpulse(pwC,zero,0.0,0.0);
  }

else
  {
  delay(tauch-gt1-1.0e-4-adjust);
  obspower(shlvl2);
  status(B);

  if(CT_flg[0] == 'y')
   {
    /*************************************************/
    /****           CT EVOLUTION            **********/
    /*************************************************/

     decrgpulse(pwC,t1,0.0,0.0);
     delay((CT_delay-tau1)*0.25-pwC*2.0/3.0);
     decpower(shlvl3);
     decshaped_pulse(shname3,shpw3,zero,0.0,0.0);
     delay((CT_delay+tau1)*0.25-shpw2*0.5);
     shapedpulse(shname2,shpw2,zero,0.0,0.0);
     delay((CT_delay+tau1)*0.25-shpw2*0.5);
     decshaped_pulse(shname3,shpw3,zero,0.0,0.0);
     decpower(pwClvl);
     delay((CT_delay-tau1)*0.25-pwC*2.0/3.0);
     decrgpulse(pwC,zero,0.0,0.0);
    }
   
   else
   {
    /*************************************************/
    /****     REAL-TIME EVOLUTION           **********/
    /*************************************************/
     if ((tau1) > shpw2)
    {
     decrgpulse(pwC,t1,0.0,0.0);
      if(Cdecflg[0] == 'y')
      {
      decpower(getval("Cdecpwr"));
      decprgon(Cdecseq, 1.0/getval("Cdecdmf"), getval("Cdecres"));
      decon();
      }
     delay((tau1-shpw2)*0.5);

    xmtrphase(zero);
    xmtron();
    obsunblank();
    obsprgon(shname2,1/shdmf2,9.0);
    delay(shpw2);
    obsprgoff();
    obsblank();
    xmtroff();

     delay((tau1-shpw2)*0.5);
      if(Cdecflg[0] == 'y')
      {
      decoff(); decprgoff();
      decpower(pwClvl);
      }
     decrgpulse(pwC,zero,0.0,0.0);
    }
   else
    {
    xmtrphase(zero);
    xmtron();
    obsunblank();
    obsprgon(shname2,1/shdmf2,9.0);
    delay((shpw2-tau1-pwC*2.0)*0.5);

    decrgpulse(pwC,t1,0.0,0.0);
      if(Cdecflg[0] == 'y')
      {
      decpower(getval("Cdecpwr"));
      decprgon(Cdecseq, 1.0/getval("Cdecdmf"), getval("Cdecres"));
      decon();
      delay(tau1);
      decoff(); decprgoff();
      decpower(pwClvl);
      }
    else
      delay(tau1);
    decrgpulse(pwC,zero,0.0,0.0);
    delay((shpw2-tau1-pwC*2.0)*0.5);
    obsprgoff();
    obsblank();
    xmtroff();
    }
  }

       obspower(shlvl1);
   status(A);


           zgradpulse(gzlvl1, gt1);
           delay(1.0e-4);
       delay(tauch-gt1-1.0e-4-POWER_DELAY);

   decpower(dpwr);

   status(C);
   setreceiver(t2);

}
}

