/* tifwrite2D.c */
/*---------------------------------------------------------------------------*/
/*                                                                           */
/* tifwrite2D.c: TIFF writing routines                                       */
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

#define TIFMAX 255.0 /* 65535.0 if we want image scaled from 0 to 65535 ... */
                     /* but beware, Microsoft Word can't read the files!! */

int wtifs2D(struct data *d,int type,int scale,int volindex)
{
  char dirname[MAXPATHLEN],filename[MAXPATHLEN];
  int dim3,nr,ne,dirlen;
  int startpos,endpos;
  double scalefactor;
  int i,j;
  int image,slice,echo,coil;
int datamode;

#ifdef DEBUG
  struct timeval tp;
  double t1,t2;
  int rtn;
#endif

  /* Check that type is valid */
  if ((type != RE) && (type != IM) && (type != MG) && (type != PH) 
    && (type != MK) && (type != SM)) {
    fprintf(stderr,"\n%s: %s()\n",__FILE__,__FUNCTION__);
    fprintf(stderr,"  Invalid 2nd argument %s(*,'type',*)\n",__FUNCTION__);
    fflush(stderr);
    return(1);
  }

  /* Number of slices and receivers */
  dim3=d->endpos-d->startpos; nr=d->nr;
  startpos=d->startpos; endpos=d->endpos;

  datamode=FID;
  /* If FT has been done flag it as IMAGE */
  if ((d->dimstatus[0] & FFT) || (d->dimstatus[1] & FFT)) datamode=IMAGE;

  /* Create appropriate dirname with '.tif' extension */
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
      strcat(filename,"MG.tif");
      break;
    case PH: /* Phase */
      strcat(filename,"PH.tif");
      break;
    case RE: /* Real */
      strcat(filename,"RE.tif");
      break;
    case IM: /* Imaginary */
      strcat(filename,"IM.tif");
      break;
    case MK: /* Mask */
      strcat(filename,"mask.tif");
      break;
    case SM: /* Sensitivity maps */
      strcat(filename,"smap.tif");
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

  /* Set suitable scaling */
  switch(scale) {

   case NSCALE: /* No scaling */
      switch(type) {
        case MK: /* Mask */
          scalefactor=TIFMAX;
#ifdef DEBUG
  fprintf(stdout,"\n%s: %s()\n",__FILE__,__FUNCTION__);
  fprintf(stdout,"  Scaling mask to %d ...\n",(int)TIFMAX);
  rtn=gettimeofday(&tp, NULL);
  t1=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
#endif
          break;
        default: /* Everything else */
        scalefactor=1.0;
#ifdef DEBUG
  fprintf(stdout,"\n%s: %s()\n",__FILE__,__FUNCTION__);
  fprintf(stdout,"  No scaling of data ...\n");
  rtn=gettimeofday(&tp, NULL);
  t1=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
#endif
        break;
      } /* end type switch */
      break;

    case SCALE: /* Scale to maximum */
      /* Set scalefactor to hold the volume maximum */
      switch(type) {
        case MG: /* Magnitude */
          if (!d->max.data) getmax(d);
#ifdef DEBUG
  fprintf(stdout,"\n%s: %s()\n",__FILE__,__FUNCTION__);
  fprintf(stdout,"  Scaling to maximum Magnitude value of %f ...\n",d->max.Mval);
  rtn=gettimeofday(&tp, NULL);
  t1=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
#endif
          scalefactor=TIFMAX/d->max.Mval;
          break;
        case PH: /* Phase */
          /* atan2 returns values (in radians) between +PI and -PI */
          scalefactor=TIFMAX/M_PI;
#ifdef DEBUG
  fprintf(stdout,"\n%s: %s()\n",__FILE__,__FUNCTION__);
  fprintf(stdout,"  Scaling to range of -PI to +PI ...\n");
  rtn=gettimeofday(&tp, NULL);
  t1=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
#endif
          break;
        case MK: /* Mask */
          scalefactor=TIFMAX;
#ifdef DEBUG
  fprintf(stdout,"\n%s: %s()\n",__FILE__,__FUNCTION__);
  fprintf(stdout,"  Scaling mask to %d ...\n",(int)TIFMAX);
  rtn=gettimeofday(&tp, NULL);
  t1=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
#endif
          break;
        case SM: /* Magnitude */
          if (!d->max.data) getmax(d); 
#ifdef DEBUG
  fprintf(stdout,"\n%s: %s()\n",__FILE__,__FUNCTION__);
  fprintf(stdout,"  Scaling to maximum Magnitude value of %f ...\n",d->max.Mval);
  rtn=gettimeofday(&tp, NULL);
  t1=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
#endif
          scalefactor=TIFMAX/d->max.Mval;
          break;
        default: /* Real or Imaginary */
          if (!d->max.data) getmax(d);
#ifdef DEBUG
  fprintf(stdout,"\n%s: %s()\n",__FILE__,__FUNCTION__);
  fprintf(stdout,"  Scaling to maximum value of %f ...\n",d->max.Rval);
  rtn=gettimeofday(&tp, NULL);
  t1=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
#endif
          scalefactor=TIFMAX/d->max.Rval;
          break;
      } /* end type switch */
      break;

    case NOISE:
      /* Set scalefactor to hold max noise of volume */
      switch(type) {
        case PH: /* Phase */
          /* atan2 returns values (in radians) between +PI and -PI */
          scalefactor=TIFMAX/M_PI;
#ifdef DEBUG
  fprintf(stdout,"\n%s: %s()\n",__FILE__,__FUNCTION__);
  fprintf(stdout,"  Scaling to range of -PI to +PI ...\n");
  rtn=gettimeofday(&tp, NULL);
  t1=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
#endif
          break;
        case MK: /* Mask */
          scalefactor=TIFMAX;
#ifdef DEBUG
  fprintf(stdout,"\n%s: %s()\n",__FILE__,__FUNCTION__);
  fprintf(stdout,"  Scaling mask to %d ...\n",(int)TIFMAX);
  rtn=gettimeofday(&tp, NULL);
  t1=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
#endif
          break;
        default:
          /* If we have not already measured the noise then attempt it now */
          /* NB zero filled data likely to return zero for noise */
          /* If noise data does not exist set it to zero */
          if (!d->noise.data) zeronoise(d);
          /* If noise data is zero sample the noise */
          if (d->noise.zero) getnoise(d,STD);

          /* Scale according to the smallest noise of all the receivers */
          coil=0;
          scalefactor=d->noise.M[0];
          for (i=1;i<nr;i++) {
            if (d->noise.M[i] < scalefactor) {
              scalefactor=d->noise.M[i];
              coil=i;
            }
          }

#ifdef DEBUG
  fprintf(stdout,"\n%s: %s()\n",__FILE__,__FUNCTION__);
  fprintf(stdout,"  Scaling to magnitude of noise in receiver %d (%f) ...\n",coil,scalefactor);
  rtn=gettimeofday(&tp, NULL);
  t1=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
#endif
          scalefactor=TIFMAX/(5.0*scalefactor);
          break;
      } /* end type switch */
      break;

    default:
      switch(type) {
        case MK: /* Mask */
          scalefactor=TIFMAX;
#ifdef DEBUG
  fprintf(stdout,"\n%s: %s()\n",__FILE__,__FUNCTION__);
  fprintf(stdout,"  Scaling mask to %d ...\n",(int)TIFMAX);
  rtn=gettimeofday(&tp, NULL);
  t1=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
#endif
          break;
        default: /* Everything else */
          scalefactor=TIFMAX/scale;
#ifdef DEBUG
  fprintf(stdout,"\n%s: %s()\n",__FILE__,__FUNCTION__);
  fprintf(stdout,"  Scaling so that %d is the maximum value ...\n",scale);
  rtn=gettimeofday(&tp, NULL);
  t1=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
#endif
        break;
      } /* end type switch */
      break;
  } /* end scale switch */

  /* Output tifs from slices */
  /* Allow for compressed multi-echo loop */
  image=volindex/ne;
  echo=volindex%ne;
  /* Loop over receivers and slices */
  if ((nr>1) && (type != MK)) {
    for (i=0;i<nr;i++) {
      for (j=0;j<dim3;j++) {
        slice=startpos+j;
        sprintf(filename,"%s/image%.3dslice%.3decho%.3dcoil%.3d.tif",dirname,image+1,slice+1,echo+1,i+1);
        wtif2D(filename,d,i,j,type,scalefactor);
      }
    }
  } else {
    for (j=0;j<dim3;j++) {
      slice=startpos+j;
      sprintf(filename,"%s/image%.3dslice%.3decho%.3d.tif",dirname,image+1,slice+1,echo+1);
      wtif2D(filename,d,0,j,type,scalefactor);
    }
  }

#ifdef DEBUG
  rtn=gettimeofday(&tp, NULL);
  t2=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
  fprintf(stdout,"  ... and writing in %s: took %f secs\n",dirname,t2-t1);
#endif

  return(0);
}

