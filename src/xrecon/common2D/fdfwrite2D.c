/* fdfwrite2D.c */
/*---------------------------------------------------------------------------*/
/*                                                                           */
/* fdfwrite2D.c: 2D fdf writing routines                                     */
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

/* Maximum characters for fdf header */
#define FDFHDRLEN 2048

static int IMAGEOFFSET=0;

void w2Dfdfs(struct data *d,int type,int precision,int volindex)
{
  int output,datamode;
  char outdir[MAXPATHLEN];

#ifdef DEBUG
  struct timeval tp;
  double t1,t2;
  int rtn;
#endif

  /* Call w2Dfdfs with a negative volindex to flag a skipped reference scan */
  /* e.g. EPI reference scans */
  if (volindex < 0) {
    IMAGEOFFSET=d->vol-d->outvol+1;
    return;
  }

  /* This function checks the output requested and sets the output directory
     accordingly. Type 'VJ' flags that we should inspect the values of VnmrJ 
     parameters in order to figure the requested output using functions
     magnitude2Dfdfs() and other2Dfdfs(). */

#ifdef DEBUG
  fprintf(stdout,"\n%s: %s()\n",__FILE__,__FUNCTION__);
  fprintf(stdout,"  Writing fdf data");
  rtn=gettimeofday(&tp, NULL);
  t1=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
  fflush(stdout);
#endif

  /* Check that type is valid */
  if (!validtype(type)) {
    fprintf(stderr,"\n%s: %s()\n",__FILE__,__FUNCTION__);
    fprintf(stderr,"  Invalid 2nd argument %s(*,'type',*,*)\n",__FUNCTION__);
    fflush(stderr);
    return;
  }

  datamode=FID;
  /* If FT has been done flag it as IMAGE */
  if ((d->dimstatus[0] & FFT) || (d->dimstatus[1] & FFT)) datamode=IMAGE;

  /* Generate output according to requested type */
  switch (type) {
    case VJ: /* Generate output according to VnmrJ parameters */
      switch(datamode) {
        case FID:
          /* When there is data from more than one receiver, magnitude output
             can be from each individual receiver or combined, depending
             on the user selection */
          magnitude2Dfdfs(d,"rawMG","rawIR","rawMG",MG,precision,volindex);
          /* Otherwise output is always from each individual receiver */
          other2Dfdfs(d,"rawPH","rawPH",PH,precision,volindex);
          other2Dfdfs(d,"rawRE","rawRE",RE,precision,volindex);
          other2Dfdfs(d,"rawIM","rawIM",IM,precision,volindex);
          break;
        default:
          /* When there is data from more than one receiver, magnitude output
             can be from each individual receiver or combined, depending
             on the user selection */
          /* We use output to flag output to directory recon */
          output=magnitude2Dfdfs(d,"imMG","imIR","recon",MG,precision,volindex);
          /* Otherwise output is always from each individual receiver */
          other2Dfdfs(d,"imPH","reconPH",PH,precision,volindex);
          other2Dfdfs(d,"imRE","reconRE",RE,precision,volindex);
          other2Dfdfs(d,"imIM","reconIM",IM,precision,volindex);
          /* Always generate output to directory recon */
          if (!output) {
            if (d->nr>1) {/* Multiple receivers */
              /* Check for VnmrJ recon to figure if we are offline */
              if (vnmrj_recon) gen2Dfdfs(d,'c',"recon",MG,precision,volindex);
              else gen2Dfdfs(d,'c',"",MG,precision,volindex);
            } else {/* Single receiver */
              /* Check for VnmrJ recon to figure if we are offline */
              if (vnmrj_recon) gen2Dfdfs(d,'s',"recon",MG,precision,volindex);
              else gen2Dfdfs(d,'s',"",MG,precision,volindex);
            }
          }
          break;
      } /* end d->datamode switch */
      break;
    default: /* Output not generated according to VnmrJ parameters */
      /* Set up output directory (outdir) according to type */
      switch(datamode) {
//      switch(d->datamode) {
        case FID: strcpy(outdir,"raw"); break;
        default:
          switch (type) {
            case MG: strcpy(outdir,"recon"); break;
            case PH: strcpy(outdir,"recon"); break;
            case RE: strcpy(outdir,"recon"); break;
            case IM: strcpy(outdir,"recon"); break;
            default: strcpy(outdir,""); break;
          }
          break;
      } /* end d->datamode switch */
      switch(type) {
        case MG:  if (datamode == FID) strcat(outdir,"MG"); break; /* Magnitude */
//        case MG:  if (d->datamode == FID) strcat(outdir,"MG"); break; /* Magnitude */
        case PH:  strcat(outdir,"PH");    break; /* Phase */
        case RE:  strcat(outdir,"RE");    break; /* Real */
        case IM:  strcat(outdir,"IM");    break; /* Imaginary */
        case MK:  strcat(outdir,"mask");  break; /* Mask */
        case RMK: strcat(outdir,"maskR"); break; /* Reverse mask of magnitude */
        case SM:  strcat(outdir,"smap");  break; /* Sensitivity maps */
        case GF:  strcat(outdir,"gmap");  break; /* Geometry factor */
        case RS:  strcat(outdir,"Rsnr");  break; /* Relative SNR */
      } /* end type switch */
      /* Select output */
      switch(type) {
        case MG:
          if (d->nr>1) {
            gen2Dfdfs(d,'c',outdir,type,precision,volindex);
            gen2Dfdfs(d,'i',outdir,type,precision,volindex);
          } else gen2Dfdfs(d,'s',outdir,type,precision,volindex);
          break;
        case MK:  gen2Dfdfs(d,'s',outdir,type,precision,volindex); break;
        case RMK: gen2Dfdfs(d,'c',outdir,type,precision,volindex); break;
        case GF:  gen2Dfdfs(d,'g',outdir,type,precision,volindex); break;
        case RS:  gen2Dfdfs(d,'r',outdir,type,precision,volindex); break;
        default:
          if (d->nr>1) gen2Dfdfs(d,'i',outdir,type,precision,volindex);
          else gen2Dfdfs(d,'s',outdir,type,precision,volindex);
          break;
      } /* end type switch */
      break;
  } /* end type switch */

#ifdef DEBUG
  rtn=gettimeofday(&tp, NULL);
  t2=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
  fprintf(stdout,"\n  Took %f secs\n",t2-t1);
  fflush(stdout);
#endif

}

