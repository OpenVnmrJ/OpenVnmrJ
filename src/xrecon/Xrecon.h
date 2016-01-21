/* Xrecon.h */
/*---------------------------------------------------------------------------*/
/*                                                                           */
/* Xrecon.h: Global includes and defines                                     */
/*                                                                           */
/* Copyright (C) 2012 Paul Kinchesh                                          */
/*               2012 Martyn Klassen                                         */
/*               2012 Margaret Kritzer                                       */
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


/*--------------------------------------------------------*/
/*---- Wrap the header to prevent multiple inclusions ----*/
/*--------------------------------------------------------*/
#ifndef _H_Xrecon_H
#define _H_Xrecon_H


/*------------------------------------------------------------*/
/*---- If C++ compiler is used it needs to know this is C ----*/
/*------------------------------------------------------------*/
//#ifdef __cplusplus
//extern "C" {
//#endif

/*----------------------------------*/
/*---- Include standard headers ----*/
/*----------------------------------*/
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <stddef.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>


/*-----------------------------*/
/*---- Include GSL Headers ----*/
/*-----------------------------*/
#include <gsl/gsl_linalg.h>
#include <gsl/gsl_complex_math.h>
#include <gsl/gsl_sf_bessel.h>


/*-----------------------------*/
/*---- Include fftw header ----*/
/*-----------------------------*/
#include <fftw3.h>


/*--------------------------------*/
/*---- Include complex header ----*/
/*--------------------------------*/
/* NB If you have a C compiler, such as gcc, that supports the recent C99 standard,
   and you #include <complex.h> before <fftw3.h>, then fftw_complex is the native
   double-precision complex type and you can manipulate it with ordinary arithmetic.
   Otherwise, FFTW defines its own complex type, which is bit-compatible with the
   C99 complex type. */
/* We will wait for C99 standard to become a bit more standard */
//#include <complex.h>


/*---------------------------------*/
/*---- Include tiff I/O header ----*/
/*---------------------------------*/
#include <tiffio.h>


/*------------------------------------------*/
/*---- Include Varian data file handler ----*/
/*------------------------------------------*/
#include "data.h"


/*-------------------------------------------*/
/*---- Make sure __FUNCTION__ is defined ----*/
/*-------------------------------------------*/
#ifndef __FUNCTION__
#define __FUNCTION__ __func__
#endif


/*------------------------------*/
/*---- Include bool headers ----*/
/*------------------------------*/
#ifdef _STDC_C99
#include <stdbool.h>
#else
#ifndef bool
#define bool int
#endif
#ifndef true
#define true 1
#endif
#ifndef false
#define false 0
#endif
#endif


/*----------------------------*/
/*---- Some basic defines ----*/
/*----------------------------*/
#define TRUE       1
#define FALSE      0
#define ACTIVE     1
#define NACTIVE    0
#define NACQ       2

/* image scaling */
#define FFT_SCALE 0.5e-6


/*-------------------------*/
/*---- Some data types ----*/
/*-------------------------*/
#define INT16      1  /* 16 bit integer */
#define INT32      2  /* 32 bit integer */
#define FLT32      3  /* 32 bit float */
#define DBL64      4  /* 64 bit double */


/*-------------------------*/
/*---- Some data modes ----*/
/*-------------------------*/
#define NONE       0  /* No data */
#define FID        1  /* Raw */
#define IMAGE      2  /* Image */
#define RE         3  /* Real */
#define IM         4  /* Imaginary */
#define MG         5  /* Magnitude */
#define PH         6  /* Phase */
#define CX         7  /* Complex */
#define STD        8  /* Standard */
#define READ       9  /* Read */
#define PHASE     10  /* Phase */
#define PHASE2    11  /* 2nd Phase */
#define REF       12  /* Reference */
#define REFREAD   13  /* Reference Read */
#define REFPHASE  14  /* Reference Phase */
#define REFPHASE2 15  /* Reference Phase 2 */
#define MK        16  /* Mask */
#define MKREAD    17  /* Mask Read */
#define MKPHASE   18  /* Mask Phase */
#define MKPHASE2  19  /* Mask Phase 2 */
#define RMK       20  /* Reverse Mask of magnitude */
#define MKROI     21  /* Mask ROI */
#define SM        22  /* Sensitivity map */
#define SMREAD    23  /* Sensitivity Map Read */
#define SMPHASE   24  /* Sensitivity Map Phase */
#define SMPHASE2  25  /* Sensitivity Map Phase 2 */
#define GF        26  /* Geometry Factor */
#define RS        27  /* Relative SNR */
#define EPIREF    28  /* EPI reference */
#define VJ        29  /* Defined by VnmrJ parameters */
#define SPATIAL   30  /* for csi */
#define SPECTRAL  31  /* for csi */
#define SPATSPECT 32  /* for csi */
#define PHASE3    33  /* for 3D CSI */


