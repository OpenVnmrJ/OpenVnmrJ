/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* based on hmqctoxy3d - HMQC-TOCSY 3D sequence with wet option

 	GGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGG
 	G For qualitative measurement of torsion angle gamma G
 	GGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGG

   sequence:

   HMQC-TOCSY:

   status   : A|--------------B-----------------|C-|---D----|-E-
     1H     :   90-1/2J-        180        -1/2J-t2-spinlock-Acq (t3)
    13C     :           90-t1/2-   -t1/2-90     -BB-         -BB
  phtable   :   t1      t2      t6       t6           t3      t4 or t5    

   Parameters:

         d2 = First evolution time
         d3 = second evolution time
        mix = TOCSY mixing time
     pwClvl = power level for X pulses
     pwC    = 90 degree X pulse
     JCH   = X-H coupling constant
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
     wet    = 'y':  wet option for H2O-sample (D2O is better!!!)
    wdwfctr = multiplication "window" factor of slpw
    hmqcflg = 'n': turns off HMQC part of the sequence

	Modified for RnaPack, Peter Lukavsky, June 2002, Stanford

	NOTE dof MUST BE SET AT 110ppm ALWAYS

	Best to use with D2O-sample!!
        Decoupling: dm='nyy'
	Use with HCP or HCN probe: all couplings (15N-1H/13C or 31P-H1/13C) 
	with ribose nuclei are < ~10Hz
	Second decoupler (13P or 15N) not necessary: dm2='nnn' (dm2='nny' optional)
        sw1(13C)=50ppm and sw2(1H)=sw or smaller

*/

#include <standard.h>
extern int dps_flag;

/* Chess - CHEmical Shift Selective Suppression */
void Chess(pulsepower,pulseshape,duration,phase,rx1,rx2,gzlvlw,gtw,gswet)
double pulsepower,duration,rx1,rx2,gzlvlw,gtw,gswet;
  codeint phase;
  char* pulseshape;
{
  obspwrf(pulsepower);
  shaped_pulse(pulseshape,duration,phase,rx1,rx2);
  zgradpulse(gzlvlw,gtw);
  delay(gswet);
}

/* Wet4 - Water Elimination */
void Wet4(phaseA,phaseB)
  codeint phaseA,phaseB;
{
  double finepwr,gzlvlw,gtw,gswet,gswet2,wetpwr,wetpw,dz;
  char   wetshape[MAXSTR];
  getstr("wetshape",wetshape);    /* Selective pulse shape (base)  */
  wetpwr=getval("wetpwr");        /* User enters power for 90 deg. */
  wetpw=getval("wetpw");        /* User enters power for 90 deg. */
  dz=getval("dz");
  finepwr=wetpwr-(int)wetpwr;     /* Adjust power to 152 deg. pulse*/
  wetpwr=(double)((int)wetpwr);
  if (finepwr==0.0) {wetpwr=wetpwr+5; finepwr=4095.0; }
  else {wetpwr=wetpwr+6; finepwr=4095.0*(1-((1.0-finepwr)*0.12)); }
  rcvroff();
  obspower(wetpwr);         /* Set to low power level        */
  gzlvlw=getval("gzlvlw");      /* Z-Gradient level              */
  gtw=getval("gtw");            /* Z-Gradient duration           */
  gswet=getval("gswet");        /* Post-gradient stability delay */
  gswet2=getval("gswet2");        /* Post-gradient stability delay */
  Chess(finepwr*0.5059,wetshape,wetpw,phaseA,20.0e-6,rof2,gzlvlw,gtw,gswet);
  Chess(finepwr*0.6298,wetshape,wetpw,phaseB,20.0e-6,rof2,gzlvlw/2.0,gtw,gswet);
  Chess(finepwr*0.4304,wetshape,wetpw,phaseB,20.0e-6,rof2,gzlvlw/4.0,gtw,gswet);
  Chess(finepwr*1.00,wetshape,wetpw,phaseB,20.0e-6,rof2,gzlvlw/8.0,gtw,gswet2);
  obspower(tpwr); obspwrf(tpwrf);     /* Reset to normal power level   */
  rcvron();
  delay(dz);
}
/*
static int 	phs1[8] = {0,0,0,0,1,1,1,1},
                phs2[8] = {0,2,0,2,1,3,1,3},
		phs3[8] = {0,0,2,2,1,1,3,3},
		phs4[8] = {0,2,0,2,1,3,1,3},
		phs5[8] = {0,0,0,0,1,1,1,1},
		phs6[8] = {0,0,0,0,1,1,1,1};
*/

