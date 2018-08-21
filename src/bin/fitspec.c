/* 
* Varian Assoc.,Inc. All Rights Reserved.
* This software contains proprietary and confidential
* information of Varian Assoc., Inc. and its contributors.
* Use, disclosure and reproduction is prohibited without
* prior consent.
*/

/* fitspec.c    - fit nmr lines to gaussian and lorentzian lines */

/* Input files are "fitspec.data" and "fitspec.inpar".          */
/* Output files are "fidspec.stat" and "fitspec.outpar".	*/
/* All files are text files for compatibility reasons.          */

/* The fit always includes a linear drift correction.           */

/* Format of the input file "fitspec.data":			*/
/*   number of points in data file 				*/
/*   sp (frequency of last point in data file)			*/
/*   wp (frequency difference between end points in data file)  */
/*   intensity of point 1					*/
/*   intensity of point 2					*/
/*   .....							*/

/* Format of the input file "fitspec.inpar":                    */
/*   for each line to fit:					*/
/*   frequency,intensity,width,gaussian fraction		*/
/*   only the frequency is required, defaults for the other	*/
/*   parameters are as follows:					*/
/*   intensity=1.0,width=1.0,gaussian fraction=0.0		*/
/*   a start immediately after any parameter keeps this         */
/*   parameter constant, rather than iterating on it.		*/

/* Original version Rene Richarz 10-4-86 for IBM PC.		*/
/* Fixed 9-22-87 to make less agressive changes			*/
/* 10-12-91 fixed fgauss_lorentz (see there), r.kyburz          */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define MAX_DATA        1024*128
#define MAX_PARAMS      102
#define MAXPATHL        128

static int      debug = 0;
static float    glatry[MAX_PARAMS],
                glbeta[MAX_PARAMS];
static float    covar[MAX_PARAMS][MAX_PARAMS];
static float    alpha[MAX_PARAMS][MAX_PARAMS];
static float    oneda[MAX_PARAMS][MAX_PARAMS];
static char     do_par[MAX_PARAMS];
static double   glochisq;
static int      error,
                count1,
                count2;
static float    sp;
char            path[MAXPATHL];

/******************************************************************/
mrqmin(x, y, sig, ndata, a, ma, lista, mfit, covar, alpha, chisq, funcs, alamda)
/******************************************************************/
/* Levenberg-Marquart method curve fitting                      */
/* See Numerical Recipes, The Art of Scientific Computing,      */
/* Cambridge University Press, Page 526.                        */

float           x[],
                y[],
                sig[];
int             ndata;		/* input data */
float           a[];
int             ma;		/* coefficients */
float           (*funcs) ();	/* function */
int             lista[MAX_PARAMS];
int             mfit;		/* coefficients to fit */
double         *chisq;		/* returned chi square */
float           covar[MAX_PARAMS][MAX_PARAMS];	/* workspace */
float           alpha[MAX_PARAMS][MAX_PARAMS];	/* workspace */
float          *alamda;		/* control */

{
   return 0;
}

/**********************************************************/
mrqcof(x, y, sig, ndata, a, ma, lista, mfit, alpha, beta, chisq, funcs)
/**********************************************************/
float           x[],
                y[],
                sig[];
int             ndata;
float           a[];
int             ma;
int             lista[MAX_PARAMS];
int             mfit;
float           alpha[MAX_PARAMS][MAX_PARAMS];
float           beta[];
double         *chisq;
int             (*funcs) ();

{
}

/*************/
gaussj(a, n, b, m)
/*************/
/* Gauss Iordan Elimination */
float           a[MAX_PARAMS][MAX_PARAMS];
int             n;
float           b[MAX_PARAMS][MAX_PARAMS];
int             m;

{
   return 0;
}

/**************************/
fgauss_lorentz(x, a, y, dyda, na)
/**************************/
/* calculate y at point x for a sum of lorentzian and gaussian functions  */
/* and a linear drift correction, as well as derivative of this function */
/* against all parameters.						 */
/* corrected for proper half-height width calculation, according to
        suggestions made by Frank Heatley, University of Manchester;
        r.kyburz, 91-10-12 */
double          x;
float           a[];
float          *y;
float           dyda[];
int             na;

