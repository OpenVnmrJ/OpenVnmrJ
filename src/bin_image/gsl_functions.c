/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*---------------------------------------------------------------------------*/
/* This is free software: you can redistribute it and/or modify              */
/* it under the terms of the GNU General Public License as published by      */
/* the Free Software Foundation, either version 3 of the License, or         */
/* (at your option) any later version.                                       */
/*                                                                           */
/* This is distributed in the hope that it will be useful,                   */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of            */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the              */
/* GNU General Public License for more details.                              */
/*                                                                           */
/* You should have received a copy of the GNU General Public License         */
/* If not, see <http://www.gnu.org/licenses/>.                               */
/*---------------------------------------------------------------------------*/
/**/

void gsl_print_matrix(gsl_matrix *M,char text[]) {
  /* Prints a GSL matrix in a nice matlab-compatible format */
  int r,c;
  printf("%s = [\n",text);
  for (r = 0; r < M->size1; r++) {
    for (c = 0; c < M->size2; c++) {
      printf("%8.6f  ",gsl_matrix_get(M,r,c));
    }
  printf(";\n");
  }
  printf("];\n");
  fflush(stdout);
}


void gsl_print_vector(gsl_vector *V,char text[]) {
  /* Prints a GSL vector in a nice matlab-compatible format */
  printf("%s = [\n",text);
  gsl_vector_fprintf(stdout,V,"%.6f");
  printf("];\n");
  fflush(stdout);
}


float gsl_vector_mean(gsl_vector *V) {
  float mean = 0;
  int r;
  for (r = 0; r < V->size; r++) mean += gsl_vector_get(V,r);
  mean /= V->size;

  return(mean);
}

float gsl_vector_mean_print(gsl_vector *V) {
  float mean = 0;
  int r;
  for (r = 0; r < V->size; r++) {
    mean += ((float) gsl_vector_get(V,r));
    printf("v, mean = %f, %f\n",(float) gsl_vector_get(V,r),mean);
  }
  mean /= V->size;
  printf("sz, mean = %f, %f\n",(float)V->size,mean);

  return(mean);
}

int gsl_vector_ispos(const gsl_vector *V) {
  int r;
  
  for (r = 0; r < V->size; r++)
    if (gsl_vector_get(V,r) <= 0) return(0);
  return(1);
}
