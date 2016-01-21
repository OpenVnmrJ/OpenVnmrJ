/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*---------------------------------------------------------------------------*/
/*                                                                           */
/* sglRF.h: Shaped Pulse Library (SPL), formerly spl.h                       */
/*                                                                           */
/* History:                                                                  */
/*  v 1.3: 28Jan09 Added BIR4 and prototype HS. Paul Kinchesh                */
/*  v 1.2: 19Mar07 Merged with SGL. Paul Kinchesh                            */
/*  v 1.1: 14Feb07 Added calibration of HS-AFPs. Paul Kinchesh               */
/*  v 1.0: 05May06 Prototyped sinc,gauss,Mao,HS-AFP,HT-AHP. Paul Kinchesh    */
/*---------------------------------------------------------------------------*/
#ifndef SGLRF_H
#define SGLRF_H

#include "sglCommon.h"

/*---------------------*/
/*---- SPL Version ----*/
/*---------------------*/
#define SPLVERSION 1.3


/*-------------------------*/
/*-------------------------*/
/*---- spl.c functions ----*/
/*-------------------------*/
/*-------------------------*/

/*--------------------------------------------------------------------*/
/*---- The main generation function is genRf(RF_PULSE_T *rf)      ----*/
/*---- It is invoked in sglWrappers.c, defined in sglCommon.h,    ----*/
/*---- and calls SPL subroutines according to the requested shape ----*/
/*--------------------------------------------------------------------*/

/*----------------------------------*/
/*---- sinc generation function ----*/
/*----------------------------------*/
void sincRf(RF_PULSE_T *rf);

/*--------------------------------------*/
/*---- Gaussian generation function ----*/
/*--------------------------------------*/
void gaussRf(RF_PULSE_T *rf);

/*----------------------------------------------------------*/
/*---- Mao generation function                          ----*/
/*---- J.Mao, T.H.Mareci, E.R.Andrew, JMR 79,1-10(1988) ----*/
/*----------------------------------------------------------*/
void maoRf(RF_PULSE_T *rf);

/*-----------------------------------------------------------------*/
/*---- hyperbolic secant (HS) adiabatic generation function    ----*/
/*---- M.S.Silver, R.I.Joseph, D.I.Hoult, JMR 59,347-351(1984) ----*/
/*-----------------------------------------------------------------*/
void hsafpRf(RF_PULSE_T *rf);

/*-----------------------------------------------------------------*/
/*---- hyperbolic secant (HS) generation function              ----*/
/*---- M.S.Silver, R.I.Joseph, D.I.Hoult, JMR 59,347-351(1984) ----*/
/*-----------------------------------------------------------------*/
void hsRf(RF_PULSE_T *rf);

/*-------------------------------------------------------------------*/
/*---- tanh/tan adiabatic half passage (AHP) generation function ----*/
/*---- M.Garwood, Y.Ke, JMR 94,511-525(1991)                     ----*/
/*-------------------------------------------------------------------*/
void htahpRf(RF_PULSE_T *rf);

/*-----------------------------------------------*/
/*---- tanh/tan BIR-4 generation function    ----*/
/*---- M.Garwood, Y.Ke, JMR 94,511-525(1991) ----*/
/*-----------------------------------------------*/
void htbir4Rf(RF_PULSE_T *rf);

/*----------------------------------*/
/*---- sine generation function ----*/
/*----------------------------------*/
void sineRf(RF_PULSE_T *rf);

/*----------------------------------------------------------*/
/*---- function to scale amplitude to maximum of 1023.0 ----*/
/*----------------------------------------------------------*/
void scaleampRf(RF_PULSE_T *rf);

/*-------------------------------------------------------*/
/*---- function to remove the cutoff of pulses       ----*/
/*---- R.A.de Graff, K.Nicolay, MRM 40,690-696(1998) ----*/
/*-------------------------------------------------------*/
void rmcutoffRf(RF_PULSE_T *rf);

/*------------------------------------------------------*/
/*---- function to calculate the integral of pulses ----*/
/*------------------------------------------------------*/
void calcintRf(RF_PULSE_T *rf);

/*-----------------------------------------------------*/
/*---- function to simulate an 'integral' for AFPs ----*/
/*-----------------------------------------------------*/
void simAFPintRf(RF_PULSE_T *rf);

