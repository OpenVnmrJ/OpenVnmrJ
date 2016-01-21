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
/* static char SCCSid[] = "@(#)imfit.c 9.1 4/16/93  (C)1991 Spectroscopy Imaging Systems"; */

/* IMFIT.C.C */

/******************************************************************************
   A. Rath, 6/8/92  
******************************************************************************/

#include <sys/file.h>
#include <sys/errno.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#define  BLOCKDIM         1024
#define	 NO               3
#define  MAXITERATIONS    8
#define  TAU      0
#define  MZERO    1
#define  SIGMA    2

#include "data.h" 

struct datafilehead main_header;

struct datablockhead block_header;

double  expfit();
double  logfit();
double  msubt();
int     N, fitflag;

float x[16],y[16],data[16][BLOCKDIM],sigma;
int nosolution;
float a[NO],b[NO][NO],c[NO],d[NO],o[NO][NO];
float max1,max2,e1,e2,e3,s,s1,a0,a1,a2,f1,f2,v;
int nparams = 3;

main(argc, argv)
int   argc;
char  *argv[];
{
    int     dim1, dim2, i, j, infile[16], outfile[3], block;
    float   phffit[BLOCKDIM], phfm0[BLOCKDIM], phfsigma[BLOCKDIM];
    double  minthresh;
    char    basename[128], string[128];
    char    phffit_name[128], phfm0_name[128], phfsigma_name[128];
    char    fittype[32];

    /* Usage: 
    imfit  t1/t2  basephfname  minthresh  time1  time2  ...  timeN
    */

    /****
    * Get fit type, base phasefile name, and arrayed times from command
    * line arguments:
    ****/
    strcpy(fittype, argv[1]);
    strcpy(basename, argv[2]);
    minthresh = atof(argv[3]);

    if (!strcmp(fittype, "T1")  ||  !strcmp(fittype, "t1"))
	fitflag = 0;
    else if (!strcmp(fittype, "T2")  ||  !strcmp(fittype, "t2"))
	fitflag = 1;

    if (argc < 8) {
	printf("Less than 4 time arguments in call to program \"imfit\"\n");
	exit(1);
    }
    if (argc > 20) {
	printf("More than 16 time arguments in call to program \"imfit\"\n");
	exit(1);
    }
    N = argc - 4;
    for (i=0; i<N; i++) {
	x[i] = atof(argv[i+4]);
    }
    /****
    * Open input files:
    ****/
    for (i=0; i<N; i++) {
	sprintf(string, "%s%d", basename, i+1);
        infile[i] = open(string, O_RDONLY);
        if (infile[i] < 0) {
            perror("Open");
            printf("Can't open input phasefile %s.\n", string);
            exit(1);
        }
    }

    /****
    * Read main_headers and block_headers.
    ****/
    for (i=0; i<N; i++) {
        if (read(infile[i], &main_header, 32) != 32) {
            perror ("read");
            exit(1);
        }
        if (read(infile[i], &block_header, 28) != 28) {
            perror ("read");
            exit(1);
        }
    }
    dim1 = main_header.ntraces;
    dim2 = main_header.np;

    /****
    * Open output phasefiles.
    ****/
    strcpy(phffit_name, basename);
    strcpy(phfm0_name, basename);
    strcpy(phfsigma_name, basename);
    strcat(phffit_name, fittype);
    strcat(phfm0_name, "m0");
    strcat(phfsigma_name, "sigma");
    outfile[TAU] = open(phffit_name, O_CREAT | O_TRUNC | O_WRONLY, 0666);
    outfile[MZERO] = open(phfm0_name, O_CREAT | O_TRUNC | O_WRONLY, 0666);
    outfile[SIGMA] = open(phfsigma_name, O_CREAT | O_TRUNC | O_WRONLY, 0666);
    if (outfile[TAU] < 0  ||  outfile[MZERO] < 0  ||  outfile[SIGMA] < 0) {
        perror("open output file:");
        exit(1);
    }

    /****
    * Write out the headers.
    ****/
    write(outfile[TAU], &main_header, 32);
    write(outfile[TAU], &block_header, 28);
    write(outfile[MZERO], &main_header, 32);
    write(outfile[MZERO], &block_header, 28);
    write(outfile[SIGMA], &main_header, 32);
    write(outfile[SIGMA], &block_header, 28);

    /****
    * Read in blocks of data from each phasefile and fit each pixel
    * with 3-parameter non-linear least squares fit to the equation
    * S = A + B*exp(alpha*t).
    ****/
    for (block=0; block<dim1*dim2/BLOCKDIM; block++){
        for (i=0; i<N; i++) {
            if (read(infile[i], data[i], 4*BLOCKDIM) != 4*BLOCKDIM) {
                perror ("read");
                exit(1);
            }
        }
        /****
        * Perform the 3-parameter fit.
        ****/
	for (i=0; i<BLOCKDIM; i++) {
	    for (j=0; j<16; j++) {
		y[j] = data[j][i];
	    }
	    if (fabs(y[0]) > minthresh  ||  fabs(y[N]) > minthresh) {
		expfit();
	    }
	    else {
		nosolution = 1;
	    }

	    if (nosolution  ||  sigma>a1) {
		phffit[i] = 1e-6;
		phfm0[i] = 1e-6;
		phfsigma[i] = 1e-6;
	    }
	    else {
		if (logfit(a2) > 0.80) {
	            phffit[i] = a1;
		    phfm0[i] = msubt(0.0);
		    phfsigma[i] = sigma;
		}
		else {
		    phffit[i] = 1e-6;
		    phfm0[i] = 1e-6;
		    phfsigma[i] = 1e-6;
		}
	    }
	}

        /****
        * Write out the resulting time constant and M0 data.
        ****/
        write(outfile[TAU], phffit, 4*BLOCKDIM);
        write(outfile[MZERO], phfm0, 4*BLOCKDIM);
        write(outfile[SIGMA], phfsigma, 4*BLOCKDIM);
    }
    printf("\n");

    /****
    * Finished with data, so close everything up cleanly.
    ****/
    for (i=0; i<N; i++) {
        close(infile[i]);
    }
    close(outfile[TAU]);
    close(outfile[MZERO]);
    close(outfile[SIGMA]);
}


