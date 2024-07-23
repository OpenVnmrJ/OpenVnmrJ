/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "vnmrsys.h"
#include "wjunk.h"
#include "vnmr_svd.h"

static int debug = 0;

/* ###### memory allocation functions ###### */

static void memerror( char *error_text)
{
    Werrprintf("Memory allocation error %s\n",error_text);
}


static double **dmatrix(int nrl, int nrh, int ncl, int nch)
/* allocate a double matrix with subscript range m[nrl..nrh][ncl..nch] */
{
    int i, nrow=nrh-nrl+1,ncol=nch-ncl+1;
    double **m;
    /* allocate pointers to rows */
    m=(double **) malloc((unsigned)((nrow+1)*sizeof(double*)));
    if (!m) memerror("allocation failure 1 in matrix()");
    m += 1;
    m -= nrl;
    /* allocate rows and set pointers to them */
    m[nrl]=(double *) malloc((unsigned)((nrow*ncol+1)*sizeof(double)));
    if (!m[nrl])
    {
       memerror("allocation failure 2 in matrix()");
       return NULL;
    }
    m[nrl] += 1;
    m[nrl] -= ncl;
    for(i=nrl+1;i<=nrh;i++) m[i]=m[i-1]+ncol;
    /* return pointer to array of pointers to rows */
    return m;
}

static void free_dmatrix(double **m, int nrl, int nrh, int ncl, int nch)
/* free a double matrix allocated by dmatrix() */
{
    free((char *) (m[nrl]+ncl-1));
    free((char *) (m+nrl-1));
}

static double *dvector(int nl, int nh)
{
    double *v;
    v=(double *)malloc((unsigned) (nh-nl+1)*sizeof(double));
    if (!v)
    {
       memerror("allocation failure in vector()");
       return NULL;
    }
    return v-nl;
}

static void free_dvector(double *v, int nl, int nh)
{
    free((char*) (v+nl));
}

/* ###### end of memory allocation functions ###### */

/* ###### main function starts here ###### */

static int getValsPerLine(FILE *file)
{
    char line[1024*4];
    char *endptr;
    char *str;
    int vals;
    double val;

    if (fgets(line, 1024*4, file) == NULL)
       return 0;
    str = line;
    vals = 0;
    while (1)
    {
       errno = 0;
       val = strtod(str, &endptr);
       if (debug)
          fprintf(stderr,"errno: %d val= %g\n",errno, val);
       if ((errno != 0) && (val == 0.0))
          return vals;
       if (debug)
          fprintf(stderr,"endptr: %s str= %s\n",endptr, str);
       if (endptr == str)
          return vals;
       str = endptr;
       vals++;
    }
    return vals;
}

static void getVals(FILE *file, double *array, int count)
{
    char line[1024*4];
    char *endptr;
    char *str;
    int i;
    double val;

    if (fgets(line, 1024*4, file) == NULL)
       return;
    str = line;
    i = 0;
    while (i < count)
    {
       i++;
       errno = 0;
       val = strtod(str, &endptr);
       if (debug)
          fprintf(stderr,"errno: %d val= %g\n",errno, val);
       if ((errno != 0) && (val == 0.0))
          return;
       if (debug)
          fprintf(stderr,"endptr: %s str= %s\n",endptr, str);
       if (endptr == str)
          return;
       str = endptr;
       array[i] = val;
    }
}

static void getMatrix(FILE *file, double **matrix, int rows, int cols)
{
    char line[1024*4];
    char *endptr;
    char *str;
    int row, col;
    double val;

    row = 0;
    while (row < rows)
    {
       row++;
       if (fgets(line, 1024*4, file) == NULL)
          return;
       str = line;
       col = 0;
       while (col < cols)
       {
          col++;
          errno = 0;
          val = strtod(str, &endptr);
          if (debug)
             fprintf(stderr,"errno: %d val= %g\n",errno, val);
          if ((errno != 0) && (val == 0.0))
             return;
          if (debug)
             fprintf(stderr,"endptr: %s str= %s\n",endptr, str);
          if (endptr == str)
             return;
          str = endptr;
          matrix[row][col] = val;
       }
    }
}