int magnitude2Dfdfs(struct data *d,char *par,char *parIR,char *outdir,int type,int precision,int volindex)
{
  int output=0;
  if ((spar(d,par,"y"))) { /* Output selected by par='y' */
    if (d->nr>1) { /* Multiple receivers */
      if ((spar(d,parIR,"y"))) /* Individual Receiver output */
        gen2Dfdfs(d,'i',outdir,type,precision,volindex);
      else {/* Combined output */
        /* Check for VnmrJ recon to figure if we are offline */
        if (vnmrj_recon) gen2Dfdfs(d,'c',outdir,type,precision,volindex);
        else gen2Dfdfs(d,'c',"",type,precision,volindex);
        output=1;
      }
    } else { /* Single receiver */
        /* Check for VnmrJ recon to figure if we are offline */
        if (vnmrj_recon) gen2Dfdfs(d,'s',outdir,type,precision,volindex);
        else gen2Dfdfs(d,'s',"",type,precision,volindex);
      output=1;
    }
  }
  return(output);
}

void other2Dfdfs(struct data *d,char *par,char *outdir,int type,int precision,int volindex)
{
  if ((spar(d,par,"y"))) { /* Output selected by par='y' */
    if (d->nr>1) /* Multiple receivers */
      gen2Dfdfs(d,'i',outdir,type,precision,volindex);
    else /* Single receiver */
      gen2Dfdfs(d,'s',outdir,type,precision,volindex);
  }
}

void gen2Dfdfs(struct data *d,int mode,char *outdir,int type,int precision,int volindex)
{
  char basename[MAXPATHLEN],dirname[MAXPATHLEN],filename[MAXPATHLEN];
  int ne;
  int start,end;
  int startpos,endpos,blockslices;
  int i,j;
  int slice,image,echo;
  int offline=FALSE;

  /* This function checks the output type requested and sets the output
     filename accordingly. 2D FDF data from each slice is output according
     to the specified mode using functions w2Dfdf() and wcomb2Dfdf().
     These functions output data either from individual receivers (w2Dfdf)
     or using a combination of data from all receivers (wcomb2Dfdf). */

  /* Check that type is valid */
  if (!validtype(type) || (type == VJ)) {
    fprintf(stderr,"\n%s: %s()\n",__FILE__,__FUNCTION__);
    fprintf(stderr,"  Invalid 4th argument %s(*,*,*,'type',*,*)\n",__FUNCTION__);
    fflush(stderr);
    return;
  }

  /* For mask and reverse mask of magnitude we allow the start and end slices
     to be defined. */
  switch(type) {
    case MK:
      start=(int)*val("maskstartslice",&d->p);
      if ((start<0) || (start>d->ns)) start=1;
      start--;
      end=(int)*val("maskendslice",&d->p);
      if ((end<1) || (end>d->ns)) end=d->ns;
      break;
    case RMK:
      start=(int)*val("maskstartslice",&d->p);
      if ((start<0) || (start>d->ns)) start=1;
      start--;
      end=(int)*val("maskendslice",&d->p);
      if ((end<1) || (end>d->ns)) end=d->ns;
      break;
    default:
      start=0;
      end=d->ns;
      break;
  } /* end type switch */

  startpos=d->startpos; endpos=d->endpos;

  if ((start>=endpos) || (end<=startpos)) {
#ifdef DEBUG
  fprintf(stdout,"\n  %s(): skipping writing of block %d (of %d)\n",__FUNCTION__,d->block+1,d->nblocks);
#endif
    return;
  }

  /* Adjust start and end according to data block */
  start=start-startpos;
  end=end-startpos;
  if (start<0) start=0;
  blockslices=endpos-startpos;
  if (end>blockslices) end=blockslices;

#ifdef DEBUG
  fprintf(stdout,"\n  Block %d (of %d) %s() start slice index = %d, end slice index = %d",d->block+1,d->nblocks,__FUNCTION__,start,end-1);
  fflush(stdout);
#endif

  /* Check for VnmrJ recon to figure if we are offline */
  if (vnmrj_recon) strcpy(basename,vnmrj_path);
  else {
    for (i=0;i<=strlen(d->file)-9;i++)
      basename[i]=d->file[i];
    basename[i]=0;
    offline=TRUE;
  }

  /* Number of echoes */
  ne=(int)*val("ne",&d->p);
  if (ne < 1) ne=1; /* Set ne to 1 if 'ne' does not exist */

  /* Allow for compressed multi-echo loop */
  image=(volindex-IMAGEOFFSET)/ne;
  echo=(volindex-IMAGEOFFSET)%ne;

  switch(mode) {
    case 'i': /* Individual output */
      for (i=0;i<d->nr;i++) {
        sprintf(dirname,"%s%s%.3d",basename,outdir,i+1);
        if (offline) strcat(dirname,".img");
        createdir(dirname);
        for (j=start;j<end;j++) {
          slice=startpos+j;
          sprintf(filename,"%s/slice%.3dimage%.3decho%.3d.fdf",dirname,slice+1,image+1,echo+1);
          w2Dfdf(filename,d,image,slice,j,echo,i,type,precision);
        }
        /* Write procpar */
        sprintf(filename,"%s/procpar",dirname);
        wprocpar(d,filename);
      }
      break;
    case 'c': /* Combined output (Magnitude only) */
      sprintf(dirname,"%s%s",basename,outdir);
      if (offline) strcat(dirname,".img");
      createdir(dirname);
      for (j=start;j<end;j++) {
        slice=startpos+j;
        sprintf(filename,"%s/slice%.3dimage%.3decho%.3d.fdf",dirname,slice+1,image+1,echo+1);
        wcomb2Dfdf(filename,d,image,slice,j,echo,type,precision);
      }
      /* Write procpar */
      sprintf(filename,"%s/procpar",dirname);
      wprocpar(d,filename);
      break;
    case 's': /* Single receiver */
      sprintf(dirname,"%s%s",basename,outdir);
      if (offline) strcat(dirname,".img");
      createdir(dirname);
      for (j=start;j<end;j++) {
        slice=startpos+j;
        sprintf(filename,"%s/slice%.3dimage%.3decho%.3d.fdf",dirname,slice+1,image+1,echo+1);
        w2Dfdf(filename,d,image,slice,j,echo,0,type,precision);
      }
      /* Write procpar */
      sprintf(filename,"%s/procpar",dirname);
      wprocpar(d,filename);
      break;
    case 'g': /* Geometry Factor output */
      sprintf(dirname,"%s%s",basename,outdir);
      if (offline) strcat(dirname,".img");
      createdir(dirname);
      for (j=start;j<end;j++) {
        slice=startpos+j;
        sprintf(filename,"%s/slice%.3dimage%.3decho%.3d.fdf",dirname,slice+1,image+1,echo+1);
        w2Dfdf(filename,d,image,slice,j,echo,0,type,precision);
      }
      /* Write procpar */
      sprintf(filename,"%s/procpar",dirname);
      wprocpar(d,filename);
      break;
    case 'r': /* Relative SNR */
      sprintf(dirname,"%s%s",basename,outdir);
      if (offline) strcat(dirname,".img");
      createdir(dirname);
      for (j=start;j<end;j++) {
        slice=startpos+j;
        sprintf(filename,"%s/slice%.3dimage%.3decho%.3d.fdf",dirname,slice+1,image+1,echo+1);
        w2Dfdf(filename,d,image,slice,j,echo,1,type,precision);
      }
      /* Write procpar */
      sprintf(filename,"%s/procpar",dirname);
      wprocpar(d,filename);
      break;
    default:
      fprintf(stderr,"\n%s: %s()\n",__FILE__,__FUNCTION__);
      fprintf(stderr,"  Invalid 2nd argument %s(*,'mode',*,*,*,*)\n",__FUNCTION__);
      fflush(stderr);
      break;
  } /* end mode switch */
}

