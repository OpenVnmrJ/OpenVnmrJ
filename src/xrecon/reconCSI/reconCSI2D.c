/* reconCSI2D.c */
/*---------------------------------------------------------------------------*/
/*                                                                           */
/* reconCSI2D.c for 2D CSI                                                   */
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

void reconCSI2D(struct data *d) {
	struct datablockhead *dbh;
	int i, j, dim1, dim2, dim3, nr;
	int ndim2, ndim3;
	int startd1, startd2, cropd1, cropd2;
	int s1, e1, s2, e2;

	/* Open fid file pointer for writing */
	openfpw(d, FID_FILE);

	/* Set data dimensions */
	dim1 = d->np / 2;
	dim2 = d->nv;
	dim3 = d->nv2;
	nr = d->nr;
	setnvols(d); /* Set the number of data volumes */

	dimorderCSI2D(d); /* Sort ascending slice and phase order */

	checkCrop(d);  // make sure cropping is legal

	/* Write fid file header */
	wdfhCSI(d, FID_FILE);


	/* Allocate memory for a single block header to be reused */
	if ((dbh = malloc( sizeof(d->bh))) == NULL)
		nomem(__FILE__, __FUNCTION__, __LINE__);

	getdbh(d,0); // read the first block header
	copydbh(&d->bh, dbh);

	/* Loop over requested volumes in data file */
	for (d->vol = d->startvol; d->vol < d->endvol; d->vol++) { // somehow I hope this is slices times other arrayed stuff MK

	  if (interupt) return;           /* Interupt/cancel from VnmrJ */

        // restore data dimensions from previous zero filling
		d->np = 2*dim1;
		d->nv = dim2;
	    d->nv2=dim3;

		getblockCSI2D(d, d->vol, NDCC); /* Get block without applying dbh.lvl and dbh.tlt */

//		zeronoise(d); /* Zero any noise measurement */

		// equalizenoise(d, STD); /* Scale for equal noise in all receivers */

		 combine_channels(d); /* optimal combination of receivers for best SNR */

		if (!spar(d, "recon", "off")) {

	     shiftdata2D(d, SPATIAL); /* Shift FID data for 2D dim1*dim2 FT */

 	     phaseramp2DCSI(d, 0, PHASE); /* Phase ramp the data to correct for phase encode offset ppe */

  		 phaseramp2DCSI(d, 0, PHASE2); /* Phase ramp the data to correct for phase encode offset ppe 2*/

    	 weightdata2D(d, SPATIAL); /* Weight data in dim2*dim3 using standard VnmrJ parameters */

		 zerofill2DCSI(d, STD); /* Zero fill data in dim1*dim2 using standard VnmrJ parameters */

	     fft2D(d, SPATIAL); /* 2D FT */

		 shiftdata2D(d, SPATIAL); /* Shift data in dim1*dim2 to get 2D images */

		}


// by now multi-channel combined
		//  update spatial dimensions which may be zero filled
		ndim2 = d->nv;
		ndim3 = d->nv2;
		nr = d->nr;

	   startd1=d->startd1;
	   startd2=d->startd1;
	   cropd1=d->cropd1;
	   cropd2=d->cropd2;

	   s1=startd1; e1=startd1+cropd1;
	   s2=startd2; e2=startd2+cropd2;

	   if(d->d1rev)
		   { e1=startd1; s1=startd1+cropd1 -1;}
	   if(d->d2rev)
	  		   { e2=startd2; s2=startd2+cropd2-1;}

		copydbh(dbh, &d->bh);

		if (d->d2rev) {
			if(d->d1rev){
				for (i = s2; i >= e2; i--) {
					for (j = s1; j >= e1; j--) {// these are voxels
						wdbh(d, FID_FILE); /* Write block header */
						w2DCSI(d, 0, 0, i, j, FID_FILE); /* Write block which is a single fid */
					}
				}
			}
			else {
				for (i = s2; i >= e2; i--) {
					for (j = s1; j < e1; j++) {// these are voxels
						wdbh(d, FID_FILE); /* Write block header */
						w2DCSI(d, 0, 0, i, j, FID_FILE); /* Write block which is a single fid */
					}
				}
			}
		}
		else {
			if(d->d1rev){
				for (i = s2; i < e2; i++) {
					for (j = s1; j >= e1; j--) {// these are voxels
						wdbh(d, FID_FILE); /* Write block header */
						w2DCSI(d, 0, 0, i, j, FID_FILE); /* Write block which is a single fid */
					}
				}
			}
			else {
				for (i = s2; i < e2; i++) {
					for (j = s1; j < e1; j++) {// these are voxels
						wdbh(d, FID_FILE); /* Write block header */
						w2DCSI(d, 0, 0, i, j, FID_FILE); /* Write block which is a single fid */
					}
				}
			}
		}

		clearCSIdata(d); /* Clear data volume from memory */
	} /* end of volume loop */

	clear2Dall(d); /* Clear everything from memory */
	closefp(d, FID_FILE);
}
