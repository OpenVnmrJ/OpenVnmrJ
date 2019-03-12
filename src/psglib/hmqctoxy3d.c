// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
/* 
 */
/* hmqctoxy3d - HMQC-TOCSY 3D sequence with presaturation option
              - written in hypercomplex phase sensitive mode only

   sequence:

   HMQC-TOCSY:

   status   : A|--------------B-----------------|C-|---D----|-E-
     1H     :   90-1/2J-        180        -1/2J-t2-spinlock-Acq (t3)
      X     :           90-t1/2-   -t1/2-90     -BB-         -BB
  phtable   :   t1      t2      t6       t6           t3      t4 or t5    

   Parameters:

         d2 = First evolution time
         d3 = second evolution time
        mix = TOCSY mixing time
     pwxlvl = power level for X pulses
     pwx    = 90 degree X pulse
     j1xh   = X-H coupling constant
     dpwr   = power level for X decoupling
     tpwr   = power level for H pulses
     pw     = 90 degree H hard pulse
     slpwr  = power level for spinlock
     slpw   = 90 degree H pulse for mlev17
     trim   = trim pulse preceeding mlev17
     phase  = 1,2: gives HYPERCOMPLEX (t1) acquisition;
     ni     = number of t1 increments
     phase2 = 1,2: gives HYPERCOMPLEX (t2) acquisition;
     ni2    = number of t2 increments
     satflg = 'y':  presaturation during satdly
     satfrq = presaturation frequency
     satdly = saturation time during the relaxation period
     satpwr = saturation power for all periods of presaturation with xmtr
    wdwfctr = multiplication "window" factor of slpw
    nullflg = TANGO nullflg flag for protons not attached to X
    hmqcflg = 'n': turns off HMQC part of the sequence

   Processing:  See manual entry.
*/

#include <standard.h>
extern int dps_flag;

static int 	phs1[8] = {0,0,0,0,1,1,1,1},
                phs2[8] = {0,2,0,2,1,3,1,3},
		phs3[8] = {0,0,2,2,1,1,3,3},
		phs4[8] = {0,2,0,2,1,3,1,3},
		phs5[8] = {0,0,0,0,1,1,1,1},
		phs6[8] = {0,0,0,0,1,1,1,1};

void mleva()
{
   double wdwfctr,window,slpw;
   wdwfctr=getval("wdwfctr");
   slpw = getval("slpw");
   window = (wdwfctr*slpw);
   txphase(v3); delay(slpw); 
   xmtroff(); delay(window); xmtron();
   txphase(v4); delay(2*slpw); 
   xmtroff(); delay(window); xmtron();
   txphase(v3); delay(slpw); 
}

void mlevb()
{
   double wdwfctr,window,slpw;
   wdwfctr=getval("wdwfctr");
   slpw = getval("slpw");
   window = (wdwfctr*slpw);
   txphase(v5); delay(slpw); 
   xmtroff(); delay(window); xmtron();
   txphase(v6); delay(2*slpw); 
   xmtroff(); delay(window); xmtron();
   txphase(v5); delay(slpw);
}


