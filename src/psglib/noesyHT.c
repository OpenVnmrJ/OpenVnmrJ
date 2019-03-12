// Copyright (C) 2015  University of Oregon
// You may distribute under the terms of either the GNU General Public
// License or the Apache License, as specified in the LICENSE file.
// For more information, see the LICENSE file.
/*  noesyHT.c - Hadamard NOESY, Eriks Kupce, Oxford, 12.03.03   

    parameters:
    ==========
    htbw1 - excitation bandwidth. For pshape = 'gaus180' good numbers are 
          90, 45, 30 or 20 Hz.
    ni  - number of increments. Must be a power of 2. Other allowed values 
          are n*12, n*20 and n*28, where n is a power of 2.
    htofs1 - ni offset. Sets the number of increments to be omitted. Typical
            value is niofs = 1.
    pshape - shape used for Hadamard encoding, typically gaus180, square180,
             sinc180.
    httype - Hadamard encoding type. Allowed values are 'i' (inversion pulses
            are used for longitudinal encoding), 'e' and 'r' (excitation and
            refocusing pulses are used for transverse encoding). Nothe that
            pshape needs be chosen accordingly.
    bscor - Bloch-Siegert correction for Hadamard encoding, typically set 
            bscor = 'y'.
    repflg - set repflg = 'n' to suppress Pbox reports, repflg =  'h' prints
           Hadamard matrix only.
    ssF1 - stepsize for Hadamard encoding pulses. This parameter is adjusted
           by looking at the maximum phase increments in Hadamard enconing
           pulses, e.g. F1_2.RF. If unsure, set ssF1 = 0 to disable it.
    compH - H-1 amplifier compression factor.           
    mix - mixing time.
          
    htcal1 - allows calibration of selective pulses. Set mix=0, htcal1=ni, 
            ni=0 and array htpwr1. 
            Adjust compH so that pulse power matches the 
            optimum power found from the calibration.
*/ 

#include <standard.h>

static shape       ad180sh, roesySL;

void pulsesequence()
{
  char    pshape[MAXSTR], httype[MAXSTR], roesy[MAXSTR], ad180[MAXSTR], sspul[MAXSTR];
  double  rg1	= 2.0e-6, corr, 
          bwzq =  getval("bwzq"),
          mix	= getval("mix"),	/* mixing time */
	  compH = getval("compH"),
	  ofs   = getval("sw"),         /* offset for roesy spinlock */
	  gt0   = getval("gt0"),	/* gradient pulse preceding d1 */
	  gt2   = getval("gt2"),	/* gradient pulse preceding mixing */
	  gt3   = getval("gt3"),	/* gradient pulse following mixing */
	  gzlvl0 = getval("gzlvl0"),	/* gradient level for gt1 */
	  gzlvl2 = getval("gzlvl2"),	/* gradient level for gt2 */
	  gzlvl3 = getval("gzlvl3"),	/* gradient level for gt3 */
	  gstab = getval("gstab");	/* delay for gradient recovery */
  shape   hdx;
  void    setphases();

  getstr("pshape", pshape);
  getstr("httype", httype);
  getstr("roesy", roesy);
  getstr("ad180", ad180);
  getstr("sspul", sspul);

  (void) setphases();
  if(httype[0] == 'i')
    assign(zero,v2);
  if(compH<0.1) 
    compH = 1.0;
    
  if(FIRST_FID) 
  { 
    ad180sh.pw = 0.0;
    if(ad180[A] == 'z')
      ad180sh = pboxAshape("ad180", "wurst180", sfrq*100.0, 0.03, 0.0, pw*compH, tpwr); 
    if(ad180[A] == 'y')
      ad180sh = pbox_ad180("ad180", pw*compH, tpwr);  /* make adiabatic 180 */ 
    if(roesy[A] == 'y')
      roesySL = pbox_shape("roesySL", "amwurst", mix, ofs, pw*compH, tpwr);  
  }
  corr = gt2 + gt3 + ad180sh.pw + 3.0*gstab;

  hdx = pboxHT_F1i(pshape, pw*compH, tpwr); /* HADAMARD pulses */
    
  if (getval("htcal1") > 0.5)          /* Optional fine power calibration */
    hdx.pwr = getval("htpwr1");

 /* THE PULSE PROGRAMM STARTS HERE */

  status(A);

    delay(5.0e-5);
    zgradpulse(gzlvl0,gt0);
    if (sspul[0] == 'y')
    {
      rgpulse(pw,zero,rof1,rof1);
      zgradpulse(gzlvl0,gt0);
    }

    pre_sat();
      
    if (getflag("wet"))
      wet4(zero,one);
      
  status(B);
         
    obspower(tpwr);  
    ifzero(v1); 
      delay(2.0*(pw+rg1));
      zgradpulse(-gzlvl2*1.3,gt2);
      delay(gstab);        
    elsenz(v1);
      rgpulse(2.0*pw,v3,rg1,rg1);
      zgradpulse(-gzlvl2,gt2);
      delay(gstab);        
    endif(v1);
      
    pbox_pulse(&hdx, zero, rg1, rg1);
    zgradpulse(gzlvl2,gt2);
    delay(gstab);

    if(ad180[A] == 'z')
    {
      rgradient('z',getval("gzlvlzq")); 
      pbox_pulse(&ad180sh, zero, rg1, rg1);
      rgradient('z',0.0);
    }
          
    if(roesy[A] == 'y')
      pbox_pulse(&roesySL, zero, rg1, rg1);  /* ROESY spinlock */
    else if(ad180[A] == 'y')
    {
      delay(mix/2.0 - corr/2.0);
      pbox_pulse(&ad180sh, zero, rg1, rg1);  
      delay(mix/2.0 - corr/2.0);
    }
    else
      delay(mix-corr); 
       
    zgradpulse(gzlvl3,gt3); 
    
    obspower(tpwr);
    txphase(v3);
    delay(2.0*gstab);
    rgpulse(pw,v3,rg1,rof2);
    
  status(C);
}

/* ------------------------ Utility functions ------------------------- */

void setphases()
{

  mod4(ct, oph);            /* 0123 */
  mod2(ct,v1); 
  dbl(v1,v1);               /* 0202 */
  add(v1,oph,v3);           /* 0321 */ 
  add(one,v3,v2);           /* 1032 */
}
