/* dread1D.c */
/*---------------------------------------------------------------------------*/
/*                                                                           */
/* dread1D.c: 1D Data reading routines                                       */
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

void getblock1D(struct data *d,int volindex,int DCCflag)
{
  /* Profiles are run from 2D or 3D scans but we process just the same as 1D */
  /* If it's a 2D or 3D sequence set the sequence mode to 1D */
  if (d->seqmode>IM2D) d->seqmode=IM1D;

  /* Set start and end of block */
  d->startpos=0;
  d->endpos=d->fh.ntraces;

  /* Get the data block */
  getblock(d,volindex,DCCflag);

}
