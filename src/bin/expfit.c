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
/************************************************************************/
/*          								*/ 
/* expfit	-  exponential curve fitting				*/
/*		   for T1, T2 and Kinetics analysis.			*/
/*		   linear('poly1'), quadratic('poly2'), cubic('poly3'), */
/*		   and exponential('exp') for diffusion and other uses. */
/*									*/
/* This program analyzes data by fitting it to the selected curve.	*/
/* For T1, T2 and kinetics data different starting values are selected, */
/* and the resulting printout is different.				*/
/*									*/
/* The program has two outputs, to 'analyze.out' used as input by       */
/* 'expl', and to the standard output, which is put in 'analyze.list'   */
/* by 'analyze'.							*/
/*									*/	
/* The program reads the input data from standard input, which must  	*/
/* 	contain the information in the following formats:		*/
/* For regression,  exponential analysis, and cp_analysis		*/
/*	<optional descriptive text line>				*/
/*	<optional y-axis title - regression only>			*/
/*	number of data sets(peaks)    number of data pairs(spectra)
           per data set (largest number if a line starts with 'NEXT')
	   and, regression only, x scale type   y scale type		*/
/*      <NEXT	 then number of pairs in next data set>			*/
/*     index#(1)							*/
/*	x y	(first data set, first data pair)			*/
/*	x y	(first line, second data pair)				*/
/*	....								*/
/*      <NEXT	 then number of pairs in next data set>			*/
/*     index#(2)							*/
/*	x,y	(second line, first spectrum)				*/
/*									*/
/* For  diffusion (including poly1 and poly2)  				*/
/* 	List of n x-y data pairs	("%s %s %d %s %s %s")		*/
/* 	A single line of descriptive text				*/
/* 	X-values	Y-values	(Two Strings)			*/
/* 	blank line							*/
/*	x y	(first line, first spectrum)				*/
/*	....								*/
/* The program can be called with the following arguments (options):	*/
/*   T1		perform T1 analysis (default)				*/
/*   T2		perform T2 analysis					*/
/*   kinetics	perform kinetics (decreasing) analysis			*/
/*   increment  perform kinetics (increasing) analysis			*/
/*   list	extended listing					*/
/*   regression generalized curve fitting				*/
/*   poly1	linear fitting						*/
/*   poly2	quadratic fitting					*/
/*   poly3	cubic fitting						*/
/*   exp  	exponential fitting (regression default)		*/
/*   diffusion  diffusion coefficients of a mixture of two compounds    */
/*		diffusion must be followed by 4 floating point constants*/
/*   contact_time   an MAS cross-polarization, spin-lock contact time  	*/
/*		measurement with 4 coefficients - P. E. M. Allen et al.	*/

/* Nn is set to 4, 3, or 2 depending upon number of constants in curve. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define	NO	4
#define MAXITERATIONS 64
#define MAXSETS       128
#define T1T2          0 
#define KINI          1      /* kinetics increasing */
#define KIND          2 
#define DIFFUSION     3 
			     /* 4 used for linking points */
#define POLY1         5 
#define POLY2         6 
#define EXPONENTIAL   7 
#define CP_ANALYSIS   8      /* Cross-polarization spin-lock contact time */
#define POLY3         9
#define POLY0	      10
#define NOTITLE       "No Title"
#define LARGE         1e+50 
#define SMALL         1e-25 
#define SMALLER       1e-50 
#define FALSE	      0
#define TRUE	      1


int numlines,numspec,maxnumspec,nspecused;
int diffusion,t1flag,t2flag,kineticflag,incrflag,listflag,corrflag;
int NEXTflag = FALSE;
int poly0=0, poly1=0, poly2=0, poly3=0;
float *xa[MAXSETS],
      *ya[MAXSETS];
float *x,*y,*aa0,*aa2,*aa3,*intercept,*tau,*sigma,*stddev;
float *err0, *err2, *err3;
float yave;
double offset;
int *converge,*nosolution;
double a[NO],c[NO],d[NO],err[NO],o[NO][NO];
double max1,max2,e1,e2,e3,s,s1,a0,a1,a2,f1,f2,v;
double a3=0.0;
double D1,C0,C1,C2;
char s0[128],sy[128];
char ss1[32],ss2[32];
char xaxis[32],yaxis[32];
int Nn = 3;
int exptype    = 0;
int regression = 0;
int cp_analysis= 0;
int polyflag   = 0;
int line_index[MAXSETS];
int spec_count[MAXSETS];

static void freemem()
{     int i;
      for (i=0; i<numlines; i++)
      {
        if (xa[i]) free(xa[i]);
        if (ya[i]) free(ya[i]);
      }
      if (x) free(x);
      if (y) free(y);
      /* if (flag) free(flag); */
      if (intercept) free(intercept);
      if (aa0) free(aa0);
      if (aa2) free(aa2);
      if (aa3) free(aa3);
      if (stddev) free(stddev);
      if (err0) free(err0);
      if (err2) free(err2);
      if (err3) free(err3);
      if (sigma) free(sigma);
      if (converge) free(converge);
      if (nosolution) free(nosolution);
}
/*------------------------------------------------------------------------------
|
|	stringReal/1
|
|	This function is used to convert a string to a real.
|
+-----------------------------------------------------------------------------*/

