/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <gsl/gsl_vector.h>
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_linalg.h>


void vnmr_svd(double **a, int irow, int jcol, double w[], double **v)
{
   gsl_matrix *gA;
   gsl_matrix *gV;
   gsl_vector *gS;
   int i,j;
   int res;
   gA = gsl_matrix_alloc(irow,jcol);
   for (i=0; i<irow; i++)
      for (j=0; j<jcol; j++)
         gsl_matrix_set(gA, i, j, a[i+1][j+1]);
   
   gV = gsl_matrix_alloc(jcol,jcol);
   gS = gsl_vector_alloc(jcol);

   res = gsl_linalg_SV_decomp_jacobi(gA, gV, gS);

   for (i=0; i<irow; i++)
      for (j=0; j<jcol; j++)
         a[i+1][j+1] = gsl_matrix_get( gA, i, j);
   for (i=0; i<jcol; i++)
      for (j=0; j<jcol; j++)
         v[i+1][j+1] = gsl_matrix_get( gV, i, j);
   for (i=0; i<jcol; i++)
      w[i+1] = gsl_vector_get( gS, i);
    

   gsl_matrix_free(gA);
   gsl_matrix_free(gV);
   gsl_vector_free(gS);
}

void vnmr_svd_solve(double **u, double w[], double **v, int irow, int jcol, double b[], double x[])
{
   gsl_matrix *gU;
   gsl_matrix *gV;
   gsl_vector *gS;
   gsl_vector *gB;
   gsl_vector *gX;
   int i,j;
   int res;
   gU = gsl_matrix_alloc(irow,jcol);
   for (i=0; i<irow; i++)
      for (j=0; j<jcol; j++)
         gsl_matrix_set(gU, i, j, u[i+1][j+1]);

   gV = gsl_matrix_alloc(jcol,jcol);
   for (i=0; i<jcol; i++)
      for (j=0; j<jcol; j++)
         gsl_matrix_set( gV, i, j, v[i+1][j+1]);
   
   gS = gsl_vector_alloc(jcol);
   for (i=0; i<jcol; i++)
      gsl_vector_set( gS, i, w[i+1]);

   gB = gsl_vector_alloc(irow);
   for (i=0; i<irow; i++)
      gsl_vector_set( gB, i, b[i+1]);

   gX = gsl_vector_alloc(jcol);
   res = gsl_linalg_SV_solve(gU, gV, gS, gB, gX);

   for (i=0; i<jcol; i++)
      x[i+1] = gsl_vector_get( gX, i);
    

   gsl_matrix_free(gU);
   gsl_matrix_free(gV);
   gsl_vector_free(gS);
   gsl_vector_free(gB);
   gsl_vector_free(gX);
fprintf(stderr,"solve m=%d n=%d\n",irow, jcol);
for (i=1; i<=jcol; i++)
     fprintf(stderr," w[%d]: %g\n",i,w[i]);
for (i=0; i<jcol; i++)
     fprintf(stderr," x[%d]: %g\n",i+1,x[i+1]);
}
