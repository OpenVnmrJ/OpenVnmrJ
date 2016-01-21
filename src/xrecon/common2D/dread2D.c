/* dread2D.c */
/*---------------------------------------------------------------------------*/
/*                                                                           */
/* dread2D.c: 2D Data read routines                                          */
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

void getblock2D(struct data *d,int volindex,int DCCflag)
{
  setblock(d,d->ns);
  getblock(d,volindex,DCCflag);
}

void getnavblock2D(struct data *d,int volindex,int DCCflag)
{
  setblock(d,d->ns);
  getnavblock(d,volindex,DCCflag);
}

int process2Dblock(struct data *d,char *startpar,char *endpar)
{
  int blockns;
  int startpos,endpos;
  int start,end;

  /* Number of pss values = slices */
  d->ns=nvals("pss",&d->p);

  /* There must be at least one block per volume */
  d->nblocks=(int)*val("nblocks",&d->p);
  if (!d->nblocks) d->nblocks++;

  /* There can not be more blocks than slices */
  if (d->nblocks>d->ns) d->nblocks=d->ns;

  /* Figure how many slices are required per block */
  blockns=d->ns/d->nblocks;
  if (d->ns%d->nblocks) blockns++;

  /* Figure start slice and end slice of the block */
  startpos=d->block*blockns;
  endpos=(d->block+1)*blockns;
  if (endpos>d->ns) endpos=d->ns;

  /* Get start slice and end slice to be processed */
  start=(int)*val(startpar,&d->p);
  if ((start<1) || (start>d->ns)) start=1;

  end=(int)*val(endpar,&d->p);
  if ((end<1) || (end>d->ns)) end=d->ns;

  /* Return whether to process or not */
  if ((start>endpos) || (end<=startpos)) {
#ifdef DEBUG
  fprintf(stdout,"\n%s: %s()\n",__FILE__,__FUNCTION__);
  fprintf(stdout,"  Skipping processing of block %d (of %d)\n",d->block+1,d->nblocks);
#endif
    return(FALSE);
  }

  return(TRUE);

}
