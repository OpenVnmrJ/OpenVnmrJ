/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/

#ifndef AIPSPECDATAMGR_H
#define AIPSPECDATAMGR_H

#include <map>
#include <string>
#include <utility>
#include <list>
using std::list;
#include <set>
using std::set;

#include "aipSpecData.h"

typedef std::map<std::string, spSpecData_t> SpecDataMap;

class SpecDataMgr 
{
public:

    static int aipLoadSpec(int argc, char *argv[], int retc, char *retv[]);
    static int aipRemoveSpec(int argc, char *argv[], int retc, char *retv[]);

    static SpecDataMgr *get();
    void removeData(const char *str);
    spSpecData_t getDataByKey(string key);
    spSpecData_t getDataByNumber(int num);
    spSpecData_t getDataByType(string type, int num=1);
    SpecDataMap *getSpecDataMap();

    int loadSpec(string key, char *path);

    float *getTrace(string key, int ind, double scale, int npt);
    float *getTrace(string key, int ind, double scale, float *data, int npt);
    float *getTrace4ComboKey(string key, int ind, double scale, int npt);
    void calcTrace(float *inTrace, int npoints, int step, double scale, float *outTrace, int npt);
    int getNtraces(string nucname);
    int getYminmax(string nucleus,double &ymin, double &ymax);
    void getTraceInfo(int i, string &key, string &path, int &ind, int &npts, double &minVal, double &maxVal,string nucname);
    string getNextKey();

protected:
    
private:

    static SpecDataMgr *specDataMgr;

    SpecDataMap *specDataMap;
    float *selData;

    SpecDataMgr();
    
};

#endif /* AIPSPECDATAMGR_H */