int wtif2D(char *filename,struct data *d,int receiver,int slice,int type,double scale)
{
  TIFF *tp;
  char *image; /* uint16 *image; if we want image scaled from 0 to 65535 */
  uint16 spp,bpp,photo,resunit;
  uint32 width,height;
  float xres,yres;
  double re,im,M;
  int dim1,dim2;
  int i,j;
  int ix1;

  /* Check that type is valid */
  if ((type != RE) && (type != IM) && (type != MG) && (type != PH) 
    && (type != MK) && (type != SM)) {
    fprintf(stderr,"\n%s: %s()\n",__FILE__,__FUNCTION__);
    fprintf(stderr,"  Invalid 5th argument %s(*,*,*,*,'type',*)\n",__FUNCTION__);
    fflush(stderr);
    return(1);
  }

  /* Data dimensions */
  dim1=d->np/2; dim2=d->nv;

  /* Allocate memory for tif */
  if ((image = (char *)_TIFFmalloc(dim2*dim1*sizeof(char))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
  /* (uint16 *)_TIFFmalloc(dim2*dim1*sizeof(uint16))
     if we want image scaled from 0 to 65535 */

  /* Fill image with slice data, scaling as we go */
  switch(type) {
    case MG: /* Magnitude */
      for(i=0;i<dim2;i++) {
        for (j=0;j<dim1;j++) {
          ix1=i*dim1+j;
          re=fabs(d->data[receiver][slice][ix1][0]);
          im=fabs(d->data[receiver][slice][ix1][1]);
          M=scale*sqrt(re*re+im*im);
          if (M > TIFMAX) M=TIFMAX;
          image[ix1] = (char)M; /* (int) if we want image scaled from 0 to 65535 */
        }
      }
      break;
    case RE: /* Real */
      for(i=0;i<dim2;i++) {
        for (j=0;j<dim1;j++) {
          ix1=i*dim1+j;
          M=d->data[receiver][slice][ix1][0];
          M*=scale/2;
          M+=TIFMAX/2;
          if (M > TIFMAX) M=TIFMAX;
          if (M < 0.0) M=0.0;
          image[ix1] = (char)M; /* (int) if we want image scaled from 0 to 65535 */
        }
      }
      break;
    case IM: /* Imaginary */
      for(i=0;i<dim2;i++) {
        for (j=0;j<dim1;j++) {
          ix1=i*dim1+j;
          M=d->data[receiver][slice][ix1][1];
          M*=scale/2;
          M+=TIFMAX/2;
          if (M > TIFMAX) M=TIFMAX;
          if (M < 0.0) M=0.0;
          image[ix1] = (char)M; /* (int) if we want image scaled from 0 to 65535 */
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
          M*=scale/2;
          M+=TIFMAX/2;
          if (M > TIFMAX) M=TIFMAX;
          if (M < 0.0) M=0.0;
          image[ix1] = (char)M; /* (int) if we want image scaled from 0 to 65535 */
        }
      }
      break;
    case MK: /* Mask */
      for(i=0;i<dim2;i++) {
        for (j=0;j<dim1;j++) {
          ix1=i*dim1+j;
          image[ix1] = (char)(d->mask[slice][ix1]*scale);
                       /* (int) if we want image scaled from 0 to 65535 */
        }
      }
      break;
    case SM: /* Sensitivity map */
      for(i=0;i<dim2;i++) {
        for (j=0;j<dim1;j++) {
          ix1=i*dim1+j;
          re=fabs(d->data[receiver][slice][ix1][0]);
          im=fabs(d->data[receiver][slice][ix1][1]);
          M=scale*sqrt(re*re+im*im);
          if (M > TIFMAX) M=TIFMAX;
          image[ix1] = (char)M; /* (int) if we want image scaled from 0 to 65535 */
        }
      }
      break;
    default:
      break;
  } /* end type switch */

  /* Set image width and height */
  width=(uint32)dim1;
  height=(uint32)dim2;

  /* Open TIFF file */
  TIFFSetErrorHandler(0); /* suppress error messages */
  tp=TIFFOpen(filename,"w");
  if (!tp) {
    fprintf(stderr,"\n%s: %s()\n",__FILE__,__FUNCTION__);
    fprintf(stderr,"  Unable to open %s\n",filename);
    fflush(stderr);
    return(1);
  }

  /* Set fields */
  spp=1; /* Samples per pixel */
  bpp=8; /* Bits per sample - bpp=16 if we want image scaled from 0 to 65535 */
  photo=PHOTOMETRIC_MINISBLACK;
  TIFFSetField(tp,TIFFTAG_IMAGEWIDTH,width/spp);
  TIFFSetField(tp,TIFFTAG_BITSPERSAMPLE,bpp);
  TIFFSetField(tp,TIFFTAG_SAMPLESPERPIXEL,spp);
  TIFFSetField(tp,TIFFTAG_PLANARCONFIG,PLANARCONFIG_CONTIG);
  TIFFSetField(tp,TIFFTAG_PHOTOMETRIC,photo);
  TIFFSetField(tp,TIFFTAG_ORIENTATION,ORIENTATION_BOTLEFT);
  /* It is good to set resolutions too (but it is not nesessary) */
  xres=yres=100;
  resunit=RESUNIT_INCH;
  TIFFSetField(tp,TIFFTAG_XRESOLUTION,xres);
  TIFFSetField(tp,TIFFTAG_YRESOLUTION,yres);
  TIFFSetField(tp,TIFFTAG_RESOLUTIONUNIT,resunit);

  /* Write image */
  for (j=0;j<height;j++)
    TIFFWriteScanline(tp,&image[j*width],j,0);

  TIFFClose(tp);
  _TIFFfree(image);
  return(0);
}
