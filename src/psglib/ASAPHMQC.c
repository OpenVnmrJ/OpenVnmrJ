// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
/* ASAP_hmqc.c - ASAP - Acceleration by Sharing Adjacent Polarization
		 E. Kupce and R. Freeman, Magn. Reson. Chem., v.45 pp.2-4 (2007).

		hsglvl  :	Homospoil gradient level (DAC units)
		hsgt	:	Homospoil gradient time
		gzlvlE	:	Encoding gradient level
		gtE	:	Encoding gradient time
                EDratio :       Encode/Decode ratio
		gstab	:	Recovery delay
		j1xh	:	One-bond XH coupling constant
		pwxlvl  :	X-nucleus pulse power
		pwx	:	X-nucleus 90 deg pulse width
		d1	:	Relaxation delay
		d2	:	Evolution delay
		asap    :       Flag to switch between ASAP and conventional mode
                mix     :       Mixing time for polarization sharing, ca 50 ms (asap)
                adiabatic:      Flag to use adiabatic pulses (y = preferred mode)
		tpwr_cf :       H1 amplifier compression factor
		lkgate_flg:	Lock gating flag

Eriks Kupce, Agilent UK 11 Nov. 2006: Original pulse sequence
Bert Heise, Agilent UK Feb. 2008: Chempack compatible version
Bert Heise, Agilent UK Jul. 2010: Lock gating added for better peak shapes
*/

#include <standard.h>
#include <chempack.h>
#ifndef   FIRST_FID
#include <Pbox_psg.h>
#endif

static shape mixsh, ad180;

static int	   ph1[2] = {0,2},
	 	   ph2[4] = {0,0,2,2},
	   	   ph3[4] = {0,2,2,0};