/*---------------------------------------------------------*/
/*---- Some data status bits (0-15) for each dimension ----*/
/*---------------------------------------------------------*/
#define DATA     0x1  /* 0 = no data, 1= data */
#define SHIFT    0x2  /* 0 = no shift, 1 = shift */
#define ZEROFILL 0x4  /* 0 = no zerofill, 1 = zerofill */
#define FFT      0x8  /* 0 = no FFT, 1 = FFT */
/* Bits 4-15 are currently unused */


/*-----------------------------*/
/*---- Some sequence modes ----*/
/*-----------------------------*/
/* For 1D */
#define IM1D      100
#define IM0DCSI   101  /* single voxel csi */

/* For 2D multislice 200 < seqmode < 300 */
#define IM2D      200
#define IM2DCC    201  /* seqcon = "*ccnn" */
#define IM2DCS    202  /* seqcon = "*csnn" */
#define IM2DSC    203  /* seqcon = "*scnn" */
#define IM2DSS    204  /* seqcon = "*ssnn" */
#define IM2DCCFSE 205  /* apptype = "im2Dfse", seqcon = "nccnn" */
#define IM2DCSFSE 206  /* apptype = "im2Dfse", seqcon = "ncsnn" */
#define IM2DSCFSE 207  /* apptype = "im2Dfse", seqcon = "nscnn" */
#define IM2DSSFSE 208  /* apptype = "im2Dfse", seqcon = "nssnn" */
#define IM2DEPI   209  /* apptype = "im2Depi" */
#define IM1DCSI   210  /* csi has one extra dimension */

#define IM2DCCLL  211  /* looklocker seqcon = "*ccnn" */
#define IM2DCSLL  212  /* looklocker seqcon = "*csnn" */
#define IM2DSCLL  213  /* looklocker seqcon = "*scnn" */
#define IM2DSSLL  214  /* looklocker seqcon = "*ssnn" */


/* For 3D 300 < seqmode < 400 */
#define IM3D      300
#define IM3DCC    301  /* seqcon = "**ccn" */
#define IM3DCS    302  /* seqcon = "**csn" */
#define IM3DSC    303  /* seqcon = "**scn" */
#define IM3DSS    304  /* seqcon = "**ssn" */
#define IM3DCFSE  305  /* apptype = "im3Dfse", seqcon = "ncccn" */
#define IM3DSFSE  306  /* apptype = "im3Dfse", seqcon = "nccsn" */
#define IM2DCSCSI   307  /* apptype = "im2DCSI", seqcon = "nncsn" */
#define IM2DSCCSI   308  /* apptype = "im2DCSI", seqcon = "nnscn" */
#define IM2DSSCSI   309  /* apptype = "im2DCSI", seqcon = "ncssn" */
#define IM2DCCCSI   310  /* apptype = "im2DCSI", seqcon = "nnccn" */

/* For 4D */
#define IM4D      400
#define IM3DCCCCSI   401  /* csi has one extra dimension */
#define IM3DCCSCSI   402
#define IM3DCSCCSI   403
#define IM3DCSSCSI   404
#define IM3DSCCCSI   405
#define IM3DSCSCSI   406
#define IM3DSSCCSI   407
#define IM3DSSSCSI   408

 /* max dimensions */
#define MAXDIM    500

/*-------------------------*/
/*---- For TIF scaling ----*/
/*-------------------------*/
#define NSCALE     0  /* Don't scale data */
#define SCALE     -1  /* Scale to maximum of data in each volume */
#define NOISE     -2  /* Scale to see noise */


/*--------------------------------*/
/*---- For NIFTI-1/Analyze7.5 ----*/
/*--------------------------------*/
#define NIFTI      1  /* NIFTI-1 format */
#define ANALYZE    2  /* Analyze7.5 format */


/*---------------------------------------------------------------------------*/
/* Modes for DC correction using dbh.lvl and dbh.tlt in getvol(d,index,mode) */
/*---------------------------------------------------------------------------*/
#define NDCC       0  /* No DC correction using dbh.lvl and dbh.tlt */
#define DCC        1  /* DC correction using dbh.lvl and dbh.tlt */


/*-----------------------------------------------------------------------------------------*/
/* Modes for writing raw data in wrawbin3D(struct data *d,int mode,int type,int precision) */
/*-----------------------------------------------------------------------------------------*/
#define D1         1  /* 1D data mode, data[nr][dim3][dim1] */
#define D12        2  /* 2D/3D data mode, data[nr][dim3][dim2*dim1] */
#define D3         3  /* 3D block processing mode, data[nr][dim2*dim1][dim3] */


