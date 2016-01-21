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

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <stddef.h>
#include <sys/time.h>

#define TRUE       1
#define FALSE      0

#define MAXSTR 1024
#define MAXELEM 1024

/* Some data types */
#define INT16      1  /* 16 bit integer */
#define INT32      2  /* 32 bit integer */
#define FLT32      3  /* 32 bit float */
#define DBL64      4  /* 64 bit double */

/* Include GSL libraries */
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_blas.h>
#include <gsl/gsl_linalg.h>
#include <gsl/gsl_eigen.h>

#include <gsl/gsl_rng.h>
#include <gsl/gsl_randist.h>
#include <gsl/gsl_vector.h>
#include <gsl/gsl_multifit_nlin.h>

/* Include local functions */
#include "gsl_functions.c"  /* local GSL functions */
#include "utils.c"
#include "getpars.c"
#include "fdf.c"
#include "image_functions.c"

/* Include Analyze libraries */
#include "AVW.c"  /* local AVW functions */

#ifdef NOTLIKELY
#include <AVW.h>
#include <AVW_ImageFile.h>
#include <AVW.c> 
#endif
//#include "avw.c"