pulsesequence()
{
  double mix = getval("mix"),
	 pw180 = 0.0,
	 tpwr_cf = getval("tpwr_cf"),
         j1xh = getval("j1xh"), 
         hsglvl = getval("hsglvl"),
         hsgt = getval("hsgt"),
         gzlvlE = getval("gzlvlE"),
         gtE = getval("gtE"),
         EDratio = getval("EDratio"),
         gstab = getval("gstab"),
	 pwxlvl = getval("pwxlvl"),
	 pwx = getval("pwx"),
	 taua=getval("taua");       /* asap delay ca 0.3/J */
  char   asap[MAXSTR], adiabatic[MAXSTR],lkgate_flg[MAXSTR],cmd[MAXSTR];
  int    ncycles=0, icosel=1,
         iphase = (int)(getval("phase")+0.5);

  getstr("asap",asap);    
  getstr("adiabatic",adiabatic);    
  getstr("lkgate_flg",lkgate_flg);
  if(j1xh<1.0) j1xh=150.0;
   
  if (FIRST_FID)
  {
    if(adiabatic[A] == 'y')
      ad180 = pbox_ad180("ad180", pwx, pwxlvl);    /* make adiabatic 180 */
    sprintf(cmd, "Pbox wu2mix -u %s -w \"WURST2m 20k/0.4m\" -p %.0f -l %.2f -s 2.0",userdir,
    tpwr, 1.0e6*pw*tpwr_cf);
    system(cmd);
    mixsh = getDsh("wu2mix");
  }

  if(adiabatic[A] == 'y')
    pw180 = ad180.pw;

  ncycles = (int) (0.5 + mix/mixsh.pw);

  if ((0.5/j1xh) < (gtE+gstab))
  {
    text_error("0.5/1jxh must be greater than gtE+gstab\n");
    psg_abort(1);
  }

  settable(t1,2,ph1);
  settable(t2,4,ph2);
  settable(t3,4,ph3);

  assign(zero,v4);
  getelem(t1,ct,v1);
  getelem(t3,ct,oph);

  if (iphase == 2)
    icosel = -1;

  initval(2.0*(double)((int)(d2*getval("sw1")+0.5)%2),v10); 
  add(v1,v10,v1);
  add(v4,v10,v4);
  add(oph,v10,oph);

  status(A);
  
     if (lkgate_flg[0] == 'y')  lk_hold(); /* turn lock sampling off */
     if (asap[A] == 'y')
     {
        mix = 7.0e-5 + 2.0*POWER_DELAY + 2.0*hsgt + mixsh.pw*ncycles;
        delay(1.0e-5);
        obspower(mixsh.pwr);
        zgradpulse(hsglvl,hsgt);
        delay(5.0e-5);

        obsunblank(); xmtron();
        obsprgon(mixsh.name, 1.0/mixsh.dmf, mixsh.dres);
        delay(mixsh.pw*ncycles);
	obsprgoff(); obsblank();
        xmtroff(); 

           
        delay(1.0e-5);
        zgradpulse(hsglvl,hsgt);
        obspower(tpwr);
        delay(d1 - mix);
      }
      else
      {
        delay(1.0e-5);
        obspower(tpwr);
        zgradpulse(hsglvl,2.0*hsgt);
        rgpulse(pw, zero, rof1, rof1);
        zgradpulse(hsglvl,hsgt);
        rgpulse(pw, one, rof1, rof1);
        zgradpulse(0.5*hsglvl,hsgt);
        delay(d1 - 2.0*hsgt - 4.0*rof1 - 2.0*pw - 1.0e-5 - POWER_DELAY);
      }


   if (satmode[0] == 'y')
     {
        if ((d1-satdly) > 0.02)
                delay(d1-satdly);
        else
                delay(0.02);
        if (getflag("slpsat"))
           {
                shaped_satpulse("relaxD",satdly,zero);
                if (getflag("prgflg"))
                   shaped_purge(v6,zero,v18,v19);
           }
        else
           {
                satpulse(satdly,zero,rof1,rof1);
                if (getflag("prgflg"))
                   purge(v6,zero,v18,v19);
           }
     }

   if (getflag("wet"))
     wet4(zero,one);

  status(B);
     decpower(pwxlvl);

      if (asap[0] == 'y')
      {
        rgpulse(pw,zero,rof1,rof1);
        delay(taua/2.0 - pw - 2.0*rof1 + pw180/2.0);
	if(adiabatic[A] == 'y')
	{
          rgpulse(2.0*pw,two,rof1,0.0);
          decshaped_pulse(ad180.name,ad180.pw,zero,0.0,rof1);
	}
	else
          simpulse(2.0*pw,2.0*pwx,two,zero,rof1,rof1); 
        delay(taua/2.0 - pw - 2.0*rof1 - pw180/2.0);
        simpulse(pw,pwx,zero,v1,rof1,1.0e-6);
      }
      else
      {
        rgpulse(pw,zero,rof1,rof1);
        delay(taua - rof1 - (2*pw/PI));
        decrgpulse(pwx,v1,rof1,1.0e-6);
      }
      delay(gtE + 2*gstab + 2*GRADIENT_DELAY - (2*pwx/PI) - pwx - 1.0e-6 - rof1);
      if(adiabatic[A] == 'y')
        decshaped_pulse(ad180.name,ad180.pw,v4,rof1,1.0e-6);
      else
        decrgpulse(2*pwx,v4,rof1,1.0e-6); 
      delay(gstab - pwx - 1.0e-6);
      zgradpulse(gzlvlE,gtE);
      delay(gstab - rof1 - pw);
      delay(d2/2);
      rgpulse(2.0*pw,zero,rof1,rof1);
      delay(d2/2);

      delay(gstab - rof1 - pw);
      zgradpulse(gzlvlE,gtE);
      delay(gstab - pwx - rof1);
      if(adiabatic[A] == 'y')
        decshaped_pulse(ad180.name,ad180.pw,zero,rof1,1.0e-6);
      else
        decrgpulse(2*pwx,zero,rof1,1.0e-6); 
      delay(gtE + 2*gstab + 2*GRADIENT_DELAY - (2*pwx/PI) - pwx - 1.0e-6 - rof1);
      decrgpulse(pwx,t2,rof1,rof1);
      if(asap[0] == 'y')
      {
        delay(0.25/j1xh + ad180.pw/2.0);    
        if(adiabatic[A] == 'y')
	{
          rgpulse(2.0*pw,zero,rof1,0.0);
          decshaped_pulse(ad180.name,ad180.pw,zero,0.0,rof1);
	}
	else
          simpulse(2.0*pw,2.0*pwx,zero,zero,rof1,rof1); 	
        delay(0.1*gstab - rof1);
        if(EDratio > 4)
        {
          zgradpulse(icosel*gzlvlE/EDratio*4.0,gtE/2.0);
          delay(0.25/j1xh - ad180.pw/2.0 - gtE/2.0 - 0.1*gstab - 2*GRADIENT_DELAY);
        }
        else
        {
          zgradpulse(icosel*gzlvlE/EDratio*2.0,gtE);
          delay(0.25/j1xh - ad180.pw/2.0 - gtE - 0.1*gstab - 2*GRADIENT_DELAY);
        }
      }
      else
      {
        delay(0.1*gstab - rof1);
        if(EDratio > 4)
        {
          zgradpulse(icosel*gzlvlE/EDratio*4.0,gtE/2.0);
          delay(0.5/j1xh - gtE/2.0 - 0.1*gstab - 2*GRADIENT_DELAY);
        }
        else
        {
          zgradpulse(icosel*gzlvlE/EDratio*2.0,gtE);
          delay(0.5/j1xh - gtE - 0.1*gstab - 2*GRADIENT_DELAY);
        }
      }
      if (lkgate_flg[0] == 'y') lk_sample(); /* turn lock sampling on */
      decpower(dpwr);
      delay(rof2 - POWER_DELAY);
      
  status(C);
} 

