/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/***********************************************************************
*   HISTORY:
*     Revision 1.1  2006/09/11 16:21:48  deans
*     Added Maj's changes for epi
*     At Maj's request, renamed sglEpi.h and sglEpi.c -> sglEPI.h and sglEPI.c
*
*     Revision 1.1  2006/08/23 14:09:57  deans
*     *** empty log message ***
*
*     Revision 1.1  2006/08/22 23:30:04  deans
*     *** empty log message ***
*
*     Revision 1.2  2006/07/11 20:09:58  deans
*     Added explicit prototypes for getvalnowarn etc. to sglCommon.h
*     - these are also defined in  cpsg.h put can't #include that file because
*       gcc complains about the "extern C" statement which is only allowed
*       when compiling with g++ (at least for gcc version 3.4.5-64 bit)
*
*     Revision 1.1  2006/07/07 01:11:28  mikem
*     modification to compile with psg
*
*
***************************************************************************/
#ifndef SGLEPI_H
#define SGLEPI_H


extern double   nseg;                              /* number of segments for multi shot experiment */
extern char     navigator[MAXSTR];                 /* navigator flag: y = ON, n = OFF */
extern double   tep;                               /* group delay - gradient*/

/* phase encode order */
extern char    ky_order[MAXSTR];                    /* phase order flag : y = ON, n = OFF */
extern double  fract_ky;

/* EPI tweakers and others */
extern double  groa;                                /* EPI tweaker - readout */
extern double  grora;                               /* EPI tweaker - dephase */
extern double  image;                               /* EPI repetitions */
extern double  images;                              /* EPI repetitions */
extern double  ssepi;                               /* EPI readput steady state */
extern char    rampsamp[MAXSTR];      

/* Gradient structures */
extern EPI_GRADIENT_T epi_grad;                     /* General EPI struct */

void   init_epi( EPI_GRADIENT_T *epi_grad);
void   calc_readout_rampsamp( READOUT_GRADIENT_T *grad,double *dwell, 
                              double *minskip,int *npr);
void   calc_epi( EPI_GRADIENT_T          *epi_grad, 
                 READOUT_GRADIENT_T      *ro_grad,
                 PHASE_ENCODE_GRADIENT_T *pe_grad,
                 REFOCUS_GRADIENT_T      *ror_grad,
                 PHASE_ENCODE_GRADIENT_T *per_grad,
                 READOUT_GRADIENT_T      *nav_grad,
                 int write_flag);
void   displayEPI( EPI_GRADIENT_T *epi_grad);

#endif
