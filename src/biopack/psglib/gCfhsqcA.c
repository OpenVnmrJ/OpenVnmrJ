/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* gCfhsqcA.c    "Fast C13 hsqc using 3919 watergate suppression"

   minimum step cycling is 2
   TROSY option is included. When TROSY is selected use nt=8
   This sequence was based on the gNfhsqc and modified for Carbon.
   It uses a 200 ppM adiabatic 180 degree 13C-pulses in the inept transfers.
   Nagarajan Murali: March 22, 2002.
   Auto-calibrated version, E.Kupce, 27.08.2002.

   The autocal and checkofs flags are generated automatically in Pbox_bio.h
   If these flags do not exist in the parameter set, they are automatically 
   set to 'y' - yes. In order to change their default values, create the flag(s) 
   in your parameter set and change them as required. 
   The available options for the checkofs flag are: 'y' (yes) and 'n' (no). 
   The offset (tof, dof, dof2 and dof3) checks can be switched off individually 
   by setting the corresponding argument to zero (0.0).
   For the autocal flag the available options are: 'y' (yes - by default), 
   'q' (quiet mode - suppress Pbox output), 'r' (read from file, no new shapes 
   are created), 's' (semi-automatic mode - allows access to user defined 
   parameters) and 'n' (no - use full manual setup, equivalent to the original code).

 */


#include <standard.h>
#include "Pbox_bio.h"               /* Pbox Bio Pack Pulse Shaping Utilities */

static double d2_init = 0.0;
static double   H1ofs=4.7, C13ofs=56.0, N15ofs=120.0, H2ofs=0.0;

static int phi1[4] = {1,1,3,3},
           phi2[2] = {0,2},
           phi3[8] = {0,0,0,0,2,2,2,2}, 
           phi4[16] = {0,0,0,0,0,0,0,0,2,2,2,2,2,2,2,2},
           rec[8] = {0,2,2,0,2,0,0,2},
/* phase cycling for TROSY */
           phT1[8] = {1,1,1,1,3,3,3,3},
           phT2[8] = {0,2,3,1,0,2,3,1},
           phT3[8] = {0,0,0,0,2,2,2,2},
           recT[8] = {0,2,3,1,0,2,1,3}; 

static shape stC200;

