/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/

/*
This is a singleton class to manage CSI data display 
*/

#ifndef AIPSPECVIEWMGR_H
#define AIPSPECVIEWMGR_H

#include "aipVnmrFuncs.h"
#include "aipSpecViewList.h"
#include "aipGframe.h"

class SpecViewMgr
{
public:

    static SpecViewMgr *get();

    static int aipShowSpec(int argc, char *argv[], int retc, char *retv[]);
    static int aipMakeMaps(int argc, char *argv[], int retc, char *retv[]);

    void selectSpecView(SpecViewList *specList, int ind, int mask);
    void updateCSIGrid(bool draw=false);
    void updateCSIGrid(SpecViewList *specList, int frameID);
    void updateArrayLayout(bool draw=false, bool selSpec=false, int layout=GRIDLAYOUT);
    void updateArrayLayout(SpecViewList *specList, int frameID);
    void getCSIDims(int *nx, int *ny, int *nz, int *ns, spGframe_t gf);
    void getArrayDim(int *n, spGframe_t gf);
    void specViewUpdate();
    void setShowRoi(spGframe_t gf);
    int makeMaps(spGframe_t gf, char *path, string key, int flag, double f, double f1, double f2, double sw);
    int calcRatioMap(char *type,int flag, string key, string name);

protected:
    
private:

    static SpecViewMgr *specViewMgr; // singleton
    static bool hide;
    std::list<int> selectedViews;
    int currentView;
    float2 prevP;

    SpecViewMgr(); // private constructor
    int getCSIGrafAreas(graphInfo_t *gInfo, int n, int frameID, bool drawIntersect);
    int getGrafAreas(graphInfo_t *gInfo, int n, int frameID, bool selSpec, int layout);
    void getVoxInds(int nt, int nv, int nv2, int nv3, int ns, int slice, int *ind);
};

#endif /* AIPSPECVIEWMGR_H */