double stringReal(s)			char *s;
{   double d;

    if (sscanf(s,"%lf",&d) == 1)
	return(d);
    else
    {	fprintf(stderr,"magic: can't convert \"%s\" to a real value, zero assummed\n",s);
	return(0.0);
    }
}

/*------------------------------------------------------------------------------
|
|	isReal/1
|
|	This function returns true (non-zero) if the given string can be
|	converted to a real.  It returns false (zero) otherwise.
|
+-----------------------------------------------------------------------------*/

int isReal(s)				char *s;
{   double tmp;

    if (sscanf(s,"%lf",&tmp) == 1)
	return(1);
    else
	return(0);
}

/*************/
double msubt(t)
/*************/
double t;
{ double uu;

  if (exptype == KINI)
    return (-a0*exp(-t/a1)+a2+a0);
  else if (exptype == KIND || exptype == EXPONENTIAL)
    return (a0*exp(-t/a1)+a2);
  else if (exptype == POLY0)
    return (a0);
  else if (exptype == POLY1)
    return (a0 + a1*t);
  else if (exptype == POLY2)
    return (a0 + a1*t +a2*t*t);
  else if (exptype == POLY3)
    return (a0 + a1*t +a2*t*t + a3*t*t*t);
  else if (exptype == CP_ANALYSIS)
    return ( ((a3 - (a3-a0)*exp(-t/a1)) * exp(-t/a2)) + a0);
  else if (exptype == DIFFUSION)
    /*return (a0 + a2*t +a1*a1*t*t);*/
    {
    uu = C0 + C1*t + C2*t*t;
    if (Nn==1) return (a0*a0*t*t);
    /* else return (a0*a0*t*t + a0*a1*t +a1*a1);*/
    else return ( a0*exp(-D1*uu) + a2*exp(-a1*D1*uu));
    }
  else
    return (a0-a2)*exp(-t/a1)+a2;
}

/*****************/
int prepare_workspace()
/*****************/
{
  x    = (float *)malloc(sizeof(float) * maxnumspec);
  y    = (float *)malloc(sizeof(float) * maxnumspec);
  /* flag = (int *)malloc(sizeof(int) * maxnumspec); */
  intercept = (float *)malloc(sizeof(float) * numlines);
  aa0       = (float *)malloc(sizeof(float) * numlines);
  tau       = (float *)malloc(sizeof(float) * numlines);
  aa2       = (float *)malloc(sizeof(float) * numlines);
  aa3       = (float *)malloc(sizeof(float) * numlines);
  sigma     = (float *)malloc(sizeof(float) * numlines);
  stddev    = (float *)malloc(sizeof(float) * numlines);
  err0      = (float *)malloc(sizeof(float) * numlines);
  err2      = (float *)malloc(sizeof(float) * numlines);
  err3      = (float *)malloc(sizeof(float) * numlines);
  converge   = (int *)malloc(sizeof(int) * numlines);
  nosolution = (int *)malloc(sizeof(int) * numlines);

  if ((x==0)||(y==0)||(aa0==0)||(aa2==0)||
   (!intercept)||(!tau)||(!sigma)||(!converge)||(!nosolution)||
    !stddev || !err0 || !err2 || !err3)
    { printf("expfit: cannot allocate buffer memory\n");
      return 1;
    }
  return 0;
}

