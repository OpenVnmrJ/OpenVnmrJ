/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <stdio.h>
#include "oopc.h"
#include "acqparms.h"
#include "rfconst.h"
#include "macros.h"
#include <math.h>
#include "cps.h"

extern int DPSprint(double, const char *, ...);
extern int DPStimer(int code, int subcode, int vnum, int fnum, ...);

extern int dpsTimer;
extern int dps_flag;

int getflag(const char *str);

static void x_chess(double pulsepower, char *pulseshape, double duration,
      codeint phase, double rx1, double rx2, double gzlvlw,
      double gtw, double gswet, int c13wet)
{

  DPSprint(0.0, "61 rlpwrf  %d  0 1 pulsepower %.9f \n", (int)(OBSch), (float)(pulsepower));
  if (c13wet)
    DPSprint(0.0, "11 decon  2 1 0 on 1\n");
  DPSprint(duration, "77 shaped_pulse  1 1 1 1 rx1 %.9f rx2 %.9f  1  ?%s phase %d duration %.9f \n",
           (float)(rx1), (float)(rx2), pulseshape, (int)(phase), (float)(duration));
  if (c13wet)
    DPSprint(0.0, "11 decoff  2 1 0 off 0 \n");
  DPSprint(gtw, "94 zgradpulse  11 0 2 z gtw %.9f gzlvlw %.9f \n",
           (float)(gtw), (float)(gzlvlw));
  DPSprint(gswet, "10 delay  1 0 1 gswet %.9f \n", (float)(gswet));
}

static void x_wet4(codeint phaseA, codeint phaseB)
{

  double finepwr,gzlvlw,gtw,gswet,dmfwet,dpwrwet,dofwet,dreswet,wetpwr,pwwet,dz;
  int c13wet;
  char wetshape[MAXSTR], wetfly[MAXSTR], dmmwet[MAXSTR], dseqwet[MAXSTR];

  c13wet=getflag("c13wet");
  getstr("wetshape",wetshape);
  getstr("wetfly",wetfly);
  getstr("dseqwet",dseqwet);
  getstr("dmmwet",dmmwet);
  wetpwr=getval("wetpwr");
  pwwet=getval("pwwet");
  dmfwet=getval("dmfwet");
  dpwrwet=getval("dpwrwet");
  dofwet=getval("dofwet");
  dreswet=getval("dreswet");
  dz=getval("dz");
  finepwr=wetpwr-(int)wetpwr;
  wetpwr=(double)((int)wetpwr);
  if (finepwr==0.0)
  {
    wetpwr=wetpwr+5;
    finepwr=4095.0;
  }
  else
  {
    wetpwr=wetpwr+6;
    finepwr = 4095.0 * exp( -0.1151 * (1.0 - finepwr));
  }

  if (c13wet)
  {
    DPSprint(0.0, "33 decoffset  2 0 1 dofwet %.9f \n", (float)(dofwet));
    DPSprint(0.0, "60 decpower  2 0 1 dpwrwet %.9f \n", (float)(dpwrwet));
    if (rfwg[DECch-1] == 'n')
        DPSprint(0.0, "73 setstatus  %d  3 1  ?%c  %d FALSE %d FALSE %d dmfwet %.9f \n",
           DECch, dmmwet, (int)(FALSE), (int)(FALSE), (int)(FALSE), (float)(dmfwet));
    else
        DPSprint(0.0, "46 decprgon  2 0 3  ?%s  %.9f 1/dmfwet %.9f dreswet %.9f \n",
           dseqwet, (float)(1/dmfwet), (float)(1/dmfwet), (float)(dreswet));
  }

  DPSprint(0.0, "60 rlpower  %d  0 1 wetpwr %.9f \n", (int)(OBSch), (float)(wetpwr));
  gzlvlw=getval("gzlvlw");
  gtw=getval("gtw");
  gswet=getval("gswet");
  x_chess(finepwr*0.5056,wetshape,pwwet,phaseA,20.0e-6,10.0e-6,
        gzlvlw/1.0,gtw,gswet,c13wet);
  x_chess(finepwr*0.6298,wetshape,pwwet,phaseB,20.0e-6,10.0e-6,
        gzlvlw/2.0,gtw,gswet,c13wet);
  x_chess(finepwr*0.4304,wetshape,pwwet,phaseB,20.0e-6,10.0e-6,
        gzlvlw/4.0,gtw,gswet,c13wet);
  x_chess(finepwr*1.0000,wetshape,pwwet,phaseB,20.0e-6,10.0e-6,
        gzlvlw/8.0,gtw,gswet,c13wet);
  if (c13wet)
  {
    if (rfwg[DECch-1] == 'n')
        DPSprint(0.0, "73 setstatus  %d  3 1  ?%c  %d FALSE %d FALSE %d dmf %.9f \n",
             DECch, 'c', (int)(FALSE), (int)(FALSE), (int)(FALSE), (float)(dmf));
    else
        DPSprint(0.0, "45 decprgoff  2 1 0 off 0 \n");
    DPSprint(0.0, "33 decoffset  2 0 1 dof %.9f \n", (float)(dof));
    DPSprint(0.0, "60 decpower  2 0 1 dpwr %.9f \n", (float)(dpwr));
  }
  DPSprint(0.0, "60 rlpower  %d  0 1 tpwr %.9f \n", (int)(OBSch), (float)(tpwr));
  DPSprint(0.0, "61 rlpwrf  %d  0 1 tpwrf %.9f \n", (int)(OBSch), (float)(tpwrf));
  DPSprint(dz, "10 delay  1 0 1 dz %.9f \n", (float)(dz));
}


