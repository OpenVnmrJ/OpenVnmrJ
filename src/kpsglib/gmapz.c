// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
/* 
 */

/*
        - Gradient Shimming - shim along the sample z axis 

        array d3 to allow phase evolution of shim gradients
        if p1=0, use gradient echo with +/- gradients
        if p1>0, use spin echo with 90-180 pulses
        Homospoil Gradients: gradtype='nnh' (must use spin echo, p1>0)


        - Sample Spinning during Gradient Shimming

        To enable sample spinning during gradient shimming:
          - Set gmapspin='y' (menu button or panel).
          - Set spin speed.
          - Click on Gradient,Nucleus (Pfg H1/Pfg H2/Homospoil H1/Homospoil H2)
              This sets the delays such that the pulses are synchronous
              with the rotor period.  It also sets pulses and other
              parameters.
          - Or enter gmapsys('spin') on the command line to just set
              the delays to be synchronous, without setting pulses.

        To disable sample spinning during gradient shimming:
          - Set gmapspin='n' (menu button or panel).
          - Set spin = 0.
          - Click on Gradient,Nucleus (Pfg H1/Pfg H2/Homospoil H1/Homospoil H2)
              to reset delays and other parameters.

        See the macro gmapsys('spin') for details.

                                                b.f. Palo Alto 1995-2001.


        - Add Convection Compensation Gradient Shimming

        vtcomplvl: = 0, performs the standard shimming experiment
                   = 1 or 2 performs the convection compensated
                     gradient shimming pulse sequences at elevated temperatures
                   (=1 nt should be a multiple of 4  (minimum phase cycle)) 
                   (=2 adds more gradients to allow little or no phase cycling)
                     (see ref 4. below)
                   vtcomplvl>0 must use spin echo, p1>0
                   vtcomplvl=2 is not allowed for homospoil shimming or
                     spinning while shimming, and is reset to vtcomplvl=1
        pw       : excitation pulse (90 degrees or less)
        p1       : refocusing pulse (180 degrees)
        d1       : relaxation delay
        d2       : gradient stabilization delay
        d3       : shim phase encode delay
        gzlvl    : gradient strength during acquisition
        lvlf     : gradient strength multiplier for convection compensation
        gt       : gradient length for convection compensation (vtcomplvl>0)
        gts      : spoil gradient length for convection compensation (vtcomplvl=2)

                                                d.a./p.s. Darmstadt 2001-2002.

References:
  1.  P. C. M. van Zijl et al., J. Magn. Reson. A, 111, 203-207 (1994).
  2.  S. Sukumar et al., J. Magn. Reson. A, 125, 159-162 (1997).
  3.  H. Barjat et al., J. Magn. Reson. A, 125, 197-201 (1997).
  4.  C-L. Evans, G.A. Morris and A.L. Davis; J. Magn. Reson, 154, 325-328 (2002).

        See man('gmapsys') for usage.
*/


#include <standard.h>

extern int specialGradtype;
extern void settmpgradtype();

static int ph1[16] = {0,1,2,3,0,1,2,3,1,2,3,0,1,2,3,0},
           ph2[2]  = {0,1},
           ph3[8]  = {0,3,0,3,1,0,1,0},
           ph4[16] = {0,1,2,3,2,3,0,1,1,2,3,0,3,0,1,2}; 

pulsesequence()
{
   double 	d3 = getval("d3"),
                vtcomplvl = getval("vtcomplvl"),
		gzlvl = getval("gzlvl"),
		gt,gts,lvlf;
   int		hs_gradtype;

   settmpgradtype("tmpgradtype");
   hs_gradtype = ((specialGradtype == 'h') || (specialGradtype == 'a'));

   if ((p1==0.0) && ((vtcomplvl>0.5) || (hs_gradtype)))
   {
      p1 = 2.0*pw;
   }
   if ((vtcomplvl==2) && ((hs_gradtype) || (spin>0)))
   {
      vtcomplvl = 1;
      if (ix==1)
      {
         text_message("gmapz: vtcomplvl set to 1\n");
         putCmd("setvalue('vtcomplvl',%.0f)\n",vtcomplvl);
         putCmd("setvalue('vtcomplvl',%.0f,'processed')\n",vtcomplvl);
      }
   }

/* lvlf, gt, gts only used for convection compensation */
   if (gradtype[2]=='l') 
     lvlf=2.5;
   else
     lvlf=3.0;
   if ((hs_gradtype) || (spin>0))
     lvlf=1.0;

   gt = (0.5*at + d2)/lvlf;
   gts=0.18*at/lvlf;
   gts = 0.2*at/lvlf;
   if (vtcomplvl==2)
      gt += gts;
   gt *= 2.0;

   settable(t1,16,ph1);
   settable(t2,2,ph2);
   settable(t3,8,ph3);
   settable(t4,16,ph4);

   sub(ct,ssctr,v12);
   getelem(t1,v12,v1);
   getelem(t2,v12,v2);
   getelem(t3,v12,v3);
   getelem(t4,v12,oph); 

   if (vtcomplvl < 0.5)
   {
      getelem(t4,v12,v1); 
      getelem(t1,v12,v2);
   }

/* --- equilibration period --- */
   status(A);
      delay(d1);

/* --- initial pulse --- */
   status(B);
      pulse(pw,v1);
/*    instead of hard pulse, could use shapedpulse("gauss",pw,oph,rof1,rof2);
        or "bir4_90_512" or other shape for selective excitation.
        selective inversion pulses may or may not work as well. */

/* --- shim phase encode, not during PFG recovery --- */
      delay(2e-5+d3);

/* First case: No convection compensation, traditional sequence */
   if (vtcomplvl < 0.5)
   {
      if (p1 > 0) 
      {
         zgradpulse(gzlvl,at/2+d2);
         delay(d2);
         pulse(p1,v2);
         delay(d2);
      }
      else
      {
         zgradpulse(-gzlvl,at/2+d2);
         delay(d2*2);
      }
   }
   else  /* (vtcomplvl > 0.5) */
   {
/* Second case: convection compensation  */

      zgradpulse(gzlvl,at/2+d2);
      delay(d2);
      if (vtcomplvl==2)
      {
         zgradpulse(lvlf*gzlvl,gts);
         delay(d2);
      }
      pulse(p1,v2); 
      delay(d2);
      zgradpulse(lvlf*gzlvl,gt);
      delay(d2);
      pulse(p1,v3);
      delay(d2);
      if (vtcomplvl==2)
      {
         zgradpulse(lvlf*gzlvl,gts); 
         delay(d2);
      }
   }

/* --- acq. of echo during gradient --- */
   rgradient('z',gzlvl);
   delay(d2);
/* gradient switches off at end of acq. */

}
