/* mask2D.c */
/*---------------------------------------------------------------------------*/
/*                                                                           */
/* mask2D.c: Mask 2D data                                                    */
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

void mask2D(struct data *d)
{
  struct file fref;
  struct data ref,d1;
  int process;

  /* Check if a suitable reference file is defined */
  /* If it is then we will use the appropriate reference data */
  if ((spar(d,"smapref","vcoil"))   /* Volume coil reference */
    && (strlen(*sval("vcoilref",&d->p)) > 0)) {
    setreffile(&fref,d,"vcoilref"); /* Set reference file */
    getpars(fref.procpar[0],&ref);  /* Get pars from reference procpar */
    opendata(fref.fid[0],&ref);     /* Open reference data file fid */
    setdatapars(&ref);              /* Set data structure parameters */
    copypar("file",&d->p,&ref.p);   /* Copy file parameter to reference data */
    copymaskpars(d,&ref);           /* Copy masking parameters to reference data */
    copynblocks(d,&ref);            /* Copy nblocks parameter to reference data */
    setref2Dmatrix(&ref,d);         /* Set reference fn and fn1 */
    d=&ref;
  } else if (strlen(*sval("senseref",&d->p)) > 0) { /* Sense reference defined */
    setreffile(&fref,d,"senseref"); /* Set reference file */
    getpars(fref.procpar[0],&ref);  /* Get pars from reference procpar */
    opendata(fref.fid[0],&ref);     /* Open reference data file fid */
    setdatapars(&ref);              /* Set data structure parameters */
    copymaskpars(d,&ref);           /* Copy masking parameters to reference data */
    copynblocks(d,&ref);            /* Copy nblocks parameter to reference data */
    setref2Dmatrix(&ref,d);         /* Set reference fn and fn1 */
    d=&ref;
  }

  setnvols(d);               /* Set the number of data volumes */
  dimorder2D(d);             /* Sort ascending slice and phase order */

  /* Loop over data blocks */
  for (d->block=0;d->block<d->nblocks;d->block++) {

    if (interupt) return;        /* Interupt/cancel from VnmrJ */

    /* Figure to process or not to process */
    process=process2Dblock(d,"maskstartslice","maskendslice");

    if (process) {
      getblock2D(d,d->vol,NDCC); /* Get block without applying dbh.lvl and dbh.tlt */
      shiftdata2D(d,STD);        /* Shift FID data for fft */
      equalizenoise(d,MK);       /* Scale for equal noise in all receivers (mask pars) */
      phaseramp2D(d,PHASE);      /* Phase ramp the data to correct for phase encode offset ppe */
      initdata(&d1);             /* Initialise 2D data structure */
      copy2Ddata(d,&d1);         /* Take a copy of the data */
      weightdata2D(d,MK);        /* Apply mask weighting to data */
      zerofill2D(d,STD);         /* Zero fill data according to fn, fn1 */
      fft2D(d,STD);              /* 2D fft */
      shiftdata2D(d,STD);        /* Shift data to get images */
      get2Dmask(d,MK);           /* Get mask for specified slices of image data */
      fill2Dmask(d,MK);          /* Density filter the mask for specified slices */
      copy2Ddata(&d1,d);         /* Restore the copied data */
      clear2Ddata(&d1);          /* Clear the copied data */
      weightdata2D(d,STD);       /* Apply standard weighting to data */
      zerofill2D(d,STD);         /* Zero fill data according to fn, fn1 */
      fft2D(d,STD);              /* 2D fft */
      shiftdata2D(d,STD);        /* Shift data to get images */
      w2Dfdfs(d,VJ,FLT32,0);     /* Write 2D fdf image data from volume j */
      w2Dfdfs(d,MK,FLT32,0);     /* Write 2D fdf image data from volume j */
      w2Dfdfs(d,RMK,FLT32,0);    /* Write 2D fdf image data from volume j */
      clear2Ddata(d);            /* Clear data volume from memory */
    }

  }

  clear2Dall(d);             /* Clear everything from memory */

}

