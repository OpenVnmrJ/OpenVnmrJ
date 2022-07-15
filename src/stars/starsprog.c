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

#include <stdlib.h>
#include "starsprog.h"

extern FILE *resfile,*macfile;

extern float csaspec_1();
extern float qcsaspec_1();
extern float quadspec_1();
extern float csaspec_2();
extern float qcsaspec_2();
extern float quadspec_2();
extern void readinput();
extern void outputfid();
extern void iterate_1();
extern void iterate_n();
extern void outputfid();
extern void printresults();
extern void reduce();

extern float qcpar[parmax];
extern float qcinc[parmax];
extern float *cc,*ss,*cg,*sg,*c2g,*s2g,*cfi,*sfi,*c2fi,*s2fi;
extern float *c3fi,*s3fi,*c4fi,*s4fi;
extern int npar,pft,fn,np,progorder,rmsflag,sites;
extern int quadflag,qcsaflag,csaflag;
float *spec,**ssbint;

int main(argc, argv)

int argc;
char *argv[];
{
/* argv[1] = input file for simulation parameters */
/* argv[2] = input file for experimental ssb integrals or spectrum points */
/* argv[3] = a macro file for adjustments of parameters */
/* argv[4] = output file af iteration result */
/* argv[5] = output file for calculated FID */
float (*progtype)(),rms;
int i,l;

if (argc < 6) { printf("\nerror: missing file-names\n"); 
                exit(-1);}
if ((resfile=fopen(argv[4],"w")) == NULL) nrerror("can not open resfile");
readinput(argv[1],argv[2],argv[3]);
if (progorder == 2) {
  l = fn<np? np : fn;
  spec = vector(0,2*l);
  for (i=0; i<2*l; spec[i++] = 0.0);}
 else spec = vector(0,2*npmax);
if (progorder == 1) ssbint = matrix(1,sites,0,2*pft);
if (csaflag) 
  progtype = progorder == 1 ? (csaspec_1) : (csaspec_2);
  else if (quadflag)
  progtype = progorder == 1 ? quadspec_1 : quadspec_2;
  else progtype = progorder == 1 ? qcsaspec_1 : qcsaspec_2;
if (npar==1) iterate_1(*progtype);
  else if (npar>1) iterate_n(*progtype);
npar=0;
rms = (*progtype)(qcpar);
if (rmsflag) printresults(qcpar,rms,0);
if (progorder == 1) free_matrix(ssbint,1,sites,0,2*pft);
free_vector(cc,0,3600);
free_vector(ss,0,3600);
free_vector(cg,0,pft);
free_vector(sg,0,pft);
free_vector(c2g,0,pft);
free_vector(s2g,0,pft);
free_vector(cfi,0,pft);
free_vector(sfi,0,pft);
free_vector(c2fi,0,pft);
free_vector(s2fi,0,pft);
if (progorder == 2) {
  free_vector(c3fi,0,pft);
  free_vector(s3fi,0,pft);
  free_vector(c4fi,0,pft);
  free_vector(s4fi,0,pft);
  }
if (!rmsflag) outputfid(argv[5]);
if (progorder == 2) {
  l = fn<np? np : fn;
  free_vector(spec,0,2*l);
  }
  else free_vector(spec,0,2*npmax);
fclose(resfile);
fclose(macfile);
printf("\nok\n"); fflush(stdout);
return 0;
}  /* end of main */
