/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/

#ifndef AIPSPECSTRUCT_H
#define AIPSPECSTRUCT_H

#ifndef MAXPATHLEN
#define MAXPATHLEN 1024
#endif

#ifndef MAXDIM
#define MAXDIM 4
#endif

#ifndef MAXWORDLEN
#define MAXWORDLEN 16 
#endif

typedef struct _specStruct {
   float *data;
   char fidpath[MAXPATHLEN];
   char type[MAXWORDLEN];        // E.g., "absval", "real", "imag", "complex" 
   char dataType[MAXWORDLEN];    // E.g., "spectrum", "baseline", "fitting"
   char storage[MAXWORDLEN];     // E.g., "float" 
   char *nucleus[MAXWORDLEN];     // E.g., "H1","C13" 
   int bits;
   int rank;			// data rank 
   int matrix[MAXDIM];		// np, nv, nv2,...
   int spec_data_rank;		// all spatial dimensions are combined as one, see spec_matrix.	
   int spec_display_rank;	// if spec_data_rank=2, spec_display_rank=1, it is arrayed 1D
   int spec_matrix[MAXDIM];	// np, nv*nv2*...
   double sfrq[MAXDIM];
   double reffrq[MAXDIM];
   double sw[MAXDIM];
   double upfield[MAXDIM];
   double sp[MAXDIM];
   double wp[MAXDIM];
   double rp[MAXDIM];
   double lp[MAXDIM];
} specStruct_t;

#endif /* AIPSPECSTRUCT_H */
