/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* cyclenoef. Difference NOE experiment
      Uses the obs xmtr for irradiation and fine power control
  Parameters:

    pw - 90 degree excitation pulse (at power tpwr)
    intsub - 'y': internal subtraction of data acquired by on-resonance
                  and off-resonance selective excitation
             'n': data acquired by on-resonance and off-resonance selec-
                  tive excitation are stored separately
    noefrq  - frequency of selective saturation (on resonance);
              (make this an array if intsub = 'n', off-res dof if cycle='n')
    control - off resonance selective saturation frequency
              (an inactive parameter if intsub = 'n')
    cycle   - cycle='y' does on-resonance saturation using frequency cycling
              around the frequency "noefrq" given by "spacing"and"pattern" 
              cycle='n' does off-resonance saturation at control
    spacing - spacing(in Hz) of multiplet
    pattern - pattern type ( 1 for singlet, 2 for doublet, etc.)    
    tau     - period spent on a single irradiation point during cycling 
    noepwr  - power of selective irradiation
   finepwr  - extra attenuation (4095=no atten. 0=60db additonal atten.)
    noetime - total length of irradiation at frequency noefrq.
    mix - mixing time
    sspul   - sspul='y' hs-90-hs before d1
    nt - intsub = 'n':  multiple of 16
         intsub = 'y':  multiple of 32
    shaped-   flag to select shaped-pulse saturation (no cycling)
    shape-    selective pulse shape used for saturation (at noepwr/noetime)

    NOTE:  This pulse sequence requires that the observe channel be
           equipped with direct synthesis RF and a linear amplifier.
           noepwr ranges 0-63 control obs attenuator
*/


#include <standard.h>

pulsesequence()
{
/*DEFINE LOCAL VARIABLES */
  double gzlvl1,gt1,mix,control,noefrq,satfrq,noepwr,satpwr,finepwr,noetime,spacing;
  int pattern,times,jj;
  char shape[MAXSTR],intsub[MAXSTR],satmode[MAXSTR],scan[MAXSTR],
       shaped[MAXSTR],cycle[MAXSTR],sspul[MAXSTR];


/* LOAD AND INITIALIZE VARIABLES */
  getstr("intsub",intsub); getstr("cycle",cycle); getstr("sspul",sspul); 
  getstr("satmode",satmode); getstr("shaped",shaped); getstr("shape",shape);
  getstr("scan",scan); 
  control = getval("control");
  noefrq = getval("noefrq");
  satfrq = getval("satfrq");
  noepwr = getval("noepwr");
  satpwr = getval("satpwr");
  finepwr = getval("finepwr");
  noetime = getval("noetime"); spacing = getval("spacing");
  pattern = (int)getval("pattern");
  mix = getval("mix");
  gzlvl1=getval("gzlvl1"); gt1=getval("gt1");
  if (pattern == 0) pattern = 1; if (tau == 0.0) tau = 0.1;
  times = (int)(noetime/(pattern*tau));

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
    zgradpulse(4000,0.001);
    pulse(pw,zero); 
    zgradpulse(5000,0.001);
    }
    if (satmode[A] == 'y')
    {
      obsoffset(satfrq);
      obspower(satpwr);
      pulse(d1,zero);
      obspower(tpwr);
      obsoffset(tof);
    }
    else
     delay(d1);
    if (scan[A] == 'n')
    {
    obspower(noepwr); obspwrf(finepwr);
    delay(0.2e-6);                             /*reduce xmtr leakage */
 status(B);

/* selective pulse or decoupler saturation */
    /* no cycling or interleaved subtraction (make control an array)*/
    if ((intsub[0] == 'n') && (cycle[0] == 'n'))
     {
      obsoffset(noefrq);
      if (shaped[A] == 'y')
       {
       shaped_pulse(shape,noetime,zero,rof1,rof2);
       zgradpulse(gzlvl1,gt1);
       }
      else
       rgpulse(noetime,zero,rof1,rof2);
     }
    /* interleaved subtraction without cycling */
    if ((intsub[0] == 'y') && (cycle[0] == 'n'))
      {
       if (shaped[A] == 'y')
       {
        ifzero(v14);
          obsoffset(control);
          shaped_pulse(shape,noetime,zero,rof1,rof2);
        elsenz(v14);
          obsoffset(noefrq);
          shaped_pulse(shape,noetime,zero,rof1,rof2);
        endif(v14);
        zgradpulse(gzlvl1,gt1);
       }
       else
       {
        ifzero(v14);
          obsoffset(control);
          rgpulse(noetime,zero,rof1,rof2);
        elsenz(v14);
          obsoffset(noefrq);
          rgpulse(noetime,zero,rof1,rof2);
        endif(v14);
       }
      }
    /* no interleaved subtraction but cycling is used (make cycle array)*/
    if ((cycle[0] == 'y') && (intsub[0] == 'n'))
    {
     for (jj = 0; jj < times; jj++)
      {
       double startfrq; int i;
       startfrq = noefrq - (pattern/2)*spacing;
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
       value of noefrq. Link array noefrq with pattern and spacing for
       multiple noe difference spectra within one experiment. For example
       set array = '(noefrq,pattern,spacing)') */

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
         startfrq = noefrq - (pattern/2)*spacing;
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
    obspower(tpwr); obspwrf(4095.0);
    

 status(C);
/* NOE mixing time */
   if (mix>0.0)
    {
    if (satmode[C] =='y')
     {
      obsoffset(satfrq);
      obspower(satpwr); rgpulse(mix,zero,rof1,rof1); obspower(tpwr);
      obsoffset(tof);
     }
    else
     delay(mix);
    }
 status(D);
/* sampling pulse */
    rgpulse(pw,v1,rof1,rof2);
  }
 else
    rgpulse(pw,oph,rof1,rof2);

}
