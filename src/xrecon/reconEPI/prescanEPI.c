/* prescanEPI.c */
/*---------------------------------------------------------------------------*/
/*                                                                           */
/* prescanEPI.c: EPI prescan recon                                           */
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

void prescanEPI(struct data *d)
{
  int dim1,dim2,dim3,nr;
  double oversample;
  int nread;
  int rampsamp=FALSE,linearsamp=FALSE;

  setnvolsEPI(d);                     /* Set the number of data volumes */

  d->nv=(int)*val("nphase",&d->p);    /* Set d->nv for dimorder2D */
  d->pssorder=sliceorder(d,d->ns,"pss"); /* Fill pssorder with the slice order */
  d->dim2order=phaseorder(d,d->nv,d->nv,"par_does_not_exist"); /* Set dim2order=-1 for sequential phase encode order */
  d->dim3order=phaseorder(d,d->nv,d->nv,"sgepelist"); /* Fill dim3order with the standard gradient echo phase encode order */
  d->nv2=d->nv;                       /* Set d->nv2 for dim3order */
  d->nv=(int)*val("nseg",&d->p);      /* Use d->nv for the number of shots */

  /* Set data dimensions */
  dim3=d->endpos-d->startpos; nr=d->nr;

  /* Check for oversampling */
  oversample=*val("oversample",&d->p);
  if (spar(d,"rampsamp","y")) rampsamp=TRUE;
  if (spar(d,"linearsamp","y")) linearsamp=TRUE;
  nread=(int)*val("nread",&d->p)/2;

  for (d->vol=0;d->vol<d->nvols;d->vol++) { /* loop over "volumes" */

    if (interupt) return;           /* Interupt/cancel from VnmrJ */

    /* Loop over data blocks */
    for (d->block=0;d->block<d->nblocks;d->block++) {

      getblockEPI(d,d->vol,NDCC);   /* Get block without applying dbh.lvl and dbh.tlt */
      zeromax(d);                   /* Zero max structure & coordinates of maximum */
      zeronoise(d);                 /* Zero values in noise structure */

      setblockEPI(d);

      /* Set data dimensions */
      dim1=d->np/2; dim2=d->nv;

      if (vnmrj_recon) settep(d,STD);

      /* If oversampled or ramp and linear sampling, zoom to get the requested matrix size */
      if ((oversample==1) && rampsamp && linearsamp) zoomdata2D(d,(dim1-nread)/2,nread,0,dim2);
      else if ((oversample>1) || (rampsamp && linearsamp)) zoomdata2D(d,(dim1-1.5*nread)/2,1.5*nread,0,dim2);

      /* Flag the data as IMAGE since magnitude output is combined from multiple receivers */
      d->dimstatus[0]+=FFT;

      /* We always want to view prescan with same orientation so fix it as though it's axial */
      setval(&d->p,"psi",180.0);
      setval(&d->p,"phi",0.0);
      setval(&d->p,"theta",0.0);

      w2Dfdfs(d,VJ,FLT32,d->vol);   /* Write 2D fdf raw data from volume */

      /* Properly flag the data as FID */
      d->dimstatus[0]-=FFT;

      clear2Ddata(d);               /* Clear data volume from memory */
      setdim(d);                    /* Reset data dimensions in case data has been zerofilled (sets d->nv=1) */
      d->nv=*val("nseg",&d->p);     /* Use d->nv for the number of shots */
      d->dimstatus[0] = NONE;       /* Make sure ZEROFILL status is not set, otherwise setdim will be called in getblock() */
      d->dimstatus[1] = NONE;       /* Make sure ZEROFILL status is not set, otherwise setdim will be called in getblock() */

    }

  }

}