/**********/
int readparams()
/**********/
{ int cntr,i,j,l;
  char c;
  char s2[128];
  char stemp[20];
  int   temp;

  s0[0]='\0';
  sy[0]='\0';
  l = 1;
  if (!regression && polyflag && exptype != CP_ANALYSIS)
    { if (scanf("%s %s %d %s %s %s\n",s2,s2,&numspec,s2,s2,s2)!=6)
        {      /*List of n x-y data pairs*/
          printf("expfit: problem on line %d of input\n",l);
          return 1;
        }
      numlines=1;
      cntr=0;
      while  ((c = getchar()) != '\n') 
        { s0[cntr]=c;
          cntr++;
        }
      s0[cntr]='\0';
      l++;
      scanf("%s %s\n",ss1,ss2); /*line with heading 'X-values  Y-values'*/
      l++;
      /* printf("s0=%s\n",s0); */
    }
  else if (regression) 
  {
     if (scanf("%d",&numlines))
     {    if (scanf("%d %s %s\n",&numspec,xaxis, yaxis) != 3)
          { printf("expfit: problem on line %d of input\n",1);
            return 1;
          }
	  strcpy(s0,NOTITLE);
	  strcpy(sy,NOTITLE);
     }
     else
     {
          scanf("%[^\n]\n", s0);
	  l++;
          if (scanf("%d",&numlines))
          {    if (scanf("%d %s %s\n",&numspec,xaxis, yaxis) != 3)
               { printf("expfit: problem on line %d of input\n",1);
                 return 1;
               }
	       strcpy(sy,NOTITLE);
          }
	  else
	  {
              scanf("%[^\n]\n", sy);
	      l++;
              if (scanf("%d %d %s %s\n",&numlines,&numspec,xaxis,yaxis)!=4) 
              {   printf("expfit: problem on line %d of input\n",l);
                  return 1;
              }
          }
     }
  }
  else
     if (scanf("%d %d\n",&numlines,&numspec)!=2) 
     {   printf("expfit: problem on line %d of input\n",l);
         return 1;
     }
  maxnumspec = numspec;
  l++;
  for (i=0; i<numlines; i++)
    { 
       xa[i] = (float *)malloc(sizeof(float) * maxnumspec);
       ya[i] = (float *)malloc(sizeof(float) * maxnumspec);
       if ((xa[i]==0)||(ya[i]==0))
       { printf("expfit: cannot allocate buffer memory\n");
         return 1;
       }
      spec_count[i] = numspec;
      NEXTflag = FALSE;;
      if (regression)
      {
        if (scanf("%d",&temp) == 1)
	    line_index[i]=temp;
        else
	  { scanf("%s %d\n",stemp,&spec_count[i]);
	    scanf("%d",&line_index[i]);
	    NEXTflag = TRUE;
	  }
        numspec = spec_count[i];
      }
      else if (!polyflag || exptype == CP_ANALYSIS)
	scanf("%d",&line_index[i]);
      else line_index[i] = i+1;
      if (scanf("\n")) printf("##numlines=%d l=%d\n",numlines,l);
      l++;
      for (j=0; j<numspec; j++)
        { if (scanf("%f %f\n",&xa[i][j],&ya[i][j])!=2)
            { 
  	      printf("expfit: problem on line %d of input\n",l);
              return 1;
            }
          l++;
        }
    }
  if (kineticflag)
    for (i=0; i<numlines; i++)
    {    
      xa[i][0] += offset;
      for (j=1; j<numspec; j++)
        xa[i][j] +=xa[i][j-1] + offset;
    }
  return 0;
}

/***********/
int writeparams()
/***********/
{ int i;
  if (polyflag) 
    {
      printf("peak    std deviation of fit");
      if (regression)
      {
        if (poly0) printf("   mean");
	else printf("     a0            a1");
        if (poly2 || poly3) printf("            a2"); 
        if (         poly3) printf("           a3"); 
      }
    }
  else if (kineticflag)
    printf("peak             tau         error");
  else if (t2flag)
    printf("peak             T2          error");
  else
    {
      printf("peak             T1          error");
      if (regression && exptype==EXPONENTIAL) printf("        a0          a2");
    }
  printf("\n");
  for (i=0; i<numlines; i++)
  {
    if (!polyflag && fabs(sigma[i]) > fabs(tau[i])) nosolution[i]=1;
    if (nosolution[i])
      printf("%4d %16.4g %12.4g  solution not found\n",
	line_index[i],tau[i],sigma[i]);
    else
      if (converge[i])
      {
	if (poly0)
        {
          printf("%4d %16.4g %12.4g\n",line_index[i],stddev[i],aa0[i]);
	}
        else if  (!polyflag)
        {
          printf("%4d %16.4g %12.4g",line_index[i],tau[i],sigma[i]);
          if (regression) printf("%12.4g %11.4g",aa0[i],aa2[i]);
          printf("\n");
        }
	else
	{
          printf("%4d %16.4g",line_index[i],stddev[i]);
	  if (regression)
          {
	    printf(" %12.4g",aa0[i]);
	    if (poly1 || poly2 || poly3) printf("  %12.4g",tau[i]);
	    if (!poly1) printf("  %12.4g",aa2[i]);
	    if (poly3) printf(" %12.4g",aa3[i]);
	  }
	  printf("\n");
	}	
      }
      else
      {
        if  (!polyflag)
          printf("%4d %16.4g %12.4g  stopped at %d iterations\n",
	    line_index[i],tau[i],sigma[i],MAXITERATIONS);
	else
          printf("%4d %16.4g          stopped at %d iterations\n",
	    line_index[i],stddev[i],MAXITERATIONS);
      }
  }
  return(1);
}

