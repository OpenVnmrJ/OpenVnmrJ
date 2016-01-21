/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* cln3d.c - CLEAN 3d spectrum. 
             Eriks Kupce, Agilent Technologies, Oxford; Feb 04 2011 
             expects inflated, 1D FT-ed complex data.
             nixmax is extended to pow2 here. 
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <string.h>
#include "vnmrio.h"
#include "ddft.h"
#include "cln3d.h"

#define MAXN_B 16  /* maximum # of blocks */

int main (argc, argv)
int argc;
char *argv[];
{
  FILE         *src;
  FILEHEADER    fhd;
  BLOCKHEADER   bhd; 
  PROCPAR       pp;
  char          fnm[MAXSTR], fnm2[MAXSTR];
  short       **sch, reps, ift, dnse;
  unsigned long npl, nplmin, nplmax;
  int           i, j, k, iph, nsp, n_b, xdim, ncyc;
  double        cf, thr;

  printf("cln3d: \n");                         // allow for SPARSE = 'yny' etc  ???
 
  cf=4.0; reps=0; iph=0; ift=1; nsp=0; dnse=0; // set defaults 
  if(argc < 3) 
    vn_error("\n Usage :  cln3d name_in.fid name_out.fid <S/N level> <-f (no ift)> <-v (verbose)> \n"); 
  strcpy(fnm, argv[1]);    (void) add_fid_ext(fnm);              
  strcpy(fnm2, argv[2]);   (void) add_fid_ext(fnm2); 
  i = 3;
  while(argc > i) 
  {
    if((argv[i][0] == '-') && (argv[i][1] == 'f')) ift=0;
    else if((argv[i][0] == '-') && (argv[i][1] == 'd')) dnse=1;
    else if((argv[i][0] == '-') && (argv[i][1] == 'v')) reps=1;
    else if((sscanf(argv[i], "%lf", &thr) > 0) && (thr > 0.5)) cf = thr; 
    i++;  
  }    
  ift = 0;                                    // ift disabled here

  (void) check_sys_arch();

  sch = get_sched(fnm, &nsp, &xdim, reps);     
  pp = get_par3d(fnm, fnm2, &fhd, &bhd, &ncyc, &iph, ift, xdim);  // fn set to np/2

  if((n_b = check_mem_size(pp, &npl, &nplmin, &nplmax, ncyc, reps)) > MAXN_B) 
    vn_error("cln3d: data size is too big. ");
  (void) wm_mtx(pp); 
            
  if((src = open_fid(fnm, "r")) == NULL) exit(0);    // open 3D data file 

  for(i=0; i<n_b; i++)                               // the CLEAN loop starts here 
  {
    printf("processing block # %2d of %2d\n", i+1, n_b);
    if((n_b > 1) && (i==(n_b-1)) && (npl > nplmin)) npl = nplmin;

    k = i*npl;                                        // read and transpose a 3D block   
    if(!get3d_f1f2(src, npl, pp.ni, k, iph, 0)) 
      vn_abort(src, "cln3d: block read failure."); 

    for(j=0; j<npl; j++)   
    {
      if(reps) printf("cleaning plane %4d...", j);
      k = clean_2d(pp, cf, sch, nsp, j, ift, dnse, reps); 
      if(reps) printf("# iterations %4d\n", k);
    }
    printf("\n %d planes processed \n", j);

    if(n_b > 1)
    {
      sprintf(fnm, "%s/tmp_%d", fnm2, i);
      k = pp.fn1; if(ift) k = pp.nimax;
      (void) save_3dblock(fnm, fhd, npl, k, iph); 
    } 
  }                 
  fclose(src);                                        // the processing loop ends here   

  (void) write_tr3d(fnm2, fhd, &bhd, pp, n_b, ncyc, ift, iph, nplmin, nplmax, reps);
       
  printf("\ncln3d done. \n");
  exit(1);
}





