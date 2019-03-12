/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* cleanHMBC.c "H1-C13 HMBC with suppression of strong coupling induced artefacts"
"MBOB LPJF type suppression"

P. Würtz, P. Permi, N.C. Nielsen, and O.W. Sørensen:
Clean-HMBC: Suppression of strong-coupling induced artifacts in HMBC spectra
Journal of Magnetic Resonance (2008)

Written by Peter Würtz, University of Helsinki, 2007-2008.
Adapted from multiplicity edited HMBC by A.J. Benie, CRC.

Echo/Antiecho gradient selection
uses double low-pass J-filtering for suppression of one-bond correlations
(default is 1st+2nd order filter suppression)

Modified for BioPack, October 2008, GG Varian Palo Alto

*/


/*
SETUP INFORMATION
Using decoupler channel 1 for 13C-pulses
Flags:
lpjf   		Order of initial low pass J-filter (LPJF)
	1: 	1st order LPJF (default)
	2:	2nd order LPJF
	3:	3rd order LPJF 
	4:	4th order LPJF
mboblpjf   	Order of terminal low pass J-filter 
	1: 	1st order LPJF 
	2:	2nd order LPJF (default)
	3:	3rd order LPJF 


satmode	y:	presaturation during relaxation delay
	n:	no presaturation during relaxation delay
MRC	y:	experiment schemes as in Nyberg, N.T. & Sørensen O.W.,
		 Magn. Reson. Chem. 2006
	n:	experiment schemes as in Benie, A.J. & Sørensen O.W.,
		 Magn. Reson. Chem. 2006

	

Pulses, Powers & Frequencies:
pw	90 degrees transmitter
pwC	90 degrees decoupler 1
pwlvl	90 degrees transmitter hard pulse power level
pwClvl	power level for decoupler 1 high power
tpwr    transmitter high power level
satpwr	power level for saturation of solvent signal using transmitter 
	during relaxation delay 
satdly	presaturation delay duration (use d1 for total relaxation delay )
satfrq	presaturation offset

Coupling constants
Jmin	smallest 1J(13CH) for LPJF setting
Jmax	largest 1J(13CH) for LPJF setting
jnch 	Long-range coupling used to determing delay for the evolution of long range coupling constants

Gradient settings
gt1 	length of the gradients used ca. 1msec
gzlvl1  max size of the low pass filter gradients.
gzlvl2  gradient for Echo/AntiEcho selection


Processing:
wft2da 
with f1coef='1 0 -1 0 0 1 0 1'

If F1 axis is upside down change the sign of last four f1coefs.
*/

#include <standard.h>

