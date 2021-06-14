/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef CHEMPACK_H
#define CHEMPACK_H

#include "group.h"

static double syncGrad(gT,gL,Mf,sR,Rv)
char *gT, *gL, *sR, *Rv;
double Mf;
{
   double tt, ll, sr, garea, reps;
   sr = getval(sR);
   tt = getval(gT)*Mf;
   ll = getval(gL);
   garea = tt*ll;
   if (sr > 199)
   {
     reps = floor((tt*sr)+0.5);
     if ((reps == 0.0) || (reps < 0.0))
        reps = 1.0;
     tt = reps/sr;
     if (ll > 0.0)
       ll = floor((garea/tt)+0.5);
     else
       ll = -floor((-garea/tt)+0.5);
   }
   tt = tt/Mf;
   if (!strcmp(Rv,"duration"))
        return(tt);
   else  // if (!strcmp(Rv,"level"))
        return(ll);
}

double syncGradTime(gTime,gLevel,Mfactor)
char *gTime, *gLevel;
double Mfactor;
{
   double Tt = getval(gTime);
   char probetype[MAXSTR];
   P_getstring(GLOBAL,"probetype",probetype,1,255);
   if (!strcmp(probetype,"nano"))
        Tt = syncGrad(gTime,gLevel,Mfactor,"srate","duration");
   return(Tt);
}

double syncGradLvl(gTime,gLevel,Mfactor)
char *gTime, *gLevel;
double Mfactor;
{
   double Ll = getval(gLevel);
   char probetype[MAXSTR];
   P_getstring(GLOBAL,"probetype",probetype,1,255);
   if (!strcmp(probetype,"nano"))
        Ll = syncGrad(gTime,gLevel,Mfactor,"srate","level");
   return(Ll);
}

void satpulse(saturation,phase,rx1,rx2)
double saturation, rx1, rx2;
codeint phase;
{
  double satpwr,
	 satfrq;
  satpwr = getval("satpwr");
  satfrq = getval("satfrq");

	obs_pw_ovr(TRUE);
       obspower(satpwr);
        if (satfrq != tof)
         obsoffset(satfrq);
        rgpulse(saturation,phase,rx1,rx2);
        if (satfrq != tof)
         obsoffset(tof);
       obspower(tpwr);
	obs_pw_ovr(FALSE);

}

void steadystate()
{
  double hsglvl,
	hsgt;
  char PFGflg[MAXSTR];

  getstr("PFGflg",PFGflg);
  hsglvl=getval("hsglvl");
  hsgt=getval("hsgt");

	if (PFGflg[0] == 'y')
        {
         zgradpulse(hsglvl,hsgt);
         rgpulse(pw,zero,rof1,rof1);
         zgradpulse(hsglvl,hsgt);
        }
        else
        {
        obspower(tpwr-12);
        rgpulse(500*pw,zero,rof1,rof1);
        rgpulse(500*pw,one,rof1,rof1);
        obspower(tpwr);
	}
}

/*   Flip back pulse definition */
void FBpulse(phase,phaseinc)
codeint phase, phaseinc;

{
  char fbshp[MAXSTR];
  double fbpw,
         fbpwr;

  getstr("fbshp",fbshp);
  fbpw = getval("fbpw");
  fbpwr = getval("fbpwr");
       obsstepsize(1.0);
       obspower(fbpwr);
       xmtrphase(phaseinc);
       shaped_pulse(fbshp,fbpw,phase,rof1,rof1);
       obspower(tpwr);
       xmtrphase(zero);
}

	
/*-----------------MLEV17c definition-----------------------------*/

void mlevc(width,phsA,phsB)
  double width;
  codeint phsA,phsB;
{
   txphase(phsA); delay(width);
   xmtroff(); delay(width); xmtron();
   txphase(phsB); delay(2*width);
   xmtroff(); delay(width); xmtron();
   txphase(phsA); delay(width);
}

void mlev17c(length,width,phsw,phsx,phsy,phsz,loop_counter)
 double length, width;
 codeint phsw,phsx,phsy,phsz,loop_counter;

