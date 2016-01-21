/* default2D.c */
/*---------------------------------------------------------------------------*/
/*                                                                           */
/* default2D.c: default 2D recon                                             */
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

void default2D(struct data *d)
{
  struct data nav;                        /* Define nav to have data structure */

  nulldata(&nav);                         /* Null the navigator data structure */

  setnvols(d);                            /* Set the number of data volumes */

  dimorder2D(d);                          /* Sort ascending slice and phase order */

  if (d->nav) initdatafrom(d,&nav);       /* If there are navigator scans initialize navigator from data */

  /* Loop over requested volumes in data file */
  for (d->vol=d->startvol;d->vol<d->endvol;d->vol++) {

    /* Loop over data blocks */
    for (d->block=0;d->block<d->nblocks;d->block++) {

      if (interupt) return;               /* Interupt/cancel from VnmrJ */

      getblock2D(d,d->vol,NDCC);          /* Get block without applying dbh.lvl and dbh.tlt */

      w2Dfdfs(d,VJ,FLT32,d->vol);         /* Write 2D fdf raw data from volume */

      zeronoise(d);                       /* Zero any noise measurement */

      equalizenoise(d,STD);               /* Scale for equal noise in all receivers (standard pars) */

      if (d->nav) {                       /* If there are navigator scans */

        nav.block = d->block;             /* Set navigator block */

        getnavblock2D(&nav,d->vol,NDCC);  /* get the navigator block */

        shiftdata2D(&nav,READ);           /* Shift navigator data in READ for FT */

        weightdata2D(&nav,REFREAD);       /* Weight navigator data in READ using reference parameters */

        zerofill2D(&nav,READ);            /* Zero fill navigator data in READ using standard parameters */

        fft2D(&nav,READ);                 /* FT navigator data in READ */

        shiftdata2D(d,READ);              /* Shift data in READ for FT */

        weightdata2D(d,READ);             /* Weight data in READ */

        zerofill2D(d,READ);               /* Zero fill data in READ using standard parameters */

        fft2D(d,READ);                    /* FT data in READ */

        navcorr(d,&nav);                  /* Correct data using navigators */

        shiftdata2D(d,READ);              /* Shift data in READ to get profiles */

        clear2Ddata(&nav);                /* Clear navigator data from memory */

        shiftdata2D(d,PHASE);             /* Shift data in PHASE for FT */

        phaseramp2D(d,PHASE);             /* Phase ramp the data to correct for phase encode offset ppe */

        weightdata2D(d,PHASE);            /* Weight data in PHASE */

        zerofill2D(d,PHASE);              /* Zero fill data in PHASE using standard parameters */

        fft2D(d,PHASE);                   /* FT data in PHASE */

        shiftdata2D(d,PHASE);             /* Shift data in PHASE to get images */

      } else {

        shiftdata2D(d,STD);               /* Shift FID data for 2D FT */

        phaseramp2D(d,PHASE);             /* Phase ramp the data to correct for phase encode offset ppe */

        weightdata2D(d,STD);              /* Weight data using standard parameters */

        zerofill2D(d,STD);                /* Zero fill data using standard parameters */

        fft2D(d,STD);                     /* 2D FT */

        shiftdata2D(d,STD);               /* Shift data to get images */

      }

      phasedata2D(d,VJ);                  /* Phase data if required */

      w2Dfdfs(d,VJ,FLT32,d->vol);         /* Write 2D fdf image data from volume */

      wnifti(d,VJ,FLT32,d->vol);          /* Write NIFTI-1/Analyze7.5 data */

      clear2Ddata(d);                     /* Clear data volume from memory */

    }

  }

  if (d->nav) clear2Dall(&nav);           /* If there are navigator scans clear everything from memory */

  clear2Dall(d);                          /* Clear everything from memory */

}