/*-------------------*/
/*---- Constants ----*/
/*-------------------*/
#ifndef M_PI
#define M_PI 3.14159265358979323846 /* pi */
#endif
#define DEG2RAD 0.017453292         /* convert degrees to radians */
#define MAXRCVRS 2048               /* maximum number of receivers */


/*------------------------------------------------------*/
/*---- Floating point comparison macros (as in SGL) ----*/
/*------------------------------------------------------*/
/* EPSILON is the largest allowable deviation due to floating point storage */
#define EPSILON 1e-9
#define FP_LT(A,B)  (((A)<(B)) && (fabs((A)-(B))>EPSILON))  /* A less than B */
#define FP_GT(A,B)  (((A)>(B)) && (fabs((A)-(B))>EPSILON))  /* A greater than B */
#define FP_EQ(A,B)  (fabs((A)-(B))<=EPSILON)                /* A equal to B */
#define FP_NEQ(A,B) (!FP_EQ(A,B))                           /* A not equal to B */
#define FP_GTE(A,B) (FP_GT(A,B) || FP_EQ(A,B))              /* A greater than or equal to B */
#define FP_LTE(A,B) (FP_LT(A,B) || FP_EQ(A,B))              /* A less than or equal to B */


/*-------------------------------------------------------*/
/*---- So we can simply include Xrecon.h in Xrecon.c ----*/
/*-------------------------------------------------------*/
#ifdef LOCAL
#define EXTERN
#else
#define EXTERN extern
#endif


int interupt;  // for killing the process

/*-------------------------------------------*/
/*---- Structure to hold input filenames ----*/
/*-------------------------------------------*/
struct file
{
  int      nfiles;    /* number of files */
  char     **fid;     /* fid files */
  char     **procpar; /* procpar files */
};


/*------------------------------------------------*/
/*---- Structure to hold 'procpar' parameters ----*/
/*------------------------------------------------*/
struct pars
{
  int      npars;   /* number of parameters in structure */
  char     **name;  /* parameter name */
  int      *type;   /* parameter type */
  int      *active; /* parameter state */
  int      *nvals;  /* number of values of parameters */
  int      **i;     /* integer values */
  double   **d;     /* real values */
  char     ***s;    /* strings */
};


/*---------------------------------------------------------*/
/*---- Structure to hold maximum value and coordinates ----*/
/*---------------------------------------------------------*/
struct max
{
  int      data;    /* Data flag 0/FALSE=no data, or 1/TRUE=data */
  double   Mval;    /* Maximum magnitude value */
  double   Rval;    /* Maximum real value */
  double   Ival;    /* Maximum imaginary value */
  int      np;      /* position of maximum along np */
  int      nv;      /* position of maximum along nv */
  int      nv2;     /* position of maximum along nv2 */
  int      nv3;     /* position of maximum along nv3 */
};


/*----------------------------------------------*/
/*---- Structure to hold noise measurements ----*/
/*----------------------------------------------*/
struct noise
{
  int      data;    /* Data flag 0/FALSE=no data, or 1/TRUE=data */
  int      zero;    /* Zero data flag 0/FALSE=noise data not zero, or 1/TRUE=noise data is zero */
  int      equal;   /* Equalize flag 0/FALSE=not equal, or 1/TRUE=equal */
  int      samples; /* Number of noise samples */
  double   *M;      /* Mean magnitude */
  double   *M2;     /* Mean magnitude^2 */
  double   *Re;     /* Mean real */
  double   *Im;     /* Mean imaginary */
  double   avM;     /* Averaged over all receivers */
  double   avM2;    /* Averaged over all receivers */
  double   avRe;    /* Averaged over all receivers */
  double   avIm;    /* Averaged over all receivers */
  gsl_matrix_complex   **mat;    /* Noise matrix */
};


/*--------------------------------*/
/*---- Structure to hold data ----*/
/*--------------------------------*/
struct data
{
  FILE                    *fp;         /* fid file pointer */
  FILE                    *datafp;     /* data file pointer */
  FILE                    *phasfp;     /* phasefile file pointer */
  FILE                    *fidfp;      /* for writing in fid format */
  char                    *file;       /* fid file */
  char                    *procpar;    /* procpar file */
  struct datafilehead     fh;          /* data file header */
  struct datablockhead    bh;          /* data block header */
  struct hypercmplxbhead  hcbh;        /* hypercomplex block header */
  struct stat             buf;         /* data file info */

  int                     nvols;       /* number of volumes at end of expt */
  int                     startvol;    /* start volume for processing */
  int                     endvol;      /* end volume for processing */
  int                     vol;         /* current volume counter */
  int                     outvol;      /* output volume (not counting reference volumes) */

