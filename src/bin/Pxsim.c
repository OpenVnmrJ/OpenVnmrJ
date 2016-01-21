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
/* Pxsim.c - Pbox simulator for dprofile macro */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <string.h>

#define SHAPELIB "/vnmrsys/shapelib/"
#define SYSSHAPELIB "/vnmr/shapelib/"
#define FN_SIM "pbox.sim"
#define FN_XY  "pbox.xy"
#define FN_Z   "pbox.mz"

#ifndef M_PI
#define M_PI	3.14159265358979323846
#endif

#define MAXSTR 512
#define MINST  64
#define MAX(A, B) ((A) > (B) ? (A) : (B))

#include "Px_util.h"
#include "Pbox_rsh.h"
#include "Pbox_sim.h"           /* Pbox simulator */

int main(int argc, char *argv[])
{
  FILE *fil;
  int i, np;
  char ext[4], str[32];
  double tmp;
  Blodata bl;

  bl.time = 60;

  if (argc < 2)
  {
    printf("Usage: Pxsim file.ext simtime #steps add/sub\n");
    exit(1);
  }

  if ((i = getext(ext, argv[1])) == 0)
  {
    printf("\nreadshape : filename extension required !\n");
    exit(1);
  }  

  if (strcmp(ext, "GRD") == 0)
  {
    printf("Gradient shapes not supported\n");
    exit(1);
  }
  else if (strcmp(ext, "RF") && strcmp(ext, "DEC"))
  {
    printf("Unexpected file type\n");
    exit(1);
  }


  bl.sign = 0;
  bl.reps = 1;
  if ((argc > 2) && (isdigit(argv[2][0]))) bl.time = stoi(argv[2]);
  if ((argc > 3) && (isdigit(argv[3][0]))) bl.nstp = stoi(argv[3]);
  else bl.nstp = 0;
  if (argc > 4) 
  {
    if (argv[4][0] == 'a') bl.sign = 1;
    else if (argv[4][0] == 's') bl.sign = -1;
  }

  if ((fil = fopen(argv[1], "r")) == NULL)     /* read file */
  {
    printf("Cannot open input file %s\n\n", argv[1]);
    exit(1);
  }
  printf("Reading shapefile %s ...\n", argv[1]);

  i = getsimdata(&bl, fil);
  if ((bl.pw<0.000001) || (bl.sw<1.0) || (bl.B1max<0.001))
  {
    printf("Insufficient information in shapefile header\n");
    exit(1);
  }
  bl.np = readshape(fil, ext, 0.0);
  if(bl.rdc > 0.001)
    bloch_rd(&bl);
  else
    bloch(&bl);
}