pulsesequence()
{

int     t1_counter;
char	    N15refoc[MAXSTR],		/* N15 pulse in middle of t1*/
            TROSY[MAXSTR];
double
   gzlvl3=getval("gzlvl3"),
   tau1, tauxh,
   gt3=getval("gt3"),
   cor=getval("cor")*1.0e-6,
   JCH = getval("JCH"),
   pwN = getval("pwN"),
   pwNlvl = getval("pwNlvl"),      
   sw1 = getval("sw1"),
   pwClvl = getval("pwClvl"), 	  	        /* coarse power for C13 pulse */
   pwC = getval("pwC"),     	      /* C13 90 degree pulse length at pwClvl */
   rf0,            	          /* maximum fine power when using pwC pulses */
   rfst,	                           /* fine power for the stCall pulse */
   bw, pws, ofs, ppm, nst,                       /* temporary Pbox parameters */
   compC = getval("compC");       /* adjustment for C13 amplifier compression */

    getstr("N15refoc",N15refoc);
    getstr("TROSY",TROSY);

/* maximum fine power for pwC pulses (and initialize rfst) */
	rf0 = 4095.0;    rfst=0.0;

      setautocal();                        /* activate auto-calibration flags */ 
        
      if (autocal[0] == 'n') 
      {
        /* 180 degree adiabatic C13 pulse from 0 to 200 ppm */
        rfst = (compC*4095.0*pwC*4000.0*sqrt((30.0*sfrq/600.0+7.0)/0.35));   
	  rfst = (int) (rfst + 0.5);
	  if ( 1.0/(4000.0*sqrt((30.0*sfrq/600.0+7.0)/0.35)) < pwC )
           { text_error( " Not enough C13 RF. pwC must be %f usec or less.\n", 
	    (1.0e6/(4000.0*sqrt((30.0*sfrq/600.0+7.0)/0.35))) ); psg_abort(1); }
      }
      else        /* if autocal = 'y'(yes), 'q'(quiet), r(read), or 's'(semi) */
      {
        if(FIRST_FID)                                            /* call Pbox */
        {
          ppm = getval("dfrq"); ofs = 0.0;   pws = 0.001;  /* 1 ms long pulse */
          bw = 200.0*ppm;       nst = 1000;          /* nst - number of steps */
          stC200 = pbox_makeA("stC200", "sech", bw, pws, ofs, compC*pwC, pwClvl, nst);
          ofs_check(H1ofs, C13ofs, N15ofs, H2ofs);
        }
        rfst = stC200.pwrf;
      }

   initval(0.0,v1);
   initval(3.0,v2);
   initval(1.0,v3);
   initval(2.0,v4);

/* check validity of parameter range */

    if((dm2[A] == 'y' || dm2[B] == 'y' || dm2[C] == 'y') )
	{
	printf("incorrect Dec2 decoupler flags!  ");
	psg_abort(1);
    } 

    if( dpwr2 > 50)
    {
	printf("don't fry the probe, dpwr too large!  ");
	psg_abort(1);
    }


/* LOAD VARIABLES */

if (TROSY[A] == 'y')
 {settable(t1, 8, phT1);
  settable(t2, 8, phT2);
  settable(t3, 8, phT3);
  settable(t4,16, phi4);
  settable(t5, 8, recT);}
else
 {settable(t1, 4, phi1);
  settable(t2, 2, phi2);
  settable(t3, 8, phi3);
  settable(t4, 16, phi4);
  settable(t5, 8, rec);}

/* INITIALIZE VARIABLES */

    tauxh = ((JCH != 0.0) ? 1/(4*(JCH)) : 2.25e-3);

/* Phase incrementation for hypercomplex data */

   if ( phase1 == 2 )     /* Hypercomplex in t1 */
   {
        tsadd(t2, 1, 4); 
   } 


/* calculate modification to phases based on current t1 values
   to achieve States-TPPI acquisition */
 
 
   if(ix == 1)
      d2_init = d2;
      t1_counter = (int) ( (d2-d2_init)*sw1 + 0.5);

      if(t1_counter %2) {
        tsadd(t2,2,4);
        tsadd(t5,2,4);
      }
   tau1 = d2;
   tau1 = tau1/2.0;

/*sequence starts!!*/

   status(A);
   obspower(tpwr);
   dec2power(pwNlvl);
   decpower(pwClvl);
   decpwrf(rfst);
   delay(d1);
   rcvroff();
   status(B);

  rgpulse(pw, v1, 0.0, 0.0);

  zgradpulse(0.3*gzlvl3,gt3);

  txphase(zero);
  dec2phase(zero);

  delay(tauxh-gt3- WFG2_START_DELAY - 0.5e-3 + 70.0e-6);               /* delay=1/4J(XH)   */

  sim3shaped_pulse("","stC200","",2*pw,1.0e-3,0.0,zero,zero,zero,0.0,0.0);

  zgradpulse(0.3*gzlvl3,gt3);

  decphase(t2);
  delay(tauxh-gt3- WFG2_START_DELAY - 0.5e-3 + 70.0e-6 );               /* delay=1/4J(XH)   */
if (TROSY[A] == 'y')

 {
  rgpulse(pw, v3, 0.0,0.0);

  zgradpulse(-0.5*gzlvl3,gt3);
  decpwrf(rf0); delay(200.0e-6);

  decrgpulse(pwC, t2, 0.0, 0.0);

  txphase(t1); decphase(zero);
  if (tau1>0.0)
   {
        if ( (N15refoc[A]=='y') && (tau1 > (pwN/2.0 +2.0*pwC/PI) ) )
           {delay(tau1 - pwN -2.0*pwC/PI);     
            dec2rgpulse(2.0*pwN, zero, 0.0, 0.0);
            delay(tau1 - pwN -2.0*pwC/PI);}     
        else
           { if (tau1>2.0*pwC/PI) 
              delay(2.0*tau1-4.0*pwC/PI);
             else
               delay(2.0*tau1);
           }
   }

  rgpulse(pw, t1, 0.0,0.0);

  decpwrf(rfst);
  zgradpulse(0.3*gzlvl3,gt3);
  delay(tauxh-gt3-WFG2_START_DELAY - 0.5e-3 + 70.0e-6);
  sim3shaped_pulse("","stC200","",2*pw,1.0e-3,0.0,zero,zero,zero,0.0,0.0);  zgradpulse(0.3*gzlvl3,gt3);
  delay(tauxh-gt3-WFG2_START_DELAY - 0.5e-3 + 70.0e-6);
  sim3pulse(pw,pwC,0.0,two,zero,one,0.0,0.0); 
   zgradpulse(gzlvl3,gt3);
   txphase(v4);

   delay(tauxh-gt3-WFG2_START_DELAY - 0.5e-3 + 70.0e-6);

   rgpulse(pw*0.231,v4,0.0,0.0);
   delay(d3);
   rgpulse(pw*0.692,v4,0.0,0.0);
   delay(d3);
   rgpulse(pw*1.462,v4,0.0,0.0);

   delay(d3/2);
   sim3shaped_pulse("","stC200","",0.0,1.0e-3,0.0,zero,zero,zero,0.0,0.0);
   txphase(v1);
   delay(d3/2);

   rgpulse(pw*1.462,v1,0.0,0.0);
   delay(d3);
   rgpulse(pw*0.692,v1,0.0,0.0);
   delay(d3);
   rgpulse(pw*0.231,v1,0.0,0.0);
   dec2phase(t3);
   zgradpulse(gzlvl3,gt3); decpwrf(rf0);
   delay(tauxh-gt3-WFG2_START_DELAY - 0.5e-3 + 70.0e-6);
   decrgpulse(pwC, t3, 0.0,0.0);}

else
   { 
  rgpulse(pw, t1, 0.0,0.0);

  zgradpulse(0.5*gzlvl3,gt3); decpwrf(rf0);
  delay(200.0e-6);

  decrgpulse(pwC, t2, 0.0, 0.0);

  txphase(t4); decphase(zero);
  if (tau1>0.0)
   {
  	if ( (N15refoc[A]=='y') && (tau1 > (pwN+2.0*pwC/PI)) )
           {delay(tau1 - pwN -2.0*pwC/PI);     
            sim3pulse(2*pw,0.0,2.0*pwN, zero, zero, zero,0.0, 0.0);
            delay(tau1 - pwN -2.0*pwC/PI);}
	else
            {
             if (tau1 > (pw +2.0*pwC/PI))
              {delay(tau1-pw -2.0*pwC/PI); rgpulse(2.0*pw, t4, 0.0, 0.0); decphase(t3); delay(tau1-pw -2.0*pwC/PI);}
             else
              {delay(tau1); decphase(t3); delay(tau1);} 
            }             
   }
  decrgpulse(pwC, t3, 0.0, 0.0);

  zgradpulse(0.5*gzlvl3,gt3);
  delay(200.0e-6);

  rgpulse(pw, v4, 0.0,0.0);

  decphase(zero);
  
  zgradpulse(gzlvl3,gt3);
  decpwrf(rfst);
  txphase(zero);
  dec2phase(zero);

  delay(tauxh-gt3- WFG2_START_DELAY - 0.5e-3 + 70.0e-6);               /* delay=1/4J(XH)   */
   rgpulse(pw*0.231,v2,0.0,0.0);
   delay(d3);
   rgpulse(pw*0.692,v2,0.0,0.0);
   delay(d3);
   rgpulse(pw*1.462,v2,0.0,0.0);

   delay(d3/2.0);
  sim3shaped_pulse("","stC200","",0.0,1.0e-3,0.0,zero,zero,zero,0.0,0.0);
   delay(d3/2.0);
  
   rgpulse(pw*1.462,v3,0.0,0.0);
   delay(d3);
   rgpulse(pw*0.692,v3,0.0,0.0);
   delay(d3);
   rgpulse(pw*0.231,v3,0.0,0.0);
   zgradpulse(gzlvl3,gt3);
   decpwrf(rf0);
   delay(tauxh-gt3- WFG2_START_DELAY - 0.5e-3 + 70.0e-6 +cor );}               /* delay=1/4J(XH)   */
   decpower(dpwr);
   setreceiver(t5);
status(C);
   rcvron();

}
