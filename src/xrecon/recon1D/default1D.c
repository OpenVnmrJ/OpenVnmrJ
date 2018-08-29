/* default1D.c */
/*---------------------------------------------------------------------------*/
/*                                                                           */
/* default1D.c: 1D recon                                                     */
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

void default1D(struct data *d)
{
  struct file fref;
  struct data ref;
  struct datablockhead *dbh;
  int dim1,dim2,dim3,nr;
  int i,j;
  int wref=FALSE;

  /* Open data and phasefile file pointers for writing */
  openfpw(d,DATA_FILE);
  openfpw(d,PHAS_FILE);

  /* Write data and phasefile file headers */
  wdfh(d,DATA_FILE);
  wdfh(d,PHAS_FILE);

  /* Set data dimensions */
  dim1=d->np/2; dim2=1; dim3=d->fh.ntraces; nr=d->nr;

  /* Set number of "volumes" */
  d->nvols=d->fh.nblocks/nr;

  /* Check if there is a water reference */
  if (spar(d,"ws","y") && spar(d,"wref","y") && spar(d,"wrefstatus","ws")) wref=TRUE;

  /* Prepare water reference */
  if (wref) {
    setreffile(&fref,d,"wrefname"); /* Set reference file */
    getpars(fref.procpar[0],&ref);  /* Get pars from reference procpar */
    opendata(fref.fid[0],&ref);     /* Open reference data file fid */
    setdatapars(&ref);              /* Set data structure parameters */
    ref.nvols=1;                    /* Set one "volumes" */
    getblock1D(&ref,0,NDCC);        /* Get block without applying dbh.lvl and dbh.tlt */
    weightdata1D(&ref,STD,D1);      /* Weight data using standard VnmrJ parameters */
  }

  /* Allocate memory for blocks headers from all receivers */
  if ((dbh = malloc(nr*sizeof(d->bh))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);

  /* For spectra we anticipate there is easily sufficient memory for data
     from all receiver blocks */
  for (d->vol=0;d->vol<d->nvols;d->vol++) { /* loop over "volumes" */

    if (interupt) return;           /* Interupt/cancel from VnmrJ */

    for (i=0;i<nr;i++) { /* loop over receivers */
      getdbh(d,nr*d->vol+i);        /* Get block header */
      copydbh(&d->bh,&dbh[i]);      /* Store the block headers for writing */
    }

    getblock1D(d,d->vol,NDCC);      /* Get data block without applying dbh.lvl and dbh.tlt */

    weightdata1D(d,STD,D1);         /* Weight data using standard VnmrJ parameters */

    if (wref) refcorr1D(d,&ref);    /* Phase correct using the reference */

    else combine1D(d);              /* Combine data from multiple receivers */

    zerofill1D(d,STD,D1);           /* Zero fill data using standard VnmrJ parameters */

    fft1D(d,D1);                    /* 1D FT */

    shiftdata1D(d,STD,D1);          /* Shift data to get spectra */

    for (i=0;i<nr;i++) {            /* loop over receivers */
      copydbh(&dbh[i],&d->bh);      /* Copy block header for writing */
      for (j=0;j<dim3;j++) {
        d->bh.index=d->vol*nr*dim3+i*dim3+j; /* Set block index */
        wdbh(d,DATA_FILE);            /* Write block header */
        wdbh(d,PHAS_FILE);            /* Write block header */
        w1Dtrace(d,i,j,DATA_FILE);    /* Write block */
        w1Dtrace(d,i,j,PHAS_FILE);    /* Write block */
      }
    }

    clear1Ddata(d);               /* Clear data "volume" from memory */

  }

  closefp(d,DATA_FILE);
  closefp(d,PHAS_FILE);

  free(dbh);

  clear1Dall(d);                /* Clear everything from memory */

}

void refcorr1D(struct data *d,struct data *ref)
{
  int dim1,dim2,dim3,nr;
  int i,j;
  double re,im,M,phase;

  /* Set data dimensions */
  dim1=d->np/2; dim2=1; dim3=d->fh.ntraces; nr=d->nr;

  for (i=0;i<nr;i++) { /* loop over receivers */
    for (j=0;j<dim1;j++) { /* loop over data points */
      re=ref->data[i][0][j][0];
      im=ref->data[i][0][j][1];
      phase=atan2(im,re);

      re=d->data[i][0][j][0];
      im=d->data[i][0][j][1];
      M = sqrt(re*re + im*im);
      d->data[i][0][j][0]=M*cos(atan2(im,re)-phase);
      d->data[i][0][j][1]=M*sin(atan2(im,re)-phase);
      if (i>0) {
        d->data[0][0][j][0]+=d->data[i][0][j][0];
        d->data[0][0][j][1]+=d->data[i][0][j][1];
      }
    }
  }
}

void combine1D(struct data *d)
{
  int dim1,dim2,dim3,nr;
  int i,j,npts;
  double re,im,M,phase,*recphase;

  /* Set data dimensions */
  dim1=d->np/2; dim2=1; dim3=d->fh.ntraces; nr=d->nr;

  /* Return if there is nothing to do */
  if (nr<2) return;

  /* Allocate memory for receiver phases */
  if ((recphase = malloc(nr*sizeof(double))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);

  /* Look at first 16 pairs of points */
  npts=16;

  for (i=0;i<nr;i++) recphase[i]=0.0;

  for (j=1;j<=npts;j++) { /* skip 1st point */
    re=d->data[0][0][j][0];
    im=d->data[0][0][j][1];
    phase=atan2(im,re);
    for (i=1;i<nr;i++) { /* loop over receivers */
      re=d->data[i][0][j][0];
      im=d->data[i][0][j][1];
      recphase[i]+=atan2(im,re)-phase;
    }
  }

  for (i=0;i<nr;i++) recphase[i]/=npts;

  /* Correct different receiver phases */
  for (i=1;i<nr;i++) { /* loop over receivers */
    for (j=0;j<dim1;j++) {
      re=d->data[i][0][j][0];
      im=d->data[i][0][j][1];
      M = sqrt(re*re + im*im);
      phase=atan2(im,re);
      d->data[i][0][j][0]=M*cos(phase-recphase[i]);
      d->data[i][0][j][1]=M*sin(phase-recphase[i]);
      d->data[0][0][j][0]+=d->data[i][0][j][0];
      d->data[0][0][j][1]+=d->data[i][0][j][1];
    }
  }

  free(recphase);
}
