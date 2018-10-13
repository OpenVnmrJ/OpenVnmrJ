/* niftiwrite.c */
/*---------------------------------------------------------------------------*/
/*                                                                           */
/* niftiwrite.c: NIFTI-1/Analyze7.5 writing routines                         */
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
#include "nifti1.h"

#define VNMR2NIFTI_VERSION "Vnmr2NIfTI v1.1"

static int VOLUMEOFFSET=0;

void wnifti(struct data *d,int type,int precision,int volindex)
{

  /* Check VnmrJ parameters for an output format */
  if (spar(d,"nifti","nifti")) writenifti(d,type,precision,NIFTI,volindex);
  else if (spar(d,"nifti","analyze")) writenifti(d,type,precision,ANALYZE,volindex);
  else return;

}

void writenifti(struct data *d,int type,int precision,int format,int volindex)
{
  int output;
  char outdir[MAXPATHLEN];
int datamode;

#ifdef DEBUG
  struct timeval tp;
  double t1,t2;
  int rtn;
#endif

  /* Call wnifti with a negative volindex to flag a skipped reference volume */
  /* e.g. EPI reference scans */
  if (volindex < 0) {
    VOLUMEOFFSET++;
    return;
  }

  /* This function checks the output requested and sets the output directory
     accordingly. Type 'VJ' flags that we should inspect the values of VnmrJ
     parameters in order to figure the requested output using functions
     magnifti() and othernifti(). */

  /* Check that type is valid */
  if (!validtype(type)) {
    fprintf(stderr,"\n%s: %s()\n",__FILE__,__FUNCTION__);
    fprintf(stderr,"  Invalid 2nd argument %s(*,'type',*,*)\n",__FUNCTION__);
    fflush(stderr);
    return;
  }

#ifdef DEBUG
  fprintf(stdout,"\n%s: %s()\n",__FILE__,__FUNCTION__);
  switch (format) {
    case NIFTI: fprintf(stdout,"  Writing NIFTI-1 data"); break;
    case ANALYZE: fprintf(stdout,"  Writing Analyze7.5 data"); break;
  }
  rtn=gettimeofday(&tp, NULL);
  t1=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
  fflush(stdout);
#endif

  datamode=FID;
  /* If FT has been done flag it as IMAGE */
  if ((d->dimstatus[0] & FFT) || (d->dimstatus[1] & FFT) || (d->dimstatus[2] & FFT)) datamode=IMAGE;

  /* Generate output according to requested type */
  switch (type) {
    case VJ: /* Generate output according to VnmrJ parameters */
      switch(datamode) {
        case FID:
          /* When there is data from more than one receiver, magnitude output
             can be from each individual receiver or combined, depending
             on the user selection */
          magnifti(d,"rniMG","rniIR","rawMG",MG,precision,format,volindex);
          /* Otherwise output is always from each individual receiver */
          othernifti(d,"rniPH","rawPH",PH,precision,format,volindex);
          othernifti(d,"rniRE","rawRE",RE,precision,format,volindex);
          othernifti(d,"rniIM","rawIM",IM,precision,format,volindex);
          break;
        default:
          /* When there is data from more than one receiver, magnitude output
             can be from each individual receiver or combined, depending
             on the user selection */
          /* We use output to flag output to directory recon */
          output=magnifti(d,"niiMG","niiIR","MG",MG,precision,format,volindex);
          /* Otherwise output is always from each individual receiver */
          othernifti(d,"niiPH","PH",PH,precision,format,volindex);
          othernifti(d,"niiRE","RE",RE,precision,format,volindex);
          othernifti(d,"niiIM","IM",IM,precision,format,volindex);
          /* Always generate output to directory recon */
          if (!output) {
            if (d->nr>1) /* Multiple receivers */
              gennifti(d,'c',"MG",MG,precision,format,volindex);
            else /* Single receiver */
              gennifti(d,'s',"MG",MG,precision,format,volindex);
          }
          break;
      } /* end datamode switch */
      break;
    default: /* Output not generated according to VnmrJ parameters */
      /* Set up output directory (outdir) according to type */
      switch(datamode) {
        case FID: strcpy(outdir,"raw"); break;
        default: strcpy(outdir,""); break;
      } /* end datamode switch */
      switch(type) {
        case MG:  if (datamode == FID) strcat(outdir,"MG"); break; /* Magnitude */
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
            gennifti(d,'c',outdir,type,precision,format,volindex);
            gennifti(d,'i',outdir,type,precision,format,volindex);
          } else gennifti(d,'s',outdir,type,precision,format,volindex);
          break;
        case MK:  gennifti(d,'s',outdir,type,precision,format,volindex); break;
        case RMK: gennifti(d,'c',outdir,type,precision,format,volindex); break;
        case GF:  gennifti(d,'g',outdir,type,precision,format,volindex); break;
        case RS:  gennifti(d,'r',outdir,type,precision,format,volindex); break;
        default:
          if (d->nr>1) gennifti(d,'i',outdir,type,precision,format,volindex);
          else gennifti(d,'s',outdir,type,precision,format,volindex);
          break;
      } /* end type switch */
      break;
  } /* end type switch */

#ifdef DEBUG
  rtn=gettimeofday(&tp, NULL);
  t2=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
  fprintf(stdout,"  Took %f secs\n",t2-t1);
  fflush(stdout);
#endif

}

int magnifti(struct data *d,char *par,char *parIR,char *outdir,int type,int precision,int format,int volindex)
{
  int output=0;
  if (spar(d,par,"y")) { /* Output selected by par='y' */
    if (d->nr>1) { /* Multiple receivers */
      if (spar(d,parIR,"y")) /* Individual Receiver output */
        gennifti(d,'i',outdir,type,precision,format,volindex);
      else {/* Combined output */
        gennifti(d,'c',outdir,type,precision,format,volindex);
        output=1;
      }
    } else { /* Single receiver */
      gennifti(d,'s',outdir,type,precision,format,volindex);
      output=1;
    }
  }
  return(output);
}

