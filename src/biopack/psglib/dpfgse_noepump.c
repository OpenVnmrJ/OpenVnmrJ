/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/**********************************************************
NOE pumping: NOESY sequence with BPPSTE diffusion filter
	ref : Aidi Chen and M. Shapiro,
		J. Am. Chem. Soc. 120, 10258-10259 (1998).

parameters:
	delflag   - 'y' runs the Dbppste sequence
                    'n' runs the normal s2pul sequence
        del       -  the actual diffusion delay
        gt1       - total diffusion-encoding pulse width
        gzlvl1    - diffusion-encoding pulse strength
        gzlvlhs   - homospoil gradient during mixing
        gths      - homospoil gradient duration
        wrefshape - shape file of the 180 deg. selective refocussing pulse
                      on the solvent (may be convoluted for multiple solvents)
        wrefpw    - pulse width for wrefshape (as given by Pbox)
        wrefpwr   - power level for wrefshape (as given by Pbox)
        wrefpwrf  - fine power for wrefshape by default it is 2048
                      needs optimization for multiple solvent suppression only
                      with fixed wrefpw
        alt_grd   - alternate gradient sign(s) in dpfgse on even transients
        trim_flg  - flag for an optional trim pulse at the end of the sequence
                              set trim_flg='y' tu suppress protein background
        trim      - pulse width of the trim pusle for trim_flg='y' (in seconds!!)
        trimpwr   - power level for the trim pulse for trim_flg='y'
        compH     - 1H amplifier compression
        dpfgse_flg- flag for solvent suppression with excitation sculpting
        gt2       - dpfgse diffusion-encoding pulse width
        gzlvl2    - dpfgse encoding pulse strength
        gstab     - gradient stabilization delay (~0.0002-0.0003 sec)
        satmode   - 'y' turns on presaturation

	tau taken as time between the mid-points
	of the bipolar gradient pulses.

p. sandor, darmstadt sept 2005.

**********************************************************/

#include <standard.h>
static int phi1[2]  = {0,2},
           phi2[8]  = {0,0,1,1,2,2,3,3},
           phi3[16] = {0,0,1,1,2,2,3,3,2,2,3,3,0,0,1,1},
           phi4[64] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                       1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
                       2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
                       3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3},
            rec[64] = {0,2,2,0,0,2,2,0,2,0,0,2,2,0,0,2,
                       1,3,3,1,1,3,3,1,3,1,1,3,3,1,1,3,
                       2,0,0,2,2,0,0,2,0,2,2,0,0,2,2,0,
                       3,1,1,3,3,1,1,3,1,3,3,1,1,3,3,1};


