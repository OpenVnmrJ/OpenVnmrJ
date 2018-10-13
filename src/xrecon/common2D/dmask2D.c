/* dmask2D.c */
/*---------------------------------------------------------------------------*/
/*                                                                           */
/* dmask2D.c: 2D routines for masking                                        */
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

/*-----------------------*/
/*---- Some defaults ----*/
/*-----------------------*/
/* mask2noise: Set mask according to noise level */
static int mask2noise=TRUE;

/* masknstds: # standard deviations above the mean noise to set the mask */
static double masknstds=22;

/* masklvl: Fraction of magnitude image to use as a mask */
static double masklvl=5;

/* maskrcvrs: for the mask of data from multiple receivers there must be signal 
              in at least maskrcvrs receivers */
static int maskrcvrs=3;

/* dfilldimfrac: Fraction of image space FOV to use for density filling */
static double dfilldimfrac=0.06;

/* dfillloops: Number of loops through the density filter */
static int dfillloops=1;

/* dfillfrac: Fraction of dfilldim sufficient to cause filling in density filter */
static double dfillfrac=0.55;

void get2Dmask(struct data *d,int mode)
{
  int dim1,dim2,ns,nr;
  int start,end;
  int startpos,endpos,blockslices;
  double masklevel;
  int i,j,k,l;
  int ix1;
  double re,im,M2,threshold,threshold2,stdev;
  int *p1;
  double *dp1;

#ifdef DEBUG
  struct timeval tp;
  double t1,t2;
  int rtn;
#endif

  /* Return if not IMAGE data and not shifted */
  if (!(d->dimstatus[0] & FFT)) return;
  if (!(d->dimstatus[0] & SHIFT)) return;
  if (!(d->dimstatus[1] & FFT)) return;
  if (!(d->dimstatus[1] & SHIFT)) return;

#ifdef DEBUG
  fprintf(stdout,"\n%s: %s()\n",__FILE__,__FUNCTION__);
  rtn=gettimeofday(&tp, NULL);
  t1=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
  fflush(stdout);
#endif

  /* Data dimensions */
  dim1=d->np/2; dim2=d->nv; ns=d->ns; nr=d->nr;

  masklevel=0.0;
  switch(mode) {
    case MK: /* Mask parameters */
      start=(int)*val("maskstartslice",&d->p);
      if ((start<0) || (start>ns)) start=1;
      start--;
      end=(int)*val("maskendslice",&d->p);
      if ((end<1) || (end>ns)) end=ns;
      if (spar(d,"masklvlmode","noise")) {
        mask2noise=TRUE;
        masklevel=*val("masklvlnoise",&d->p);
      } else {
        mask2noise=FALSE;
        masklevel=*val("masklvlmax",&d->p)/100.0;
      }
      maskrcvrs=(int)*val("maskrcvrs",&d->p);
      if (maskrcvrs<1) maskrcvrs=1;
      break;
    case SM: /* Mask parameters */
      start=0;
      end=ns;
      if (spar(d,"masklvlmode","noise")) {
        mask2noise=TRUE;
        masklevel=*val("masklvlnoise",&d->p);
      } else {
        mask2noise=FALSE;
        masklevel=*val("masklvlmax",&d->p)/100.0;
      }
      maskrcvrs=(int)*val("maskrcvrs",&d->p);
      if (maskrcvrs<1) maskrcvrs=1;
      break;
    default: /* Default parameters */
      start=0;
      end=ns;
      if (mask2noise) masklevel=masknstds;
      else masklevel=masklvl;
      break;
  } /* end mode switch */

  /* Adjust start and end according to data block */
  startpos=d->startpos; endpos=d->endpos;
  if ((start>endpos) || (end<startpos)) return;
  start=start-startpos;
  end=end-startpos;
  if (start<0) start=0;
  blockslices=endpos-startpos;
  if (end>blockslices) end=blockslices;

  /* Allocate memory for the mask */
  if ((d->mask = (int **)malloc((end-start)*sizeof(int *))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
  for (j=start;j<end;j++)
    if ((d->mask[j] = (int *)malloc(dim2*dim1*sizeof(int))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);

  /* Zero mask */
  for (j=start;j<end;j++) {
    p1 = d->mask[j];
    for(k=0;k<dim2*dim1;k++) *p1++ = 0;
  }

#ifdef DEBUG
  rtn=gettimeofday(&tp, NULL);
  t2=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
  fprintf(stdout,"  Blank mask generated: took %f secs\n",t2-t1);
  fflush(stdout);
#endif

  /* Set threshold */
  if (mask2noise) {
    /* If noise data does not exist set it to zero */
    if (!d->noise.data) zeronoise(d);
    /* If noise data is zero sample the noise */
    if (d->noise.zero) getnoise(d,mode);
    stdev=sqrt(d->noise.avM2-d->noise.avM*d->noise.avM);
    threshold=d->noise.avM+masklevel*stdev;
  } else {
    /* Get maximum if it has not been measured */
    if (!d->max.data) getmax(d);
    threshold=masklevel*d->max.Mval;
  }
  threshold2=threshold*threshold;

#ifdef DEBUG
  fprintf(stdout,"\n%s: %s()\n",__FILE__,__FUNCTION__);
  rtn=gettimeofday(&tp, NULL);
  t1=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
  fflush(stdout);
#endif

  /* Generate the mask */
  for (i=0;i<nr;i++) {
    for (j=start;j<end;j++) {
      dp1 = d->data[i][j][0];
      p1 = d->mask[j];
      for(k=0;k<dim2*dim1;k++) {
        re=*dp1++;
        im=*dp1++;
        M2=re*re+im*im;
        if (M2 > threshold2) p1[k] += 1;
      }
    }
  }

  /* Create a global mask if d->mask is generated from multiple receivers */
  if (nr > 1) {
    /* There must be signal in at least maskrcvrs receivers */
    maskrcvrs--;
    for (j=start;j<end;j++) {
      for(k=0;k<dim2;k++) {
        for (l=0;l<dim1;l++) {
          ix1=k*dim1+l;
          if (d->mask[j][ix1] > maskrcvrs) d->mask[j][ix1]=1;
          else d->mask[j][ix1]=0;
        }
      }
    }
    maskrcvrs++;
  }

#ifdef DEBUG
  rtn=gettimeofday(&tp, NULL);
  t2=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
  fprintf(stdout,"  Masked with threshold = %f: took %f secs\n",threshold,t2-t1);
  fflush(stdout);
#endif

}

void fill2Dmask(struct data *d,int mode)
{
  int dim1,dim2,ns,nr;
  int start,end;
  int startpos,endpos,blockslices;
  int i,j,k,l,m;
  int ix1;
  int total,N;
  int origtotal,origN;
  int LThdim,GThdim;
  int *newmask;
  int *p1,*p2;
  int dfilldim;

#ifdef DEBUG
  struct timeval tp;
  double t1,t2;
  int rtn;
#endif

  /* Return if there is no mask data */
  if (!d->mask) return;

  /* Data dimensions */
  dim1=d->np/2; dim2=d->nv; ns=d->ns; nr=d->nr;

  switch(mode) {
    case MK: /* Mask parameters */
      if (!(spar(d,"dfill","y"))) return;
      start=(int)*val("maskstartslice",&d->p);
      if ((start<0) || (start>ns)) start=1;
      start--;
      end=(int)*val("maskendslice",&d->p);
      if ((end<1) || (end>ns)) end=ns;
      dfilldimfrac=*val("dfilldim",&d->p)/100.0;
      dfillfrac=*val("dfillfrac",&d->p);
      dfillloops=(int)*val("dfillloops",&d->p);
      break;
    case SM: /* Sensitivity map parameters */
      if (!(spar(d,"dfill","y"))) return;
      start=0;
      end=ns;
      dfilldimfrac=*val("dfilldim",&d->p)/100.0;
      dfillfrac=*val("dfillfrac",&d->p);
      dfillloops=(int)*val("dfillloops",&d->p);
      break;
    default: /* Default parameters */
      start=0;
      end=ns;
      break;
  } /* end mode switch */

  /* Return if there is nothing to do */
  if (dfillloops<1) return;

  dfilldim=dfilldimfrac*dim1;
  if (dfilldim<2) return;

  /* Make sure dfilldim is less than half FOV in each direction */
  if (dfilldim > dim1/2) dfilldim = dim1/2;
  if (dfilldim > dim2/2) dfilldim = dim2/2;

  /* Make sure dfilldim is odd */
  if (dfilldim%2 == 0) dfilldim++;

#ifdef DEBUG
  fprintf(stdout,"\n%s: %s()\n",__FILE__,__FUNCTION__);
  fprintf(stdout,"  Mask dfilldim = %d ...\n",dfilldim);
  fflush(stdout);
#endif

  /* Adjust start and end according to data block */
  startpos=d->startpos; endpos=d->endpos;
  if ((start>endpos) || (end<startpos)) return;
  start=start-startpos;
  end=end-startpos;
  if (start<0) start=0;
  blockslices=endpos-startpos;
  if (end>blockslices) end=blockslices;

  /* Create storage for the new mask */
  if ((newmask = (int *)malloc(dim2*dim1*sizeof(int))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);

  LThdim=dfilldim/2;  /* as dfilldim is odd LThdim is less than half dfilldim */
  GThdim=LThdim+1;    /* GThdim is greater than half dfilldim */

  for (i=0;i<dfillloops;i++) {

#ifdef DEBUG
  rtn=gettimeofday(&tp, NULL);
  t1=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
  fflush(stdout);
#endif

    for (j=start;j<end;j++) {

      /* Make sure newmask is zero */
      p1 = newmask;
      for(k=0;k<dim2*dim1;k++) *p1++ = 0;

      /* create a running total for 1st corner */
      origtotal=0;
      origN=0;
      for(k=0;k<LThdim;k++) {
        p1 = d->mask[j]+k*dim1;
        for (l=0;l<LThdim;l++) {
          origtotal += *p1++;
          origN++;
        }
      }

      for (l=0;l<GThdim;l++) {

        total=origtotal;
        N=origN;
        p1 = d->mask[j]+l+LThdim;
        for (m=0;m<LThdim;m++) {
          p1 += m*dim1;
          total += *p1;
          N++;
        }
        origtotal=total;
        origN=N;

        for(k=0;k<GThdim;k++) {
          ix1=k*dim1+l;
          p1 = d->mask[j]+(k+LThdim)*dim1;
          for (m=0;m<l+GThdim;m++) {
            total += *p1++;
            N++;
          }
          if ((double)total/(double)N > dfillfrac) newmask[ix1]=1;
        }

        for(k=GThdim;k<dim2-LThdim;k++) {
          ix1=k*dim1+l;
          p1 = d->mask[j]+(k+LThdim)*dim1;
          for (m=0;m<l+GThdim;m++) total += *p1++;
          p1 = d->mask[j]+(k-GThdim)*dim1;
          for (m=0;m<l+GThdim;m++) total -= *p1++;
          if ((double)total/(double)N > dfillfrac) newmask[ix1]=1;
        }

        for(k=dim2-LThdim;k<dim2;k++) {
          ix1=k*dim1+l;
          p1 = d->mask[j]+(k-GThdim)*dim1;
          for (m=0;m<l+GThdim;m++) {
            total -= *p1++;
            N--;
          }
          if ((double)total/(double)N > dfillfrac) newmask[ix1]=1;
        }

      }

      for (l=GThdim;l<dim1-LThdim;l++) {

        total=origtotal;
        N=origN;
        p1 = d->mask[j]+l+LThdim;
        for (m=0;m<LThdim;m++) {
          p1 += m*dim1;
          total += *p1;
        }
        p1 = d->mask[j]+l-GThdim;
        for (m=0;m<LThdim;m++) {
          p1 += m*dim1;
          total -= *p1;
        }
        origtotal=total;
        origN=N;

        for(k=0;k<GThdim;k++) {
          ix1=k*dim1+l;
          p1 = d->mask[j]+(k+LThdim)*dim1+l-LThdim;
          for (m=0;m<dfilldim;m++) {
            total += *p1++;
            N++;
          }
          if ((double)total/(double)N > dfillfrac) newmask[ix1]=1;
        }

        for(k=GThdim;k<dim2-LThdim;k++) {
          ix1=k*dim1+l;
          p1 = d->mask[j]+(k+LThdim)*dim1+l-LThdim;
          for (m=0;m<dfilldim;m++) total += *p1++;
          p1 = d->mask[j]+(k-GThdim)*dim1+l-LThdim;
          for (m=0;m<dfilldim;m++) total -= *p1++;
          if ((double)total/(double)N > dfillfrac) newmask[ix1]=1;
        }

        for(k=dim2-LThdim;k<dim2;k++) {
          ix1=k*dim1+l;
          p1 = d->mask[j]+(k-GThdim)*dim1+l-LThdim;
          for (m=0;m<dfilldim;m++) {
            total -= *p1++;
            N--;
          }
          if ((double)total/(double)N > dfillfrac) newmask[ix1]=1;
        }

      }

      for (l=dim1-LThdim;l<dim1;l++) {

        total=origtotal;
        N=origN;
        p1 = d->mask[j]+l-GThdim;
        for (m=0;m<LThdim;m++) {
          p1 += m*dim1;
          total -= *p1;
          N--;
        }
        origtotal=total;
        origN=N;

        for(k=0;k<GThdim;k++) {
          ix1=k*dim1+l;
          p1 = d->mask[j]+(k+LThdim)*dim1+l-LThdim;
          for (m=l-LThdim;m<dim1;m++) {
            total += *p1++;
            N++;
          }
          if ((double)total/(double)N > dfillfrac) newmask[ix1]=1;
        }

        for(k=GThdim;k<dim2-LThdim;k++) {
          ix1=k*dim1+l;
          p1 = d->mask[j]+(k+LThdim)*dim1+l-LThdim;
          for (m=l-LThdim;m<dim1;m++) total += *p1++;
          p1 = d->mask[j]+(k-GThdim)*dim1+l-LThdim;
          for (m=l-LThdim;m<dim1;m++) total -= *p1++;
          if ((double)total/(double)N > dfillfrac) newmask[ix1]=1;
        }

        for(k=dim2-LThdim;k<dim2;k++) {
          ix1=k*dim1+l;
          p1 = d->mask[j]+(k-GThdim)*dim1+l-LThdim;
          for (m=l-LThdim;m<dim1;m++) {
            total -= *p1++;
            N--;
          }
          if ((double)total/(double)N > dfillfrac) newmask[ix1]=1;
        }

      }

      /* Copy newmask to mask */
      p1 = d->mask[j];
      p2 = newmask;
      for(k=0;k<dim2*dim1;k++) *p1++ = *p2++;

    } /* end ns for loop */

#ifdef DEBUG
  rtn=gettimeofday(&tp, NULL);
  t2=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
  fprintf(stdout,"  Density Filter, pass %d: took %f secs\n",i+1,t2-t1);
  fflush(stdout);
#endif

  } /* end dfillloops for loop */

  free(newmask);

}