void othernifti(struct data *d,char *par,char *outdir,int type,int precision,int format,int volindex)
{
  if (spar(d,par,"y")) { /* Output selected by par='y' */
    if (d->nr>1) /* Multiple receivers */
      gennifti(d,'i',outdir,type,precision,format,volindex);
    else /* Single receiver */
      gennifti(d,'s',outdir,type,precision,format,volindex);
  }
}

void gennifti(struct data *d,int mode,char *outdir,int type,int precision,int format,int volindex)
{
  char basename[MAXPATHLEN],dirname[MAXPATHLEN],filebase[MAXPATHLEN];
  int ne;
  int volume,niftivols,nifticycle,niftifile;
  int newfile=FALSE;
  int i;
  int image,echo;

  /* This function checks the output type requested and sets the output
     filename accordingly. NIFTI-1/Analyze7.5 data is output according
     to the specified mode using functions wnii() and wcombnii().
     These functions output data either from individual receivers (wnii)
     or using a combination of data from all receivers (wcombnii). */

  /* Check that type is valid */
  if (!validtype(type) || (type == VJ)) {
    fprintf(stderr,"\n%s: %s()\n",__FILE__,__FUNCTION__);
    fprintf(stderr,"  Invalid 4th argument %s(*,*,*,'type',*,*,*)\n",__FUNCTION__);
    fflush(stderr);
    return;
  }

#ifdef DEBUG
  fprintf(stdout,"\n  Block %d (of %d)\n",d->block+1,d->nblocks);
  fflush(stdout);
#endif

  /* Check for VnmrJ recon to figure basename */
  if (vnmrj_recon) strcpy(basename,vnmrj_path);
  else {
    for (i=0;i<=strlen(d->file)-9;i++)
      basename[i]=d->file[i];
    basename[i]=0;
  }

  /* Create the output directory, if it is not already there */
  switch (format) {
    case NIFTI: sprintf(basename,"%s%s",basename,"nifti"); break;
    case ANALYZE: sprintf(basename,"%s%s",basename,"analyze"); break;
    default:
      fprintf(stderr,"\n%s: %s()\n",__FILE__,__FUNCTION__);
      fprintf(stderr,"  Invalid 6th argument %s(*,*,*,*,*,'format',*)\n",__FUNCTION__);
      fflush(stderr);
      return;
      break;
  } /* end format switch */
  createdir(basename);

  /* Number of echoes */
  ne=(int)*val("ne",&d->p);
  if (ne < 1) ne=1; /* Set ne to 1 if 'ne' does not exist */

  /* Allow for compressed multi-echo loop */
  image=(volindex-VOLUMEOFFSET)/ne;
  echo=(volindex-VOLUMEOFFSET)%ne;

  /* Figure the file we need to create / append to */
  niftivols=(int)*val("niftivols",&d->p);     /* The number of volumes per NIFTI file */
  nifticycle=(int)*val("nifticycle",&d->p);   /* The number of volume cycles per NIFTI file */
  volume=(volindex-VOLUMEOFFSET);
  niftifile=(volume/(niftivols*nifticycle))*nifticycle+volume%nifticycle;
  if (((volume/nifticycle)%niftivols == 0) && (d->block == 0)) newfile=TRUE;

  switch(mode) {
    case 'i': /* Individual output */
      for (i=0;i<d->nr;i++) {
        sprintf(dirname,"%s/%s%.3d",basename,outdir,i+1);
        createdir(dirname);
        sprintf(filebase,"%s/image%.3d",dirname,niftifile+1);
        if (newfile) wniftihdr(filebase,d,precision,format,image);
        wniftidata(filebase,d,i,type,precision,format,newfile);
      }
      break;
    case 'c': /* Combined output (Magnitude only) */
      sprintf(dirname,"%s/%s",basename,outdir);
      createdir(dirname);
      sprintf(filebase,"%s/image%.3d",dirname,niftifile+1);
      if (newfile) wniftihdr(filebase,d,precision,format,image);
      wcombniftidata(filebase,d,type,precision,format,newfile);
      break;
    case 's': /* Single receiver */
      sprintf(dirname,"%s/%s",basename,outdir);
      createdir(dirname);
      sprintf(filebase,"%s/image%.3d",dirname,niftifile+1);
      if (newfile) wniftihdr(filebase,d,precision,format,image);
      wniftidata(filebase,d,0,type,precision,format,newfile);
      break;
    case 'g': /* Geometry Factor output */
      sprintf(dirname,"%s/%s",basename,outdir);
      createdir(dirname);
      sprintf(filebase,"%s/image%.3d",dirname,niftifile+1);
      if (newfile) wniftihdr(filebase,d,precision,format,image);
      wniftidata(filebase,d,0,type,precision,format,newfile);
      break;
    case 'r': /* Relative SNR */
      sprintf(dirname,"%s/%s",basename,outdir);
      createdir(dirname);
      sprintf(filebase,"%s/image%.3d",dirname,niftifile+1);
      if (newfile) wniftihdr(filebase,d,precision,format,image);
      wniftidata(filebase,d,1,type,precision,format,newfile);
      break;
    default:
      fprintf(stderr,"\n%s: %s()\n",__FILE__,__FUNCTION__);
      fprintf(stderr,"  Invalid 2nd argument %s(*,'mode',*,*,*,*)\n",__FUNCTION__);
      fflush(stderr);
      break;
  } /* end mode switch */
}