  int                     nblocks;     /* number of blocks */
  int                     startpos;    /* start position of block for processing */
  int                     endpos;      /* end position of block for processing */
  int                     block;       /* current block counter */

  int                     np,nv,nv2,nv3; /* number of points and views including nv3 for CSI */
  int                     nseg,etl;    /* number of segments and echo train length */
  int                     ne;          /* number of echoes */
  int                     fn,fn1,fn2,fn3;  /* zerofilling */
  int                     nr,ns;       /* number of receivers and slices */
  int                     korder;       /* for csi, indicates elliptical or otherwise acquired */
  int                     synclist[3];  /* indicates which dimensions are part of sync'ed lists such as pelist, pe2list, etc. */
  int                     startd1, startd2, startd3;  // for csi cropping
  int                     cropd1, cropd2, cropd3;     // for csi cropping

  int                     profile;     /* profile flag, 0/FALSE=not profile, 1/TRUE=profile */
  int                     proj2D;      /* 2D projection flag, 0/FALSE=not 2D projection, 1/TRUE=2D projection */

  int                     nav;         /* navigator flag, 0/FALSE=none, 1/TRUE=navigators */
  int                     nnav;        /* number of navigators */
  int                     *navpos;     /* position(s) of navigator data amongst actual data */

  int                     seqmode;     /* sequence mode depends on seqcon and apptype */

  int                     *dim2order;  /* dim2 order in fid file */
  int                     *dim3order;  /* dim3 order in fid file */
  int                     *dim4order;  /* dim4 order in fid file */
  int                     *pssorder;   /* slice order in fid file */

  int                     *dim2orderR; /* Table for reduced dim2 order */
  int                     *dim3orderR; /* Table for reduced dim3 order */
  int                     *dim4orderR; /* Table for reduced dim4 order */
  int                     dim2R,dim3R, dim4R; /* Size of dim2 and dim3 and dim4 BEFORE reduction */
  int					  d1rev, d2rev, d3rev; /* flag for csi dimension reversal */

  fftw_complex            ***data;     /* actual data */
  fftw_complex	          **csi_data; /* different organization */
  int                     **mask;      /* data mask */

  int                     ndim;        /* number of data dimensions */
  int                     *dimstatus;  /* status of each dimension */

  int                     datamode;    /* data mode to flag different types of data, eg. see default EPI recon */

  struct max              max;         /* maximum value and coordinates */
  struct noise            noise;       /* noise measurement data */

  struct pars             p;           /* 'procpar' parameters */
  struct pars             a;           /* 'array' parameters */
  struct pars             s;           /* 'sviblist' parameters */

  char                    *fdfhdr;     /* fdf header */
};


/*-------------------------------------*/
/*---- Structure to hold dimstatus ----*/
/*-------------------------------------*/
struct dimstatus
{
  int ndim;        /* number of data dimensions */
  int *dimstatus;  /* status of each dimension */
};


/*-------------------------------------------------*/
/*---- Structure to hold segment scaling (EPI) ----*/
/*-------------------------------------------------*/
struct segscale
{
  int data;
  double ***value;
};


/*------------------------------*/
/*---- Main recon functions ----*/
/*------------------------------*/
void recon1D(struct data *d);    /* recon1D.c */
void recon2D(struct data *d);    /* recon2D.c */
void reconEPI(struct data *d);   /* reconEPI.c */
void recon3D(struct data *d);    /* recon3D.c */
void recon2DCSI(struct data *d);


/*---------------------------------*/
/*---- Default recon functions ----*/
/*---------------------------------*/
void default1D(struct data *d);     /* default1D.c */
void default2D(struct data *d);     /* default2D.c */
void defaultEPI(struct data *d);    /* defaultEPI.c */
void default3D(struct data *d);     /* default3D.c */


/*-------------------------------*/
/*---- default1D.c functions ----*/
/*-------------------------------*/
void refcorr1D(struct data *d,struct data *ref);
void combine1D(struct data *d);


/*-------------------------------*/
/*---- profile1D.c functions ----*/
/*-------------------------------*/
void profile1D(struct data *d);


/*----------------------------*/
/*---- proj2D.c functions ----*/
/*----------------------------*/
void proj2D(struct data *d);


/*------------------------*/
/*---- CSI  functions ----*/
/*------------------------*/
void reconCSI(struct data *d);
void reconCSI2D(struct data *d);
void reconCSI3D(struct data *d);
void dimorderCSI2D(struct data *d);
void dimorderCSI3D(struct data *d);
void getblockCSI2D(struct data *d,int volindex,int DCCflag);
void getblockCSI3D(struct data *d,int volindex,int DCCflag);
void w2DCSI(struct data *d,int receiver,int d4, int d3, int d2,int fileid);
void wdfhCSI(struct data *d,int fileid);
void zerofill2DCSI(struct data *d, int mode);
void phaseramp2DCSI(struct data *d, int d4index, int mode);
void shiftdim3dataCSI(struct data *d,int shft);
void phaserampdim3CSI(struct data *d, int mode);
void regridEPSI(struct data *d);


