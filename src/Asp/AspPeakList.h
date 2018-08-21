/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/
#ifndef ASPPEAKLIST_H
#define ASPPEAKLIST_H

#include <map>
#include <string>
#include <utility>
#include <list>
using std::list;
#include <set>
using std::set;

#include "AspPeak.h"
#include "AspDataInfo.h"
#include "AspCell.h"
#include "AspTraceList.h"

typedef std::map<string, spAspPeak_t> AspPeakMap;

class AspPeakList 
{
public:

    AspPeakList();
    ~AspPeakList();

    void display(spAspCell_t cell, AspTraceList *selTraceList, spAspDataInfo_t dataInfo, 
	int peakFlag, int specFlag);
    void addPeak(int ind, spAspPeak_t peak);
    void clearList();
    void deletePeak(int ind);
    void deletePeak();
    spAspPeak_t getPeak(int id);
    spAspPeak_t getSelPeak();
    spAspPeak_t getFirstPeak(AspPeakMap::iterator& peakItr);
    spAspPeak_t getNextPeak(AspPeakMap::iterator& peakItr);
    spAspPeak_t selectPeak(int x, int y, bool &changeFlag);
    void autoLayout(spAspCell_t cell, spAspDataInfo_t dataInfo, int peakFlag);

    AspPeakMap *getPeakMap() {return peakMap;}
    list<spAspPeak_t> *getPeakList(bool update=false);

    int getSize() {return peakMap->size();}
    void resetLabels();

    //get first peak for trace specified by dataID
    spAspPeak_t getFirstPeak(string dataID);
    // get all peaks for a trace
    list<spAspPeak_t> getPeakList(string dataID);
    // shift peaks of a trace
    void shiftPeaks(string dataID, double shift); 

protected:
    
private:

    AspPeakMap *peakMap;
    list<spAspPeak_t> *peakList;
    list<spAspPeak_t> *disList;
};

#endif /* ASPPEAKLIST_H */
