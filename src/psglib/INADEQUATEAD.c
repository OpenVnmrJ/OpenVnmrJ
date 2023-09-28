// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
#ifndef LINT
#endif

/* INADEQUATEAD:  carbon-carbon connectivity using double quantum
            spectroscopy;  F1 quadrature is achieved by hardware
            small-angle phaseshifts. Options for shaped 180 pulse.

  Parameters:
      pw = 90 degree carbon pulse
     jcc = carbon carbon coupling constant in hz
   phase =   0: absolute value with F1 quadrature
           1,2: hypercomplex method (phase-sensitive, F1 quadrature)
      nt = min:  multiple of   8 (phase=0)
                 multiple of   4 (phase=1,2  phase=3)
           max:  multiple of 128 (phase=0)
                 multiple of  64 (phase=1,2  phase=3)
    adiabatic: flag to determine whether an adiabatic refocusing 180 degrees
               pulse will be used. Strongly suggested for most cases. If set to                'y' an adiabatic or sp26 pulse (see below) is used. If set to 'n'               a hard 180 is used. If set to 'p' then a user supplied shaped 
               pulse is used.
        xsens: set to 'y' if an XSens Cold Probe is used. Calculates an sp26
               constant amplitude 180 pulse which deals better with the Xsens
               peculiar power behaviour.
      adshape: Shape file that contains the shape for the refocusing pulse,
               active only when adiabatic='p'
      adpwlvl: Power level for the shaped refocusing pulse, active only when 
               adiabatic='p'
         adpw: Pulse width, in usec, of the shaped refocusing pulse, active 
               only when adiabatic='p'

  NOTE:  Data acquired with phase=0 should be processed with wft2d.  Data
         acquired with phase=1,2 should be processed with wft2da.
         If phase-sensitive data without F1 quadrature are desired, set phase=1
         and process with wft2da.
	 For 1D spectra, set phase=1 for maximum sensitivity.

  s. farmer  25 February   1988 
  D. Argyropoulos, P. Sandor, April 2010  */

#include <standard.h>
#define BASE 45.0
#include <Pbox_psg.h>

static shape shapead;