void add2mask2D(struct data *d)
{
  struct file fref;
  struct data ref;

  /* Check if a suitable reference file is defined */
  /* If it is then we will use the appropriate reference data */
  if ((spar(d,"smapref","vcoil"))   /* Volume coil reference */
    && (strlen(*sval("vcoilref",&d->p)) > 0)) {
    setreffile(&fref,d,"vcoilref");   /* Set reference file */
    getpars(fref.procpar[0],&ref);  /* Get pars from reference procpar */
    opendata(fref.fid[0],&ref);     /* Open reference data file fid */
    setdatapars(&ref);              /* Set data structure parameters */
    copypar("file",&d->p,&ref.p);   /* Copy file parameter to reference data */
    copymaskpars(d,&ref);           /* Copy masking parameters to reference data */
    setref2Dmatrix(&ref,d);         /* Set reference fn and fn1 */
    d=&ref;
  } else if (strlen(*sval("senseref",&d->p)) > 0) { /* Sense reference defined */
    setreffile(&fref,d,"senseref"); /* Set reference file */
    getpars(fref.procpar[0],&ref);  /* Get pars from reference procpar */
    opendata(fref.fid[0],&ref);     /* Open reference data file fid */
    setdatapars(&ref);              /* Set data structure parameters */
    copymaskpars(d,&ref);           /* Copy masking parameters to reference data */
    setref2Dmatrix(&ref,d);         /* Set reference fn and fn1 */
    d=&ref;
  }

  setnvols(d);               /* Set the number of data volumes */
  dimorder2D(d);             /* Sort ascending slice and phase order */

  /* Loop over data blocks */
  for (d->block=0;d->block<d->nblocks;d->block++) {

    getblock2D(d,d->vol,NDCC); /* Get block without applying dbh.lvl and dbh.tlt */
    shiftdata2D(d,STD);      /* Shift FID data for fft */
    equalizenoise(d,MK);     /* Scale for equal noise in all receivers (mask pars) */
    phaseramp2D(d,PHASE);    /* Phase ramp the data to correct for phase encode offset ppe */
    weightdata2D(d,STD);     /* Apply mask weighting to data */
    zerofill2D(d,STD);       /* Zero fill data according to fn, fn1 */
    fft2D(d,STD);            /* 2D fft */
    shiftdata2D(d,STD);      /* Shift data to get images */

    /* Read the mask */
    if (!read2Dmask(d,MK)) {
      fprintf(stderr,"\n%s: %s()\n",__FILE__,__FUNCTION__);
      fprintf(stderr,"  Problem reading mask\n");
      fflush(stderr);
      return;
    }

    /* Read the mask ROIs and add to mask */
    if (!read2Dmask(d,MKROI)) {
      fprintf(stderr,"\n%s: %s()\n",__FILE__,__FUNCTION__);
      fprintf(stderr,"  Problem reading maskroi\n");
      fflush(stderr);
      return;
    }

    w2Dfdfs(d,VJ,FLT32,0);   /* Write 2D fdf image data from volume */
    w2Dfdfs(d,MK,FLT32,0);   /* Write 2D fdf image data from volume */
    w2Dfdfs(d,RMK,FLT32,0);  /* Write 2D fdf image data from volume */

  }

  clear2Dall(d);             /* Clear everything from memory */

}

