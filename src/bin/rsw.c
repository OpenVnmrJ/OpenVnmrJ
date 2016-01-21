/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* rsw.c - reduce sw. Read a 1D FT-ed VNMR nD data file and write out a reduced sw window
   for further processing. Data format is always float. Use: wft followed by flush. 
   Applies a zero order phase correction given by rp or as an argument.
   Applies a scaling factor given as an argument.
   Usage :  rsw exp25 fname.fid from(pt) size(pts) (real pts). <-t> <ph0> <scf>
*/
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <stdio.h>
#include "vnmrio.h"
#include "ddft.h"


int main (argc, argv)
int argc;
char *argv[];
{
  FILE        *src, *trg;
  FILEHEADER   fhd, fhd2;
  BLOCKHEADER  bhd, bhd2;
  char         fnm[MAXSTR], fnm2[MAXSTR];
  int          i, j, i1, i2, s0, s1, s2, np, nb, nbh, nbh2, nx, nz, fn, np0, iph, ift;
  float       *ffid;
  double       ph0, scf, cs, sn, re, im, sw, tof, rfl;
  void         inv_fft(), reset_pars_rsw();

  ift=0; iph = 0; ph0 = 0.0; scf=1.0;
  
  printf("\n rsw : \n");
  
  if((argc < 5) || (!fid_ext(argv[2])) || (sscanf(argv[3], "%d", &s1)==0) || (sscanf(argv[4], "%d", &s2) == 0) || (s2<16)) 
    vn_error("\n Usage :  rsw exp25 fname.fid from(pt) size(pts) (real pts). ");
  if((argc > 5) && (argv[5][0]=='-') && (argv[5][1]=='t')) ift = 1;
  if((argc > 6) && (iph = sscanf(argv[6], "%lf", &cs))) ph0 = cs;
  if((argc > 7) && (sscanf(argv[7], "%lf", &cs))) scf = cs;

  sprintf(fnm, "%s", argv[1]); 
  sprintf(fnm2, "%s", argv[2]);
  if(fid_exists(fnm2)) exit(0);
  
  (void) check_sys_arch();

  if((src = open_curpar(fnm, "r")) == NULL) exit(0);  
  if(getval_i(src, "fn", &fn) < 1) fn = 0; 
  i = getval(src, "sw", &sw); 
  i = getval(src, "tof", &tof); 
  i = getval(src, "rfl", &rfl); 
  if(iph==0) iph = getval(src, "rp", &ph0);    
  fclose(src);

  s0=s2;
  if(ift) s2=pow2(s2);                       /* if iFT make it a power of 2 */
  ph0 *= -M_PI/180.0;  cs = cos(ph0); sn = sin(ph0);
  
  (void) mk_dir(fnm2);
  (void) copy_par(fnm, fnm2);

  if((src = open_fid(fnm, "r")) == NULL) exit(0);   /* read FID-file headers */
  if(!get_fheader(src, &fhd, 1)) vn_abort(src, "\nfailed to get FID file header \n");
  if(!get_bheader(src, &bhd, 0)) vn_abort(src, "\nfailed to get FID block header \n");   
  np0 = fhd.np;
  fhd.status = 25;
  bhd.status = 25;
  nz = s2;
  scf *= (double) np0/2;
  if(fn<1) fn = pow2(np0);
  if(ift) nz = (int) (0.5 + s2*np0/fn);   
  (void) set_fhd(&fhd, nz, -1);  
  fclose(src);
  
  if((trg = open_fid(fnm2, "w")) == NULL) exit(0);
  if((src = open_data(fnm, "r")) == NULL) exit(0);   /* read DATA-file header */
  if(!get_fheader(src, &fhd2, 0)) vn_abort(src, "\nfailed to get DATA file header \n");   
  if(!put_fheader(trg, &fhd, 0))   vn_abort(trg, "\nfailed to write FID file header \n"); 
   
  np = fhd2.np;
  nb = fhd2.nblocks;
  nbh = fhd.nbheaders;
  nbh2 = fhd2.nbheaders;
  ffid = (float *) calloc(np, sizeof(float));

  nx = s1 + s0;
  for(i=0; i<nb; i++) 
  {
    for(j=0; j<nbh2; j++)
    {
      if(fread(&bhd2, sizeof(BLOCKHEADER), 1, src) < 1) break;
    }
    if(j<nbh2) break;
  
    for(j=0; j<nbh; j++)
    {
      bhd.iblock = i+1;
      if(fwrite_bh(&bhd, sizeof(BLOCKHEADER), 1, trg) < 1) break;
    }
    if(j<nbh) break;
    
    if(fread(&ffid[0], sizeof(float), np, src) < np) break;
    if(pc_bytes) { for(j=0; j<np; j++) swapbytes(&ffid[j], sizeof(float)); }
    if(iph)
    { 
      for(j=s1; j<nx; j+=2) 
      {
        re = ffid[j]*cs - ffid[j+1]*sn; 
        im = ffid[j+1]*cs + ffid[j]*sn;
        ffid[j] = (float) re; ffid[j+1] = (float) im; 
      }
    }  
    i1=nx; i2=s2;  
    if(ift) 
    {
      i1 = s1 + nz; i2=nz;
      inv_fft(&ffid, np0, fn, s0, s1, s2, i);
    }
    for(j=s1; j<i1; j++) ffid[j] *= scf;       // scale
    if(pc_bytes) 
    { 
      for(j=s1; j<i1; j++) swapbytes(&ffid[j], sizeof(float)); 
    }   
    if(fwrite(&ffid[s1], sizeof(float), i2, trg) < i2) break;
    printf("blocks processed\r%-4d ", i+1);                    
  }
  printf("blocks processed");     
  fclose(src); fclose(trg);  

  printf("\n fixing procpar... \n");  

  (void) fix_np(fnm2, nz, -1, -1, 0);
  re =  (double) s1/fn;
  im =  (double) s0/fn;
  tof += 0.5*sw*(1.0 - 2.0*re - im); 
  i = (s2-s0)/2;
  rfl += sw*(re + im + (double) i/fn - 1.0);
  sw *= (double) s2/fn;
  
  (void) reset_pars_rsw(fnm2, sw, 0.0, 0.0);
  if(!setpar_d(fnm2, "tof", tof)) printf("rsw :  failed to change the tof value\n");
  if(!setpar_d(fnm2, "rfl", rfl)) printf("rsw :  failed to change the rfl value\n");
  if(ift) 
  {
    if(!setpar_d(fnm2, "at", 0.5*nz/sw)) printf("rsw :  failed to change the at value\n");
  }

  printf("\nDone.\n"); 
  exit(1);
}