/*----------------------------*/
/*---- mask2D.c functions ----*/
/*----------------------------*/
void mask2D(struct data *d);
void add2mask2D(struct data *d);
int read2Dmask(struct data *d,int mode);
void mask2Ddata(struct data *d1,struct data *d2);


/*-----------------------------*/
/*---- sense2D.c functions ----*/
/*-----------------------------*/
void sense2D(struct data *d);
void sense2Dunfold(struct data *d,struct data *ref);
void svd2Dinmem(struct data *ref);
void setref2Dmatrix(struct data *ref,struct data *d);
int checksenseref(struct data *d,struct data *ref);


/*-----------------------------*/
/*---- sensi2D.c functions ----*/
/*-----------------------------*/
void sensibility2D(struct data *d);
void gmap2D(struct data *d);


/*----------------------------*/
/*---- smap2D.c functions ----*/
/*----------------------------*/
void smap2D(struct data *d,int mode);
int smap2Dvcoil(struct data *d,struct data *ref,int mode);
int smap2Dacoil(struct data *d,int mode);
int setvcoil2D(struct data *d,struct data *ref,struct file *fref);
void gen2Dsmapsos(struct data *d);
void gen2Dsmapsuper(struct data *d);
void gen2Dsmapvcoil(struct data *d,struct data *ref);


/*-------------------------------*/
/*---- asltest2D.c functions ----*/
/*-------------------------------*/
void asltest2D(struct data *d);


/*-----------------------------------*/
/*---- multiblock3D.c  functions ----*/
/*-----------------------------------*/
void multiblock3D(struct data *d);


/*------------------------------*/
/*---- dprocEPI.c functions ----*/
/*------------------------------*/
void addscaledEPIref(struct data *d,struct data *ref1,struct data *ref2);
void addEPIref(struct data *d,struct data *ref);
void ftnvEPI(struct data *d);
void nvfillEPI(struct data *d);
void ftnpEPI(struct data *d);
void zoomEPI(struct data *d);
void revreadEPI(struct data *d);
void prepEPIref(struct data *ref1,struct data *ref2);
void phaseEPIref(struct data *ref1,struct data *ref2,struct data *ref3);
void phaseEPI(struct data *d,struct data *ref);
void phaserampEPI(struct data *d,int mode);
void navcorrEPI(struct data *d);
void stripEPInav(struct data *d);
void weightnavs(struct data *d,double gf);
void setsegscale(struct data *d,struct segscale *scale);
void segscale(struct data *d,struct segscale *scale);
void analyseEPInav(struct data *d);
void getblockEPI(struct data *d,int volindex,int DCCflag);
void setblockEPI(struct data *d);
void setoutvolEPI(struct data *d);
int outvolEPI(struct data *d);
void setnvolsEPI(struct data *d);
void setblockSGE(struct data *d);


/*---------------------------------*/
/*---- prescanEPI.c functions -----*/
/*---------------------------------*/
void prescanEPI(struct data *d);
void settep(struct data *d, int mode);


/*---------------------------*/
/*---- dproc.c functions ----*/
/*---------------------------*/
int *sliceorder(struct data *d,int dim,char *par);
int *phaseorder(struct data *d,int views,int dim,char *par);
int *phaselist(struct data *d,char *par);
void getmax(struct data *d);
void getnoise(struct data *d,int mode);
void equalizenoise(struct data *d,int mode);
void dccorrect(struct data *d);
void scaledata(struct data *d,double factor);
void combine_channels(struct data *d);
void opti_comb(struct data *d, int offset);
double *unwrap1D(double *angles, int nangles);


/*-----------------------------*/
/*---- dproc1D.c functions ----*/
/*-----------------------------*/
void fft1D(struct data *d,int dataorder);
void shiftdata1D(struct data *d,int mode,int dataorder);
void shift1Ddata(struct data *d,int shft,int dataorder);
void weightdata1D(struct data *d,int mode,int dataorder);
void zerofill1D(struct data *d,int mode,int dataorder);
void phaseramp1D(struct data *d,int mode);
void phasedata1D(struct data *d,int mode,int dataorder);
void getmaxtrace1D(struct data *d,int trace);