/*--- start of main function ---*/
//
// svdfit(matrix, experimental, result)
//
// Solve, in least-squares sense, [A] * [x] = [b].
// b is vector of nrow observations;
// A is (nrow by ncol) matrix of ncol basis vectors
// each of nrow values
// x is the solution vector giving the coefficient of each
// basis vector to fit the data.
// Input arguments are:
// matrix is a file with the linear equation coefficients
// It is nrow,ncol and corresponds to [A] above
//
// experiments is a file with the nrow experimental values
// and corresponds to [b] above
//
// result is the name of the result file which will contain the
// ncol solution values and corresponds to [x] above
//
int svdfit(int argc, char *argv[], int retc, char *retv[])
{
    FILE *afile,*bfile,*xfile;
    int nrows, ncols;
    int i,j;
    double *x,*b,*w,**u,**v;
    double max;

    if (argc != 4)
    {
        Werrprintf("%s requires three input arguments",argv[0]);
        ABORT;
    }

    if ((bfile=fopen(argv[2],"r"))==0)
    {
        Werrprintf("%s: cannot open experimental data file [b] %s",
                  argv[0], argv[2]);
        ABORT;
    }
    nrows = getValsPerLine(bfile);
    if (debug)
       fprintf(stderr,"nrows vals in %s is %d\n", argv[2], nrows);
    if (nrows == 0)
    {
        Werrprintf("%s: experimental data file [b] %s is empty",
                  argv[0], argv[2]);
        fclose(bfile);
        ABORT;
    }
    if ((afile=fopen(argv[1],"r"))==0)
    {
        Werrprintf("%s: cannot open experimental data file [b] %s",
                  argv[0], argv[1]);
        fclose(bfile);
        ABORT;
    }
    ncols = getValsPerLine(afile);
    if (debug)
       fprintf(stderr,"ncols vals in %s is %d\n", argv[1], ncols);
    if (ncols == 0)
    {
        Werrprintf("%s: no basis data in file [A] %s",
                  argv[0], argv[1]);
        fclose(afile);
        fclose(bfile);
        ABORT;
    }
    if (ncols > nrows)
    {
        Werrprintf("%s is underdetermined: rows  < cols",
                  argv[0]);
        fclose(afile);
        fclose(bfile);
        ABORT;
    }
    if ( (x=dvector(1,ncols)) == NULL)
    {
        fclose(afile);
        fclose(bfile);
        ABORT;
    }
    b=dvector(1,nrows);
    w=dvector(1,ncols);
    u=dmatrix(1,nrows,1,ncols);
    v=dmatrix(1,ncols,1,ncols);
    rewind(bfile);
    getVals(bfile, b, nrows);
    if (debug)
    for (i = 1; i <= nrows; i++)
       fprintf(stderr,"b[%d]= %g\n", i, b[i]);
    fclose(bfile);
    rewind(afile);
    getMatrix(afile, u, nrows, ncols);
    fclose(afile);
    if (debug)
       for (i = 1; i <= nrows; i++)
       {
          for (j = 1; j <= ncols; j++)
             fprintf(stderr,"u[%d][%d] = %g  ",i,j, u[i][j]);
          fprintf(stderr,"\n");
       }
    if ((xfile=fopen(argv[3],"w"))==0)
    {
        Werrprintf("%s: cannot open output results file [x] %s",
                  argv[0], argv[3]);
        ABORT;
    }
    
    vnmr_svd(u, nrows, ncols, w, v); // Given A (in u) get U, W, and V.
    if (debug)
       for (j = 1; j <= ncols; j++)
          fprintf(stderr,"w[%d]= %g\n", j, w[j]);
    max = 0.0;
    for (j = 1; j <= ncols; j++) {
       if (w[j] > max) {
          max = w[j];
       }
    }
    double tol = 1e-6; // Tolerance for zeroing singular values.
    for (j = 1; j <= ncols; j++) {
       if (w[j] < tol * max) {
          w[j] = 0;
       }
    }
    if (debug)
       for (j = 1; j <= ncols; j++)
          fprintf(stderr,"w[%d]= %g\n", j, w[j]);
    vnmr_svd_solve(u, w, v, nrows, ncols, b, x);
    if (debug)
       for (j = 1; j <= ncols; j++)
          fprintf(stderr,"x[%d]= %g\n", j, x[j]);
    for (j = 1; j <= ncols; j++)
       fprintf(xfile,"%g\n", x[j]);
    fclose(xfile);
    free_dvector(x,1,ncols);
    free_dvector(b,1,nrows);
    free_dvector(w,1,ncols);
    free_dmatrix(u,1,nrows ,1,ncols);
    free_dmatrix(v,1,ncols ,1,ncols);
    RETURN;
}
