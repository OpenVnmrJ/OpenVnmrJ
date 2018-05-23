// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
/* ROESYAD - adiabatic ROESY experiment with water suppression by gradient echo
		
Literature reference:
        T.L. Hwang and A.J. Shaka; JMR A112, 275-279 (1995) Excitation Sculpting
        C. Dalvit; J. Biol. NMR, 11, 437-444 (1998) Excitation Sculpting
        M.J. Trippleton and J. Keeler;

Parameters:
        sspul       - flag for optional GRD-90-GRD steady-state sequence
        mixR        - ROESY mixing time
        zfilt       - flag for Z-filter gradients during mixing
        gzlvlz      - gradient amplitude for the Z filter
        gtz         - gradient duration for the Z filter
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
        lkgate_flg  - lock gating option (on during d1 off during the seq. and at)

KrishK - Aug. 2006
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
static shape mixsh;

static char shapename1[MAXSTR];

static int phs1[32] = {0,0,0,0,2,2,2,2,0,0,0,0,2,2,2,2,1,1,1,1,3,3,3,3,1,1,1,1,3,3,3,3},
           phs2[32] = {2,0,2,0,2,0,2,0,2,0,2,0,2,0,2,0,3,1,3,1,3,1,3,1,3,1,3,1,3,1,3,1},
           phs3[32] = {0,0,0,0,2,2,2,2,0,0,0,0,2,2,2,2,1,1,1,1,3,3,3,3,1,1,1,1,3,3,3,3},
           phs4[32] = {2,0,2,0,0,2,0,2,3,1,3,1,1,3,1,3,3,1,3,1,1,3,1,3,0,2,0,2,2,0,2,0},
           phs5[32] = {0,0,2,2,0,0,2,2,1,1,3,3,1,1,3,3,1,1,3,3,1,1,3,3,2,2,0,0,2,2,0,0},
           phs6[32] = {0,0,0,0,2,2,2,2,1,1,1,1,3,3,3,3,1,1,1,1,3,3,3,3,2,2,2,2,0,0,0,0},
           phs8[16] = {0,1,0,1,2,3,2,3,0,1,0,1,2,3,2,3},
           phs9[16] = {0,0,1,1,0,0,1,1,2,2,3,3,2,2,3,3};
		
pulsesequence()
{
   double          mixR = getval("mixR"),
		   gzlvlz = getval("gzlvlz"),
		   gtz = getval("gtz"),
		   gstab = getval("gstab"),
                   slpwrR = getval("slpwrR"),
                   spinlockR = getval("spinlockR"),
                   tiltfactor = getval("tiltfactor"),
                   phincr1 = getval("phincr1"),
                   flippw = getval("flippw"),
                   tof = getval("tof"),
                   tpos1,tpos2; 
                   
   char		   zfilt[MAXSTR], 
                   alt_grd[MAXSTR], flipback[MAXSTR];
   int		   phase1 = (int)(getval("phase")+0.5);

/* LOAD VARIABLES */

//           (void) set_RS(0);   /* set up random sampling */

   getstr("zfilt",zfilt);
   getstr("flipback", flipback);
   getstr("alt_grd",alt_grd);
                     /* alternate gradient sign on every 2nd transient */
  
   tpos1 = tof + (spinlockR/tiltfactor);
   tpos2 = tof - (spinlockR/tiltfactor);

  assign(ct,v17);

  if (phincr1 < 0.0) phincr1=360+phincr1;
  initval(phincr1,v5);

   settable(t1,32,phs1);
   settable(t2,32,phs2);
   settable(t3,32,phs3);
   settable(t5,32,phs5);
   settable(t4,32,phs4);
   settable(t6,32,phs6);
   settable(t8,16,phs8);
   settable(t9,16,phs9);
  
   getelem(t1,v17,v6);   /* v6 - presat */
   getelem(t2,v17,v1);   /* v1 - first 90 */
   getelem(t3,v17,v2);   /* v2 - 2nd 90 */
   getelem(t4,v17,v7);   /* v7 - presat during mixN */
   getelem(t5,v17,v3);   /* v3 - 3rd 90 */
   getelem(t6,v17,oph);  /* oph - receiver */
   getelem(t8,v17,v8);   /* 1st echo in Excitation Sculpting */
   getelem(t9,v17,v9);   /* 2nd echo in Excitation Sculpting */

   /*  Make adiabatic roesy mixing shape */
     if(FIRST_FID)
       {
         opx("Pbox");
         pboxpar("steps", 1000.0);
         mixsh = pbox_shape("roesweep", "amwu", 0.0, 0.0, 0.0,0.0);
       }
   
   if (alt_grd[0] == 'y') mod2(ct,v10);

  initval(2.0*(double)(((int)(d2*getval("sw1")+0.5)%2)),v14);

   if (phase1 == 2)                              /* hypercomplex */
     {  incr(v1); }

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

   if (getflag("lkgate_flg"))  lk_hold(); /* turn lock sampling off */

   status(B);
      if (getflag("cpmgflg"))
      {
        rgpulse(pw, v1, rof1, 0.0);
        cpmg(v1, v15);
      }
      else
        rgpulse(pw, v1, rof1, 2.0e-6);
      if (d2>0)
       delay(d2- rof1 -(4*pw/PI));  /*corrected evolution time */
      else
       delay(d2);
      rgpulse(pw, v2, rof1, rof1);
      if (zfilt[0] == 'y')
         ifzero(v10); zgradpulse(gzlvlz,gtz);
         elsenz(v10); zgradpulse(-gzlvlz,gtz); endif(v10);
      delay(gstab);
      obsoffset(tpos1);
      obspower(slpwrR);
      shapedpulse("roesweep",mixR/2.0,v2,rof1,rof2);
      
      obsoffset(tpos2);
      shapedpulse("roesweep",mixR/2.0,v2,rof1,rof2);
      obspower(tpwr);
      obsoffset(tof);
      if (zfilt[0] == 'y')
         {
          ifzero(v10); zgradpulse(gzlvlz,gtz);
          elsenz(v10); zgradpulse(-gzlvlz,gtz); endif(v10);
          delay(gstab);
         }
      if (flipback[A] == 'y')
         FlipBack(v3,v5);

   status(C);
      rgpulse(pw, v3, rof1, 2.0e-6);
      ExcitationSculpting(v8,v9,v10);
      delay(rof2);

}