void inv_fft(xfid, xnp, xfn, s0, s1, np, ix)    
float *xfid[];
int    xnp, xfn, s0, s1, np, ix;
{
  int     j, k, np2, np0;
  double *tRe, *tIm;
      
  np2 = np/2;
  tRe = (double *)calloc(np2, sizeof(double));
  tIm = (double *)calloc(np2, sizeof(double));
  
  for(j=0; j<np2; j++) tRe[j] = 0.0, tIm[j] = 0.0;     
  
  np0 = s1+s0; 
  k = (np - s0)/4; 
  if(k<0) k=0;
  for(j=s1; j<np0; j+=2, k++) 
    tIm[k] = (*xfid)[j], tRe[k] = (*xfid)[j+1];     
  
  ft(&tRe, &tIm, 'x', np2);

  k = s1; 
  for(j=0; j<np2; j++, k+=2) 
    (*xfid)[k] = tIm[j], (*xfid)[k+1] = tRe[j];    
    
  free(tRe); free(tIm);  

  return;
}


void reset_pars_rsw(dir, sw, sw1, sw2)  /* reset parameters for rsw */
char *dir;
double sw, sw1, sw2;
{
  FILE	  *fnm, *ftmp;
  int     i;
  double  tm;
  char    str[512], str2[512];
  
  strcat(strcpy(str, dir), "/procpar");
  strcat(strcpy(str2, dir), "/tmp");

  rename(str, str2); 

  if((fnm=fopen(str,"w"))==NULL) 
    vn_error("failed to open procpar. "); 
  if((ftmp=fopen(str2,"r"))==NULL) 
    vn_error("failed to open tmp file. "); 

  fseek(ftmp, 0, 0); 
  while (fgets(str, 512, ftmp))		/* fix parameters */
  {
    switch(str[0])
    {   
      case 'f' :
        if((str[1] == 'n') && (str[2] == ' '))	     /* fn */
          deactivate_par(str);

      break;
                
      case 'l' :   	        /* lp, lp1, lp2, lp3 */      
        if(str[1] == 'p')
        {
          sscanf(str, "%s", str2);
          if((str2[2] == '\0') || ((str2[3] == '\0') && ((str2[2] == '1') || (str2[2] == '2') || (str2[2] == '3')))) 
          {
            fputs(str, fnm); fgets(str, 512, ftmp);             
            if(sscanf(str, "%d %lf", &i, &tm) == 2)
              sprintf(str, "%d 0.0\n", i); 
          }
        }  
      break;          
                
      case 'p' :   	        /* proc, proc1, proc2, proc3 */      
        if(str[1] == 'r') 
        {
          sscanf(str, "%s", str2);
          if((strcmp(str2, "proc1") == 0) || (strcmp(str2, "proc2") == 0) || (strcmp(str2, "proc3") == 0))
          {
            fputs(str, fnm); fgets(str, 512, ftmp);            
            if(sscanf(str, "%d %s", &i, str2) == 2)
              sprintf(str, "%d \"\" \n", i); 
          }          
        }  
      break;          
                
      case 'r' :   	        /* rp, rp1, rp2, rp3 */      
        if(str[1] == 'p')
        {
          sscanf(str, "%s", str2);
          if((str2[2] == '\0') || ((str2[3] == '\0') && ((str2[2] == '1') || (str2[2] == '2') || (str2[2] == '3')))) 
          {
            fputs(str, fnm); fgets(str, 512, ftmp);             
            if(sscanf(str, "%d %lf", &i, &tm) == 2)
              sprintf(str, "%d 0.0\n", i); 
          }
        }  
      break;          
                
      case 's' :        
        if(str[1] == 'w') 	        /* sw, sw1, sw2 */
        {
          sscanf(str, "%s", str2);
          if((str2[2] == '\0') && (sw > 0.1))
          {
            fputs(str, fnm); fgets(str, 512, ftmp);             
            if(sscanf(str, "%d %lf", &i, &tm) == 2)
              sprintf(str, "%d %.2f\n", i, sw); 
          }
          else if(str2[3] == '\0') 
          {
            if((str2[2] == '1') && (sw1 > 0.1))
            {
              fputs(str, fnm); fgets(str, 512, ftmp);             
              if(sscanf(str, "%d %lf", &i, &tm) == 2)
                sprintf(str, "%d %.2f\n", i, sw1); 
            }
            else if((str2[2] == '2') && (sw2 > 0.1))
            {
              fputs(str, fnm); fgets(str, 512, ftmp);             
              if(sscanf(str, "%d %lf", &i, &tm) == 2)
                sprintf(str, "%d %.2f\n", i, sw2); 
            }
          }
        }  
        else                           /* ssfilter */ 
        {
          sscanf(str, "%s\n", str2);
          if(strcmp(str2, "ssfilter") == 0)
            deactivate_par(str);
        }
      break;          
    }
    fputs(str, fnm);
  }
  fclose(fnm);
  fclose(ftmp);

  strcat(strcpy(str2, dir), "/tmp");
  remove(str2);
}