void w2Dfdf(char *filename,struct data *d,int image,int slice,int sliceindex,int echo,int receiver,int type,int precision)
{
  FILE *f_out;
  float *floatdata;
  double *doubledata;
  char null[1];
  double re,im,M,scale;
  int dim1,dim2;
  int i,j;
  int ix1,ix2;
  int datamode;

  /* Check that type is valid */
  if (!validtype(type) || (type == VJ)) {
    fprintf(stderr,"\n%s: %s()\n",__FILE__,__FUNCTION__);
    fprintf(stderr,"  Invalid 8th argument %s(*,*,*,*,*,*,*,'type',*)\n",__FUNCTION__);
    fflush(stderr);
    return;
  }

  /* Data dimensions */
  dim1=d->np/2; dim2=d->nv;

  datamode=FID;
  /* If FT has been done flag it as IMAGE */
  if ((d->dimstatus[0] & FFT) || (d->dimstatus[1] & FFT)) datamode=IMAGE;

  /* Generate fdf header */
  gen2Dfdfhdr(d,image+IMAGEOFFSET,slice,echo,receiver,type,precision);

  /* Set NULL terminator for fdf header */
  *null=(char)0;

  /* For Sensitivity Map, Geometry Factor and Relative SNR we just output
     the magnitude. For type 'MG' output is scaled by aipScale so we use
     type 'SM' which is not scaled */
  switch(type) {
    case GF: type = SM; break; /* Geometry Factor */
    case RS: type = SM; break; /* Relative SNR */
  }

  /* Image scaling */
  scale=*val("aipScale",&d->p);
  if (FP_EQ(scale,0.0)) scale=1.0;

  /* Allocate memory and write fdf header and image */
  switch(precision) {
    case FLT32: /* 32 bit float */
      if ((floatdata = (float *)malloc(dim2*dim1*sizeof(float))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
      switch(type) {
        case MG: /* Magnitude */
          for(i=0;i<dim2;i++) {
            for (j=0;j<dim1;j++) {
              ix1=i*dim1+j;
              ix2=(dim1-j-1)*dim2+dim2-i-1;
              re=fabs(d->data[receiver][sliceindex][ix1][0]);
              im=fabs(d->data[receiver][sliceindex][ix1][1]);
              M=sqrt(re*re+im*im);
              floatdata[ix2] = (float)M;
            }
          }
          break;
        case PH: /* Phase */
          for(i=0;i<dim2;i++) {
            for (j=0;j<dim1;j++) {
              ix1=i*dim1+j;
              ix2=(dim1-j-1)*dim2+dim2-i-1;
              re=d->data[receiver][sliceindex][ix1][0];
              im=d->data[receiver][sliceindex][ix1][1];
              /* atan2 returns values (in radians) between +PI and -PI */
              M=atan2(im,re);
              floatdata[ix2] = (float)M;
            }
          }
          break;
        case RE: /* Real */
          for(i=0;i<dim2;i++) {
            for (j=0;j<dim1;j++) {
              ix1=i*dim1+j;
              ix2=(dim1-j-1)*dim2+dim2-i-1;
              floatdata[ix2] = (float)d->data[receiver][sliceindex][ix1][0];
            }
          }
          break;
        case IM: /* Imaginary */
          for(i=0;i<dim2;i++) {
            for (j=0;j<dim1;j++) {
              ix1=i*dim1+j;
              ix2=(dim1-j-1)*dim2+dim2-i-1;
              floatdata[ix2] = (float)d->data[receiver][sliceindex][ix1][1];
            }
          }
          break;
        case MK: /* Mask */
          for(i=0;i<dim2;i++) {
            for (j=0;j<dim1;j++) {
              ix1=i*dim1+j;
              ix2=(dim1-j-1)*dim2+dim2-i-1;
              floatdata[ix2] = (float)d->mask[sliceindex][ix1];
            }
          }
          break;
        case RMK: /* Reverse Mask of Magnitude */
          for(i=0;i<dim2;i++) {
            for (j=0;j<dim1;j++) {
              ix1=i*dim1+j;
              ix2=(dim1-j-1)*dim2+dim2-i-1;
              if (!d->mask[sliceindex][ix1]) {
                re=fabs(d->data[receiver][sliceindex][ix1][0]);
                im=fabs(d->data[receiver][sliceindex][ix1][1]);
                M=sqrt(re*re+im*im);
                floatdata[ix2] = (float)M;
              } else {
                floatdata[ix2] = 0.0;
              }
            }
          }
          break;
        case SM: /* Sensitivity Map */
          for(i=0;i<dim2;i++) {
            for (j=0;j<dim1;j++) {
              ix1=i*dim1+j;
              ix2=(dim1-j-1)*dim2+dim2-i-1;
              re=fabs(d->data[receiver][sliceindex][ix1][0]);
              im=fabs(d->data[receiver][sliceindex][ix1][1]);
              M=sqrt(re*re+im*im);
              floatdata[ix2] = (float)M;
            }
          }
        default:
          break;
      } /* end type switch */
      /* Scale image data */
      switch(datamode) {
//      switch(d->datamode) {
        case IMAGE:
          if ((type == RE) || (type == IM) || (type == MG)) {
            for(i=0;i<dim2;i++) {
              for (j=0;j<dim1;j++) {
                ix1=i*dim1+j;
                floatdata[ix1] *= scale;
              }
            }
          }
          break;
        default:
          break;
      } /* end d->datamode switch */
      /* Write data */
      if ((f_out = fopen(filename, "w")) == NULL) {
        fprintf(stderr,"\n%s: %s()\n",__FILE__,__FUNCTION__);
        fprintf(stderr,"  Unable to write to %s\n",filename);
        fflush(stderr);
        free(d->fdfhdr); /* Wipe fdf header */
        return;
      }
      fprintf(f_out,"%s",d->fdfhdr);
      fwrite(null,sizeof(char),1,f_out);
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
              ix2=(dim1-j-1)*dim2+dim2-i-1;
              re=fabs(d->data[receiver][sliceindex][ix1][0]);
              im=fabs(d->data[receiver][sliceindex][ix1][1]);
              M=sqrt(re*re+im*im);
              doubledata[ix2] = M;
            }
          }
          break;
        case PH: /* Phase */
          for(i=0;i<dim2;i++) {
            for (j=0;j<dim1;j++) {
              ix1=i*dim1+j;
              ix2=(dim1-j-1)*dim2+dim2-i-1;
              re=d->data[receiver][sliceindex][ix1][0];
              im=d->data[receiver][sliceindex][ix1][1];
              /* atan2 returns values (in radians) between +PI and -PI */
              M=atan2(im,re);
              doubledata[ix2] = M;
            }
          }
          break;
        case RE: /* Real */
          for(i=0;i<dim2;i++) {
            for (j=0;j<dim1;j++) {
              ix1=i*dim1+j;
              ix2=(dim1-j-1)*dim2+dim2-i-1;
              doubledata[ix2] = d->data[receiver][sliceindex][ix1][0];
            }
          }
          break;
        case IM: /* Imaginary */
          for(i=0;i<dim2;i++) {
            for (j=0;j<dim1;j++) {
              ix1=i*dim1+j;
              ix2=(dim1-j-1)*dim2+dim2-i-1;
              doubledata[ix2] = d->data[receiver][sliceindex][ix1][1];
            }
          }
          break;
        case MK: /* Mask */
          for(i=0;i<dim2;i++) {
            for (j=0;j<dim1;j++) {
              ix1=i*dim1+j;
              ix2=(dim1-j-1)*dim2+dim2-i-1;
              doubledata[ix2] = (double)d->mask[sliceindex][ix1];
            }
          }
          break;
        case RMK: /* Reverse Mask of Magnitude */
          for(i=0;i<dim2;i++) {
            for (j=0;j<dim1;j++) {
              ix1=i*dim1+j;
              ix2=(dim1-j-1)*dim2+dim2-i-1;
	      if (!d->mask[sliceindex][ix1]) {
                re=fabs(d->data[receiver][sliceindex][ix1][0]);
                im=fabs(d->data[receiver][sliceindex][ix1][1]);
                M=sqrt(re*re+im*im);
                doubledata[ix2] = M;
              } else {
                doubledata[ix2] = 0.0;
              }
            }
          }
          break;
        case SM: /* Sensitivity map */
          for(i=0;i<dim2;i++) {
            for (j=0;j<dim1;j++) {
              ix1=i*dim1+j;
              ix2=(dim1-j-1)*dim2+dim2-i-1;
              re=fabs(d->data[receiver][sliceindex][ix1][0]);
              im=fabs(d->data[receiver][sliceindex][ix1][1]);
              M=sqrt(re*re+im*im);
              doubledata[ix2] = M;
            }
          }
          break;
        default:
          break;
      } /* end type switch */
      /* Scale image data */
      switch(datamode) {
//      switch(d->datamode) {
        case IMAGE:
          if ((type == RE) || (type == IM) || (type == MG)) {
            for(i=0;i<dim2;i++) {
              for (j=0;j<dim1;j++) {
                ix1=i*dim1+j;
                doubledata[ix1] *= scale;
              }
            }
          }
          break;
        default:
          break;
      } /* end d->datamode switch */
      /* Write data */
      if ((f_out = fopen(filename, "w")) == NULL) {
        fprintf(stderr,"\n%s: %s()\n",__FILE__,__FUNCTION__);
        fprintf(stderr,"  Unable to write to %s\n",filename);
        fflush(stderr);
        free(d->fdfhdr); /* Wipe fdf header */
        return;
      }
      fprintf(f_out,"%s",d->fdfhdr);
      fwrite(null,sizeof(char),1,f_out);
      fwrite(doubledata,sizeof(double),dim1*dim2,f_out);
      fclose(f_out);
      free(doubledata);
      break;
    default:
      fprintf(stderr,"\n%s: %s()\n",__FILE__,__FUNCTION__);
      fprintf(stderr,"  Invalid 9th argument %s(*,*,*,*,*,*,*,*,'precision')\n",__FUNCTION__);
      fflush(stderr);
      break;
  } /* end precision switch */

  /* Wipe fdf header */
  free(d->fdfhdr);
}

