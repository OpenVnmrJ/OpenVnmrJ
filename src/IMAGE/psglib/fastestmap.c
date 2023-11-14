/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#ifndef LINT
#endif
/* 
 */
/*----------------------------------------------------------------------------+
|                                                                             |
|    FASTMAP:FAST AUTOMATIC SHIMMING TECHNIQUE BY MAPPING ALONG PROJECTIONS   |
|                                                                             |
|    MODIFICATIONS                                                            |
|    11/27/94 - modified sequence for mapping along projections               |
|    Modified for 4T - RG 07/01/98                                            |
|    Modified for a new 4T - IT 12/08/98                                      |
|    Modified for ultra fast shimming  IT 06/25/99                            |
|    Last modification - IT 06/25/99                                          |
+----------------------------------------------------------------------------*/


/**[1] C PREPROCESSOR MATERIAL INCLUDE FILES******************************/
#include "standard.h"

#define GAMMA   4.257713e3	/* 1/(Gauss*s) */
#define Grad_DAC  32767		/* Maximum gradient in DAC units */
#define s45 0.70711
#define c45 0.70711
#define s315 -0.70711
#define c315 0.70711



/**[2] PULSE SEQUENCE CODE**********************************************/

void pulsesequence()
{

  /**[2.1] DECLARATIONS**************************************************/

  /*GENERAL PARAMETERS */
  double sequence_error, gmax, trise, lro, thk, X0, Y0, Z0, invpw2bw=0.0;
  int maxproj, nproj, alt, off, j;
  char print_grad[MAXSTR], print_freq[MAXSTR], print_index[MAXSTR];

  /*GRADIENTS */
  double gread, grr, gsl, gsr, grew, gspoil, u2g, gsp;
  double grise_halfsine, grise_rampup, grise_triangle;
  double kread, kslice, krew, krew_epi;
//  double vtheta, vpsi, vphi;

  double gay0[6], gay1[6], gay2[6], gay3[6], gay4[6], gay5[6], gax6[6];
  double gax0[6], gax1[6], gax2[6], gax3[6], gax4[6], gax5[6], gay6[6];
  double gaz0[6], gaz1[6], gaz2[6], gaz3[6], gaz4[6], gaz5[6], gaz6[6];

  /*RF PULSES AND DELAYS */
  double te1, te2, predelay, tau, tramp, tror, tssr, trew, tgsettle, tspoil,
    tsp;
  double DA, DB, DC, DD, invpw, invpw1, invpw2;
  double te1min, te12min, te2min, taumin;
  char pipat1[MAXSTR], pipat2[MAXSTR], epi[MAXSTR], triang[MAXSTR];

  /*FREQUENCIES */
  double toxr, toyr, tozr, toxs, toys, tozs;
  double to1[6], to2[6], to3[6], tox0s, toy0s, toz0s, tox0r, toy0r, toz0r;
  double tof;


  /**[2.2] PARAMETER READ IN FROM EXPERIMENT****************************/
//  vtheta=0.0;
//  vphi=0.0;
//  vpsi=0.0;
  gmax = getval("gmax");
  u2g=gmax/Grad_DAC; 
  tof = getval("resto");
  at = getval("at");
  maxproj = getval("maxproj");
  off = getval("off");
  te1 = getval("te1");
  te2 = getval("te2");
  tr = getval("tr");
  tau = getval("tau");
  tramp = getval("tramp");
  tror = getval("tror");
  tssr = getval("tssr");
  trew = getval("trew");
  tgsettle = getval("tgsettle");
  tspoil = getval("tspoil");
  tsp = getval("tsp");
  gsp = getval("gsp")*u2g;
  invpw = getval("invpw");
  gspoil = getval("gspoil")*u2g;
  kread = getval("kread");
  kslice = getval("kslice");
  krew = getval("krew");
  krew_epi = getval("krew_epi");
  lro = getval("lro");
  thk = getval("thk");
  X0 = getval("X0");
  Y0 = getval("Y0");
  Z0 = getval("Z0");
  trise = getval("trise");

  getstr("pwpat", pwpat);
  getstr("pipat1", pipat1);
  getstr("pipat2", pipat2);
  getstr("epi", epi);
  getstr("triang", triang);
  getstr("print_grad", print_grad);
  getstr("print_freq", print_freq);
  getstr("print_index", print_index);


  /**[2.3] CALCULATIONS***********************************************/

        if ((strstr(pipat1,"fm_invpat_2") != NULL) && (strstr(pipat2,"fm_invpat2_2") != NULL) )
        {
          invpw2bw = 4.0;
        }
        if ((strstr(pipat1,"fm_invpat_10") != NULL) && (strstr(pipat2,"fm_invpat2_10") != NULL))
        {
          invpw2bw = 10.0;
        }


  /*SELECTION OF DATA COLLECTION MODE */
  if ((epi[0] == 'y') || (epi[0] == 'Y'))
  {
    alt = 1;
    nproj = maxproj;
  }
  else
  {
    nf = 1;
    alt = 2;
    nproj = 2.0 * maxproj;
  }

  /*RF PULSES */
  invpw1 = invpw;
  invpw2 = 2.0 * invpw;

  /*GRADIENTS FOR READOUT AND SLICE SELECTION */
  thk = thk / 10.0;
  gread = sw / (GAMMA * lro);
  gsl = invpw2bw / (invpw * thk * GAMMA);

  /*FREQUENCIES FOR SLICE SELECTION AND DATA COLLECTION */
  toxr = tof + GAMMA * gread * X0;
  toyr = tof + GAMMA * gread * Y0;
  tozr = tof + GAMMA * gread * Z0;
  toxs = tof + GAMMA * gsl * X0;
  toys = tof + GAMMA * gsl * Y0;
  tozs = tof + GAMMA * gsl * Z0;

  /*REPHASING GRADIENTS */
  grr = -kread*gread * (tramp/2.0 + tgsettle + at/2.0) / (2.0*tror/3.14159265358979323846);
  gsr = kslice*gsl * (tramp + 2.0*tgsettle) / (2.0*tssr/3.14159265358979323846);
  grew = -krew_epi * 2.0*gread*(tramp + tgsettle + at)/trew;

  /*PRINTOUT OF CALCULATED PARAMETERS */
  if (print_grad[0] == 'y')
  {
    if (ix == 1)
    {
      fprintf(stderr, "\n\n*******************************************\n");
      fprintf(stderr, "CALCULATED GRADIENTS FOR FASTESTMAP:");
      fprintf(stderr, "\ngread=%+8.1f  gsl=%+8.1f", gread, gsl);
      fprintf(stderr, "\ngrr  =%+8.1f  gsr=%+8.1f  grew=%+8.1f\n",
		      grr, gsr, grew);
    }
  }

  if (print_freq[0] == 'y')
  {
    if (ix == 1)
    {
      fprintf(stderr, "\nCALCULATED FRQUENCIES FOR FASTESTMAP:");
      fprintf(stderr, "\ntoxr=%+8.1f  toyr=%+8.1f  tozr=%+8.1f",
		      toxr, toyr, tozr);
      fprintf(stderr, "\ntoxs=%+8.1f  toys=%+8.1f  tozs=%+8.1f\n",
		      toxs, toys, tozs);
    }
  }


  /*PARAMETER TO CHECK THE RISERATE OF SHAPED GRADIENTS */
  /*grise_halfsine = Grad_DAC * Grad_DAC / (trise * 3212 * 32);*/
  grise_halfsine = gmax / (trise * 3.1);
  grise_rampup = gmax / trise;
  grise_triangle = gmax / (trise * 2);


  /*ASSIGNMENT OF FREQUENCIES AND GRADIENTS */
  if (maxproj <= 3)
  {
    to1[0] = toys;
    to2[0] = tozs;
    to3[0] = toxr;
    to1[1] = toxs;
    to2[1] = tozs;
    to3[1] = toyr;
    to1[2] = toxs;
    to2[2] = toys;
    to3[2] = tozr;

    gax0[0] = grr;
    gax1[0] = 0;
    gax2[0] = 0;
    gay0[0] = 0;
    gay1[0] = gsl;
    gay2[0] = 0;
    gaz0[0] = 0;
    gaz1[0] = 0;
    gaz2[0] = gsr;

    gax0[1] = 0;
    gax1[1] = gsl;
    gax2[1] = 0;
    gay0[1] = grr;
    gay1[1] = 0;
    gay2[1] = 0;
    gaz0[1] = 0;
    gaz1[1] = 0;
    gaz2[1] = gsr;

    gax0[2] = 0;
    gax1[2] = gsl;
    gax2[2] = 0;
    gay0[2] = 0;
    gay1[2] = 0;
    gay2[2] = gsr;
    gaz0[2] = grr;
    gaz1[2] = 0;
    gaz2[2] = 0;

    gax3[0] = 0;
    gax4[0] = gread;
    gax5[0] = grew;
    gax6[0] = 0;
    gay3[0] = 0;
    gay4[0] = 0;
    gay5[0] = 0;
    gay6[0] = gsp;
    gaz3[0] = gsl;
    gaz4[0] = 0;
    gaz5[0] = 0;
    gaz6[0] = gsp;

    gax3[1] = 0;
    gax4[1] = 0;
    gax5[1] = 0;
    gax6[1] = gsp;
    gay3[1] = 0;
    gay4[1] = gread;
    gay5[1] = grew;
    gay6[1] = 0;
    gaz3[1] = gsl;
    gaz4[1] = 0;
    gaz5[1] = 0;
    gaz6[1] = gsp;

    gax3[2] = 0;
    gax4[2] = 0;
    gax5[2] = 0;
    gax6[2] = gsp;
    gay3[2] = gsl;
    gay4[2] = 0;
    gay5[2] = 0;
    gay6[2] = gsp;
    gaz3[2] = 0;
    gaz4[2] = gread;
    gaz5[2] = grew;
    gaz6[2] = 0;
  }
  else
  {
    tox0s = toxs - tof;
    tox0r = toxr - tof;
    toy0s = toys - tof;
    toy0r = toyr - tof;
    toz0s = tozs - tof;
    toz0r = tozr - tof;

    to1[0] = tof + tox0s * c315 + toy0s * s315;
    to1[1] = tof + tox0s * c45 + toy0s * s45;
    to1[2] = tof + tox0s;
    to1[3] = tof + tox0s;
    to1[4] = tof + tox0s * s315 + toz0s * c315;
    to1[5] = tof + tox0s * s45 + toz0s * c45;

    to2[0] = tof + toz0s;
    to2[1] = tof + toz0s;
    to2[2] = tof + toy0s * s315 + toz0s * c315;
    to2[3] = tof + toy0s * s45 + toz0s * c45;
    to2[4] = tof + toy0s;
    to2[5] = tof + toy0s;

    to3[0] = tof + tox0r * c45 + toy0r * s45;
    to3[1] = tof + tox0r * c315 + toy0r * s315;
    to3[2] = tof + toy0r * s45 + toz0r * c45;
    to3[3] = tof + toy0r * s315 + toz0r * c315;
    to3[4] = tof + tox0r * s45 + toz0r * c45;
    to3[5] = tof + tox0r * s315 + toz0r * c315;

    gax0[0] = grr * c45;
    gax1[0] = gsl * c315;
    gax2[0] = 0;
    gay0[0] = grr * s45;
    gay1[0] = gsl * s315;
    gay2[0] = 0;
    gaz0[0] = 0;
    gaz1[0] = 0;
    gaz2[0] = gsr;

    gax0[1] = grr * c315;
    gax1[1] = gsl * c45;
    gax2[1] = 0;
    gay0[1] = grr * s315;
    gay1[1] = gsl * s45;
    gay2[1] = 0;
    gaz0[1] = 0;
    gaz1[1] = 0;
    gaz2[1] = gsr;

    gax0[2] = 0;
    gax1[2] = gsl;
    gax2[2] = 0;
    gay0[2] = grr * s45;
    gay1[2] = 0;
    gay2[2] = gsr * s315;
    gaz0[2] = grr * c45;
    gaz1[2] = 0;
    gaz2[2] = gsr * c315;

    gax0[3] = 0;
    gax1[3] = gsl;
    gax2[3] = 0;
    gay0[3] = grr * s315;
    gay1[3] = 0;
    gay2[3] = gsr * s45;
    gaz0[3] = grr * c315;
    gaz1[3] = 0;
    gaz2[3] = gsr * c45;

    gax0[4] = grr * s45;
    gax1[4] = gsl * s315;
    gax2[4] = 0;
    gay0[4] = 0;
    gay1[4] = 0;
    gay2[4] = gsr;
    gaz0[4] = grr * c45;
    gaz1[4] = gsl * c315;
    gaz2[4] = 0;

    gax0[5] = grr * s315;
    gax1[5] = gsl * s45;
    gax2[5] = 0;
    gay0[5] = 0;
    gay1[5] = 0;
    gay2[5] = gsr;
    gaz0[5] = grr * c315;
    gaz1[5] = gsl * c45;
    gaz2[5] = 0;

    gax3[0] = 0;
    gax4[0] = gread * c45;
    gax5[0] = grew * c45;
    gay3[0] = 0;
    gay4[0] = gread * s45;
    gay5[0] = grew * s45;
    gaz3[0] = gsl;
    gaz4[0] = 0;
    gaz5[0] = 0;

    gax3[1] = 0;
    gax4[1] = gread * c315;
    gax5[1] = grew * c315;
    gay3[1] = 0;
    gay4[1] = gread * s315;
    gay5[1] = grew * s315;
    gaz3[1] = gsl;
    gaz4[1] = 0;
    gaz5[1] = 0;

    gax3[2] = 0;
    gax4[2] = 0;
    gax5[2] = 0;
    gay3[2] = gsl * s315;
    gay4[2] = gread * s45;
    gay5[2] = grew * s45;
    gaz3[2] = gsl * c315;
    gaz4[2] = gread * c45;
    gaz5[2] = grew * c45;

    gax3[3] = 0;
    gax4[3] = 0;
    gax5[3] = 0;
    gay3[3] = gsl * s45;
    gay4[3] = gread * s315;
    gay5[3] = grew * s315;
    gaz3[3] = gsl * c45;
    gaz4[3] = gread * c315;
    gaz5[3] = grew * c315;

    gax3[4] = 0;
    gax4[4] = gread * s45;
    gax5[4] = grew * s45;
    gay3[4] = gsl;
    gay4[4] = 0;
    gay5[4] = 0;
    gaz3[4] = 0;
    gaz4[4] = gread * c45;
    gaz5[4] = grew * c45;

    gax3[5] = 0;
    gax4[5] = gread * s315;
    gax5[5] = grew * s315;
    gay3[5] = gsl;
    gay4[5] = 0;
    gay5[5] = 0;
    gaz3[5] = 0;
    gaz4[5] = gread * c315;
    gaz5[5] = grew * c315;

    gax6[0] = 0;
    gay6[0] = 0;
    gaz6[0] = gsp;

    gax6[1] = 0;
    gay6[1] = 0;
    gaz6[1] = gsp;

    gax6[2] = gsp;
    gay6[2] = 0;
    gaz6[2] = 0;

    gax6[3] = gsp;
    gay6[3] = 0;
    gaz6[3] = 0;

    gax6[4] = 0;
    gay6[4] = gsp;
    gaz6[4] = 0;

    gax6[5] = 0;
    gay6[5] = gsp;
    gaz6[5] = 0;
  }

  /*DELAYS */
  te1min = rof2 + tror + tramp + tgsettle + 0.5 * invpw1;
  te12min = invpw1 + 4.0 * (tgsettle + tramp);
  te2min = 0.5 * invpw1 + 2.0 * tgsettle + 2.0 * tramp + tssr + at / 2.0;

  if (triang[0] == 'y')
  {
    taumin = 2.0 * tramp + tgsettle + at + trew;
  }
  else
  {
    taumin = 4.0 * tramp + 2.0 * tgsettle + 2.0 * at;
  }

  DA = te1 / 2.0 - te1min;
  if (epi[0] == 'n')
    DA = DA + ((ix - 1) % 2) * tau;
  DB = ((te1 + te2) / 2.0 - te12min) / 2.0 - tsp;
  DC = te2 / 2.0 - te2min;
  DD = 0.0;
  if (epi[0] == 'y')
  {
    DD = tau - taumin;
  }


  /*REPETITION TIME */
  predelay = tr - rof1 - pw/2.0 - te1 - te2 - invpw2 + 0.5*at + tramp - tspoil;
  if (epi[0] == 'y')
  {
    predelay = predelay - nf * tau;
  }
  else
  {
    predelay = predelay - ((ix - 1) % 2) * tau;
  }

  /*INDEX FOR GRADIENT AND OFFSET SELECTION */
  j = ((((ix - 1) % nproj) / alt) + off);
  if (print_index[0] == 'y')
  {
    if (ix == 1)
    {
      fprintf(stderr, "\nINDEX OF GRADIENT SELECTION:\n");
    }
    fprintf(stderr, "  ix =%2d ", (int) ix);
    fprintf(stderr, "  j =%2d \n", (int) j);
  }



  /**[2.4] PARAMETER MISSET ERRORS***********************************/
  sequence_error = 0.0;

  if (pwpat[0] == '\0')
  {
    warn_message("SEQUENCE ERROR: Can't find shape name pwpat\n");
    text_error("SEQUENCE ERROR: Can't find shape name pwpat\n");
    sequence_error = 1.0;
  }

  if (pipat1[0] == '\0')
  {
    warn_message("SEQUENCE ERROR: Can't find shape name pipat1\n");
    text_error("SEQUENCE ERROR: Can't find shape name pipat1\n");
    sequence_error = 1.0;
  }

  if (pipat2[0] == '\0')
  {
    warn_message("SEQUENCE ERROR: Can't find shape name pipat2\n");
    text_error("SEQUENCE ERROR: Can't find shape name pipat2\n");
    sequence_error = 1.0;
  }

  if (predelay < 0.0)
  {
    warn_message("SEQUENCE ERROR: tr is too short\n");
    text_error("SEQUENCE ERROR: tr is too short\n");
    sequence_error = 1.0;
  }

  if (DA < 0.0)
  {
    warn_message("SEQUENCE ERROR: te1 is too short, DA<0, te1(min) = %5.3f s\n",
	       2.0 * te1min);
    text_error("SEQUENCE ERROR: te1 is too short, DA<0, te1(min) = %5.3f s\n",
	       2.0 * te1min);
    sequence_error = 1.0;
  }

  if (DB < 0.0)
  {
    warn_message("SEQUENCE ERROR: te1+te2 is too short, DB < 0\n");
    text_error("SEQUENCE ERROR: te1+te2 is too short, DB < 0\n");
    text_error("                te1(min)+te2(min) = %5.3f s\n", 2.0 * te12min);
    sequence_error = 1.0;
  }

  if (DC < 0.0)
  {
    warn_message("SEQUENCE ERROR: te2 is too short, DC<0, te2(min) = %5.3f s\n",
               2.0 * te2min);
    text_error("SEQUENCE ERROR: te2 is too short, DC<0, te2(min) = %5.3f s\n",
               2.0 * te2min);
    sequence_error = 1.0;
  }

  if (DD < 0.0)
  {
    warn_message("SEQUENCE ERROR: tau is too short, DD<0, tau(min) = %6.4f s\n",
	       taumin);
    text_error("SEQUENCE ERROR: tau is too short, DD<0, tau(min) = %6.4f s\n",
	       taumin);
    sequence_error = 1.0;
  }

  if (tgsettle < rof1)
  {
    warn_message("SEQUENCE ERROR: Negative delay, tgsettle is too short\n");
    text_error("SEQUENCE ERROR: Negative delay, tgsettle is too short\n");
    sequence_error = 1.0;
  }

  if (tgsettle < rof2)
  {
    warn_message("SEQUENCE ERROR: Negative delay, tgsettle is too short\n");
    text_error("SEQUENCE ERROR: Negative delay, tgsettle is too short\n");
    sequence_error = 1.0;
  }

  if (fabs(gread) > 40.0)
  {
    warn_message("SEQUENCE ERROR: Read gradient gread out of range!\n");
    text_error("SEQUENCE ERROR: Read gradient gread out of range!\n");
    sequence_error = 1.0;
  }

  if (fabs(gsl) > 40.0)
  {
    warn_message("SEQUENCE ERROR: Slice selection gradient gsl out of range!\n");
    text_error("SEQUENCE ERROR: Slice selection gradient gsl out of range!\n");
    sequence_error = 1.0;
  }

  if (fabs(grr) > 40.0)
  {
    warn_message("SEQUENCE ERROR: Gradient grr out of range!\n");
    text_error("SEQUENCE ERROR: Gradient grr out of range!\n");
    sequence_error = 1.0;
  }

  if (fabs(gsr) > 40.0)
  {
    warn_message("SEQUENCE ERROR: Gradient gsr out of range!\n");
    text_error("SEQUENCE ERROR: Gradient gsr out of range!\n");
    sequence_error = 1.0;
  }

  if (fabs(grew) > 40.0)
  {
    warn_message("SEQUENCE ERROR: Gradient grew out of range!\n");
    text_error("SEQUENCE ERROR: Gradient grew out of range!\n");
    sequence_error = 1.0;
  }

  if (fabs(grr / tror) > grise_halfsine)
  {
    warn_message("SEQUENCE ERROR: tror is too short (grr)\n");
    text_error("SEQUENCE ERROR: tror is too short (grr)\n");
    sequence_error = 1.0;
  }

  if (fabs(gsl / tramp) > grise_rampup)
  {
    warn_message("SEQUENCE ERROR: tramp too short (gsl)\n");
    text_error("SEQUENCE ERROR: tramp too short (gsl)\n");
    sequence_error = 1.0;
  }

  if (fabs(gsr / tssr) > grise_halfsine)
  {
    warn_message("SEQUENCE ERROR: tssr is too short (gsr)\n");
    text_error("SEQUENCE ERROR: tssr is too short (gsr)\n");
    sequence_error = 1.0;
  }

  if (fabs(gread / tramp) > grise_rampup)
  {
    warn_message("SEQUENCE ERROR: tramp is too short (gread)\n");
    text_error("SEQUENCE ERROR: tramp is too short (gread)\n");
    sequence_error = 1.0;
  }

  if (fabs(grew / trew) > grise_triangle)
  {
    warn_message("SEQUENCE ERROR: trew is too short (grew)\n");
    text_error("SEQUENCE ERROR: trew is too short (grew)\n");
    sequence_error = 1.0;
  }

  if (sequence_error > 0.5)
  {
    abort_message("Aborted");
  }


  /**[2.6] PHASE CYCLE**************************************************/
  loadtable("table_fastestmap");
  setreceiver(t5);

  /**[2.7] CONTROL OF ORIENTATIONS ************************************/

  initval(nf, v6);

  /**[2.7] SEQUENCE ELEMENTS*******************************************/

  status(A);

  delay(predelay);
  xgate(ticks);

  /*EXCITATION AND FIRST SPIN ECHO PERIOD*********** */
  obsoffset(tof);
  obspower(tpwr);
  shaped_pulse(pwpat, pw, t1, rof1, rof2);
  obsoffset(to1[j]);
  delay(DA);

  /*REPHASING OF READOUT GRADIENT********************* */
  obl_shapedgradient("fm_hsine", tror, -gay0[j], -gax0[j], gaz0[j], WAIT);
 

  /*FIRST OF TWO 180 RF PULSES FOR SLICE 1 */
  obl_shapedgradient("fm_rampup", tramp, -gay1[j], -gax1[j], gaz1[j], WAIT);
  delay(tgsettle - rof1);
  shaped_pulse(pipat1, invpw1, t2, rof1, rof2);
  delay(tgsettle - rof2);
  obl_shapedgradient("fm_rampdown", tramp, -gay1[j], -gax1[j], gaz1[j], WAIT);
  
  /*CRUSHER GRADIENT - 1 **************************** */
  obl_shapedgradient("fm_hsine", tsp, -gay6[j], -gax6[j], gaz6[j], WAIT);
 
  /*SELECTION OF SLICE 2 **************************** */
  obsoffset(to2[j]);
  delay(DB);
  obl_shapedgradient("fm_rampup", tramp, -gay3[j], -gax3[j], gaz3[j], WAIT);
  delay(tgsettle - rof1);
  shaped_pulse(pipat2, invpw2, t3, rof1, rof2);
  delay(tgsettle - rof2);
  obl_shapedgradient("fm_rampdown", tramp, -gay3[j], -gax3[j], gaz3[j], WAIT);
 
  /*CRUSHER GRADIENT - 2 **************************** */
  obl_shapedgradient("fm_hsine", tsp, +gay6[j], +gax6[j],  -gaz6[j], WAIT);
  
  /*SECOND OF TWO 180 RF PULSES FOR SLICE 1 ********* */
  obsoffset(to1[j]);
  delay(DB);
  obl_shapedgradient("fm_rampup", tramp,  -gay1[j], -gax1[j], gaz1[j], WAIT);
  delay(tgsettle - rof1);
  shaped_pulse(pipat1, invpw1, t4, rof1, rof2);
  delay(tgsettle - rof2);
  obl_shapedgradient("fm_rampdown", tramp, -gay1[j], -gax1[j], gaz1[j], WAIT);
 
  /*REPHASING THE SLICE 2 *************************** */
  obl_shapedgradient("fm_hsine", tssr, -gay2[j], -gax2[j], gaz2[j], WAIT);
  
  /*READOUT GRADIENT AND DATA ACQUISITION *********** */
  obsoffset(to3[j]);
  delay(DC);
  starthardloop(v6);
  obl_shapedgradient("fm_rampup", tramp, -gay4[j], -gax4[j], gaz4[j], WAIT);
  delay(tgsettle);
  startacq(alfa);
  acquire(np,1.0/sw);
  endacq();
  obl_shapedgradient("fm_rampdown", tramp, -gay4[j], -gax4[j], gaz4[j], WAIT);
    if (epi[0] == 'y')
  {
    delay(DD / 2.0);
    if (triang[0] == 'y')
    {
      obl_shapedgradient("fm_triangle", trew, -gay5[j], -gax5[j], gaz5[j], WAIT);
      
    }
    else
    {
      obl_shapedgradient("fm_rampup", tramp, +krew * gay4[j], +krew * gax4[j], -krew * gaz4[j], WAIT);
      delay(tgsettle);
      delay(np / (2.0 * sw));
      obl_shapedgradient("fm_rampdown", tramp,+krew * gay4[j], +krew * gax4[j],  -krew * gaz4[j], WAIT);
    }
    delay(DD / 2.0);
  }
  endhardloop();
  obl_gradient(-gspoil, -gspoil, gspoil);
  delay(tspoil);
  zero_all_gradients();   

}
/*****************************************************************

10oct01(ss) tof=resto

*****************************************************************/
