/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#define MIN_COEFVAL	0.0005

struct _subcoef
{
   int		rr_nzero;
   int		ir_nzero;
   int		ri_nzero;
   int		ii_nzero;
   float	*coefval;
};

typedef struct _subcoef subcoef;

struct _coef3D
{
   subcoef	f3t2;
   subcoef	f2t1;
   int		ncoefs;
};

typedef struct _coef3D coef3D;

