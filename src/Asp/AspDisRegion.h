/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#ifndef ASPDISREGION_H
#define ASPDISREGION_H

#include <list>
#include "AspFrame.h"
#include "AspRegionList.h"

class AspDisRegion {

public:

    static int aspRegion(int argc, char *argv[], int retc, char *retv[]);
    static void display(spAspFrame_t frame); 
    
    static spAspRegion_t selectRegion(spAspFrame_t frame, int x, int y);
    static void deleteRegion(spAspFrame_t frame, spAspRegion_t region);

    static void save(spAspFrame_t frame, char *path = NULL);
    static void load(spAspFrame_t frame, char *path = NULL, bool show=false);

    static spAspRegion_t createRegion(spAspFrame_t frame, int x,int y,int prevX, int prevY);
    static void modifyRegion(spAspFrame_t frame, spAspRegion_t region, int x, int y, 
		int prevX, int prevY, int mask);

    static double noisemult;

    static AspRegionList *getRegionList() {return regionList;}
    static int getRegionFlag() {return regionFlag;}
    static void calcBCModel(spAspTrace_t trace);
    static void calcBCModel_CWT(spAspTrace_t trace);
    static void calcBaseMask(spAspFrame_t frame);
    static void basePoints(spAspFrame_t frame, int n);
    static void saveBCfrq(spAspFrame_t frame);
    static void loadBCfrq(spAspFrame_t frame);
    static void doBC(spAspFrame_t frame);
    static void undoBC(spAspFrame_t frame);

private:

    static AspRegionList *regionList;
    static int regionFlag;
};

#endif /* ASPDISREGION_H */
