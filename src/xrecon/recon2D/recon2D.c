/* recon2D.c */
/*---------------------------------------------------------------------------*/
/*                                                                           */
/* recon2D.c: 2D recon                                                       */
/*                                                                           */
/* Copyright (C) 2012 Paul Kinchesh                                          */
/*                                                                           */
/* This file is part of Xrecon.                                              */
/*                                                                           */
/* Xrecon is free software: you can redistribute it and/or modify            */
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

#include "../Xrecon.h"

void recon2D(struct data *d)
{

  /* If profile flag is set then recon profiles */
  if (d->profile) profile1D(d);

  /* Else check recon flag for non-standard recons ... */

  /* Masking */
  else if (spar(d,"recon","mask")) mask2D(d);

  /* Adding to masks */
  else if (spar(d,"recon","add2mask")) add2mask2D(d);

  /* Sensitivity maps */
  else if (spar(d,"recon","smap")) smap2D(d,SM);

  /* SENSE */
  else if (spar(d,"recon","sense")) sense2D(d);

  /* Ability to perform SENSE */
  else if (spar(d,"recon","sensibility")) sensibility2D(d);

  /* ASL test mode */
  else if (spar(d,"recon","asltest")) asltest2D(d);

  /* Otherwise perform standard 2D recon */
  else default2D(d);

}
