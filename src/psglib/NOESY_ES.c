// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
/* NOESY_ES - NOESY experiment with water suppression by gradient echo.
                    with optional ZQ artifact suppression during mixing 

Literature reference:
        T.L. Hwang and A.J. Shaka; JMR A112, 275-279 (1995) Excitation Sculpting
        C. Dalvit; J. Biol. NMR, 11, 437-444 (1998) Excitation Sculpting
        M.J. Trippleton and J. Keeler;
             Angew. Chem. Int. Ed. 2003, 42 3938-3941. ZQ suppression

Parameters:
        sspul       - flag for optional GRD-90-GRD steady-state sequence
        mixN        - NOESY mixing time
        Gzqfilt     - flag for optional ZQ artifact suppression
        zqfpw1      - pulse width for the ZQ filter
        zqfpwr1     - power level for the ZQ filter
        gzlvlzq1    - gradient amplitude for the ZQ filter
        zqfpat1     - shape pattern for the ZQ filter
	gzlvl1      - homospoil gradient amplitude
        gt1         - homospoil gradient duration
        gstab       - gradient stabilization delay
        flipback    - flag for an optional selective 90 flipback pulse
                                on the solvent to keep it along z all time
        flipshape   - shape of the selective pulse for flipback='y'
        flippw      - pulse width of the selective pulse for flipback='y'
        flippwr     - power level of the selective pulse for flipback='y'
        flippwrf    - fine power for flipshape by default it is 2048
                        may need optimization with fixed flippw and flippwr
        phincr1     - small angle phase shift between the hard and the
                            selective pulse for flipback='y' (1 deg. steps)
                            to be optimized for best result
        esshape     - shape file of the 180 deg. selective refocussing pulse
                        on the solvent (may be convoluted for multiple solvents)
        essoftpw    - pulse width for esshape (as given by Pbox)
        essoftpwr   - power level for esshape (as given by Pbox)        
        essoftpwrf  - fine power for esshape by default it is 2048
                        needs optimization for multiple solvent suppression only
                                with fixed esoftpw 
        esgzlvl     - gradient power for the solvent suppression echo
        esgt        - gradient duration for the solvent suppression echo
        alt_grd     - alternate gradient sign(s) in dpfgse on even transients
        lkgate_flg  - lock gating option (on during d1, off during the seq. and at)

The water refocusing shape and the water flipback shape can be created/updated
using the "make_es_shape" and "make_es_flipshape" macros, respectively. For
multiple frequency solvent suppression the esshape file needs to be created
manually.
PeterS - Excitation Sculpting added 2012
JohnR - includes CPMG option : Jan 2015
****v15 is reserved for CPMG ***

*/

#include <standard.h>
#include <chempack.h>
/*
#include <ExcitationSculpting.h>
#include <FlipBack.h>
*/

static int phs1[32] = {0,0,0,0,2,2,2,2,0,0,0,0,2,2,2,2,1,1,1,1,3,3,3,3,1,1,1,1,3,3,3,3},
           phs2[32] = {2,0,2,0,2,0,2,0,2,0,2,0,2,0,2,0,3,1,3,1,3,1,3,1,3,1,3,1,3,1,3,1},
           phs3[32] = {0,0,0,0,2,2,2,2,0,0,0,0,2,2,2,2,1,1,1,1,3,3,3,3,1,1,1,1,3,3,3,3},
           phs7[32] = {2,0,2,0,0,2,0,2,3,1,3,1,1,3,1,3,3,1,3,1,1,3,1,3,0,2,0,2,2,0,2,0},
           phs5[32] = {0,0,2,2,0,0,2,2,1,1,3,3,1,1,3,3,1,1,3,3,1,1,3,3,2,2,0,0,2,2,0,0},
           phs8[16] = {0,1,0,1,2,3,2,3,0,1,0,1,2,3,2,3},
           phs9[16] = {0,0,1,1,0,0,1,1,2,2,3,3,2,2,3,3},
            rec[32] = {0,0,0,0,2,2,2,2,1,1,1,1,3,3,3,3,1,1,1,1,3,3,3,3,2,2,2,2,0,0,0,0};
		