static int      phs1[4] = {0,0,0,0},
                phs2[4] = {0,2,0,2},
                phs3[4] = {0,0,2,2},
                phs4[4] = {0,2,0,2},
                phs5[4] = {0,0,0,0},
                phs6[4] = {0,0,0,0};


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

   double  JCH,
	   pwC,
	   pwClvl,
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
char     sspul[MAXSTR],
         hmqcflg[MAXSTR],
	 wet[MAXSTR];      /* Flag to select optional WET water suppression */


    
    mix = getval("mix");
    slpwr = getval("slpwr");
    slpw = getval("slpw");
    trim = getval("trim");
    wdwfctr = getval("wdwfctr");
    window = (wdwfctr*slpw);
    hsgpwr = getval("hsgpwr");
    pwC = getval("pwC");
    pwClvl = getval("pwClvl");
    JCH = getval("JCH");
    getstr("sspul",sspul);
    getstr("hmqcflg",hmqcflg);
    getstr("wet",wet);
    tau = 1.0/(2.0*JCH);
    cycles = (mix-trim)/(64.66*slpw+32*window);
    cycles = 2.0*(double)(int)(cycles/2.0);
    initval(cycles,v11);
    d2corr = pw+2.0e-6+(2*pwC/PI);
    d3corr = 2*POWER_DELAY + (2*pw/PI) + 1.0e-6;
    if (hmqcflg[0] == 'y')
	d3corr = d3corr - (2*pw/PI);
    iphase = (int) (getval("phase") + 0.5);
    iphase2 = (int)(getval("phase2") + 0.5);

    settable(t1,4,phs1);
    settable(t2,4,phs2);
    settable(t3,4,phs3);
    settable(t4,4,phs4);
    settable(t5,4,phs5);
    settable(t6,4,phs6);

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
        obsoffset(tof);
        decoffset(dof - (30*dfrq));	/* 80ppm - middle of ribose region */
        dec2offset(dof2);
        obspower(tpwr);
        decpower(pwClvl);
        dec2power(dpwr2);

	delay(0.25);

       if (sspul[0] == 'y')
       {
	zgradpulse(hsgpwr,0.01);
        rgpulse(pw,zero,rof1,rof1);
	zgradpulse(hsgpwr,0.01);
       }

       delay(d1 - 0.25);

  if (wet[A] == 'y') {
     Wet4(zero,one);
  }

    if (hmqcflg[0] == 'y')
     {
       rcvroff();
       rgpulse(pw,v1,rof1,1.0e-6);
       delay(tau);
       decrgpulse(pwC,v2,1.0e-6,1.0e-6);
       if (d2 > 0.0)
        delay(d2/2.0 - d2corr);
       else
        delay(d2/2.0);
       rgpulse(2.0*pw,t6,1.0e-6,1.0e-6);
       if (d2 > 0.0)
        delay(d2/2.0 - d2corr);
       else
        delay(d2/2.0);
       decrgpulse(pwC,t6,1.0e-6,1.0e-6);
       delay(tau);
      }
     else
      {
       rcvroff();
       rgpulse(pw,v1,rof1,1.0e-6);
      }
       decpower(dpwr);

    status(B);
        if (d3>0.0)
          delay(d3 - d3corr);
        else
          delay(d3);
       
    status(A);

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

    status(C);
}
