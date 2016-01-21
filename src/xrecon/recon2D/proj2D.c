/* proj2D.c */
/*---------------------------------------------------------------------------*/
/*                                                                           */
/* proj2D.c: 2D projection recon                                             */
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

void proj2D(struct data *d)
{
  char seqcon[6];

  /* For 2D projections we fake the data is 2D and recon accordingly */

  /* Get the seqcon */
  strcpy(seqcon,*sval("seqcon",&d->p));

  /* ROxPE projection has profile='ny' */
  if (spar(d,"profile","ny")) {
    /* Adjust seqcon for 2D */
    seqcon[3]='n';
    setsval(&d->p,"seqcon",seqcon);
    setseqmode(d);
    default2D(d);
  }

  /* ROxPE2 projection has profile='yn' */
  if (spar(d,"profile","yn")) {
    /* Adjust seqcon for 2D */
    seqcon[2]=seqcon[3];
    seqcon[3]='n';
    setsval(&d->p,"seqcon",seqcon);
    setval(&d->p,"nv",d->nv2);
    setval(&d->p,"nseg",d->nv2);
    setval(&d->p,"etl",1);
    setval(&d->p,"pelist",0);
    setval(&d->p,"nv2",1);
    setsval(&d->p,"apptype","im2D");
    setdatapars(d);
    setseqmode(d);
    /* The following is required to be able to graphically plan on the output images:
       thk = lpe
       lpe = lpe2
       ppe of each slice in turn = pss of each slice in turn
       psi, phi, theta need to be corrected (90 degree rotation about readout axis)
    */
    /* For now just adjust FOV and offset for centre of slice(s) */
    setval(&d->p,"thk",*val("lpe",&d->p));
    setval(&d->p,"lpe",*val("lpe2",&d->p));
    setval(&d->p,"ppe",*val("pss0",&d->p));
    default2D(d);
  }

}
