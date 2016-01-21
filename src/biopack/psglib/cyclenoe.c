/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* cyclenoe. Difference NOE experiment
      Uses the obs xmtr for irradiation
  Parameters:

    pw - 90 degree excitation pulse (at power tpwr)
    intsub - 'y': internal subtraction of data acquired by on-resonance
                  and off-resonance selective excitation
             'n': data acquired by on-resonance and off-resonance selec-
                  tive excitation are stored separately
    satfrq  - frequency of selective saturation (on resonance);
              (make this an array if intsub = 'n', off-res dof if cycle='n')
    control - off resonance selective saturation frequency
              (an inactive parameter if intsub = 'n')
    cycle   - cycle='y' does on-resonance saturation using frequency cycling
              around the frequency "satfrq" given by "spacing"and"pattern" 
              cycle='n' does off-resonance saturation at control
    spacing - spacing(in Hz) of multiplet
    pattern - pattern type ( 1 for singlet, 2 for doublet, etc.)    
    tau     - period spent on a single irradiation point during cycling 
    satpwr  - power of selective irradiation
    sattime - total length of irradiation at frequency satfrq.
    mix - mixing time
    sspul   - sspul='y' does Trim(x)-Trim(y) before d1
    nt - intsub = 'n':  multiple of 16
         intsub = 'y':  multiple of 32


    NOTE:  This pulse sequence requires that the observe channel be
           equipped with direct synthesis RF and a linear amplifier.
           satpwr ranges 0-63 control obs attenuator
       Contact-  G.Gray (palo alto)    revision- from cycledof.c
      Upgraded to VJ2.1B [fixed "power(v6,TODEV)" syntax] - PAKeifer 070207
*/


#include <standard.h>

pulsesequence()
{
/*DEFINE LOCAL VARIABLES */
  double mix,satfrq,control,satpwr,sattime,
         tau,spacing;
  int pattern,times,jj;
  char intsub[MAXSTR],cycle[MAXSTR],sspul[MAXSTR];


/* LOAD AND INITIALIZE VARIABLES */
  tau = getval("tau");
  getstr("intsub",intsub); getstr("cycle",cycle); getstr("sspul",sspul); 
  satfrq = getval("satfrq"); control = getval("control");
  sattime = getval("sattime"); spacing = getval("spacing");
  satpwr = getval("satpwr"); pattern = (int)getval("pattern");
  mix = getval("mix");
  if (pattern == 0) pattern = 1; if (tau == 0.0) tau = 0.1;
  times = (int)(sattime/(pattern*tau));

/* CHECK CONDITIONS */


/* CALCULATE PHASES */
 if (intsub[0] == 'y') hlv(ct,v1); else assign(ct,v1);
 assign(v1,oph);
 if (intsub[0] == 'y')
   {
    mod2(ct,v14);  /* trigger for the alteration of the saturation freq */
    ifzero(v14); add(oph,two,oph); endif(v14);
   }


/* BEGIN ACTUAL PULSE SEQUENCE CODE */
 status(A);
    if (sspul[A] == 'y')
    {
    pulse(200*pw,zero); pulse(200*pw,one); 
    }
    hsdelay(d1);
    obspower(satpwr);
    delay(0.2e-6);                             /*reduce xmtr leakage */
 status(B);

/* selective pulse or decoupler saturation */
    /* no cycling or interleaved subtraction (make control an array)*/
    if ((intsub[0] == 'n') && (cycle[0] == 'n'))
     {
      obsoffset(satfrq);
      rgpulse(sattime,zero,rof1,rof2);
     }
    /* interleaved subtraction without cycling */
    if ((intsub[0] == 'y') && (cycle[0] == 'n'))
      {
       ifzero(v14);
          obsoffset(control);
          rgpulse(sattime,zero,rof1,rof2);
       elsenz(v14);
          obsoffset(satfrq);
          rgpulse(sattime,zero,rof1,rof2);
       endif(v14);
      }
    /* no interleaved subtraction but cycling is used (make cycle array)*/
    if ((cycle[0] == 'y') && (intsub[0] == 'n'))
    {
     for (jj = 0; jj < times; jj++)
      {
       double startfrq; int i;
       startfrq = satfrq - (pattern/2)*spacing;
       if ((pattern %2) == 0) startfrq = startfrq + spacing/2.0;
       for (i = 0; i < pattern; i++)
        {
         obsoffset(startfrq);
         rgpulse(tau,zero,rof1,rof2);
         startfrq = startfrq + spacing;
        }
      }  
    }
    /* interleaved subtraction with cycling (no array needed for one
       value of satfrq. Link array satfrq with pattern and spacing for
       multiple noe difference spectra within one experiment. For example
       set array = '(satfrq,pattern,spacing)') */

    if ((cycle[0] == 'y') && (intsub[0] == 'y'))
     {
      ifzero(v14);
       for (jj = 0; jj < times; jj++)
        {
         double startfrq; int i;
         startfrq = control - (pattern/2)*spacing;
         if ((pattern %2) == 0) startfrq = startfrq + spacing/2.0;
         for (i = 0; i < pattern; i++)
          {
           obsoffset(startfrq);
           rgpulse(tau,zero,rof1,rof2);
           startfrq = startfrq + spacing;
          }
         }
      elsenz(v14);
       for (jj = 0; jj < times; jj++)
        {
         double startfrq; int i;
         startfrq = satfrq - (pattern/2)*spacing;
         if ((pattern %2) == 0) startfrq = startfrq + spacing/2.0;
         for (i = 0; i < pattern; i++)
          {
           obsoffset(startfrq);
           rgpulse(tau,zero,rof1,rof2);
           startfrq = startfrq + spacing;
          }
        }  
      endif(v14);
     }
    /* restore power levels as controlled by tpwr   */
    obsoffset(tof);
    obspower(tpwr); 
    

 status(C);
/* NOE mixing time */
    hsdelay(mix);
 status(D);
/* sampling pulse */
    rgpulse(pw,v1,rof1,rof2);
}
