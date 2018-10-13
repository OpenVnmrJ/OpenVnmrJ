/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* STD-dpfgse_noesy -   NOESY experiment with water suppression by gradient echo.
                    with optional ZQ artifact suppression during mixing  

Literature reference:
        T.L. Hwang and A.J. Shaka; JMR A112, 275-279 (1995) dpfgse
        C. Dalvit; J. Biol. NMR, 11, 437-444 (1998) dpfgse
        M.J. Trippleton and J. Keeler;
             Angew. Chem. Int. Ed. 2003, 42 3938-3941. ZQ suppression
        M Mayer and B. Meyer: Angew. Chem. Int. Ed. 38, No12 p. 1784 1999.
                                  STD difference

Parameters:
        sspul       - flag for optional GRD-90-GRD steady-state sequence
        gt0         - gradient duration for sspul
        gzlvl0      - gradient power for sspul
	mix	    - mixing time 
        zqflg	    - flap for optional ZQ artifact suppression
	gt1	    - gradient duration for the HS pulse during mix
	gzlvl1	    - gradient power for the HS pulse during mix
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
        wrefshape   - shape file of the 180 deg. selective refocussing pulse
                        on the solvent (may be convoluted for 
				multiple solvents)
        wrefpw     - pulse width for wrefshape (as given by Pbox)
        wrefpwr    - power level for wrefshape (as given by Pbox) 
        wrefpwrf   - fine power for wrefshape
                      by default it is 2048 needs optimization for
                      multiple solvent with fixed wrefpw suppression only
        gt2        - gradient duration for the solvent suppression echo
        gzlvl2     - gradient power for the solvent suppression echo
        alt_grd    - flag for alternating gradient sign in mix-dpfgse segment
        gstab      - gradient stabilization delay
        xferdly     - saturation transfer delay ( ~1.5-2sec)
        satshape    - shape of the pulses in the pulse-train (def:gauss)
        d3          - interpulse delay in the xfer pulsetrain
                        ( 1 msec is recommenden in the literature
                                with no obvious reason)
        satpwr      - power level for the saturation pulse-train
                       (in the literature 86 Hz peak power is recommended
                       corresponding to a 630 deg. flip angle at 50 msec satpw
                       please note that the actual flip angle is irrelevant,
                       the selectivity is controlled by the power level, satpwr)
        satpw       - pulse width of the shaped pulses in the pulse train
                            duration ca 50 ms
        satfrq      - frequency for protein saturation
                         (use a region with an intense protein signal
                         and NO ligand signal)
        satfrqref   - reference frequency (outside the signal region ~at 30ppm)
        trim_flg   - flag for an optional trim pulse at the end of the 
		       sequence
                       set trim_flg='y' tu suppress protein background
        trim       - pulse width of the trim pusle for trim_flg='y'
        trimpwr    - power level for the trim pulse for trim_flg='y'
p. sandor, darmstadt march 2006.
modified for BioPack, G.Gray, Palo Alto Aug 2007
*/	  

static int phi1[32] = {0,0,0,0,0,0,0,0,2,2,2,2,2,2,2,2,
                       1,1,1,1,1,1,1,1,3,3,3,3,3,3,3,3},
           phi2[32] = {0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,
                       1,1,2,2,1,1,2,2,1,1,2,2,1,1,2,2},
           phi3[32] = {2,2,3,3,2,2,3,3,2,2,3,3,2,2,3,3,
                       3,3,0,0,3,3,0,0,3,3,0,0,3,3,0,0},
           phi4[32] = {0,0,0,0,1,1,1,1,0,0,0,0,1,1,1,1,
                       1,1,1,1,2,2,2,2,1,1,1,1,2,2,2,2},
           phi5[32] = {2,2,2,2,3,3,3,3,2,2,2,2,3,3,3,3,
                       3,3,3,3,0,0,0,0,3,3,3,3,0,0,0,0},
           phi7[32] = {0,0,2,2,0,0,2,2,0,0,2,2,0,0,2,2,
                       1,1,3,3,1,1,3,3,1,1,3,3,1,1,3,3},
           phi8[32] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                       1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
           phi9[32] = {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
                       2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2},
           rec[32]  = {0,2,0,2,2,0,2,0,2,0,2,0,0,2,0,2,
                       1,3,1,3,3,1,3,1,3,1,3,1,1,3,1,3};

#include <standard.h>