void pulsesequence()
{
   double          jcc = getval("jcc"),
                   adpw = getval("adpw"),
                   adpwlvl = getval("adpwlvl"),
                   hsgt = getval("hsgt"),
                   hsglvl = getval("hsglvl"),
		   ref_pwr,
		   ref_pw90,
                   corr, jtau;
   char            adiabatic[MAXSTR],adshape[MAXSTR],sspul[MAXSTR],
		   fname[MAXSTR],cmd[MAXSTR],xsens[MAXSTR];
   extern char     userdir[];
   FILE		   *fp; 

/* INITIALIZE PARAMETER VALUES */
   ref_pwr=getval("tpwr");
   ref_pw90=(getval("tpwr_cf"))*(getval("pw90"));
   jtau = 1.0 / (4.0 * jcc);
   getstr("adiabatic",adiabatic);    
   getstr("adshape",adshape);
   getstr("sspul",sspul);
   getstr("xsens",xsens);

   if (rof1 > 2.0e-6) rof1=2.0e-6;

/* STEADY-STATE PHASECYCLING
   This section determines if the phase calculations trigger off of (SS - SSCTR)
   or off of CT */

   ifzero(ssctr);
      mod4(ct, v3);
      hlv(ct, v9);
   elsenz(ssctr);
      sub(ssval, ssctr, v12);		/* v12 = 0,...,ss-1 */
      mod4(v12, v3);
      hlv(v12, v9);
   endif(ssctr);


/* CALCULATE PHASECYCLE */
   hlv(v9, v9);
   if (phase1 == 0)		/* ABSOLUTE VALUE SPECTRA */
   {
      assign(v9, v10);
      hlv(v9, v9);
      mod2(v10, v10);
   }
   else
   {
      assign(zero, v10);	/* v10 = F1 quadrature */
   }
   assign(v9, v1);
   hlv(v9, v9);
   assign(v9, v2);
   hlv(v9, v9);
   hlv(v9, v9);
   mod2(v9, v9);		/* v9 = F2 quad. image suppression */

   dbl(v1, v1);			/* v1 = suppresses artifacts due to imperfec-
				        tions in the first 90 degree pulse */
   add(v9, v1, v1);
   assign(v1, oph);

   dbl(v2, v8);
   add(v9, v2, v2);		/* v2 = suppresses artifacts due to imperfect
				   180 refocusing pulse */
   add(v8, oph, oph);

   dbl(v3, v4);
   add(v3, v4, v4);
   add(v3, v9, v3);
   add(v4, oph, oph);		/* v3 = selects DQC during the t1 evolution
				        period */
   add(v10, oph, oph);

   if (phase1 == 2)
      incr(v10);		/* HYPERCOMPLEX */

/* Shaped 180 pulse calculation */
if (adiabatic[A] == 'y')
{
 if (xsens[A] == 'n')
 /* First case: Standard RT or Cold probe, composite adiabatic pulse */
 {
 (void) sprintf(fname,"%s/shapelib/c13compad.RF",userdir);
 (void) sprintf(adshape,"c13compad");
if ((getval("arraydim") < 1.5) || (ix==1))
  {
    (void) sprintf(fname, "%s/shapelib/Pbox.inp", userdir);
    if ( (fp=fopen(fname, "w")) != NULL )
      {
        fprintf(fp, "\n Pbox Input File ");
        fprintf(fp, "\n name = c13compad.RF ");
        fprintf(fp, "\n { cawurst-20 80k/0.5m 0 0 0 } +");
        fprintf(fp, "\n { cawurst-20 80k/1m 0 0 90 } +");
        fprintf(fp, "\n { cawurst-20 80k/0.5m 0 0 0 }");
        fprintf(fp, "\n $1 adb = 5");
        fprintf(fp, "\n $2 adb = 10");
        fprintf(fp, "\n $3 adb = 5");
        fprintf(fp, "\n RF Calibration Data: ref_pwr = %4d  ref_pw90 = %5.1f", (int)ref_pwr, ref_pw90*1.0e6);
        fclose(fp);
        sprintf(cmd,"cd $vnmruser/shapelib; Pbox > /dev/null");
        system(cmd);
      }
    else       { printf("\nUnable to write Pbox.inp file!"); psg_abort(1);}
  }

        shapead = getRsh("c13compad");
           adpw = shapead.pw;
        adpwlvl = shapead.pwr+2;
 }
 else
 { /* Second case: XSens Cold Probe, sp25 fixed amplitude pulse */
 (void) sprintf(fname,"%s/shapelib/sp25shape.RF",userdir);
 (void) sprintf(adshape,"sp25shape");
if ((getval("arraydim") < 1.5) || (ix==1))
  {
    (void) sprintf(fname, "%s/shapelib/Pbox.inp", userdir);
    if ( (fp=fopen(fname, "w")) != NULL )
      {
        fprintf(fp, "\n Pbox Input File ");
        fprintf(fp, "\n name = sp25shape.RF ");
        fprintf(fp, "\n { sp26 %5.1f }", sw*1.5);
        fprintf(fp, "\n RF Calibration Data: ref_pwr = %4d  ref_pw90 = %5.1f", (int)ref_pwr, ref_pw90*1.0e6);
        fclose(fp);
        sprintf(cmd,"cd $vnmruser/shapelib; Pbox > /dev/null");
        system(cmd);
      }
    else       { printf("\nUnable to write Pbox.inp file!"); psg_abort(1);}
  }

        shapead = getRsh("sp25shape");
           adpw = shapead.pw;
        adpwlvl = shapead.pwr;
 }
}

/* ACTUAL PULSE SEQUENCE BEGINS */
    obsstepsize(BASE);
   status(A);
      if (sspul[A] == 'y')
	{ zgradpulse(hsglvl,hsgt);
          rgpulse(pw,zero,rof1,rof1);
          zgradpulse(hsglvl,hsgt);
        }
      hsdelay(d1);
   status(B);
      xmtrphase(v10);
      rgpulse(pw, v1, rof1, rof1);
      if((adiabatic[A] == 'y') || (adiabatic[A] == 'p'))
       {
        delay(jtau-adpw/2);
        obspower(adpwlvl);
        shaped_pulse(adshape,adpw,v2,rof1,rof1);
        obspower(tpwr);
        delay(jtau-adpw/2);
       }
      else
       {
        delay(jtau);
        rgpulse(2.0*pw, v2, rof1, rof1);
        delay(jtau);
       }
   status(C);
      rgpulse(pw, v9, rof1, rof1);
      corr = 2.0*rof1 + 4.0*pw/3.14159265358979323846;
      xmtrphase(zero);
      corr += SAPS_DELAY;
      if (d2 > corr)
        delay(d2-corr);
      rgpulse(pw, v3, rof1, rof2);
   status(D);
}
