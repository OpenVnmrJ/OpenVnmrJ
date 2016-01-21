/* dproc1D.c */
/*---------------------------------------------------------------------------*/
/*                                                                           */
/* dproc1D.c: 1D Data processing routines                                    */
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

void fft1D(struct data *d,int dataorder)
{
  fftw_complex *trace;
  fftw_plan p;
  int dim1=0,dim3=0,nr=0;
  int i,j;

#ifdef DEBUG
  struct timeval tp;
  double t1,t2;
  int rtn;
  fprintf(stdout,"\n%s: %s()\n",__FILE__,__FUNCTION__);
  rtn=gettimeofday(&tp, NULL);
  t1=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
  fflush(stdout);
#endif

/*
  fftw_import_wisdom_from_file(FILE *input_file);
*/

  /* Set data dimensions */
  switch (dataorder) {
    case D1:
      dim1=d->np/2; dim3=d->fh.ntraces; nr=d->nr; 
      /* Set status for dimension */
      d->dimstatus[0]+=FFT;
      if (d->dimstatus[0] & SHIFT) d->dimstatus[0]-=SHIFT;
      break;
    case D3:
      dim1=d->nv2; dim3=(d->endpos-d->startpos)*d->np/2; nr=d->nr;
      /* Set status for dimension */
      d->dimstatus[2]+=FFT;
      if (d->dimstatus[2] & SHIFT) d->dimstatus[2]-=SHIFT;
      break;
  }

  /* Allocate memory for a trace */
  if ((trace = (fftw_complex *)fftw_malloc(dim1*sizeof(fftw_complex))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);

  /* Measured plan for 1D FT */
  p=fftw_plan_dft_1d(dim1,trace,trace,FFTW_FORWARD,FFTW_MEASURE);

  /* Do the planned FT ... */
  for (i=0;i<nr;i++) {
    for (j=0;j<dim3;j++) {
      fftw_execute_dft(p,d->data[i][j],d->data[i][j]);
    }
  }
  /* ... and tidy up */
  fftw_destroy_plan(p);
  fftw_free(trace);

#ifdef DEBUG
  rtn=gettimeofday(&tp, NULL);
  t2=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
  fprintf(stdout,"  ft (%d): %d trace(s), %d receiver(s): took %f secs\n",
    dim1,dim3,nr,t2-t1);
  fflush(stdout);
#endif

}

void shiftdata1D(struct data *d,int mode,int dataorder)
{
  int dim1=0,shft=0,*status=NULL;

  /* Set data dimension and status */
  switch (dataorder) {
    case D1:
      dim1=d->np/2;
      status=&d->dimstatus[0];
      break;
    case D3: 
      dim1=d->nv2;
      status=&d->dimstatus[2];
      break;
  }

  /* Set shift points */
  switch(mode) {
    case STD:
      if (*status & SHIFT) {
        shft=(int)ceil(dim1/2.0);
        *status-=SHIFT;
      } else {
        shft=(int)floor(dim1/2.0);
        *status+=SHIFT;
      }
#ifdef DEBUG
  fprintf(stdout,"\n%s: %s()\n",__FILE__,__FUNCTION__);
  switch(dataorder) {
    case D1: fprintf(stdout,"  Standard shift: npshft = %d\n",shft); break;
    case D3: fprintf(stdout,"  Standard shift: nv2shft = %d\n",shft); break;
  }
  fflush(stdout);
#endif
      break;
    default:
      /* Invalid mode */
      fprintf(stderr,"\n%s: %s()\n",__FILE__,__FUNCTION__);
      fprintf(stderr,"  Invalid 2nd argument %s(*,'type',*)\n",__FUNCTION__);
      return;
      break;
  } /* end mode switch */

  /* Shift the data */
  shift1Ddata(d,shft,dataorder);

}

void shift1Ddata(struct data *d,int shft,int dataorder)
{
  fftw_complex *trace;
  int dim1=0,dim3=0,nr=0;
  int i,j,k;

#ifdef DEBUG
  struct timeval tp;
  double t1,t2;
  int rtn;
  fprintf(stdout,"\n%s: %s()\n",__FILE__,__FUNCTION__);
  fprintf(stdout,"  shft = %d: ",shft);
  rtn=gettimeofday(&tp, NULL);
  t1=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
  fflush(stdout);
#endif

  /* Set data dimensions */
  switch (dataorder) {
    case D1: dim1=d->np/2; dim3=d->fh.ntraces; nr=d->nr; break;
    case D3: dim1=d->nv2; dim3=(d->endpos-d->startpos)*d->np/2; nr=d->nr; break;
  }

  /* Make sure shift is in range */
  shft = shft % dim1;

  /* Return if shift not required */
  if (shft == 0) return;

  /* Allocate memory for data */
  if ((trace = (fftw_complex *)fftw_malloc(dim1*sizeof(fftw_complex))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);

  /* Loop over receivers and slices */
  for (i=0;i<nr;i++) {
    for (j=0;j<dim3;j++) {
      /* Fill data with shifted data */
      for (k=0;k<dim1;k++) {
        trace[(k+shft+dim1)%dim1][0]=d->data[i][j][k][0];
        trace[(k+shft+dim1)%dim1][1]=d->data[i][j][k][1];
      }
      /* Copy shifted data back to d->data */
      for (k=0;k<dim1;k++) {
        d->data[i][j][k][0]=trace[k][0];
        d->data[i][j][k][1]=trace[k][1];
      }
    }
  }

  fftw_free(trace);

#ifdef DEBUG
  rtn=gettimeofday(&tp, NULL);
  t2=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
  fprintf(stdout,"took %f secs\n",t2-t1);
  fflush(stdout);
#endif

}

void weightdata1D(struct data *d,int mode,int dataorder)
{
  int i,j,k;
  double f;
  int dim1=0,dim3=0,nr=0;
  double lb=0,gf=0,sb=0,at,factor=1,shft=0;
  double *weight;

#ifdef DEBUG
  struct timeval tp;
  double t1,t2;
  int rtn;
#endif

  switch(mode) {
    case STD: /* Standard parameters */
      switch (dataorder) {
        case D1:
          if (d->dimstatus[0] & FFT) return;
          lb=*val("lb",&d->p); gf=*val("gf",&d->p); sb=*val("sb",&d->p);
          break;
        case D3:
          if (d->dimstatus[2] & FFT) return;
          lb=*val("lb2",&d->p); gf=*val("gf2",&d->p); sb=*val("sb2",&d->p);
          break;
      }
      break;
    default:
      break;
  } /* end mode switch */

  /* Return if no weighting is active */
  if (!lb && !gf && !sb) return;

#ifdef DEBUG
  fprintf(stdout,"\n%s: %s()\n",__FILE__,__FUNCTION__);
  rtn=gettimeofday(&tp, NULL);
  t1=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
  fflush(stdout);
#endif

  /* Data dimensions */
  switch (dataorder) {
    case D1:
      /* D1 used for im1D spectral recons and image profiles */
      dim1=d->np/2; dim3=d->fh.ntraces; nr=d->nr;
      if (d->profile) {
        factor=1;
        if (d->dimstatus[0] & SHIFT) shft=0;
        else shft=dim1/2-1;
      } else {
        at=*val("at",&d->p);
        factor=at/dim1;
        shft=0;
      }
      break;
    case D3:
      /* D3 used for 3D 2nd phase encode */
      dim1=d->nv2; dim3=(d->endpos-d->startpos)*d->np/2; nr=d->nr;
      factor=1;
      if (d->dimstatus[2] & SHIFT) shft=0;
      else shft=dim1/2-1;
      break;
  }

  /* Calculate weighting */
  if ((weight = (double *)malloc(dim1*sizeof(double))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
  for (k=0;k<dim1;k++) weight[k]=1.0;

  /* Lorentzian line broadening */
  if (lb) { /* lb is active */
    for (k=0;k<dim1/2;k++) {
      /* Lorentzian broadening as fraction of FOV */
      /* f=exp(-k*M_PI*lb); */
      /* Lorentzian broadening in # pixels */
      /* f=exp(-k*M_PI*lb/dim1); */
      /* Lorentzian broadening in Hz (as VnmrJ) */
      f=exp(-abs(k-shft)*M_PI*lb*factor);
      weight[k] *=f;
      weight[dim1-1-k] *=f;
    }
  }

  /* Gaussian line broadening */
  if (gf) { /* gf is active */
    for (k=0;k<dim1/2;k++) {
      /* Gaussian broadening as fraction of FOV */
      f=(k-shft)*M_PI*gf;
      f=exp(-f*f);
      weight[k] *=f;
      weight[dim1-1-k] *=f;
    }
  }

  /* Sinebell line broadening */
  if (sb) { /* sb is active */
    for (k=0;k<dim1/2;k++) {
      /* Sinebell broadening as fraction of FOV */
      f=(k-shft)*M_PI*sb;
      if (FP_LT(fabs(f),M_PI/2))
        f=cos(f);
      else
        f=0.0;
      weight[k] *=f;
      weight[dim1-1-k] *=f;
    }
  }

  /* Weight the data */
  for (i=0;i<nr;i++) {
    for (j=0;j<dim3;j++) {
      for (k=0;k<dim1;k++) {
        d->data[i][j][k][0] *=weight[k];
        d->data[i][j][k][1] *=weight[k];
      }
    }
  }

#ifdef DEBUG
  rtn=gettimeofday(&tp, NULL);
  t2=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
  switch (dataorder) {
    case D1:
      if (lb) fprintf(stdout,"  lb weighting = %f\n",lb);
      if (gf) fprintf(stdout,"  gf weighting = %f\n",gf);
      if (sb) fprintf(stdout,"  sb weighting = %f\n",sb);
      break;
    case D3:
      if (lb) fprintf(stdout,"  lb2 weighting = %f\n",lb);
      if (gf) fprintf(stdout,"  gf2 weighting = %f\n",gf);
      if (sb) fprintf(stdout,"  sb2 weighting = %f\n",sb);
      break;
  }
  fprintf(stdout,"  Took %f secs\n",t2-t1);
  fflush(stdout);
#endif
}

void zerofill1D(struct data *d,int mode,int dataorder)
{
  int i,j,k;
  int ix1,ix2;
  int dim1=0,dim3=0,nr=0;
  int fn=0,ndim1,range1,shft1,nshft1;
  int *status=NULL;
  int echo=FALSE;
  double oversample;
  fftw_complex *data;

#ifdef DEBUG
  struct timeval tp;
  double t1,t2;
  int rtn;
#endif

  switch(mode) {
    case MK: /* Mask parameters */
      fn=*val("maskfn",&d->p);
      break;
    default: /* Internally set */
      switch (dataorder) {
        case D1:
          if (d->dimstatus[0] & FFT) return;
          oversample=*val("oversample",&d->p);
          if (oversample == 0.0) oversample=1;
          fn=d->fn*oversample;
          break;
        case D3:
          if (d->dimstatus[2] & FFT) return;
          fn=d->fn2;
          break;
      }
      break;
  } /* end mode switch */

  /* Check that fn is active */
  if (!fn) return;

#ifdef DEBUG
  fprintf(stdout,"\n%s: %s()\n",__FILE__,__FUNCTION__);
  rtn=gettimeofday(&tp, NULL);
  t1=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
  fflush(stdout);
#endif

  /* Set data dimensions */
  switch (dataorder) {
    case D1:
      dim1=d->np/2; dim3=d->fh.ntraces; nr=d->nr;
      status=&d->dimstatus[0];
      if (d->profile) echo=TRUE;
      break;
    case D3:
      dim1=d->nv2;
      dim3=(d->endpos-d->startpos)*d->np/2; nr=d->nr;
      status=&d->dimstatus[2];
      echo=TRUE;
      break;
  }

  /* Make sure fn is exactly divisible by 4 */
  fn /=4; fn *=4;

  /* New data dimension */
  ndim1=fn/2;

  /* Make data the new size and initialise to zero */
  if ((data = (fftw_complex *)fftw_malloc(ndim1*sizeof(fftw_complex))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
  for (k=0;k<ndim1;k++) {
    data[k][0]=0.0;
    data[k][1]=0.0;
  }

  /* Set up for new matrix size to be larger than original */
  range1=dim1;              /* data range */
  shft1=0;                  /* shift required for original data */
  nshft1=abs(ndim1-dim1);   /* shift for new matrix */

  /* Correct new matrix shift if original data is not shifted */
  if (!(*status & SHIFT)) nshft1/=2;

  /* Now allow new matrix size to be smaller than original */
  if (ndim1<dim1) {
    range1=ndim1; shft1=nshft1; nshft1=0; /* reset range and swap shifts */
  }

  if (echo) { /* The data is an echo */

    /* Resize according to whether data is shifted */
    if (*status & SHIFT) { /* dim1 shifted */
      for (i=0;i<nr;i++) {
        for (j=0;j<dim3;j++) {
          /* Copy from d->data[i][j] to data */
          for(k=0;k<range1;k++) {
            ix1=(2*k/range1)*nshft1+k;
            ix2=(2*k/range1)*shft1+k;
            data[ix1][0]=d->data[i][j][ix2][0];
            data[ix1][1]=d->data[i][j][ix2][1];
          }
          /* free d->data[i][j], reallocate and copy data back in its place */
          fftw_free(d->data[i][j]);
          if ((d->data[i][j] = (fftw_complex *)fftw_malloc(ndim1*sizeof(fftw_complex))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
          for (k=0;k<ndim1;k++) {
            d->data[i][j][k][0]=data[k][0];
            d->data[i][j][k][1]=data[k][1];
          }
        }
      }
    } else { /* dim1 not shifted */
      for (i=0;i<nr;i++) {
        for (j=0;j<dim3;j++) {
          /* Copy from d->data[i][j] to data */
          for(k=0;k<range1;k++) {
            ix1=nshft1+k;
            ix2=shft1+k;
            data[ix1][0]=d->data[i][j][ix2][0];
            data[ix1][1]=d->data[i][j][ix2][1];
          }
          /* free d->data[i][j], reallocate and copy data back in its place */
          fftw_free(d->data[i][j]);
          if ((d->data[i][j] = (fftw_complex *)fftw_malloc(ndim1*sizeof(fftw_complex))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
          for (k=0;k<ndim1;k++) {
            d->data[i][j][k][0]=data[k][0];
            d->data[i][j][k][1]=data[k][1];
          }
        }
      }
    }
  }

  else { /* data is not an echo */
    for (i=0;i<nr;i++) {
      for (j=0;j<dim3;j++) {
        /* Copy from d->data[i][j] to data */
        for(k=0;k<range1;k++) {
          data[k][0]=d->data[i][j][k][0];
          data[k][1]=d->data[i][j][k][1];
        }
        /* free d->data[i][j], reallocate and copy data back in its place */
        fftw_free(d->data[i][j]);
        if ((d->data[i][j] = (fftw_complex *)fftw_malloc(ndim1*sizeof(fftw_complex))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
        for (k=0;k<ndim1;k++) {
          d->data[i][j][k][0]=data[k][0];
          d->data[i][j][k][1]=data[k][1];
        }
      }
    }
  }

  /* Free data */
  fftw_free(data);

  /* Set zerofill flag */
  *status+=ZEROFILL;

  /* Update parameters to reflect new data size */
  switch (dataorder) {
    case D1: d->np=fn;    break;
    case D3: d->nv2=fn/2; break;
  }

#ifdef DEBUG
  rtn=gettimeofday(&tp, NULL);
  t2=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
  fprintf(stdout,"  Zero filling to give %d data: took %f secs\n",ndim1,t2-t1);
  fflush(stdout);
#endif

}

void phaseramp1D(struct data *d,int mode)
{
  int dim1,dim3,nr;
  int i,j,k;
  double re,im,M,theta,factor;
  double offset,fov;

#ifdef DEBUG
  struct timeval tp;
  double t1,t2;
  int rtn;
#endif

  /* Data dimensions */
  dim1=d->nv2; dim3=(d->endpos-d->startpos)*d->np/2; nr=d->nr;

  switch (mode) {
    case PHASE2:
      /* Return if there is no phase ramp to apply */
      offset=*val("ppe2",&d->p);
      if (offset == 0.0) return;
#ifdef DEBUG
  fprintf(stdout,"\n%s: %s()\n",__FILE__,__FUNCTION__);
  rtn=gettimeofday(&tp, NULL);
  t1=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
  fflush(stdout);
#endif
      fov=*val("lpe2",&d->p);
      /* Set phase ramp factor to correct for offset */
      factor=-2*M_PI*offset/fov;
      /* Apply phase ramp to generate frequency shift */
      for (i=0;i<nr;i++) {
        for (j=0;j<dim3;j++) {
          for (k=0;k<dim1/2;k++) {
            re=d->data[i][j][k][0];
            im=d->data[i][j][k][1];
            M=sqrt(re*re+im*im);
            theta = atan2(im,re) + factor*(k);
            d->data[i][j][k][0]=M*cos(theta);
            d->data[i][j][k][1]=M*sin(theta);
          }
          for (k=dim1/2;k<dim1;k++) {
            re=d->data[i][j][k][0];
            im=d->data[i][j][k][1];
            M=sqrt(re*re+im*im);
            theta = atan2(im,re) + factor*(k-dim1);
            d->data[i][j][k][0]=M*cos(theta);
            d->data[i][j][k][1]=M*sin(theta);
          }
        }
      }
#ifdef DEBUG
  rtn=gettimeofday(&tp, NULL);
  t2=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
  fprintf(stdout,"  Phase encode phase ramp (%d traces): took %f secs\n",dim3,t2-t1);
  fflush(stdout);
#endif
      break;
  }

}

void phasedata1D(struct data *d,int mode,int dataorder)
{
  int dim1=0,dim3=0,nr=0;
  double rp=0,lp=0;
  double re,im,M,theta;
  double factor;
  int i,j,k;

#ifdef DEBUG
  struct timeval tp;
  double t1,t2;
  int rtn;
#endif

  switch(mode) {
    case VJ: /* Standard VnmrJ parameters */
      if ((!(spar(d,"imRE","y")))
        && (!(spar(d,"imIM","y"))))
        return;
      break;
  } /* end mode switch */

  /* Get phasing parameters */
  switch (dataorder) {
    case D1: rp=*val("rp",&d->p); lp=*val("lp",&d->p); break;
    case D3: rp=0; lp=*val("lp2",&d->p); break;
  }

  /* Return if no phasing is required */
  if (!rp && !lp) return;

#ifdef DEBUG
  fprintf(stdout,"\n%s: %s()\n",__FILE__,__FUNCTION__);
  rtn=gettimeofday(&tp, NULL);
  t1=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
  fflush(stdout);
#endif

  /* Set data dimensions */
  switch (dataorder) {
    case D1: dim1=d->np/2; dim3=d->fh.ntraces; nr=d->nr; break;
    case D3: dim1=d->nv2; dim3=(d->endpos-d->startpos)*d->np/2; nr=d->nr; break;
  }

  /* Phase */
  /* Set factor so that lp adjusts the phase by the specified
     phase in degrees accross the profile */
  factor=DEG2RAD/dim1;
  for (i=0;i<nr;i++) {
    for (j=0;j<dim3;j++) {
      for (k=0;k<dim1/2;k++) {
        re=d->data[i][j][k][0];
        im=d->data[i][j][k][1];
        M=sqrt(re*re+im*im);
        theta = atan2(im,re) + rp*DEG2RAD + lp*factor*k;
        d->data[i][j][k][0]=M*cos(theta);
        d->data[i][j][k][1]=M*sin(theta);
      }
      for (k=dim1/2;k<dim1;k++) {
        re=d->data[i][j][k][0];
        im=d->data[i][j][k][1];
        M=sqrt(re*re+im*im);
        theta = atan2(im,re) + rp*DEG2RAD + lp*factor*(k-dim1);
        d->data[i][j][k][0]=M*cos(theta);
        d->data[i][j][k][1]=M*sin(theta);
      }
    }
  }

#ifdef DEBUG
  rtn=gettimeofday(&tp, NULL);
  t2=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
  fprintf(stdout,"  Took %f secs:\n",t2-t1);
  fflush(stdout);
#endif

}

void getmaxtrace1D(struct data *d,int trace)
{
  int dim1,dim2,dim3,nr;
  double re,im,M;
  int i,j;

#ifdef DEBUG
  struct timeval tp;
  double t1=0,t2=0;
  int rtn;
  if (trace == 0) {
    fprintf(stdout,"\n%s: %s()\n",__FILE__,__FUNCTION__);
    rtn=gettimeofday(&tp, NULL);
    t1=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
    fflush(stdout);
  }
#endif

  /* Data dimensions */
  dim1=d->np/2; dim2=1; dim3=d->fh.ntraces; nr=d->nr;

  /* Set some defaults */
  zeromax(d);

  /* Now get maximum and coordinates */
  for (i=0;i<nr;i++) {
    for (j=0;j<dim1;j++) {
      re=fabs(d->data[i][trace][j][0]);
      im=fabs(d->data[i][trace][j][1]);
      M=re*re+im*im;
      if (M > d->max.Mval) {
        d->max.Mval=M;
        d->max.np=j;
      }
      if (re > d->max.Rval) d->max.Rval=re;
      if (im > d->max.Ival) d->max.Ival=im;
    }
  }
  d->max.Mval=sqrt(d->max.Mval);

  /* Set d->max.Rval = d->max.Ival */
  if (d->max.Rval>d->max.Ival) d->max.Ival=d->max.Rval;
  else d->max.Rval=d->max.Ival;

  /* Set data flag */
  d->max.data=TRUE;

#ifdef DEBUG
  if (trace == 0) {
    rtn=gettimeofday(&tp, NULL);
    t2=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
    fprintf(stdout,"  Took %f secs:\n",t2-t1);
    fprintf(stdout,"  d->max.Mval = %f\n",d->max.Mval);
    fprintf(stdout,"  d->max.Rval = %f\n",d->max.Rval);
    fprintf(stdout,"  d->max.Ival = %f\n",d->max.Ival);
    fprintf(stdout,"  d->max.np = %d\n",d->max.np);
    fprintf(stdout,"  d->max.nv = %d\n",d->max.nv);
    fprintf(stdout,"  d->max.nv2 = %d\n",d->max.nv2);
    fflush(stdout);
  }
#endif

}