void wniftihdr(char *filebase,struct data *D,int precision,int format,int image)
{
  struct nifti_1_header nfh;
  FILE *fp;
  char filename[MAXPATHLEN];
  int niftivols,timecourse,nii=FALSE;
  double pro,ppe,ppe2=0,psi,phi,theta;
  double cospsi,cosphi,costheta;
  double sinpsi,sinphi,sintheta;
  double d11,d12,d13,d21,d22,d23,d31,d32,d33;
  double R11,R12,R13,R21,R22,R23,R31,R32,R33;
  double a,b,c,d,fb2,fc2,fd2;
  double user_x,user_y,user_z=0;
  float fourbytespace=0;
  int i;
#ifdef DEBUG
  double c1,c2,c3,r1,r2,r3,det;
#endif

  switch(format) {
    case NIFTI:
      nii=spar(D,"niftifile","nii");           /* nii is TRUE if niftifile is "nii" */
      if (nii) sprintf(filename,"%s.nii",filebase);
      else sprintf(filename,"%s.hdr",filebase);
      nfh.sizeof_hdr = 348;                    /* Size of header */
      strcpy(nfh.data_type,"");                /* UNUSED char[10] */
      strcpy(nfh.db_name,"");                  /* UNUSED char[18] */
      nfh.extents = 0;                         /* UNUSED int */
      nfh.session_error = 0;                   /* UNUSED short */
      nfh.regular = 0;                         /* UNUSED char */
      if (im2D(D))
        nfh.dim_info = FPS_INTO_DIM_INFO(1,2,3);
        /* This field encodes the freq_dim, phase_dim and slice_dim respectively.
           We will write regular rectangular data in this order. NIFTI1 uses
           FPS_INTO_DIM_INFO(0,0,3) to indicate spiral/radial scans since the
           concepts of frequency- and phase-encoding directions don't apply.
           We will need a mechanism to flag this. */
      if (im3D(D))
        nfh.dim_info = FPS_INTO_DIM_INFO(0,0,0); /* Its not obvious what to put */
      nfh.dim[0]=3;                            /* We will have at least a single volume */
      nfh.dim[1] = (short)D->np/2;             /* Size of dim1 */
      nfh.dim[2] = (short)D->nv;               /* Size of dim2 */
      if (im2D(D)) nfh.dim[3] = (short)D->ns;  /* Size of dim3 for 2D multislice */
      if (im3D(D)) nfh.dim[3] = (short)D->nv2; /* Size of dim3 for 3D */
      /* Now figure extra dimensions */
      niftivols=(int)*val("niftivols",&D->p);  /* The number of volumes per NIFTI file */
      timecourse=spar(D,"niftitime","y");      /* timecourse is TRUE if its a timecourse */
      if (niftivols>1) {
        nfh.dim[0]++;                          /* We have at least an extra dimension */
        if (timecourse) {                      /* If its a timecourse ... */
          nfh.dim[4] = (short)niftivols;       /* ... the volumes will be in dim[4] ... */
          nfh.dim[5] = 0;                      /* ... and dim[5] doesn't exist */
        } else {                               /* Else its not a timecourse ... */
          nfh.dim[0]++;                        /* ... the extra volumes are in dim[5] */
          nfh.dim[4] = 1;                      /* ... dim[4] set for one timepoint ... */
          nfh.dim[5] = (short)niftivols;       /* ... extra volumes */
        }
      } else {
        nfh.dim[4] = 0;                        /* For 1 volume dim[4] doesn't exist */
        nfh.dim[5] = 0;
      }
      nfh.dim[6] = 0;                          /* There are no extra dimensions */
      nfh.dim[7] = 0;
      nfh.intent_p1 = 0.0; /* The purpose of the intent_* fields is to help interpret the values */
      nfh.intent_p2 = 0.0; /* stored in the dataset. Some non-statistical values for intent_code */
      nfh.intent_p3 = 0.0; /* and conventions are provided for storing other complex data types  */
      nfh.intent_code = 0; /* e.g. intent_code = NIFTI_INTENT_ESTIMATE; intent_name = "T1";      */
                           /* could be used to signify that the voxel values are estimates of    */
                           /* the NMR parameter T1. */
      switch (precision) {
        case INT16:
          nfh.datatype = NIFTI_TYPE_INT16;
          nfh.bitpix = 16;
          break;
        case INT32:
          nfh.datatype = NIFTI_TYPE_INT32;
          nfh.bitpix = 32;
          break;
        case FLT32:
          nfh.datatype = NIFTI_TYPE_FLOAT32;
          nfh.bitpix = 32;
          break;
        case DBL64:
          nfh.datatype = NIFTI_TYPE_FLOAT64;
          nfh.bitpix = 64;
          break;
      }
      nfh.slice_start = 0;                     /* "padded" slices not yet supported */
      nfh.pixdim[0] = 1;                       /* We use a right-handed coordinate system */
      nfh.pixdim[1] = (float)*val("lro",&D->p)*10/nfh.dim[1];  /* Readout pixel dimension in mm */
      nfh.pixdim[2] = (float)*val("lpe",&D->p)*10/nfh.dim[2];  /* Phase encode pixel dimension in mm */
      if (im2D(D)) nfh.pixdim[3] = (float)(*val("thk",&D->p)+*val("gap",&D->p)*10);  /* Slice thickness nad gap in mm */
      if (im3D(D)) nfh.pixdim[3] = (float)*val("lpe2",&D->p)*10/nfh.dim[3];  /* 2nd phase encode pixel dimension in mm */
      if ((niftivols>1) && timecourse) {       /* If its a timecourse ... */
        nfh.pixdim[4] = *val("niftit",&D->p);  /* ... the repetition time in sec */
      } else {
        nfh.pixdim[4] = 0;                     /* No time course */
      }
      nfh.pixdim[5] = 0;
      nfh.pixdim[6] = 0;                       /* There are no extra dimensions */
      nfh.pixdim[7] = 0;
      if (nii) nfh.vox_offset = 352.0;         /* For a '.nii' file the offset to data is 352.0 bytes */
      else nfh.vox_offset = 0.0;               /* Otherwise 0.0; */
      nfh.scl_slope = 0.0;                     /* If != 0.0, true data value = value*fdf_sclslope + fdf_sclinter ... */
      nfh.scl_inter = 0.0;                     /* ... useful for integer formats but we will use float */
      /* Slice end and the order of slice acquisition */
      if (im2D(D)) {
        nfh.slice_end = nfh.dim[3]-1;          /* "padded" slices not accounted for */
        switch ((int)*val("iplanType",&D->p)) {
          case 0: nfh.slice_code = NIFTI_SLICE_SEQ_INC; break;
          case 1: nfh.slice_code = NIFTI_SLICE_ALT_INC; break;
          case 2: nfh.slice_code = NIFTI_SLICE_SEQ_DEC; break;
          case 3: nfh.slice_code = NIFTI_SLICE_ALT_DEC; break;
        }
      } else {
        nfh.slice_end = 0;
        nfh.slice_code = NIFTI_SLICE_UNKNOWN;
      }
#ifdef DEBUG
  fprintf(stdout,"\n%s: %s()\n",__FILE__,__FUNCTION__);
  switch(nfh.slice_code) {
    case 0: fprintf(stdout,"  nfh.slice_code = NIFTI_SLICE_UNKNOWN\n"); break;
    case 1: fprintf(stdout,"  nfh.slice_code = NIFTI_SLICE_SEQ_INC\n"); break;
    case 2: fprintf(stdout,"  nfh.slice_code = NIFTI_SLICE_SEQ_DEC\n"); break;
    case 3: fprintf(stdout,"  nfh.slice_code = NIFTI_SLICE_ALT_INC\n"); break;
    case 4: fprintf(stdout,"  nfh.slice_code = NIFTI_SLICE_ALT_DEC\n"); break;
  }
#endif
      if (timecourse) /* pixdim are in mm,mm,mm,s */
        nfh.xyzt_units = SPACE_TIME_TO_XYZT(NIFTI_UNITS_MM,NIFTI_UNITS_SEC);
      else            /* pixdim are in mm,mm,mm */
        nfh.xyzt_units = SPACE_TIME_TO_XYZT(NIFTI_UNITS_MM,0);
      nfh.cal_max = 0.0;                       /* not supported */
      nfh.cal_min = 0.0;                       /* not supported */
      if (timecourse) {
        nfh.slice_duration = (float)*val("niftitslice",&D->p); /* Slice duration */
        nfh.toffset = (float)*val("niftitoffset",&D->p);       /* Time axis shift */
      } else {
        nfh.slice_duration = 0.0;              /* Slice duration */
        nfh.toffset = 0.0;                     /* Time axis shift */
      }
      nfh.glmax = 0;                           /* UNUSED int */
      nfh.glmin = 0;                           /* UNUSED int */
      strcpy(nfh.descrip,VNMR2NIFTI_VERSION);  /* Conversion & version */
      strcat(nfh.descrip,", ");
      strcat(nfh.descrip,*sval("seqfil",&D->p));
      strcpy(nfh.aux_file,"");                 /* not used */
      nfh.qform_code = NIFTI_XFORM_SCANNER_ANAT;   /* Scanner-based anatomical coordinates */
      /* sform_code>0 is used to set the orientation and location to a standard
         grid e.g. the Talairach coordinate system so we just set to zero */
      nfh.sform_code = 0;
      /* The quaternion representation describes how one would rotate the
         data set (the user's coordinate system) to the magnet coordinate system
         whilst the offsets describe how one would translate the data set to
         the magnet coordinate system, specifically
         [ X ]   [ R11 R12 R13 ] [        pixdim[1] * i ]   [ qoffset_X ]
         [ Y ] = [ R21 R22 R23 ] [        pixdim[2] * j ] + [ qoffset_Y ]
         [ Z ]   [ R31 R32 R33 ] [ qfac * pixdim[3] * k ]   [ qoffset_Z ]
         where: X,Y,Z are coordinates in the right-handed magnet coordinate system,
                i,j,k correspond to the index coordinates of the data set
                      with    i = 0 .. dim[1]-1
                              j = 0 .. dim[2]-1  (if dim[0] >= 2)
                              k = 0 .. dim[3]-1  (if dim[0] >= 3)
                qfac is either -1 (left-handed i,j,k) or 1 (right-handed i,j,k);
                     qfac is stored in the otherwise unused pixdim[0].
                     If pixdim[0]=0.0 (which should not occur) it's assumed qfac=1.
                R11 -> R33 are direction cosines of rotation matrix R
                qoffset_X,qoffset_Y,qoffset_Z = X,Y,Z for the center of the
                     i,j,k = 0,0,0 voxel (the first value in the dataset array) */

      /* Consider neurological RAS orientation (Right=X, Anterior=Y, Superior=Z),
         which is a right-handed magnet coordinate system.
         Looking down the horizontal bore tube from the front of the magnet,
         X points to the left, Y points up and Z points down the bore tube.
         We will output readout=i, phase=j, slice/phase2=k */

      /* Get "Euler" angles */
      psi=getelem(D,"psi",image);
      phi=getelem(D,"phi",image);
      theta=getelem(D,"theta",image);
      /* Generate direction cosine matrix from "Euler" angles just as recon_all does for 2D */
      cospsi=cos(DEG2RAD*psi);
      sinpsi=sin(DEG2RAD*psi);
      cosphi=cos(DEG2RAD*phi);
      sinphi=sin(DEG2RAD*phi);
      costheta=cos(DEG2RAD*theta);
      sintheta=sin(DEG2RAD*theta);
      d11=-1*cosphi*cospsi - sinphi*costheta*sinpsi;
      d12=-1*cosphi*sinpsi + sinphi*costheta*cospsi;
      d13=-1*sinphi*sintheta;
      d21=-1*sinphi*cospsi + cosphi*costheta*sinpsi;
      d22=-1*sinphi*sinpsi - cosphi*costheta*cospsi;
      d23=cosphi*sintheta;
      d31=-1*sintheta*sinpsi;
      d32=sintheta*cospsi;
      d33=costheta;
      /* VnmrJ manual states that    [ X ]   [ d11 d21 d31 ] [ x ]
                                     [ Y ] = [ d12 d22 d32 ] [ y ]
                                     [ Z ]   [ d13 d23 d33 ] [ z ]
         describes how to get to the magnet frame from the user frame using
         orientation[] = {d11,d12,d13,d21,d22,d23,d31,d32,d33};
         This doesn't quite seem correct though since readout and phase encode
         directions are flipped.
         So we swap columns 1 and 2 and then change the sign of column 2 in
         order to maintain a right-handed coordinate system, i.e.
                                     [ X ]   [ d21 -d11 d31 ] [ x ]
                                     [ Y ] = [ d22 -d12 d32 ] [ y ]
                                     [ Z ]   [ d23 -d13 d33 ] [ z ]
         which seems to work OK, at least for
         Axial, Axial90, Coronal, Cornal90, Sagittal and Sagittal90
      */
      R11=d21; R12=-d11; R13=d31;
      R21=d22; R22=-d12; R23=d32;
      R31=d23; R32=-d13; R33=d33;

#ifdef DEBUG
  /* Sanity check for direction cosines
     - they should all be 1 for a proper normalized rotation matrix */
  /* columns */
  c1=sqrt(R11*R11+R21*R21+R31*R31);
  c2=sqrt(R12*R12+R22*R22+R32*R32);
  c3=sqrt(R13*R13+R23*R23+R33*R33);
  fprintf(stdout,"  Rotation matrix, user to magnet frame:\n");
  fprintf(stdout,"  |%6.3f %6.3f %6.3f|\n",R11,R12,R13);
  fprintf(stdout,"  |%6.3f %6.3f %6.3f|\n",R21,R22,R23);
  fprintf(stdout,"  |%6.3f %6.3f %6.3f|\n",R31,R32,R33);
  fprintf(stdout,"  Direction cosines, sum of (matrix elements squared):\n");
  fprintf(stdout,"  (should all be 1 for a proper normalized rotation matrix)\n");
  fprintf(stdout,"    Column: 1 = %f, 2 = %f, 3 = %f\n",c1,c2,c3);
  /* rows */
  r1=sqrt(R11*R11+R12*R12+R13*R13);
  r2=sqrt(R21*R21+R22*R22+R23*R23);
  r3=sqrt(R31*R31+R32*R32+R33*R33);
  fprintf(stdout,"    Row:    1 = %f, 2 = %f, 3 = %f\n",r1,r2,r3);
  /* determinant */
  det= R11*R22*R33-R11*R32*R23-R21*R12*R33+R21*R32*R13+R31*R12*R23-R31*R22*R13;
  fprintf(stdout,"  Direction cosines, determinant = %f\n",det);
  fprintf(stdout,"  (should be 1 for a right handed frame)\n");
#endif

      /* Compute quaternion parameters */
      a = R11+R22+R33+1.0;
      if(a > 0.5) { /* its the simplest case */
        a = 0.5*sqrt(a);
        b = 0.25*(R32-R23)/a;
        c = 0.25*(R13-R31)/a;
        d = 0.25*(R21-R12)/a;
      } else { /* its somewhat trickier */
        fb2 = 1.0+R11-(R22+R33);  /* 4*b*b */
        fc2 = 1.0+R22-(R11+R33);  /* 4*c*c */
        fd2 = 1.0+R33-(R11+R22);  /* 4*d*d */
        if (fb2 > 1.0) {
          b = 0.5*sqrt(fb2);
          c = 0.25*(R12+R21)/b;
          d = 0.25*(R13+R31)/b;
          a = 0.25*(R32-R23)/b;
        } else if (fc2 > 1.0) {
          c = 0.5l * sqrt(fc2);
          b = 0.25*(R12+R21)/c;
          d = 0.25*(R23+R32)/c;
          a = 0.25*(R13-R31)/c;
        } else {
          d = 0.5*sqrt(fd2);
          b = 0.25*(R13+R31)/d;
          c = 0.25*(R23+R32)/d;
          a = 0.25*(R21-R12)/d;
        }
        if (a < 0.0) { b=-b; c=-c; d=-d; a=-a; }
      }
      nfh.quatern_b = b; nfh.quatern_c = c; nfh.quatern_d = d;

      /* Now for the offsets */
      pro=getelem(D,"pro",image);
      ppe=getelem(D,"ppe",image);
      if (im3D(D)) ppe=getelem(D,"ppe",image);
      /* Determine the position of the first data point in the user frame */
      /* Assume data has been acquired in a positive readout gradient */
      user_x = (float)10*(pro + *val("lro",&D->p)/2.0) - nfh.pixdim[1]/2;
      /* Assume phase encoding gradient starts positive and ends up negative */
      user_y = (float)10*(ppe - *val("lpe",&D->p)/2.0);
      /* For 2D we just need centre of the first slice which we have ordered to be the most negative */
      if (im2D(D)) user_z = (float)10*sliceposition(D,0);
      /* Assume 2nd phase encoding gradient starts positive and ends up negative */
      if (im3D(D)) user_z = (float)10*(ppe2 - *val("lpe2",&D->p)/2.0);
      /* Rotate the first data point into the magnet coordinate system to get the offsets */
      nfh.qoffset_x = R11*user_x + R12*user_y + R13*user_z;
      nfh.qoffset_y = R21*user_x + R22*user_y + R23*user_z;
      nfh.qoffset_z = R31*user_x + R32*user_y + R33*user_z;
#ifdef DEBUG
  fprintf(stdout,"  qoffset_x = %f, qoffset_y = %f, qoffset_z = %f\n",nfh.qoffset_x,nfh.qoffset_y,nfh.qoffset_z);
#endif
      for (i=0; i<4; i++)
        nfh.srow_x[i] = 0.0;
      for (i=0; i<4; i++)
        nfh.srow_y[i] = 0.0;
      for (i=0; i<4; i++)
        nfh.srow_z[i] = 0.0;
      strcpy(nfh.intent_name,""); /* not supported */
      if (nii) strcpy(nfh.magic,"n+1");
      else  strcpy(nfh.magic,"ni1");

      break;
    case ANALYZE:
      sprintf(filename,"%s.hdr",filebase);
      nfh.sizeof_hdr = 348;                    /* Size of header */
      strcpy(nfh.data_type,"");                /* Is this ever used ? */
      strcpy(nfh.db_name,"");                  /* Is this ever used ? */
      nfh.extents = 16384;                     /* Should be 16384 */
      nfh.session_error = 0;                   /* Presumably 0 for no session error */
      nfh.regular = 'r';                       /* Must be character 'r' to indicate that all
                                                  images and volumes are the same */
      nfh.dim_info = 0;                        /* Presumably char hkey_un0 should be 0 */
      nfh.dim[0] = 3;                          /* We will have at least a single volume */
      nfh.dim[1] = (short)D->np/2;             /* Size of dim1 */
      nfh.dim[2] = (short)D->nv;               /* Size of dim2 */
      if (im2D(D)) nfh.dim[3] = (short)D->ns;  /* Size of dim3 for 2D multislice */
      if (im3D(D)) nfh.dim[3] = (short)D->nv2; /* Size of dim3 for 3D */
      /* Now figure extra dimensions */
      niftivols=(int)*val("niftivols",&D->p);  /* The number of volumes per NIFTI file */
      if (niftivols>1) {
        nfh.dim[0]++;                          /* We have an extra dimension */
        nfh.dim[4]=(short)niftivols;           /* The additional volumes will be in dim[4] */
      } else {
        nfh.dim[4] = 0;                        /* For 1 volume dim[4] doesn't exist */
      }
      nfh.dim[5] = 0;                          /* There are no extra dimensions */
      nfh.dim[6] = 0;
      nfh.dim[7] = 0;
      nfh.intent_p1 = 0.0;                     /* UNUSED 2*short */
      nfh.intent_p2 = 0.0;                     /* UNUSED 2*short */
      nfh.intent_p3 = 0.0;                     /* UNUSED 2*short */
      nfh.intent_code = 0;                     /* UNUSED short */
      switch (precision) {
        case INT16:
          nfh.datatype = NIFTI_TYPE_INT16;
          nfh.bitpix = 16;
          break;
        case INT32:
          nfh.datatype = NIFTI_TYPE_INT32;
          nfh.bitpix = 32;
          break;
        case FLT32:
          nfh.datatype = NIFTI_TYPE_FLOAT32;
          nfh.bitpix = 32;
          break;
        case DBL64:
          nfh.datatype = NIFTI_TYPE_FLOAT64;
          nfh.bitpix = 64;
          break;
      }
      nfh.slice_start = 0;                     /* UNUSED short */
      nfh.pixdim[0] = 0;                       /* Not used by ANALYZE */
      nfh.pixdim[1] = (float)*val("lro",&D->p)*10/nfh.dim[1];  /* Readout pixel dimension in mm */
      nfh.pixdim[2] = (float)*val("lpe",&D->p)*10/nfh.dim[2];  /* Phase encode pixel dimension in mm */
      if (im2D(D)) nfh.pixdim[3] = (float)(*val("thk",&D->p)+*val("gap",&D->p)*10); /* Slice thickness nad gap in mm */
      if (im3D(D)) nfh.pixdim[3] = (float)*val("lpe2",&D->p)*10/nfh.dim[3];  /* 2nd phase encode pixel dimension in mm */
      nfh.pixdim[4] = 0;                       /* No time course */
      nfh.pixdim[5] = 0;
      nfh.pixdim[6] = 0;                       /* There are no extra dimensions */
      nfh.pixdim[7] = 0;
      nfh.vox_offset = 0.0;                    /* Data starts at beginning of '.img' file */
      nfh.scl_slope = 0.0;                     /* UNUSED float */
      nfh.scl_inter = 0.0;                     /* UNUSED float */
      nfh.slice_end = 0;                       /* UNUSED 0.5*float */
      nfh.slice_code = 0;                      /* UNUSED 0.25*float */
      nfh.xyzt_units = 0;                      /* UNUSED 0.25*float */
      nfh.cal_max = 0.0;                       /* not supported */
      nfh.cal_min = 0.0;                       /* not supported */
      nfh.slice_duration = 0.0;                /* compressed set to 0.0 */
      nfh.toffset = 0.0;                       /* verified set to 0.0 */
      nfh.glmax = 0;                           /* may need to set a max ? */
      nfh.glmin = 0;                           /*  */
      strcpy(nfh.descrip,VNMR2NIFTI_VERSION);  /* Conversion & version */
      strcat(nfh.descrip,", ");
      strcat(nfh.descrip,*sval("seqfil",&D->p));
      strcpy(nfh.aux_file,"");                 /* not used */
      nfh.qform_code = 0;  /* For Analyze this short = 2*char
                              The 1st char is for the orientation and is
                              often not set correctly (or plain ignored).
                              For the time being just set to zero.
                              The 2nd char is part of another field */
      /* For all successive history substruct fields we just set the replacement
         NIFTI fields to 0, at least until we find a good reason not to */
      nfh.sform_code = 0;
      nfh.quatern_b = 0.0;
      nfh.quatern_c = 0.0;
      nfh.quatern_d = 0.0;
      nfh.qoffset_x = 0.0;
      nfh.qoffset_y = 0.0;
      nfh.qoffset_z = 0.0;
      for (i=0; i<4; i++)
        nfh.srow_x[i] = 0.0;
      for (i=0; i<4; i++)
        nfh.srow_y[i] = 0.0;
      for (i=0; i<4; i++)
        nfh.srow_z[i] = 0.0;
      strcpy(nfh.intent_name,"");
      strcpy(nfh.magic,"");

      break;
  }

  /* Write header */
  if ((fp = fopen(filename, "w")) == NULL) {
    fprintf(stderr,"\n%s: %s()\n",__FILE__,__FUNCTION__);
    fprintf(stderr,"  Unable to write to %s\n",filename);
    fflush(stderr);
    return;
  }
  fwrite(&nfh,1,sizeof(nifti_1_header),fp); /* write the 348 bytes */
  if (nii) fwrite(&fourbytespace,1,sizeof(float),fp);
  fclose(fp);

}