/*-----------------------------*/
/*---- dproc2D.c functions ----*/
/*-----------------------------*/
void dimorder2D(struct data *d);
void fft2D(struct data *d,int mode);
void ifft2D(struct data *d,int mode);
void shiftdata2D(struct data *d,int mode);
void shift2Ddata(struct data *d,int npshft,int nvshft);
void shift2DCSIdata(struct data *d,int nvshft,int nv2shft);
void weightdata2D(struct data *d,int mode);
void zerofill2D(struct data *d,int mode);
void phaseramp2D(struct data *d,int mode);
void phasedata2D(struct data *d,int mode);
void zoomdata2D(struct data *d,int startdim1, int widthdim1, int startdim2, int widthdim2);
void revread(struct data *d);
void navcorr(struct data *d,struct data *nav);


/*-----------------------------*/
/*---- dproc3D.c functions ----*/
/*-----------------------------*/
void dimorder3D(struct data *d);
void getblock3D(struct data *d,int volindex,int DCCflag);
void getnavblock3D(struct data *d,int volindex,int DCCflag);
void shiftdatadim3(struct data *d,int dataorder,int mode);
void shiftdim3data(struct data *d,int dim3shft);
void shiftD3data(struct data *d,int dim3shft);
void weightdatadim3(struct data *d,int mode);
void zerofilldim3(struct data *d,int mode);
void fftdim3(struct data *d);
void phaserampdim3(struct data *d,int mode);
void phasedatadim3(struct data *d,int mode);


/*-----------------------------*/
/*---- noise2D.c functions ----*/
/*-----------------------------*/
void get2Dnoisematrix(struct data *d,int mode);
void zero2Dnoisematrix(struct data *d);
void print2Dnoisematrix(struct data *d);


/*-----------------------------*/
/*---- dmask2D.c functions ----*/
/*-----------------------------*/
void get2Dmask(struct data *d,int mode);
void fill2Dmask(struct data *d,int mode);


/*---------------------------*/
/*---- dread.c functions ----*/
/*---------------------------*/
void setblock(struct data *d,int dim);
void getblock(struct data *d,int volindex,int DCCflag);
int readfblock(struct data *d,int volindex,int DCCflag);
int readlblock(struct data *d,int volindex,int DCCflag);
int readsblock(struct data *d,int volindex,int DCCflag);
int setoffset(struct data *d,int volindex,int receiver,int dim3index,int dim2index);
void getnavblock(struct data *d,int volindex,int DCCflag);
int readnavfblock(struct data *d,int volindex,int DCCflag);
int readnavlblock(struct data *d,int volindex,int DCCflag);
int readnavsblock(struct data *d,int volindex,int DCCflag);
int setnavoffset(struct data *d,int volindex,int receiver,int dim3index,int seg,int nav);
int sliceindex(struct data *d,int slice);
int segindex(struct data *d,int phase);
int phaseindex(struct data *d,int phase);
int phase2index(struct data *d,int phase2);
int phase3index(struct data *d,int phase3);
void synctablesort2D(struct data *d, int d2, int d3, int *p2, int *p3);
int synctablesort3D(struct data *d, int d2, int d3, int d4, int *p2, int *p3, int *p4);


/*-----------------------------*/
/*---- dread1D.c functions ----*/
/*-----------------------------*/
void getblock1D(struct data *d,int volindex,int DCCflag);


/*-----------------------------*/
/*---- dread2D.c functions ----*/
/*-----------------------------*/
void getblock2D(struct data *d,int volindex,int DCCflag);
void getnavblock2D(struct data *d,int volindex,int DCCflag);
int process2Dblock(struct data *d,char *startpar,char *endpar);


/*---------------------------*/
/*---- dhead.c functions ----*/
/*---------------------------*/
void getdfh(struct data *d);
void getdbh(struct data *d,int blockindex);
void gethcbh(struct data *d,int blockindex);
void reversedfh(struct datafilehead *dfh);
void reversedbh(struct datablockhead *dbh);
void reversehcbh(struct hypercmplxbhead *hcbh);
void printfileheader(struct datafilehead *dfh);
void printfilestatus(struct datafilehead *dfh);
void printblockheader(struct datablockhead *dbh,int blockindex);
void printhcblockheader(struct hypercmplxbhead *hcbh,int blockindex);


