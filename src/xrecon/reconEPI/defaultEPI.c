/* defaultEPI.c */
/*---------------------------------------------------------------------------*/
/*                                                                           */
/* defaultEPI.c: default EPI recon                                           */
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

void defaultEPI(struct data *d)
{
  int DISCARD=-1;
  int refEPI=FALSE,refSGE=FALSE;
  int getscaleref=FALSE;
  double oversample;
  struct data ref1,ref2,ref3,ref4,ref5,ref6;
  struct segscale scale1,scale2;

  enum {
    OFF = 0,
    POINTWISE = 1,
    TRIPLE = 2,
    SCALED_TRIPLE = 3
  } epi_pc;

  setnvolsEPI(d);                     /* Set the number of data volumes */

  d->nv=(int)*val("nphase",&d->p);    /* Set d->nv for dimorder2D */
  d->pssorder=sliceorder(d,d->ns,"pss"); /* Fill pssorder with the slice order */
  d->dim2order=phaseorder(d,d->nv,d->nv,"par_does_not_exist"); /* Set dim2order=-1 for sequential phase encode order */
  d->dim3order=phaseorder(d,d->nv,d->nv,"sgepelist"); /* Fill dim3order with the standard gradient echo phase encode order */
  d->nv2=d->nv;                       /* Set d->nv2 for dim3order */
  d->nv=(int)*val("nseg",&d->p);      /* Use d->nv for the number of shots */

  /* Set EPI correction scheme */
  if (spar(d,"epi_pc","POINTWISE")) epi_pc=POINTWISE;
  else if (spar(d,"epi_pc","TRIPLE")) epi_pc=TRIPLE;
  else if (spar(d,"epi_pc","SCALED_TRIPLE")) epi_pc=SCALED_TRIPLE;
  else epi_pc=OFF;

  /* Check whether to output or discard reference scans */
  if (spar(d,"imRF","y")) refEPI=TRUE;
  if (spar(d,"imSGE","y")) refSGE=TRUE;

  /* Set reference data */
  if (epi_pc > OFF) {        /* Pointwise or triple reference phase correction */
    initdatafrom(d,&ref1);
    initdatafrom(d,&ref3);   /* ref3 used in pointwise phase correction of inverted
                                reference scans if reference output is selected */
  }
  if (epi_pc > POINTWISE) {  /* Triple reference phase corrections */
    initdatafrom(d,&ref2);
    initdatafrom(d,&ref4);
  }
  if (epi_pc > TRIPLE) { /* Scaled triple reference phase correction */
    initdatafrom(d,&ref5);
    initdatafrom(d,&ref6);
  }

  /* Set default of no compressed segment scaling just in case it's never set */
  scale1.data=FALSE;
  scale2.data=FALSE;

  /* Loop over data blocks */
  for (d->block=0;d->block<d->nblocks;d->block++) {

    if (interupt) return;             /* Interupt/cancel from VnmrJ */

    d->outvol=0; /* Initialise output data volume (that will not include reference scans) */

    for (d->vol=0;d->vol<d->nvols;d->vol++) { /* Loop over "volumes" */

      setoutvolEPI(d);                /* Set output data volume */

      if (d->outvol>d->endvol) break; /* Break if last requested volume has been processed */

      getblockEPI(d,d->vol,NDCC);     /* Get block without applying dbh.lvl and dbh.tlt */
      zeromax(d);                     /* Zero max structure & coordinates of maximum */
      zeronoise(d);                   /* Zero values in noise structure */

#ifdef DEBUG
  fprintf(stdout,"\n%s: %s()\n",__FILE__,__FUNCTION__);
  switch (epi_pc) {
    case OFF: fprintf(stdout,"  Correction: OFF \n"); break;
    case POINTWISE: fprintf(stdout,"  Correction: POINTWISE \n"); break;
    case TRIPLE: fprintf(stdout,"  Correction: TRIPLE \n"); break;
    case SCALED_TRIPLE: fprintf(stdout,"  Correction: SCALED_TRIPLE \n"); break;
  }
  fflush(stdout);
#endif

      /* Process data as directed by image parameter */
      switch ((int)getelem(d,"image",d->vol)) {

        case 0:   /* Reference, no phase-encode */
          setblockEPI(d);                     /* Set block for 2D data (d->nv > 1) and navigators */
          if (refEPI) w2Dfdfs(d,VJ,FLT32,d->vol); /* Output raw data for the volume, if requested */
          if (epi_pc > OFF) {                 /* If there is phase correction */
            ftnpEPI(d);                       /* FT along readout dimension */
            clear2Ddata(&ref1);               /* Clear ref1 */
            copy2Ddata(d,&ref1);              /* Copy to ref1 */
            ref1.datamode=EPIREF;             /* Flag as EPIREF data */
          } else {                            /* else there is no phase correction */
            ref1.datamode=NONE;               /* Flag as no data */
            if (refEPI) ftnpEPI(d);           /* FT along readout dimension */
          }
          setsegscale(d,&scale1);             /* Set scaling for compressed segments */
          segscale(d,&scale1);                /* Scale compressed segments */
          if (refEPI)                         /* If reference output is requested */
            w2Dfdfs(d,VJ,FLT32,d->vol);       /* Output data for the volume */
          else w2Dfdfs(d,VJ,FLT32,DISCARD);   /* Else use DISCARD to flag skip the volume */
          wnifti(d,VJ,FLT32,DISCARD);
          break;
        case -1:  /* Inverted Readout Reference, with phase-encode */
#ifdef DEBUG
  fprintf(stdout,"  Processing reference -1 data ...\n");
  fflush(stdout);
#endif
          setblockEPI(d);                     /* Set block for 2D data (d->nv > 1) and navigators */
          if (refEPI) w2Dfdfs(d,VJ,FLT32,d->vol); /* Output raw data for the volume, if requested */
          ftnpEPI(d);                         /* FT along readout dimension */
          segscale(d,&scale2);                /* Scale compressed segments */
          if (epi_pc > POINTWISE) {           /* if triple or scaled triple reference phase correction */
            clear2Ddata(&ref2);               /* Clear ref2 */
            copy2Ddata(d,&ref2);              /* Copy to ref2 */
            ref2.datamode=EPIREF;             /* Flag ref2 as EPIREF data */
            if (ref3.datamode == EPIREF) {    /* if there is ref3 reference data */
              phaseEPIref(&ref2,&ref3,&ref4); /* Phase correct ref2 data using ref3 and put result in ref4 */
/*              analyseEPInav(&ref4);           // Analyse the navigators */
              stripEPInav(&ref4);             /* Strip the navigator scans */
              ftnvEPI(&ref4);                 /* FT along phase encode dimension */
              revreadEPI(&ref4);              /* Reverse the data in readout dimension */
              getscaleref=TRUE;               /* Flag to store the next regular image for scaling in SCALED_TRIPLE */
            }
          }
          if (refEPI) {                       /* if reference output is requested */
            if (ref3.datamode == EPIREF) {    /* if there is ref3 reference data */
              phaseEPI(d,&ref3);              /* Phase correct with the reference */
            }
            navcorrEPI(d);                    /* Phase correct with the navigator */
            stripEPInav(d);                   /* Strip the navigator scans */
            ftnvEPI(d);                       /* FT along phase encode dimension */
            revreadEPI(d);                    /* Reverse the data in readout dimension */
            w2Dfdfs(d,VJ,FLT32,d->vol);       /* Output data for the volume */
          }
          else w2Dfdfs(d,VJ,FLT32,DISCARD);   /* Use DISCARD to flag skip the volume */
          wnifti(d,VJ,FLT32,DISCARD);
          break;
        case -2:  /* Inverted Readout Reference, no phase-encode */
#ifdef DEBUG
  fprintf(stdout,"  Processing reference -2 data ...\n");
  fflush(stdout);
#endif
          setblockEPI(d);                     /* Set block for 2D data (d->nv > 1) and navigators */
          if (refEPI) w2Dfdfs(d,VJ,FLT32,d->vol);
          ftnpEPI(d);                         /* FT along readout dimension */
          setsegscale(d,&scale2);             /* Set scaling for compressed segments */
          segscale(d,&scale2);                /* Scale compressed segments */
          if (epi_pc > POINTWISE) {           /* if old triple or triple reference phase correction */
            clear2Ddata(&ref3);               /* Clear ref3 */
            copy2Ddata(d,&ref3);              /* Copy to ref3 */
            ref3.datamode=EPIREF;             /* Flag ref3 as EPIREF data */
            if (ref2.datamode == EPIREF) {    /* if there is ref2 reference data */
              phaseEPIref(&ref2,&ref3,&ref4); /* Phase correct ref2 data using ref3 and put result in ref4 */
/*              analyseEPInav(&ref4);           // Analyse the navigators */
              stripEPInav(&ref4);             /* Strip the navigator scans */
              ftnvEPI(&ref4);                 /* FT along phase encode dimension */
              revreadEPI(&ref4);              /* Reverse the data in readout dimension */
              getscaleref=TRUE;               /* Flag to store the next regular image for scaling in SCALED_TRIPLE */
            }
          }
          if (refEPI) {                       /* if reference output is requested */
            if (epi_pc == POINTWISE) {        /* if pointwise reference phase correction */
              clear2Ddata(&ref3);             /* Clear ref3 */
              copy2Ddata(d,&ref3);            /* Copy to ref3 */
              ref3.datamode=EPIREF;           /* Flag ref3 as EPIREF data */
            }
            revreadEPI(d);                    /* Reverse the data in readout dimension */
            w2Dfdfs(d,VJ,FLT32,d->vol);       /* Output data for the volume */
          }
          else w2Dfdfs(d,VJ,FLT32,DISCARD);   /* Use DISCARD to flag skip the volume */
          wnifti(d,VJ,FLT32,DISCARD);
          break;
        case 1:   /* Regular image */
#ifdef DEBUG
  fprintf(stdout,"  Processing image 1 data ...\n");
  fflush(stdout);
#endif
          setblockEPI(d);                     /* Set block for 2D data (d->nv > 1) and navigators */
          if (d->outvol>=d->startvol)
            w2Dfdfs(d,VJ,FLT32,d->vol);       /* Output raw data for the volume, if requested */
          switch (epi_pc) {
            case SCALED_TRIPLE:               /* Scaled triple reference phase correction */
              if (getscaleref) {              /* If the scale reference has just been acquired */
                clear2Ddata(&ref5);           /* Clear ref5 */
                copy2Ddata(d,&ref5);          /* Copy to ref5 */
                ref5.datamode=EPIREF;         /* Flag ref5 as EPIREF data */
                prepEPIref(&ref5,&ref1);      /* Prepare ref5 data so it can be used to scale ref4 data */
              }
              break;
            default:
              break;
          }
          ftnpEPI(d);                         /* FT along readout dimension */
          segscale(d,&scale1);                /* Scale compressed segments */
          phaseEPI(d,&ref1);                  /* Phase correct with the reference */
          navcorrEPI(d);                      /* Phase correct with the navigator */
/*          analyseEPInav(d);                   // Analyse the navigators */
          stripEPInav(d);                     /* Strip the navigator scans */
          ftnvEPI(d);                         /* FT along phase encode dimension */
          switch (epi_pc) {
            case TRIPLE:                      /* Triple reference phase correction */
              addEPIref(d,&ref4);             /* Add ref4 data to cancel N/2 ghost */
              break;
            case SCALED_TRIPLE:               /* Scaled triple reference phase correction */
              if (getscaleref) {              /* If the scale reference has just been acquired */
                addEPIref(d,&ref4);           /* Add ref4 data to cancel N/2 ghost */
                getscaleref=FALSE;            /* Flag that scale reference has been acquired */
              } else {
                addscaledEPIref(d,&ref4,&ref5); /* Scale ref4 data according to d/ref5, then add to d */
              }
              break;
            default:
              break;
          }
          if (d->outvol>=d->startvol) {
            phasedata2D(d,VJ);                /* Phase data if required */
            w2Dfdfs(d,VJ,FLT32,d->vol);       /* Write 2D fdf data from volume */
            wnifti(d,VJ,FLT32,d->vol);
          }
          break;
        default: /* Reference Standard Gradient Echo */
          setblockSGE(d);
          if (refSGE) w2Dfdfs(d,VJ,FLT32,d->vol); /* Output raw data for the volume, if requested */
          shiftdata2D(d,STD);                 /* Shift FID data for fft */
          zeronoise(d);                       /* Zero any noise measurement */
          equalizenoise(d,STD);               /* Scale for equal noise in all receivers */
          phaseramp2D(d,READ);                /* Phase ramp the data to correct for readout offset pro */
          phaseramp2D(d,PHASE);               /* Phase ramp the data to correct for phase encode offset ppe */
          weightdata2D(d,STD);                /* Weight data using standard VnmrJ parameters */
          zerofill2D(d,STD);                  /* Zero fill data using standard VnmrJ parameters */
          fft2D(d,STD);                       /* 2D fft */
          phasedata2D(d,VJ);                  /* Phase data if required */
          shiftdata2D(d,STD);                 /* Shift data to get images */
          oversample=*val("oversample",&d->p); /* Check to see if there is oversampling */
          if (oversample>1) zoomEPI(d);       /* If oversampled, zoom to get the requested FOV */
          if (refSGE)                         /* If standard gradient echo reference output is requested */
            w2Dfdfs(d,VJ,FLT32,d->vol);       /* Output data for the volume */
          else w2Dfdfs(d,VJ,FLT32,DISCARD);   /* Else use DISCARD to flag skip the volume */
          wnifti(d,VJ,FLT32,DISCARD);
          break;
      } /* end image parameter switch */

      clear2Ddata(d);               /* Clear data volume from memory */
      setdim(d);                    /* Reset data dimensions in case data has been zerofilled (sets d->nv=1) */
      d->nv=*val("nseg",&d->p);     /* Use d->nv for the number of shots */
      d->nv2=(int)*val("nphase",&d->p); /* Use d->nv2 for number of standard gradient echo phase encodes */
      d->dimstatus[0] = NONE;       /* Make sure ZEROFILL status is not set, otherwise setdim will be called in getblock() */
      d->dimstatus[1] = NONE;       /* Make sure ZEROFILL status is not set, otherwise setdim will be called in getblock() */

    } /* end volume loop */

  } /* end data block loop */

  /* Clear all reference data */
  if (epi_pc > OFF) {        /* Pointwise or triple reference phase correction */
    clear2Dall(&ref1);
    clear2Dall(&ref3);
  }
  if (epi_pc > POINTWISE) {  /* Triple and scaled triple reference phase correction */
    clear2Dall(&ref2);
    clear2Dall(&ref4);
  }
  if (epi_pc > TRIPLE) {     /* Scaled triple reference phase correction */
    clear2Dall(&ref5);
    clear2Dall(&ref6);
  }

  clear2Dall(d);             /* Clear everything from memory */

}
