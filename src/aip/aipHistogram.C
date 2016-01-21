/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include "aipHistogram.h"

Histogram::Histogram(int bins, float *databuf, long long npixels, bool phaseImage)
{
    nbins = bins;
    counts = new int[nbins];
    if (counts == NULL || databuf == NULL || npixels == 0) {
        return;
    }
    memset(counts, 0, nbins * sizeof(int));

    // Find max and min of data
    float *bufend = databuf + npixels;
    top = bottom = databuf[0];
    float *pf;
    for (pf = databuf + 1; pf < bufend; ++pf) {
        if (top < *pf) {
            top = *pf;
        } else if (bottom > *pf) {
            bottom = *pf;
        }
    }

    //if(bottom < 0.0 && !phaseImage) bottom = 0.0;

    if (top == bottom) {
        // Degenerate case; put all counts in "middle" bin
        counts[nbins/2] = (int)npixels;
    } else {
        int index;
        float scale = nbins / (top - bottom);
	float offset = bottom;
        for (pf = databuf + 1; pf < bufend; ++pf) {
            index = (int)(scale * (*pf - offset));
            if (*pf == top || index >= nbins){
                // Include border case in top bin
                // (otherwise always lose top pixel)
                index = nbins - 1;
            }
            ++counts[index];
        }
    }
}