/* Subroutines here */
void LPJF(pass,jmin,jmax,het90,rofa,rofb,zgradlen,zgradlvl,zgradrec,tauadjust)
int pass;
double jmin, jmax, tauadjust,
 het90, rofa, rofb,
 zgradlen, zgradlvl, zgradrec; 
{
/* subroutine to provide various low pass filters. 
   The default is a third order low pass filter

   Parameters:
   pass: flag for the low pass filter order
   jmin,jmax: the value of the minimum and maximum 1JCH constant
   het90,rofa, rofb: the length of the 13C nucleus 90deg pulse.
   rofa/b correspond to rof1/2
   zgradlen/lvl/rec: z gradient length, level and the recovery delay
   tauadjust: the ammount by which to shorten the inital tau delay to
   compensate for timing differences
 */

 double tau, taumin,taumax, taumin2, taumax2;

 if (jmin == 0.0 || jmax == 0.0)
  {
  jmin = 140;
  jmax = 175;
  }

 switch (pass)
  {

  case 0:
  delay(zgradrec);
  break;


  case 1:
  taumax = 1.0/(jmin + jmax);
  delay(taumax - zgradlen - 2*GRADIENT_DELAY - zgradrec - tauadjust);
  zgradpulse(zgradlvl,zgradlen);
  delay(zgradrec);
  decrgpulse(het90,zero,zgradlen,rofb);
  zgradpulse(zgradlvl*-1,zgradlen);
  break;

  case 2:
  zgradlvl=zgradlvl/3.0;
  taumin = 1.0/(2.0*(jmin + 0.146*(jmax - jmin)));
  taumax = 1.0/(2.0*(jmax - 0.146*(jmax - jmin)));
  delay(taumin - zgradlen - 2*GRADIENT_DELAY - zgradrec - tauadjust);
  zgradpulse(zgradlvl*3.0,zgradlen);
  decrgpulse(het90,zero,zgradrec,rofb);
  zgradpulse(zgradlvl*-2.0,zgradlen);
  delay(taumax - zgradlen - 2*GRADIENT_DELAY - rofa - rofb - het90);
  decrgpulse(het90,zero,rofa,rofb);
  zgradpulse(zgradlvl*-1.0,zgradlen);
  break;

  default: /* low pass 3rd order filter DEFAULT */ 
  zgradlvl=zgradlvl/7;
  taumin = 1.0/(2.0*(jmin + 0.070*(jmax - jmin)));
  tau = 1.0/(jmin + jmax); 
  taumax = 1.0/(2.0*(jmax - 0.070*(jmax - jmin)));
  delay(taumin - zgradlen - 2*GRADIENT_DELAY - zgradrec - tauadjust);
  zgradpulse(zgradlvl*7.0,zgradlen);
  decrgpulse(het90,zero,zgradrec,rofb);
  zgradpulse(zgradlvl*-4.0,zgradlen);
  delay(tau - zgradlen - 2*GRADIENT_DELAY - rofb - rofa - het90);
  decrgpulse(het90,zero,rofa,rofb);
  zgradpulse(zgradlvl*-2.0,zgradlen);
  delay(taumax - zgradlen - 2*GRADIENT_DELAY - rofb - rofa - het90);
  decrgpulse(het90,zero,rofa,rofb);
  zgradpulse(zgradlvl*-1.0,zgradlen);
  break;

  case 4:     /* low pass 4th order filter */ 
  zgradlvl=zgradlvl/15;
  taumin = 1.0/(2.0*(jmin + 0.040*(jmax - jmin)));
  taumin2 = 1.0/(2.0*(jmin + 0.310*(jmax - jmin)));
  taumax2 =  1.0/(2.0*(jmax - 0.310*(jmax - jmin)));
  taumax = 1.0/(2.0*(jmax - 0.040*(jmax - jmin)));
  delay(taumin - zgradlen - 2*GRADIENT_DELAY - zgradrec - tauadjust);
  zgradpulse(zgradlvl*15.0,zgradlen);
  decrgpulse(het90,zero,zgradrec,rofb);
  zgradpulse(zgradlvl*-8.0,zgradlen);
  delay(taumin2 - zgradlen - 2*GRADIENT_DELAY - rofb - rofa - het90);
  decrgpulse(het90,zero,zgradrec,rofb);
  zgradpulse(zgradlvl*-4.0,zgradlen);
  delay(taumax2 - zgradlen - 2*GRADIENT_DELAY - rofb - rofa - het90);
  decrgpulse(het90,zero,rofa,rofb);
  zgradpulse(zgradlvl*-2.0,zgradlen);
  delay(taumax - zgradlen - 2*GRADIENT_DELAY - rofb - rofa - het90);
  decrgpulse(het90,zero,rofa,rofb);
  zgradpulse(zgradlvl*-1.0,zgradlen);
  break;

  case 9: /* low pass 2nd order filter with 3rd order delays for initial and terminal filter combinations */ 
  zgradlvl=zgradlvl/3.0;
  taumin = 1.0/(2.0*(jmin + 0.070*(jmax - jmin)));
  taumax = 1.0/(2.0*(jmax - 0.070*(jmax - jmin)));
  delay(taumin - zgradlen - 2*GRADIENT_DELAY - zgradrec - tauadjust);
  zgradpulse(zgradlvl*3.0,zgradlen);
  decrgpulse(het90,zero,zgradrec,rofb);
  zgradpulse(zgradlvl*-2.0,zgradlen);
  delay(taumax - zgradlen - 2*GRADIENT_DELAY - rofa - rofb - het90);
  decrgpulse(het90,zero,rofa,rofb);
  zgradpulse(zgradlvl*-1.0,zgradlen);
  break;


  }
 } /* end of LPJF subroutine */

