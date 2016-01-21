/* multiblock3D.c */
/*---------------------------------------------------------------------------*/
/*                                                                           */
/* multiblock3D.c: default multi-block 3D recon                              */
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

void multiblock3D(struct data *d)
{
  struct dimstatus status;                /* Define status to have dimstatus structure */

  struct data nav;                        /* Define nav to have data structure */

  nulldata(&nav);                         /* Null the navigator data structure */

  setnvols(d);                            /* Set the number of data volumes */

  dimorder3D(d);                          /* Sort ascending slice and phase order */

  if (d->nav) initdatafrom(d,&nav);       /* If there are navigator scans initialize navigator from data */

  /* Loop over requested volumes in data file */
  for (d->vol=d->startvol;d->vol<d->endvol;d->vol++) {

    if (interupt) return;                 /* Interupt/cancel from VnmrJ */

    zeronoise(d);                         /* Zero any noise measurement */

    /* Loop over data blocks */
    for (d->block=0;d->block<d->nblocks;d->block++) {

      getblock3D(d,d->vol,NDCC);          /* Get block without applying dbh.lvl and dbh.tlt */

      w3Dfdfs(d,VJ,FLT32,d->vol);         /* Write 3D fdf raw data from processing block */

      if (d->nr > 1) getnoise(d,STD);     /* Sample noise data if multiple receivers */

      clear2Ddata(d);                     /* Clear data processing block from memory */

    }

    /* Loop over data blocks */
    for (d->block=0;d->block<d->nblocks;d->block++) {

		if(interupt)return;

		getblock3D(d,d->vol,NDCC);          /* Get block without applying dbh.lvl and dbh.tlt */

      equalizenoise(d,STD);               /* Scale for equal noise in all receivers */

      if (d->nav) {                       /* If there are navigator scans */

  		if(interupt)return;
  		nav.block = d->block;             /* Set navigator block */

		if(interupt)return;
		getnavblock3D(&nav,d->vol,NDCC);  /* get the navigator block */

		if(interupt)return;
		shiftdata2D(&nav,READ);           /* Shift navigator data in READ for FT */

		if(interupt)return;
		weightdata2D(&nav,REFREAD);       /* Weight navigator data in READ using reference parameters */

		if(interupt)return;
		zerofill2D(&nav,READ);            /* Zero fill navigator data in READ using standard parameters */

		if(interupt)return;
		fft2D(&nav,READ);                 /* FT navigator data in READ */

		if(interupt)return;
		shiftdata2D(d,READ);              /* Shift data in READ for FT */

		if(interupt)return;
		weightdata2D(d,READ);             /* Weight data in READ */

		if(interupt)return;
		zerofill2D(d,READ);               /* Zero fill data in READ using standard parameters */

		if(interupt)return;
		fft2D(d,READ);                    /* FT data in READ */

		if(interupt)return;
		navcorr(d,&nav);                  /* Correct data using navigators */

		if(interupt)return;
		shiftdata2D(d,READ);              /* Shift data in READ to get profiles */

        clear2Ddata(&nav);                /* Clear navigator data from memory */

		if(interupt)return;
		shiftdata2D(d,PHASE);             /* Shift data in PHASE for FT */

		if(interupt)return;
		phaseramp2D(d,PHASE);             /* Phase ramp the data to correct for phase encode offset ppe */

		if(interupt)return;
		weightdata2D(d,PHASE);            /* Weight data in PHASE */

		if(interupt)return;
		zerofill2D(d,PHASE);              /* Zero fill data in PHASE using standard parameters */

		if(interupt)return;
		fft2D(d,PHASE);                   /* FT data in PHASE */

		if(interupt)return;
		shiftdata2D(d,PHASE);             /* Shift data in PHASE to get images */

      } else {

  		if(interupt)return;
  		shiftdata2D(d,STD);               /* Shift FID data for 2D FT */

		if(interupt)return;
		phaseramp2D(d,PHASE);             /* Phase ramp the data to correct for phase encode offset ppe */

		if(interupt)return;
		weightdata2D(d,STD);              /* Weight data using standard parameters */

		if(interupt)return;
		zerofill2D(d,STD);                /* Zero fill data using standard parameters */

		if(interupt)return;
		fft2D(d,STD);                     /* 2D FT */

		if(interupt)return;
		shiftdata2D(d,STD);               /* Shift data to get images */

      }

		if(interupt)return;
		phasedata2D(d,VJ);                  /* Phase data in dim1*dim2 if required */

		if(interupt)return;
		wrawbin3D(d,D12,CX,DBL64);          /* Write complex raw data from "D12" processing block */

      clear2Ddata(d);                     /* Clear data processing block from memory */

    }

    /* Loop over data blocks */
    for (d->block=0;d->block<d->nblocks;d->block++) {
		if(interupt)return;

      setdimstatus(d,&status,d->block);   /* If 1st block store dimstatus, otherwise refresh */

      rrawbin3D(d,D12,CX,DBL64);          /* Read complex "D12" processing block into "D3" processing block */

      shiftdata1D(d,STD,D3);              /* Shift FID data for 1D dim3 FT */

      phaseramp1D(d,PHASE2);              /* Phase ramp the data to correct for phase encode 2 offset ppe2 */

      weightdata1D(d,STD,D3);             /* Weight data in dim3 using standard VnmrJ parameters */

      zerofill1D(d,STD,D3);               /* Zero fill data in dim3 using standard VnmrJ parameters */

      fft1D(d,D3);                        /* 1D dim3 FT */

      phasedata1D(d,VJ,D3);               /* Phase data in dim3 if required */

      shiftdata1D(d,STD,D3);              /* Shift data in dim3 to get image */

      wrawbin3D(d,D3,CX,DBL64);           /* Write complex raw data from "D3" processing block */

      cleardim3data(d);                   /* Clear data processing block from memory */

    }

    delrawbin3D(d,D12,CX);                /* Delete the raw data we have finished with */

    /* Loop over data blocks */
    for (d->block=0;d->block<d->nblocks;d->block++) {
		if(interupt)return;

      rrawbin3D(d,D3,CX,DBL64);           /* Read complex "D3" processing block into "D12" processing block */

      w3Dfdfs(d,VJ,FLT32,d->vol);         /* Write 3D fdf image data from volume */

      wnifti(d,VJ,FLT32,d->vol);          /* Write NIFTI-1/Analyze7.5 data */

      clear2Ddata(d);                     /* Clear data volume from memory */

    }

    delrawbin3D(d,D3,CX);                 /* Delete the raw data we have finished with */

  } /* end of volume for loop */

  if (d->nav) clear2Dall(&nav);           /* If there are navigator scans clear everything from memory */

  clear2Dall(d);                          /* Clear everything from memory */

  exit(0);

}
