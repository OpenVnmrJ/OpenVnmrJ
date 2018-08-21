/* dutils1D.c */
/*---------------------------------------------------------------------------*/
/*                                                                           */
/* dutils1D.c: 1D data utilities                                             */
/*                                                                           */
/* Copyright (C) 2012 Paul Kinchesh                                          */
/*               2012 Martyn Klassen                                         */
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

void clear1Dall(struct data *d)
{
  clear1Ddata(d);
  clear1Dmask(d);
  clearnoise1D(d);
  clearpars(&d->p);
  clearpars(&d->s);
  cleararray(&d->a);
}

void clear1Ddata(struct data *d)
{
  int dim3;
  int i,j;
  if (d->data) {
    dim3=d->fh.ntraces;
    for (i=0;i<d->nr;i++) {
      for (j=0;j<dim3;j++) fftw_free(d->data[i][j]);
      fftw_free(d->data[i]);
    }
    fftw_free(d->data);
    d->data = NULL;
  }
}

void clearnoise1D(struct data *d)
{
  int dim3;
  int j;
  if (d->noise.data) {
    free(d->noise.M);
    free(d->noise.M2);
    free(d->noise.Re);
    free(d->noise.Im);
    d->noise.M=NULL;
    d->noise.M2=NULL;
    d->noise.Re=NULL;
    d->noise.Im=NULL;
  }
  if (d->noise.mat) {
    dim3=d->fh.ntraces;
    for (j=0;j<dim3;j++) gsl_matrix_complex_free(d->noise.mat[j]);
    free(d->noise.mat);
    d->noise.mat=NULL;
  }
  /* Set flags */
  d->noise.data=FALSE;
  d->noise.zero=FALSE;
  d->noise.equal=FALSE;
  d->noise.samples=NONE;
}

void clear1Dmask(struct data *d)
{
  int dim3;
  int i; 
  if (d->mask) {
    dim3=d->fh.ntraces;
    for (i=0;i<dim3;i++) free(d->mask[i]);
    free(d->mask);
    d->mask=NULL;
  }
}
