/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/* calcq.c : version for export : 25 viii 99 : P B Chilvers */

/* useage : calcq('filename') */
/* original before barcorrection */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "vnmrsys.h"
#include "group.h"
#include "data.h"
#include "allocate.h"
#include "pvars.h"
#include "wjunk.h"
#include "init2d.h"

#define debug  1
#define debug2  1
#define debug3  1
#define Pi 3.14159265358979323846
#define twoPi 2.0*Pi
#define phtol Pi
#define MAX 80

FILE *bug,*bug2,*bug3;
static float  *data;    
static int    npxx;
static dfilehead fidhead;
static char path[MAXPATHL];
static int Dres;
static int getspec(int spc);
static void nrerror(char *error_text);
extern float  *get_data_buf();
static double *dvector(int nl, int nh);
double **dmatrix(int nrl,int nrh,int ncl,int nch);

/* ###### main function starts here ###### */

int calcq(int argc, char *argv[], int retc, char *retv[])
{
  int spc,i,j,k,q,r,c;
  int nji,nki,tni,alt,ncbout,ncycles;
  int nfieldpts,shim,nshims,gzwin,cb,ncb;
  FILE *out,*ampout,*ampout2;
  double qd,rd,jd,kd,tnid,sw1,sw2,tau,Hzpp;
  double phi,lsfrq1,lsfrq2,rcycles;
  double grwin,rni,arraydim,rfn,phase2,phase3,phase4;
  double alpha,beta,wt;
  double *v,*rbars;
  double **cormp,**barcor;
  double **lstpnt,***corr,**lstpnt2,***corr2,**lstpnt3,***corr3;
  double ***phd,***amp,***ci;
  double ***sqrre,***sqrim,***sqrre2,***sqrim2;
  char gxyzstr[MAX];

/* ###### check input aruments ###### */

  (void) retc;
  (void) retv;
  if (argc != 2)
     {Werrprintf("wrong number of input arguments: argc only %d passed",argc);
      ABORT;}

/* ###### open output files ###### */

  if ((out = fopen(argv[1],"w")) == NULL)
     {Werrprintf("Cannot open shimmap output file");
      ABORT; }
  if ((ampout = fopen("fieldamp","w")) == NULL)
     {Werrprintf("Cannot open shimmap output file");
      ABORT; }
  if ((ampout2 = fopen("shimamps","w")) == NULL)
     {Werrprintf("Cannot open shimmap output file");
      ABORT; }
  if (debug)  bug = fopen("debug","w");
  if (debug2) bug2 = fopen("debug2","w");
  if (debug3) bug3 = fopen("debug3","w");

/* ###### read relavent parameters ###### */

  P_getreal(CURRENT,"fn",&rfn,1);
  P_getreal(CURRENT,"gzwin",&grwin,1);
  P_getreal(CURRENT,"arraydim",&arraydim,1);
  P_getreal(CURRENT,"ni",&rni,1);
  P_getreal(CURRENT,"lsfrq1",&lsfrq1,1);
  P_getreal(CURRENT,"lsfrq2",&lsfrq2,1);
  P_getreal(CURRENT,"sw1",&sw1,1);
  P_getreal(CURRENT,"sw2",&sw2,1);
  P_getreal(CURRENT,"tau",&tau,2);
  P_getreal(CURRENT,"cycles",&rcycles,1);
  strcpy(gxyzstr,"0000000000000000");
  P_getstring(CURRENT,"gxyzcode",gxyzstr,1,MAX);
 
/* ###### calculate other variables ###### */

  nji = (int)rni;
  tni = nji; nki = nji;
  tnid = (double)tni;
  ncb = nji*nki;
  npxx = (int)rfn;
  nshims = (int)((arraydim/(2.0*rni*rni))-1);
  tau *= (2.0*Pi); /* convert to Hz */
  ncycles = (int)rcycles;
  wt=0.0;

  if ( (ncycles < 1) || (ncycles > 10) ) {
     Werrprintf("cycles reset to 1");
     ncycles = 1; }

  Hzpp = sw/(npxx/2);
  printf("Hzpp = %f\n",Hzpp);

  gzwin = (int)(0.005*npxx*(100.0 - grwin)); 
  if (gzwin%2) gzwin--;  

/* ###### set up vectors and matrices ###### */

  data=(float *)allocateWithId(sizeof(float)*fn,"calcq");

  v = dvector(0,npxx/2-1);
  rbars = dvector(1,ncb);
  lstpnt = dmatrix(0,ncb,0,nshims+1);
  lstpnt2 = dmatrix(0,ncb,0,nshims+1);
  lstpnt3 = dmatrix(0,ncb,0,nshims+1);
  cormp = dmatrix(0,ncb,0,nshims+1);
  barcor = dmatrix(0,ncb,0,nshims+1);

  sqrre=(double ***)allocateWithId((unsigned) ncb*sizeof(double **),"calcq")-1;
  for (i=1;i<=ncb;i++) sqrre[i]=dmatrix(0,nshims+1,0,npxx/2-1);

  sqrim=(double ***)allocateWithId((unsigned) ncb*sizeof(double **),"calcq")-1;
  for (i=1;i<=ncb;i++) sqrim[i]=dmatrix(0,nshims+1,0,npxx/2-1);

  sqrre2=(double ***)allocateWithId((unsigned) ncb*sizeof(double **),"calcq")-1;
  for (i=1;i<=ncb;i++) sqrre2[i]=dmatrix(0,nshims+1,0,npxx/2-1);

  sqrim2=(double ***)allocateWithId((unsigned) ncb*sizeof(double **),"calcq")-1;
  for (i=1;i<=ncb;i++) sqrim2[i]=dmatrix(0,nshims+1,0,npxx/2-1);

  phd=(double ***)allocateWithId((unsigned) ncb*sizeof(double **),"calcq")-1;
  for (i=1;i<=ncb;i++) phd[i]=dmatrix(0,nshims+1,0,npxx/2-1);

  amp=(double ***)allocateWithId((unsigned) ncb*sizeof(double **),"calcq")-1;
  for (i=1;i<=ncb;i++) amp[i]=dmatrix(0,nshims+1,0,npxx/2-1);

  ci=(double ***)allocateWithId((unsigned) ncb*sizeof(double **),"calcq")-1;
  for (i=1;i<=ncb;i++) ci[i]=dmatrix(0,nshims+1,0,npxx/2-1);

  corr=(double ***)allocateWithId((unsigned) ncb*sizeof(double **),"calcq")-1;
  for (i=1;i<=ncb;i++) corr[i]=dmatrix(0,nshims+1,0,npxx/2-1);

  corr2=(double ***)allocateWithId((unsigned) ncb*sizeof(double **),"calcq")-1;
  for (i=1;i<=ncb;i++) corr2[i]=dmatrix(0,nshims+1,0,npxx/2-1);

  corr3=(double ***)allocateWithId((unsigned) ncb*sizeof(double **),"calcq")-1;
  for (i=1;i<=ncb;i++) corr3[i]=dmatrix(0,nshims+1,0,npxx/2-1);

/* ###### initialise vectors and matrices ###### */

  for (i=0;i<npxx/2;i++)
      v[i] = sw - i*sw/(npxx/2 - 1) - rflrfp; 

  for (cb=1;cb<=ncb;cb++){
      for (shim=1;shim<=nshims+1;shim++){
          lstpnt[cb][shim] = 0.0; lstpnt2[cb][shim] = 0.0; lstpnt3[cb][shim] = 0.0;
          cormp[cb][shim] = 0.0;
          barcor[cb][shim] = 0.0;
          for (i=0;i<npxx/2;i++){
              sqrre[cb][shim][i] = 0.0;  sqrim[cb][shim][i] = 0.0;
              sqrre2[cb][shim][i] = 0.0; sqrim2[cb][shim][i] = 0.0;
              phd[cb][shim][i] = 0.0;    amp[cb][shim][i] = 0.0;
              ci[cb][shim][i] = 0.0;     corr[cb][shim][i] = 0.0; 
              corr2[cb][shim][i] = 0.0;  corr3[cb][shim][i] = 0.0;
          } /* end of i loop */
      } /* end of shim loop */
  } /* end of cb loop */

  ncbout=0;
  for (cb=1;cb<=ncb;cb++){
      if (gxyzstr[cb-1] == '0')
         rbars[cb] = 0.0;
      else
         {rbars[cb] = 1.0; ncbout++;}
  }
  nfieldpts = ncbout*((npxx - gzwin*2)/2); 

/* ###### perform discrete 2Dft ###### */

  phi=0.0; cb=0;
  for (q=1;q<=nji;q++) 
   {
    for (r=1;r<=nki;r++) 
     {cb++;
      for (i=0;i<npxx;i+=2) 
       {
        spc=0;
        for (j=1;j<=nji;j++)
         {
          for (k=1;k<=nki;k++)
           {
            qd = (double)q; rd = (double)r; jd = (double)j; kd = (double)k;
            phi = 2.0*Pi*(((jd-1.0)*(qd-1.0) + (kd-1.0)*(rd-1.0))/tnid); 
            phi += 2.0*Pi*((jd+kd)*(0.5 + (1.0/(2.0*tnid))) - jd*(lsfrq2/sw2) - kd*(lsfrq1/sw1));
            for (shim=1;shim<=(nshims+1);shim++)
             {
              for (alt=0;alt<=1;alt++)
               {
                getspec(spc);
                if (alt == 0) { /* alt = 0 for 1st tau value */
                   sqrre[cb][shim][i/2] += data[i]*cos(phi) + data[i+1]*sin(phi);
                   sqrim[cb][shim][i/2] += data[i+1]*cos(phi) - data[i]*sin(phi);  }

                if (alt == 1) { /* alt = 1 for 2nd tau value */
                   sqrre2[cb][shim][i/2] += data[i]*cos(phi) + data[i+1]*sin(phi);
                   sqrim2[cb][shim][i/2] += data[i+1]*cos(phi) - data[i]*sin(phi); }

                spc++;
               } /* end of tau loop */
             } /* end of shims loop */
           } /* end of inner summation loop */
         } /* end of summation loops */

      } /* end of i loop */
    } /* end of r loop */
  } /* end of q loop */

/* ################### calculate phases ################### */
/* use i for sqrre and sqrim for tau = 0                    */
/* use j for sqrre2 and sqrim2 for tau = inc                */
/* where j is the corrected index value, when taking into   */
/* account the effect of the field inhomogeneity and the    */
/* shim gradient increment on second profile                */

for (k=1;k<=ncycles;k++){ /* start of cycle loop for phase calculations */

  for (cb=1;cb<=ncb;cb++)
      {
       if (rbars[cb] > 0.0) {
       for (i=(gzwin/2)-5;i<((npxx-gzwin)/2)+5;i++) /* calc phases +- 10 pts more than window */
         {
        for (shim=1;shim<=nshims+1;shim++)
            {
             if (shim == 1) { /* calculate field map only */

                  j = (int)(ci[cb][shim][i]); 
                  if (ci[cb][shim][i] < 0.0)
                     {wt = -(ci[cb][shim][i] - j); q=1;}
                  else
                     {wt = ci[cb][shim][i] - j; q=-1;}

                  /* phase difference for first point */
                  alpha = sqrre[cb][shim][i-j]*sqrre2[cb][shim][i-j] + sqrim[cb][shim][i-j]*sqrim2[cb][shim][i-j];
                  beta = sqrim[cb][shim][i-j]*sqrre2[cb][shim][i-j] - sqrre[cb][shim][i-j]*sqrim2[cb][shim][i-j]; 
                  phase2 = atan2(beta,alpha); 
if (debug) fprintf(bug,"%d\t%f\t",i,phase2);

                  /* calculate unwrapping correction */
                  if (k == 1) {
                     corr[cb][shim][i] = corr[cb][shim][i-1];
                     if ((phase2 - lstpnt[cb][shim]) > phtol) corr[cb][shim][i] -= twoPi;
                     if ((phase2 - lstpnt[cb][shim]) < -phtol) corr[cb][shim][i] += twoPi;
                     lstpnt[cb][shim] = phase2; }
                  phase2 += corr[cb][shim][i-j];
if (debug) fprintf(bug,"%f\t%f\n",corr[cb][shim][i],phase2);

                  /* phase difference for second point */
                  alpha = sqrre[cb][shim][i-j+q]*sqrre2[cb][shim][i-j+q] + sqrim[cb][shim][i-j+q]*sqrim2[cb][shim][i-j+q];
                  beta = sqrim[cb][shim][i-j+q]*sqrre2[cb][shim][i-j+q] - sqrre[cb][shim][i-j+q]*sqrim2[cb][shim][i-j+q]; 
                  phase3 = atan2(beta,alpha); 
                  phase3 += corr[cb][shim][i-j+q];

                  /* calculate phase difference field - weighted average of shim phase differences */
                  phase4 = phase2*(1.0 - wt) + phase3*wt;
                  phd[cb][shim][i] = phase4;

                  amp[cb][1][i] = sqrt((sqrre[cb][1][i]*sqrre[cb][1][i] + sqrim[cb][1][i]*sqrim[cb][1][i]));
                  if (i == (npxx/4 - 5)) cormp[cb][1] = corr[cb][1][i]; /* store corr at midpoint */

                }
             else
                { /* calculate shimmaps */

                  j = (int)(ci[cb][shim][i]); 
                  if (ci[cb][shim][i] < 0.0)
                     {wt = -(ci[cb][shim][i] - j); q=1;}
                  else
                     {wt = ci[cb][shim][i] - j; q=-1;}

                  /* phase difference for first point */
                  alpha = sqrre[cb][shim][i-j]*sqrre2[cb][shim][i-j] + sqrim[cb][shim][i-j]*sqrim2[cb][shim][i-j];
                  beta = sqrim[cb][shim][i-j]*sqrre2[cb][shim][i-j] - sqrre[cb][shim][i-j]*sqrim2[cb][shim][i-j]; 
                  phase2 = atan2(beta,alpha); 
if (debug3 && shim == 2) fprintf(bug3,"%d\t%f\t",i,phase2);

                  /* calculate unwrapping correction on first cycle */
                  if (k == 1) {
                     corr2[cb][shim][i] = corr2[cb][shim][i-1];
                     if ((phase2 - lstpnt2[cb][shim]) > phtol) corr2[cb][shim][i] -= twoPi;
                     if ((phase2 - lstpnt2[cb][shim]) < -phtol) corr2[cb][shim][i] += twoPi;
                     lstpnt2[cb][shim] = phase2; }
                  phase2 += corr2[cb][shim][i-j];
if (debug3 && shim == 2) fprintf(bug3,"%f\t%f\n",corr2[cb][shim][i],phase2);

                  /* phase difference for second point */
                  alpha = sqrre[cb][shim][i-j+q]*sqrre2[cb][shim][i-j+q] + sqrim[cb][shim][i-j+q]*sqrim2[cb][shim][i-j+q];
                  beta = sqrim[cb][shim][i-j+q]*sqrre2[cb][shim][i-j+q] - sqrre[cb][shim][i-j+q]*sqrim2[cb][shim][i-j+q]; 
                  phase3 = atan2(beta,alpha); 
                  phase3 += corr2[cb][shim][i-j+q];

                  /* calculate phase difference field - weighted average of shim phase differences */
                  phase4 = phase2*(1.0 - wt) + phase3*wt;
                  phd[cb][shim][i] = phase4; /* field error not subtracted yet */

   amp[cb][shim][i] = sqrt((sqrre[cb][shim][i]*sqrre[cb][shim][i] + sqrim[cb][shim][i]*sqrim[cb][shim][i]));
                  if (i == (npxx/4 - 5)) cormp[cb][shim] = corr2[cb][shim][i];
                }

           } /* end of shim loop */
      } /* end of i loop */
    }/* end of if rbars condition */
  } /* end of cb loop */

/* ###### calculate phase index corrections, ci ###### */
/* ###### bring phd maps into correct range, i.e. subtract cormp's ###### */

  r=0;
  for (shim=1;shim<=(nshims+1);shim++)
      {
if (debug2) fprintf(bug2,"\n");
       for (cb=1;cb<=ncb;cb++)
           {
            if (rbars[cb] > 0.0) {
            for (i=(gzwin/2);i<((npxx-gzwin)/2);i++) 
                {r++;
                  if (shim == 1)
                     {phd[cb][shim][i] = (phd[cb][shim][i] - cormp[cb][shim])/tau;
                      ci[cb][shim][i] = phd[cb][shim][i]/Hzpp;

                      if (i == (npxx/4 - 5)) {
                         if (phd[cb][shim][i] > Pi/tau) barcor[cb][shim] = -2.0*Pi/tau;
                         if (phd[cb][shim][i] < -Pi/tau) barcor[cb][shim] = 2.0*Pi/tau;}
if (debug2) fprintf(bug2,"%d\t%f\t%f\n",r,phd[cb][shim][i],barcor[cb][shim]);
                     }
                  else
                     {phd[cb][shim][i] = (phd[cb][shim][i] - cormp[cb][shim])/tau;
                      ci[cb][shim][i] = phd[cb][shim][i]/Hzpp; /* calculate correction before subtract fieldmap */
                      phd[cb][shim][i] = phd[cb][shim][i]  - phd[cb][1][i];

                      if (i == (npxx/4 - 5)) {
                         if (phd[cb][shim][i] > Pi/tau) barcor[cb][shim] = -2.0*Pi/tau;
                         if (phd[cb][shim][i] < -Pi/tau) barcor[cb][shim] = 2.0*Pi/tau;}
                      }

                } /* end of i loop */
             }/* end of if rbars condition */
           } /* end of cb loop */
       } /* end of shim loop */

} /* end of cycles loop */


/* ###### Output results to shimmap file ###### */

  if (nshims == 0){ /* output field map */
     fprintf(out,"exp 4\n");
     fprintf(out,"  1  %d\n",nfieldpts);
     fprintf(out,"index\n");
     fprintf(out,"\n1  0  0  0\n");

     c=0;
     for (cb=1;cb<=ncb;cb++)
         {
          if (rbars[cb] > 0.0)
             {
              for (i=(gzwin/2);i<((npxx-gzwin)/2);i++)
                  {c++;
                   fprintf(out,"%d\t%f\n",c,phd[cb][1][i] + barcor[cb][1]);
                  }
              } 
          }
     }
  else /* output shimmaps without field map */
     {
      fprintf(out,"exp 4\n");
      fprintf(out,"  %d  %d\n",nshims,nfieldpts);
      fprintf(out,"index\n");

      fprintf(ampout2,"exp 4\n");
      fprintf(ampout2,"  %d  %d\n",nshims,nfieldpts);
      fprintf(ampout2,"shimmap amplitude\n");

      c=0;
      for (shim=2;shim<=nshims+1;shim++)
          {fprintf(out,"\n1  0  0  0\n");
           fprintf(ampout2,"\n1  0  0  0\n");
           for (cb=1;cb<=ncb;cb++){
               if (rbars[cb] > 0.0){
                  for (i=(gzwin/2);i<((npxx-gzwin)/2);i++){ c++;
                      fprintf(out,"%d\t%f\n",c,phd[cb][shim][i] + barcor[cb][shim]);
                      fprintf(ampout2,"%d\t%f\n",c,amp[cb][shim][i]);
                  } } } }
     }

/* output amplitude file */

  if (debug == 1) {
     fprintf(ampout,"exp 4\n");
     fprintf(ampout,"  1  %d\n",nfieldpts);
     fprintf(ampout,"index\n");
     fprintf(ampout,"\n1  0  0  0\n");

     cb=0; c=0;
     for (q=1;q<=nji;q++){
         for (r=1;r<=nki;r++){
             cb++;
             if (rbars[cb] > 0.0){
                for (i=(gzwin/2);i<((npxx-gzwin)/2);i++){ c++;
                    fprintf(ampout,"%d\t%f\n",c,amp[cb][1][i]);
             } } } }
  }


/* ###### final remarks ###### */

  fclose(out);
  fclose(ampout);
  fclose(ampout2);
  if (debug)  fclose(bug);
  if (debug2)  fclose(bug2);
  if (debug3)  fclose(bug3);
  releaseAllWithId("calcq");
  RETURN;

}/* ############### end of main ############## */