void wcomb2Dfdf(char *filename,struct data *d,int image,int slice,int sliceindex,int echo,int type,int precision)
{
  FILE *f_out;
  float *floatdata;
  double *doubledata;
  char null[1];
  double re,im,M,scale;
  int dim1,dim2,nr;
  int i,j,k;
  int ix1,ix2;
  int datamode;

  /* Check that type is valid */
  if ((type != MG) && (type != RMK)) {
    fprintf(stderr,"\n%s: %s()\n",__FILE__,__FUNCTION__);
    fprintf(stderr,"  Invalid 7th argument %s(*,*,*,*,*,*,'type',*)\n",__FUNCTION__);
    fflush(stderr);
    return;
  }

  datamode=FID;
  /* If FT has been done flag it as IMAGE */
  if ((d->dimstatus[0] & FFT) || (d->dimstatus[1] & FFT)) datamode=IMAGE;

  /* Data dimensions */
  dim1=d->np/2; dim2=d->nv; nr=d->nr;

  /* Generate fdf header */
  gen2Dfdfhdr(d,image+IMAGEOFFSET,slice,echo,0,MG,precision);

  /* Set NULL terminator for fdf header */
  *null=(char)0;

  /* Image scaling */
  scale=*val("aipScale",&d->p);
  if (FP_EQ(scale,0.0)) scale=1.0;

  /* Allocate memory and write fdf header and image */
  switch(precision) {
    case FLT32: /* 32 bit float */
      if ((floatdata = (float *)malloc(dim2*dim1*sizeof(float))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
      switch(type) {
        case MG: /* Magnitude */
          for(i=0;i<dim2;i++) {
            for (j=0;j<dim1;j++) {
              ix1=i*dim1+j;
              ix2=(dim1-j-1)*dim2+dim2-i-1;
              M=0.0;
              for (k=0;k<nr;k++) {
                re=fabs(d->data[k][sliceindex][ix1][0]);
                im=fabs(d->data[k][sliceindex][ix1][1]);
                M+=(re*re+im*im);
              }
              floatdata[ix2] = (float)sqrt(M);
            }
          }
          break;
        case RMK: /* Reverse mask of magnitude */
          for(i=0;i<dim2;i++) {
            for (j=0;j<dim1;j++) {
              ix1=i*dim1+j;
              ix2=(dim1-j-1)*dim2+dim2-i-1;
              if (!d->mask[sliceindex][ix1]) {
                M=0.0;
                for (k=0;k<nr;k++) {
                  re=fabs(d->data[k][sliceindex][ix1][0]);
                  im=fabs(d->data[k][sliceindex][ix1][1]);
                  M+=(re*re+im*im);
                }
                floatdata[ix2] = (float)sqrt(M);
              } else {
                floatdata[ix2] = 0.0;
              }
            }
          }
          break;
      } /* end type switch */
      /* Scale image data */
      switch(datamode) {
//      switch(d->datamode) {
        case IMAGE:
          for(i=0;i<dim2;i++) {
            for (j=0;j<dim1;j++) {
              ix1=i*dim1+j;
              floatdata[ix1] *= scale;
            }
          }
          break;
        default:
          break;
      } /* end d->datamode switch */
      /* Write data */
      if ((f_out = fopen(filename, "w")) == NULL) {
        fprintf(stderr,"\n%s: %s()\n",__FILE__,__FUNCTION__);
        fprintf(stderr,"  Unable to write to %s\n",filename);
        fflush(stderr);
        free(d->fdfhdr); /* Wipe fdf header */
        return;
      }
      fprintf(f_out,"%s",d->fdfhdr);
      fwrite(null,sizeof(char),1,f_out);
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
              ix2=(dim1-j-1)*dim2+dim2-i-1;
              M=0.0;
              for (k=0;k<nr;k++) {
                re=fabs(d->data[k][sliceindex][ix1][0]);
                im=fabs(d->data[k][sliceindex][ix1][1]);
                M+=(re*re+im*im);
              }
              doubledata[ix2] = sqrt(M);
            }
          }
          break;
        case RMK: /* Reverse mask of magnitude */
          for(i=0;i<dim2;i++) {
            for (j=0;j<dim1;j++) {
              ix1=i*dim1+j;
              ix2=(dim1-j-1)*dim2+dim2-i-1;
              if (!d->mask[sliceindex][ix1]) {
                M=0.0;
                for (k=0;k<nr;k++) {
                  re=fabs(d->data[k][sliceindex][ix1][0]);
                  im=fabs(d->data[k][sliceindex][ix1][1]);
                  M+=(re*re+im*im);
                }
                doubledata[ix2] = sqrt(M);
              } else {
                doubledata[ix2] = 0.0;
              }
            }
          }
          break;
      } /* end type switch */
      /* Scale image data */
      switch(datamode) {
//      switch(d->datamode) {
        case IMAGE:
          for(i=0;i<dim2;i++) {
            for (j=0;j<dim1;j++) {
              ix1=i*dim1+j;
              doubledata[ix1] *= scale;
            }
          }
          break;
        default:
          break;
      } /* end d->datamode switch */
      /* Write data */
      if ((f_out = fopen(filename, "w")) == NULL) {
        fprintf(stderr,"\n%s: %s()\n",__FILE__,__FUNCTION__);
        fprintf(stderr,"  Unable to write to %s\n",filename);
        fflush(stderr);
        free(d->fdfhdr); /* Wipe fdf header */
        return;
      }
      fprintf(f_out,"%s",d->fdfhdr);
      fwrite(null,sizeof(char),1,f_out);
      fwrite(doubledata,sizeof(double),dim1*dim2,f_out);
      fclose(f_out);
      free(doubledata);
      break;
    default:
      fprintf(stderr,"\n%s: %s()\n",__FILE__,__FUNCTION__);
      fprintf(stderr,"  Invalid 8th argument %s(*,*,*,*,*,*,*,'precision')\n",__FUNCTION__);
      fflush(stderr);
      break;
  } /* end precision switch */

  /* Wipe fdf header */
  free(d->fdfhdr);
}