static void x_comp90pulse(double width, codeint phase, double rx1, double rx2)
{

  DPSprint(0.0, "70 incr 59 1 0 phase %d \n", (int)(phase));
  DPSprint(width, "48 rgpulse  1 0 1 1 rx1 %.9f rx1 %.9f  1 phase %d width %.9f \n",
     (float)(rx1), (float)(rx1), (int)(phase), (float)(width));
  DPSprint(0.0, "70 incr 59 1 0 phase %d \n", (int)(phase));
  DPSprint(width, "48 rgpulse  1 0 1 1 rx1 %.9f rx1 %.9f  1 phase %d width %.9f \n",
     (float)(rx1), (float)(rx1), (int)(phase), (float)(width));
  DPSprint(0.0, "70 incr 59 1 0 phase %d \n", (int)(phase));
  DPSprint(width, "48 rgpulse  1 0 1 1 rx1 %.9f rx1 %.9f  1 phase %d width %.9f \n",
     (float)(rx1), (float)(rx1), (int)(phase), (float)(width));
  DPSprint(0.0, "70 incr 59 1 0 phase %d \n", (int)(phase));
  DPSprint(width, "48 rgpulse  1 0 1 1 rx1 %.9f rx2 %.9f  1 phase %d width %.9f \n",
     (float)(rx1), (float)(rx2), (int)(phase), (float)(width));
}

static void t_chess(double pulsepower, char *pulseshape, double duration,
      codeint phase, double rx1, double rx2, double gzlvlw,
      double gtw, double gswet, int c13wet)
{

  DPStimer(61,0,0,0);
  if (c13wet)
    DPStimer(11,0,0,0);
  DPStimer(77,0,0,3,0,0,0,0 ,(double)duration,(double)rx1,(double)rx2);
  if (c13wet)
    DPStimer(11,0,0,0);
  DPStimer(94,0,0,1,0,0,0,0 ,(double)gtw);
  DPStimer(10,0,0,1,0,0,0,0 ,(double)gswet);
}