{
   register int    i,
                   ii;
   register float  fac,
                   ex,
                   arg,
                   quot;

   /* first calculate the linear drift correction */
   *y = a[0] + x * a[1];	/* value of y at point x */
   dyda[0] = 1;			/* derivative dy/d(a[0]), the zero order term  */
   dyda[1] = x;			/* derivative dy/d(a[1]), the 1st order term */
   for (i = 2; i < na; i += 4)	/* now go through all lorentzian/gaussian
				 * lines */
   {				/* calculate the gaussian part */
      /* a[i]=intensity, a[i+1]=frequency */
      /* a[i+2]=width, a[i+3]=gaussian fraction */
      arg = 1.665109 * (x - a[i + 1]) / a[i + 2];
      ex = exp((double) - (arg * arg));
      fac = a[i] * a[i + 3] * ex * 2.0 * arg;
      *y += a[i + 3] * a[i] * ex;	/* add to value at point y 	 */
      dyda[i] = a[i + 3] * ex;	/* derivative dy/d(intensity)	 */
      dyda[i + 1] = fac / a[i + 2];	/* derivative dy/d(frequency)	 */
      dyda[i + 2] = fac * arg / (1.665109 * a[i + 2]);
                                        /* derivative dy/d(width)       */
      dyda[i + 3] = a[i] * ex;	/* derivative dy/d(gauss frac)	 */
      /* now add the lorentzian part */
      ex = a[i + 2] * a[i + 2] / 4.0;
      arg = ex + (x - a[i + 1]) * (x - a[i + 1]);
      fac = (1.0 - a[i + 3]);
      quot = ex / arg;
      *y += fac * a[i] * quot;	/* add to value at point y	 */
      dyda[i] += fac * quot;	/* derivative dy/d(intensity)	 */
      dyda[i + 1] += fac * a[i] * quot * 2.0 * (x - a[i + 1]) / arg;
      /* derivative dy/d(frequency)	 */
      dyda[i + 2] += fac * 0.5 * a[i] * a[i + 2] * (1.0 - quot) / arg;
      /* derivative dy/d(width)	 */
      dyda[i + 3] += -a[i] * quot;	/* derivative dy/d(gauss frac)  */
   }
}

/*************************/
covsrt(covar, ma, lista, mfit)
/*************************/
float           covar[MAX_PARAMS][MAX_PARAMS];
int             lista[MAX_PARAMS];

{
}

/*********************/
readdata(x, y, sig, ndata)
/*********************/
float           x[],
                y[],
                sig[];
int            *ndata;

{
   FILE           *fopen();
   FILE           *inputfile;
   int             i,
                   s;
   float           wp;
   float           sum,
                   sumsq,
                   stddev;
   char            filename[MAXPATHL];

   strcpy(filename, path);
#ifdef VMS
   strcat(filename, "fitspec.data");
#else
   strcat(filename, "/fitspec.data");
#endif
   if (inputfile = fopen(filename, "r"))
   {
      if (fscanf(inputfile, "%d\n", ndata) != 1)
      {
	 error = 6;
	 fclose(inputfile);
	 return 1;
      }
      if (fscanf(inputfile, "%f\n", &sp) != 1)
      {
	 error = 6;
	 fclose(inputfile);
	 return 1;
      }
      if (fscanf(inputfile, "%f\n", &wp) != 1)
      {
	 error = 6;
	 fclose(inputfile);
	 return 1;
      }
      if (*ndata < 32)
      {
	 error = 7;
	 fclose(inputfile);
	 return 1;
      }
      if (*ndata > MAX_DATA)
      {
	 error = 8;
	 fclose(inputfile);
	 return 1;
      }
      for (i = 0; i < *ndata; i++)
      {
	 if (fscanf(inputfile, "%f\n", &y[i]) != 1)
	 {
	    error = 6;
	    return 1;
	 }
	 x[i] = wp - (i * wp / *ndata);
      }
      /* now compute standard deviation */
      s = *ndata / 10;
      if (s < 8)
	 s = 8;
      sum = 0;
      sumsq = 0;
      for (i = 0; i < s; i++)
      {
	 sum += y[i];
	 sumsq += y[i] * y[i];
      }
      stddev = sqrt((double) (s * sumsq - sum * sum) / (s * (s - 1)));
      sum = 0;
      sumsq = 0;
      for (i = *ndata - s; i < *ndata; i++)
      {
	 sum += y[i];
	 sumsq += y[i] * y[i];
      }
      stddev = (stddev + sqrt((double) (s * sumsq - sum * sum) / (s * (s - 1)))) / 2;
      for (i = 0; i < *ndata; i++)
	 sig[i] = stddev;
      fclose(inputfile);
      return 0;
   }
   else
   {
      error = 6;
      return 1;
   }
}

/*************************/
readparams(a, lista, ma, mfit)
/*************************/
float           a[MAX_PARAMS];
int             lista[MAX_PARAMS];
int            *ma,
               *mfit;