/**************/
void printpeakdata(int k, int numspecs)
/**************/
{ int i;
  double corr = 0.0;
  if (corrflag) corr = 0.03;
  /* if  (!polyflag) */
	printf("\npeak number %d\n",line_index[k]);

  if (poly0)
      {  printf("Mean =%8.4g  Standard Deviation =%8.4g\n",aa0[k],stddev[k]);
      }
  else if (poly1)
      {  printf("Coefficients:        a0= %10.3g  a1= %13.6g\n",aa0[k],tau[k]);
         printf("Standard deviation:      %10.3g      %13.6g\n",err0[k],sigma[k]);
      }
  else if (diffusion || poly2)
      {  printf("Coefficients:        a0= %10.3g  a1= %13.6g  a2= %10g\n",
		aa0[k],tau[k],aa2[k]);
         printf("Standard deviation:      %10.3g      %13.6g      %10g\n",
		err0[k],sigma[k],err2[k]);
      }
  else if (kineticflag)
        printf(" tau = %12.3g    ",tau[k]);
  else if (t2flag)
        printf(" T2 = %12.3g     ",tau[k]);
  else if (cp_analysis)
	  printf("\n\n   Sinf=%.1f  S0=%.1f  Tch=%.5f  T1rho=%.3f\n\n",
		aa0[k],aa3[k], tau[k], aa2[k]);
  else if (poly3)
	  printf("\n     a0=%.4g   a1=%.4g   a2=%.4g   a3=%.4g\n",aa0[k],
		tau[k], aa2[k],aa3[k]);
  else if (poly2)
	  printf("\n     a0=%.4g   a1=%.4g   a2=%.4g   \n",aa0[k],
		tau[k], aa2[k]);
  else
        printf(" T1 = %12.3g     ",tau[k]);
  if (!diffusion && !polyflag) 
	printf(" error = %12.3g",sigma[k]);
  printf("\n");

  if (nosolution[k])
      printf("solution not found\n");
  else if (!converge[k])
      printf("results after %d iterations\n",MAXITERATIONS);
     
  if (polyflag)
  printf("%11s %12s   calculated   difference\n",ss1,ss2);
  else printf("      time       observed   calculated   difference\n");
  for (i=0; i<spec_count[k]; i++)
    { x[i] = xa[k][i] + corr;
      y[i] = ya[k][i];
    }
  for (i=0; i<numspecs; i++)
    { 
      a0 = aa0[k];
      a1 = tau[k];
      a2 = aa2[k];
      a3 = aa3[k];
      if (1) /*(converge[k])*/
        printf("%12.4g %12.3g %12.3g %12.3g\n",
          x[i],y[i],msubt(x[i]),y[i]-msubt(x[i]));
      else
        printf("%12.3g %12.3g    ---------    --------\n",x[i],y[i]);
    }
}

/**************/
void initial_values()
/**************/
{ register int i;
  double    min,max;
  double    yt = 0.0;
  int      jmin,jmax;

  a[3] = 0.0;
  min  = 1e32;
  max  = -1e32;
  jmin = 0;
  jmax = 0;
  for (i=0; i<numspec; i++)
    { 
         if (x[i]<min)
            { min = x[i];
              jmin = i;
            }
         if (x[i]>max)
            { max = x[i];
              jmax = i;
            }
	  if (corrflag && !kineticflag) x[i] += 0.03; 
				/* correction for housekeeping delay */
        
    }
  if (cp_analysis)
    {
       a[2] = max / 2.0;
       a[1] = min * 2.0;
       yt = 0.0;
    }
  else if (diffusion)
   { 
     a[0] = y[jmin]/2.0;
     a[1] = 0.5;
     a[2] = a[0];
   }
  else if (poly1 || poly2 || poly3)
   { 
     if (fabs(max-min)<1e6)
        a[1]=0;
     else
        a[1] = (y[jmax]-y[jmin])/(max-min);
     a[0] = y[jmin] - a[1]*min;
     if (a[0]==0.0) a[0]=y[jmax]/100.0;
     a[2] = y[jmax]/max/max/2.0;
     if (poly3)
     {  a[3] = a[2]/max; 
        if (a[3] == 0.0) a[3] = 1e-8;
        a[1] /= 10.0;
        a[2] /= 10.0; 
     }
     if (a[2] == 0.0) a[2] = 1e-6; 
     if (a[1] == 0.0) a[1] = 1e-4; 
   }
  else if (kineticflag || exptype == EXPONENTIAL)
    { if (incrflag || exptype == EXPONENTIAL)
        { a[2] = y[jmax];
          yt   = 0.63 * a[2];
        }
      else
        { a[2] = y[jmin];
          yt   = 0.37 * a[2];
        }
      if (y[jmin] > y[jmax]) 
          a[0]= (y[jmin] - y[jmax]);
      else
          a[0] = -a[2];
    }
  else
    { a[0] = y[jmin];
      if (t2flag)
        a[2] = y[jmax] / 10.0;
      else
        { a[2] = y[jmax];
          if ((a[2]<0.0) && (y[jmin]<0))
            a[2] = -y[jmin];
        }
      yt = 0.37 * (a[0] - a[2]) + a[2];
    }
  max2 = 2.0 * max;
  min = 1e32;
  max = -1e32;
  jmin = 0;
  yave = 0;
  s = 0;
  for (i=0; i<numspec; i++)
    { /* if (flag[i]) */
        { if (fabs(y[i]-yt) < min)
            { min = fabs(y[i]-yt);
              jmin = i;
            }
          if (fabs(y[i]) > max)
            max  = fabs(y[i]);
 	  yave += y[i];
        }
    }
  yave /= numspec;
  if (poly0)
    {
      s = 0.0;
      for (i=0; i<numspec; i++)
      {	s += (y[i] - yave) * (y[i] -yave);
      }
      s = sqrt((double)(s/(numspec - 1)));
      a0 = yave;
      a1 = 0.0;
      a[0] = a0; 
      a[1] = a1; 
    }
  else
  {
  if (cp_analysis)
    {
       a[3] = 1.2 * max;  /* (S0)   */
       a[0] = y[jmin];    /* (Sinf) */
    }
  else if (!polyflag)
    { a[1] = x[jmin];
      if (a[1] < max2/10.0) a[1] = max2/10.0;
    }
  max1 = 10 * max;
  if (!cp_analysis) 
  {   if (a[2] == 0.0) a[2] = 1e-6; 
      if (a[0] == 0.0) a[0] = a[2]/1000;
  }
  /* if (Nn == 4) 
	 printf("a0=%10g a1=%10g a2=%10g a3=%10g\n",a[0],a[1],a[2],a[3]); */
  }
}