pulsesequence ()
{
double  gstab = getval("gstab"),
	gt1 = getval("gt1"),
	gzlvl1 = getval("gzlvl1"),
        gt2 = getval("gt2"),
        gzlvl2 = getval("gzlvl2"),
        mix = getval("mix"),
        wrefpw = getval("wrefpw"),
        wrefpwr = getval("wrefpwr"),
        wrefpwrf = getval("wrefpwrf"),
        phincr1 = getval("phincr1"),
        flippwr = getval("flippwr"),
        flippwrf = getval("flippwrf"),
        flippw = getval("flippw"),
        trimpwr = getval("trimpwr"),
        gt0 = getval("gt0"),
        gzlvl0 = getval("gzlvl0"),
	trim = getval("trim"),
        satpwr = getval("satpwr"),
        satfrq = getval("satfrq"),
        satfrqref = getval("satfrqref"),
        satpw = getval("satpw"),
        d3 = getval("d3"),
        xferdly = getval("xferdly"),
        h1freq_local = getval("h1freq_local"),
        gcal_local = getval("gcal_local"),
        coil_size = getval("coil_size"),
        swfactor = 9.0,    /* do the adiabatic sweep over 9.0*sw  */
        zqpwr=getval("zqpwr"), zqpw=getval("zqpw"),
        gzlvlzq,invsw,cycles;
	
int	iphase = (int) (getval("phase") + 0.5);
char    sspul[MAXSTR], trim_flg[MAXSTR], wrefshape[MAXSTR],flipback[MAXSTR], 
        zqshape[MAXSTR],
	zqflg[MAXSTR], alt_grd[MAXSTR],flipshape[MAXSTR],satshape[MAXSTR];
        
  getstr("sspul", sspul);
  getstr("trim_flg", trim_flg);
  getstr("wrefshape", wrefshape);
  getstr("flipshape", flipshape);
  getstr("flipback", flipback);
  getstr("zqflg", zqflg);
  getstr("zqshape", zqshape);
  getstr("alt_grd",alt_grd);
  getstr("satshape",satshape);
  rof1 = getval("rof1"); if(rof1 > 2.0e-6) rof1=2.0e-6;
  if (phincr1 < 0.0) phincr1=360+phincr1;
  initval(phincr1,v13);
  cycles = xferdly/(d3+satpw) + 0.5;
     initval(cycles,v14);

  if (coil_size == 0) coil_size=16;
  invsw = sw*swfactor;
  if (invsw > 60000.0) invsw = 60000.0; /* do not exceed 60 kHz */
  invsw = invsw/0.97;     /* correct for end effects of the cawurst-20 shape */

  if ((zqflg[0] == 'y') && (mix < 0.051))
   {
     printf("Mixing time should be more than 51 ms for zero quantum suppression\n");
     psg_abort(1);
   }

  gzlvlzq=(invsw*h1freq_local*2349)/(gcal_local*coil_size*sfrq*1e+6);

  sub(ct,ssctr,v12);
  settable(t1,32,phi1);		  getelem(t1,v12,v1);
  settable(t2,32,phi2);		  getelem(t2,v12,v2);
  settable(t3,32,phi3);		  getelem(t3,v12,v3);	
  settable(t4,32,phi4);		  getelem(t4,v12,v4);
  settable(t5,32,phi5);		  getelem(t5,v12,v5);
  settable(t7,32,phi7);		  getelem(t7,v12,v7);
/*  settable(t8,32,phi8);		  getelem(t8,v12,v8); */
  settable(t6,32,rec);		  getelem(t6,v12,oph);
  settable(t9,32,phi9);		  getelem(t9,v12,v6);

  if (zqflg[0] == 'y') add(oph,two,oph);
  hlv(ct,v10); mod2(v10,v10);        /*changing gradient sign between sanc1-2 and 3-4 etc. */

  if (iphase == 2) 
   { incr(v1); incr(v6); }
/* HYPERCOMPLEX MODE USES REDFIELD TRICK TO MOVE AXIAL PEAKS TO EDGE */
   initval(2.0*(double)(((int)(d2*getval("sw1")+0.5)%2)),v9);
   if ((iphase == 1)||(iphase == 2)) {add(v1,v9,v1); add(oph,v9,oph), add(v6,v9,v6);}

status(A);
 obspower(tpwr); obspwrf(4095.0); decpower(dpwr);
   if (sspul[A] == 'y')
    {
       zgradpulse(gzlvl0,gt0);
       rgpulse(pw,zero,rof1,rof1);
       zgradpulse(gzlvl0,gt0);
    }
   if (d1 > xferdly) delay(d1-xferdly);

             /* set saturation frequencies */
   mod2(ct,v8);                /*  0 1 0 1 0 1 0 1 ..frequency  switch
                                  on every second transient */
   ifzero(v8); obsoffset(satfrq);
   elsenz(v8); obsoffset(satfrqref);
   endif(v8);

 /*  Start the selective saturation of protein */

    obspower(satpwr);
    if (cycles > 0.0)
   {
    starthardloop(v14);
      delay(d3);
      shaped_pulse(satshape,satpw,zero,rof1,rof1);
      endhardloop();
   }
   obspower(tpwr); obsoffset(tof);

status(B);
   settable(t8,32,phi8);           getelem(t8,v12,v8);
   obsstepsize(45.0);
   initval(7.0,v11);
   xmtrphase(v11);

   rgpulse(pw,v1,rof1,rof1);
   if (trim_flg[0] == 'y')
	{ obspower(trimpwr);
          rgpulse(trim,v6,rof1,rof1);
          obspower(tpwr);
        }
   xmtrphase(zero);
   if (trim_flg[0] == 'y')
     {
      if (d2-2.0*pw/3.14 - 2.0*rof1 - SAPS_DELAY - 2.0*POWER_DELAY> 0)
                 delay(d2-2.0*pw/3.14-2.0*rof1-SAPS_DELAY - 2.0*POWER_DELAY);
      else
        delay(0.0);
     }
   else
     {
      if (d2-4.0*pw/3.14 - 2.0*rof1 - SAPS_DELAY> 0)
                 delay(d2-4.0*pw/3.14-2.0*rof1-SAPS_DELAY);
      else
        delay(0.0);
     }     
   rgpulse(pw,v7,rof1,rof1);
     if (zqflg[0] == 'y')
      {
            ifzero(v10); rgradient('z',gzlvlzq);
                 elsenz(v10); rgradient('z',-1.0*gzlvlzq); endif(v10);
          obspower(zqpwr);
          shaped_pulse(zqshape,zqpw,zero,rof1,rof1);
          obspower(tpwr);
          rgradient('z',0.0);
          delay((mix-0.050-gt1)*0.7);
          if (alt_grd[0] == 'y')
           {
              ifzero(v10); zgradpulse(gzlvl1,gt1);
                    elsenz(v10); zgradpulse(-1.0*gzlvl1,gt1); endif(v10);
           }
          else zgradpulse(gzlvl1,gt1);
          if (flipback[0] == 'n')
            delay((mix-0.05-gt1)*0.3);
          else     
            { delay((mix-0.05-gt1)*0.3 - flippw - rof1);
              obsstepsize(1.0);
              xmtrphase(v13);
              add(v8,two,v8);
              obspower(flippwr+6); obspwrf(flippwrf);
              shaped_pulse(flipshape,flippw,v8,rof1,rof1);
              xmtrphase(zero);
              add(v8,two,v8);
              obspower(tpwr); obspwrf(4095.0);
            }
      }
     else
      {
         delay(mix*0.7);
         if (alt_grd[0] == 'y')
           {
             ifzero(v10); zgradpulse(gzlvl1,gt1);
                    elsenz(v10); zgradpulse(-1.0*gzlvl1,gt1); endif(v10);
           }
         else zgradpulse(gzlvl1,gt1);
         if (flipback[0] == 'n')
           delay(mix*0.3-gt2); 
          else    
            { delay(mix*0.3 - flippw - rof1);
              obsstepsize(1.0);
              xmtrphase(v13);
              add(v8,two,v8);
              obspower(flippwr+6); obspwrf(flippwrf);
              shaped_pulse(flipshape,flippw,v8,rof1,rof1);
              xmtrphase(zero);
              add(v8,two,v8);
              obspower(tpwr); obspwrf(4095.0);
            }
      }
   obspower(tpwr);
   rgpulse(pw,v8,rof1,rof1);
   if (alt_grd[0] == 'y')
    {
     ifzero(v10); zgradpulse(gzlvl2,gt2); 
		elsenz(v10); zgradpulse(-1.0*gzlvl2,gt2); endif(v10);
    }
    else zgradpulse(gzlvl2,gt2);
   delay(gstab);
   obspower(wrefpwr+6); obspwrf(wrefpwrf);
   shaped_pulse(wrefshape,wrefpw,v5,rof1,rof1);
   obspower(tpwr); obspwrf(4095.0);
   rgpulse(2.0*pw,v4,rof1,rof1);
   if (alt_grd[0] == 'y')
    {
     ifzero(v10); zgradpulse(gzlvl2,gt2);
                elsenz(v10); zgradpulse(-1.0*gzlvl2,gt2); endif(v10);
    }
    else zgradpulse(gzlvl2,gt2);
   delay(gstab);
   if (alt_grd[0] == 'y')
    {
     ifzero(v10); zgradpulse(1.2*gzlvl2,gt2);
                elsenz(v10); zgradpulse(-1.2*gzlvl2,gt2); endif(v10);
    }
    else zgradpulse(1.2*gzlvl2,gt2);
   delay(gstab);
   obspower(wrefpwr+6); obspwrf(wrefpwrf);
   shaped_pulse(wrefshape,wrefpw,v3,rof1,rof1);
   obspower(tpwr); obspwrf(4095.0);
   rgpulse(2.0*pw,v2,rof1,rof2);
   if (alt_grd[0] == 'y')
    {
     ifzero(v10); zgradpulse(1.2*gzlvl2,gt2);
                elsenz(v10); zgradpulse(-1.2*gzlvl2,gt2); endif(v10);
    }
    else zgradpulse(1.2*gzlvl2,gt2);
   delay(gstab);
   status(C);
}