static void t_wet4(codeint phaseA, codeint phaseB)
{
  double finepwr,gzlvlw,gtw,gswet,wetpwr,pwwet,dz;
  int c13wet;
  char wetshape[MAXSTR], wetfly[MAXSTR], dmmwet[MAXSTR], dseqwet[MAXSTR];

  c13wet=getflag("c13wet");
  getstr("wetshape",wetshape);
  getstr("wetfly",wetfly);
  getstr("dseqwet",dseqwet);
  getstr("dmmwet",dmmwet);
  wetpwr=getval("wetpwr");
  pwwet=getval("pwwet");
//  dmfwet=getval("dmfwet");
//  dpwrwet=getval("dpwrwet");
//  dofwet=getval("dofwet");
//  dreswet=getval("dreswet");
  dz=getval("dz");
  finepwr=wetpwr-(int)wetpwr;
  wetpwr=(double)((int)wetpwr);
  if (finepwr==0.0)
  {
    wetpwr=wetpwr+5;
    finepwr=4095.0;
  }
  else
  {
    wetpwr=wetpwr+6;
    finepwr = 4095.0 * exp( -0.1151 * (1.0 - finepwr));
  }

  if (c13wet)
  {
    DPStimer(33,0,0,0);
    DPStimer(60,0,0,0);
    if (rfwg[DECch-1] == 'n')
        DPStimer(73,0,0,0);
    else
        DPStimer(46,0,0,0);
  }

  DPStimer(60,0,0,0);
  gzlvlw=getval("gzlvlw");
  gtw=getval("gtw");
  gswet=getval("gswet");
  t_chess(finepwr*0.5056,wetshape,pwwet,phaseA,20.0e-6,10.0e-6,
        gzlvlw/1.0,gtw,gswet,c13wet);
  t_chess(finepwr*0.6298,wetshape,pwwet,phaseB,20.0e-6,10.0e-6,
        gzlvlw/2.0,gtw,gswet,c13wet);
  t_chess(finepwr*0.4304,wetshape,pwwet,phaseB,20.0e-6,10.0e-6,
        gzlvlw/4.0,gtw,gswet,c13wet);
  t_chess(finepwr*1.0000,wetshape,pwwet,phaseB,20.0e-6,10.0e-6,
        gzlvlw/8.0,gtw,gswet,c13wet);
  if (c13wet)
  {
    if (rfwg[DECch-1] == 'n')
        DPStimer(73,0,0,0);
    else
        DPStimer(45,0,0,0);
    DPStimer(33,0,0,0);
    DPStimer(60,0,0,0);
  }
  DPStimer(60,0,0,0);
  DPStimer(61,0,0,0);
  DPStimer(10,0,0,1,0,0,0,0 ,(double)dz);
}


static void t_comp90pulse(double width, codeint phase, double rx1, double rx2)
{

  DPStimer(70,59,1,0,(int)phase,0,0,0);
  DPStimer(48,0,0,3,0,0,0,0 ,(double)width,(double)rx1,(double)rx1);
  DPStimer(70,59,1,0,(int)phase,0,0,0);
  DPStimer(48,0,0,3,0,0,0,0 ,(double)width,(double)rx1,(double)rx1);
  DPStimer(70,59,1,0,(int)phase,0,0,0);
  DPStimer(48,0,0,3,0,0,0,0 ,(double)width,(double)rx1,(double)rx1);
  DPStimer(70,59,1,0,(int)phase,0,0,0);
  DPStimer(48,0,0,3,0,0,0,0 ,(double)width,(double)rx1,(double)rx2);
}

int getflag(const char *str)
{
   char strval[MAXSTR];

   getstr(str,strval);
   if ((strval[0]=='y') || (strval[0]=='Y'))
      return(TRUE);
   else
      return(FALSE);
}

static double dmfwet,dreswet;
static char dseqwet[MAXSTR];

/* chess - CHEmical Shift Selective Suppression */
static void chess(double pulsepower, char *pulseshape, double duration,
      codeint phase, double rx1, double rx2, double gzlvlw,
      double gtw, double gswet, int c13wet)  
{
  if (dps_flag)
  {
     if (dpsTimer == 0)
        x_chess(pulsepower, pulseshape, duration, phase, rx1, rx2, gzlvlw,
                gtw, gswet, c13wet);
     else
        t_chess(pulsepower, pulseshape, duration, phase, rx1, rx2, gzlvlw,
                gtw, gswet, c13wet); 
     return;
  }
  rlpwrf(pulsepower,OBSch);
  if (c13wet)
  {
#ifdef NVPSG
    decunblank();
    decon();
    decprgon(dseqwet,1/dmfwet,dreswet); 
#else
    decon();
#endif
  }
  shaped_pulse(pulseshape,duration,phase,rx1,rx2);
  if (c13wet)
  {
#ifdef NVPSG
    decprgoff();
    decoff();
    decblank();
#else
    decoff();
#endif
  }
  zgradpulse(gzlvlw,gtw);
  delay(gswet);
}
 