{
   double  cycles;
   cycles = length/(96.67*width);
   cycles = 2*(double) (int) (cycles/2);
   initval(cycles, loop_counter);

  if (length > 0.0)
  {

   obsunblank();
   xmtron(); 

   starthardloop(loop_counter);

    mlevc(width,phsy,phsz); 
    mlevc(width,phsw,phsx);
    mlevc(width,phsw,phsx); 
    mlevc(width,phsy,phsz);

    mlevc(width,phsy,phsz);
    mlevc(width,phsy,phsz); 
    mlevc(width,phsw,phsx);
    mlevc(width,phsw,phsx); 

    mlevc(width,phsw,phsx); 
    mlevc(width,phsy,phsz);
    mlevc(width,phsy,phsz); 
    mlevc(width,phsw,phsx);

    mlevc(width,phsw,phsx);
    mlevc(width,phsw,phsx); 
    mlevc(width,phsy,phsz);
    mlevc(width,phsy,phsz); 

    txphase(phsx); delay(0.67*width);

  endhardloop();

   xmtroff();  
   obsblank();

  }
}


/*-----------------MLEV17 definition-----------------------------*/

void mlev(width,phsA,phsB)
  double width;
  codeint phsA,phsB;
{
   txphase(phsA); delay(width);
   txphase(phsB); delay(2*width);
   txphase(phsA); delay(width);
}

void mlev17(length,width,phsw,phsx,phsy,phsz,loop_counter)
 double length, width;
 codeint phsw,phsx,phsy,phsz,loop_counter;

{
   double  cycles;
   cycles = length/(64.67*width);
   cycles = 2*(double) (int) (cycles/2);
   initval(cycles, loop_counter);

  if (length > 0.0)
  {

   obsunblank();
   xmtron();

   starthardloop(loop_counter);

    mlev(width,phsy,phsz);
    mlev(width,phsw,phsx);
    mlev(width,phsw,phsx);
    mlev(width,phsy,phsz);

    mlev(width,phsy,phsz);
    mlev(width,phsy,phsz);
    mlev(width,phsw,phsx);
    mlev(width,phsw,phsx);

    mlev(width,phsw,phsx);
    mlev(width,phsy,phsz);
    mlev(width,phsy,phsz);
    mlev(width,phsw,phsx);

    mlev(width,phsw,phsx);
    mlev(width,phsw,phsx);
    mlev(width,phsy,phsz);
    mlev(width,phsy,phsz);

    txphase(phsx); delay(0.67*width);

   endhardloop();

   xmtroff(); 
   obsblank();

  }
}


/*------------------------dipsi2 spinlock definition-------------*/

void dips2(width,phsA,phsB)
double width;
codeint phsA,phsB;
{
      txphase(phsA); delay(320*width/90);
      txphase(phsB); delay(410*width/90);
      txphase(phsA); delay(290*width/90);
      txphase(phsB); delay(285*width/90);
      txphase(phsA); delay(30*width/90);
      txphase(phsB); delay(245*width/90);
      txphase(phsA); delay(375*width/90);
      txphase(phsB); delay(265*width/90);
      txphase(phsA); delay(370*width/90);
}

void dipsi2(length,width,phsx,phsy,loop_counter)
double length,width;
codeint phsx,phsy,loop_counter;
{
  double cycles;
  cycles = length/(width*115.11);
  cycles = 2*(double)(int)(cycles/2);
  initval(cycles, loop_counter);

  if (length > 0.0)
  {
    obsunblank();
    xmtron(); 
    starthardloop(loop_counter);
       dips2(width,phsx,phsy); dips2(width,phsy,phsx); 
       dips2(width,phsy,phsx); dips2(width,phsx,phsy);
    endhardloop();
    xmtroff(); 
    obsblank();
   }
}

/*------------------------dipsi3 spinlock definition-------------*/

void dips3(width,phsA,phsB)
double width;
codeint phsA,phsB;
{
      txphase(phsA); delay(245*width/90);
      txphase(phsB); delay(395*width/90);
      txphase(phsA); delay(250*width/90);
      txphase(phsB); delay(275*width/90);
      txphase(phsA); delay(30*width/90);
      txphase(phsB); delay(230*width/90);
      txphase(phsA); delay(90*width/90);
      txphase(phsB); delay(245*width/90);
      txphase(phsA); delay(370*width/90);
      txphase(phsB); delay(340*width/90);
      txphase(phsA); delay(350*width/90);
      txphase(phsB); delay(260*width/90);
      txphase(phsA); delay(270*width/90);
      txphase(phsB); delay(30*width/90);
      txphase(phsA); delay(225*width/90);
      txphase(phsB); delay(365*width/90);
      txphase(phsA); delay(255*width/90);
      txphase(phsB); delay(395*width/90);
}