void pulsesequence()
{

   double  j1xh,
           cycles,
           trim,
	   d2corr,
	   d3corr,
	   hsgpwr,
           wdwfctr,
           window,
           slpwr,
           slpw,
           mix;
    int iphase,
        iphase2;
    char satflg[MAXSTR],
         sspul[MAXSTR],
         hmqcflg[MAXSTR],
         nullflg[MAXSTR];



    mix = getval("mix");
    slpwr = getval("slpwr");
    slpw = getval("slpw");
    trim = getval("trim");
    wdwfctr = getval("wdwfctr");
    window = (wdwfctr*slpw);
    hsgpwr = getval("hsgpwr");
    j1xh = getval("j1xh");
    getstr("satflg",satflg);
    getstr("sspul",sspul);
    getstr("hmqcflg",hmqcflg);
    getstr("nullflg",nullflg);
    tau = 1.0/(2.0*j1xh);
    cycles = (mix-trim)/(64.66*slpw+32*window);
    cycles = 2.0*(double)(int)(cycles/2.0);
    initval(cycles,v11);
    d2corr = pw+2.0e-6+(2*pwx/PI);
    d3corr = 2*POWER_DELAY + (2*pw/PI) + 1.0e-6;
    if (hmqcflg[0] == 'y')
	d3corr = d3corr - (2*pw/PI);
    iphase = (int) (getval("phase") + 0.5);
    iphase2 = (int)(getval("phase2") + 0.5);

    settable(t1,8,phs1);
    settable(t2,8,phs2);
    settable(t3,8,phs3);
    settable(t4,8,phs4);
    settable(t5,8,phs5);
    settable(t6,8,phs6);

    getelem(t1,ct,v1);
    getelem(t2,ct,v2);

    getelem(t3,ct,v3);
    add(v3,one,v4);
    add(v4,one,v5);
    add(v5,one,v6);

   if (hmqcflg[0] == 'y')
    getelem(t4,ct,oph);
   else
    getelem(t5,ct,oph);


    if (iphase == 2)
       incr(v2);
    if (iphase2 == 2)
       incr(v1);
 
    initval(2.0*(double)(((int)(d2*getval("sw1")+0.5)%2)),v14);
    initval(2.0*(double)(((int)(d3*getval("sw2")+0.5)%2)),v13);

    add(v1,v13,v1);
    add(v2,v14,v2);
    add(oph,v14,oph);
    add(oph,v13,oph);
   

/* BEGIN ACTUAL PULSE SEQUENCE CODE */
    status(A);
       decpower(pwxlvl);
       obspower(tpwr);

       if (sspul[0] == 'y')
       {
	zgradpulse(hsgpwr,0.01);
        rgpulse(pw,zero,rof1,rof1);
	zgradpulse(hsgpwr,0.01);
       }

       delay(d1);

       if (satflg[0] == 'y')
         {
          obspower(satpwr);
           if (satfrq != tof)
            obsoffset(satfrq);
           rgpulse(satdly,one,4.0e-5,1.0e-5);
           if (satfrq != tof)
            obsoffset(tof);
          obspower(tpwr);
          delay(4.0e-5);
         }

    status(B);
    if (hmqcflg[0] == 'y')
     {
       if (nullflg[0] == 'y')
         {
          rgpulse(pw,zero,rof1,rof1);
          delay(tau);
          simpulse(2.0*pw,2.0*pwx,zero,zero,rof1,rof1);
          delay(tau);
          rgpulse(1.5*pw,two,rof1,rof1);
	  zgradpulse(hsgpwr,0.002);
	  delay(1e-3);
	 }

       rcvroff();
       rgpulse(pw,v1,rof1,1.0e-6);
       delay(tau);
       decrgpulse(pwx,v2,1.0e-6,1.0e-6);
       if (d2 > 0.0)
        delay(d2/2.0 - d2corr);
       else
        delay(d2/2.0);
       rgpulse(2.0*pw,t6,1.0e-6,1.0e-6);
       if (d2 > 0.0)
        delay(d2/2.0 - d2corr);
       else
        delay(d2/2.0);
       decrgpulse(pwx,t6,1.0e-6,1.0e-6);
       delay(tau);
      }
     else
      {
       rcvroff();
       rgpulse(pw,v1,rof1,1.0e-6);
      }
       decpower(dpwr);

    status(C);
        if (d3>0.0)
          delay(d3 - d3corr);
        else
          delay(d3);
       
    status(D);

       if (mix > 0.0)
        {
	 obspower(slpwr);
	 if (dps_flag) 
		rgpulse(mix,zero,rof1,rof1);
	 else
	 {
	 xmtron();
	 txphase(v4); delay(trim);
          starthardloop(v11);
             mleva(); mlevb(); mlevb(); mleva();
             mlevb(); mlevb(); mleva(); mleva();
             mlevb(); mleva(); mleva(); mlevb();
             mleva(); mleva(); mlevb(); mlevb();
		txphase(v4); delay(0.67*slpw);
          endhardloop();
	 xmtroff();
	 }
	 obspower(tpwr);
        }
       delay(rof2 - POWER_DELAY);
       rcvron();

    status(E);
}
