/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#ifndef ASPDISPEAKS_H
#define ASPDISPEAKS_H

#include <list>
#include "AspFrame.h"

class AspDisPeaks {

public:

    static int aspPeaks(int argc, char *argv[], int retc, char *retv[]);
    static void display(spAspFrame_t frame); 
    
    static spAspPeak_t selectPeak(spAspFrame_t frame, int x, int y);
    static void deletePeak(spAspFrame_t frame, spAspPeak_t peak);

    static void save(spAspFrame_t frame, char *path = NULL);
    static void load(spAspFrame_t frame, char *path = NULL, bool show=false);

    static void peakPicking(spAspFrame_t frame, int x, int y, int w, int h);
    static void modifyPeak(spAspFrame_t frame, spAspPeak_t peak, int x, int y);

    static double noisemult;
    static int posPeaks;

private:

    static void nll(spAspFrame_t frame, int argc, char *argv[]);
    static void dpf(spAspFrame_t frame, int argc, char *argv[]);
};

#endif /* ASPDISPEAKS_H */