{
   FILE           *fopen();
   FILE           *inputfile;
   int             i,
                   r,
                   j,
                   ch,
                   listi;
   float           frequency,
                   intensity,
                   width,
                   gaussian;
   int             do_freq,
                   do_int,
                   do_width,
                   do_gauss;
   char            filename[MAXPATHL];

   strcpy(filename, path);
#ifdef VMS
   strcat(filename, "fitspec.inpar");
#else
   strcat(filename, "/fitspec.inpar");
#endif
   if (inputfile = fopen(filename, "r"))
   {
      i = 0;
      listi = 0;
      a[i] = 0;			/* linear drift */
      lista[listi++] = i++;
      a[i] = 0;
      lista[listi++] = i++;
      do
      {
	 if (fscanf(inputfile, "%f", &frequency) != 1)
	 {
	    ch = fgetc(inputfile);
	    while (ch == ' ')
	       ch = fgetc(inputfile);
	    if (ch == EOF)
	    {
	       *ma = i;
	       if (*ma < 5)
	       {
		  error = 3;
		  fclose(inputfile);
		  return 1;
	       }
	       fclose(inputfile);
	       *mfit = listi;
	       if (*mfit < 3)
	       {
		  error = 3;
		  fclose(inputfile);
		  return 1;
	       }
	       return 0;
	    }
	    else
	    {
	       error = 3;
	       fclose(inputfile);
	       return 1;
	    }
	 }
	 ch = fgetc(inputfile);
	 while (ch == ' ')
	    ch = fgetc(inputfile);
	 if (ch == '*')
	 {
	    do_freq = 0;
	    ch = fgetc(inputfile);
	    while (ch == ' ')
	       ch = fgetc(inputfile);
	 }
	 else
	    do_freq = 1;
	 if (ch == ',')
	 {
	    if (fscanf(inputfile, "%f", &intensity) != 1)
	    {
	       error = 3;
	       fclose(inputfile);
	       return 1;
	    }
	    ch = fgetc(inputfile);
	    while (ch == ' ')
	       ch = fgetc(inputfile);
	    if (ch == '*')
	    {
	       do_int = 0;
	       ch = fgetc(inputfile);
	       while (ch == ' ')
		  ch = fgetc(inputfile);
	    }
	    else
	       do_int = 1;
	    if (ch == ',')
	    {
	       if (fscanf(inputfile, "%f", &width) != 1)
	       {
		  error = 3;
		  fclose(inputfile);
		  return 1;
	       }
	       ch = fgetc(inputfile);
	       while (ch == ' ')
		  ch = fgetc(inputfile);
	       if (ch == '*')
	       {
		  do_width = 0;
		  ch = fgetc(inputfile);
		  while (ch == ' ')
		     ch = fgetc(inputfile);
	       }
	       else
		  do_width = 1;
	       if (ch == ',')
	       {
		  if (fscanf(inputfile, "%f", &gaussian) != 1)
		  {
		     error = 3;
		     fclose(inputfile);
		     return 1;
		  }
		  ch = fgetc(inputfile);
		  while (ch == ' ')
		     ch = fgetc(inputfile);
		  if (ch == '*')
		  {
		     do_gauss = 0;
		     ch = fgetc(inputfile);
		     while (ch == ' ')
			ch = fgetc(inputfile);
		  }
		  else
		     do_gauss = 1;
	       }
	       else
	       {
		  gaussian = 0.0;
		  do_gauss = 0;
	       }
	    }
	    else
	    {
	       width = 1.0;
	       do_width = 1;
	       gaussian = 0.0;
	       do_gauss = 0;
	    }
	 }
	 else
	 {
	    intensity = 1.0;
	    do_int = 1;
	    width = 1.0;
	    do_width = 1;
	    gaussian = 0.0;
	    do_gauss = 0;
	 }
	 if (ch != '\n')
	 {
	    error = 3;
	    fclose(inputfile);
	    return 1;
	 }
	 if (i >= MAX_PARAMS - 2)
	 {
	    error = 4;
	    fclose(inputfile);
	    return 1;
	 }
	 a[i] = intensity;
	 if (do_int)
	 {
	    lista[listi++] = i;
	    do_par[i] = ' ';
	 }
	 else
	    do_par[i] = '*';
	 i++;
	 a[i] = frequency - sp;
	 if (do_freq)
	 {
	    lista[listi++] = i;
	    do_par[i] = ' ';
	 }
	 else
	    do_par[i] = '*';
	 i++;
	 a[i] = width;
	 if (do_width)
	 {
	    lista[listi++] = i;
	    do_par[i] = ' ';
	 }
	 else
	    do_par[i] = '*';
	 i++;
	 a[i] = gaussian;
	 if (do_gauss)
	 {
	    lista[listi++] = i;
	    do_par[i] = ' ';
	 }
	 else
	    do_par[i] = '*';
	 i++;
      }
      while (1);
   }
   else
   {
      error = 3;
      return 1;
   }
}