void settep(struct data *d, int mode)
{
  int dim1,dim2,dim3,nr;
  int **max;
  int i,j,k,l,n;
  int ix;
  int nseg,nnav,etl,altread,echo,oddcount,evencount;
  int steadyStates,flipLine;
  double re,im,M2,maxM,oddmax,evenmax,sw,tepadjust;
  char tepfile[MAXPATHLEN];
  FILE *f_out;

#ifdef DEBUG
  struct timeval tp;
  double t1,t2;
  int rtn;
  fprintf(stdout,"\n%s: %s()\n",__FILE__,__FUNCTION__);
  fprintf(stdout,"  Attempting to calculate correction for tep ...\n");
  rtn=gettimeofday(&tp, NULL);
  t1=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
  fflush(stdout);
#endif

  /* Set data dimensions */
  dim1=d->np/2; dim2=d->nv; dim3=d->endpos-d->startpos; nr=d->nr;

  /* Allow for arbitrary number of acquired steady state echoes (default 1) */
  steadyStates = (int)*val("acquiredSteadyStates",&d->p);
  if (parindex("acquiredSteadyStates",&d->p) < 0) steadyStates = 1;

  /* Allow for sequences with readout polarity set constant for the first acquired EPI echo,
     i.e. the set polarity depends on # navigators and # acquired steady states */
  flipLine = (int)*val("flipLine",&d->p);
  if (flipLine != 1) flipLine = 0;

  switch(mode) {
    case STD: /* Use maximum magnitude */

      /* If noise data does not exist set it to zero */
      if (!d->noise.data) zeronoise(d);
      /* If noise data is zero sample the noise */
      if (d->noise.zero) getnoise(d,STD);

      /* Set some defaults */
      zeromax(d);

      if ((max = (int **)malloc(nr*sizeof(int *))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
      for (i=0;i<nr;i++)
        if ((max[i] = (int *)malloc(dim2*sizeof(int))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);

      /* Get coordinates of maxima for odd and even echoes */
      for (i=0;i<nr;i++) {
        for (j=0;j<dim3;j++) {
          for (k=0;k<dim2;k++) {
            max[i][k]=-1;
            maxM=0.0;
            for (l=0;l<dim1;l++) {
              re=fabs(d->data[i][j][k*dim1+l][0]);
              im=fabs(d->data[i][j][k*dim1+l][1]);
              M2=re*re+im*im;
              if ((M2 > maxM) && (M2 > 4*d->noise.M2[i])) {
                maxM=M2;
                max[i][k]=l;
              }
            }
          }
        }
      }

      oddcount=0; oddmax=0.0;
      evencount=0; evenmax=0.0;
      nseg=(int)*val("nseg",&d->p);
      nnav=(int)*val("nnav",&d->p);
      etl=(int)*val("etl",&d->p);

      altread=spar(d,"altread","y");   /* alternating read gradient for alternate segments ... */
      if (nseg<2) altread=FALSE;       /* ... only if nseg>1 ... */
      if (nseg%2==0) altread=FALSE;    /* ... and only if nseg is odd */

      for (n=0;n<nseg;n++) {
        ix=n*(nnav+etl);
        for (i=0;i<nr;i++) {
          echo=0;
          if (altread && n%2) echo=1;
          for (k=0;k<nnav;k++) {
            if (max[i][k] > -1) {
              if (echo%2==flipLine) {
                oddmax += max[i][ix+k];
                oddcount++;
              } else {
                evenmax += max[i][ix+k];
                evencount++;
              }
            }
            echo++;
          }
          /* Skip the steady state readout gradients */
          echo += steadyStates;
          for (k=nnav;k<etl;k++) {
            if (max[i][k] > -1) {
              if (echo%2==flipLine) {
                oddmax += max[i][ix+k];
                oddcount++;
              } else {
                evenmax += max[i][ix+k];
                evencount++;
              }
            }
            echo++;
          }
        }
      } /* end nseg loop */

      oddmax /=oddcount;
      evenmax /=evencount;

      for (i=0;i<nr;i++) free(max[i]);
      free(max);

      sw=*val("sw",&d->p);
      tepadjust=0.5e6*(oddmax-evenmax)/sw;

      strcpy(tepfile,vnmrj_path);
      strcat(tepfile,"tepadjust");

      if ((f_out = fopen(tepfile, "w")) == NULL) {
        fprintf(stderr,"\n%s: %s()\n",__FILE__,__FUNCTION__);
        fprintf(stderr,"  Unable to write to %s\n",tepfile);
        fflush(stderr);
        return;
      }

      fprintf(f_out,"tepadjust %f",tepadjust);
      fclose(f_out);

#ifdef DEBUG
  rtn=gettimeofday(&tp, NULL);
  t2=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
  fprintf(stdout,"\n%s: %s()\n",__FILE__,__FUNCTION__);
  fprintf(stdout,"  Magnitude data suggests add %f us to value of tep\n",tepadjust);
  fprintf(stdout,"  Result written to file %s\n",tepfile);
  fflush(stdout);
#endif

      break;
    default:
      break;
  }
}
