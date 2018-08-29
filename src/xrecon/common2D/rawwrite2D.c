/* rawwrite2D.c */
/*---------------------------------------------------------------------------*/
/*                                                                           */
/* rawwrite2D.c: 2D raw binary writing routines                              */
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

int wrawbin2D(struct data *d,int type,int precision,int volindex)
{
  char dirname[MAXPATHLEN],filename[MAXPATHLEN];
  int dim3,nr,ne,dirlen,datamode;
  int startpos,endpos;
  int image,slice,echo;
  int i,j;

#ifdef DEBUG
  struct timeval tp;
  double t1,t2;
  int rtn;
#endif

#ifdef DEBUG
  fprintf(stdout,"\n%s: %s()\n",__FILE__,__FUNCTION__);
  fprintf(stdout,"  Writing raw binary data");
  rtn=gettimeofday(&tp, NULL);
  t1=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
  fflush(stdout);
#endif

  /* Check that type is valid */
  if ((type != RE) && (type != IM) && (type != MG) && (type != PH) 
    && (type != MK) && (type != SM)) {
    fprintf(stderr,"\n%s: %s()\n",__FILE__,__FUNCTION__);
    fprintf(stderr,"  Invalid 2nd argument %s(*,'type',*,*)\n",__FUNCTION__);
    fflush(stderr);
    return(1);
  }

  /* Number of slices and receivers */
  dim3=d->endpos-d->startpos; nr=d->nr;
  startpos=d->startpos; endpos=d->endpos;

  datamode=FID;
  /* If FT has been done flag it as IMAGE */
  if ((d->dimstatus[0] & FFT) || (d->dimstatus[1] & FFT)) datamode=IMAGE;

  /* Create appropriate dirname with '.raw' extension */
  switch(datamode) {
    case FID:
      strcpy(filename,"raw");
      break;
    default:
      if ((type == MK) || (type == SM)) strcpy(filename,"");
      else strcpy(filename,"recon");
      break;
  } /* end datamode switch */
  switch(type) {
    case MG: /* Magnitude */
      strcat(filename,"MG.raw");
      break;
    case PH: /* Phase */
      strcat(filename,"PH.raw");
      break;
    case RE: /* Real */
      strcat(filename,"RE.raw");
      break;
    case IM: /* Imaginary */
      strcat(filename,"IM.raw");
      break;
    case MK: /* Mask */
      strcat(filename,"mask.raw");
      break;
    case SM: /* Sensitivity maps */
      strcat(filename,"smap.raw");
      break;
  } /* end type switch */
  strcpy(dirname,d->file);
  dirlen=strlen(dirname);
  if (vnmrj_recon) {
    for (i=0;i<=strlen(filename);i++)
      dirname[dirlen-10+i]=filename[i];
    dirname[dirlen-10+i-1]=0;
  } else {
    for (i=0;i<=strlen(filename);i++)
      dirname[dirlen-8+i]=filename[i];
    dirname[dirlen-8+i-1]=0;
  }

  /* If dirname doesn't exist create it */
  createdir(dirname);

  /* Number of echoes */
  ne=(int)*val("ne",&d->p);
  if (ne < 1) ne=1; /* Set ne to 1 if 'ne' does not exist */

  /* Output raw binary data from slices */
  /* Allow for compressed multi-echo loop */
  image=volindex/ne;
  echo=volindex%ne;
  /* Loop over receivers and slices */
  if ((nr>1) && (type != MK)) {
    for (i=0;i<nr;i++) {
      for (j=0;j<dim3;j++) {
        slice=startpos+j;
        sprintf(filename,"%s/image%.3dslice%.3decho%.3dcoil%.3d.raw",dirname,image+1,slice+1,echo+1,i+1);
        wraw2D(filename,d,i,j,type,precision);
      }
    }
  } else {
    for (j=0;j<dim3;j++) {
      slice=startpos+j;
      sprintf(filename,"%s/image%.3dslice%.3decho%.3d.raw",dirname,image+1,slice+1,echo+1);
      wraw2D(filename,d,0,j,type,precision);
    }
  }

#ifdef DEBUG
  rtn=gettimeofday(&tp, NULL);
  t2=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
  fprintf(stdout," in %s: took %f secs\n",dirname,t2-t1);
  fflush(stdout);
#endif

  return(0);
}

