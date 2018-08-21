/* reconCSI3D.c */
/*---------------------------------------------------------------------------*/
/*                                                                           */
/* reconCSI3D.c for 3D CSI                                                   */
/*                                                                           */
/* Copyright (C) 2012 Margaret Kritzer                                       */
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

void reconCSI3D(struct data *d) {
	struct datablockhead *dbh;
	int i, j, k, dim1, dim2, dim3,dim4, nr;
	int startd1, startd2, cropd1, cropd2;
	int startd3, cropd3;
	int fn1, fn2, fn3;
	int tsize;

	/* Open fid file pointer for writing */
	openfpw(d, FID_FILE);

	/* Set data dimensions */
	dim1 = d->np / 2;
	dim2 = d->nv;
	dim3 = d->nv2;
	dim4 = d->nv3;
	nr = d->nr;
	setnvols(d); /* Set the number of data volumes */
	dimorderCSI3D(d); /* sorts or just reads lists depending on undersample param */


	/* Allocate memory for a single block header to be reused */
	if ((dbh = malloc( sizeof(d->bh))) == NULL)
		nomem(__FILE__, __FUNCTION__, __LINE__);

	getdbh(d,0); // read the first block header
	copydbh(&d->bh, dbh);

	// memory for 3D
	if ((d->csi_data = (fftw_complex **) fftw_malloc(nr
			* sizeof(fftw_complex *))) == NULL)
		nomem(__FILE__, __FUNCTION__, __LINE__);

	tsize=dim1*dim2*dim3*dim4;
	// adjust for zerofilling
	fn1 = d->fn1;
	fn2 = d->fn2;
	fn3 = d->fn3;
	if (spar(d, "epsi", "y")) {
		if (fn1) {
			fn1 /= 4;
			fn1 *= 4;
			d->fn1 = fn1;
			fn1 /= 2;
			tsize *= fn1;
			tsize /= dim2;
		}
		if (fn2) {
			fn2 /= 4;
			fn2 *= 4;
			d->fn2 = fn2;
			fn2 /= 2;
			tsize *= fn2;
			tsize /= dim1;
		}
		if (fn3) {
			fn3 /= 4;
			fn3 *= 4;
			d->fn3 = fn3;
			fn3 /= 2;
			tsize *= fn3;
			tsize /= dim3;
		}
	} else {
		if (fn1) {
			fn1 /= 4;
			fn1 *= 4;
			d->fn1 = fn1;
			fn1 /= 2;
			tsize *= fn1;
			tsize /= dim2;
		}
		if (fn2) {
			fn2 /= 4;
			fn2 *= 4;
			d->fn2 = fn2;
			fn2 /= 2;
			tsize *= fn2;
			tsize /= dim3;
		}
		if (fn3) {
			fn3 /= 4;
			fn3 *= 4;
			d->fn3 = fn3;
			fn3 /= 2;
			tsize *= fn3;
			tsize /= dim4;
		}
	}

	d->csi_data[0] = NULL;
	d->csi_data[0] = (fftw_complex *) fftw_malloc(tsize * sizeof(fftw_complex));
	if (d->csi_data[0] == NULL)
		nomem(__FILE__, __FUNCTION__, __LINE__);
	(void)memset(d->csi_data[0],0.0,tsize*sizeof(fftw_complex));

	if (spar(d, "epsi", "y")) // read in all data and regrid first
	{
		for (d->vol = 0; d->vol < d->nvols; d->vol++) { // slice encodes
			  if (interupt) return;           /* Interupt/cancel from VnmrJ */

			d->np = 2*dim1;
			d->nv=dim2;
			d->nv2=dim3;
			d->nv3=dim4;
			getblockCSI3D(d, d->vol, NDCC); /* Get 2D block without applying dbh.lvl and dbh.tlt */
		}
		(void) regridEPSI(d);
	}


	checkCrop3D(d); // make sure cropping is legal

	/* Write fid file header */
	wdfhCSI(d, FID_FILE);


//	setnvols(d); /* Set the number of data volumes */
//    dimorderCSI3D(d); /* sorts or just reads lists depending on undersample param */

	/* Do the in-plane processing first */
	for (d->vol = 0; d->vol < d->nvols; d->vol++) { // this should be slice-encodes

		  if (interupt) return;           /* Interupt/cancel from VnmrJ */

	    if(!spar(d,"epsi","y")){
	    	/* restore original data dimensions */
			d->np = 2 * dim1;
			d->nv = dim2;
			d->nv2 = dim3;
			d->nv3 = dim4;
			getblockCSI3D(d, d->vol, NDCC); /* Get 2D block without applying dbh.lvl and dbh.tlt */
	    }

		 if (!spar(d, "recon", "off")) {

			shiftdata2D(d, SPATIAL); /* Shift FID data for 2D dim1*dim2 FT */

			phaseramp2DCSI(d, d->vol, PHASE); /* Phase ramp the data to correct for phase encode offset ppe */

			phaseramp2DCSI(d, d->vol, PHASE2); /* Phase ramp the data to correct for phase encode offset ppe 2*/

			weightdata2D(d, SPATIAL); /* Weight data in dim2*dim3 using standard VnmrJ parameters */

		     fft2D(d, SPATIAL); /* 2D FT */

			 shiftdata2D(d, SPATIAL); /* Shift data in dim1*dim2 to get 2D images */
		}

	} /* end of volume loop */
	
	 if (!spar(d, "recon", "off")) {  // process 3rd dimension if required

	    shiftdatadim3(d, D12, SPATIAL); /* Shift FID data for 1D dim3 FT */

 		phaserampdim3CSI(d, PHASE3); /* Phase ramp the data to correct for phase encode 2 offset ppe2 */

	    weightdatadim3(d, SPATIAL); /* Weight data in dim3 using standard VnmrJ parameters */

		 fftdim3(d); /* 1D dim3 fft */

		// NOT YET phasedatadim3(d,VJ);                  /* Phase data in dim3 if required */

		  shiftdatadim3(d, D12, SPATIAL); /* Shift data in dim3 to get image */
	}


	startd1=d->startd1;
	startd2=d->startd1;
	cropd1=d->cropd1;
	cropd2=d->cropd2;
	startd3=d->startd3;
	cropd3=d->cropd3;

	copydbh(dbh, &d->bh);
	for(k=startd3; k< startd3+cropd3;k++) {
				for (i = startd2; i< startd2+cropd2; i++) {
					for(j=startd1;j<startd1+cropd1;j++) {// these are voxels
						wdbh(d, FID_FILE); /* Write block header */
						w2DCSI(d, 0, k, i,j, FID_FILE); /* Write block which is a single fid */
					}
				}
			}

	clear2Dall(d); /* Clear everything from memory */
	closefp(d, FID_FILE);
}
