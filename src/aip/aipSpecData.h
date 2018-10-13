/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/

#ifndef AIPSPECDATA_H
#define AIPSPECDATA_H

#include "sharedPtr.h"
#include "aipVnmrFuncs.h"
#include "ddlSymbol.h"
#include "aipSpecStruct.h"

class SpecData 
{
public:

    DDLSymbolTable *st;
    specStruct_t *specStruct;

    SpecData(string key, string path, DDLSymbolTable *newSt, specStruct_t *newSs);
    ~SpecData();

    static bool fillSpecStruct(DDLSymbolTable *newSt, specStruct_t *newSs);
    static void getDataMax(register float  *datapntr, register int npnt, register float  *max);
    static void getDataMin(register float  *datapntr, register int npnt, register float  *min);

    string getKey() {return key;}
    string getPath() {return path;}
    string getLabel() {return label;}
    void setLabel(string l) {label=l;}
    string getColor() {return color;}
    void setColor(string c) {color=c;}
    specStruct_t *getSpecStruct() {return specStruct;}
    float *getTrace(int ind, int np);
    int getNumTraces();
    string getDataType();

protected:
    
private:
    string key;
    string path;
    string label;
    string color;
};

typedef boost::shared_ptr<SpecData> spSpecData_t;
#endif /* AIPSPECDATA_H */
