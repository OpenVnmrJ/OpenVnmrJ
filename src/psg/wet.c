/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/* wet.c - wet element for wet seqeunces */

#include <stdio.h>
#include "oopc.h"
#include "acqparms.h"
#include "macros.h"
#include "rfconst.h"

/* wet4 - Water Elimination */
wet4(phaseA,phaseB)
codeint phaseA,phaseB;
{
  double finepwr,gzlvlw,gtw,gswet,dmfwet,dpwrwet,dofwet,dreswet,wetpwr,pwwet,dz;
  int c13wet;
  char wetshape[MAXSTR], wetfly[MAXSTR], dmmwet[MAXSTR], dseqwet[MAXSTR];
  c13wet=getflag("c13wet");             /* Water suppression flag        */  
  getstr("wetshape",wetshape);    /* Selective pulse shape (base)  */
  getstr("wetfly",wetfly);
  getstr("dseqwet",dseqwet);
  getstr("dmmwet",dmmwet);
  wetpwr=getval("wetpwr");        /* User enters power for 90 deg. */
  pwwet=getval("pwwet");        /* User enters power for 90 deg. */
  dmfwet=getval("dmfwet");
  dpwrwet=getval("dpwrwet");
  dofwet=getval("dofwet");
  dreswet=getval("dreswet");
  dz=getval("dz");
  finepwr=wetpwr-(int)wetpwr;     /* Adjust power to 152 deg. pulse*/
  wetpwr=(double)((int)wetpwr);
  if (finepwr==0.0) {wetpwr=wetpwr+5; finepwr=4095.0; }
  else {wetpwr=wetpwr+6; finepwr=4095.0*(1-((1.0-finepwr)*0.12)); }

  if (c13wet)
  {
    decoffset(dofwet);
    decpower(dpwrwet);
    if (rfwg[DECch-1] == 'n')
	setstatus(DECch,FALSE,dmmwet,FALSE,dmfwet);
    else
	decprgon(dseqwet,1/dmfwet,dreswet); 
  }
  obspower(wetpwr);         /* Set to low power level        */
  gzlvlw=getval("gzlvlw");      /* Z-Gradient level              */
  gtw=getval("gtw");            /* Z-Gradient duration           */
  gswet=getval("gswet");        /* Post-gradient stability delay */
  chess(finepwr*0.5056,wetshape,pwwet,phaseA,20.0e-6,10.0e-6,gzlvlw/1.0,gtw,gswet,c13wet);
  chess(finepwr*0.6298,wetshape,pwwet,phaseB,20.0e-6,10.0e-6,gzlvlw/2.0,gtw,gswet,c13wet);
  chess(finepwr*0.4304,wetshape,pwwet,phaseB,20.0e-6,10.0e-6,gzlvlw/4.0,gtw,gswet,c13wet);
  chess(finepwr*1.0000,wetshape,pwwet,phaseB,20.0e-6,10.0e-6,gzlvlw/8.0,gtw,gswet,c13wet);
  if (c13wet)
  {
    if (rfwg[DECch-1] == 'n')
	setstatus(DECch,FALSE,'c',FALSE,dmf);
    else
	decprgoff();
    decoffset(dof);
    decpower(dpwr);
  }
  obspower(tpwr); obspwrf(tpwrf);      /* Reset to normal power level   */
  delay(dz);
}

/* chess - CHEmical Shift Selective Suppression */
chess(pulsepower,pulseshape,duration,phase,rx1,rx2,gzlvlw,gtw,gswet,c13wet)  
double pulsepower,duration,rx1,rx2,gzlvlw,gtw,gswet;
int c13wet;
codeint phase;
char* pulseshape;
{
  obspwrf(pulsepower);
  if (c13wet) decon();
  shaped_pulse(pulseshape,duration,phase,rx1,rx2);
  if (c13wet) decoff();
  zgradpulse(gzlvlw,gtw);
  delay(gswet);
}
 
int getflag(str)
char str[MAXSTR];
{
   char strval[MAXSTR];
   getstr(str,strval);
   if ((strval[0]=='y') || (strval[0]=='Y')) return(TRUE);
     else                                    return(FALSE);
}

comp90pulse(width,phase,rx1,rx2)
double width,rx1,rx2;
codeint phase;
{
  incr(phase); rgpulse(width,phase,rx1,rx1);  /*  Y  */
  incr(phase); rgpulse(width,phase,rx1,rx1);  /* -X  */
  incr(phase); rgpulse(width,phase,rx1,rx1);  /* -Y  */
  incr(phase); rgpulse(width,phase,rx1,rx2);  /*  X  */
}