void dipsi3(length,width,phsx,phsy,loop_counter)
double length,width;
codeint phsx,phsy,loop_counter;
{
  double cycles;
  cycles = length/(width*217.33);
  cycles = 2*(double)(int)(cycles/2);
  initval(cycles, loop_counter);

  if (length > 0.0)
  {
    obsunblank();
    xmtron(); 
    starthardloop(loop_counter);
       dips3(width,phsx,phsy); dips3(width,phsy,phsx); 
       dips3(width,phsy,phsx); dips3(width,phsx,phsy);
    endhardloop();
    xmtroff(); 
    obsblank();
   }
}

/*-------------transverse roesy spinlock definition---------------*/

void troesy(length,width,phs1,phs2,loop_counter)
 double length,width;
 codeint phs1, phs2, loop_counter;
{
  double cycles;
   cycles = length/(width*4);
   cycles = 2*(double) (int) (cycles/2);
   initval(cycles, loop_counter);

  if (length > 0.0)
  {
   obsunblank();
   xmtron();
   starthardloop(loop_counter);
	txphase(phs1); delay(2*width);
	txphase(phs2); delay(2*width);
   endhardloop();
   xmtroff();
   obsblank();
  }
}

/*------------------dante spinlock---------------------*/

void dante_spinlock(length,width,phs1,loop_counter)
 double length,width;
 codeint phs1, loop_counter;
{
  double cycles,
  	 ratio;
   ratio = getval("ratio");
   cycles = length/(width/3*16*(ratio+1));
   cycles = 2*(double) (int) (cycles/2);
   initval(cycles, loop_counter);

  if (length > 0.0)
  {
       delay(ratio*width/6);
      starthardloop(loop_counter);
       rgpulse(width/3,phs1,0.0,ratio*width/3);
       rgpulse(width/3,phs1,0.0,ratio*width/3);
       rgpulse(width/3,phs1,0.0,ratio*width/3);
       rgpulse(width/3,phs1,0.0,ratio*width/3);
       rgpulse(width/3,phs1,0.0,ratio*width/3);
       rgpulse(width/3,phs1,0.0,ratio*width/3);
       rgpulse(width/3,phs1,0.0,ratio*width/3);
       rgpulse(width/3,phs1,0.0,ratio*width/3);
       rgpulse(width/3,phs1,0.0,ratio*width/3);
       rgpulse(width/3,phs1,0.0,ratio*width/3);
       rgpulse(width/3,phs1,0.0,ratio*width/3);
       rgpulse(width/3,phs1,0.0,ratio*width/3);
       rgpulse(width/3,phs1,0.0,ratio*width/3);
       rgpulse(width/3,phs1,0.0,ratio*width/3);
       rgpulse(width/3,phs1,0.0,ratio*width/3);
       rgpulse(width/3,phs1,0.0,ratio*width/3);
      endhardloop();
       delay(ratio*width/6);
 } 
}

/*------------------cw spinlock---------------------*/

void cw_spinlock(length,phs1)
 double length;
 codeint phs1;
{
	rgpulse(length,phs1,0.0,0.0);
} 

/*----------------- SpinLock definition---------------------*/

void SpinLock(pattern,length,width,phsa,phsb,phsc,phsd,loop_counter)
 double length,width;
 codeint phsa,phsb,phsc,phsd,loop_counter;
 char pattern[MAXSTR];
{

   obs_pw_ovr(TRUE);

   if (!strcmp(pattern,"mlev17c"))
      mlev17c(length,width,phsa,phsb,phsc,phsd, loop_counter);

   if (!strcmp(pattern,"dipsi2"))
      dipsi2(length,width,phsb,phsd, loop_counter);

   if (!strcmp(pattern,"mlev17"))
      mlev17(length,width,phsa,phsb,phsc,phsd, loop_counter);

   if (!strcmp(pattern,"dipsi3"))
      dipsi3(length,width,phsb,phsd,loop_counter);

   if (!strcmp(pattern,"cw"))
      cw_spinlock(length,phsb);

   if (!strcmp(pattern,"dante"))
      dante_spinlock(length,width,phsb,loop_counter);

   if (!strcmp(pattern,"troesy"))
      troesy(length,width,phsa,phsc,loop_counter);

   obs_pw_ovr(FALSE);

}

#endif  // CHEMPACK_H