/* ###### memory allocation functions ###### */
double **dmatrix(int nrl,int nrh,int ncl,int nch)
{
  int i;
  double **m;

  m=(double **) allocateWithId((unsigned) (nrh-nrl+1)*sizeof(double*),"calcq");
  if(!m) nrerror("allocation failure 2 in matrix()");
  m -= nrl;

  for(i=nrl;i<=nrh;i++) {
      m[i]=(double *) allocateWithId((unsigned) (nch-ncl+1)*sizeof(double),"calcq");
      if (!m[i]) nrerror("allocation failure 2 in matrix()");
      m[i] -= ncl;
  }
  return m;
}

static double *dvector(int nl, int nh)
{
double *v;
v=(double *)allocateWithId((unsigned) (nh-nl+1)*sizeof(double),"calcq");
if (!v) nrerror("allocation failure in vector()");
return v-nl;
}
 
static void nrerror(char *error_text)
{
   Werrprintf("Program error %s\n",error_text);
   exit(1);
}

float *vector(int nl, int nh)
{
float *v;
v=(float *)allocateWithId((unsigned) (nh-nl+1)*sizeof(float),"calcq");
if (!v) nrerror("allocation failure in vector()");
return v-nl;
}

/* ###### end of memory allocation functions ###### */

/* ###### start of getspec ###### */
static int getspec(int spc)
{
   dpointers  inblock;

   if ( (Dres = D_gethead(D_DATAFILE, &fidhead)) )
   {
   	if (Dres == D_NOTOPEN)
   	{
   		Wscrprintf("Original Spectrum had to be re-opened \n");
   		strcpy(path, curexpdir);
   		strcat(path, "/datdir/data");
   		Dres = D_open(D_DATAFILE, path, &fidhead); /* open the file */
   	}

   	if (Dres)
   	{
   	   	Werrprintf("calcq : spectrum %d not found",spc);
	   	D_error(Dres);
	   	return(-1);
	}
   }

   if ( (Dres = D_getbuf(D_DATAFILE, fidhead.nblocks, spc, &inblock)) )
   {
   	D_error(Dres);
   }

   /* data is the full spectrum with re amd im points */
   data = (float *)inblock.data;

   if ( (Dres = D_release(D_DATAFILE, spc)) )
   {
        D_error(Dres);
   }
   return(0);
} /* ###### end of getspec ###### */