/*************/
double msubt(t)
/*************/
double t;
{ 
    return (a0-a2)*exp(-t/a1)+a2;
}


/**************/
initial_values()
/**************/
{ register int i;
  float min,max,yt;
  int jmin,jmax;
  min  = 1e32;
  max  = -1e32;
  jmin = 0;
  jmax = 0;
  for (i=0; i<N; i++) {
      min = x[i] < min ? x[i] : min;
      jmin = x[i] < min ? i : jmin;
      max = x[i] > max ? x[i] : max;
      jmax = x[i] > max ? i : jmax;
        }

     a[0] = y[jmin];
      if (fitflag)
        a[2] = y[jmax] / 10.0;
      else
        { a[2] = y[jmax];
          if ((a[2]<0.0) && (y[jmin]<0))
            a[2] = -y[jmin];
        }
      yt = 0.37 * (a[0] - a[2]) + a[2];
  max2 = 2.0 * max;
  min = 1e32;
  max = -1e32;
  jmin = 0;
  for (i=0; i<N; i++)
        { if (fabs(y[i]-yt) < min)
            { min = fabs(y[i]-yt);
              jmin = i;
            }
          if (fabs(y[i]) > max)
            max  = fabs(y[i]);
        }
     a[1] = x[jmin];
      if (a[1] < max2/10.0) a[1] = max2/10.0;
  max1 = 10 * max;
}

/***************/
invertmatrix(q,a)
/***************/
float q[NO][NO]; float a[NO][NO];
{ int lz[NO];
  float b[NO],c[NO];
  int i,j,k,l,lp,m;
  float y,w;
  for (i=0; i<nparams; i++)
    for (j=0; j<nparams; j++)
      a[i][j] = q[i][j];
  for (j=0; j<nparams; j++)
    lz[j] = j;
  for (i=0; i<nparams; i++)
    { k = i;
      y = a[i][i];
      l = i - 1;
      lp = i + 1;
      for (j=lp; j<nparams; j++)
        { w = a[i][j];
          if (fabs(w)>fabs(y))
            { k = j;
              y = w;
            }
        }
      for (j=0; j<nparams; j++)
        { c[j] = a[j][k];
          a[j][k] = a[j][i];
          a[j][i] = -c[j]/y;
          a[i][j] /= y;
          b[j] = a[i][j];
        }
      a[i][i] = 1.0/y;
      j = lz[i];
      lz[i] = lz[k];
      lz[k] = j;
      for (k=0; k<nparams; k++)
        { if (i!=k)
            for (j=0; j<nparams; j++)
              if (i!=j)
                a[k][j] -= b[j] * c[k];
        }
    }
  for (i=0; i<nparams; i++)
    { if (i!=lz[i])
        for (j=i+1; j<nparams; j++)
          { if (i==lz[j])
              { m = lz[i];
                lz[i] = lz[j];
                lz[j] = m;
                for (l=0; l<nparams; l++)
                  { c[l] = a[i][l];
                    a[i][l] = a[j][l];
                    a[j][l] = c[l];
                  }
              }
          }
    }
}