void wniftidata(char *filebase,struct data *d,int receiver,int type,int precision,int format,int newfile)
{
  FILE *fp;
  char filename[MAXPATHLEN];
  float *floatdata;
  double *doubledata;
  double re,im,M;
  int dim1,dim2,dim3;
  int nii=FALSE;
  int i,j;

  /* Data dimensions */
  dim1=d->np/2; dim2=d->nv;
  dim3=1;
  if (im2D(d)) dim3 = d->ns;  /* Size of dim3 for 2D multislice */
  if (im3D(d)) dim3 = d->nv2; /* Size of dim3 for 3D */

  switch(format) {
    case NIFTI:
      nii=spar(d,"niftifile","nii");           /* nii is TRUE if niftifile is "nii" */
      if (nii) sprintf(filename,"%s.nii",filebase);
      else sprintf(filename,"%s.img",filebase);
      break;
    case ANALYZE:
      sprintf(filename,"%s.img",filebase);
      break;
  }
  if (newfile && nii) newfile=FALSE;
  if (newfile) {
    if ((fp = fopen(filename, "w")) == NULL) {
      fprintf(stderr,"\n%s: %s()\n",__FILE__,__FUNCTION__);
      fprintf(stderr,"  Unable to write to %s\n",filename);
      fflush(stderr);
      return;
    }
  } else { /* Open file for appending */
    if ((fp = fopen(filename, "a")) == NULL) {
      fprintf(stderr,"\n%s: %s()\n",__FILE__,__FUNCTION__);
      fprintf(stderr,"  Unable to append to %s\n",filename);
      fflush(stderr);
      return;
    }
  }

  /* For Sensitivity Map, Geometry Factor and Relative SNR we just output
     the magnitude. */
  switch(type) {
    case SM: type = MG; break; /* Sensitivity Map */
    case GF: type = MG; break; /* Geometry Factor */
    case RS: type = MG; break; /* Relative SNR */
  }

  switch(precision) {
    case FLT32: /* 32 bit float */
      if ((floatdata = (float *)malloc(dim2*dim1*sizeof(float))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
      switch(type) {
        case MG: /* Magnitude */
          for(i=0;i<dim3;i++) {
            for(j=0;j<dim2*dim1;j++) {
              re=d->data[receiver][i][j][0];
              im=d->data[receiver][i][j][1];
              M=sqrt(re*re+im*im);
              floatdata[j] = (float)M;
            }
            fwrite(floatdata,sizeof(float),dim1*dim2,fp);
          }
          break;
        case PH: /* Phase */
          for(i=0;i<dim3;i++) {
            for(j=0;j<dim2*dim1;j++) {
              re=d->data[receiver][i][j][0];
              im=d->data[receiver][i][j][1];
              /* atan2 returns values (in radians) between +PI and -PI */
              M=atan2(im,re);
              floatdata[j] = (float)M;
            }
            fwrite(floatdata,sizeof(float),dim1*dim2,fp);
          }
          break;
        case RE: /* Real */
          for(i=0;i<dim3;i++) {
            for(j=0;j<dim2*dim1;j++) {
              floatdata[j] = (float)d->data[receiver][i][j][0];
            }
            fwrite(floatdata,sizeof(float),dim1*dim2,fp);
          }
          break;
        case IM: /* Imaginary */
          for(i=0;i<dim3;i++) {
            for(j=0;j<dim2*dim1;j++) {
              floatdata[j] = (float)d->data[receiver][i][j][1];
            }
            fwrite(floatdata,sizeof(float),dim1*dim2,fp);
          }
          break;

        case MK: /* Mask */
          for(i=0;i<dim3;i++) {
            for(j=0;j<dim2*dim1;j++) {
              floatdata[j] = (float)d->mask[i][j];
            }
            fwrite(floatdata,sizeof(float),dim1*dim2,fp);
          }
          break;
        case RMK: /* Reverse Mask of Magnitude */
          for(i=0;i<dim3;i++) {
            for(j=0;j<dim2*dim1;j++) {
              if (!d->mask[i][j]) {
                re=d->data[receiver][i][j][0];
                im=d->data[receiver][i][j][1];
                M=sqrt(re*re+im*im);
                floatdata[j] = (float)M;
              } else {
                floatdata[j] = 0.0;
              }
            }
            fwrite(floatdata,sizeof(float),dim1*dim2,fp);
          }
          break;
        default:
          break;
      } /* end type switch */
      free(floatdata);
      break;
    case DBL64: /* 64 bit double */
      if ((doubledata = (double *)malloc(dim2*dim1*sizeof(double))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
      switch(type) {
        case MG: /* Magnitude */
          for(i=0;i<dim3;i++) {
            for(j=0;j<dim2*dim1;j++) {
              re=d->data[receiver][i][j][0];
              im=d->data[receiver][i][j][1];
              M=sqrt(re*re+im*im);
              doubledata[j] = M;
            }
            fwrite(doubledata,sizeof(double),dim1*dim2,fp);
          }
          break;
        case PH: /* Phase */
          for(i=0;i<dim3;i++) {
            for(j=0;j<dim2*dim1;j++) {
              re=d->data[receiver][i][j][0];
              im=d->data[receiver][i][j][1];
              /* atan2 returns values (in radians) between +PI and -PI */
              M=atan2(im,re);
              doubledata[j] = M;
            }
            fwrite(doubledata,sizeof(double),dim1*dim2,fp);
          }
          break;
        case RE: /* Real */
          for(i=0;i<dim3;i++) {
            for(j=0;j<dim2*dim1;j++) {
              doubledata[j] = d->data[receiver][i][j][0];
            }
            fwrite(doubledata,sizeof(double),dim1*dim2,fp);
          }
          break;
        case IM: /* Imaginary */
          for(i=0;i<dim3;i++) {
            for(j=0;j<dim2*dim1;j++) {
              doubledata[j] = d->data[receiver][i][j][1];
            }
            fwrite(doubledata,sizeof(double),dim1*dim2,fp);
          }
          break;
        case MK: /* Mask */  dim3=1;

          for(i=0;i<dim3;i++) {
            for(j=0;j<dim2*dim1;j++) {
              doubledata[j] = (double)d->mask[i][j];
            }
            fwrite(doubledata,sizeof(double),dim1*dim2,fp);
          }
          break;
        case RMK: /* Reverse Mask of Magnitude */
          for(i=0;i<dim3;i++) {
            for(j=0;j<dim2*dim1;j++) {
              if (!d->mask[i][j]) {
                re=d->data[receiver][i][j][0];
                im=d->data[receiver][i][j][1];
                M=sqrt(re*re+im*im);
                doubledata[j] = M;
              } else {
                doubledata[j] = 0.0;
              }
            }
            fwrite(doubledata,sizeof(double),dim1*dim2,fp);
          }
          break;
        default:
          break;
      } /* end type switch */
      free(doubledata);
      break;
  } /* end precision switch */

  /* Close file */
  fclose(fp);

}

void wcombniftidata(char *filebase,struct data *d,int type,int precision,int format,int newfile)
{
  FILE *fp;
  char filename[MAXPATHLEN];
  float *floatdata;
  double *doubledata;
  double re,im,M;
  int dim1,dim2,dim3,nr;
  int nii=FALSE;
  int i,j,k;

  /* Data dimensions */
  dim1=d->np/2; dim2=d->nv; nr=d->nr;
  dim3=1;
  if (im2D(d)) dim3 = d->ns;  /* Size of dim3 for 2D multislice */
  if (im3D(d)) dim3 = d->nv2; /* Size of dim3 for 3D */

  switch(format) {
    case NIFTI:
      nii=spar(d,"niftifile","nii");           /* nii is TRUE if niftifile is "nii" */
      if (nii) sprintf(filename,"%s.nii",filebase);
      else sprintf(filename,"%s.img",filebase);
      break;
    case ANALYZE:
      sprintf(filename,"%s.img",filebase);
      break;
  }
  if (newfile && nii) newfile=FALSE;
  if (newfile) {
    if ((fp = fopen(filename, "w")) == NULL) {
      fprintf(stderr,"\n%s: %s()\n",__FILE__,__FUNCTION__);
      fprintf(stderr,"  Unable to write to %s\n",filename);
      fflush(stderr);
      return;
    }
  } else { /* Open file for appending */
    if ((fp = fopen(filename, "a")) == NULL) {
      fprintf(stderr,"\n%s: %s()\n",__FILE__,__FUNCTION__);
      fprintf(stderr,"  Unable to append to %s\n",filename);
      fflush(stderr);
      return;
    }
  }

  switch(precision) {
    case FLT32: /* 32 bit float */
      if ((floatdata = (float *)malloc(dim2*dim1*sizeof(float))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
      switch(type) {
        case MG: /* Magnitude */
          for(i=0;i<dim3;i++) {
            for(j=0;j<dim2*dim1;j++) {
              M=0.0;
              for (k=0;k<nr;k++) {
                re=d->data[k][i][j][0];
                im=d->data[k][i][j][1];
                M+=re*re+im*im;
              }
              floatdata[j] = (float)sqrt(M);
            }
            fwrite(floatdata,sizeof(float),dim1*dim2,fp);
          }
          break;
        case RMK: /* Reverse mask of magnitude */
          for(i=0;i<dim3;i++) {
            for(j=0;j<dim2*dim1;j++) {
              if (!d->mask[i][j]) {
                M=0.0;
                for (k=0;k<nr;k++) {
                  re=d->data[k][i][j][0];
                  im=d->data[k][i][j][1];
                  M+=re*re+im*im;
                }
                floatdata[j] = (float)sqrt(M);
              } else {
                floatdata[j] = 0.0;
              }
            }
            fwrite(floatdata,sizeof(float),dim1*dim2,fp);
          }
          break;
        default:
          break;
      } /* end type switch */
      free(floatdata);
      break;
    case DBL64: /* 64 bit double */
      if ((doubledata = (double *)malloc(dim2*dim1*sizeof(double))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
      switch(type) {
        case MG: /* Magnitude */
          for(i=0;i<dim3;i++) {
            for(j=0;j<dim2*dim1;j++) {
              M=0.0;
              for (k=0;k<nr;k++) {
                re=d->data[k][i][j][0];
                im=d->data[k][i][j][1];
                M+=re*re+im*im;
              }
              doubledata[j] = sqrt(M);
            }
            fwrite(doubledata,sizeof(double),dim1*dim2,fp);
          }
          break;
        case RMK: /* Reverse mask of magnitude */
          for(i=0;i<dim3;i++) {
            for(j=0;j<dim2*dim1;j++) {
              if (!d->mask[i][j]) {
                M=0.0;
                for (k=0;k<nr;k++) {
                  re=d->data[k][i][j][0];
                  im=d->data[k][i][j][1];
                  M+=re*re+im*im;
                }
                doubledata[j] = sqrt(M);
              } else {
                doubledata[j] = 0.0;
              }
            }
            fwrite(doubledata,sizeof(double),dim1*dim2,fp);
          }
          break;
        default:
          break;
      } /* end type switch */
      free(doubledata);
      break;
  } /* end precision switch */

  /* Close file */
  fclose(fp);

}