void MBOB_LPJF(pass,jmin,jmax,het90,rofa,rofb,zgradrec)
int pass;
double jmin, jmax, het90, rofa, rofb, zgradrec; 
{
/* subroutine to provide various low pass filters in MBOB manner
   The default is a third order MBOB low pass filter

   Parameters:
   pass: flag for the low pass filter order
   jmin,jmax: the value of the minimum and maximum 1JCH constant
   het90,rofa, rofb: the length of the 13C nucleus 90deg pulse.
   tauF: tau delay array for MBOB LPJF
   compensate for timing differences
 */

 double tau, taumin,taumax, tauTotal;
 double tauF_1, tauF_2, tauF_3, tauF_4, tauF_5, tauF_6, tauF_7, tauF_8;


 if (jmin == 0.0 || jmax == 0.0)
  {
  jmin = 140;
  jmax = 175;
  }

 switch (pass)
  {
   
  case 0:
  decrgpulse(het90,v1,zgradrec,0.0);
  break;


  case 1:
  tau = 1.0/(jmin + jmax);
  tauTotal = tau + 4.0*het90;  
  tauF_1 = 0.0;
  tauF_2 = tau;

  decrgpulse(het90,v1,zgradrec,0.0);
  ifzero(v4);
    delay(tauF_1);
    decrgpulse(het90,zero,rofa,0.0);
    decrgpulse(2.0*het90,one,0.0,0.0);
    decrgpulse(het90,zero,0.0,0.0);
    delay(tauTotal - 4.0*het90 - tauF_1);
  elsenz(v4);
    delay(tauF_2);
    decrgpulse(het90,zero,rofa,0.0);
    decrgpulse(2.0*het90,one,0.0,0.0);
    decrgpulse(het90,zero,0.0,0.0);
    delay(tauTotal - 4.0*het90 - tauF_2);
  endif(v4);

  break;

  case 2:
  taumin = 1.0/(2.0*(jmin + 0.146*(jmax - jmin)));
  taumax = 1.0/(2.0*(jmax - 0.146*(jmax - jmin)));
  tauTotal = taumin + taumax + 4.0*het90;  
  tauF_1 = 0.0;
  tauF_2 = taumin;
  tauF_3 = taumax;
  tauF_4 = taumin + taumax;

  decrgpulse(het90,v1,zgradrec,0.0);

  ifzero(v5);
    delay(tauF_1);
    decrgpulse(het90,zero,rofa,0.0);
    decrgpulse(2.0*het90,one,0.0,0.0);
    decrgpulse(het90,zero,0.0,0.0);
    delay(tauTotal - 4.0*het90 - tauF_1);
  elsenz(v5);
    decr(v5);
  ifzero(v5);
    delay(tauF_2);
    decrgpulse(het90,zero,rofa,0.0);
    decrgpulse(2.0*het90,one,0.0,0.0);
    decrgpulse(het90,zero,0.0,0.0);
    delay(tauTotal - 4.0*het90 - tauF_2);
  elsenz(v5);
    decr(v5);
  ifzero(v5);
    delay(tauF_3);
    decrgpulse(het90,zero,rofa,0.0);
    decrgpulse(2.0*het90,one,0.0,0.0);
    decrgpulse(het90,zero,0.0,0.0);
    delay(tauTotal - 4.0*het90 - tauF_3);
  elsenz(v5);
    delay(tauF_4);
    decrgpulse(het90,zero,rofa,0.0);
    decrgpulse(2.0*het90,one,0.0,0.0);
    decrgpulse(het90,zero,0.0,0.0);
    delay(tauTotal - 4.0*het90 - tauF_4);
  endif(v5);
  endif(v5);
  endif(v5); 
  break;  

  case 9:
  taumin = 1.0/(2.0*(jmin + 0.070*(jmax - jmin)));
  taumax = 1.0/(2.0*(jmax - 0.070*(jmax - jmin)));
  tauTotal = taumin + taumax + 4.0*het90;  
  tauF_1 = 0.0;
  tauF_2 = taumin;
  tauF_3 = taumax;
  tauF_4 = taumin + taumax;

  decrgpulse(het90,v1,zgradrec,0.0);

  ifzero(v5);
    delay(tauF_1);
    decrgpulse(het90,zero,rofa,0.0);
    decrgpulse(2.0*het90,one,0.0,0.0);
    decrgpulse(het90,zero,0.0,0.0);
    delay(tauTotal - 4.0*het90 - tauF_1);
  elsenz(v5);
    decr(v5);
  ifzero(v5);
    delay(tauF_2);
    decrgpulse(het90,zero,rofa,0.0);
    decrgpulse(2.0*het90,one,0.0,0.0);
    decrgpulse(het90,zero,0.0,0.0);
    delay(tauTotal - 4.0*het90 - tauF_2);
  elsenz(v5);
    decr(v5);
  ifzero(v5);
    delay(tauF_3);
    decrgpulse(het90,zero,rofa,0.0);
    decrgpulse(2.0*het90,one,0.0,0.0);
    decrgpulse(het90,zero,0.0,0.0);
    delay(tauTotal - 4.0*het90 - tauF_3);
  elsenz(v5);
    delay(tauF_4);
    decrgpulse(het90,zero,rofa,0.0);
    decrgpulse(2.0*het90,one,0.0,0.0);
    decrgpulse(het90,zero,0.0,0.0);
    delay(tauTotal - 4.0*het90 - tauF_4);
  endif(v5);
  endif(v5);
  endif(v5); 
  break;  


  default: /* mbob low pass 3rd order filter DEFAULT  */
  taumin = 1.0/(2.0*(jmin + 0.070*(jmax - jmin)));
  tau = 1.0/(jmin + jmax); 
  taumax = 1.0/(2.0*(jmax - 0.070*(jmax - jmin)));
  tauTotal = taumin +tau + taumax + 4.0*het90;  
  tauF_1 = 0.0;
  tauF_2 = taumin;
  tauF_3 = tau;
  tauF_4 = taumax;
  tauF_5 = taumin+tau;
  tauF_6 = taumin+taumax;
  tauF_7 = tau+taumax;
  tauF_8 = taumin+tau+taumax;

  decrgpulse(het90,v1,zgradrec,0.0);
  ifzero(v7);
    delay(tauF_1);
    decrgpulse(het90,zero,rofa,0.0);
    decrgpulse(2.0*het90,one,0.0,0.0);
    decrgpulse(het90,zero,0.0,0.0);
    delay(tauTotal - 4.0*het90 - tauF_1);
  elsenz(v7);
    decr(v7);
  ifzero(v7);
    delay(tauF_2);
    decrgpulse(het90,zero,rofa,0.0);
    decrgpulse(2.0*het90,one,0.0,0.0);
    decrgpulse(het90,zero,0.0,0.0);
    delay(tauTotal - 4.0*het90 - tauF_2);
  elsenz(v7);
    decr(v7);
  ifzero(v7);
    delay(tauF_3);
    decrgpulse(het90,zero,rofa,0.0);
    decrgpulse(2.0*het90,one,0.0,0.0);
    decrgpulse(het90,zero,0.0,0.0);
    delay(tauTotal - 4.0*het90 - tauF_3);
  elsenz(v7);
    decr(v7);
  ifzero(v7);
    delay(tauF_4);
    decrgpulse(het90,zero,rofa,0.0);
    decrgpulse(2.0*het90,one,0.0,0.0);
    decrgpulse(het90,zero,0.0,0.0);
    delay(tauTotal - 4.0*het90 - tauF_4);
  elsenz(v7);
    decr(v7);
  ifzero(v7);
    delay(tauF_5);
    decrgpulse(het90,zero,rofa,0.0);
    decrgpulse(2.0*het90,one,0.0,0.0);
    decrgpulse(het90,zero,0.0,0.0);
    delay(tauTotal - 4.0*het90 - tauF_5);
  elsenz(v7);
    decr(v7);
  ifzero(v7);
    delay(tauF_6);
    decrgpulse(het90,zero,rofa,0.0);
    decrgpulse(2.0*het90,one,0.0,0.0);
    decrgpulse(het90,zero,0.0,0.0);
    delay(tauTotal - 4.0*het90 - tauF_6);
  elsenz(v7);
    decr(v7);
  ifzero(v7);
    delay(tauF_7);
    decrgpulse(het90,zero,rofa,0.0);
    decrgpulse(2.0*het90,one,0.0,0.0);
    decrgpulse(het90,zero,0.0,0.0);
    delay(tauTotal - 4.0*het90 - tauF_7);
  elsenz(v7);
    delay(tauF_8);
    decrgpulse(het90,zero,rofa,0.0);
    decrgpulse(2.0*het90,one,0.0,0.0);
    decrgpulse(het90,zero,0.0,0.0);
    delay(tauTotal - 4.0*het90 - tauF_8);
  endif(v7);
  endif(v7);
  endif(v7); 
  endif(v7);
  endif(v7);
  endif(v7); 
  endif(v7);
  break;  


  }
 } /* end of MBOB_LPJF subroutine */
 