/***********/
det(o)	float o[NO][NO];
/***********/
{ int i,j;
  float aooo,detv,diag,adiag;
  float ooo[NO][NO];

  aooo=o[0][0];
  if (1e-12 > fabs(aooo))
      return(1);
  for (i=0; i<nparams; i++) for(j=0; j<nparams; j++) ooo[i][j] = o[i][j]/aooo;
  if (nparams==2) diag = ooo[1][1];
  else diag = ooo[1][1]*ooo[2][2];
  if (1e-12 > fabs(diag)) 
     return(1);
  if (nparams==2)
    if (1e-7 > fabs(diag-ooo[0][1]*ooo[1][0])) return(1);
    else return(0);

  detv = diag -ooo[1][2]*ooo[2][1]
       -ooo[1][0] * (ooo[0][1]*ooo[2][2] - ooo[2][1]*ooo[0][2])
       +ooo[2][0] * (ooo[0][1]*ooo[1][2] - ooo[1][1]*ooo[0][2]);
  if (0.5e-6 * fabs(diag) > fabs(detv)) 
      return(1);
  else return(0);
}
  
/********************/
calc_params()
/********************/
{ register int i,j,k,count;
  float cc = 1.0;
  float savea1;

  savea1=a[1];
  count = 0;
  e3 = 0.25;
  s1 = 0.0;
  do
    { s = 0.0;
      for (i=0; i<nparams; i++)
        { c[i] = 0;
          for (j=0; j<nparams; j++)
            b[i][j] = 0;
        }
      a0 = a[0]; a1 = a[1]; a2 = a[2];
      if (count <0) printf("%d %12f %12f %12f\n",count,a[0],a[1],a[2]); 
        if (fabs(a0) > cc*max1)
          { if (a0 > 0.0) a0 = cc*max1;
            else if (a0 < 0.0) a0 = -cc*max1;
          }
        if (fabs(a2) > cc*max1)
          { if (a2 > 0.0) a2 = cc*max1;
            else if (a2 < 0.0) a2 = -cc*max1;
          }
        if (a1<0.0)
          { a1=savea1/cc; cc *= 2.0; a[1]=a1;
	  }
        else if (a1>cc*max2) a1 = cc*max2;
      for (i=0; i<N; i++)
        { 
             f1 = msubt(x[i]);
              v  = f1 - y[i];
              s += v * v;
              for (j=0; j<nparams; j++)
                { a[j] *= e2;
                  a0 = a[0]; a1 = a[1]; a2 = a[2];
                  f2 = msubt(x[i]);
                  a[j] /= e2;
                  a0 = a[0]; a1 = a[1]; a2 = a[2];
                  d[j] = (f2-f1) / (e1*a[j]);
                  c[j] += v * d[j];
                  for (k=0; k<=j; k++)
                    { b[j][k] += d[j] * d[k];
                      b[k][j] = b[j][k];
                    }
                }
        }
      s = sqrt(s/(double)(N-nparams));
      if (fabs(s1-s)<0.001*s)
        if (count > MAXITERATIONS) return(0);
        else cc += 0.1;
      count++;
      if (count>MAXITERATIONS) {
	  return 1;
      }
      if (nparams!=1) if (det(b))
	{ nosolution =1; 
          /*printf("Problem With Determinant\n\n");*/
	  return(0);
	}
      if (s1>s) e3 = 0.5;
      s1 = s;
      invertmatrix(b,o);
      for (i=0; i<nparams; i++)
        for (j=0; j<nparams; j++)
          { a[i] -= o[i][j] * c[j] * e3;
          }
        if ((a[1] <0.0) && (fabs(a[1]) <max2/5.0)) 
	  a[1] = -max2/5.0; 
      if (e3<1.0)
        { e3 *= 2.0;
          if (e3>1.0) e3 = 1.0;
        }
    }
    while (1);
}

double  expfit()
{
  int i,j;

  e1 = 0.005;
  e2 = 1.0 + e1;

          nosolution = 0;
          initial_values();
          if (calc_params())
              sigma = s * sqrt(fabs(o[1][1]));
          else
	      sigma = 100.0;
}


double logfit(m0)
  double  m0;
{
    int i,j,n;
    double   sumx=0.0, sumy=0.0, sumxy=0.0, sumx2=0.0, sumy2=0.0;
    double   ln_pixel, num, den1, den2, correlation;

    n = 0;
    for (j=0; j<N; j++) {
        if ((y[j] - m0)/(y[0] - m0) > 0.1) {
            ln_pixel = log(fabs(y[j] - m0));
            sumx += x[j];
            sumy += ln_pixel;
            sumxy += ln_pixel*x[j];
            sumx2 += x[j]*x[j];
            sumy2 += ln_pixel*ln_pixel;
            n++;
        }
    }
    num = (double)n*sumxy - sumx*sumy;
    den1 = (double)n*sumx2 - sumx*sumx;
    den2 = (double)n*sumy2 - sumy*sumy;
    correlation = fabs(num/sqrt(den1*den2));
    return (fabs(correlation));
}
