/* dproc2D.c */
/*---------------------------------------------------------------------------*/
/*                                                                           */
/* dproc2D.c: 2D Data processing routines                                    */
/*                                                                           */
/* Copyright (C) 2012 Paul Kinchesh                                          */
/*               2012 Martyn Klassen                                         */
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

void dimorder2D(struct data *d)
{
  /* Fill dim2order with the nv phase encode order */
  d->dim2order=phaseorder(d,d->nv,d->nseg*d->etl,"pelist");

  /* Fill pssorder with the slice order */
  d->pssorder=sliceorder(d,d->ns,"pss");

}

void fft2D(struct data *d,int mode)
{
  fftw_complex *slice;
  fftw_plan p=NULL;
  int dim1,dim2,dim3,nr;
  int i,j;
  int dims[2];
  int stride, dist, howmany;
  int offset;

#ifdef DEBUG
  struct timeval tp;
  double t1,t2;
  int rtn;
  fprintf(stdout,"\n%s: %s()\n",__FILE__,__FUNCTION__);
  rtn=gettimeofday(&tp, NULL);
  t1=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
  fflush(stdout);
#endif

  /* Data dimensions */
  dim1=d->np/2; dim2=d->nv; dim3=d->endpos-d->startpos; nr=d->nr;

  /*
    fftw_import_wisdom_from_file(FILE *input_file);
  */
  /* Allocate memory for slice data */
  if(mode == SPATIAL)
    {
	  dim3=d->nv2;
     if ((slice = (fftw_complex *)fftw_malloc(dim3*dim2*dim1*sizeof(fftw_complex))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
     // csi_slice = d->csi_data[0];
     nr=1; // channels combined
    }
  else
    {
      if ((slice = (fftw_complex *)fftw_malloc(dim2*dim1*sizeof(fftw_complex))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
    }


  switch(mode) {
    case STD:
      /* Measured plan for 2D FT */
      p=fftw_plan_dft_2d(dim2,dim1,slice,slice,FFTW_FORWARD,FFTW_MEASURE);
      /* Set status for dimensions */
      for (i=0;i<2;i++) {
        d->dimstatus[i]+=FFT;
        if (d->dimstatus[i] & SHIFT) d->dimstatus[i]-=SHIFT;
      }
#ifdef DEBUG
  fprintf(stdout,"  2D FT (%d x %d): %d slice(s), %d receiver(s): ",dim2,dim1,dim3,nr);
  fflush(stdout);
#endif
      break;
    case READ:
      /* Measured plan for 1D FT in READ */
      p=fftw_plan_many_dft(1,&dim1,dim2,slice,NULL,1,dim1,slice,NULL,1,dim1,FFTW_FORWARD,FFTW_MEASURE);
      /* Set status for dimension */
      d->dimstatus[0]+=FFT;
      if (d->dimstatus[0] & SHIFT) d->dimstatus[0]-=SHIFT;
#ifdef DEBUG
  fprintf(stdout,"  dim1 FT (%d): %d trace(s), %d slice(s), %d receiver(s): ",dim1,dim2,dim3,nr);
  fflush(stdout);
#endif
      break;
    case PHASE:
      /* Measured plan for 1D FT in PHASE */
      p=fftw_plan_many_dft(1,&dim2,dim1,slice,NULL,dim1,1,slice,NULL,dim1,1,FFTW_FORWARD,FFTW_MEASURE);
      /* Set status for dimension */
      d->dimstatus[1]+=FFT;
      if (d->dimstatus[1] & SHIFT) d->dimstatus[1]-=SHIFT;
#ifdef DEBUG
  fprintf(stdout,"  dim2 FT (%d): %d trace(s), %d slice(s), %d receiver(s): ",dim2,dim1,dim3,nr);
  fflush(stdout);
#endif
      break;
    case SPATIAL:
      /* Measured plan for 2D FT in PHASE */
      dims[0]=dim3; dims[1]=dim2;
      stride=dim1;   // step from point to point within FT
      dist=1; // step from FT to FT
      howmany=dim1; // how many FTs
      p=fftw_plan_many_dft(2,dims,howmany,slice,NULL,stride,dist,slice,NULL,stride,dist,FFTW_FORWARD,FFTW_MEASURE);
      /* Set status for dimension */
      d->dimstatus[1]+=FFT;
      d->dimstatus[2]+=FFT;
      if (d->dimstatus[1] & SHIFT) d->dimstatus[1]-=SHIFT;
	  offset=0;
	  if(spar(d,"apptype","im3Dcsi"))
		  offset=d->vol*dim1*dim2*dim3;

      /* Do the planned FT ... */
       for (i=0;i<nr;i++) {
    		fftw_execute_dft(p,&d->csi_data[i][offset],&d->csi_data[i][offset]);
         }



      nr=0; //skip FT step at end
#ifdef DEBUG
  fprintf(stdout,"  CSI 2D FT (%d): %d trace(s), %d slice(s), %d receiver(s): ",dim2,dim1,dim3,nr);
  fflush(stdout);
#endif
      break;
  }

  /* Do the planned FT ... */
  for (i=0;i<nr;i++) {
    for (j=0;j<dim3;j++) {
    	if(interupt)
    		{
    		  fftw_destroy_plan(p);
    		  fftw_free(slice);
    		return;
    		}
    	fftw_execute_dft(p,d->data[i][j],d->data[i][j]);
    }
  }
  /* ... and tidy up */
  fftw_destroy_plan(p);
  fftw_free(slice);

#ifdef DEBUG
  rtn=gettimeofday(&tp, NULL);
  t2=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
  fprintf(stdout,"took %f secs\n",t2-t1);
  fflush(stdout);
#endif

}

void ifft2D(struct data *d,int mode)
{
  fftw_complex *slice;
  fftw_plan p=NULL;
  int dim1,dim2,dim3,nr;
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

  /* Data dimensions */
  dim1=d->np/2; dim2=d->nv; dim3=d->endpos-d->startpos; nr=d->nr;

  /*
    fftw_import_wisdom_from_file(FILE *input_file);
  */
  /* Allocate memory for slice data */
  if ((slice = (fftw_complex *)fftw_malloc(dim2*dim1*sizeof(fftw_complex))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);

  switch(mode) {
    case STD:
      /* Measured plan for 2D inverse FT */
      p=fftw_plan_dft_2d(dim2,dim1,slice,slice,FFTW_BACKWARD,FFTW_MEASURE);
      /* Set status for dimensions */
      for (i=0;i<2;i++) {
        if (d->dimstatus[i] & FFT) d->dimstatus[i]-=FFT;
        if (!(d->dimstatus[i] & SHIFT)) d->dimstatus[i]+=SHIFT;
      }
#ifdef DEBUG
  fprintf(stdout,"  2D inverse FT (%d x %d): %d slice(s), %d receiver(s): ",dim2,dim1,dim3,nr);
  fflush(stdout);
#endif
      break;
    case READ:
      /* Measured plan for 1D inverse FT in READ */
      p=fftw_plan_many_dft(1,&dim1,dim2,slice,NULL,1,dim1,slice,NULL,1,dim1,FFTW_BACKWARD,FFTW_MEASURE);
      /* Set status for dimension */
      if (d->dimstatus[0] & FFT) d->dimstatus[0]-=FFT;
      if (!(d->dimstatus[0] & SHIFT)) d->dimstatus[0]+=SHIFT;
#ifdef DEBUG
  fprintf(stdout,"  dim1 inverse FT (%d): %d trace(s), %d slice(s), %d receiver(s): ",dim1,dim2,dim3,nr);
  fflush(stdout);
#endif
      break;
    case PHASE:
      /* Measured plan for 1D inverse FT in PHASE */
      p=fftw_plan_many_dft(1,&dim2,dim1,slice,NULL,dim1,1,slice,NULL,dim1,1,FFTW_BACKWARD,FFTW_MEASURE);
      /* Set status for dimension */
      if (d->dimstatus[1] & FFT) d->dimstatus[1]-=FFT;
      if (!(d->dimstatus[1] & SHIFT)) d->dimstatus[1]+=SHIFT;
#ifdef DEBUG
  fprintf(stdout,"  dim2 inverse FT (%d): %d trace(s), %d slice(s), %d receiver(s): ",dim2,dim1,dim3,nr);
  fflush(stdout);
#endif
      break;
  }

  /* Do the planned inverse FT ... */
  for (i=0;i<nr;i++) {
    for (j=0;j<dim3;j++) {
      fftw_execute_dft(p,d->data[i][j],d->data[i][j]);
    }
  }
  /* ... and tidy up */
  fftw_destroy_plan(p);
  fftw_free(slice);

#ifdef DEBUG
  rtn=gettimeofday(&tp, NULL);
  t2=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
  fprintf(stdout,"took %f secs\n",t2-t1);
  fflush(stdout);
#endif

}

void shiftdata2D(struct data *d,int mode)
{
  int dim1,dim2, dim3;
  int npshft=0,nvshft=0, nv2shft=0;

  /* Data dimensions */
  dim1=d->np/2; dim2=d->nv; dim3=d->nv2;

  /* Set shift points */
  switch(mode) {
    case STD:
      if (d->dimstatus[0] & SHIFT) {
        npshft=(int)ceil(dim1/2.0);
        d->dimstatus[0]-=SHIFT;
      } else {
        npshft=(int)floor(dim1/2.0);
        d->dimstatus[0]+=SHIFT;
      }
      if (d->dimstatus[1] & SHIFT) {
        nvshft=(int)ceil(dim2/2.0);
        d->dimstatus[1]-=SHIFT;
      } else {
        nvshft=(int)floor(dim2/2.0);
        d->dimstatus[1]+=SHIFT;
      }
#ifdef DEBUG
  fprintf(stdout,"\n%s: %s()\n",__FILE__,__FUNCTION__);
  fprintf(stdout,"  Standard shift: npshft = %d, nvshft = %d\n",npshft,nvshft);
  fflush(stdout);
#endif
      break;
    case READ:
      if (d->dimstatus[0] & SHIFT) {
        npshft=(int)ceil(dim1/2.0);
        d->dimstatus[0]-=SHIFT;
      } else {
        npshft=(int)floor(dim1/2.0);
        d->dimstatus[0]+=SHIFT;
      }
#ifdef DEBUG
  fprintf(stdout,"\n%s: %s()\n",__FILE__,__FUNCTION__);
  fprintf(stdout,"  Shift in read: npshft = %d\n",npshft);
  fflush(stdout);
#endif
      break;
    case PHASE:
      if (d->dimstatus[1] & SHIFT) {
        nvshft=(int)ceil(dim2/2.0);
        d->dimstatus[1]-=SHIFT;
      } else {
        nvshft=(int)floor(dim2/2.0);
        d->dimstatus[1]+=SHIFT;
      }
#ifdef DEBUG
  fprintf(stdout,"\n%s: %s()\n",__FILE__,__FUNCTION__);
  fprintf(stdout,"  Shift in phase: nvshft = %d\n",nvshft);
  fflush(stdout);
#endif
      break;
    case SPATIAL:
      if (d->dimstatus[1] & SHIFT) {
        nvshft=(int)ceil(dim2/2.0);
        d->dimstatus[1]-=SHIFT;
      } else {
        nvshft=(int)floor(dim2/2.0);
        d->dimstatus[1]+=SHIFT;
      }
      if (d->dimstatus[2] & SHIFT) {
        nv2shft=(int)ceil(dim3/2.0);
        d->dimstatus[2]-=SHIFT;
      } else {
        nv2shft=(int)floor(dim3/2.0);
        d->dimstatus[2]+=SHIFT;
      }
#ifdef DEBUG
  fprintf(stdout,"\n%s: %s()\n",__FILE__,__FUNCTION__);
  fprintf(stdout,"  Standard shift: npshft = %d, nvshft = %d\n",npshft,nvshft);
  fflush(stdout);
#endif
  shift2DCSIdata(d,nvshft,nv2shft);
  break;

    default:
      /* Invalid mode */
      fprintf(stderr,"\n%s: %s()\n",__FILE__,__FUNCTION__);
      fprintf(stderr,"  Invalid 2nd argument %s(*,'mode')\n",__FUNCTION__);
      fflush(stderr);
      return;
      break;
  } /* end mode switch */

  /* Shift the data */
  if(mode != SPATIAL)shift2Ddata(d,npshft,nvshft);
}

void shift2Ddata(struct data *d,int npshft,int nvshft)
{
  fftw_complex *slice;
  int dim1,dim2,dim3,nr;
  int i,j,k,l;

#ifdef DEBUG
  struct timeval tp;
  double t1,t2;
  int rtn;
  fprintf(stdout,"\n%s: %s()\n",__FILE__,__FUNCTION__);
  fprintf(stdout,"  npshft = %d, nvshft = %d: ",npshft,nvshft);
  rtn=gettimeofday(&tp, NULL);
  t1=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
  fflush(stdout);
#endif

  /* Data dimensions */
  dim1=d->np/2; dim2=d->nv; dim3=d->endpos-d->startpos; nr=d->nr;

  /* Make sure shifts are in range */
  nvshft = nvshft % dim2;
  npshft = npshft % dim1;

  /* Return if shift not required */
  if ((nvshft == 0) && (npshft == 0)) return;

  /* Allocate memory for slice data */
  if ((slice = (fftw_complex *)fftw_malloc(dim2*dim1*sizeof(fftw_complex))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);

  /* Loop over receivers and slices */
  for (i=0;i<nr;i++) {
    for (j=0;j<dim3;j++) {
      /* Fill slice with shifted data */
      for (k=0;k<dim2;k++) {
        for (l=0;l<dim1;l++) {
          slice[((k+nvshft+dim2)%dim2)*dim1+(l+npshft+dim1)%dim1][0]=d->data[i][j][k*dim1+l][0];
          slice[((k+nvshft+dim2)%dim2)*dim1+(l+npshft+dim1)%dim1][1]=d->data[i][j][k*dim1+l][1];
        }
      }
      /* Copy shifted data back to d->data */
      for (k=0;k<dim2*dim1;k++) {
        d->data[i][j][k][0]=slice[k][0];
        d->data[i][j][k][1]=slice[k][1];
      }
    }
  }

  fftw_free(slice);

#ifdef DEBUG
  rtn=gettimeofday(&tp, NULL);
  t2=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
  fprintf(stdout,"took %f secs\n",t2-t1);
  fflush(stdout);
#endif

}

void shift2DCSIdata(struct data *d,int nvshft,int nv2shft)
{
  fftw_complex *slice;
  int dim1,dim2,dim3,nr;
  int i,j,k,l;
  int jj, kk;
  int oj,ok;
  int offset;


#ifdef DEBUG
  struct timeval tp;
  double t1,t2;
  int rtn;
  fprintf(stdout,"\n%s: %s()\n",__FILE__,__FUNCTION__);
  fprintf(stdout,"  nvshft = %d, nv2shft = %d: ",nvshft,nv2shft);
  rtn=gettimeofday(&tp, NULL);
  t1=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
  fflush(stdout);
#endif

  /* Data dimensions */
  dim1=d->np/2; dim2=d->nv; dim3=d->nv2; nr=d->nr;

  nr=1; // channels are combined into 0th channel

  /* Make sure shifts are in range */
  nvshft = nvshft % dim2;
  nv2shft = nv2shft % dim3;

  /* Return if shift not required */
  if ((nvshft == 0) && (nv2shft == 0)) return;

  offset=0;
  if(spar(d,"apptype","im3Dcsi"))
	  offset = d->vol * dim1*dim2*dim3;

  /* Allocate memory for slice data */
  if ((slice = (fftw_complex *)fftw_malloc(dim3*dim2*dim1*sizeof(fftw_complex))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);

  /* Loop over receivers for single slice */
  /* Loop over receivers and slices */
   for (i=0;i<nr;i++) {
       /* Fill slice with shifted data */
	   for (j=0;j<dim3;j++) {
		   jj=((j+nv2shft+dim3)%dim3)*dim2*dim1;
		   oj=j*dim2*dim1;
		   oj += offset;
		   for (k=0;k<dim2;k++) {
			   kk=((k+nvshft+dim2)%dim2)*dim1;
			   ok=k*dim1;
			   for (l=0;l<dim1;l++) {
				     slice[jj+kk+l][0]=d->csi_data[i][oj+ok+l][0];
				     slice[jj+kk+l][1]=d->csi_data[i][oj+ok+l][1];
			   }
		   }
	   }

  /* Copy shifted data back to d->data */
	for(j=0;j<dim3;j++){
		for (k=0;k<dim2*dim1;k++) {
			d->csi_data[i][offset + j*dim2*dim1+k][0]=slice[j*dim2*dim1+k][0];
			d->csi_data[i][offset + j*dim2*dim1+k][1]=slice[j*dim2*dim1+k][1];
		}
	}

  }//rcvr loop

  fftw_free(slice);

#ifdef DEBUG
  rtn=gettimeofday(&tp, NULL);
  t2=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
  fprintf(stdout,"took %f secs\n",t2-t1);
  fflush(stdout);
#endif

}

void weightdata2D(struct data *d,int mode)
{
  int i,j,k,l;
  int jj;
  int ix1,ix2;
  int d1,d2;
  int iw;
  int offset;
  double f;
  int dim0=0,dim1,dim2,dim3,nr;
  double lb,lb1,gf,gf1,sb,sb1;
  int shft;
  double *weight;


#ifdef DEBUG
  struct timeval tp;
  double t1,t2;
  int rtn;
#endif

  /* Data dimensions */
    dim1=d->np/2; dim2=d->nv; dim3=d->endpos-d->startpos; nr=d->nr;
    d1=0;d2=1;

    if(mode==SPATIAL)
    {
  		  dim0=d->np/2; dim1=d->nv; dim2=d->nv2; dim3=d->nv3;
  		  d1=1;d2=2;
  		  nr=1;  // channels combined already
    }

  switch(mode) {

  case SPATIAL:  /* spatial weighting for csi */
    /* Return if ftproc[1]=0 (as VnmrJ) */
    if ((*val("ftproc",&d->p) == 0.0)) return;
    if (d->dimstatus[d1] & FFT || d->dimstatus[d2] & FFT) return;
    lb=*val("lb1",&d->p);   /* Lorentzian for nv */
    lb1=*val("lb2",&d->p); /* Lorentzian  for nv2 direction*/
    gf=*val("gf1",&d->p);   /* Gaussian  for nv */
    gf1=*val("gf2",&d->p); /* Gaussian for nv2 */
    sb=*val("sb1",&d->p);   /* Sinebell for nv */
    sb1=*val("sb2",&d->p); /* Sinebell for nv2 */
    break;

  case STD: /* Standard parameters */
     /* Return if ftproc[1]=0 (as VnmrJ) */
     if ((*val("ftproc",&d->p) == 0.0)) return;
     if (d->dimstatus[d1] & FFT || d->dimstatus[d2] & FFT) return;
     lb=*val("lb",&d->p);   /* Lorentzian */
     lb1=*val("lb1",&d->p); /* Lorentzian */
     gf=*val("gf",&d->p);   /* Gaussian */
     gf1=*val("gf1",&d->p); /* Gaussian */
     sb=*val("sb",&d->p);   /* Sinebell */
     sb1=*val("sb1",&d->p); /* Sinebell */
/*
    at=*val("at",&d->p);
*/
    break;

    case READ: /* Read only using standard parameters */
      /* Return if ftproc[1]=0 (as VnmrJ) */
      if ((*val("ftproc",&d->p) == 0.0)) return;
      if (d->dimstatus[0] & FFT) return;
      lb=*val("lb",&d->p); /* Lorentzian */
      gf=*val("gf",&d->p); /* Gaussian */
      sb=*val("sb",&d->p); /* Sinebell */
      lb1=0.0; gf1=0.0; sb1=0.0;
      break;
    case PHASE: /* Phase only using standard parameters */
      /* Return if ftproc[1]=0 (as VnmrJ) */
      if ((*val("ftproc",&d->p) == 0.0)) return;
      if (d->dimstatus[1] & FFT) return;
      lb1=*val("lb1",&d->p); /* Lorentzian */
      gf1=*val("gf1",&d->p); /* Gaussian */
      sb1=*val("sb1",&d->p); /* Sinebell */
      lb=0.0; gf=0.0; sb=0.0;
      break;
     case REFREAD: /* Read only using reference parameters */
      if (d->dimstatus[0] & FFT) return;
      lb=*val("reflb",&d->p); /* Lorentzian */
      gf=*val("refgf",&d->p); /* Gaussian */
      sb=*val("refsb",&d->p); /* Sinebell */
      lb1=0.0; gf1=0.0; sb1=0.0;
      break;
    case MK: /* Mask parameters */
      if (d->dimstatus[0] & FFT || d->dimstatus[1] & FFT) return;
      lb=*val("masklb",&d->p);   /* Lorentzian */
      lb1=*val("masklb1",&d->p); /* Lorentzian */
      gf=*val("maskgf",&d->p);   /* Gaussian */
      gf1=*val("maskgf1",&d->p); /* Gaussian */
      sb=*val("masksb",&d->p);   /* Sinebell */
      sb1=*val("masksb1",&d->p); /* Sinebell */
      break;
    case SM: /* Sensitivity map parameters */
      if (d->dimstatus[0] & FFT || d->dimstatus[1] & FFT) return;
      lb=*val("smaplb",&d->p);   /* Lorentzian */
      lb1=*val("smaplb1",&d->p); /* Lorentzian */
      gf=*val("smapgf",&d->p);   /* Gaussian */
      gf1=*val("smapgf1",&d->p); /* Gaussian */
      sb=*val("smapsb",&d->p);   /* Sinebell */
      sb1=*val("smapsb1",&d->p); /* Sinebell */
      break;
    default:
      /* Invalid mode */
      fprintf(stderr,"\n%s: %s()\n",__FILE__,__FUNCTION__);
      fprintf(stderr,"  Invalid 2nd argument %s(*,'mode')\n",__FUNCTION__);
      fflush(stderr);
      return;
      break;
  } /* end mode switch */

  /* Return if no weighting is active */
  if (!lb && !lb1 && !gf && !gf1 && !sb && !sb1) return;

#ifdef DEBUG
  fprintf(stdout,"\n%s: %s()\n",__FILE__,__FUNCTION__);
  rtn=gettimeofday(&tp, NULL);
  t1=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
  fflush(stdout);
#endif



  /* Calculate weighting */
  if ((weight = (double *)malloc(dim2*dim1*sizeof(double))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
  for(k=0;k<dim2;k++) {
    for (l=0;l<dim1;l++) {
      ix1=k*dim1+l;
      weight[ix1]=1.0;
    }
  }

  /* Lorentzian line broadening */
  if (lb) { /* lb is active */
    if (d->dimstatus[d1] & SHIFT) shft=0;
    else shft=dim1/2-1;
    for (l=0;l<dim1/2;l++) {
      /* Lorentzian broadening as fraction of FOV */
      f=exp(-abs(l-shft)*M_PI*lb);
      /* Lorentzian broadening in # pixels */
      /* f=exp(-abs(l-lbs)*M_PI*lb/dim1); */
      /* Lorentzian broadening in Hz (as VnmrJ) */
      /* f=exp(-abs(l-lbs)*at*M_PI*lb/dim1); */
      for(k=0;k<dim2;k++) {
        ix1=k*dim1+l;
        ix2=k*dim1+dim1-1-l;
        weight[ix1] *=f;
        weight[ix2] *=f;
      }
    }
  }
  if (lb1) { /* lb1 is active */
    if (d->dimstatus[d2] & SHIFT) shft=0;
    else shft=dim2/2-1;
    for(k=0;k<dim2/2;k++) {
      /* Lorentzian broadening as fraction of FOV */
      f=exp(-abs(k-shft)*M_PI*lb1);
      for (l=0;l<dim1;l++) {
        ix1=k*dim1+l;
        ix2=(dim2-1-k)*dim1+l;
        weight[ix1] *=f;
        weight[ix2] *=f;
      }
    }
  }

  /* Gaussian line broadening */
  if (gf) { /* gf is active */
    if (d->dimstatus[d1] & SHIFT) shft=0;
    else shft=dim1/2-1;
    for (l=0;l<dim1/2;l++) {
      /* Gaussian broadening as fraction of FOV */
      f=(l-shft)*M_PI*gf;
      f=exp(-f*f);
      for(k=0;k<dim2;k++) {
        ix1=k*dim1+l;
        ix2=k*dim1+dim1-1-l;
        weight[ix1] *=f;
        weight[ix2] *=f;
      }
    }
  }
  if (gf1) { /* gf1 is active */
    if (d->dimstatus[d2] & SHIFT) shft=0;
    else shft=dim2/2-1;
    for(k=0;k<dim2/2;k++) {
      /* Gaussian broadening as fraction of FOV */
      f=(k-shft)*M_PI*gf1;
      f=exp(-f*f);
      for (l=0;l<dim1;l++) {
        ix1=k*dim1+l;
        ix2=(dim2-1-k)*dim1+l;
        weight[ix1] *=f;
        weight[ix2] *=f;
      }
    }
  }

  /* Sinebell line broadening */
  if (sb) { /* sb is active */
    if (d->dimstatus[d1] & SHIFT) shft=0;
    else shft=dim1/2-1;
    for (l=0;l<dim1/2;l++) {
      /* Sinebell broadening as fraction of FOV */
      f=(l-shft)*M_PI*sb;
      if (FP_LT(fabs(f),M_PI/2))
        f=cos(f);
      else
        f=0.0;
      for(k=0;k<dim2;k++) {
        ix1=k*dim1+l;
        ix2=k*dim1+dim1-1-l;
        weight[ix1] *=f;
        weight[ix2] *=f;
      }
    }
  }
  if (sb1) { /* sb1 is active */
    if (d->dimstatus[d2] & SHIFT) shft=0;
    else shft=dim2/2-1;
    for(k=0;k<dim2/2;k++) {
      /* Sinebell broadening as fraction of FOV */
      f=(k-shft)*M_PI*sb1;
      if (FP_LT(fabs(f),M_PI/2))
        f=cos(f);
      else
        f=0.0;
      for (l=0;l<dim1;l++) {
        ix1=k*dim1+l;
        ix2=(dim2-1-k)*dim1+l;
        weight[ix1] *=f;
        weight[ix2] *=f;
      }
    }
  }



  /* Weight the data */
  if (mode == SPATIAL) {
	  offset=0;
	  if(spar(d,"apptype","im3Dcsi"))
		  offset=d->vol*dim0*dim1*dim2;

		for (i = 0; i < nr; i++) {
			for (j = 0; j < dim2; j++) {
				jj=offset + j*dim1*dim0;
				for (k = 0; k < dim1; k++) {
					iw=j*dim1+k;
					for (l = 0; l < dim0; l++) {
						ix1 = k * dim0 + l;
						d->csi_data[i][jj+ix1][0] *= weight[iw];
						d->csi_data[i][jj+ix1][1] *= weight[iw];
					}
				}
			}
		}
	} else {
		/* Weight the data */
		for (i = 0; i < nr; i++) {
			for (j = 0; j < dim3; j++) {
				for (k = 0; k < dim2; k++) {
					for (l = 0; l < dim1; l++) {
						ix1 = k * dim1 + l;
						d->data[i][j][ix1][0] *= weight[ix1];
						d->data[i][j][ix1][1] *= weight[ix1];
					}
				}
			}
		}
	}

  free(weight);

#ifdef DEBUG
  rtn=gettimeofday(&tp, NULL);
  t2=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
  if (lb) fprintf(stdout,"  lb weighting = %f\n",lb);
  if (lb1) fprintf(stdout,"  lb1 weighting = %f\n",lb1);
  if (gf) fprintf(stdout,"  gf weighting = %f\n",gf);
  if (gf1) fprintf(stdout,"  gf1 weighting = %f\n",gf1);
  if (sb) fprintf(stdout,"  sb weighting = %f\n",sb);
  if (sb1) fprintf(stdout,"  sb1 weighting = %f\n",sb1);
  fprintf(stdout,"  Took %f secs\n",t2-t1);
  fflush(stdout);
#endif

}

void zerofill2D(struct data *d,int mode)
{
  int i,j,k,l;
  int ix1,ix2;
  int dim1,dim2,dim3,nr;
  int fn=0,fn1=0;
  int ndim1,ndim2,range1,range2,shft1,shft2,nshft1,nshft2;
  double oversample;
  fftw_complex *data;

#ifdef DEBUG
  struct timeval tp;
  double t1,t2;
  int rtn;
#endif

  switch(mode) {
    case STD: /* Standard parameters */
      if (d->dimstatus[0] & FFT || d->dimstatus[1] & FFT) return;
      oversample=*val("oversample",&d->p);
      if (oversample == 0.0) oversample=1;
      fn=d->fn*oversample;
      fn1=d->fn1;
      break;
    case READ: /* Standard parameter */
      if (d->dimstatus[0] & FFT) return;
      oversample=*val("oversample",&d->p);
      if (oversample == 0.0) oversample=1;
      fn=d->fn*oversample;
      break;
    case PHASE: /* Standard parameter */
      if (d->dimstatus[1] & FFT) return;
      fn1=d->fn1;
      break;
    case MK: /* Mask parameters */
      if (d->dimstatus[0] & FFT || d->dimstatus[1] & FFT) return;
      fn=*val("maskfn",&d->p);
      fn1=*val("maskfn1",&d->p);
      break;
    default:
      /* Invalid mode */
      fprintf(stderr,"\n%s: %s()\n",__FILE__,__FUNCTION__);
      fprintf(stderr,"  Invalid 2nd argument %s(*,'mode')\n",__FUNCTION__);
      fflush(stderr);
      return;
      break;
  } /* end mode switch */

  /* Check that either fn or fn1 are active */
  /* If so, make sure they are exactly divisible by 4 */
  if (!fn && !fn1) return;
  if (!fn) fn = d->np;
  else { fn /=4; fn *=4; }
  if (!fn1) fn1 = d->nv*2;
  else { fn1 /=4; fn1 *=4; }

#ifdef DEBUG
  fprintf(stdout,"\n%s: %s()\n",__FILE__,__FUNCTION__);
  rtn=gettimeofday(&tp, NULL);
  t1=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
  fflush(stdout);
#endif

  /* Initial data dimensions */
  dim1=d->np/2; dim2=d->nv; dim3=d->endpos-d->startpos; nr=d->nr;

  /* New data dimensions */
  ndim1=fn/2;
  ndim2=fn1/2;

  /* Make data the new size and initialise to zero */
  if ((data = (fftw_complex *)fftw_malloc(ndim2*ndim1*sizeof(fftw_complex))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
  for(k=0;k<ndim2;k++) {
    for (l=0;l<ndim1;l++) {
      ix1=k*ndim1+l;
      data[ix1][0]=0.0;
      data[ix1][1]=0.0;
    }
  }

  /* Set up for new matrix size to be larger than original */
  range1=dim1; range2=dim2; /* data range */
  shft1=0; shft2=0;         /* shifts required for original data */
  nshft1=abs(ndim1-dim1);   /* shift for new matrix */
  nshft2=abs(ndim2-dim2);   /* shift for new matrix */

  /* Correct new matrix shifts if original data is not shifted */
  if (!(d->dimstatus[0] & SHIFT)) nshft1/=2;
  if (!(d->dimstatus[1] & SHIFT)) nshft2/=2;

  /* Now allow new matrix size to be smaller than original */
  if (ndim1<dim1) {
    range1=ndim1; shft1=nshft1; nshft1=0; /* reset range and swap shifts */
  }
  if (ndim2<dim2) {
    range2=ndim2; shft2=nshft2; nshft2=0; /* reset range and swap shifts */
  }

  /* Resize according to whether data is shifted */
  if (d->dimstatus[0] & SHIFT) { /* dim1 shifted */
    if (d->dimstatus[1] & SHIFT) { /* both dim1 and dim2 shifted */
      for (i=0;i<nr;i++) {
        for (j=0;j<dim3;j++) {
          /* Copy from d->data[i][j] to data */
          for(k=0;k<range2;k++) {
            for (l=0;l<range1;l++) {
              ix1=((2*k/range2)*nshft2+k)*ndim1+(2*l/range1)*nshft1+l;
              ix2=((2*k/range2)*shft2+k)*dim1+(2*l/range1)*shft1+l;
              data[ix1][0]=d->data[i][j][ix2][0];
              data[ix1][1]=d->data[i][j][ix2][1];
            }
          }
          /* free d->data[i][j], reallocate and copy data back in its place */
          fftw_free(d->data[i][j]);
          if ((d->data[i][j] = (fftw_complex *)fftw_malloc(ndim2*ndim1*sizeof(fftw_complex))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
          for(k=0;k<ndim2;k++) {
            for (l=0;l<ndim1;l++) {
              ix1=k*ndim1+l;
              d->data[i][j][ix1][0]=data[ix1][0];
              d->data[i][j][ix1][1]=data[ix1][1];
            }
          }
        }
      }
    } else { /* dim1 is shifted, dim2 not */
      for (i=0;i<nr;i++) {
        for (j=0;j<dim3;j++) {
          /* Copy from d->data[i][j] to data */
          for(k=0;k<range2;k++) {
            for (l=0;l<range1;l++) {
              ix1=(nshft2+k)*ndim1+(2*l/range1)*nshft1+l;
              ix2=(shft2+k)*dim1+(2*l/range1)*shft1+l;
              data[ix1][0]=d->data[i][j][ix2][0];
              data[ix1][1]=d->data[i][j][ix2][1];
            }
          }
          /* free d->data[i][j], reallocate and copy data back in its place */
          fftw_free(d->data[i][j]);
          if ((d->data[i][j] = (fftw_complex *)fftw_malloc(ndim2*ndim1*sizeof(fftw_complex))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
          for(k=0;k<ndim2;k++) {
            for (l=0;l<ndim1;l++) {
              ix1=k*ndim1+l;
              d->data[i][j][ix1][0]=data[ix1][0];
              d->data[i][j][ix1][1]=data[ix1][1];
            }
          }
        }
      }
    }
  } else { /* dim1 not shifted */
    if (d->dimstatus[1] & SHIFT) { /* dim2 is shifted, dim1 not */
      for (i=0;i<nr;i++) {
        for (j=0;j<dim3;j++) {
          /* Copy from d->data[i][j] to data */
          for(k=0;k<range2;k++) {
            for (l=0;l<range1;l++) {
              ix1=((2*k/range2)*nshft2+k)*ndim1+nshft1+l;
              ix2=((2*k/range2)*shft2+k)*dim1+shft1+l;
              data[ix1][0]=d->data[i][j][ix2][0];
              data[ix1][1]=d->data[i][j][ix2][1];
            }
          }
          /* free d->data[i][j], reallocate and copy data back in its place */
          fftw_free(d->data[i][j]);
          if ((d->data[i][j] = (fftw_complex *)fftw_malloc(ndim2*ndim1*sizeof(fftw_complex))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
          for(k=0;k<ndim2;k++) {
            for (l=0;l<ndim1;l++) {
              ix1=k*ndim1+l;
              d->data[i][j][ix1][0]=data[ix1][0];
              d->data[i][j][ix1][1]=data[ix1][1];
            }
          }
        }
      }
    } else { /* Neither dim1 nor dim2 shifted */
      for (i=0;i<nr;i++) {
        for (j=0;j<dim3;j++) {
          /* Copy from d->data[i][j] to data */
          for(k=0;k<range2;k++) {
            for (l=0;l<range1;l++) {
              ix1=(nshft2+k)*ndim1+nshft1+l;
              ix2=(shft2+k)*dim1+shft1+l;
              data[ix1][0]=d->data[i][j][ix2][0];
              data[ix1][1]=d->data[i][j][ix2][1];
            }
          }
          /* free d->data[i][j], reallocate and copy data back in its place */
          fftw_free(d->data[i][j]);
          if ((d->data[i][j] = (fftw_complex *)fftw_malloc(ndim2*ndim1*sizeof(fftw_complex))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
          for(k=0;k<ndim2;k++) {
            for (l=0;l<ndim1;l++) {
              ix1=k*ndim1+l;
              d->data[i][j][ix1][0]=data[ix1][0];
              d->data[i][j][ix1][1]=data[ix1][1];
            }
          }
        }
      }
    }
  }

  /* Free data */
  fftw_free(data);

  /* Set zerofill flags */
  if (d->np != fn) d->dimstatus[0]+=ZEROFILL;
  if (d->nv != ndim2) d->dimstatus[1]+=ZEROFILL;

  /* Update parameters to reflect new data size */
  d->np=fn;
  d->nv=ndim2;

#ifdef DEBUG
  rtn=gettimeofday(&tp, NULL);
  t2=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
  fprintf(stdout,"  Zero filling to give %d x %d data: took %f secs\n",ndim1,ndim2,t2-t1);
  fflush(stdout);
#endif

}

void phaseramp2D(struct data *d,int mode)
{
  int dim1,dim2,dim3,nr;
  int i,j,k,l,ix;
  double re,im,M,theta,factor;
  double offset,fov,oversample;

#ifdef DEBUG
  struct timeval tp;
  double t1,t2;
  int rtn;
#endif

  /* Data dimensions */
  dim1=d->np/2; dim2=d->nv; dim3=d->endpos-d->startpos; nr=d->nr;

  switch (mode) {
    case READ:
      /* Return if there is no phase ramp to apply */
      offset=*val("pro",&d->p);
      if (offset == 0.0) return;
#ifdef DEBUG
  fprintf(stdout,"\n%s: %s()\n",__FILE__,__FUNCTION__);
  rtn=gettimeofday(&tp, NULL);
  t1=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
  fflush(stdout);
#endif
      fov=*val("lro",&d->p);
      oversample=*val("oversample",&d->p);
      if (oversample<1) oversample=1;
      /* Set phase ramp factor to correct for offset */
      factor=2*M_PI*offset/(fov*oversample);
      /* Apply phase ramp to generate frequency shift */
      for (i=0;i<nr;i++) {
        for (j=0;j<dim3;j++) {
          for (k=0;k<dim2;k++) {
            ix = k*dim1;
            for (l=0;l<dim1/2;l++) {
              re=d->data[i][j][ix+l][0];
              im=d->data[i][j][ix+l][1];
              M=sqrt(re*re+im*im);
              theta = atan2(im,re) + factor*(l);
              d->data[i][j][ix+l][0]=M*cos(theta);
              d->data[i][j][ix+l][1]=M*sin(theta);
            }
            for (l=dim1/2;l<dim1;l++) {
              re=d->data[i][j][ix+l][0];
              im=d->data[i][j][ix+l][1];
              M=sqrt(re*re+im*im);
              theta = atan2(im,re) + factor*(l-dim1);
              d->data[i][j][ix+l][0]=M*cos(theta);
              d->data[i][j][ix+l][1]=M*sin(theta);
            }
          }
        }
      }
#ifdef DEBUG
  rtn=gettimeofday(&tp, NULL);
  t2=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
  fprintf(stdout,"  Readout phase ramp (%d traces): took %f secs\n",dim2,t2-t1);
  fflush(stdout);
#endif
      break;
    case PHASE:
      /* Return if there is no phase ramp to apply */
      offset=*val("ppe",&d->p);
      if (offset == 0.0) return;
#ifdef DEBUG
  fprintf(stdout,"\n%s: %s()\n",__FILE__,__FUNCTION__);
  rtn=gettimeofday(&tp, NULL);
  t1=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
  fflush(stdout);
#endif
      fov=*val("lpe",&d->p);
      /* Set phase ramp factor to correct for offset */
      factor=-2*M_PI*offset/fov;
      /* Apply phase ramp to generate frequency shift */
      for (i=0;i<nr;i++) {
        for (j=0;j<dim3;j++) {
          for (k=0;k<dim2/2;k++) {
            ix = k*dim1;
            for (l=0;l<dim1;l++) {
              re=d->data[i][j][ix+l][0];
              im=d->data[i][j][ix+l][1];
              M=sqrt(re*re+im*im);
              theta = atan2(im,re) + factor*(k);
              d->data[i][j][ix+l][0]=M*cos(theta);
              d->data[i][j][ix+l][1]=M*sin(theta);
            }
          }
          for (k=dim2/2;k<dim2;k++) {
            ix = k*dim1;
            for (l=0;l<dim1;l++) {
              re=d->data[i][j][ix+l][0];
              im=d->data[i][j][ix+l][1];
              M=sqrt(re*re+im*im);
              theta = atan2(im,re) + factor*(k-dim2);
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

void phaseramp2DCSI(struct data *d, int d4index, int mode)
{
  int dim1,dim2,dim3,nr;
  int fn1, fn2;
  int i,j,k,l,ix, islice;
  int kk;
  int d4offset;
  int fovsize1, fovsize2;
  double re,im,M,theta,factor;
  double offset,fov,oversample;
  double doffset;

#ifdef DEBUG
  struct timeval tp;
  double t1,t2;
  int rtn;
#endif

  /* Data dimensions */
  dim1=d->np/2; dim2=d->nv; dim3=d->nv2; nr=d->nr; islice=d->startpos;
  d4offset = d4index * dim1*dim2*dim3;

   fovsize1=dim2; fovsize2=dim3;

   fn1=d->fn1; fn2=d->fn2;
   if(fn1){ fn1 /=4; fn1 *=4; fovsize1=fn1/2;}
   if(fn2){ fn2 /=4; fn2 *=4; fovsize2=fn2/2;}


  nr=1; // channels already combined

  switch (mode) {
    case PHASE:
      /* Return if there is no phase ramp to apply */
      offset=*val("ppe",&d->p);
      doffset=*val("dppe",&d->p);
      if ((offset == 0.0)&& (doffset == 0.0))
    	  return;
#ifdef DEBUG
  fprintf(stdout,"\n%s: %s()\n",__FILE__,__FUNCTION__);
  rtn=gettimeofday(&tp, NULL);
  t1=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
  fflush(stdout);
#endif
      fov=*val("lpe",&d->p);
      oversample=*val("oversample",&d->p);
      if (oversample<1) oversample=1;
      /* Set phase ramp factor to correct for offset */
      offset += doffset*(fov/fovsize1); // add fractonal voxel offset
      factor=-2*M_PI*offset/(fov);
      /* Apply phase ramp to generate phase shift */
      for (i=0;i<nr;i++) {
          for (k=0;k<dim3;k++) {
        	  kk=d4offset + k*dim2*dim1;
            for (l=0;l<dim2/2;l++) {
            	ix=l*dim1;
					for(j=0;j<dim1;j++){
				//		re=d->data[i][k][ix+j][0];
					//	im=d->data[i][k][ix+j][1];

						re=d->csi_data[i][kk+ix+j][0];
						im=d->csi_data[i][kk+ix+j][1];
						M=sqrt(re*re+im*im);
						theta = atan2(im,re) + factor*(l);
						d->csi_data[i][kk+ix+j][0]=M*cos(theta);
						d->csi_data[i][kk+ix+j][1]=M*sin(theta);
					}
            }
            for (l=dim2/2;l<dim2;l++) {
            	ix=l*dim1;
            	for(j=0;j<dim1;j++){
            		re=d->csi_data[i][kk+ix+j][0];
            		im=d->csi_data[i][kk+ix+j][1];
            		M=sqrt(re*re+im*im);
            		theta = atan2(im,re) + factor*(l-dim2);
            		d->csi_data[i][kk+ix+j][0]=M*cos(theta);
            		d->csi_data[i][kk+ix+j][1]=M*sin(theta);
            	}
            }
          } // dim3 loop
      } // nr loop
#ifdef DEBUG
  rtn=gettimeofday(&tp, NULL);
  t2=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
  fprintf(stdout,"  Readout phase ramp (%d traces): took %f secs\n",dim2,t2-t1);
  fflush(stdout);
#endif
      break;
    case PHASE2:
      /* Return if there is no phase ramp to apply */
      offset=*val("ppe2",&d->p);
      doffset=*val("dppe2",&d->p);
            if ((offset == 0.0)&& (doffset == 0.0))
          	  return;
#ifdef DEBUG
  fprintf(stdout,"\n%s: %s()\n",__FILE__,__FUNCTION__);
  rtn=gettimeofday(&tp, NULL);
  t1=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
  fflush(stdout);
#endif
      fov=*val("lpe2",&d->p);
      offset += doffset*(fov/fovsize2); // add fractonal voxel offset
      /* Set phase ramp factor to correct for offset */
      factor=-2*M_PI*offset/fov;
      /* Apply phase ramp to generate phase shift */
      for (i=0;i<nr;i++) {
          for (k=0;k<dim3/2;k++) {
				  kk=d4offset + k*dim2*dim1;
            for (l=0;l<dim2;l++) {
            	ix=l*dim1;
					for(j=0;j<dim1;j++){
						re=d->csi_data[i][kk+ix+j][0];
						im=d->csi_data[i][kk+ix+j][1];
						M=sqrt(re*re+im*im);
						theta = atan2(im,re) + factor*(k);
						d->csi_data[i][kk+ix+j][0]=M*cos(theta);
						d->csi_data[i][kk+ix+j][1]=M*sin(theta);
					}
            }
          }
          for (k=dim3/2;k<dim3;k++) {
        	  kk=d4offset + k*dim1*dim2;
            for (l=0;l<dim2;l++) {
            	ix=l*dim1;
					for(j=0;j<dim1;j++){
						re=d->csi_data[i][kk+ix+j][0];
						im=d->csi_data[i][kk+ix+j][1];
						M=sqrt(re*re+im*im);
						theta = atan2(im,re) + factor*(k-dim3);
						d->csi_data[i][kk+ix+j][0]=M*cos(theta);
						d->csi_data[i][kk+ix+j][1]=M*sin(theta);
					}
            }
          }
      } // rcvr loop
#ifdef DEBUG
  rtn=gettimeofday(&tp, NULL);
  t2=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
  fprintf(stdout,"  Phase encode phase ramp for CSI (%d traces): took %f secs\n",dim1,t2-t1);
  fflush(stdout);
#endif
      break;
  }
}




void phasedata2D(struct data *d,int mode)
{
  int dim1,dim2,dim3,nr;
  double rp,lp,lp1;
  double re,im,M,theta;
  int i,j,k,l;
  int ix,shft1=0,shft2=0;

#ifdef DEBUG
  struct timeval tp;
  double t1,t2;
  int rtn;
#endif

  switch(mode) {
    case VJ: /* Standard VnmrJ parameters */
      /* Only phase data if there's real or imaginary output */
      if ((!(spar(d,"imRE","y")))
        && (!(spar(d,"imIM","y"))))
        return;
      break;
  } /* end mode switch */

  /* Only phase image data */
  if (!(d->dimstatus[0] & FFT) || !(d->dimstatus[0] & FFT)) return;

  /* Get phasing parameters */
  rp=*val("rp",&d->p);
  lp=*val("lp",&d->p);
  lp1=*val("lp1",&d->p);

  /* Return if no phasing is required */
  if (!rp && !lp && !lp1) return;

#ifdef DEBUG
  fprintf(stdout,"\n%s: %s()\n",__FILE__,__FUNCTION__);
  rtn=gettimeofday(&tp, NULL);
  t1=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
  fflush(stdout);
#endif

  /* Data dimensions */
  dim1=d->np/2; dim2=d->nv; dim3=d->endpos-d->startpos; nr=d->nr;

  /* Set shifts */
  if (!(d->dimstatus[0] & SHIFT)) shft1=(int)ceil(dim1/2.0);
  if (!(d->dimstatus[1] & SHIFT)) shft2=(int)ceil(dim2/2.0);

  /* Phase */
  /* Set lp and lp1 so that they adjust the phase by the specified
     phase in degrees accross the image */
  rp *=DEG2RAD;
  lp *=DEG2RAD/dim1;
  lp1 *=DEG2RAD/dim2;
  for (i=0;i<nr;i++) {
    for (j=0;j<dim3;j++) {
      for(k=0;k<dim2;k++) {
        for (l=0;l<dim1;l++) {
          ix=((k+shft2)%dim2)*dim1+(l+shft1)%dim1;
          re=d->data[i][j][ix][0];
          im=d->data[i][j][ix][1];
          M=sqrt(re*re+im*im);
          theta = atan2(im,re) + rp +lp*(l-dim1/2) +lp1*(k-dim2/2);
          d->data[i][j][ix][0]=M*cos(theta);
          d->data[i][j][ix][1]=M*sin(theta);
        }
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

void zoomdata2D(struct data *d,int startdim1, int widthdim1, int startdim2, int widthdim2)
{
  int dim1,dim2,dim3,nr;
  int i,j,k,l;
  int ix1,ix2;
  fftw_complex *data;

#ifdef DEBUG
  struct timeval tp;
  double t1,t2;
  int rtn;
  fprintf(stdout,"\n%s: %s()\n",__FILE__,__FUNCTION__);
  rtn=gettimeofday(&tp, NULL);
  t1=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
  fflush(stdout);
#endif

  /* Data dimensions */
  dim1=d->np/2; dim2=d->nv; dim3=d->endpos-d->startpos; nr=d->nr;

  /* Make data the new size and initialise to zero */
  if ((data = (fftw_complex *)fftw_malloc(widthdim2*widthdim1*sizeof(fftw_complex))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);

  /* Copy from d->data[i][j] to data */
  for (i=0;i<nr;i++) {
    for (j=0;j<dim3;j++) {
      for(k=0;k<widthdim2;k++) {
        ix1=k*widthdim1;
        ix2=(k+startdim2)*dim1+startdim1;
        for (l=0;l<widthdim1;l++) {
          data[ix1][0]=d->data[i][j][ix2][0];
          data[ix1][1]=d->data[i][j][ix2][1];
          ix1++; ix2++;
        }
      }
      /* free d->data[i][j], reallocate and copy data back in its place */
      fftw_free(d->data[i][j]);
      if ((d->data[i][j] = (fftw_complex *)fftw_malloc(widthdim2*widthdim1*sizeof(fftw_complex))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
      ix1=0;
      for(k=0;k<widthdim2;k++) {
        for (l=0;l<widthdim1;l++) {
          d->data[i][j][ix1][0]=data[ix1][0];
          d->data[i][j][ix1][1]=data[ix1][1];
          ix1++;
        }
      }
    }
  }

  /* Update parameters to reflect new data size */
  d->np=widthdim1*2;
  d->nv=widthdim2;

#ifdef DEBUG
  rtn=gettimeofday(&tp, NULL);
  t2=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
  fprintf(stdout,"  Zoom to give %d x %d data: took %f secs\n",widthdim1,widthdim2,t2-t1);
  fflush(stdout);
#endif

}

void revread(struct data *d)
{
  int dim1,dim2,dim3,nr;
  int i,j,k,l,ix;
  fftw_complex *data;

#ifdef DEBUG
  fprintf(stdout,"\n%s: %s()\n",__FILE__,__FUNCTION__);
  fprintf(stdout,"  Reversing data in readout dimension ... ");
  fflush(stdout);
#endif

  /* Initial data dimensions */
  dim1=d->np/2; dim2=d->nv; dim3=d->endpos-d->startpos; nr=d->nr;

  /* Make data the new size */
  if ((data = (fftw_complex *)fftw_malloc(dim1*sizeof(fftw_complex))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);

  for (i=0;i<nr;i++) {
    for (j=0;j<dim3;j++) {
      for (k=0;k<dim2;k++) {
        ix = k*dim1+dim1; /* not ix = k*dim1+dim1-1, presumably because central pixel is offset */
        for (l=1;l<dim1;l++) {
          data[l][0] = d->data[i][j][ix-l][0];
          data[l][1] = d->data[i][j][ix-l][1];
        }
        ix = k*dim1;
        for (l=0;l<dim1;l++) {
          d->data[i][j][ix+l][0] = data[l][0];
          d->data[i][j][ix+l][1] = data[l][1];
        }
      }
    }
  }

#ifdef DEBUG
  fprintf(stdout,"done\n");
  fflush(stdout);
#endif

}

void navcorr(struct data *d,struct data *nav)
{
  int dim1,dim2,dim3,nr;
  double re,im,M,phase,*navphase;
  int navinvert,navsign;
  int i,j,k,l,ix;

#ifdef DEBUG
  struct timeval tp;
  double t1,t2;
  int rtn;
  fprintf(stdout,"\n%s: %s()\n",__FILE__,__FUNCTION__);
  rtn=gettimeofday(&tp, NULL);
  t1=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
  fflush(stdout);
#endif

  if (!d->nav) return;

  /* Cater for pointwise and linear navigator corrections */
  if (!spar(d,"nav_type","pointwise") && !spar(d,"nav_type","linear")) return;

  /* Data dimensions */
  dim1=d->np/2; dim2=d->nv; dim3=d->endpos-d->startpos; nr=d->nr;

  /* For now, just use the first navigator of each segment for phase correction */

  /* Check if navigator read out is inverted. If so reverse navigator profiles */
  navinvert=(int)*val("navinvert",&d->p);
  navsign=1;
  if (navinvert) {
    navsign=-1;
    revread(nav);
  }

  /* Allocate for a trace of navigator phase */
  if ((navphase = (double *)malloc(dim1*sizeof(double))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);

  /* Pointwise correction, just correct each point */
  if (spar(d,"nav_type","pointwise")) {
    for (i=0;i<nr;i++) {
      for (j=0;j<dim3;j++) {
        for (k=0;k<dim2;k++) {
          ix = segindex(d,k)*dim1;
          for (l=0;l<dim1;l++) {
            re=nav->data[i][j][ix+l][0];
            im=nav->data[i][j][ix+l][1];
            navphase[l]=atan2(im,re);
          }
          ix = k*dim1;
          for (l=0;l<dim1;l++) {
            re=d->data[i][j][ix+l][0];
            im=d->data[i][j][ix+l][1];
            phase=atan2(im,re);
            M = sqrt(re*re + im*im);
            d->data[i][j][ix+l][0]=M*cos(phase-navsign*navphase[l]);
            d->data[i][j][ix+l][1]=M*sin(phase-navsign*navphase[l]);
          }
        }
      }
    }
  }

  /* Linear correction */

  free(navphase);

#ifdef DEBUG
  rtn=gettimeofday(&tp, NULL);
  t2=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
  fprintf(stdout,"  %s navigator phase correction (%d traces): took %f secs\n",*sval("nav_type",&d->p),dim2,t2-t1);
  fflush(stdout);
#endif

}
