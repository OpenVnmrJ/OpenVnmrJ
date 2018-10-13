/* dproc.c */
/*---------------------------------------------------------------------------*/
/*                                                                           */
/* dproc.c: Data processing routines                                         */
/*                                                                           */
/* Copyright (C) 2012 Paul Kinchesh                                          */
/*               2012 Martyn Klassen                                         */
/*               2012 Margaret Kritzer                                       */
/*               2012 Lana Kaiser                                            */
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

int *sliceorder(struct data *d,int dim,char *par)
{
  int *dimorder;
  double *pss;
  int index;
  int standardorder;
  int i,j,k;

#ifdef DEBUG
  struct timeval tp;
  double t1,t2;
  int rtn;
  fprintf(stdout,"\n%s: %s()\n",__FILE__,__FUNCTION__);
  rtn=gettimeofday(&tp, NULL);
  t1=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
  fflush(stdout);
#endif

  /* Set dimorder=-1 if par is not found */
  standardorder=TRUE;
  index=parindex(par,&d->p);
  if (index < 0) {
    if ((dimorder = (int *)malloc(sizeof(int))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
    dimorder[0]=-1;
  } else {
    /* Take a copy of the starting slice order */
    if ((pss = (double *)malloc(dim*sizeof(double))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
    for (i=0;i<dim;i++) pss[i]=d->p.d[index][i];
    /* Sort slice positions into ascending order */
    qsort(pss,dim,sizeof(pss[0]),doublecmp);
    /* Fill dimorder with an index of where successive slices are */
    if ((dimorder = (int *)malloc(dim*sizeof(int))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
    for (i=0;i<dim;i++) {
      for (j=0;j<dim;j++) {
        if (d->p.d[index][i] == pss[j]) {
          dimorder[i]=j;
          break;
        }
      }
    }
    /* If any slices have equal positions their dimorder index will be equal */
    /* Adjust any duplicate values appropriately */
    for (i=0;i<dim;i++) {
      k=1;
      for (j=i+1;j<dim;j++) {
        if (dimorder[j]==dimorder[i]) {
          dimorder[j]+=k;
          k++;
        }
      }
    }
    /* Set dimorder=-1 if we simply have standard order */
    for (i=0;i<dim;i++) {
      if (dimorder[i] != i) {
        standardorder=FALSE;
        break;
      }
    }
    if (standardorder) {
      free(dimorder);
      if ((dimorder = (int *)malloc(sizeof(int))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
      dimorder[0]=-1;
    }

#ifdef DEBUG
  rtn=gettimeofday(&tp, NULL);
  t2=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
  fprintf(stdout,"  Slice ordering took %f secs\n",t2-t1);
  if (!standardorder) {
    fprintf(stdout,"            pss\n");
    fprintf(stdout,"  original\tnew\t    dimorder\n");
    for (i=0;i<dim;i++)
      fprintf(stdout,"  %f\t%f\t%d\n",d->p.d[index][i],pss[i],dimorder[i]);
  }
  fflush(stdout);
#endif

    free(pss);
  }

  return(dimorder);

}


int *phaseorder(struct data *d,int views,int dim,char *par)
{
  int *dimorder;
  int index,min;
  int standardorder;
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

  /* Set dimorder=-1 if par is not found or has wrong size */
  standardorder=TRUE;
  index=parindex(par,&d->p);
  if ((index < 0) || (nvals(par,&d->p) != dim)) {
    if ((dimorder = (int *)malloc(sizeof(int))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
    dimorder[0]=-1;
  } else {
    /* Set dimorder=-1 if par has too few values */
    if (dim<views) {
      if ((dimorder = (int *)malloc(sizeof(int))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
      dimorder[0]=-1;
    } else {
      /* Fill dimorder with an index of where successive phase encodes are */
      if ((dimorder = (int *)malloc(dim*sizeof(int))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
      /* Initialize the values to be out of range */
      for (i=0;i<dim;i++) dimorder[i] = dim+1;
      /* If there's more values than views set the standard minimum multiplier */
      if (dim>views) min=-views/2+1;
      else {
        /* Find the minimum value (phase encode multipliers can run from -nv/2 to +nv/2-1 or -nv/2+1 to +nv/2) */
        min=0;
        for (i=0;i<dim;i++) if (d->p.d[index][i]<min) min=d->p.d[index][i];
      }
      for (i=0;i<dim;i++) {
        for (j=0;j<dim;j++) {
          if (d->p.d[index][i] == j+min) {
            dimorder[i]=j;
            break;
          }
        }
      }
      /* Set dimorder=-1 if we simply have standard order */
      for (i=0;i<dim;i++) {
        if (dimorder[i] != i) {
          standardorder=FALSE;
          break;
        }
      }
      if (standardorder) {
        free(dimorder);
        if ((dimorder = (int *)malloc(sizeof(int))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
        dimorder[0]=-1;
      }
    }
  }

#ifdef DEBUG
  rtn=gettimeofday(&tp, NULL);
  t2=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
  fprintf(stdout,"  Phase ordering took %f secs\n",t2-t1);
  if (!standardorder) {
  fprintf(stdout,"  dimorder:\n");
    for (i=0;i<dim;i++) {
      fprintf(stdout,"%5.1d",dimorder[i]);
      if ((i+1)%16 == 0) fprintf(stdout,"\n");
    }
  }
  fflush(stdout);
#endif

  return(dimorder);

}

int *phaselist(struct data *d,char *par)
{
  int *dimorder;
  int index,min;
  int i;
  int nacqs=0;
  char seqcon[6];
  int npts;

#ifdef DEBUG
  struct timeval tp;
  double t1,t2;
  int rtn;
  fprintf(stdout,"\n%s: %s()\n",__FILE__,__FUNCTION__);
  rtn=gettimeofday(&tp, NULL);
  t1=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
  fflush(stdout);
#endif

  /* Set dimorder=-1 if par is not found or has wrong size */
  index=parindex(par,&d->p);
  strcpy(seqcon,*sval("seqcon",&d->p));


  if(!strcmp(par,"pelist"))
   {
 	  if (seqcon[2] == 's')
 		  nacqs=d->fh.nblocks;
 	  else
 		  nacqs=d->fh.ntraces;
   }
  else if(!strcmp(par,"pe2list"))
   {
 	  if (seqcon[3] == 's')
 		  nacqs=d->fh.nblocks;
 	  else
 		  nacqs=d->fh.ntraces;
   }
  else if(!strcmp(par,"pe3list"))
   {
 	  if (seqcon[4] == 's')
 		  nacqs=d->fh.nblocks;
 	  else
 		  nacqs=d->fh.ntraces;
   }

  npts=nvals(par,&d->p);

  if ((index < 0) || (nacqs%npts)) {
    if ((dimorder = (int *)malloc(sizeof(int))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
    dimorder[0]=-1;
  }
   else {
      /* Fill dimorder with the table values */
			if ((dimorder = (int *) malloc(npts * sizeof(int))) == NULL)
				nomem(__FILE__, __FUNCTION__, __LINE__);
			/* Find the minimum value (phase encode multipliers can run from -nv/2 to +nv/2-1 or -nv/2+1 to +nv/2) */
			min = d->p.d[index][0];
			for (i = 0; i < npts; i++) {
				if (d->p.d[index][i] < min)
					min = d->p.d[index][i];
			}
			if(min > 0)min=0;  // some schemes go 0 -> (n-1)
			for (i = 0; i < npts; i++) {
				dimorder[i] = d->p.d[index][i] - min;
			}
   }

#ifdef DEBUG
  rtn=gettimeofday(&tp, NULL);
  t2=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
  fprintf(stdout,"  Phase ordering took %f secs\n",t2-t1);
  fprintf(stdout,"  dimorder:\n");
    for (i=0;i<npts;i++) {
      fprintf(stdout,"%5.1d",dimorder[i]);
      if ((i+1)%16 == 0) fprintf(stdout,"\n");
    }

  fflush(stdout);
#endif

  return(dimorder);

}

void getmax(struct data *d)
{
  int dim1,dim2,dim3,nr;
  double re,im,M;
  int i,j,k,l;

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

  /* Make sure Mval holds M^2 */
  d->max.Mval*=d->max.Mval;
  /* Get maximum and coordinates */
  for (i=0;i<nr;i++) {
    for (j=0;j<dim3;j++) {
      for(k=0;k<dim2;k++) {
        for (l=0;l<dim1;l++) {
          re=fabs(d->data[i][j][k*dim1+l][0]);
          im=fabs(d->data[i][j][k*dim1+l][1]);
          M=re*re+im*im;
          if (M > d->max.Mval) {
            d->max.Mval=M;
            d->max.np=l;
            d->max.nv=k;
            d->max.nv2=d->startpos+j;
          }
          if (re > d->max.Rval) d->max.Rval=re;
          if (im > d->max.Ival) d->max.Ival=im;
        }
      }
    }
  }
  d->max.Mval=sqrt(d->max.Mval);

  /* Set d->max.Rval = d->max.Ival */
  if (d->max.Rval>d->max.Ival) d->max.Ival=d->max.Rval;
  else d->max.Rval=d->max.Ival;

  /* Set data flag */
  d->max.data=TRUE;

#ifdef DEBUG
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
#endif

}

void getnoise(struct data *d,int mode)
{
  int dim1,dim2,dim3,nr;
  int start1,end1,start2,end2;
  double noisefrac;
  int dim1centre,dim2centre,dim1offset,dim2offset,dim3offset,ix;
  int i,j,k,l;
  double re,im,M2;
  int factor=0,datamode;

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

  datamode=FID;
  /* If FT has been done flag it as IMAGE */
  if ((d->dimstatus[0] & FFT) || (d->dimstatus[1] & FFT) || (d->dimstatus[2] & FFT)) datamode=IMAGE;

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

  /* For 3D data block check to see if we should sample noise */
  if (im3D(d)) { /* 3D data */
    /* Default start1, end1, start2, end2 for a single processing block */
    dim3offset = (int)((d->nv2*noisefrac)/2.0);
    if (!dim3offset) dim3offset = 1;
    /* For shifted FID and not-shifted IMAGE the noise is at centre */
    /* Set the centre accordingly */
    if ((!(d->dimstatus[2] & FFT) && (d->dimstatus[2] & SHIFT))
      || ((d->dimstatus[2] & FFT) && !(d->dimstatus[2] & SHIFT))) {
      start1=d->nv2/2-dim3offset;
      end1=d->nv2/2+dim3offset;
      start2=0; end2=0;
      /* See if we should skip the processing block */
      if ((d->startpos>=end1) || (d->endpos<start1)) return;
      /* Adjust start1, end1 for the processing block */
      if (d->startpos<start1) start1-=d->startpos;
      else start1=0;
      if (d->endpos<end1) end1=d->endpos-d->startpos;
      else end1-=d->startpos;
    } else {
      start1=0;
      end1=dim3offset;
      start2=d->nv2-dim3offset; 
      end2=d->nv2;
      /* See if we should skip the processing block */
      if ((d->startpos>=end1) && (d->endpos<start2)) return;
      /* Figure the start1, end1, start2, end2 for the processing block */
      if (d->startpos<end1) {
        if (d->endpos<end1) {
          end1=d->endpos-d->startpos;
          end2=0;
        } else {
          end1-=d->startpos;
        }
      } else {
        end1=0;
      }
      if (d->endpos>=start2) {
        start2-=d->startpos;
        if (start2<0) start2=0;
        end2=d->endpos-d->startpos;
      } else {
        end2=0;
      }
      if (!end2) start2=0;
    }
#ifdef DEBUG
  fprintf(stdout,"  Sampling data block %d (start1 = %d, end1 = %d, start2 = %d, end2 = %d)\n",d->block+1,start1,end1,start2,end2);
  fflush(stdout);
#endif
  } else { /* 2D data */
    start1=0; end1=dim3;
    start2=0; end2=0;    /* Just skip the loop from start2 to end2 */
  }

  /* If there is no noise data allocate and zero */
  if (!d->noise.data) zeronoise(d);

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

  factor=d->noise.samples/nr;
  for (i=0;i<nr;i++) {
    /* Correct existing noise measurements */
    d->noise.M[i]  *=factor;
    d->noise.M2[i] *=factor;
    d->noise.Re[i]  *=factor;
    d->noise.Im[i]  *=factor;
    /* Sample the noise */
    for (j=start1;j<end1;j++) {
      for(k=0;k<2*dim2offset;k++) {
        for (l=0;l<2*dim1offset;l++) {
          ix=(k+dim2centre-dim2offset)%dim2;
          ix=ix*dim1+(l+dim1centre-dim1offset)%dim1;
          re=d->data[i][j][ix][0];
          im=d->data[i][j][ix][1];
          M2=re*re+im*im;
          d->noise.M[i]+=sqrt(M2);
          d->noise.M2[i]+=M2;
          d->noise.Re[i]+=fabs(re);
          d->noise.Im[i]+=fabs(im);
          d->noise.samples++;
        }
      }
    }
    for (j=start2;j<end2;j++) {
      for(k=0;k<2*dim2offset;k++) {
        for (l=0;l<2*dim1offset;l++) {
          ix=(k+dim2centre-dim2offset)%dim2;
          ix=ix*dim1+(l+dim1centre-dim1offset)%dim1;
          re=d->data[i][j][ix][0];
          im=d->data[i][j][ix][1];
          M2=re*re+im*im;
          d->noise.M[i]+=sqrt(M2);
          d->noise.M2[i]+=M2;
          d->noise.Re[i]+=fabs(re);
          d->noise.Im[i]+=fabs(im);
          d->noise.samples++;
        }
      }
    }
  }
  /* calculate the mean */
  factor=d->noise.samples/nr;
  for (i=0;i<nr;i++) {
    d->noise.M[i] /=factor;
    d->noise.M2[i] /=factor;
    /* For Real and Imaginary we must consider console type.
       The DDR in VNMRS produces equal noise Re and Im channels - no quad images */
    if (spar(d,"console","vnmrs")) { /* VNMRS */
      d->noise.Re[i] += d->noise.Im[i];
      d->noise.Re[i] /=2.0;
      d->noise.Im[i] = d->noise.Re[i];
    }
    d->noise.Re[i] /=factor;
    d->noise.Im[i] /=factor;
  }

  /* Now average over all receivers */
  d->noise.avM =0;
  d->noise.avM2 =0;
  d->noise.avRe =0;
  d->noise.avIm =0;
  for (i=0;i<nr;i++) {
    d->noise.avM += d->noise.M[i];
    d->noise.avM2 += d->noise.M2[i];
    d->noise.avRe += d->noise.Re[i];
    d->noise.avIm += d->noise.Im[i];
  }
  d->noise.avM /=nr;
  d->noise.avM2 /=nr;
  d->noise.avRe /=nr;
  d->noise.avIm /=nr;

  /* Flag data is non-zero */
  d->noise.zero=FALSE;

#ifdef DEBUG
  rtn=gettimeofday(&tp, NULL);
  t2=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
  fprintf(stdout,"  Noise data averaged over %d points: took %f secs\n",d->noise.samples/nr,t2-t1);
  for (i=0;i<nr;i++) {
  fprintf(stdout,"  Receiver %d: M = %.3f, M2 = %.3f, Re = %.3f, Im = %.3f\n",
    i,d->noise.M[i],d->noise.M2[i],d->noise.Re[i],d->noise.Im[i]);
  }
  fprintf(stdout,"  Average:    M = %.3f, M2 = %.3f, Re = %.3f, Im = %.3f\n",
    d->noise.avM,d->noise.avM2,d->noise.avRe,d->noise.avIm);
  fflush(stdout);
#endif

}

void combine_channels(struct data *d)
{
  int dim1,dim2,dim3,nr;
  int j,k;
  int offset;

#ifdef DEBUG
  struct timeval tp;
  double t1,t2;
  int rtn;
#endif


  /* Data dimensions */
  dim1=d->np/2; dim2=d->nv; dim3=d->nv2; nr=d->nr;

  /* Return if only one receiver */
  if (nr < 2) return;


#ifdef DEBUG
  fprintf(stdout,"\n%s: %s()\n",__FILE__,__FUNCTION__);
  rtn=gettimeofday(&tp, NULL);
  t1=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
  fflush(stdout);
#endif


 // optimally combine channels for each fid

    for (j=0;j<dim3;j++) {
      for(k=0;k<dim2;k++) {
    	  offset=j*dim2*dim1 + k*dim1;
    	  (void)opti_comb(d, offset);
        }
      }


  /* Set equalize flag */
  d->noise.equal=TRUE;

#ifdef DEBUG
  rtn=gettimeofday(&tp, NULL);
  t2=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
  fprintf(stdout,"  Noise data equalized: took %f secs\n",t2-t1);
  fflush(stdout);
#endif

}

void opti_comb(struct data *d,int offset)
{
  int dim1,dim2,dim3,nr;
  int i,j,k;
  int no_pts, no_skip, end_fid;
  int npa, st1;
  int indexmax=0;
  int nangles;

//  double maxval;
//  double snr_thresh;
  double *absfid;
  double *sensit;
  double *maxsig;
  double a, b, aa, bb;
  double sumx, sumxx;
  double maxmean;
  double weight;
  double coef;
//  double realsum, imsum, rmsd;
  double *ang_x0, *ang_y0, *uang_x0, *uang_y0;
  double admax, admean, admin;
  double adiff, final_ang;
  double min_out, max_out;

#ifdef DEBUG
  struct timeval tp;
  double t1,t2;
  int rtn;
#endif

  /* Data dimensions */
  dim1=d->np/2; dim2=d->nv; dim3=d->endpos-d->startpos; nr=d->nr;

  /* Return if only one receiver */
  if (nr < 2) return;

  no_pts=(int)*val("no_points",&d->p);
  // snr_thresh=(double)*val("snr",&d->p);
  no_skip=(int)*val("no_skip",&d->p);
  end_fid=(int)*val("end_fid",&d->p);
  npa = no_pts - no_skip;
  st1 = dim1-end_fid;

// mallocs
  absfid=calloc(dim1, sizeof(double));
  sensit=calloc(nr, sizeof(double));
  maxsig=calloc(nr, sizeof(double));
  ang_x0=calloc(dim1, sizeof(double));
  ang_y0=calloc(dim1, sizeof(double));


#ifdef DEBUG
  fprintf(stdout,"\n%s: %s()\n",__FILE__,__FUNCTION__);
  rtn=gettimeofday(&tp, NULL);
  t1=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
  fflush(stdout);
#endif

// pseudo-code provided by Lana Kaiser
  // Step 1: a) determine the channel with maximum signal - indexmax
  // b) calculate noise (sensit) and signal (maxsig) of each channel
  //  for i=1:4;
  //   sensit(i)=std( abs(fid(i,(end-endfid):end))); %noise
  //   maxsig(i)=mean(abs(fid1(i,no_skip:no_points))); %signal
  //  end
 //  indexmax=find(maxsig==max(maxsig)

	maxmean = 0.0;
	for (i = 0; i < nr; i++) {
		for (j = 0; j < dim1; j++) {
			a = d->csi_data[i][offset + j][0];
			b = d->csi_data[i][offset + j][1];
			absfid[j] = a * a + b * b;
			absfid[j]=sqrt(absfid[j]);
		}
		sumx = sumxx = 0.0;
		for (j = st1; j < dim1; j++) {
			sumx += absfid[j];
			sumxx += absfid[j] * absfid[j];
		}
		sensit[i] = sumx * sumx - sumxx / (end_fid);
		sensit[i] /= (end_fid - 1);
		maxsig[i] = 0.0;
		for (j = no_skip; j < no_pts; j++) {
			maxsig[i] += absfid[j];
		}
		maxsig[i] /= npa;
		if (maxsig[i] > maxmean){
			indexmax = i;
			maxmean = maxsig[i];
		}

	}

 // Step 2: Create a coefficient based on s/n ratio and multiply each fid by this coeff.
 // The formula is derived based on the maximization of the derivative of S/N formula.
 // for i=1:4
 //     w(i)=(maxsig(i)/sensit(i)^2)*(sensit(indexmax)^2/maxsig(indexmax));
 //     newfid(i,:)=w(i)*newfid(i,:);
 // end

    coef=sensit[indexmax]*sensit[indexmax];
    if(maxsig[indexmax] != 0.0)
    	coef /= maxsig[indexmax];
    else
    	coef=1.0;


	for(i=0;i<nr;i++){
		weight=sensit[i]*sensit[i];
		if(maxsig[i] != 0.0){
			weight /= maxsig[i];
			if(weight != 0.0)
				weight=coef/weight;
		}
		else
			weight=1.0;

//		fprintf(stderr,"rcvr %d  weight  %f  maxsig  %f  sensit  %f\n",i,weight, maxsig[i],sensit[i]);



		for(j=0;j<dim1;j++){
			d->csi_data[i][offset + j][0] *= weight;
			d->csi_data[i][offset + j][1] *= weight;
		}
	}

//	Step 3: Use the channel with the maximum SNR (indexmax) as a phase reference:

//	fid_orig=fid(indexmax); % phase reference fid
//	% fid_tophase=the fids from all the other channels, already weighted, for example
//	fid_tophase=newfid(3);

//	x0=fid_orig((no_skip+1):(no_points+no_skip));
//	y0=fid_tophase((no_skip+1):(no_points+no_skip));

//	%Figure out the the noise level @end of the fid
//	realsum=sum(real(fid_orig(1,(end-endfid):end)).^2);
//	imsum=sum(imag(fid_orig(1,(end-endfid):end)).^2);
//	rmsd=sqrt((realsum+imsum)/(endfid-1))

//	%Determine if your number of points no_points is getting into very
//	%low SNR region
//	lowsnr=find(abs(x0) < snr*rmsd);
//	if ~isempty(lowsnr)
//	    disp('WARNING, REDUCE THE NUMBER OF SAMPLING POINTS')
//	else
//	    %Have enough SNR

//	    ang_diff=unwrap(angle(x0))-unwrap(angle(y0));
//	    %find outliers
//	    final_ang=mean(ang_diff)

//	    min_out=(abs(min(ang_diff))-abs(final_ang))/abs(final_ang);
//	    max_out=(abs(max(ang_diff))-abs(final_ang))/abs(final_ang);
//	        if abs(min_out)>0.15
//	        load train; sound(y);
//	        disp('WARNING, SOME DISCREPANCY IN ANGLE > 15 percent')
//	        end

//	        if abs(max_out)>0.15
//	        load train; sound(y);
//	        disp('WARNING, SOME DISCREPANCY IN ANGLE >15 percent')
//	        end

//	end
//	%phase shift is in degrees/
//	phase_shift=180*final_ang/pi
//	fid_outp=fid_tophase.*exp(sqrt(-1)*final_ang);


	// prepare the reference phase
	k=0;
	for(j=no_skip;j<no_skip+no_pts;j++){
		ang_x0[k++]=atan2(d->csi_data[indexmax][offset][1], d->csi_data[indexmax][offset][0]);
	}
	nangles=k;
    uang_x0 = unwrap1D(ang_x0, nangles);

	// I am assuming we passed the 'enough SNR' test
	for(i=0;i<nr;i++){
		if(i != indexmax){
			k=0;
			for(j=no_skip;j<no_skip+no_pts;j++){
				ang_y0[k++]=atan2(d->csi_data[i][offset][1], d->csi_data[i][offset][0]);
			}
			uang_y0 = unwrap1D(ang_y0, nangles);
			adiff=uang_x0[0] - uang_y0[0];
			admax=admin=admean=adiff;
			for(j=1;j<nangles;j++){
				adiff=uang_x0[j] - uang_y0[j];
				if(adiff>admax)admax=adiff;
				if(adiff<admin)admin=adiff;
				admean += adiff;
			}
			final_ang=admean/nangles;  // in radians

			min_out=(fabs(admin) - fabs(final_ang))/(fabs(final_ang));
			max_out=(fabs(admax) - fabs(final_ang))/(fabs(final_ang));
			// need to check these and sound the alarm if over 0.15

			aa=cos(final_ang);
			bb=sin(final_ang);
			aa=1.0; bb=0.0;
			for(j=0;j<dim1;j++)
			{
				a = d->csi_data[i][offset + j][0];
				b = d->csi_data[i][offset + j][1];
                // apply phase correction
				d->csi_data[i][offset + j][0] = a*aa-b*bb;
				d->csi_data[i][offset + j][1] = b*aa+a*bb;
			}

		}
	}

	// NOW we sum everything into 0th channel

	for(i=1;i<nr;i++){
		for(j=0;j<dim1;j++){
			d->csi_data[0][offset +j][0] += d->csi_data[i][offset + j][0];
			d->csi_data[0][offset +j][1] += d->csi_data[i][offset + j][1];
		}
	}

	// free
	  (void)free(absfid);
	  (void)free(sensit);
	  (void)free(maxsig);
	  (void)free(ang_x0);
	  (void)free(ang_y0);

#ifdef DEBUG
  rtn=gettimeofday(&tp, NULL);
  t2=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
  fprintf(stdout,"  Channels optimally combined took %f secs\n",t2-t1);
  fflush(stdout);
#endif

}

double *unwrap1D(double *angles, int nangles)
{
	int i,j;
	double *uangles;
	double diff;

	uangles=malloc(nangles*sizeof(double));

	uangles[0]=angles[0];
	for(i=1;i<nangles;i++)
	{
		diff=angles[i]-angles[i-1];
		if(diff > M_PI)
		{
			for(j=i;j<nangles;j++)
				uangles[j]=angles[j] - 2*M_PI;
		}
		else if (diff < -1*M_PI)
		{
			for(j=i;j<nangles;j++)
				uangles[j]=angles[j] + 2*M_PI;
		}
	}

	return(uangles);
}
void equalizenoise(struct data *d,int mode)
{
  int dim1,dim2,dim3,nr;
  int i,j,k,l;
  double *Rsf,*Isf;
  double maxval;

#ifdef DEBUG
  struct timeval tp;
  double t1,t2;
  int rtn;
#endif

  switch(mode) {
    case MK: /* Mask parameters */
      /* Return unless maskeqnoise='y' */
      if (!(spar(d,"maskeqnoise","y"))) return;
      break;
    case SM: /* Sensitivity map parameters */
      /* Return unless smapeqnoise='y' */
      if (!(spar(d,"smapeqnoise","y"))) return;
      break;
    default: /* Default parameters */
      /* Return unless eqnoise='y' */
      if (!(spar(d,"eqnoise","y"))) return;
      break;
  } /* end mode switch */

  /* Data dimensions */
  dim1=d->np/2; dim2=d->nv; dim3=d->endpos-d->startpos; nr=d->nr;

  /* Return if only one receiver */
  if (nr < 2) return;

  /* If noise data does not exist set it to zero */
  if (!d->noise.data) zeronoise(d);

  /* If noise data is zero sample the noise */
  if (d->noise.zero) getnoise(d,mode);

#ifdef DEBUG
  fprintf(stdout,"\n%s: %s()\n",__FILE__,__FUNCTION__);
  rtn=gettimeofday(&tp, NULL);
  t1=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
  fflush(stdout);
#endif

  /* Get the maximum noise level */
  maxval=d->noise.Re[0];
  for (i=1;i<nr;i++) if (d->noise.Re[i] > maxval) maxval=d->noise.Re[i];
  for (i=0;i<nr;i++) if (d->noise.Im[i] > maxval) maxval=d->noise.Im[i];

  /* Find scale factors */
  if ((Rsf = (double *)malloc(nr*sizeof(double))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
  if ((Isf = (double *)malloc(nr*sizeof(double))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
  for (i=0;i<nr;i++) Rsf[i] = maxval/d->noise.Re[i];
  for (i=0;i<nr;i++) Isf[i] = maxval/d->noise.Im[i];

  /* Scale to equalize noise */
  for (i=0;i<nr;i++) {
    for (j=0;j<dim3;j++) {
      for(k=0;k<dim2;k++) {
        for (l=0;l<dim1;l++) {
          d->data[i][j][k*dim1+l][0] *=Rsf[i];
          d->data[i][j][k*dim1+l][1] *=Isf[i];
        }
      }
    }
  }

  /* Set equalize flag */
  d->noise.equal=TRUE;

#ifdef DEBUG
  rtn=gettimeofday(&tp, NULL);
  t2=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
  fprintf(stdout,"  Noise data equalized: took %f secs\n",t2-t1);
  fflush(stdout);
#endif

}


void dccorrect(struct data *d)
{
  int dim1,dim2,dim3,nr;
  int start1,end1,start2,end2;
  double noisefrac;
  int dim1centre,dim2centre,dim1offset,dim2offset,dim3offset,ix;
  int i,j,k,l;
  int n;
  double redc,imdc;

#ifdef DEBUG
  struct timeval tp;
  double t1,t2;
  int rtn;
#endif

  /* Return if not raw data */
  if ((d->dimstatus[0] & FFT) || (d->dimstatus[1] & FFT) || (d->dimstatus[2] & FFT)) return;

#ifdef DEBUG
  fprintf(stdout,"\n%s: %s()\n",__FILE__,__FUNCTION__);
  rtn=gettimeofday(&tp, NULL);
  t1=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
  fflush(stdout);
#endif

  /* Data dimensions */
  dim1=d->np/2; dim2=d->nv; dim3=d->endpos-d->startpos; nr=d->nr;

  /* Noise fraction */
  noisefrac=Knoisefraction;

  /* For 3D data block check to see if we should sample noise */
  if (im3D(d)) { /* 3D data */
    /* Default start1, end1, start2, end2 for a single processing block */
    dim3offset = (int)((d->nv2*noisefrac)/2.0);
    if (!dim3offset) dim3offset = 1;
    /* For shifted FID the noise is at centre */
    /* Set the centre accordingly */
    if ((!(d->dimstatus[2] & FFT) && (d->dimstatus[2] & SHIFT))) {
      start1=d->nv2/2-dim3offset;
      end1=d->nv2/2+dim3offset;
      start2=0; end2=0;
      /* See if we should skip the processing block */
      if ((d->startpos>=end1) || (d->endpos<start1)) return;
      /* Adjust start1, end1 for the processing block */
      if (d->startpos<start1) start1-=d->startpos;
      else start1=0;
      if (d->endpos<end1) end1=d->endpos-d->startpos;
      else end1-=d->startpos;
    } else {
      start1=0;
      end1=dim3offset;
      start2=d->nv2-dim3offset; 
      end2=d->nv2;
      /* See if we should skip the processing block */
      if ((d->startpos>=end1) && (d->endpos<start2)) return;
      /* Figure the start1, end1, start2, end2 for the processing block */
      if (d->startpos<end1) {
        if (d->endpos<end1) {
          end1=d->endpos-d->startpos;
          end2=0;
        } else {
          end1-=d->startpos;
        }
      } else {
        end1=0;
      }
      if (d->endpos>=start2) {
        start2-=d->startpos;
        if (start2<0) start2=0;
        end2=d->endpos-d->startpos;
      } else {
        end2=0;
      }
      if (!end2) start2=0;
    }
#ifdef DEBUG
  fprintf(stdout,"  Sampling data block %d (start1 = %d, end1 = %d, start2 = %d, end2 = %d)\n",d->block+1,start1,end1,start2,end2);
  fflush(stdout);
#endif
  } else { /* 2D data */
    start1=0; end1=dim3;
    start2=0; end2=0;    /* Just skip the loop from start2 to end2 */
  }

  /* For shifted FID the noise is at centre */
  /* Set the centre accordingly */
  if ((!(d->dimstatus[0] & FFT) && (d->dimstatus[0] & SHIFT))) {
    dim1centre=dim1/2;
  } else {
    dim1centre=dim1;
  }
  if ((!(d->dimstatus[1] & FFT) && (d->dimstatus[1] & SHIFT))) {
    dim2centre=dim2/2;
  } else {
    dim2centre=dim2;
  }

  /* Set the offset for the specified range */
  dim1offset = (int)((dim1*noisefrac)/2.0);
  dim2offset = (int)((dim2*noisefrac)/2.0);
  if (!dim1offset) dim1offset = 1;
  if (!dim2offset) dim2offset = 1;

  for (i=0;i<nr;i++) {
    n=0;
    redc=0.0;
    imdc=0.0;
    for (j=start1;j<end1;j++) {
      for(k=0;k<2*dim2offset;k++) {
        for (l=0;l<2*dim1offset;l++) {
          ix=(k+dim2centre-dim2offset)%dim2;
          ix=ix*dim1+(l+dim1centre-dim1offset)%dim1;
          redc+=d->data[i][j][ix][0];
          imdc+=d->data[i][j][ix][1];
          n++;
        }
      }
    }
    for (j=start2;j<end2;j++) {
      for(k=0;k<2*dim2offset;k++) {
        for (l=0;l<2*dim1offset;l++) {
          ix=(k+dim2centre-dim2offset)%dim2;
          ix=ix*dim1+(l+dim1centre-dim1offset)%dim1;
          redc+=d->data[i][j][ix][0];
          imdc+=d->data[i][j][ix][1];
          n++;
        }
      }
    }
    redc/=n;
    imdc/=n;
    for (j=0;j<dim3;j++) {
      for(k=0;k<dim2;k++) {
        for (l=0;l<dim1;l++) {
          d->data[i][j][k*dim1+l][0]-=redc;
          d->data[i][j][k*dim1+l][1]-=imdc;
        }
      }
    }
  }

#ifdef DEBUG
  rtn=gettimeofday(&tp, NULL);
  t2=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
  fprintf(stdout,"  DC correction using noise region in data: took %f secs\n",t2-t1);
  fflush(stdout);
#endif

}

void scaledata(struct data *d,double factor)
{
  int dim1,dim2,dim3,nr;
  int i,j,k,l;

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

  /* Scale data */
  for (i=0;i<nr;i++) {
    for (j=0;j<dim3;j++) {
      for(k=0;k<dim2;k++) {
        for (l=0;l<dim1;l++) {
          d->data[i][j][k*dim1+l][0] *=factor;
          d->data[i][j][k*dim1+l][1] *=factor;
        }
      }
    }
  }

#ifdef DEBUG
  rtn=gettimeofday(&tp, NULL);
  t2=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
  fprintf(stdout,"  Data scaled by %f: took %f secs\n",factor,t2-t1);
  fflush(stdout);
#endif

  /* Zero noise data */
  zeronoise(d);

}
