/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/
#ifndef ASPTRACELIST_H
#define ASPTRACELIST_H

#include <map>
#include <string>
#include <utility>
#include <list>
using std::list;
#include <set>
using std::set;

#include "AspTrace.h"
#include "AspCell.h"
#include "AspDataInfo.h"
#include "aipSpecStruct.h"
#include "AspRenderedTrace.h"

typedef std::map<string, spAspTrace_t> AspTraceMap;

class AspTraceList 
{
public:

    AspTraceList();
    ~AspTraceList();

    void addTrace(string key, spAspTrace_t trace);
    void clearList();
    void deleteTrace(int ind);
    void deleteTrace(string key);
    void deleteTrace();
    spAspTrace_t getTraceByKey(string key);
    spAspTrace_t getTraceByInd(int id);
    spAspTrace_t getTrace(int id);
    spAspTrace_t getFirstTrace(AspTraceMap::iterator& traceItr);
    spAspTrace_t getNextTrace(AspTraceMap::iterator& traceItr);
    spAspTrace_t getSelTrace();

    AspTraceMap *getTraceMap() {return traceMap;}

    int getSize() {return traceMap->size();}
    double getMinX();
    double getMaxX();

    void resetVpos();
    void setLabelFlag(int flag);
    void setLabels(int argc, char *argv[], int flag);
    string getIndList();
    list<spAspTrace_t> *getTraceList();

    spAspTrace_t selectTrace(spAspCell_t cell,int x, int y, bool &changeFlag);
    spAspTrace_t nearestTrace(spAspCell_t cell,int x, int y);
    void showSpec(spAspCell_t cell, int mode);
    void showSpec_dss(spAspCell_t cell);
    void showSpec_ds(spAspCell_t cell);
    void showSpec(spAspCell_t cell,spAspTrace_t trace);
    void showBCModel(spAspCell_t cell);
    
    int maxInd;
    int getNtraces() { return renderedTraces->size(); }

protected:
    
private:

    AspTraceMap *traceMap;
    list<spAspTrace_t> *traceList;
    list<AspRenderedTrace> *renderedTraces;
};

#endif /* ASPTRACELIST_H */