#ifdef DEBUG
/************/
void printmatrix(b) double b[NO][NO];
/************/
{
   int i;
   if (Nn <4) for (i=0; i<4; i++) b[i][3]=0.0;
   if (Nn <3) for (i=0; i<4; i++) b[i][2]=0.0;
   for (i=0; i<Nn; i++)
       printf("d%d %12g %12g %16g %12g\n", i,b[i][0],b[i][1],b[i][2],b[i][3]); 
}
#endif

/***************/
void invertmatrix(q,a)
/***************/
double q[NO][NO]; double a[NO][NO];
{ int lz[NO];
  double b[NO],c[NO];
  int i,j,k,l,lp,m;
  double y,w;
  for (i=0; i<Nn; i++)
    for (j=0; j<Nn; j++)
      a[i][j] = q[i][j];
  for (j=0; j<Nn; j++)
    lz[j] = j;
  for (i=0; i<Nn; i++)
    { k = i;
      y = a[i][i];
      l = i - 1;
      lp = i + 1;
      for (j=lp; j<Nn; j++)
        { w = a[i][j];
          if (fabs(w)>fabs(y))
            { k = j;
              y = w;
            }
        }
      for (j=0; j<Nn; j++)
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
      for (k=0; k<Nn; k++)
        { if (i!=k)
            for (j=0; j<Nn; j++)
              if (i!=j)
                a[k][j] -= b[j] * c[k];
        }
    }
  for (i=0; i<Nn; i++)
    { if (i!=lz[i])
        for (j=i+1; j<Nn; j++)
          { if (i==lz[j])
              { m = lz[i];
                lz[i] = lz[j];
                lz[j] = m;
                for (l=0; l<Nn; l++)
                  { c[l] = a[i][l];
                    a[i][l] = a[j][l];
                    a[j][l] = c[l];
                  }
              }
          }
    }
}

/***********/
int det(o)	double o[NO][NO];
/***********/
{ int i,j;
  double aooo,detv,diag;
  double ooo[NO][NO];

  aooo=o[0][0];
  if (SMALLER > fabs(aooo))
      return(1);
  for (i=0; i<Nn; i++) for(j=0; j<Nn; j++) ooo[i][j] = o[i][j]/aooo;
  if (Nn==2) diag = ooo[1][1];
  else 
  {   diag = ooo[1][1]*ooo[2][2];
      if (Nn==4) diag *= ooo[3][3];
  }
  if (fabs(diag) < SMALLER) 
  {
     printf("diag=%g\n",diag);
     return(1);
  }
  if (Nn==2)
  {
    if (SMALLER > fabs(diag-ooo[0][1]*ooo[1][0])) return(1);
    else return(0);
  }

  detv = diag -ooo[1][2]*ooo[2][1]
       -ooo[1][0] * (ooo[0][1]*ooo[2][2] - ooo[2][1]*ooo[0][2])
       +ooo[2][0] * (ooo[0][1]*ooo[1][2] - ooo[1][1]*ooo[0][2]);
  if (SMALL * fabs(diag) > fabs(detv)) 
  {
     printf("detv=%g\n",detv);
     return(1);
  }
  else return(0);
}
  