/* wet4 - Water Elimination */
void wet4(codeint phaseA, codeint phaseB)
{
  double finepwr,gzlvlw,gtw,gswet,dpwrwet,dofwet,wetpwr,pwwet,dz;
  int c13wet;
  char wetshape[MAXSTR], wetfly[MAXSTR];
#ifndef NVPSG
  char dmmwet[MAXSTR];
#endif

  if (dps_flag)
  {
     if (dpsTimer == 0)
        x_wet4(phaseA, phaseB);
     else
        t_wet4(phaseA, phaseB);
     return;
  }
  c13wet=getflag("c13wet");             /* Water suppression flag        */  
  getstr("wetshape",wetshape);    /* Selective pulse shape (base)  */
  getstr("wetfly",wetfly);
  wetpwr=getval("wetpwr");        /* User enters power for 90 deg. */
  pwwet=getval("pwwet");        /* User enters power for 90 deg. */
  dz=getval("dz");
  finepwr=wetpwr-(int)wetpwr;     /* Adjust power to 152 deg. pulse*/
  wetpwr=(double)((int)wetpwr);
  if (finepwr==0.0)
  {
    wetpwr=wetpwr+5; 
    finepwr=4095.0;
  }
  else
  {
    wetpwr=wetpwr+6; 
    finepwr = 4095.0 * exp( -0.1151 * (1.0 - finepwr));
  }
  if (c13wet)
  {
    dofwet=getval("dofwet");
    dpwrwet=getval("dpwrwet");
    dmfwet=getval("dmfwet");
    dreswet=getval("dreswet");
    getstr("dseqwet",dseqwet);
    decoffset(dofwet);
    decpower(dpwrwet);
#ifndef NVPSG
    getstr("dmmwet",dmmwet);
    if (rfwg[DECch-1] == 'n')
	setstatus(DECch,FALSE,dmmwet[0],FALSE,dmfwet);
    else
	decprgon(dseqwet,1/dmfwet,dreswet); 
#endif
  }
  rlpower(wetpwr,OBSch);         /* Set to low power level        */
  gzlvlw=getval("gzlvlw");      /* Z-Gradient level              */
  gtw=getval("gtw");            /* Z-Gradient duration           */
  gswet=getval("gswet");        /* Post-gradient stability delay */
  chess(finepwr*0.5056,wetshape,pwwet,phaseA,20.0e-6,10.0e-6,
        gzlvlw/1.0,gtw,gswet,c13wet);
  chess(finepwr*0.6298,wetshape,pwwet,phaseB,20.0e-6,10.0e-6,
        gzlvlw/2.0,gtw,gswet,c13wet);
  chess(finepwr*0.4304,wetshape,pwwet,phaseB,20.0e-6,10.0e-6,
        gzlvlw/4.0,gtw,gswet,c13wet);
  chess(finepwr*1.0000,wetshape,pwwet,phaseB,20.0e-6,10.0e-6,
        gzlvlw/8.0,gtw,gswet,c13wet);
  if (c13wet)
  {
#ifndef NVPSG
    if (rfwg[DECch-1] == 'n')
	setstatus(DECch,FALSE,'c',FALSE,dmf);
    else
	decprgoff();
#endif
    decoffset(dof);
    decpower(dpwr);
  }
  rlpower(tpwr,OBSch);
  rlpwrf(tpwrf,OBSch);      /* Reset to normal power level   */
  delay(dz);
}

void comp90pulse(double width, codeint phase, double rx1, double rx2)
{
  if (dps_flag)
  {
     if (dpsTimer == 0)
        x_comp90pulse(width, phase, rx1, rx2);
     else
        t_comp90pulse(width, phase, rx1, rx2);
     return;
  }
  incr(phase); rgpulse(width,phase,rx1,rx1);  /*  Y  */
  incr(phase); rgpulse(width,phase,rx1,rx1);  /* -X  */
  incr(phase); rgpulse(width,phase,rx1,rx1);  /* -Y  */
  incr(phase); rgpulse(width,phase,rx1,rx2);  /*  X  */
}
