/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/

#ifndef ASPROILIST_H
#define ASPROILIST_H

#include <iostream>
#include <fstream>
#include <list>
#include <vector>
#include <map>

#include "AspRoi.h" 
#include "AspTraceList.h" 

typedef std::map<string, spAspRoi_t> AspRoiMap;

class AspRoiList {

protected:

public:

    AspRoiList(int f, bool show=false);
    ~AspRoiList();

    void drawRois(spAspCell_t cell);
    void unselectRois();

    int frameID;
    bool autoAdjusting;

    int getNumRois();
    void removeRois();
    spAspRoi_t getRoi(int id);
    spAspRoi_t getRoi(double freq1, double freq2);
    spAspRoi_t getFirstRoi(AspRoiMap::iterator& roiItr);
    spAspRoi_t getNextRoi(AspRoiMap::iterator& roiItr);

    spAspRoi_t selectRoi(spAspCell_t cell, int x, int y, bool handle=false);

    void loadRois(spAspCell_t cell, char *path);
    void saveRois(char *path);

    void setShowRois(bool b);
    void hideRoi(int ind);
    void hideSelRoi();
    void deleteSelRoi();
    void deleteRoi(int ind);
    void deleteRoi(double c1);
    void deleteRoi(double c1, double c2);
    void deleteRoi(spAspRoi_t roi);
    void addRoi(spAspRoi_t roi);
    AspRoiMap *getRoiMap() {return roiList;}
    void setRoiColor(char *name);
    void setRoiOpaque(int op);
    void setRoiHeight(int h);
    string getRoiPath() { return roiPath;}
    void addRois(string nucname, int argc, char *argv[]);
    void autoAdjust(spAspRoi_t roi = nullAspRoi);
    void autoDefine(AspTraceList *traceList, spAspDataInfo_t dataInfo, double f1, double f2);
    void autoDefine(AspTraceList *traceList, spAspDataInfo_t dataInfo);

    list<double> *getRoiFreqs(int dim);

private:

    string roiPath;
    AspRoiMap *roiList;
    list<double> *roiFreqs;
};

#endif /* ASPROILIST_H */
