/* dproc3D.c */
/*---------------------------------------------------------------------------*/
/*                                                                           */
/* dproc3D.c: 3D Data processing routines                                    */
/*                                                                           */
/* Copyright (C) 2012 Paul Kinchesh                                          */
/*               2012 Margaret Kritzer                                       */
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

void dimorder3D(struct data *d)
{
  /* Fill dim2order with the nv phase encode order */
  d->dim2order=phaseorder(d,d->nv,d->nseg*d->etl,"pelist");

  /* Fill dim3order with the nv2 phase encode order */
  d->dim3order=phaseorder(d,d->nv2,d->nv2,"pe2list");

  /* Fill pssorder with the slice order */
  d->pssorder=sliceorder(d,d->ns,"pss");

}

void getblock3D(struct data *d,int volindex,int DCCflag)
{
  /* Check d->nvols has been set */
  if (d->nvols == 0) setnvols(d); /* Set the number of data volumes */

  /* Set start and end position of block */
  setblock(d,d->nv2);

  /* Get the block */
  getblock(d,volindex,DCCflag);

}

void getnavblock3D(struct data *d,int volindex,int DCCflag)
{
  /* Check d->nvols has been set */
  if (d->nvols == 0) setnvols(d); /* Set the number of data volumes */

  /* Set start and end position of block */
  setblock(d,d->nv2);

  /* Get the block */
  getnavblock(d,volindex,DCCflag);

}

void shiftdatadim3(struct data *d,int dataorder,int mode)
{
  int dim1,dim2,dim3, dim4;
  int dim3shft=0;

  /* Data dimensions */
  dim1=d->np/2; dim2=d->nv; dim3=d->nv2; dim4=d->nv3;

  /* Set shift points */
  switch(mode) {
     case SPATIAL:
      if (d->dimstatus[3] & SHIFT) {
        dim3shft=(int)ceil(dim4/2.0);
        d->dimstatus[3]-=SHIFT;
      } else {
        dim3shft=(int)floor(dim4/2.0);
        d->dimstatus[3]+=SHIFT;
      }
      break;
    case STD:
      if (d->dimstatus[2] & SHIFT) {
        dim3shft=(int)ceil(dim3/2.0);
        d->dimstatus[2]-=SHIFT;
      } else {
        dim3shft=(int)floor(dim3/2.0);
        d->dimstatus[2]+=SHIFT;
      }
#ifdef DEBUG
  fprintf(stdout,"\n%s: %s()\n",__FILE__,__FUNCTION__);
  fprintf(stdout,"  Standard shift: dim3shft = %d\n",dim3shft);
  fflush(stdout);
#endif
      break;
    default:
      /* Invalid mode */
      fprintf(stderr,"\n%s: %s()\n",__FILE__,__FUNCTION__);
      fprintf(stderr,"  Invalid 2nd argument %s(*,'type')\n",__FUNCTION__);
      fflush(stderr);
      return;
      break;
  } /* end mode switch */

  /* Shift the data */
  switch (dataorder) {
    case D12:
    	if(mode == SPATIAL)
    		shiftdim3dataCSI(d,dim3shft);
    	else
    		shiftdim3data(d,dim3shft);
    	break;
    case D3:  shift1Ddata(d,dim3shft,D3); break;
  }

}