/*--------------------------------------------*/
/*---- function to simulate an 'integral' ----*/
/*--------------------------------------------*/
void simintRf(RF_PULSE_T *rf,double targetmz);

/*-------------------------------------------------------------*/
/*---- function to generate positive pulse amplitudes only ----*/
/*-------------------------------------------------------------*/
void absampRf(RF_PULSE_T *rf);

/*---------------------------------------------------*/
/*---- function to write the pulse shape to disk ----*/
/*---------------------------------------------------*/
void writeRf(RF_PULSE_T *rf);

/*------------------------------------*/
/*---- function to set shape name ----*/
/*------------------------------------*/
void setshapeRf(RF_PULSE_T *rf);

/*-----------------------------------------------*/
/*---- function to get parameters from VnmrJ ----*/
/*-----------------------------------------------*/
void getparsRf(RF_PULSE_T *rf);

/*-------------------------------------------------------*/
/*---- function to round pulse resolution & duration ----*/
/*-------------------------------------------------------*/
void roundparsRf(RF_PULSE_T *rf);

/*------------------------------------------------------------------------*/
/*---- function to round pulse resolution & duration to multiple of 4 ----*/
/*------------------------------------------------------------------------*/
void roundpars4Rf(RF_PULSE_T *rf);

/*------------------------------------------------*/
/*---- function to return parameters to VnmrJ ----*/
/*------------------------------------------------*/
void putparsRf(RF_PULSE_T *rf);

/*-----------------------------------------*/
/*---- abort funtion for out of memory ----*/
/*-----------------------------------------*/
void nomem();


/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
/*>>>>>>>>>>>>> START OF SECTTION TO REMOVE AFTER PSG IS FIXED <<<<<<<<<<<*/
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
void t_sincRf(RF_PULSE_T *rf);
void t_gaussRf(RF_PULSE_T *rf);
void t_maoRf(RF_PULSE_T *rf);
void t_hsafpRf(RF_PULSE_T *rf);
void t_hsRf(RF_PULSE_T *rf);
void t_htahpRf(RF_PULSE_T *rf);
void t_htbir4Rf(RF_PULSE_T *rf);
void t_sineRf(RF_PULSE_T *rf);
void t_scaleampRf(RF_PULSE_T *rf);
void t_rmcutoffRf(RF_PULSE_T *rf);
void t_calcintRf(RF_PULSE_T *rf);
void t_simAFPintRf(RF_PULSE_T *rf);
void t_simintRf(RF_PULSE_T *rf,double targetmz);
void t_absampRf(RF_PULSE_T *rf);
void t_writeRf(RF_PULSE_T *rf);
void t_setshapeRf(RF_PULSE_T *rf);
void t_getparsRf(RF_PULSE_T *rf);
void t_roundparsRf(RF_PULSE_T *rf);
void t_roundpars4Rf(RF_PULSE_T *rf);
void t_putparsRf(RF_PULSE_T *rf);
void t_nomem();

void x_sincRf(RF_PULSE_T *rf);
void x_gaussRf(RF_PULSE_T *rf);
void x_maoRf(RF_PULSE_T *rf);
void x_hsafpRf(RF_PULSE_T *rf);
void x_hsRf(RF_PULSE_T *rf);
void x_htahpRf(RF_PULSE_T *rf);
void x_htbir4Rf(RF_PULSE_T *rf);
void x_sineRf(RF_PULSE_T *rf);
void x_scaleampRf(RF_PULSE_T *rf);
void x_rmcutoffRf(RF_PULSE_T *rf);
void x_calcintRf(RF_PULSE_T *rf);
void x_simAFPintRf(RF_PULSE_T *rf);
void x_simintRf(RF_PULSE_T *rf,double targetmz);
void x_absampRf(RF_PULSE_T *rf);
void x_writeRf(RF_PULSE_T *rf);
void x_setshapeRf(RF_PULSE_T *rf);
void x_getparsRf(RF_PULSE_T *rf);
void x_roundparsRf(RF_PULSE_T *rf);
void x_roundpars4Rf(RF_PULSE_T *rf);
void x_putparsRf(RF_PULSE_T *rf);
void x_nomem();

#endif