/********************/
int calc_params(linenumber)  int linenumber;
/********************/
{ register int i,j,k,count;
  double cc = 1.0;
  double savea1;
  double b[NO][NO];

  savea1=a[1];
  count = 0;
  e3 = 0.25;
  s1 = 0.0;
  do
    { s = 0.0;
      for (i=0; i<Nn; i++)
        { c[i] = 0;
          for (j=0; j<Nn; j++)
            b[i][j] = 0.0;
        }
      a0 = a[0]; a1 = a[1]; a2 = a[2]; a3 = a[3];
      /*if (Nn<4) a[3]=0;
         printf("%d %12f %12f %12f %12f\n",count,a[0],a[1],a[2],a[3]); */
      if (!polyflag)
      {
        if (fabs(a0) > cc*max1)
          { if (a0 > 0.0) a0 = cc*max1;
            else if (a0 < 0.0) a0 = -cc*max1;
          }
        if (fabs(a2) > cc*max1)
          { if (a2 > 0.0) a2 = cc*max1;
            else if (a2 < 0.0) a2 = -cc*max1;
          }
        if (a1<0.0)
          { if (t1flag || t2flag || exptype == EXPONENTIAL) 
            { a1=savea1/cc; cc *= 2.0;
  	      a[1]=a1;
 	    }
	    else if (fabs(a1) < max2/5.0) a1 = -max2/5.0;
	  }
        else if (a1>cc*max2) a1 = cc*max2;
      }
      else if (diffusion && (a0<0.0))
      {
         a[0] = fabs(a[2] / 10.0);
         a0 = a[0];
      }
      else if (diffusion && (a1<0.0))
      {
         a[1] = 5.0;
         a1 = a[1];
      }
      else if (diffusion && (a2<0.0))
      {
         a[2] = fabs(a[0] / 10.0);
         a2 = a[2];
      }
      else if (cp_analysis)
      {
 	 if (a2 <0.0) a2 = 1.0;
	 if (a1 <0.0) a1 = a2/100.0;
 	 else if (a1 > a2/4.0) a2 = a1/4.0;
	 if (a1 != a[1] || a2 != a[2]);
	 /* printf("a0=%10g a1=%10g a2=%10g a3=%10g\n",a0,a1,a2,a3); */
	 a[1] = a1; a2=a[2];
      }
      for (i=0; i<numspec; i++)
        { /* if (flag[i]) */
            { f1 = msubt(x[i]);
              v  = f1 - y[i];
              s += v * v;
              for (j=0; j<Nn; j++)
                { a[j] *= e2;
                  a0 = a[0]; a1 = a[1]; a2 = a[2]; a3 = a[3];
                  f2 = msubt(x[i]);
                  a[j] /= e2;
                  a0 = a[0]; a1 = a[1]; a2 = a[2]; a3 = a[3];
	 	  if (fabs(a[j]) <SMALL)
		  {
		     d[j] = (f2-f1) / (e1*SMALL);
		     if (a[j] < 0.0) d[j] = -d[j];	
		  }
                  else
		     d[j] = (f2-f1) / (e1*a[j]);
                  c[j] += v * d[j];
                  for (k=0; k<=j; k++)
                    { b[j][k] += d[j] * d[k];
                      b[k][j] = b[j][k];
                    }
                }
            }
        }
      s = sqrt((double)(s/(nspecused-Nn)));
      if (fabs(s1-s)<=0.001*s||s*s<=SMALL)
      {
        if (count > 4) return(0);
        else cc += 0.1;
      }
      count++;
      if (count>MAXITERATIONS)
	  return(1);
      if (Nn!=1) if (det(b))
	{ nosolution[linenumber] =1; 
 	  /* printmatrix(b); */
          printf("Problem With Determinant\n\n");
	  return(0);
	}
      /* if (count<4) printmatrix(b); */
      if (s1>s) e3 = 0.5;
      s1 = s;
      invertmatrix(b,o);
      for (i=0; i<Nn; i++)
        for (j=0; j<Nn; j++)
          {  
  	    a[i] -= o[i][j] * c[j] * e3;
          }
      for (i=0; i<Nn; i++)
        if (!(a[i]<LARGE && a[i] > -LARGE)) 
        {  printf("NUMBER ERROR\n");
	   return (1);
        }
      if (!polyflag)
        if ((a[1] <0.0) && (fabs(a[1]) <max2/5.0)) 
	  a[1] = -max2/5.0; 
      if (e3<1.0)
        { e3 *= 2.0;
          if (e3>1.0) e3 = 1.0;
        }
    }
    while (1);
}

/*****************/
static int error1()
/*****************/
{
    fprintf(stderr,"Four Real Arguments Must Follow 'diffusion'\n");
    exit(1);
}