/***************/
writeparams(a, ma)
/***************/
float           a[MAX_PARAMS];
int             ma;

{
   FILE           *fopen();
   FILE           *outputfile;
   int             i,
                   r,
                   j;
   float           frequency,
                   intensity,
                   width,
                   gaussian;
   char            filename[MAXPATHL];

   strcpy(filename, path);
#ifdef VMS
   strcat(filename, "fitspec.outpar");
#else
   strcat(filename, "/fitspec.outpar");
#endif
   if (outputfile = fopen(filename, "w"))
   {
      for (i = 2; i < ma; i += 4)
      {
	 fprintf(outputfile, "%g%c,%g%c,%g%c,%g%c\n",
		 a[i + 1] + sp, do_par[i + 1], a[i], do_par[i],
		 a[i + 2], do_par[i + 2], a[i + 3], do_par[i + 3]);
      }
      fclose(outputfile);
      return 0;
   }
   else
   {
      error = 5;
      return 1;
   }
}

/****************/
writestatus(chisq)
/****************/
double          chisq;
{
   FILE           *fopen();
   FILE           *outputfile;
   char            filename[MAXPATHL];

   strcpy(filename, path);
#ifdef VMS
   strcat(filename, "fitspec.stat");
#else
   strcat(filename, "/fitspec.stat");
#endif
   if (outputfile = fopen(filename, "w+"))
   {
      fprintf(outputfile, "%d\n", error);
      fprintf(outputfile, "%f\n", chisq);
      fprintf(outputfile, "%d\n", count1);
      fprintf(outputfile, "%d\n", count2);
      fclose(outputfile);
   }
   else
   {
      printf("cannot create 'fitspec.stat' file\n");
      return 1;
   }
}

/*************/
main(argc, argv)
/*************/
int             argc;
char           *argv[];

{
   float           x[MAX_DATA],
                   y[MAX_DATA],
                   sig[MAX_DATA];
   int             ndata,
                   ma,
                   mfit;
   float           a[MAX_PARAMS];
   int             lista[MAX_PARAMS];
   double          chisq,
                   lastchisq;
   float           alamda;
   int             i;
   int             fgauss_lorentz();
   char            filename[MAXPATHL];

   fprintf(stderr,"OpenVnmrJ verison of fitspec is not available\n");
   exit(EXIT_SUCCESS);
   path[0] = 0;
#ifdef VMS
   if (argc > 1)
   {
      i = strlen( argv[ 1 ] ) - 1;
      if (argv[1][i] == ']' || argv[1][i] == ':')
        strcpy(path,argv[1]);
   }
#else
   if ((argc > 1) && (argv[1][0] == '/'))
   {
      strcpy(path, argv[1]);
      strcat(path, "/");
   }
#endif
   strcpy(filename, path);
#ifdef VMS
   strcat(filename, "fitspec.stat");
#else
   strcat(filename, "/fitspec.stat");
#endif
   unlink(filename);
   strcpy(filename, path);
#ifdef VMS
   strcat(filename, "fitspec.outpar");
#else
   strcat(filename, "/fitspec.outpar");
#endif
   unlink(filename);

   error = 0;
   count1 = 0;
   count2 = 0;
   chisq = 1e30;
   alamda = -1.0;
   if (readdata(x, y, sig, &ndata))
   {
      writestatus(chisq);
      return 1;
   }
   if (readparams(a, lista, &ma, &mfit))
   {
      writestatus(chisq);
      return 1;
   }
   do
   {
      lastchisq = chisq;
      if (mrqmin(x, y, sig, ndata, a, ma, lista, mfit, covar, alpha,
		 &chisq, fgauss_lorentz, &alamda))
      {
	 if (debug)
	    printf("ERROR C\n");
	 writestatus(chisq);
	 return 1;
      }
   }
   while ((fabs((double) (chisq - lastchisq) / chisq) > 1e-4) ||
	  ((count1 <= 5) && (count2 <= 10)));
   alamda = 0.0;
   if (mrqmin(x, y, sig, ndata, a, ma, lista, mfit, covar, alpha,
	      &chisq, fgauss_lorentz, &alamda))
   {
      writestatus(chisq);
      return 1;
   }

   writeparams(a, ma);
   writestatus(chisq);
   return 0;
}