/*----------------------------*/
/*---- dutils.c functions ----*/
/*----------------------------*/
void opendata(char *datafile,struct data *d);
void setnvols(struct data *d);
void setstartendvol(struct data *d);
void setdatapars(struct data *d);
void setseqmode(struct data *d);
void setdim(struct data *d);
int spar(struct data *d,char *par,char *str);
int im1D(struct data *d);
int im2D(struct data *d);
int im2DLL(struct data *d);
int im3D(struct data *d);
int im4D(struct data *d);
int imCSI(struct data *d);
double getelem(struct data *d,char *par,int image);
int wprocpar(struct data *d,char *filename);
void copydbh(struct datablockhead *dbh1,struct datablockhead *dbh2);
void setfile(struct file *datafile,char *datadir);
void setreffile(struct file *datafile,struct data *d,char *refpar);
void setfn(struct data *d1,struct data *d2,double multiplier);
void setfn1(struct data *d1,struct data *d2,double multiplier);
void setdimstatus(struct data *d,struct dimstatus *status,int block);
void copynblocks(struct data *d1,struct data *d2);
void copymaskpars(struct data *d1,struct data *d2);
void copysmappars(struct data *d1,struct data *d2);
void copysensepars(struct data *d1,struct data *d2);
int checkequal(struct data *d1,struct data *d2,char *par,char *comment);
void initdatafrom(struct data *d1, struct data *d2);
void initdata(struct data *d);
void nulldata(struct data *d);
void cleardimorder(struct data *d);
void clearstatus(struct data *d);
void zeromax(struct data *d);
void zeronoise(struct data *d);
void initnoise(struct data *d);
void nullnoise(struct data *d);
void closedata(struct data *d);


/*------------------------------*/
/*---- dutils1D.c functions ----*/
/*------------------------------*/
void clear1Dall(struct data *d);
void clear1Ddata(struct data *d);
void clearnoise1D(struct data *d);
void clear1Dmask(struct data *d);


/*------------------------------*/
/*---- dutils2D.c functions ----*/
/*------------------------------*/
int check2Dref(struct data *d,struct data *ref);
void copy2Ddata(struct data *d1,struct data *d2);
void clear2Dall(struct data *d);
void clear2Ddata(struct data *d);
void clearCSIdata(struct data *d);
void clearnoise2D(struct data *d);
void clear2Dmask(struct data *d);
void checkCrop(struct data *d);
void checkCrop3D(struct data *d);


/*------------------------------------*/
/*---- pars.c 'procpar' functions ----*/
/*------------------------------------*/
void getpars(char *procpar,struct data *d);
void getprocparpars(char *procpar,struct pars *p);
void skipprocparvals(FILE *fp,struct pars *p,int type,int nvals);
void procparerror(const char *function,char *par,char *str);
void setarray(struct pars *p,struct pars *a);
int list2array(char *par,struct pars *p1,struct pars *p2);
void setval(struct pars *p,char *par,double value);
void setsval(struct pars *p,char *par,char *svalue);
void cppar(char *par1,struct pars *p1,char *par2,struct pars *p2);
void copypar(char *par,struct pars *p1,struct pars *p2);
void copypars(struct pars *p1, struct pars *p2);
void copyvalues(struct pars *p1,int ix1,struct pars *p2,int ix2);
void mallocnpars(struct pars *p);
void freenpars(struct pars *p);
void printarray(struct pars *a,char *array);
void printprocpar(struct pars *p);
int ptype(char *par,struct pars *p);
int nvals(char *par,struct pars *p);
int *ival(char *par,struct pars *p);
double *val(char *par,struct pars *p);
char **sval(char *par,struct pars *p);
int parindex(char *par,struct pars *p);
int arraycheck(char *par,struct pars *a);
int getcycle(char *par,struct pars *a);
int cyclefaster(char *par1,char *par2,struct pars *a);
void clearpars(struct pars *p);
void cleararray(struct pars *a);
void nullpars(struct pars *p);


/*-------------------------------------*/
/*---- utils.c utilities functions ----*/
/*-------------------------------------*/
int check_CPU_type();
void complexmatrixmultiply(gsl_matrix_complex *A,gsl_matrix_complex *B,gsl_matrix_complex *C);
void complexmatrixconjugate(gsl_matrix_complex *A);
int round2int(double dval);
void reverse2ByteOrder(int nele,char *ptr);
void reverse4ByteOrder(int nele,char *ptr);
void reverse8ByteOrder(int nele,char *ptr);
int doublecmp(const void *double1,const void *double2);
int nomem(char *file,const char *function,int line);
int createdir(char *dirname);
int checkdir(char *dirname);
int checkfile(char *filename);


/*-----------------------------*/
/*---- write1D.c functions ----*/
/*-----------------------------*/
void w1Dtrace(struct data *d,int receiver,int trace,int fileid);
void w1Dblock(struct data *d,int receiver,int fileid);
void wdfh(struct data *d,int fileid);
void wdbh(struct data *d,int fileid);
void openfpw(struct data *d,int fileid);
void closefp(struct data *d,int fileid);


