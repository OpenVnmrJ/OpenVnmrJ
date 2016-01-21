/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*------------------------------------------------------
|                                   
|   Include file for epi reconstruction definitions
|                                   
+------------------------------------------------------*/
#ifndef EPI_RECON
#define EPI_RECON

#define SHORTSTR 50
#define MAXPE 4096 
#define MAXRO 4096 
#define MINPE 4 
#define MINRO 4
#define BIG3D 33554432

#define MSEC_TO_SEC 0.001
#define SEC_TO_MSEC 1000.
#define SEC_TO_USEC 1000000.0
#define MSEC_TO_USEC 1000.0
#define DEG_TO_RAD M_PI/180.
#define MM_TO_CM 0.10

#define IMAGE_SCALE 1.0 

#define ERROR 1
#define TRUE 1
#define FALSE 0
#define PHS_DONE 0x1 
#define IMG_DONE 0x2

#define MAGNITUDE 0
#define HALFF .25
#define RAW_MAG 1
#define RAW_PHS 2
#define RAW_MP 3
#define PHS_IMG 4
#define MAXDISPLAY 1000
#define DISPFLAG(a,b)  ((!(b))||((a)>MAXDISPLAY)||((a)%(b))) ? FALSE : TRUE
#define MAXJOINTARRAY 10

/* windows for filtering */
#define NOFILTER 0
#define BLACKMANN 1
#define HANN 2
#define HAMMING 3
#define GAUSSIAN 4
#define GAUSS_NAV 5
#define MAX_FILTER GAUSS_NAV

struct _fdfInfo
{
  char imdir[MAXPATHL];
  int fullpath;
  int nro;
  int npe;
  float fovro;
  float fovpe;
  float thickness;
  float gap;
  char fidname[MAXPATHL];
  int datatype;
  int slice;
  int slices;
  int echo;
  int echoes;
  int npsi;
  int nphi;
  int ntheta;
  float te;
  float tr;
  int ro_size;
  int pe_size;
  char seqname[MAXSTR];
  char studyid[MAXSTR];
  char position1[MAXSTR];
  char position2[MAXSTR];
  float ti;
  int array_index;
  float sfrq;
  float dfrq;
  float image;
  char tn[MAXSTR];
  char dn[MAXSTR];
};

typedef struct _fdfInfo fdfInfo;

struct _svInfo
{
  char petable[MAXSTR];
  dfilehead fid_file_head;
  int etl;
  int uslicesperblock;
  int slicesperblock;
  int slabsperblock;
  int viewsperblock;
  int sviewsperblock;
  int within_slices;
  int within_views;
  int within_sviews;
  int within_slabs;
  int pe_size;
  int pe2_size;
  int slices;
  int slabs;
  int echoes;
  int ro_size;
  int slice_reps;
  int slab_reps;
  int nblocks;
  int ntraces;
  int dimafter;
  int dimfirst;
  int epi_dualref;
  int pc_option;
  int nav_option;
  int nnav;
  int nav_first;
  int nav_pts;
  /* flags */
  int threeD;
  int epi_seq;
  int multi_shot;
  int multi_slice;
  int multi_echo;
  int phase_compressed;
  int slice_compressed;
  int sliceenc_compressed;
  int slab_compressed;
  int flash_converted;
  int look_locker;
};

typedef struct _svInfo svInfo;


struct _arrayElement
{
  int size;
  int denom;
  int idenom;
  int nparams;
  char names[MAXJOINTARRAY][MAXSTR];
};
typedef struct _arrayElement arrayElement;

struct _recon_info
{
  svInfo svinfo;
  int do_setup;
  int pc_slicecnt;
  int slicecnt;
  int image_order;
  int phase_order;
  int rawmag_order;
  int rawphs_order;
  int dispcnt;
  int dispint;
  fdfInfo picInfo;
  MFILE_ID fidMd;
  int nchannels;
  int tbytes;
  int ebytes;
  int bbytes;
  int imglen;
  int ntlen;
  int within_nt;
  int epi_rev; 
  float *rwindow;
  float *pwindow;
  float *swindow;
  float *nwindow;
  int dc_flag;
  int zeropad;
  int zeropad2;
  int zeropad3;
  double image_scale;
  double ro_frq;
  double pe_frq;
  double ro_ph;
  double pe_ph;
  int dsize;
  int nsize;
  int rawflag;
  int smash;
  int sense;
  int phsflag;
  int phsrev_flag;
  int alt_phaserev_flag;
  int alt_readrev_flag;
  int narray;
  int variableNT;
  arrayElement *arrayelsP;
};
typedef struct  _recon_info reconInfo;

#endif 
