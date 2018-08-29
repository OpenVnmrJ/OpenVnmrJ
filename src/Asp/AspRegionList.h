/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/
#ifndef ASPREGIONLIST_H
#define ASPREGIONLIST_H

#include <map>
#include <string>
#include <utility>
#include <list>
using std::list;
#include <set>
using std::set;

#include "AspRegion.h"
#include "AspDataInfo.h"
#include "AspCell.h"
#include "AspTraceList.h"

typedef std::map<double, spAspRegion_t> AspRegionMap;

class AspRegionList 
{
public:

    AspRegionList();
    ~AspRegionList();

    void display(spAspCell_t cell, AspTraceList *selTraceList, spAspDataInfo_t dataInfo);
    void addRegion(int ind, spAspRegion_t region);
    void clearList();
    void deleteRegion(int ind);
    void deleteRegion();
    spAspRegion_t getRegion(int id);
    spAspRegion_t getSelRegion();
    spAspRegion_t getFirstRegion(AspRegionMap::iterator& regionItr);
    spAspRegion_t getNextRegion(AspRegionMap::iterator& regionItr);
    spAspRegion_t selectRegion(spAspCell_t cell, int x, int y, bool &changeFlag);

    AspRegionMap *getRegionMap() {return regionMap;}
    int getSize() {return regionMap->size();}
    void getBCMask(spAspTrace_t trace, float *baseData, float *mask, int npts);

protected:
    
private:

    AspRegionMap *regionMap;
};

#endif /* ASPREGIONLIST_H */
