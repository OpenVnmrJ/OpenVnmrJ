/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#ifndef VNMRFDIST_H
#define VNMRFDIST_H

#ifdef VNMR_GPL
#include <gsl/gsl_cdf.h>
#define f_distribution(f,n1,n2) gsl_cdf_fdist_P( (f), (n1), (n2) )

#else

extern double f_distribution(double f, double n1, double n2);
#endif

#endif 