pulsesequence()
{
   double          mixN = getval("mixN"),
		   mixNcorr,fbcorr,d2corr,
                   phincr1 = getval("phincr1"),
		   gzlvl1 = getval("gzlvl1"),
		   gt1 = getval("gt1"),
		   gstab = getval("gstab"),
                   zqfpw1 = getval("zqfpw1"),
                   zqfpwr1 = getval("zqfpwr1"),
                   gzlvlzq1 = getval("gzlvlzq1"),
                   flippw = getval("flippw");
   char		   alt_grd[MAXSTR], zqfpat1[MAXSTR],
		   flipback[MAXSTR];
   int		   phase1 = (int)(getval("phase")+0.5);

/* LOAD VARIABLES */

//           (void) set_RS(0);   /* set up random sampling */ 
 
   getstr("zqfpat1",zqfpat1);
   getstr("flipback", flipback);
   getstr("alt_grd",alt_grd); 
                     /* alternate gradient sign on every 2nd transient */
                              
   if (flipback[A] == 'y') fbcorr = flippw + 2.0*rof1;
   else fbcorr = 0.0;    /* correck for flipback if used */
   if (phincr1 < 0.0) phincr1=360+phincr1;
   initval(phincr1,v5); 
   mixNcorr = fbcorr;

   if (getflag("PFGflg"))
   {
	mixNcorr = gt1 + gstab;
	if (getflag("Gzqfilt"))
		mixNcorr += gstab + zqfpw1;
   }

   if (mixNcorr > mixN)
	mixN=mixNcorr;

  assign(ct,v17);

   settable(t1,32,phs1);
   settable(t2,32,phs2);
   settable(t3,32,phs3);
   settable(t5,32,phs5);
   settable(t7,32,phs7);
   settable(t8,16,phs8);
   settable(t9,16,phs9);
   settable(t6,32,rec);
  
   getelem(t1,v17,v6);   /* v6 - presat */
   getelem(t2,v17,v1);   /* v1 - first 90 */
   getelem(t3,v17,v2);   /* v2 - 2nd 90 */
   getelem(t7,v17,v7);   /* v7 - presat during mixN */
   getelem(t5,v17,v3);   /* v3 - 3rd 90 */
   getelem(t8,v17,v8);   /* 1st ES echo */
   getelem(t9,v17,v9);   /* 2nd ES echo */
   getelem(t6,v17,oph);  /* oph - receiver */

   if (alt_grd[0] == 'y') mod2(ct,v10);

  initval(2.0*(double)(((int)(d2*getval("sw1")+0.5)%2)),v14);

   if (phase1 == 2)                                        /* hypercomplex */
     {  incr(v1);  }

   add(v1, v14, v1);
   add(oph,v14,oph);

/* BEGIN THE ACTUAL PULSE SEQUENCE */
   status(A);

   if (getflag("lkgate_flg"))  lk_sample(); /* turn lock sampling on */

   obspower(tpwr);
   delay(5.0e-5);
   if (getflag("sspul"))
        steadystate();

   delay(d1);

   d2corr = rof1 +  2.0e-6 + (4*pw/PI);

   if (getflag("lkgate_flg"))  lk_hold(); /* turn lock sampling off */

   status(B);

      if (getflag("cpmgflg"))
      {
        rgpulse(pw, v1, rof1, 0.0);
        cpmg(v1, v15);
      }
      else
        rgpulse(pw, v1, rof1, 2.0e-6);

      if (d2>d2corr)
        delay(d2 - d2corr); /*corrected evolution time */
      else
        delay(d2);

      rgpulse(pw, v2, rof1, rof1);

      delay((mixN - mixNcorr)*0.7);

      if (getflag("PFGflg"))
      {
        if (getflag("Gzqfilt"))
        {
         obspower(zqfpwr1);
         ifzero(v10); rgradient('z',gzlvlzq1);
         elsenz(v10); rgradient('z',-gzlvlzq1); endif(v10);
         delay(100.0e-6);
         shaped_pulse(zqfpat1,zqfpw1,zero,rof1,rof1);
         delay(100.0e-6);
         rgradient('z',0.0);
         delay(gstab);
	 obspower(tpwr);
        }
        ifzero(v10); zgradpulse(gzlvl1,gt1);
        elsenz(v10); zgradpulse(-gzlvl1,gt1); endif(v10);
        delay(gstab);
      }
      delay((mixN - mixNcorr)*0.3);

      if (flipback[A] == 'y')
         FlipBack(v3,v5);

   status(C);
      rgpulse(pw, v3, rof1, 2.0e-6);
      ExcitationSculpting(v8,v9,v10);
      delay(rof2);
}
