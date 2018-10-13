/* noise2D.c */
/*---------------------------------------------------------------------------*/
/*                                                                           */
/* noise2D.c: 2D routines for noise matrix measurements                      */
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

/*-----------------------*/
/*---- Some defaults ----*/
/*-----------------------*/
/* Knoisefraction: Fraction of k-space FOV to use to sample noise */
static double Knoisefraction=0.05;

/* IMnoisefraction: Fraction of image space FOV to use to sample noise */
static double IMnoisefraction=0.05;


void get2Dnoisematrix(struct data *d,int mode)
{
  int dim1,dim2,dim3,nr;
  double noisefrac;
  int dim1centre,dim2centre,dim1offset,dim2offset,ix;
  int h,i,j,k,l;
  int n=0,datamode;
  gsl_complex cx,cx1,cx2;

#ifdef DEBUG
  struct timeval tp;
  double t1,t2;
  int rtn;
#endif

  /* If noise data does not exist set it to zero */
  if (!d->noise.data) zeronoise(d);

  /* If noise data is zero sample the noise */
  if (d->noise.zero) getnoise(d,mode);

  /* Equalize noise if it has not been done */
  if (!d->noise.equal) equalizenoise(d,mode);

#ifdef DEBUG
  fprintf(stdout,"\n%s: %s()\n",__FILE__,__FUNCTION__);
  rtn=gettimeofday(&tp, NULL);
  t1=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
  fflush(stdout);
#endif

  /* Data dimensions */
  dim1=d->np/2; dim2=d->nv; dim3=d->endpos-d->startpos; nr=d->nr;

  datamode=FID;
  /* If FT has been done flag it as IMAGE */
  if ((d->dimstatus[0] & FFT) || (d->dimstatus[1] & FFT)) datamode=IMAGE;

  noisefrac=0.0;
  switch(mode) {
    case MK: /* Mask parameters */
      switch(datamode) {
        case FID:
          noisefrac=*val("masknoisefrac",&d->p);
          if (!noisefrac) noisefrac=Knoisefraction;
          break;
        default:
          noisefrac=*val("masklvlnoisefrac",&d->p);
          if (!noisefrac) noisefrac=IMnoisefraction;
      } /* end datamode switch */
      break;
    case SM: /* Sensitivity map parameters */
      switch(datamode) {
        case FID:
          noisefrac=*val("smapnoisefrac",&d->p);
          if (!noisefrac) noisefrac=Knoisefraction;
          break;
        default:
          noisefrac=*val("masklvlnoisefrac",&d->p);
          if (!noisefrac) noisefrac=IMnoisefraction;
      } /* end datamode switch */
      break;
    default: /* Default parameters */
      switch(datamode) {
        case FID:
          noisefrac=Knoisefraction;
          break;
        default:
          noisefrac=IMnoisefraction;
      } /* end datamode switch */
  } /* end mode switch */

  /* Allocate memory for noise matrix */
  if (!d->noise.mat) {
    if ((d->noise.mat = (gsl_matrix_complex **)malloc(dim3*sizeof(gsl_matrix_complex *))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
    for (j=0;j<dim3;j++)
      d->noise.mat[j]=(gsl_matrix_complex *)gsl_matrix_complex_calloc(nr,nr);
  }

  /* For shifted FID and not-shifted IMAGE the noise is at centre */
  /* Set the centre accordingly */
  if ((!(d->dimstatus[0] & FFT) && (d->dimstatus[0] & SHIFT))
    || ((d->dimstatus[0] & FFT) && !(d->dimstatus[0] & SHIFT))) {
    dim1centre=dim1/2;
  } else {
    dim1centre=dim1;
  }
  if ((!(d->dimstatus[1] & FFT) && (d->dimstatus[1] & SHIFT))
    || ((d->dimstatus[1] & FFT) && !(d->dimstatus[1] & SHIFT))) {
    dim2centre=dim2/2;
  } else {
    dim2centre=dim2;
  }

  /* Set the offset for the specified range */
  dim1offset = (int)((dim1*noisefrac)/2.0);
  dim2offset = (int)((dim2*noisefrac)/2.0);
  if (!dim1offset) dim1offset = 1;
  if (!dim2offset) dim2offset = 1;

  for (h=0;h<nr;h++) {
    for (i=0;i<nr;i++) {
      for (j=0;j<dim3;j++) {
        n=0;
        GSL_SET_COMPLEX(&cx,0.0,0.0);
        for(k=0;k<2*dim2offset;k++) {
          for (l=0;l<2*dim1offset;l++) {
            ix=(k+dim2centre-dim2offset)%dim2;
            ix=ix*dim1+(l+dim1centre-dim1offset)%dim1;
            GSL_SET_REAL(&cx1,d->data[h][j][ix][0]);
            GSL_SET_IMAG(&cx1,d->data[h][j][ix][1]);
            GSL_SET_REAL(&cx2,d->data[i][j][ix][0]);
            GSL_SET_IMAG(&cx2,d->data[i][j][ix][1]);
            cx=gsl_complex_add(cx,gsl_complex_mul(cx1,gsl_complex_conjugate(cx2)));
            n++;
          }
        }
        /* Take the mean and normalize */
        GSL_SET_COMPLEX(&cx,GSL_REAL(cx)/(n*d->noise.avM2),GSL_IMAG(cx)/(n*d->noise.avM2));
        gsl_matrix_complex_set(d->noise.mat[j],h,i,cx);
      }
    }
  }

#ifdef DEBUG
  rtn=gettimeofday(&tp, NULL);
  t2=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
  fprintf(stdout,"  Took %f secs\n",t2-t1);
  print2Dnoisematrix(d);
  fflush(stdout);
#else
  /* Print Noise Matrix, if requested */
  if ((spar(d,"printNM","y"))) print2Dnoisematrix(d);
#endif

}

void zero2Dnoisematrix(struct data *d)
{
  int dim3,nr;
  int h,i,j;
  gsl_complex cx;
  /* Set dimension */
  dim3=d->endpos-d->startpos; nr=d->nr;
  /* Allocate memory for noise matrix */
  if (!d->noise.mat) {
    if ((d->noise.mat = (gsl_matrix_complex **)malloc(dim3*sizeof(gsl_matrix_complex *))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
    for (j=0;j<dim3;j++)
      d->noise.mat[j]=(gsl_matrix_complex *)gsl_matrix_complex_calloc(nr,nr);
  } else {
    GSL_SET_COMPLEX(&cx,0.0,0.0);
    for (j=0;j<dim3;j++) {
      for (h=0;h<d->nr;h++) {
        for (i=0;i<d->nr;i++) {
          gsl_matrix_complex_set(d->noise.mat[j],h,i,cx);
        }
      }
    }
  }
}

void print2Dnoisematrix(struct data *d)
{
  int dim3;
  int h,i,j;
  gsl_complex cx;

  if (d->noise.mat) {
    fprintf(stdout,"\n%s: %s()\n",__FILE__,__FUNCTION__);
    dim3=d->endpos-d->startpos;
    for (j=0;j<dim3;j++) {
      fprintf(stdout,"  Noise Matrix, slice %d:\n",j+d->startpos+1);
      for (h=0;h<d->nr;h++) {
        for (i=0;i<d->nr;i++) {
          cx=gsl_matrix_complex_get(d->noise.mat[j],h,i);
          fprintf(stdout,"  [%d,%d] = %f , %fi\n",h,i,GSL_REAL(cx),GSL_IMAG(cx));
        }
      }
    }
  }
}
