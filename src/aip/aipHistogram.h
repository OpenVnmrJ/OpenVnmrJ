/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/

#ifndef AIPHISTOGRAM_H
#define AIPHISTOGRAM_H

#include <string.h>

class Histogram
{
  public:
    int nbins;		// Number of bins
    int *counts;	// Number of pixels in each bin
    //int nCounts;        // Total number of counts in all bins
    float bottom;	// Intensity at bottom of first bin
    float top;		// Intensity at top of last bin

    Histogram() {
        counts = NULL;
        nbins = 0;
    }

    Histogram(int bins) {
        if ( (counts = new int[nbins=bins]) ) {
            memset(counts, 0, nbins * sizeof(int));
        }
    }

    Histogram(int bins, float *data, long long npixels, bool phaseImage=false);

    ~Histogram() {
        if (counts)
           delete [] counts;
        counts = NULL;
    }
};

#endif /* AIPHISTOGRAM_H */
