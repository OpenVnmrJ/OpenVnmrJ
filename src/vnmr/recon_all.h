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
#define MAXPE 512
#define MAXRO 1024
#define MSEC_TO_SEC 0.001
#define SEC_TO_MSEC 1000.
#define IMAGE_SCALE 0.5e-6
#define ERROR 1
#define TRUE 1
#define FALSE 0

struct _fdfInfo
{
  char imdir[MAXPATHL];
  int fullpath;
  int nro;
  int npe;
  float fovro;
  float fovpe;
  char fidname[MAXPATHL];
  int slice;
  int slices;
  int echo;
  int echoes;
  float te;
  float tr;
  int ro_size;
  int pe_size;
  char seqname[MAXSTR];
  float ti;
  int array_index;
  float image;
  float orientation[9];
};

typedef struct _fdfInfo fdfInfo;

struct _svInfo
{
  int multi_shot;
  int multi_slice;
  int etl;
  int slicesperblock;
  int viewsperblock;
  int within_slices;
  int within_views;
  int phase_compressed;
  int slice_compressed;
  int views;
  int slices;
};

typedef struct _svInfo svInfo;

#endif EPI_RECON