void gen2Dfdfhdr(struct data *d,int image,int slice,int echo,int receiver,int type,int precision)
{

  char str[100];
  int cycle,n,id;
  double pro=0.0,ppe=0.0,psi=0.0,phi=0.0,theta=0.0;
  double cospsi,cosphi,costheta;
  double sinpsi,sinphi,sintheta;
  double or0,or1,or2,or3,or4,or5,or6,or7,or8;
  double value;
  int dim1,dim2,ns,nr;
  int ne;
  int i,j,add;
  int align=0,hdrlen,pad_cnt;
  int *intval;
  double *dblval;
  char **strval;
  int datamode;

  /* Check that type is valid */
  if (!validtype(type) || (type == VJ)) {
    fprintf(stderr,"\n%s: %s()\n",__FILE__,__FUNCTION__);
    fprintf(stderr,"  Invalid 6th argument %s(*,*,*,*,*,'type',*)\n",__FUNCTION__);
    fflush(stderr);
    return;
  }

  datamode=FID;
  /* If FT has been done flag it as IMAGE */
  if ((d->dimstatus[0] & FFT) || (d->dimstatus[1] & FFT)) datamode=IMAGE;

  /* Allocate for header */
  if ((d->fdfhdr = (char *)malloc(FDFHDRLEN*sizeof(char))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);

  /* Data dimensions */
  dim1=d->np/2; dim2=d->nv; ns=d->ns; nr=d->nr;

  /* Number of echoes */
  ne=(int)*val("ne",&d->p);
  if (ne < 1) ne=1; /* Set ne to 1 if 'ne' does not exist */

  /* Set up for orientation */
  if (cyclefaster("pss","pro",&d->a)) value=getelem(d,"pro",image*ns);
  else pro=getelem(d,"pro",image);
  if (cyclefaster("pss","ppe",&d->a)) value=getelem(d,"ppe",image*ns);
  else ppe=getelem(d,"ppe",image);
  if (cyclefaster("pss","psi",&d->a)) value=getelem(d,"psi",image*ns);
  else psi=getelem(d,"psi",image);
  if (cyclefaster("pss","phi",&d->a)) value=getelem(d,"phi",image*ns);
  else phi=getelem(d,"phi",image);
  if (cyclefaster("pss","theta",&d->a)) value=getelem(d,"theta",image*ns);
  else theta=getelem(d,"theta",image);

  /* Create header */
  sprintf(d->fdfhdr,"#!/usr/local/fdf/startup\n");
  strcat(d->fdfhdr,"float  rank = 2;\n");
  strcat(d->fdfhdr,"char  *spatial_rank = \"2dfov\";\n");
  switch(precision) {
    case FLT32: /* 32 bit float */
      strcat(d->fdfhdr,"char  *storage = \"float\";\n");
      strcat(d->fdfhdr,"float  bits = 32;\n");
      break;
    case DBL64: /* 64 bit double */
      strcat(d->fdfhdr,"char  *storage = \"double\";\n");
      strcat(d->fdfhdr,"float  bits = 64;\n");
      break;
    default:
      strcat(d->fdfhdr,"char  *storage = \"not supported\";\n");
      strcat(d->fdfhdr,"float  bits = ?;\n");
      break;
  } /* end precision switch */
  switch(type) {
    case MG: /* Magnitude */
      strcat(d->fdfhdr,"char  *type = \"absval\";\n");
      break;
    case RE: /* Real */
      strcat(d->fdfhdr,"char  *type = \"real\";\n");
      break;
    case IM: /* Imaginary */
      strcat(d->fdfhdr,"char  *type = \"imag\";\n");
      break;
    case PH: /* Phase */
      strcat(d->fdfhdr,"char  *type = \"phase\";\n");
      break;
    case MK: /* Mask */
      strcat(d->fdfhdr,"char  *type = \"mask\";\n");
      break;
    case RMK: /* Reverse mask of magnitude */
      strcat(d->fdfhdr,"char  *type = \"absval\";\n");
      break;
    case SM: /* Sensitivity map */
      strcat(d->fdfhdr,"char  *type = \"smap\";\n");
      break;
    case GF: /* Geometry factor */
      strcat(d->fdfhdr,"char  *type = \"gmap\";\n");
      break;
    case RS: /* Relative SNR */
      strcat(d->fdfhdr,"char  *type = \"rsnrmap\";\n");
      break;
    default:
      break;
  } /* end type switch */
  sprintf(str,"float  matrix[] = {%d, %d};\n",dim2,dim1);
  strcat(d->fdfhdr,str);
  switch(datamode) {
//  switch(d->datamode) {
    case IMAGE: /* Image */
      strcat(d->fdfhdr,"char  *abscissa[] = {\"cm\", \"cm\"};\n");
      break;
    case FID: /* FID */
/*
      strcat(d->fdfhdr,"char  *abscissa[] = {\"s\", \"s\"};\n");
*/
      /* We must define as for image space to get a good display in VnmrJ */
      strcat(d->fdfhdr,"char  *abscissa[] = {\"cm\", \"cm\"};\n");
      break;
    default:
      break;
  } /* end d->datamode switch */
  switch(type) {
    case PH: /* Phase */
      strcat(d->fdfhdr,"char  *ordinate[] = { \"radians\" };\n");
      break;
    case MK: /* Mask */
      strcat(d->fdfhdr,"char  *ordinate[] = { \"mask\" };\n");
      break;
    default:
      strcat(d->fdfhdr,"char  *ordinate[] = { \"intensity\" };\n");
      break;
  } /* end type switch */
  switch(datamode) {
//  switch(d->datamode) {
    case IMAGE: /* Image */
      sprintf(str,"float  span[] = {%.6f, %.6f};\n",*val("lpe",&d->p),*val("lro",&d->p));
      strcat(d->fdfhdr,str);
      sprintf(str,"float  origin[] = {%.6f,%.6f};\n",-ppe,pro);
      strcat(d->fdfhdr,str);
      break;
    case FID: /* FID */
/*
      sprintf(str,"float  span[] = {%.6f,%.6f};\n",dim2/(*val("sw1",&d->p)),dim1/(*val("sw",&d->p)));
      strcat(d->fdfhdr,str);
      sprintf(str,"float  origin[] = {%.6f,%.6f};\n",0.0,0.0);
      strcat(d->fdfhdr,str);
*/
      /* We must define as for image space to get a good display in VnmrJ */
      sprintf(str,"float  span[] = {%.6f, %.6f};\n",*val("lpe",&d->p),*val("lro",&d->p));
      strcat(d->fdfhdr,str);
      sprintf(str,"float  origin[] = {%.6f,%.6f};\n",-ppe,pro);
      strcat(d->fdfhdr,str);
      break;
    default:
      break;
  } /* end d->datamode switch */
  sprintf(str,"char  *nucleus[] = {\"%s\",\"%s\"};\n",*sval("tn",&d->p),*sval("dn",&d->p));
  strcat(d->fdfhdr,str);
  sprintf(str,"float  nucfreq[] = {%.6f,%.6f};\n",*val("sfrq",&d->p),*val("dfrq",&d->p));
  strcat(d->fdfhdr,str);
//  switch(d->datamode) {
  switch(datamode) {
    case IMAGE: /* Image */
      sprintf(str,"float  location[] = {%.6f,%.6f,%.6f};\n",-ppe,pro,sliceposition(d,slice));
      strcat(d->fdfhdr,str);
      sprintf(str,"float  roi[] = {%.6f,%.6f,%.6f};\n",*val("lpe",&d->p),*val("lro",&d->p),*val("thk",&d->p)/10.0);
      strcat(d->fdfhdr,str);
      break;
    case FID: /* FID */
      sprintf(str,"float  location[] = {%.6f,%.6f,%.6f};\n",-ppe,pro,sliceposition(d,slice));
      strcat(d->fdfhdr,str);
      sprintf(str,"float  roi[] = {%.6f,%.6f,%.6f};\n",dim2/(*val("sw1",&d->p)),dim1/(*val("sw",&d->p)),*val("thk",&d->p)/10.0);
      strcat(d->fdfhdr,str);
      break;
    default:
      break;
  } /* end d->datamode switch */
  sprintf(str,"float  gap = %.6f;\n",*val("gap",&d->p));
  strcat(d->fdfhdr,str);
  sprintf(str,"char  *file = \"%s\";\n",d->file);
  strcat(d->fdfhdr,str);
  sprintf(str,"int    slice_no = %d;\n",slice+1);
  strcat(d->fdfhdr,str);
  sprintf(str,"int    slices = %d;\n",ns);
  strcat(d->fdfhdr,str);
  sprintf(str,"int    echo_no = %d;\n",echo+1);
  strcat(d->fdfhdr,str);
  sprintf(str,"int    echoes = %d;\n",ne);
  strcat(d->fdfhdr,str);

  if (ne < 2) {
    if (cyclefaster("pss","te",&d->a)) value=getelem(d,"te",image*ns);
    else value=getelem(d,"te",image);
  } else { /* a multi echo expt */
    /* The TE array should hold the echo time of each echo */
    if (nvals("TE",&d->p) == *val("ne",&d->p)) {
      dblval=val("TE",&d->p);
      value=dblval[echo]/1000.0;
    } else {
      value=1.0; /* Just set a silly value */
    }
  }
  sprintf(str,"float  TE = %.3f;\n",1000.0*value);
  strcat(d->fdfhdr,str);
  sprintf(str,"float  te = %.6f;\n",value);
  strcat(d->fdfhdr,str);

  if (cyclefaster("pss","tr",&d->a)) value=getelem(d,"tr",image*ns);
  else value=getelem(d,"tr",image);
  sprintf(str,"float  TR = %.3f;\n",1000.0*value);
  strcat(d->fdfhdr,str);
  sprintf(str,"float  tr = %.6f;\n",value);
  strcat(d->fdfhdr,str);

  /* Check nread and nphase which are used in EPI */
/*
  value=*val("nread",&d->p);
  if (value>0) sprintf(str,"int    ro_size = %d;\n",(int)value/2);
  else sprintf(str,"int    ro_size = %d;\n",(int)*val("np",&d->p)/2);
  strcat(d->fdfhdr,str);
  value=*val("nphase",&d->p);
  if (value>0) sprintf(str,"int    pe_size = %d;\n",(int)value);
  else sprintf(str,"int    pe_size = %d;\n",(int)*val("nv",&d->p));
  strcat(d->fdfhdr,str);
*/
  sprintf(str,"int    ro_size = %d;\n",dim1);
  strcat(d->fdfhdr,str);
  sprintf(str,"int    pe_size = %d;\n",dim2);
  strcat(d->fdfhdr,str);

  sprintf(str,"char  *sequence = \"%s\";\n",*sval("seqfil",&d->p));
  strcat(d->fdfhdr,str);
  sprintf(str,"char  *studyid = \"%s\";\n",*sval("studyid_",&d->p));
  strcat(d->fdfhdr,str);  
  sprintf(str,"char  *position1 = \"%s\";\n","");
  strcat(d->fdfhdr,str);
  sprintf(str,"char  *position2 = \"%s\";\n","");
  strcat(d->fdfhdr,str);

  if (cyclefaster("pss","ti",&d->a)) value=getelem(d,"ti",image*ns);
  else value=getelem(d,"ti",image);
  sprintf(str,"float  TI = %.3f;\n",1000.0*value);
  strcat(d->fdfhdr,str);
  sprintf(str,"float  ti = %.6f;\n",value);
  strcat(d->fdfhdr,str);

  sprintf(str,"int    array_index = %d;\n",image+1-IMAGEOFFSET);
  strcat(d->fdfhdr,str);

  /* The array_dim is the number of *image???*.fdf = (for 2D) # volumes divided by # echoes */
  value=(double)d->nvols/d->ne;
  /* But if there are reference volumes (e.g. EPI) they must be accounted for */
  /* Check for image parameter array, since that is used to signify reference scans */
  if (arraycheck("image",&d->a)) {
    /* count # image=1 values */
    for (i=0;i<nvals("image",&d->p);i++) if (getelem(d,"image",i)<1) value--;
  }
  sprintf(str,"float  array_dim = %.4f;\n",value);
  strcat(d->fdfhdr,str);

  sprintf(str,"float  image = 1.0;\n");
  strcat(d->fdfhdr,str);
  id = (image-IMAGEOFFSET)*ns*ne + slice*ne + echo;
  sprintf(str,"int    display_order = %d;\n",id);
  strcat(d->fdfhdr,str);

  /* The following assumes that fid data is always stored bigendian ..
     .. if we must reverse byte order to interpret then CPU is lilendian */
  if (reverse_byte_order) {
    sprintf(str,"int    bigendian = 0;\n");
    strcat(d->fdfhdr,str);
  }

  /* Image scaling */
  value=*val("aipScale",&d->p);
  if (FP_EQ(value,0.0)) value=1.0;
  sprintf(str,"float  imagescale = %.9f;\n",value);
  strcat(d->fdfhdr,str);

  sprintf(str,"float  psi = %.4f;\n",psi);
  strcat(d->fdfhdr,str);
  sprintf(str,"float  phi = %.4f;\n",phi);
  strcat(d->fdfhdr,str);
  sprintf(str,"float  theta = %.4f;\n",theta);
  strcat(d->fdfhdr,str);

  /* Generate direction cosine matrix from "Euler" angles just as recon_all */
  cospsi=cos(DEG2RAD*psi);
  sinpsi=sin(DEG2RAD*psi);
  cosphi=cos(DEG2RAD*phi);
  sinphi=sin(DEG2RAD*phi);
  costheta=cos(DEG2RAD*theta);
  sintheta=sin(DEG2RAD*theta);

  or0=-1*cosphi*cospsi - sinphi*costheta*sinpsi;
  or1=-1*cosphi*sinpsi + sinphi*costheta*cospsi;
  or2=-1*sinphi*sintheta;
  or3=-1*sinphi*cospsi + cosphi*costheta*sinpsi;
  or4=-1*sinphi*sinpsi - cosphi*costheta*cospsi;
  or5=cosphi*sintheta;
  or6=-1*sintheta*sinpsi;
  or7=sintheta*cospsi;
  or8=costheta;

  sprintf(str,"float  orientation[] = {%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f};\n",
    or0,or1,or2,or3,or4,or5,or6,or7,or8);
  strcat(d->fdfhdr,str);

  /* Add arrayed parameters */
  if (d->a.npars>0) {
    for (i=0;i<*d->a.nvals;i++) {
      if (addpar2hdr(&d->a,i)) { /* If not already included by default */
        cycle=(int)d->a.d[0][i];
        n=nvals(d->a.s[0][i],&d->p);
        if (cyclefaster("pss",d->a.s[0][i],&d->a)) id=(image*ns/cycle)%n;
        else id=(image/cycle)%n;
        switch (ptype(d->a.s[0][i],&d->p)) {
          case 0:
            strcat(d->fdfhdr,"int    ");
            intval=ival(d->a.s[0][i],&d->p);
            sprintf(str,"%s = %d;\n",d->a.s[0][i],intval[id]);
            strcat(d->fdfhdr,str);
            break;
          case 1:
            strcat(d->fdfhdr,"float  ");
            dblval=val(d->a.s[0][i],&d->p);
            sprintf(str,"%s = %.6f;\n",d->a.s[0][i],dblval[id]);
            strcat(d->fdfhdr,str);
            break;
          case 2:
            strcat(d->fdfhdr,"char  *");
            strval=sval(d->a.s[0][i],&d->p);
            sprintf(str,"%s = \"%s\";\n",d->a.s[0][i],strval[id]);
            strcat(d->fdfhdr,str);
            break;
        }
      }
    }
  }

  /* Add sviblist parameters */
  if (d->s.npars>0) {
    for (i=0;i<*d->s.nvals;i++) {
      if (addpar2hdr(&d->s,i)) { /* If not already included by default */
        add = 1;
        if (d->a.npars>0) { /* Don't include arrayed parameters */
          for (j=0;j<*d->a.nvals;j++)
            if (!strcmp(d->a.s[0][j],d->s.s[0][i])) add--;
        }
        if (add) {
          /* NB The parameter may be arrayed with setprotect('par','on',256) */
          n=nvals(d->s.s[0][i],&d->p);
          if (n>0) {
            id=image%n; /* Assume 'cycle' is 1 - no mechanism exists to suggest otherwise */
            switch (ptype(d->s.s[0][i],&d->p)) {
              case 0: /* Integer */
                strcat(d->fdfhdr,"int    ");
                intval=ival(d->s.s[0][i],&d->p);
                sprintf(str,"%s = %d;\n",d->s.s[0][i],intval[id]);
                strcat(d->fdfhdr,str);
                break;
              case 1: /* Real */
                strcat(d->fdfhdr,"float  ");
                dblval=val(d->s.s[0][i],&d->p);
                sprintf(str,"%s = %.6f;\n",d->s.s[0][i],dblval[id]);
                strcat(d->fdfhdr,str);
                break;
              case 2: /* String */
                strcat(d->fdfhdr,"char  *");
                strval=sval(d->s.s[0][i],&d->p);
                sprintf(str,"%s = \"%s\";\n",d->s.s[0][i],strval[id]);
                strcat(d->fdfhdr,str);
                break;
            }
          }
        }
      }
    }
  }

  strcat(d->fdfhdr,"int checksum = 1291708713;\n");
  strcat(d->fdfhdr,"\f\n");

  /* Add padding */
  switch(precision) {
    case FLT32: /* 32 bit float */
      align = sizeof(float);
      break;
    case DBL64: /* 64 bit double */
      align = sizeof(double);
      break;
    default:
      break;
  } /* end precision switch */
  hdrlen=strlen(d->fdfhdr);
  hdrlen++; /* allow for NULL terminator */
  pad_cnt=hdrlen%align;
  pad_cnt=(align-pad_cnt)%align;
  for(i=0;i<pad_cnt;i++) strcat(d->fdfhdr,"\n");

}

int validtype(int type)
{
  switch (type) {
    case RE:  return(1); break;
    case IM:  return(1); break;
    case MG:  return(1); break;
    case PH:  return(1); break;
    case MK:  return(1); break;
    case RMK: return(1); break;
    case SM:  return(1); break;
    case GF:  return(1); break;
    case RS:  return(1); break;
    case VJ:  return(1); break;
    default:  return(0); break;
  }
}

int addpar2hdr(struct pars *p,int ix)
{
  /* Return '1' if parameter is not one of the following included by default */
  if ((strcmp(p->s[0][ix],"pss")) && (strcmp(p->s[0][ix],"nv"))
    && (strcmp(p->s[0][ix],"nv2")) && (strcmp(p->s[0][ix],"nv3"))
    && (strcmp(p->s[0][ix],"pro")) && (strcmp(p->s[0][ix],"ppe"))
    && (strcmp(p->s[0][ix],"psi")) && (strcmp(p->s[0][ix],"phi"))
    && (strcmp(p->s[0][ix],"theta")) && (strcmp(p->s[0][ix],"te"))
    && (strcmp(p->s[0][ix],"tr")) && (strcmp(p->s[0][ix],"ti"))
    && (strcmp(p->s[0][ix],"TE")) && (strcmp(p->s[0][ix],"TR"))
    && (strcmp(p->s[0][ix],"TI")) && (strcmp(p->s[0][ix],"image"))) {
    return(1);
  }
  return(0);
}

double sliceposition(struct data *d,int slice)
{
  /* Set slice according to its correct index */
  slice=sliceindex(d,slice);
  /* Return the position */
  return(d->p.d[parindex("pss",&d->p)][slice]);
}
