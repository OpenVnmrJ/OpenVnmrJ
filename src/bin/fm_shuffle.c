/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#include <sys/file.h>
#include <sys/errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>


#define maxsize 65536

extern void putdata(int *si, float *wbuff, char infile_name1[]);
extern void getdata(int *si, int *nproj, float *wbuff, char infile_name1[]);

int main(int argc, char *argv[])
{
  float    wrkbuff0[maxsize],wrkbuff1[maxsize];
  char 	fil_in[100];  
  int j, ne, skip, size, nf, maxproj;

  strcpy(fil_in,argv[1]);
  getdata(&size,&maxproj,&wrkbuff0[0],fil_in);
  skip=atoi(argv[2]);

  fprintf(stderr,"shuffling every %d fid\n", maxproj/skip);
  for (ne=0;ne<maxproj/skip;ne++)
  {
    for (nf=0;nf<skip;nf++)
    {
      for (j=0;j<size;j++)
      {
        wrkbuff1[(nf+ne*skip)*size+j] = wrkbuff0[(nf*maxproj/skip+ne)*size+j];
      }
    }
  }
  putdata(&size,&wrkbuff1[0],fil_in);
}