void shiftdim3data(struct data *d,int dim3shft)
{
  int dim3,nr;
  fftw_complex ***data;
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

  /* Set dim3 and receivers */
  dim3=d->nv2; nr=d->nr;

  /* Allocate memory for some data pointers to dim3 */
  if ((data = (fftw_complex ***)fftw_malloc(nr*sizeof(fftw_complex **))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
  for (i=0;i<nr;i++) { /* loop over receiver blocks */
    if ((data[i] = (fftw_complex **)fftw_malloc(dim3*sizeof(fftw_complex *))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
    for (j=0;j<dim3;j++) /* loop over dim3 */
      if ((data[i][j] = (fftw_complex *)fftw_malloc(sizeof(fftw_complex))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
  }

  /* Set new pointers to data in requested order */
  for (j=0;j<dim3-dim3shft;j++)
    for (i=0;i<nr;i++)
    {
    	data[i][j]=d->data[i][j+dim3shft];
    	if(interupt)return;
    }
  for (j=dim3-dim3shft;j<dim3;j++)
    for (i=0;i<nr;i++)
    {
      data[i][j]=d->data[i][j+dim3shft-dim3];
      if(interupt)return;
    }

  /* Reset original data pointers in requested order */
  for (i=0;i<nr;i++)
    for (j=0;j<dim3;j++)
      d->data[i][j]=data[i][j];

  fftw_free(data);

#ifdef DEBUG
  rtn=gettimeofday(&tp, NULL);
  t2=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
  fprintf(stdout,"  Took %f secs\n",t2-t1);
  fflush(stdout);
#endif
}

void weightdatadim3(struct data *d,int mode)
{
  int i,j,k,l,jj;
  int ix1;
  double f;
  int dim0=0,dim1,dim2,dim3,nr;
  double lb2,gf2,sb2,shft=0;
  double *weight;
  double wt;

#ifdef DEBUG
  struct timeval tp;
  double t1,t2;
  int rtn;
#endif

  /* Return if frequency domain data */
  if (d->dimstatus[2] & FFT) return;



  switch(mode) {
    case MK: /* Mask parameters */
      if (spar(d,"maskwlb2","y")) /* Lorentzian */
        lb2=*val("masklb2",&d->p);
      else
        lb2=0.0;
      if (spar(d,"maskwgf2","y")) /* Gaussian */
        gf2=*val("maskgf2",&d->p); 
      else
        gf2=0.0;
      if (spar(d,"maskwsb2","y")) /* Sinebell */
        sb2=*val("masksb2",&d->p);
      else
        sb2=0.0;
      break;
    case SM: /* Sensitivity map parameters */
      if (spar(d,"smapwlb2","y")) /* Lorentzian */
        lb2=*val("smaplb2",&d->p);
      else
        lb2=0.0;
      if (spar(d,"smapwgf2","y")) /* Gaussian */
        gf2=*val("smapgf2",&d->p);
      else
        gf2=0.0;
      if (spar(d,"smapwsb2","y")) /* Sinebell */
        sb2=*val("smapsb2",&d->p);
      else
        sb2=0.0;
      break;
    case SPATIAL:
    default: /* Standard parameters */
      lb2=*val("lb2",&d->p);
      gf2=*val("gf2",&d->p);
      sb2=*val("sb2",&d->p);
/*
      at=*val("at",&d->p);
*/
  } /* end mode switch */

  /* Return if no weighting is active */
  if (!lb2 && !gf2 && !sb2) return;

#ifdef DEBUG
  fprintf(stdout,"\n%s: %s()\n",__FILE__,__FUNCTION__);
  rtn=gettimeofday(&tp, NULL);
  t1=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
  fflush(stdout);
#endif

  /* Data dimensions */
  dim1=d->np/2; dim2=d->nv; dim3=d->endpos-d->startpos; nr=d->nr;

  if(mode==SPATIAL)
   {
 		  dim1=d->nv; dim2=d->nv2; dim3=d->nv3;
   }

  /* Set shft according to whether data is shifted */
  if (d->dimstatus[2] & SHIFT) shft=0;
  else shft=dim3/2-1;

  /* Calculate weighting */
  if ((weight = (double *)malloc(dim3*sizeof(double))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
  for(j=0;j<dim3;j++) weight[j]=1.0;

  /* Lorentzian line broadening */
  if (lb2) { /* lb2 is active */
    for (j=0;j<dim3/2;j++) {
      /* Lorentzian broadening as fraction of FOV */
      f=exp(-abs(j-shft)*M_PI*lb2);
      /* Lorentzian broadening in # pixels */
      /* f=exp(-j*M_PI*lb2/dim3); */
      /* Lorentzian broadening in Hz (as VnmrJ) */
      /* f=exp(-j*at*M_PI*lb2/dim3); */
      weight[j] *=f;
      weight[dim3-1-j] *=f;
    }
  }

  /* Gaussian line broadening */
  if (gf2) { /* gf2 is active */
    for (j=0;j<dim3/2;j++) {
      /* Gaussian broadening as fraction of FOV */
      f=(j-shft)*M_PI*gf2;
      f=exp(-f*f);
      weight[j] *=f;
      weight[dim3-1-j] *=f;
    }
  }

  /* Sinebell line broadening */
  if (sb2) { /* sb2 is active */
    for (j=0;j<dim3/2;j++) {
      /* Sinebell broadening as fraction of FOV */
      f=(j-shft)*M_PI*sb2;
      if (FP_LT(fabs(f),M_PI/2))
        f=cos(f);
      else
        f=0.0;
      weight[j] *=f;
      weight[dim3-1-j] *=f;
    }
  }

  /* Weight the data */
  if (mode == SPATIAL) {
		for (i = 0; i < nr; i++) {
			for (j = 0; j < dim3; j++) {
				wt=weight[j];
				jj = j * dim2 * dim1 * dim0;
				for (k = 0; k < dim2 * dim1 * dim0; k++) {
					d->csi_data[i][jj + k][0] *= wt;
					d->csi_data[i][jj + k][1] *= wt;
				}
			}
		}
  }
  else {
		for (i = 0; i < nr; i++) {
			for (j = 0; j < dim3; j++) {
				for (k = 0; k < dim2; k++) {
					for (l = 0; l < dim1; l++) {
						ix1 = k * dim1 + l;
						d->data[i][j][ix1][0] *= weight[j];
						d->data[i][j][ix1][1] *= weight[j];
						if(interupt)return;
					}
				}
			}
		}
	}

  free(weight);

#ifdef DEBUG
  rtn=gettimeofday(&tp, NULL);
  t2=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
  if (lb2) fprintf(stdout,"  lb2 weighting = %f\n",lb2);
  if (gf2) fprintf(stdout,"  gf2 weighting = %f\n",gf2);
  if (sb2) fprintf(stdout,"  sb2 weighting = %f\n",sb2);
  fprintf(stdout,"  Took %f secs\n",t2-t1);
  fflush(stdout);
#endif
}

void zerofilldim3(struct data *d,int mode)
{
  int dim1,dim2,dim3,nr;
  int ix1,ix2;
  int fn2=0,ndim3,range3,shft3,nshft3;
  fftw_complex **data;
  int i,j,k;

#ifdef DEBUG
  struct timeval tp;
  double t1,t2;
  int rtn;
#endif

  /* Return if frequency domain data */
  if (d->dimstatus[2] & FFT) return;

  switch(mode) {
    case MK: /* Mask parameters */
      fn2=*val("maskfn2",&d->p);
      break;
    default: /* Internally set */
      fn2=d->fn2;
  } /* end mode switch */

  /* Check that fn2 is active */
  /* If so, make sure it is exactly divisible by 4 */
  if (!fn2) return;
  fn2 /=4; fn2 *=4;

#ifdef DEBUG
  fprintf(stdout,"\n%s: %s()\n",__FILE__,__FUNCTION__);
  rtn=gettimeofday(&tp, NULL);
  t1=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
  fflush(stdout);
#endif

  /* New data dimension */
  ndim3=fn2/2;

  /* Initial data dimensions */
  dim1=d->np/2; dim2=d->nv; dim3=d->nv2; nr=d->nr;

  /* Set up for new matrix size to be larger than original */
  range3=dim3;              /* data range */
  shft3=0;                  /* shift required for original data */
  nshft3=abs(ndim3-dim3);   /* shift for new matrix */

  /* Correct new matrix shift if original data is not shifted */
  if (!(d->dimstatus[2] & SHIFT)) nshft3/=2;

  /* Now allow new matrix size to be smaller than original */
  if (ndim3<dim3) {
    range3=ndim3; shft3=nshft3; nshft3=0; /* reset range and swap shifts */
  }

  /* Allocate memory for data */
  if ((data = (fftw_complex **)fftw_malloc(dim3*sizeof(fftw_complex *))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
  for (j=0;j<dim3;j++) /* loop over dim3 */
    if ((data[j] = (fftw_complex *)fftw_malloc(dim2*dim1*sizeof(fftw_complex))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);

  if (d->dimstatus[2] & SHIFT) { /* dim3 shifted */
    for (i=0;i<nr;i++) {
      /* Copy d->data[i] to data */
      for (j=0;j<dim3;j++) { /* loop over dim3 */
        for(k=0;k<dim2*dim1;k++) {
          data[j][k][0]=d->data[i][j][k][0];
          data[j][k][1]=d->data[i][j][k][1];
        }
      }
      fftw_free(d->data[i]);
      if ((d->data[i] = (fftw_complex **)fftw_malloc(ndim3*sizeof(fftw_complex *))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
      for (j=0;j<ndim3;j++) /* loop over new dim3 */
        if ((d->data[i][j] = (fftw_complex *)fftw_malloc(dim2*dim1*sizeof(fftw_complex))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
      /* Zero d->data[i] appropriately */
      for (j=0;j<ndim3;j++) {
        for (k=0;k<dim2*dim1;k++) {
          d->data[i][j][k][0]=0.0;
          d->data[i][j][k][1]=0.0;
        }
      }
      /* Fill d->data[i] appropriately */
      for (j=0;j<range3;j++) {
        ix1=(2*j/range3)*nshft3+j;
        ix2=(2*j/range3)*shft3+j;
        for (k=0;k<dim2*dim1;k++) {
          d->data[i][ix1][k][0]=data[ix2][k][0];
          d->data[i][ix1][k][1]=data[ix2][k][1];
          if(interupt)return;
        }
      }
    }
  } else { /* dim3 not shifted */
    for (i=0;i<nr;i++) {
      /* Copy d->data[i] to data */
      for (j=0;j<dim3;j++) { /* loop over dim3 */
        for(k=0;k<dim2*dim1;k++) {
          data[j][k][0]=d->data[i][j][k][0];
          data[j][k][1]=d->data[i][j][k][1];
          if(interupt)return;
        }
      }
      fftw_free(d->data[i]);
      if ((d->data[i] = (fftw_complex **)fftw_malloc(ndim3*sizeof(fftw_complex *))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
      for (j=0;j<ndim3;j++) /* loop over new dim3 */
        if ((d->data[i][j] = (fftw_complex *)fftw_malloc(dim2*dim1*sizeof(fftw_complex))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
      /* Zero d->data[i] appropriately */
      for (j=0;j<ndim3;j++) {
        for (k=0;k<dim2*dim1;k++) {
          d->data[i][j][k][0]=0.0;
          d->data[i][j][k][1]=0.0;
        }
      }
      /* Fill d->data[i] appropriately */
      for (j=0;j<range3;j++) {
        ix1=nshft3+j;
        ix2=shft3+j;
        for (k=0;k<dim2*dim1;k++) {
          d->data[i][ix1][k][0]=data[ix2][k][0];
          d->data[i][ix1][k][1]=data[ix2][k][1];
        }
      }
    }
  }

  /* Free data memory */
  for (j=0;j<dim3;j++) fftw_free(data[j]);
  fftw_free(data);

  /* Set zerofill flag */
  d->dimstatus[2]+=ZEROFILL;

  /* Update parameters to reflect new data size */
  d->nv2=ndim3;
  d->startpos=0;
  d->endpos=ndim3;

#ifdef DEBUG
  rtn=gettimeofday(&tp, NULL);
  t2=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
  fprintf(stdout,"  Zero filling dim3 to %d: took %f secs\n",ndim3,t2-t1);
  fflush(stdout);
#endif

}

void fftdim3(struct data *d)
{
  fftw_complex *data;
  fftw_plan p;
  int dim1,dim2,dim3,nr;
  int slicesize, offset;
  int i,j,k;

#ifdef DEBUG
  struct timeval tp;
  double t1,t2;
  int rtn;
  fprintf(stdout,"\n%s: %s()\n",__FILE__,__FUNCTION__);
  fprintf(stdout,"  Starting 1D FT in dim3 ...\n");
  rtn=gettimeofday(&tp, NULL);
  t1=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
  fflush(stdout);
#endif

  /* Data dimensions */
  dim1=d->np/2; dim2=d->nv; dim3=d->nv2; nr=d->nr;
  slicesize=dim1*dim2;
  if(imCSI(d))
  {
	  slicesize *= dim3;
	  dim3 = d->nv3;
  }

  /* Allocate memory for a dim3 trace */
  if ((data = (fftw_complex *)fftw_malloc(dim3*sizeof(fftw_complex))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);

  /* Measured plan for 1D FT */
  p=fftw_plan_dft_1d(dim3,data,data,FFTW_FORWARD,FFTW_MEASURE);

  /* FT  dim3 ... */
	if (imCSI(d)) {
		for (i = 0; i < nr; i++) {
			for (k = 0; k < slicesize; k++) {
				offset=0;
				for (j = 0; j < dim3; j++) {
					data[j][0] = d->csi_data[i][offset+k][0];
					data[j][1] = d->csi_data[i][offset+k][1];
					offset += slicesize;
				}
				/* ft */
				fftw_execute(p);
				offset=0;
				for (j = 0; j < dim3; j++) {
					d->csi_data[i][offset+k][0] = data[j][0];
					d->csi_data[i][offset+k][1] = data[j][1];
					offset += slicesize;
					if(interupt){
						 fftw_destroy_plan(p);
						  fftw_free(data);
						return;
					}
				}
			}
		}
		/* Set status for dimension */
		d->dimstatus[3] += FFT;
		if (d->dimstatus[3] & SHIFT)
			d->dimstatus[3] -= SHIFT;
	}
	else {
		for (i = 0; i < nr; i++) {
			for (k = 0; k < slicesize; k++) {
				for (j = 0; j < dim3; j++) {
					data[j][0] = d->data[i][j][k][0];
					data[j][1] = d->data[i][j][k][1];
					if(interupt)
						{
						 fftw_destroy_plan(p);
						  fftw_free(data);
						return;
						}
				}
				/* ft */
				fftw_execute(p);
				for (j = 0; j < dim3; j++) {
					d->data[i][j][k][0] = data[j][0];
					d->data[i][j][k][1] = data[j][1];
				}
			}
		}
		/* Set status for dimension */
		d->dimstatus[2] += FFT;
		if (d->dimstatus[2] & SHIFT)
			d->dimstatus[2] -= SHIFT;
	}


  /* ... and tidy up */
  fftw_destroy_plan(p);
  fftw_free(data);


#ifdef DEBUG
  rtn=gettimeofday(&tp, NULL);
  t2=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
  fprintf(stdout,"\n%s: %s()\n",__FILE__,__FUNCTION__);
  fprintf(stdout,"  1D FT of %d x %d x %d data: took %f secs\n",dim1,dim2,dim3,t2-t1);
  fflush(stdout);
#endif

}

void phaserampdim3(struct data *d,int mode)
{
  int dim1,dim2,dim3,nr;
  int i,j,k,l,ix;
  double re,im,M,theta,factor;
  double offset,fov;

#ifdef DEBUG
  struct timeval tp;
  double t1,t2;
  int rtn;
#endif

  /* Data dimensions */
  dim1=d->np/2; dim2=d->nv; dim3=d->endpos-d->startpos; nr=d->nr;

  switch (mode) {
    case PHASE2:
      /* This should only be used if a 3D is processed in a single data block */
      if (dim3 != d->nv2) return;
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
        for (j=0;j<dim3/2;j++) {
          for (k=0;k<dim2;k++) {
            ix = k*dim1;
            for (l=0;l<dim1;l++) {
              re=d->data[i][j][ix+l][0];
              im=d->data[i][j][ix+l][1];
              M=sqrt(re*re+im*im);
              theta = atan2(im,re) + factor*(j);
              d->data[i][j][ix+l][0]=M*cos(theta);
              d->data[i][j][ix+l][1]=M*sin(theta);
            }
          }
        }
        for (j=dim3/2;j<dim3;j++) {
          for (k=0;k<dim2;k++) {
            ix = k*dim1;
            for (l=0;l<dim1;l++) {
              re=d->data[i][j][ix+l][0];
              im=d->data[i][j][ix+l][1];
              M=sqrt(re*re+im*im);
              theta = atan2(im,re) + factor*(j-dim3);
              d->data[i][j][ix+l][0]=M*cos(theta);
              d->data[i][j][ix+l][1]=M*sin(theta);
            }
          }
        }
      }
#ifdef DEBUG
  rtn=gettimeofday(&tp, NULL);
  t2=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
  fprintf(stdout,"  Phase encode phase ramp (%d traces): took %f secs\n",dim1,t2-t1);
  fflush(stdout);
#endif
      break;
  }

}

void phasedatadim3(struct data *d,int mode)
{
  int dim1,dim2,dim3,nr;
  double lp2;
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
  lp2=*val("lp2",&d->p);

  /* Return if no phasing is required */
  if (!lp2) return;

#ifdef DEBUG
  fprintf(stdout,"\n%s: %s()\n",__FILE__,__FUNCTION__);
  rtn=gettimeofday(&tp, NULL);
  t1=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
  fflush(stdout);
#endif

  /* Data dimensions */
  dim1=d->np/2; dim2=d->nv; dim3=d->endpos-d->startpos; nr=d->nr;

  /* Phase */
  /* Set factor so that lp2 adjusts the phase by the specified
     phase in degrees accross the image */
  factor=DEG2RAD/dim3;
  for (i=0;i<nr;i++) {
    for (j=0;j<dim3/2;j++) {
      for(k=0;k<dim2*dim1;k++) {
        re=d->data[i][j][k][0];
        im=d->data[i][j][k][1];
        M=sqrt(re*re+im*im);
        theta = atan2(im,re) + lp2*factor*j;
        d->data[i][j][k][0]=M*cos(theta);
        d->data[i][j][k][1]=M*sin(theta);
      }
    }
    for (j=dim3/2;j<dim3;j++) {
      for(k=0;k<dim2*dim1;k++) {
        re=d->data[i][j][k][0];
        im=d->data[i][j][k][1];
        M=sqrt(re*re+im*im);
        theta = atan2(im,re) + lp2*factor*(j-dim3);
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