/*--------------------------------*/
/*---- fdfwrite2D.c functions ----*/
/*--------------------------------*/
void w2Dfdfs(struct data *d,int type,int scalevalue,int volindex);
int magnitude2Dfdfs(struct data *d,char *par,char *parIR,char *outdir,int type,int precision,int volindex);
void other2Dfdfs(struct data *d,char *par,char *outdir,int type,int precision,int volindex);
void gen2Dfdfs(struct data *d,int mode,char *outdir,int type,int precision,int volindex);
void w2Dfdf(char *filename,struct data *d,int image,int slice,int sliceindex,int echo,int receiver,int type,int precision);
void wcomb2Dfdf(char *filename,struct data *d,int image,int slice,int sliceindex,int echo,int type,int precision);
void gen2Dfdfhdr(struct data *d,int image,int slice,int echo,int receiver,int type,int precision);
int validtype(int type);
int addpar2hdr(struct pars *p,int ix);
double sliceposition(struct data *d,int slice);


/*--------------------------------*/
/*---- fdfwrite3D.c functions ----*/
/*--------------------------------*/
void w3Dfdfs(struct data *d,int type,int scalevalue,int volindex);
int magnitude3Dfdfs(struct data *d,char *par,char *parIR,char *outdir,int type,int precision,int volindex);
void other3Dfdfs(struct data *d,char *par,char *outdir,int type,int precision,int volindex);
void gen3Dfdfs(struct data *d,int mode,char *outdir,int type,int precision,int volindex);
void w3Dfdf(char *filename,struct data *d,int image,int slab,int echo,int receiver,int type,int precision);
void wcomb3Dfdf(char *filename,struct data *d,int image,int slab,int echo,int type,int precision);
void gen3Dfdfhdr(struct data *d,int image,int slice,int echo,int receiver,int type,int precision);


/*-----------------------------*/
/*---- rawIO3D.c functions ----*/
/*-----------------------------*/
void wrawbin3D(struct data *d,int dataorder,int type,int precision);
void gen3Draw(struct data *d,int output,char *outdir,int dataorder,int type,int precision);
void w3Draw(char *filename,struct data *d,int receiver,int type,int precision);
void w3DrawD3(char *filename,struct data *d,int receiver);
void wcomb3Draw(char *filename,struct data *d,int type,int precision);
void cleardim3data(struct data *d);
void rrawbin3D(struct data *d,int dataorder,int type,int precision);
void get3Draw(struct data *d,int mode,char *indir,int dataorder);
void r3Draw(char *filename,struct data *d,int receiver);
void r3DrawD3(char *filename,struct data *d,int receiver);
void delrawbin3D(struct data *d,int dataorder,int type);
void del3Draw(struct data *d,char *dir);


/*--------------------------------*/
/*---- niftiwrite.c functions ----*/
/*--------------------------------*/
void wnifti(struct data *d,int type,int precision,int volindex);
void writenifti(struct data *d,int type,int precision,int format,int volindex);
int magnifti(struct data *d,char *par,char *parIR,char *outdir,int type,int precision,int format,int volindex);
void othernifti(struct data *d,char *par,char *outdir,int type,int precision,int format,int volindex);
void gennifti(struct data *d,int mode,char *outdir,int type,int precision,int format,int volindex);
void wniftihdr(char *filebase,struct data *d,int precision,int format,int image);
void wniftidata(char *filebase,struct data *d,int receiver,int type,int precision,int format,int newfile);
void wcombniftidata(char *filebase,struct data *d,int type,int precision,int format,int newfile);


/*--------------------------------*/
/*---- tifwrite2D.c functions ----*/
/*--------------------------------*/
int wtifs2D(struct data *d,int type,int scalevalue,int volindex);
int wtif2D(char *filename,struct data *d,int receiver,int slice,int type,double scale);


/*--------------------------------*/
/*---- rawwrite2D.c functions ----*/
/*--------------------------------*/
int wrawbin2D(struct data *d,int type,int precision,int volindex);
int wraw2D(char *filename,struct data *d,int receiver,int slice,int type,int precision);


/*-----------------------------*/
/*---- options.c functions ----*/
/*-----------------------------*/
void getoptions(struct file *f,int argc,char *argv[]);


/*---------------------------*/
/*---- For byte swapping ----*/
/*---------------------------*/
EXTERN int reverse_byte_order;


/*-------------------------*/
/*---- For VnmrJ recon ----*/
/*-------------------------*/
EXTERN int vnmrj_recon;
EXTERN char vnmrj_path[MAXPATHLEN];


/*-------------------------------------------------------------*/
/*---- If C++ compiler is used it needs to know above is C ----*/
/*-------------------------------------------------------------*/
//#ifdef __cplusplus
//}
//#endif

/*----------------------------*/
/*---- End of header wrap ----*/
/*----------------------------*/
#endif
