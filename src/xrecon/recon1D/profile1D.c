/* profile1D.c */
/*---------------------------------------------------------------------------*/
/*                                                                           */
/* profile1D.c: 1D profile recon                                             */
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

void profile1D(struct data *d)
{
  int dim1,dim2,dim3,nr;
  int i,j;
  struct datablockhead *dbh;

  /* Open data and phasefile file pointers for writing */
  openfpw(d,DATA_FILE);
  openfpw(d,PHAS_FILE);

  /* Write data and phasefile file headers */
  wdfh(d,DATA_FILE);
  wdfh(d,PHAS_FILE);

  /* Set data dimensions */
  dim1=d->np/2; dim2=1; dim3=d->fh.ntraces; nr=d->nr;

  /* Set nuber of "volumes" */
  d->nvols=d->fh.nblocks/nr;

  /* Allocate memory for blocks headers from all receivers */
  if ((dbh = malloc(nr*sizeof(d->bh))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);

  /* For profiles we anticipate there is easily sufficient memory for data
     from all receiver blocks */
  for (d->vol=0;d->vol<d->nvols;d->vol++) { /* loop over "volumes" */

    for (i=0;i<nr;i++) { /* loop over receivers */
      getdbh(d,nr*d->vol+i);        /* Get block header */
      copydbh(&d->bh,&dbh[i]);      /* Store the block headers for writing */
    }

    getblock1D(d,d->vol,NDCC);      /* Get data block without applying dbh.lvl and dbh.tlt */

    shiftdata1D(d,STD,D1);          /* Shift FID data for fft */

    weightdata1D(d,STD,D1);         /* Weight data using standard VnmrJ parameters */

    zerofill1D(d,STD,D1);           /* Zero fill data using standard VnmrJ parameters */

    fft1D(d,D1);                    /* 1D fft */

    shiftdata1D(d,STD,D1);          /* Shift data to get profiles */

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

  clear1Dall(d);                /* Clear everything from memory */

  closefp(d,DATA_FILE);
  closefp(d,PHAS_FILE);

}
