/* recon3D.c */
/*---------------------------------------------------------------------------*/
/*                                                                           */
/* recon3D.c: 3D recon                                                       */
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

void recon3D(struct data *d)
{

  /* If profile flag is set then recon profiles */
  if (d->profile) profile1D(d);

  /* If 2D projection flag then recon projection */
  else if (d->proj2D) proj2D(d);

  /* Else check recon flag for non-standard recons ... */

  /* Otherwise perform standard 3D recon */
  else default3D(d);

}
