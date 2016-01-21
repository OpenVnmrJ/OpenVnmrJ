/* write1D.c */
/*---------------------------------------------------------------------------*/
/*                                                                           */
/* write1D.c: 1D data writing functions                                      */
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

/* Magnitude, Real & Imaginary image data are scaled for compatibility with VnmrJ */


void w1Dtrace(struct data *d,int receiver,int trace,int fileid)
{
  FILE *fp=0;
  float *floatdata;
  int dim1,dim2,dim3,nr;
  double re,im,M;
  int k;

  /* Data dimensions */
  dim1=d->np/2; dim2=1; dim3=d->fh.ntraces; nr=d->nr;

  switch(fileid) {
  case FID_FILE:
    fp=d->fidfp;
    if ((floatdata = (float *)malloc(d->np*sizeof(float))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
    for (k=0;k<dim1;k++) {
      floatdata[2*k] = (float)d->data[receiver][trace][k][0]*FFT_SCALE;
      floatdata[2*k+1] = (float)d->data[receiver][trace][k][1]*FFT_SCALE;
    }
    if (reverse_byte_order)
      reverse4ByteOrder(d->np,(char *)floatdata);
    fwrite(floatdata,sizeof(float),d->np,fp);
    break;
    case DATA_FILE:
      fp=d->datafp;
      if ((floatdata = (float *)malloc(d->np*sizeof(float))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
      for (k=0;k<dim1;k++) {
        floatdata[2*k] = (float)d->data[receiver][trace][k][0]*FFT_SCALE;
        floatdata[2*k+1] = (float)d->data[receiver][trace][k][1]*FFT_SCALE;
      }
      if (reverse_byte_order) 
        reverse4ByteOrder(d->np,(char *)floatdata);
      fwrite(floatdata,sizeof(float),d->np,fp);
      break;
    case PHAS_FILE:
      fp=d->phasfp;
      if ((floatdata = (float *)malloc(dim1*sizeof(float))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
      for (k=0;k<dim1;k++) {
        re=d->data[receiver][trace][k][0];
        im=d->data[receiver][trace][k][1];
        M=sqrt(re*re+im*im);
        floatdata[k] = (float)M*FFT_SCALE;
      }
      if (reverse_byte_order) 
        reverse4ByteOrder(dim1,(char *)floatdata);
      fwrite(floatdata,sizeof(float),dim1,fp);
      break;
    default:
      break;
  }
}

void w1Dblock(struct data *d,int receiver,int fileid)
{
  FILE *fp=0;
  float *floatdata;
  int dim1,dim2,dim3,nr;
  double re,im,M;
  int j,k;

  /* Data dimensions */
  dim1=d->np/2; dim2=1; dim3=d->fh.ntraces; nr=d->nr;

  switch(fileid) {
  case FID_FILE:
    fp=d->fidfp;
    if ((floatdata = (float *)malloc(d->np*sizeof(float))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
    for (j=0;j<dim3;j++) {
      for (k=0;k<dim1;k++) {
        floatdata[2*k] = (float)d->data[receiver][j][k][0]*FFT_SCALE;
        floatdata[2*k+1] = (float)d->data[receiver][j][k][1]*FFT_SCALE;
      }
      if (reverse_byte_order)
        reverse4ByteOrder(d->np,(char *)floatdata);
      fwrite(floatdata,sizeof(float),d->np,fp);
    }
    break;
    case DATA_FILE:
      fp=d->datafp;
      if ((floatdata = (float *)malloc(d->np*sizeof(float))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
      for (j=0;j<dim3;j++) {
        for (k=0;k<dim1;k++) {
          floatdata[2*k] = (float)d->data[receiver][j][k][0]*FFT_SCALE;
          floatdata[2*k+1] = (float)d->data[receiver][j][k][1]*FFT_SCALE;
        }
        if (reverse_byte_order) 
          reverse4ByteOrder(d->np,(char *)floatdata);
        fwrite(floatdata,sizeof(float),d->np,fp);
      }
      break;
    case PHAS_FILE:
      fp=d->phasfp;
      if ((floatdata = (float *)malloc(dim1*sizeof(float))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
      for (j=0;j<dim3;j++) {
        for (k=0;k<dim1;k++) {
          re=d->data[receiver][j][k][0];
          im=d->data[receiver][j][k][1];
          M=sqrt(re*re+im*im);
          floatdata[k] = (float)M*FFT_SCALE;
        }
        if (reverse_byte_order) 
          reverse4ByteOrder(dim1,(char *)floatdata);
        fwrite(floatdata,sizeof(float),dim1,fp);
      }
      break;
    default:
      break;
  }
}

void wdfh(struct data *d,int fileid)
{
  FILE *fp=0;
  struct datafilehead fh;
  int vers_id;

/* d->fh.vers_id seems to be zero for fids ??
  vers_id=d->fh.vers_id-FID_FILE; */
  vers_id=1;

  switch(fileid) {
    case FID_FILE:
      fh.nblocks   = d->fh.nblocks;    /* number of blocks in file */
      fh.ntraces   = d->fh.ntraces;    /* number of traces per block */
      fh.np        = d->np;            /* number of elements per trace */
      fh.ebytes    = 4;                /* number of bytes per element */
      fh.tbytes    = fh.np*fh.ebytes;  /* number of bytes per trace */
      fh.bbytes    = fh.ntraces*fh.tbytes+28;  /* number of bytes per block */
      fh.vers_id   = d->fh.vers_id;    /* software version and file_id status bits */
      fh.status    = S_DATA            /* there will be data */
                   + S_FLOAT           /* the data will be 32-bit float */
                   + S_COMPLEX         /* the data will be complex */
                   + (d->fh.status & S_DDR);   /* take the DDR flag from the fid */
      fh.nbheaders = 1;
      fp=d->fidfp;
      break;
    case DATA_FILE:
      fh.nblocks   = d->fh.nblocks*d->fh.ntraces; /* number of blocks in file */
      fh.ntraces   = 1;                /* number of traces per block */
      if (d->fn > 0)
        fh.np      = d->fn;            /* number of elements per trace */
      else
        fh.np      = d->np;            /* number of elements per trace */
      fh.ebytes    = 4;                /* number of bytes per element */
      fh.tbytes    = fh.np*fh.ebytes;  /* number of bytes per trace */
      fh.bbytes    = fh.tbytes+28;     /* number of bytes per block */
      fh.vers_id   = vers_id+DATA_FILE;  /* software version and file_id status bits */
      fh.status    = S_DATA            /* there will be data */
                   + S_SPEC            /* the data will be a spectrum */
                   + S_FLOAT           /* the data will be 32-bit float */
                   + S_COMPLEX         /* the data will be complex */
                   + (d->fh.status & S_DDR)    /* take the DDR flag from the fid */
                   + S_NP;             /* the data will have np dimension active */
      fh.nbheaders = 1;
      fp=d->datafp;
      break;
    case PHAS_FILE:
      fh.nblocks   = d->fh.nblocks*d->fh.ntraces; /* number of blocks in file */
      fh.ntraces   = 1;                /* number of traces per block */
      if (d->fn > 0)
        fh.np      = d->fn/2;          /* number of elements per trace */
      else
        fh.np      = d->np/2;          /* number of elements per trace */
      fh.ebytes    = 4;                /* number of bytes per element */
      fh.tbytes    = fh.np*fh.ebytes;  /* number of bytes per trace */
      fh.bbytes    = fh.tbytes+28;     /* number of bytes per block */
      fh.vers_id   = vers_id+PHAS_FILE;  /* software version and file_id status bits */
      fh.status    = S_DATA            /* there will be data */
                   + S_SPEC            /* the data will be a spectrum */
                   + S_FLOAT           /* the data will be 32-bit float */
/*                   + (d->fh.status & S_DDR)  // take the DDR flag from the fid */
                   + S_NP;             /* the data will have np dimension active */
      fh.nbheaders = 1;
      fp=d->phasfp;
      break;
  }

  if (reverse_byte_order) reversedfh(&fh);
  fwrite(&fh,1,sizeof(d->fh),fp);

}

void wdbh(struct data *d,int fileid)
{
  FILE *fp=0;
  struct datablockhead bh;

  switch(fileid) {
    case FID_FILE:
      bh.scale   = d->bh.scale;   /* scaling factor */
      bh.status  = d->bh.status;  /* status of data in block */
      bh.index   = d->bh.index;   /* block index */
      bh.mode    = d->bh.mode;    /* mode of data in block */
      bh.ctcount = d->bh.ctcount; /* ct value for FID */
      bh.lpval   = d->bh.lpval;   /* F2 left phase in phasefile */
      bh.rpval   = d->bh.rpval;   /* F2 right phase in phasefile */
      bh.lvl     = d->bh.lvl;     /* F2 level drift correction */
      bh.tlt     = d->bh.tlt;     /* F2 tilt drift correction  */
      fp=d->fidfp;
      break;
    case DATA_FILE:
      bh.scale   = d->bh.scale;   /* scaling factor */
      bh.status  = S_DATA         /* there will be data */
                   + S_SPEC       /* the data will be a spectrum */
                   + S_FLOAT      /* the data will be 32-bit float */
                   + S_COMPLEX    /* the data will be complex */
                   + NP_CMPLX;    /* the np dimension is complex */
      bh.index   = d->bh.index;   /* block index */
      bh.mode    = NP_AVMODE;     /* mode of data in block */
      bh.ctcount = d->bh.ctcount; /* ct value for FID */
      bh.lpval   = d->bh.lpval;   /* F2 left phase in phasefile */
      bh.rpval   = d->bh.rpval;   /* F2 right phase in phasefile */
      bh.lvl     = d->bh.lvl;     /* F2 level drift correction */
      bh.tlt     = d->bh.tlt;     /* F2 tilt drift correction  */
      fp=d->datafp;
      break;
    case PHAS_FILE:
      bh.scale   = d->bh.scale;   /* scaling factor */
      bh.status  = S_DATA         /* there will be data */
                   + S_SPEC       /* the data will be a spectrum */
                   + S_FLOAT;     /* the data will be 32-bit float */
      bh.index   = d->bh.index;   /* block index */
      bh.mode    = NP_AVMODE;     /* mode of data in block */
      bh.ctcount = d->bh.ctcount; /* ct value for FID */
      bh.lpval   = d->bh.lpval;   /* F2 left phase in phasefile */
      bh.rpval   = d->bh.rpval;   /* F2 right phase in phasefile */
      bh.lvl     = d->bh.lvl;     /* F2 level drift correction */
      bh.tlt     = d->bh.tlt;     /* F2 tilt drift correction  */
      fp=d->phasfp;
      break;
  }

  if (reverse_byte_order) reversedbh(&bh);
  fwrite(&bh,1,sizeof(d->bh),fp);

}

void openfpw(struct data *d,int fileid)
{
  FILE *fp;
  char filename[MAXPATHLEN];

  strcpy(filename,d->file); /* copy fid file name */

  if (vnmrj_recon) {
    filename[strlen(filename)-10]=0; /* remove "acqfil/fid" from string */
    strcat(filename,"datdir/");      /* add "datdir/" */
  } else {
    filename[strlen(filename)-3]=0; /* remove "fid" from string */
  }

  switch(fileid) {
	case FID_FILE:
	  strcat(filename,"newfid"); // not to be confused with the old fid
	  break;
    case DATA_FILE:
      strcat(filename,"data");
      break;
    case PHAS_FILE:
      strcat(filename,"phasefile");
      break;
    default:
      break;
  }

  /* Open the file for writing */
  if ((fp=fopen(filename,"w")) == NULL) {
    fprintf(stderr,"\n%s: %s()\n",__FILE__,__FUNCTION__);
    fprintf(stderr,"  Unable to open file %s\n",filename);
    fprintf(stderr,"  Aborting ...\n\n");
    exit(1);
  }

  switch(fileid) {
  case FID_FILE:
    d->fidfp=fp;
    break;
    case DATA_FILE:
      d->datafp=fp;
      break;
    case PHAS_FILE:
      d->phasfp=fp;
      break;
    default:
      break;
  }

}

void closefp(struct data *d,int fileid)
{
  switch(fileid) {
    case FID_FILE:
     fclose(d->fidfp);
      break;
    case DATA_FILE:
      fclose(d->datafp);
      break;
    case PHAS_FILE:
      fclose(d->phasfp);
      break;
  }
}
