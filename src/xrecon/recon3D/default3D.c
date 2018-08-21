/* default3D.c */
/*---------------------------------------------------------------------------*/
/*                                                                           */
/* default3D.c: default 3D recon                                             */
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

void default3D(struct data *d)
{

  /* If requested, process in multiple data blocks (useful for big data) */
  if (d->nblocks > 1) multiblock3D(d);

  /* Otherwise, just process in a single data block ... */

  struct data nav;                        /* Define nav to have data structure */

  nulldata(&nav);                         /* Null the navigator data structure */

  setnvols(d);                            /* Set the number of data volumes */

  dimorder3D(d);                          /* Sort ascending slice and phase order */

  if (d->nav) initdatafrom(d,&nav);       /* If there are navigator scans initialize navigator from data */

  /* Loop over requested volumes in data file */
  for (d->vol=d->startvol;d->vol<d->endvol;d->vol++) {

    if (interupt) return;                 /* Interupt/cancel from VnmrJ */

    getblock3D(d,d->vol,NDCC);            /* Get block without applying dbh.lvl and dbh.tlt */

    w3Dfdfs(d,VJ,FLT32,d->vol);           /* Write 3D fdf raw data from processing block */

    zeronoise(d);                         /* Zero any noise measurement */

    equalizenoise(d,STD);                 /* Scale for equal noise in all receivers */

    if (d->nav) {                         /* If there are navigator scans */

      nav.block = d->block;               /* Set navigator block */

		if(interupt)return;
      getnavblock3D(&nav,d->vol,NDCC);    /* get the navigator block */

		if(interupt)return;
		shiftdata2D(&nav,READ);             /* Shift navigator data in READ for FT */

		if(interupt)return;
		weightdata2D(&nav,REFREAD);         /* Weight navigator data in READ using reference parameters */

		if(interupt)return;
      zerofill2D(&nav,READ);              /* Zero fill navigator data in READ using standard parameters */

		if(interupt)return;
		fft2D(&nav,READ);                   /* FT navigator data in READ */

		if(interupt)return;
		shiftdata2D(d,READ);                /* Shift data in READ for FT */

		if(interupt)return;
		weightdata2D(d,READ);               /* Weight data in READ */

		if(interupt)return;
		zerofill2D(d,READ);                 /* Zero fill data in READ using standard parameters */

		if(interupt)return;
		fft2D(d,READ);                      /* FT data in READ */

		if(interupt)return;
		navcorr(d,&nav);                    /* Correct data using navigators */

		if(interupt)return;
		shiftdata2D(d,READ);                /* Shift data in READ to get profiles */

      clear2Ddata(&nav);                  /* Clear navigator data from memory */

		if(interupt)return;
		shiftdata2D(d,PHASE);               /* Shift data in PHASE for FT */

		if(interupt)return;
		phaseramp2D(d,PHASE);               /* Phase ramp the data to correct for phase encode offset ppe */

		if(interupt)return;
		weightdata2D(d,PHASE);              /* Weight data in PHASE */

		if(interupt)return;
		zerofill2D(d,PHASE);                /* Zero fill data in PHASE using standard parameters */

		if(interupt)return;
		fft2D(d,PHASE);                     /* FT data in PHASE */

		if(interupt)return;
		shiftdata2D(d,PHASE);               /* Shift data in PHASE to get images */

    } else {

		    if (interupt)return;
			shiftdata2D(d, STD); /* Shift FID data for 2D dim1*dim2 FT */

			if(interupt)return;
			phaseramp2D(d, PHASE); /* Phase ramp the data to correct for phase encode offset ppe */

			if(interupt)return;
			weightdata2D(d, STD); /* Weight data in dim1*dim2 using standard VnmrJ parameters */

			if(interupt)return;
			zerofill2D(d, STD); /* Zero fill data in dim1*dim2 using standard VnmrJ parameters */

			if(interupt)return;
			fft2D(d, STD); /* 2D dim1*dim2 FT */

			if(interupt)return;
			shiftdata2D(d, STD); /* Shift data in dim1*dim2 to get 2D images */

		}
	if(interupt)return;
    phasedata2D(d,VJ);                    /* Phase data in dim1*dim2 if required */

	if(interupt)return;
    shiftdatadim3(d,D12,STD);             /* Shift FID data for 1D dim3 FT */

	if(interupt)return;
	phaserampdim3(d,PHASE2);              /* Phase ramp the data to correct for phase encode 2 offset ppe2 */

	if(interupt)return;
    weightdatadim3(d,STD);                /* Weight data in dim3 using standard VnmrJ parameters */

	if(interupt)return;
	zerofilldim3(d,STD);                  /* Zero fill data in dim3 using standard VnmrJ parameters */

	if(interupt)return;
	fftdim3(d);                           /* 1D dim3 fft */

	if(interupt)return;
	phasedatadim3(d,VJ);                  /* Phase data in dim3 if required */

	if(interupt)return;
	shiftdatadim3(d,D12,STD);             /* Shift data in dim3 to get image */

	if(interupt)return;
	w3Dfdfs(d,VJ,FLT32,d->vol);           /* Write 3D fdf image data from volume */

	if(interupt)return;
	wnifti(d,VJ,FLT32,d->vol);            /* Write NIFTI-1/Analyze7.5 data */

    clear2Ddata(d);                       /* Clear data volume from memory */

  } /* end of volume for loop */

  if (d->nav) clear2Dall(&nav);           /* If there are navigator scans clear everything from memory */

  clear2Dall(d);                          /* Clear everything from memory */

}
