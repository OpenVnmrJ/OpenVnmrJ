/* asltest2D.c */
/*---------------------------------------------------------------------------*/
/*                                                                           */
/* asltest2D.c: ASL test mode recon                                          */
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

void asltest2D(struct data *d)
{

  cppar("testpsi",&d->p,"psi",&d->p);
  cppar("testphi",&d->p,"phi",&d->p);
  cppar("testtheta",&d->p,"theta",&d->p);
  cppar("testlro",&d->p,"lro",&d->p);
  cppar("testpro",&d->p,"pro",&d->p);
  cppar("testlpe",&d->p,"lpe",&d->p);
  cppar("testppe",&d->p,"ppe",&d->p);
  cppar("testthk",&d->p,"thk",&d->p);
  cppar("testgap",&d->p,"gap",&d->p);
  cppar("testpss",&d->p,"pss",&d->p);
  setval(&d->p,"nseg",d->nv);
  setdatapars(d);

  /* Now do default recon */
  default2D(d);

}