/* End of subroutines */

/* The pulsesequence starts here */

void pulsesequence()
{

 /* Variables */
 double
  decstopd, jnch=getval("jnch"), /* present in gChsqc parameter set */
  Epsilon, EpsilonpwC, gradlen,
  /* taumethyl = 1.0/(2.0*126), */
  gstab=getval("gstab"),
  gt1=getval("gt1"),
  gzlvl1=getval("gzlvl1"),
  gzlvl2=getval("gzlvl2"),
  Jmax=getval("Jmax"),
  Jmin=getval("Jmin"),
  pwC=getval("pwC"),
  pwClvl=getval("pwClvl"),
  tCH=getval("tCH"),  /*present in gChsqc parameter set*/
  t1evol, t1min, tau,
  tpwr=getval("tpwr"),
  tauSC, 
  
  gH=26.752196, /* gyromagnetic ratio of the detected nucleus */
  g13C=6.72828; /* gyromagnetic ratio for the indirectly detected nucleus
                 change as needed e.g. for N15 use 2.712621 (ignore the sign) */

 double grad[5]={0.0,0.0,0.0,0.0,0.0};

 char altgrad[MAXSTR],MRC[MAXSTR];

 int
     lpjf=(int)(getval("lpjf") + 0.5),	
     mboblpjf=(int)(getval("mboblpjf") + 0.5),	
     phase=(int)(getval("phase") + 0.5);

 /* get character strings */
 getstr("altgrad",altgrad);
 getstr("MRC",MRC);

 /* Initialise Variables */
 decstopd=POWER_DELAY + PRG_STOP_DELAY;
 gradlen=gt1 + 2*GRADIENT_DELAY;
 tauSC=1/(Jmin+Jmax);
 tau=tCH*2;   /*tCH is 1/4J for gChsqc*/

 /* make sure that for the first increment d2 is zero
    (except for in setup experiments) */
 if((ix==0)&&(ni>1)) d2=0.0;

 /* check that rof1 is not too long and set up t1 */
/* if (rof1>2.0e-6) rof1=2.0e-6; */
 t1evol=2*rof1 + 1.0e-6;
 t1evol=0.5*(d2 + t1evol);
 t1min=2*rof1 + 1.0e-6;
 Epsilon=t1min+2*pw;
 EpsilonpwC=Epsilon+2*pwC;
 
 /* Start of Real time maths do not mix with 'normal' maths! */
 /* Phase Cycle calculations
    sets up v14 to be used instead of ct so that steady state scans cycle
    sets up the counter v11 for the up/down spectrum selection */
 sub(ct,ssctr,v14);
 mod2(v14,v11);

 /* set array-counter for MBOB LPJF supression */
 mod2(ct,v4);  /* ct mod2 for 1st order mbob-lpjf */
 mod4(ct,v5);  /* ct mod4 for 2nd order mbob-lpjf */
 dbl(two,v8);
 dbl(v8,v8);   /* v8 = 8 */
 modn(ct,v8,v7);  /* ct mod8 for 3rd order mbob-lpjf */

 /*the rest of the phase cycled phases*/
 dbl(v14,oph);
 hlv(v14,v2);
 hlv(v2,v3);
 dbl(v2,v2);
 dbl(v3,v13);
 add(v2,v13,v2);
 add(v14,one,v1);
 hlv(v1,v1);
 dbl(v1,v1);
 /* Translation of the above
 v1=0 2 2 0
 v2=0 0 2 2 2 2 0 0
 v3={0}4 {1}4 {2}4 {3}4
 oph=0 2
 v13&v14 are only used to calculate phases 1 to 4 and oph
 v11 is used for selecting up/down spectra it is just a series of the form 0 1..
 */
 


 if(MRC[A]=='y') assign(zero,v11);

/* Strong coupling filtering phase cycle */
if (mboblpjf==1){

 hlv(v14,v1);
 add(v1,one,v1);
 hlv(v1,v1);
 dbl(v1,v1);


 hlv(v14,v13);
 hlv(v13,v13);
 hlv(v13,v2);
 dbl(v13,v13);
 dbl(v2,v2);
 add(v2,v13,v2);

 
 hlv(v3,v3);

 hlv(v14,oph);
 dbl(oph,oph);
 }
if ((mboblpjf==2)||(mboblpjf==9)){
 hlv(v14,v1);
 hlv(v1,v1);
 add(v1,one,v1);
 hlv(v1,v1);
 dbl(v1,v1);

 hlv(v14,v13);
 hlv(v13,v13);
 hlv(v13,v13);
 hlv(v13,v2);
 dbl(v13,v13);
 dbl(v2,v2);
 add(v2,v13,v2);

 hlv(v3,v3);
 hlv(v3,v3);

 hlv(v14,oph);
 hlv(oph,oph);
 dbl(oph,oph);
 }
if (mboblpjf==3){
 hlv(v14,v1);
 hlv(v1,v1);
 hlv(v1,v1);
 add(v1,one,v1);
 hlv(v1,v1);
 dbl(v1,v1);

 hlv(v14,v13);
 hlv(v13,v13);
 hlv(v13,v13);
 hlv(v13,v13);
 hlv(v13,v2);
 dbl(v13,v13);
 dbl(v2,v2);
 add(v2,v13,v2);

 hlv(v3,v3);
 hlv(v3,v3);
 hlv(v3,v3);

 hlv(v14,oph);
 hlv(oph,oph);
 hlv(oph,oph);
 dbl(oph,oph);
 }

 /* Calculate modifications to phases for States-TPPI acquisition
    id2 is a real time variable that contains the increment number */
 mod2(id2,v12);
 dbl(v12,v12);
 add(v2,v12,v2);
 add(oph,v12,oph);


 /* Gradient setup
   define a standard basic gradient strength */
 if(altgrad[A]=='y') gzlvl2=gzlvl2/((gH+g13C)-((gH*g13C)/(gH-g13C)));
 else gzlvl2=gzlvl2/(gH+g13C);
 /* define the actual strengths and perform the necessary adjustments for
    Echo/AntiEcho the adjustment for the up down spectra is controlled by
    real time if statements */
 if(altgrad[A]=='n')
  {
  grad[2]=(gH+g13C)*gzlvl2;
  grad[3]=(gH-g13C)*gzlvl2*-1;
  if(phase==2)
   {
   grad[2]=(gH-g13C)*gzlvl2*-1;
   grad[3]=(gH+g13C)*gzlvl2;
   }
  }
 else if(altgrad[A]=='y')
  {
  grad[0]=g13C*gzlvl2*-1;
  grad[2]=((gH+g13C)-((gH*g13C)/(gH-g13C)))*gzlvl2;
  grad[3]=(gH-g13C)*gzlvl2*-1;
  if(phase==2)
   {
   grad[2]=(gH-g13C)*gzlvl2*-1;
   grad[3]=((gH+g13C)-((gH*g13C)/(gH-g13C)))*gzlvl2;
   }
  }


  if((altgrad[A]!='y')&&(altgrad[A]!='n'))
  {
   printf("Error altgrad must be set to either 'y' or 'n' \n");
   psg_abort(1);
  }


 /* pulsesequence starts here with pulses */
 
status(A); 

 if(satmode[A]=='y')
  {
  if((satdly>d1)||(satpwr>30)||(satdly>2.0))
   {
   if(satdly>d1)
    printf("Presaturation time, satdly (%f) must be shorter than d1 (%f) \n",
     satdly,d1);
   if(satpwr>30)
    printf("Presaturation power, satpwr is too high with %f (max = 30 dB) \n",
     satpwr);
   if(satdly>2.0)
    printf("Presaturation delay, satdly is too long with %f (max = 2 s) \n",
     satdly);
   psg_abort(1);
   }
  else
   {
   delay(d1-satdly); /* if doing presat shorten d1 by the presat ammount */
   obsoffset(satfrq);
   delay(4e-6);
   obspower(satpwr);
   rgpulse(satdly - 2.0*POWER_DELAY - rof1 - 8e-6,zero,rof1,0.0);
   obspower(tpwr);
   obsoffset(tof);
   delay(4e-6);
   }
  }
 else
  {
  delay(d1);
  }
  
 status(B);
 rgpulse(pw,zero,rof1,0.0);
 decpower(pwClvl);

 LPJF(lpjf,Jmin,Jmax,pwC,rof1,0.0,gt1,gzlvl1,gstab,decstopd);
 delay(1/(2.0*jnch) - gradlen*2 - gstab);
 if(altgrad[A]=='y') zgradpulse(grad[0],gt1);
  else delay(gradlen);
 decrgpulse(pwC,v2,gstab,0.0);

 delay(t1evol - rof1);
 rgpulse(pw*2,zero,rof1,0.0);
 delay(t1evol - rof1);
 delay(rof1);
 if(altgrad[A]=='y') zgradpulse(grad[2],gt1); 
 delay(tau*0.5 - gradlen - gstab);

 if(altgrad[A]=='n') zgradpulse(grad[2],gt1);
  
 decrgpulse(pwC*2,v3,gstab,0.0);
 zgradpulse(grad[3],gt1);
 delay(tau*0.5 - gradlen - gstab + Epsilon);

 MBOB_LPJF(mboblpjf,Jmin,Jmax,pwC,rof1,0.0,gstab); 





 
  	
 status(C); /* acquistion */

} /* end of pulseprogram */