/*************/
int main(argc,argv)
/*************/
int argc; char *argv[];
{ int argnumber,linenumber,cntr,index;
  int i,j;
  double val;
  FILE *fopen();
  FILE *datafile;
  char  ss[128];
  char  filename[256];
  double stringReal();

  for (i=0; i < MAXSETS; i++)
  {
     xa[i] = 0;
     ya[i] = 0;
  }
  x  = 0;
  y  = 0;
  /* flag = 0; */
  cntr = 0;

  /* options */
  t1flag = 1;
  t2flag = 0;
  kineticflag = 0;
  diffusion = 0;
  cp_analysis   = 0;
  incrflag = 0;
  listflag = 0;
  corrflag = 0;
  poly1    = 0;
  poly2    = 0;
  poly3    = 0;
  argnumber = 1;
  offset = 0.0;
  filename[0] = 0;
  /*   printf("Expfit - Jan 22, 90\n"); */

  if (argc<2) 
  {
#ifdef VMS
     printf("For usage of expfit, see [vnmr.manual]expfit\n");
#else
     printf("For usage of expfit, see /vnmr/manual/expfit\n");
#endif
     exit(1);
  }
  while (argnumber<argc)
    { if (strcmp(argv[argnumber],"T1")==0)
        { t1flag = 1; t2flag = 0; kineticflag = 0;
	  exptype = T1T2;
        }
      else if (strcmp(argv[argnumber],"T2")==0)
        { t2flag = 1; t1flag = 0; kineticflag = 0;
	  exptype = T1T2;
        }
      else if (strcmp(argv[argnumber],"kinetics")==0)
        { kineticflag = 1; t1flag = 1; t2flag = 0;
	  exptype = KIND;
        }
      else if (strcmp(argv[argnumber],"increment")==0)
        { incrflag = 1; kineticflag = 1; t1flag = 1; t2flag = 0;
	  exptype = KINI;
        }
      else if (strcmp(argv[argnumber],"diffusion")==0)
        { diffusion = 1; t1flag = 0; t2flag = 0; kineticflag =0;
	  exptype = DIFFUSION;
	  Nn=3;
          cntr++;
	  if (argc <= argnumber + 4) error1();
	  if (isReal(argv[++argnumber])) D1 = stringReal(argv[argnumber]);
	  else error1();
	  if (isReal(argv[++argnumber])) C0 = stringReal(argv[argnumber]);
	  else error1();
	  if (isReal(argv[++argnumber])) C1 = stringReal(argv[argnumber]);
	  else error1();
	  if (isReal(argv[++argnumber])) C2 = stringReal(argv[argnumber]);
	  else error1();
	}
      else if (strcmp(argv[argnumber],"poly0")==0)
        { poly0 = 1; diffusion = 0; t1flag = 0; t2flag = 0; kineticflag =0;
          exptype = POLY0;
	  Nn=1;
	}
      else if (strcmp(argv[argnumber],"poly1")==0)
        { poly1 = 1; diffusion = 0; t1flag = 0; t2flag = 0; kineticflag =0;
          exptype = POLY1;
	  Nn=2;
        }
      else if (strcmp(argv[argnumber],"poly2")==0)
        { poly2 = 1; diffusion = 0; t1flag = 0; t2flag = 0; kineticflag =0;
          exptype = POLY2;
        }
      else if (strcmp(argv[argnumber],"poly3")==0)
        { poly3 = 1; diffusion = 0; t1flag = 0; t2flag = 0; kineticflag =0;
          exptype = POLY3;
	  Nn = 4;
        }
      else if (strcmp(argv[argnumber],"exp")==0)
        { exptype = EXPONENTIAL ;
	  diffusion = 0; t1flag = 0; t2flag = 0; kineticflag =0;
        }
      else if (strcmp(argv[argnumber],"list")==0)
        { listflag = 1;
        }
      else if (strcmp(argv[argnumber],"regression")==0)
        { regression = 1;
        }
      else if (strcmp(argv[argnumber],"contact_time")==0 ||
               strcmp(argv[argnumber],"cp_analysis")==0)
        { cp_analysis = 1;
          exptype = CP_ANALYSIS;
 	  Nn = 4;
        }
      else if ((argv[argnumber][0] == 'c') ||
               (strcmp(argv[argnumber],"corr")==0))
        { corrflag = 1;
        }
      else if (argv[argnumber][0] == '/')
        { strcpy(filename,argv[argnumber]);
          strcat(filename,"/");
        }
      else if (argv[argnumber][ strlen( argv[argnumber] ) - 1 ] == ']' ||
               argv[argnumber][ strlen( argv[argnumber] ) - 1 ] == ':')
          strcpy(filename,argv[argnumber]);
      else if (kineticflag)  /* offset =(d1 + d2 + at)*nt */
           {
	     cntr++;
	     if (cntr == 1 && argc <= argnumber+3)
	       { printf("Four Real Arguments Must Follow 'kinetics' or 'increment'\n");
		 exit(1);
	       }
             val = stringReal(argv[argnumber]);
             if (cntr>=1 && cntr<=3) offset += val;
 	     else if (cntr==4) offset = val*offset;
	   }
      argnumber++;
    }
  polyflag = poly0 || poly1 || poly2 || poly3 || cp_analysis || diffusion;
  if (regression && exptype == T1T2) exptype = EXPONENTIAL;
  if (poly0) printf("Mean of Data\n");
  else
  {
  if (poly1) printf("Linear");
  else if (poly2) printf("Quadratic");
  else if (poly3) printf("Cubic");
  else if (diffusion) printf("Two component diffusion");
  else if (cp_analysis) printf("\nContact time");
  else printf("Exponential");
  printf(" data analysis:\n");
  }
  if (cp_analysis) printf("\n");

  /* printf("offset is %f   \n",offset); */
  if (readparams())
    { if (xa[0]) free(xa[0]);
      if (ya[0]) free(ya[0]);
      return(1);
    }
  if (s0[0] != '\0' && strcmp(s0,NOTITLE)) printf("%s\n",s0);
  printf("\n");
  if (prepare_workspace())
    {
      freemem();
      return(1);
    }

  /* flag array can be used to exclude certain spectra from calculation */
  /* this feature is not implemented */
  /* for (i=0; i<numspec; i++) flag[i] = 1; */
  nspecused = numspec;

  if (nspecused<=Nn)
    { printf("expfit: not enough spectra, at least %1d are needed.\n",Nn + 1);
      freemem();
      return(1);
    }
  if (numlines<1)
    { printf("expfit: no lines in input\n");
      freemem();
      return(1);
    }
  e1 = 0.005;
  e2 = 1.0 + e1;

  strcat(filename,"analyze.out");
  if ((datafile=fopen(filename,"w+"))==0)
    { printf("expfit: cannot open file %s\n",filename);
      freemem();
      return(1);
    }
  if (regression)
      fprintf(datafile,"exp %d regression\n",exptype);
  else
      fprintf(datafile,"exp %d\n",exptype);
  if (exptype==DIFFUSION) fprintf(datafile,"%g  %g  %g  %g\n",D1,C0,C1,C2);
  if (regression)
    fprintf(datafile,"%5d %5d %s %s\n",numlines,maxnumspec,xaxis,yaxis);
  else
    fprintf(datafile,"%5d %5d\n",numlines,numspec);
  if (exptype == EXPONENTIAL) strcpy(ss,"Exponential Data Analysis");
  else if (exptype == POLY0)
    {  if (regression)
         strcpy(ss,"Data with Mean");
    }
  else if (exptype == POLY1)
    {  if (regression)
         strcpy(ss,"Linear Data Analysis");
       else
  	 strcpy(ss,"Diffusion (Ln(Amplitude) vs (Square(Gradient))");  
    }
  else if (exptype == DIFFUSION || exptype == POLY2) 
    {  if (regression)
         strcpy(ss,"Quadratic Data Analysis");
       else
	 strcpy(ss,"Diffusion Measurement");        
    }
  else if (exptype == POLY3)
    {  if (regression)
         strcpy(ss,"Cubic Data Analysis");
    }
  else if (exptype == KIND || exptype==KINI) strcpy(ss,"Kinetics Measurement");
  else if (exptype == T1T2) 
    {
    if (t1flag) strcpy(ss,"T1 Exponential Data Analysis");
    else  if (t2flag) strcpy(ss,"T2 Exponential Data Analysis");
    }
  else if (exptype == CP_ANALYSIS)
    {  strcpy(ss,"Contact Time Data Analysis");
    }
  else if ((polyflag || exptype == EXPONENTIAL) && s0[0] != '\0') 
    {  strcpy(ss,s0);
       s0[0] = '\0';
    }
  if (regression && ss[0] == '\0') strcpy(ss,NOTITLE);
  if (regression || strcmp(ss,NOTITLE)) fprintf(datafile,"%s\n",ss);
  if (regression)
    {
      fprintf(datafile,"%s\n",s0);
      fprintf(datafile,"%s\n",sy);
    }

  for (linenumber=0; linenumber<numlines; linenumber++)
        { 
          fprintf(datafile,"\n");
	  if (NEXTflag)
            { numspec = spec_count[linenumber];
	      fprintf(datafile,"      NEXT   %12d\n",numspec);
	    }
	  for (i=0; i<numspec; i++)
            { x[i] = xa[linenumber][i];
              y[i] = ya[linenumber][i];
            }
          initial_values();
          nosolution[linenumber] = 0;
          if (!poly0 && calc_params(linenumber))
            {
              converge[linenumber] = 0;
            }
           else 
              converge[linenumber] = 1;
	 
 	  if (Nn<3) 
	    { a2=0; err[2]=0;
	    }
	  index = line_index[linenumber];
          if (index == 0) index = linenumber + 1;
	  stddev[linenumber] = s;
 	  if (poly0) 
            {  a1 = stddev[linenumber];
 	       err[1] = 0;
	    }
          fprintf(datafile,"%d %12g %12g %12g",index,a0,a1,a2);
	  if (cp_analysis || poly3) fprintf(datafile," %12g",a3);
 	  fprintf(datafile,"\n");
	  /* s is the standard deviation of the y values, from calc_params */
	  if (cp_analysis) sigma[linenumber] = s;
          else 
 	    if (!poly0) sigma[linenumber] = s * sqrt(fabs(o[1][1]));

	  if (!poly0) for (i=0; i<Nn; i++) err[i] = s * sqrt(fabs(o[i][i]));

	      err0[linenumber] = err[0];
	      err2[linenumber] = err[2];
	  if (Nn == 4)
	      err3[linenumber] = err[3];

	  aa0[linenumber] = a0;
          tau[linenumber] = a1;
	  aa2[linenumber] = a2;
	  if (poly3 || cp_analysis)
	    aa3[linenumber] = a3;
          if (incrflag)
            intercept[linenumber] = a0 + a2;
          else if (kineticflag)
            intercept[linenumber] = a2;
          else
            intercept[linenumber] = a0;
          if (Nn > 1 && numlines < 3) 
	  if ((!listflag) && (diffusion || poly1 || poly2))
	    { for (i=0; i<Nn; i++)
	      { if (i==0) printf("Correlation Table:\n");
		printf("%d",i);
	 	for (j=0; j<i;j++)
		  printf("%10.4f",o[i][j]/sqrt(o[i][i] * o[j][j]));
		printf("%10.4f\n",1.0);
 	      }
  	      printf("\n");
	    }
         
          for (j=0; j<numspec; j++)
            fprintf(datafile,"%12g %12g\n",x[j],y[j]);
        }
  fclose(datafile);

  if (writeparams())
    {
      if (listflag)
        for (linenumber=0; linenumber<numlines; linenumber++)
          printpeakdata(linenumber,spec_count[linenumber]);
      freemem();
      return(1);
    }
  freemem();
  return(0);
}