int wraw2D(char *filename,struct data *d,int receiver,int slice,int type,int precision)
{
  FILE *f_out;
  float *floatdata;
  double *doubledata;
  double re,im,M;
  int dim1,dim2;
  int i,j;
  int ix1;

  /* Check that type is valid */
  if ((type != RE) && (type != IM) && (type != MG) && (type != PH) 
    && (type != MK) && (type != SM)) {
    fprintf(stderr,"\n%s: %s()\n",__FILE__,__FUNCTION__);
    fprintf(stderr,"  Invalid 5th argument wraw2D(*,*,*,*,'type',*)\n");
    fflush(stderr);
    return(1);
  }

  /* Data dimensions */
  dim1=d->np/2; dim2=d->nv;

  /* Allocate memory and write raw binary image */
  switch(precision) {
    case FLT32: /* 32 bit float */
      if ((floatdata = (float *)malloc(dim2*dim1*sizeof(float))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
      switch(type) {
        case MG: /* Magnitude */
          for(i=0;i<dim2;i++) {
            for (j=0;j<dim1;j++) {
              ix1=i*dim1+j;
              re=fabs(d->data[receiver][slice][ix1][0]);
              im=fabs(d->data[receiver][slice][ix1][1]);
              M=sqrt(re*re+im*im);
              floatdata[ix1] = (float)M;
            }
          }
          break;
        case RE: /* Real */
          for(i=0;i<dim2;i++) {
            for (j=0;j<dim1;j++) {
              ix1=i*dim1+j;
              floatdata[ix1] = (float)d->data[receiver][slice][ix1][0];
            }
          }
          break;
        case IM: /* Imaginary */
          for(i=0;i<dim2;i++) {
            for (j=0;j<dim1;j++) {
              ix1=i*dim1+j;
              floatdata[ix1] = (float)d->data[receiver][slice][ix1][1];
            }
          }
          break;
        case PH: /* Phase */
          for(i=0;i<dim2;i++) {
            for (j=0;j<dim1;j++) {
              ix1=i*dim1+j;
              re=d->data[receiver][slice][ix1][0];
              im=d->data[receiver][slice][ix1][1];
              /* atan2 returns values (in radians) between +PI and -PI */
              M=atan2(im,re);
              floatdata[ix1] = (float)M;
            }
          }
          break;
        case MK: /* Mask */
          for(i=0;i<dim2;i++) {
            for (j=0;j<dim1;j++) {
              ix1=i*dim1+j;
              floatdata[ix1] = (float)d->mask[slice][ix1];
            }
          }
          break;
        case SM: /* Sensitivity map */
          for(i=0;i<dim2;i++) {
            for (j=0;j<dim1;j++) {
              ix1=i*dim1+j;
              re=fabs(d->data[receiver][slice][ix1][0]);
              im=fabs(d->data[receiver][slice][ix1][1]);
              M=sqrt(re*re+im*im);
              floatdata[ix1] = (float)M;
            }
          }
          break;
        default:
          break;
      } /* end type switch */
      /* Write data */
      if ((f_out = fopen(filename, "w")) == NULL) {
        fprintf(stderr,"\n%s: %s()\n",__FILE__,__FUNCTION__);
        fprintf(stderr,"  Unable to write to %s\n",filename);
        fflush(stderr);
        return(1);
      }
      fwrite(floatdata,sizeof(float),dim1*dim2,f_out);
      fclose(f_out);
      free(floatdata);
      break;
    case DBL64: /* 64 bit double */
      if ((doubledata = (double *)malloc(dim2*dim1*sizeof(double))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
      switch(type) {
        case MG: /* Magnitude */
          for(i=0;i<dim2;i++) {
            for (j=0;j<dim1;j++) {
              ix1=i*dim1+j;
              re=fabs(d->data[receiver][slice][ix1][0]);
              im=fabs(d->data[receiver][slice][ix1][1]);
              M=sqrt(re*re+im*im);
              doubledata[ix1] = M;
            }
          }
          break;
        case RE: /* Real */
          for(i=0;i<dim2;i++) {
            for (j=0;j<dim1;j++) {
              ix1=i*dim1+j;
              doubledata[ix1] = d->data[receiver][slice][ix1][0];
            }
          }
          break;
        case IM: /* Imaginary */
          for(i=0;i<dim2;i++) {
            for (j=0;j<dim1;j++) {
              ix1=i*dim1+j;
              doubledata[ix1] = d->data[receiver][slice][ix1][1];
            }
          }
          break;
        case PH: /* Phase */
          for(i=0;i<dim2;i++) {
            for (j=0;j<dim1;j++) {
              ix1=i*dim1+j;
              re=d->data[receiver][slice][ix1][0];
              im=d->data[receiver][slice][ix1][1];
              /* atan2 returns values (in radians) between +PI and -PI */
              M=atan2(im,re);
              doubledata[ix1] = M;
            }
          }
          break;
        case MK: /* Mask */
          for(i=0;i<dim2;i++) {
            for (j=0;j<dim1;j++) {
              ix1=i*dim1+j;
              doubledata[ix1] = (double)d->mask[slice][ix1];
            }
          }
          break;
        case SM: /* Sensitivity map */
          for(i=0;i<dim2;i++) {
            for (j=0;j<dim1;j++) {
              ix1=i*dim1+j;
              re=fabs(d->data[receiver][slice][ix1][0]);
              im=fabs(d->data[receiver][slice][ix1][1]);
              M=sqrt(re*re+im*im);
              doubledata[ix1] = M;
            }
          }
          break;
        default:
          break;
      } /* end type switch */
      /* Write data */
      if ((f_out = fopen(filename, "w")) == NULL) {
        fprintf(stderr,"\n%s: %s()\n",__FILE__,__FUNCTION__);
        fprintf(stderr,"  Unable to write to %s\n",filename);
        fflush(stderr);
        return(1);
      }
      fwrite(doubledata,sizeof(double),dim1*dim2,f_out);
      fclose(f_out);
      free(doubledata);
      break;
    default:
      fprintf(stderr,"\n%s: %s()\n",__FILE__,__FUNCTION__);
      fprintf(stderr,"  Invalid 6th argument %s(*,*,*,*,*,'precision')\n",__FUNCTION__);
      fflush(stderr);
      return(1);
      break;
  } /* end precision switch */

  return(0);
}

