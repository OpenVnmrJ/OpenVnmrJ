/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*
 */

#include <stdio.h>
#include <math.h>
#include <string.h>

#define pftmax 512
#define pftmin 32
#define fnmax 32768
#define fnmin 512
#define npmax 32768
#define npmin 512
#define ntmax 256
#define ntmin 12
#define sitesmax 2
#define sqrt6 2.449489742783
#define sqrt3h 1.224744871392
#define pi 3.14159265358979
#define rad 0.00174532923847  /* 2.0*pi/3600.0*/
#define thetamag 0.955316618  /* acos(sqrt(1.0/3.0)) =  the magic angle */
#define parmax 26
#define FALSE 0
#define TRUE 1

struct datafilehead {
  int nblocks,ntraces,np,ebytes,tbytes,bbytes;
  short vers_id,status;
  int nbheaders;
  };

struct datablockhead {
  short scale,status,index,mode;
  int ctcount;
  float lpval,rpval,lvl,tlt;
 };


void cossin();
void nrerror();
float **matrix();
void free_vector();
void free_matrix();
float *vector();
int *intvector();
void free_intvector();
