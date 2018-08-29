/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* fidx.c - scale fid. Applies a scaling factor to a . fid file 
            Agilent Technologies, Eriks Kupce, Oxford, July 2011; 
   Usage :  fidx fname.fid scf
*/
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include "vnmrio.h"

int main (argc, argv)
int argc;
char *argv[];
{
  FILE        *src, *trg;
  FILEHEADER   fhd;
  BLOCKHEADER  bhd;
  char         fnm[MAXSTR], fnm2[MAXSTR];
  int          i, j;
  float       *ffid;
  double       scf;

  scf=1.0;
  if((argc < 2) || (sscanf(argv[2], "%lf", &scf) == 0)) 
    vn_error("\n Usage :  fidx fname.fid  scf ");

  printf("\n fidx : \n");
  
  sprintf(fnm, "%s", argv[1]); 
  (void) add_fid_ext(fnm);
  
  (void) check_sys_arch();

  sprintf(fnm2, "%s/fid.fidx", fnm);
  sprintf(fnm, "%s/fid", fnm);
  rename(fnm, fnm2);
  if((src = open_file(fnm2, "r")) == NULL) exit(0);  
  if((trg = open_file(fnm, "w")) == NULL) exit(0);  

  if(!get_fheader(src, &fhd, 0)) vn_abort(src, "\nfailed to get DATA file header \n");   
  if(!put_fheader(trg, &fhd, 0)) vn_abort(trg, "\nfailed to write FID file header \n"); 
   
  ffid = (float *) calloc(fhd.np, sizeof(float));

  for(i=0; i<fhd.nblocks; i++) 
  {
    for(j=0; j<fhd.nbheaders; j++)
    {
      if(fread(&bhd, sizeof(BLOCKHEADER), 1, src) < 1) break;
      if(fwrite(&bhd, sizeof(BLOCKHEADER), 1, trg) < 1) break;
    }
    if(j<fhd.nbheaders) break;
      
    if(fread(&ffid[0], sizeof(float), fhd.np, src) < fhd.np) break;
    if(pc_bytes) 
    { 
      for(j=0; j<fhd.np; j++) 
      {
        swapbytes(&ffid[j], sizeof(float)); 
        ffid[j] *= scf;
        swapbytes(&ffid[j], sizeof(float)); 
      }
    }
    else 
    {
      for(j=0; j<fhd.np; j++) 
        ffid[j] *= scf;
    }
    if(fwrite(&ffid[0], sizeof(float), fhd.np, trg) < fhd.np) break;
    printf("blocks processed\r%-4d ", i+1);                    
  }
  printf("blocks processed");     
  fclose(src); fclose(trg);  

  remove(fnm2);

  printf("\nDone.\n"); 
  exit(1);
}