void pulsesequence()
{
double	del      = getval("del"),
        gstab    = getval("gstab"),
	gt1      = getval("gt1"),
	gzlvl1   = getval("gzlvl1"),
        gt2      = getval("gt2"),
        gzlvl2   = getval("gzlvl2"),
        gt0      = getval("gt0"),
        gzlvl0   = getval("gzlvl0"),
        gths     = getval("gths"),
        gzlvlhs  = getval("gzlvlhs"),
        satpwr   = getval("satpwr"),
        satdly   = getval("satdly"),
        satfrq   = getval("satfrq"),
        trim     = getval("trim"),
        trimpwr  = getval("trimpwr"),
        mix      = getval("mix"),
        wrefpwr  = getval("wrefpwr"),
        wrefpw   = getval("wrefpw"),
        wrefpwrf = getval("wrefpwrf");
char	delflag[MAXSTR],satmode[MAXSTR],dpfgse_flg[MAXSTR],sspul[MAXSTR],
        trim_flg[MAXSTR],alt_grd[MAXSTR],wrefshape[MAXSTR];

   getstr("delflag",delflag);
   getstr("satmode",satmode);
   getstr("dpfgse_flg",dpfgse_flg);
   getstr("trim_flg", trim_flg);
   getstr("alt_grd",alt_grd);
   getstr("wrefshape", wrefshape);
   getstr("sspul",sspul);

   if (alt_grd[0] == 'y') mod2(ct,v6);
               /* alternate gradient sign on every 2nd transient */

/* SET PHASES */
  sub(ct,ssctr,v12); /* Steady-state phase cycling */
  settable(t1, 2, phi1); getelem(t1,v12,v1);
  settable(t2, 8, phi2); getelem(t2,v12,v2);
  settable(t3,16, phi3); getelem(t3,v12,v3);
  settable(t4,64, phi4); getelem(t4,v12,v4);
  settable(t5,64, rec);  getelem(t5,v12,oph);

   /* equilibrium period */
   status(A);
      obspower(tpwr);
      if (sspul[A] == 'y')
      {
       zgradpulse(gzlvl0,gt0);
       rgpulse(pw,zero,rof1,rof1);
       zgradpulse(gzlvl0,gt0);
      }
      if (satmode[0] == 'y')
      {
       if (d1 - satdly > 0) delay(d1 - satdly);
       else delay(0.02);
       obspower(satpwr);
        if (satfrq != tof) obsoffset(satfrq);
        rgpulse(satdly,zero,rof1,rof1);
        if (satfrq != tof) obsoffset(tof);
       obspower(tpwr);
       delay(1.0e-5);
      }
      else delay(d1);
   status(B);
   /* first part of bppdel sequence */
   if (delflag[0]=='y')
   {  if (gt1>0 && gzlvl1>0)
      {  rgpulse(pw, zero, rof1, 0.0);		/* first 90, zero */

         ifzero(v6); zgradpulse(gzlvl1,gt1/2.0);
             elsenz(v6); zgradpulse(-1.0*gzlvl1,gt1/2.0); endif(v6);
   	 delay(gstab);
	 rgpulse(pw*2.0, zero, rof1, 0.0);	/* first 180, zero */
         ifzero(v6); zgradpulse(-1.0*gzlvl1,gt1/2.0);
             elsenz(v6); zgradpulse(gzlvl1,gt1/2.0); endif(v6);
   	 delay(gstab);
   	 rgpulse(pw, v1, rof1, 0.0);		/* second 90, v1 */

       if (satmode[1] == 'y')
        {
         obspower(satpwr);
         rgpulse(del-4.0*pw-3.0*rof1-gt1-2.0*gstab,zero,rof1,rof1);
         obspower(tpwr);
        }
       else
   	{
         delay(del-4.0*pw-3.0*rof1-gt1-2.0*gstab);/*diffusion delay */
        }
         rgpulse(pw, v2, rof1, 0.0);            /* third 90, v2 */

         ifzero(v6); zgradpulse(gzlvl1,gt1/2.0);
             elsenz(v6); zgradpulse(-1.0*gzlvl1,gt1/2.0); endif(v6);
   	 delay(gstab);
	 rgpulse(pw*2.0, zero, rof1, rof1);	/* second 180, zero */
         ifzero(v6); zgradpulse(-1.0*gzlvl1,gt1/2.0);
             elsenz(v6); zgradpulse(gzlvl1,gt1/2.0); endif(v6);
   	 delay(gstab);
         rgpulse(pw, v3, rof1, rof1);           /* mixing 90, v3 */
         delay(0.7*mix);
         ifzero(v6); zgradpulse(gzlvlhs,gths);
             elsenz(v6); zgradpulse(-1.0*gzlvlhs,gths); endif(v6);
         delay(0.3*mix-gths);
         if ((dpfgse_flg[A] == 'n')&&(trim_flg[0] == 'n')) rgpulse(pw, v4, rof1, rof2);
         else rgpulse(pw, v4, rof1, rof1);      /* read pulse  */

         /*   DPFGSE block   */

         if (dpfgse_flg[A] == 'y')
          {
           add(v4,two,v7);
           ifzero(v6); zgradpulse(gzlvl2,gt2);
                  elsenz(v6); zgradpulse(-gzlvl2,gt2); endif(v6);
           obspower(wrefpwr+6); obspwrf(wrefpwrf);
               delay(gstab);
           shaped_pulse(wrefshape,wrefpw,v7,rof1,rof1);
           obspower(tpwr); obspwrf(4095.0);
           rgpulse(2.0*pw,v4,rof1,rof1);
           ifzero(v6); zgradpulse(gzlvl2,gt2);
                  elsenz(v6); zgradpulse(-gzlvl2,gt2); endif(v6);
           obspower(wrefpwr+6); obspwrf(wrefpwrf);
           delay(gstab);
           ifzero(v6); zgradpulse(1.2*gzlvl2,gt2);
                  elsenz(v6); zgradpulse(-1.2*gzlvl2,gt2); endif(v6);
           delay(gstab);
           shaped_pulse(wrefshape,wrefpw,v7,rof1,rof1);
           obspower(tpwr); obspwrf(4095.0);
           if (trim_flg[A] == 'y') rgpulse(2.0*pw,v4,rof1,0.0);
           else        rgpulse(2.0*pw,v4,rof1,rof2);
           ifzero(v6); zgradpulse(1.2*gzlvl2,gt2);
                  elsenz(v6); zgradpulse(-1.2*gzlvl2,gt2); endif(v6);
           delay(gstab);
          }
          if (trim_flg[A] == 'y')
               { obspower(trimpwr);
                 add(v4,one,v5);
                rgpulse(trim,v5,rof1,rof2);
               }
     }
   }
   else
      rgpulse(pw,oph,rof1,rof2);

   /* --- observe period --- */

   status(C);
}

