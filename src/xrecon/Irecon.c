/* Irecon.c */
/* Irecon is the internal (to Vnmrbg) shared library version of Xrecon       */
/*---------------------------------------------------------------------------*/
/*                                                                           */
/* Irecon.c: External reconstruction                                         */
/*                                                                           */
/* Copyright (C) 2011 Paul Kinchesh                                          */
/*               2011 Martyn Klassen                                         */
/*                                                                           */
/* Irecon is free software: you can redistribute it and/or modify            */
/* it under the terms of the GNU General Public License as published by      */
/* the Free Software Foundation, either version 3 of the License, or         */
/* (at your option) any later version.                                       */
/*                                                                           */
/* Xrecon is distributed in the hope that it will be useful,                 */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of            */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the              */
/* GNU General Public License for more details.                              */
/*                                                                           */
/* You should have received a copy of the GNU General Public License         */
/* along with Xrecon. If not, see <http://www.gnu.org/licenses/>.            */
/*                                                                           */
/*---------------------------------------------------------------------------*/
/**/
/*---------------------------------------------------------------------------*/
/*                                                                           */
/* Usage:   Xrecon [-v] VnmrJ.fid(s)                                         */
/*            Options:                                                       */
/*              [-v] flags Xrecon that it has been called from within VnmrJ  */
/*                   'VnmrJ.fid' should then be set to curexp                */
/*                                                                           */
/*---------------------------------------------------------------------------*/
/**/


/*---------------------*/
/*---- Define data ----*/
/*---------------------*/
struct data d0;


/*----------------------------------*/
/*---- Define input filename(s) ----*/
/*----------------------------------*/
struct file f0;


/*-------------------------*/
/*---- Include globals ----*/
/*-------------------------*/
#define LOCAL /* EXTERN globals will not be 'extern' */
#include "Xrecon.h"

extern int interuption;

int Irecon(int argc, char *argv[], int retc, char *retv[])
{
  int i;

  /* Null the data structure */
  nulldata(&d0);

  /* Get input options and directories */
  getoptions(&f0,argc,argv);

  /* Loop over input '.fid' directories */
  for (i=0;i<f0.nfiles;i++) {

    /* Get pars from f.procpar[i] */
    getpars(f0.procpar[i],&d0);

    /* Open data file f.fid[i] and get file header */
    opendata(f0.fid[i],&d0);

    /* Set data structure members from procpar values */
    setdatapars(&d0);

    /* Check apptype to determine recon type */

    if (spar(&d0,"apptype","im1D")) recon1D(&d0);

    else if (spar(&d0,"apptype","im2D")) recon2D(&d0);

    else if (spar(&d0,"apptype","im2Dfse")) recon2D(&d0);

    else if (spar(&d0,"apptype","im2Depi")) reconEPI(&d0);

    else if (spar(&d0,"apptype","im3D")) recon3D(&d0);

    else if (spar(&d0,"apptype","im3Dfse")) recon3D(&d0);

    else if (spar(&d0,"apptype","im2Dcsi")) reconCSI2D(&d0);

    else if (spar(&d0,"apptype","im3Dcsi")) reconCSI3D(&d0);

    /* Close data file f.fid[i] */
    closedata(&d0);

  }

  return(0);

}
