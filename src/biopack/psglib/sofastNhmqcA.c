/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*  sofastNhmqcA.c 

 "A" version of sofastNhmqc.c. This offers constant-time evolution and the
 option of user-selectable Variable Angle selective pulse to begin the exp.
 The variable "fla" is used to select the proper vap file from wavelib. If
 shname1 is not 'vap', a pc9 default shape is used (for the flip angle "fla".


   Schanda & Brutscher, JACS 2005, 127, 8014   
   Schanda, Kupce, Brutscher, JBiomolNMR 2005  
   ab_flg='a','b'  and  dm2='nnn'for IPAP              
   requires no 15N decoupling                  


   for ab_flg='a','b' phase=1,2 array='ab_flg,phase'
   you can acquire both antiphase and in-phase spectra at the same time. To
   obtain both components with the same phase use
        wft2d(1,0,0,0,0,0,0,0,0,0,-1,0,0,0,0,0)     for anti-phase signals
        wft2d(0,0,0,0,0,0,1,0,0,0,0,0,1,0,0,0)      for in-phase signals

   submitted by Eriks Kupce (Varian), Sept 2005
   added to BioPack, September 2005 (GG)


*/
#include <standard.h>
#include <Pbox_psg.h>

static shape sh1, sh2, sh2dec;

void pulsesequence()
{
   char   CT_flg[MAXSTR],	/* Constant time flag */
	  f1180[MAXSTR],
          shname1[MAXSTR],      /* First pulse: name or 'vap' for
                                   automatic VAP(Variable angle pulse) */
	  ab_flg[MAXSTR];	/* inversion of 15N for coupling*/
	  
   int    t1_counter,
          phase;

   double d2_init=0.0, pwNa = 0.0,
   	  adjust = getval("adjust"),
          ref_pwr = getval("ref_pwr"),
          ref_pw90 = getval("ref_pw90"),
          fla = getval("fla"),             /* flip-angle */
          gzlvl1 = getval("gzlvl1"),
          gzlvl2 = getval("gzlvl2"), 
          gt1 = getval("gt1"),
          gt2 = getval("gt2"),
          gstab = getval("gstab"),
          pwNlvl = getval("pwNlvl"),
          pwN = getval("pwN"),
          dpwr2 = getval("dpwr2"),
          d2 = getval("d2"),
          tau1 = getval("tau1"),
          taunh = 1/(2.0*getval("JNH"));

   void   compo_pulse(), make_shapes();

   getstr("CT_flg", CT_flg);
   getstr("f1180",f1180);
   getstr("shname1",shname1);
   getstr("ab_flg",ab_flg);

   if(ab_flg[A] == 'a' || ab_flg[A] == 'b')
     pwNa = pwN;
  
   phase = (int) (getval("phase") + 0.5); 

   if(FIRST_FID) 
     make_shapes(shname1, ref_pw90, ref_pwr, fla);
   
   tau1 = d2;
   if((f1180[A] == 'y') && (ni > 1.0))
   { 
     tau1 += ( 1.0 / (2.0*sw1) ); 
     if(tau1 < 0.2e-6) 
       tau1 = 0.0; 
   }
  
   if (f1180[0] == 'y')  
     tau1 = tau1-pwN*4.0/3.0;

   if(ix == 1) d2_init = d2;

   dbl(ct,v1);  /* v1 = 0202 */
   t1_counter = (int) ((d2-d2_init)*sw1 + 0.5);
   if(t1_counter % 2)
     add(two,v1,v1); 
   assign(v1,oph);
   if (phase == 2) 
     incr(v1);   

   status(A);

   dec2power(pwNlvl);

   zgradpulse(gzlvl2, gt2);
     delay(gstab);
     delay(d1-gt2);

   pbox_pulse(&sh1,zero,2.0e-4,2.0e-6);
           zgradpulse(gzlvl1, gt1);
           delay(gstab);
     
  if (ni == 0)
  {
     delay(taunh-gt1-gstab-WFG_START_DELAY+pwN*4.0-adjust);
     pbox_pulse(&sh2,zero,2.0e-6,2.0e-6);
     dec2rgpulse(pwN,v1,0.0,0.0);
     dec2rgpulse(pwN*2.0,zero,0.0,0.0);
     dec2rgpulse(pwN,zero,0.0,0.0);
  }
  else
  {
    delay(taunh-gt1-gstab-adjust);
    obspower(sh2dec.pwr);
    compo_pulse(pwN,v1,tau1);
  }

  zgradpulse(gzlvl1, gt1);
  delay(taunh*0.5-gt1-pwNa);

  if(ab_flg[A] == 'a' || ab_flg[A] == 'b')
  {
    dec2rgpulse(pwNa,zero,0.0,0.0); 
    if (ab_flg[0] == 'b')      /*INVERSION OR FLIP BACK*/
      dec2rgpulse(pwNa,two,0.0,0.0);
    else
      dec2rgpulse(pwNa,zero,0.0,0.0);
  }

  delay(taunh*0.5-pwNa-POWER_DELAY); 
  dec2power(dpwr2);
  
/*  obsoffset(tof+3.3*sfrq);  */

	status(C);
}


/*************************************************************/
/* pulse sandwich including incremented time period          */
/* flanked by 15N pulses and 1H and 13C shaped refocusing    */
/* pulses                                                    */
/*************************************************************/
void compo_pulse(n15pw,n15phase,inc_delay)
double n15pw,inc_delay;
codeint n15phase;
{
  double h1pw = sh2dec.pw;
  
  if ((inc_delay) > h1pw)
  {
     dec2rgpulse(n15pw,n15phase,0.0,0.0);
     delay((inc_delay-h1pw)*0.5);

    xmtrphase(zero);
    obsunblank();
    pbox_xmtron(&sh2dec);
    delay(h1pw);
    pbox_xmtroff();
    obsblank();

     delay((inc_delay-h1pw)*0.5);
     dec2rgpulse(n15pw,zero,0.0,0.0);
  }
  else
  {
    xmtrphase(zero);
    obsunblank();
    pbox_xmtron(&sh2dec);
    delay((h1pw-inc_delay-n15pw*2.0)*0.5);
     dec2rgpulse(n15pw,n15phase,0.0,0.0);
    delay(inc_delay);
     dec2rgpulse(n15pw,zero,0.0,0.0);
    delay((h1pw-inc_delay-n15pw*2.0)*0.5);
    pbox_xmtroff();
    obsblank();
  }

}


void make_shapes(shname1, rf90, rpwr, fla)
char *shname1;
double rf90, rpwr, fla;
{
  char wvname[MAXSTR];
  double  ppm = getval("sfrq"),
          ofs = 8.0*ppm, 
          bw = 4.0*ppm, 
          rftof = 4.77*ppm;
          
  if((shname1[0]=='v')&&(shname1[1]=='a')&&(shname1[2]=='p'))
  {
    sprintf(wvname, "vap%.0f", fla);
    sh1 = pbox_shape("hn_vap", wvname, bw, ofs-rftof, rf90, rpwr);
  }
  else
  {
    opx("hn_pc9f");
    putwave("pc9f", bw, ofs-rftof, 0.0, 0.0, fla);
    cpx(rf90, rpwr);
    sh1 = getRsh("hn_pc9f");
  }
  sh2 = pbox_shape("hn_refoc", "rsnob", bw, ofs-rftof, rf90, rpwr);
  opx("hn_refoc.DEC");
  putwave("rsnob", bw, ofs-rftof, 0.5, 0.0, 0.0);
  cpx(rf90, rpwr);
  sh2dec = getDsh("hn_refoc");
}