int read2Dmask(struct data *d,int mode)
{
  FILE *f_in;
  float *floatdata;
  int dim1,dim2,dim3,nr;
  int startpos,slice;
  char dirname[MAXPATHLEN],filename[MAXPATHLEN];
  int i,j,k,l;
  int ix1,ix2;
  int offline=FALSE;
  char tmp_str[2],tmp_char;
  char header[MAXPATHLEN],varname[MAXPATHLEN];
  char *header_ptr;
  char storage[MAXPATHLEN];
  int rank,bits,fdfdim1,fdfdim2;

#ifdef DEBUG
  struct timeval tp;
  double t1,t2;
  int rtn;
#endif

  /* Check for VnmrJ recon to figure if we are offline */
  if (vnmrj_recon) strcpy(dirname,vnmrj_path);
  else {
    for (i=0;i<=strlen(d->file)-9;i++)
      dirname[i]=d->file[i];
    dirname[i]=0;
    offline=TRUE;
  }
  switch(mode) {
    case MK:
      strcat(dirname,"mask");
      break;
    case MKROI:
      strcat(dirname,"maskroi");
      break;
    default:
      return(0);
  } /* end mode switch */
  if (offline) strcat(dirname,".img");

  /* Return if mask directory does not exist */
  if (checkdir(dirname)) return(0);

#ifdef DEBUG
  fprintf(stdout,"\n%s: %s()\n",__FILE__,__FUNCTION__);
  fprintf(stdout,"  Reading mask data %s: ",dirname);
  rtn=gettimeofday(&tp, NULL);
  t1=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
  fflush(stdout);
#endif

  /* Data dimensions */
  if (d->fn<1) dim1=d->np/2; else dim1=d->fn/2;
  if (d->fn1<1) dim2=d->nv; else dim2=d->fn1/2;
  dim3=d->endpos-d->startpos; nr=d->nr;
  startpos=d->startpos;

  /* Allocate memory for the mask */
  if (!d->mask) {
    if ((d->mask = (int **)malloc(dim3*sizeof(int *))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
    for (j=0;j<dim3;j++)
      if ((d->mask[j] = (int *)malloc(dim2*dim1*sizeof(int))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
  }

  /* Read data */
  ix1=0;
  for (j=0;j<dim3;j++) {
    slice=startpos+j;
    /* Return if mask data does not exist */
    switch(mode) {
      case MK:
        sprintf(filename,"%s/slice%.3dimage001echo001.fdf",dirname,slice+1);
        if (checkfile(filename)) {
          clear2Dmask(d);
          return(0);
        }
        break;
      case MKROI:
        sprintf(filename,"%s/roi.%.4d.fdf",dirname,slice+1);
        if (checkfile(filename)) {
          clear2Dmask(d);
          return(0);
        }
        break;
      default:
        clear2Dmask(d);
        return(0);
    } /* end mode switch */
    /* Open FDF file and read data */
    if ((f_in = fopen(filename,"r")) == NULL) {
      fprintf(stderr,"\n%s: %s()\n",__FILE__,__FUNCTION__);
      fprintf(stderr,"  Unable to read %s\n",filename);
      fflush(stderr);
      clear2Dmask(d);
      return(0);
    }
    header_ptr = header;
    /* Get rank, storage, bits and matrix from fdf header */
    while((fscanf(f_in,"%c",&tmp_char)) != EOF) {
      sprintf(tmp_str,"%c",tmp_char);
      /* If char is "," ";" or "}" insert a space before */
      if (((int)tmp_char == 44) || ((int)tmp_char == 59)
        || ((int)tmp_char == 125))
        strcat(header," ");
      strcat(header,tmp_str);
      /* If char is "," or "{" insert a space after */
      if (((int)tmp_char == 44) || ((int)tmp_char == 123))
        strcat(header," ");
      if ((int)tmp_char == 10) { /*  Carriage return */
        sscanf(header_ptr,"%*s %s",varname);
        if (!strcmp(varname,"rank"))
          sscanf(header_ptr,"%*s %*s %*s %d",&rank);
        if (!strcmp(varname,"*storage"))
          sscanf(header_ptr,"%*s %*s %*s %s",storage);
        if (!strcmp(varname,"bits"))
          sscanf(header_ptr,"%*s %*s %*s %d",&bits);
        if (!strcmp(varname,"matrix[]") && (rank == 2))
          sscanf(header_ptr,"%*s %*s %*s %*s %d %*s %d",&fdfdim2,&fdfdim1);
        strcpy(header,"");
      }
      if ((int)tmp_char == 0) /* NULL character */
        /* we must now be at start of data */
        break;
    }

    if ((rank !=2) || (fdfdim1 != dim1) || (fdfdim2 != dim2)) {
      fprintf(stderr,"\n%s: %s()\n",__FILE__,__FUNCTION__);
      fprintf(stderr,"  Mask %s has incompatible dimensions\n",filename);
      fflush(stderr);
      clear2Dmask(d);
      return(0);
    }

    /* Allocate memory and read the data */
    if (!strcmp(storage,"\"float\"")) {
      if ((floatdata = (float *)malloc(dim2*dim1*sizeof(float))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
      fread(floatdata,sizeof(float),dim2*dim1,f_in);

      switch(mode) {
        case MK:
          for(k=0;k<dim2;k++) {
            for (l=0;l<dim1;l++) {
              ix1=k*dim1+l;
              ix2=(dim1-l-1)*dim2+dim2-k-1;
              d->mask[j][ix1] = (int)floatdata[ix2];
            }
          }
          break;
        case MKROI:
          for(k=0;k<dim2;k++) {
            for (l=0;l<dim1;l++) {
              ix1=k*dim1+l;
              ix2=(dim1-l-1)*dim2+dim2-k-1;
              if (floatdata[ix2] > 0.0) d->mask[j][ix1] = 1;
            }
          }
          /* Zero the point that was used for masking ROIs in VnmrJ */
          d->mask[j][ix1] = 0;
          break;
      } /* end mode switch */
      free(floatdata);
    }
    else {
      clear2Dmask(d);
      return(0);
    }
  }

#ifdef DEBUG
  rtn=gettimeofday(&tp, NULL);
  t2=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
  fprintf(stdout,"took %f secs\n",t2-t1);
  fflush(stdout);
#endif

  return(1);
}

void mask2Ddata(struct data *d1,struct data *d2)
{
  int dim1,dim2,dim3,nr;
  int i,j,k;
  double *dp1;
  int *p2;

#ifdef DEBUG
  struct timeval tp;
  double t1,t2;
  int rtn;
  fprintf(stdout,"\n%s: %s()\n",__FILE__,__FUNCTION__);
  fprintf(stdout,"  Masking data: ");
  rtn=gettimeofday(&tp, NULL);
  t1=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
  fflush(stdout);
#endif

  /* Data dimensions */
  dim1=d1->np/2; dim2=d1->nv; dim3=d1->endpos-d1->startpos; nr=d1->nr;

  /* Now mask data */
  for (i=0;i<nr;i++) {
    for (j=0;j<dim3;j++) {
      dp1 = *d1->data[i][j];
      p2 = d2->mask[j];
      for(k=0;k<dim2*dim1;k++) {
        *dp1++ *= *p2;
        *dp1++ *= *p2++;
      }
    }
  }

#ifdef DEBUG
  rtn=gettimeofday(&tp, NULL);
  t2=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
  fprintf(stdout,"took %f secs\n",t2-t1);
  fflush(stdout);
#endif

}
