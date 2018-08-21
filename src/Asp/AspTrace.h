/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/

#ifndef ASPTRACE_H
#define ASPTRACE_H

#include <list>

#include "sharedPtr.h"
#include "AspUtil.h"
#include "AspDataInfo.h"

#define ADD_TRACE	1
#define SUB_TRACE	2
#define REP_TRACE	3

class AspTrace;
typedef boost::shared_ptr<AspTrace> spAspTrace_t;
extern spAspTrace_t nullAspTrace;

class AspTrace
{
public:
    int id;			// Unique ID for this gframe

    AspTrace(int traceInd, string key, string path, int index, int npts, double min, double max);
    AspTrace(char words[MAXWORDNUM][2*MAXSTR], int nw);
    AspTrace(char *str);
    ~AspTrace();

    void initTrace();
    string colorStr;
    double scale; // vertical scaling in addition to vs
    string dataKey; // key for fdf data
    int dataInd; // element of multi trace data 
    string path; // fdf path
    int traceInd; // reflect order of trace creation
    double vp; // vertical offset factor (multipled to vo).
    string label; // user specified label (if empty, dataKey will be displayed) 
    int labelFlag; // show/hide traceInd or label
    string cmd; // such as "load", "load_dc",...
    string rootPath;

    double getMminX();
    double getMmaxX();
    double getMinX();
    double getMaxX();
    void setMinX(double val);
    void setMaxX(double val);
    int getNpts();
    int getTotalpts();
    float *getData();
    float *getFirstDataPtr();
    float getAmp(double freq);
    string getColor();
    int setColor();
    int getFirst() {return first;}
    int getLast() {return last;}
    int val2dpt(double val);
    double dpt2val(int dpt);

    double getPPP();
    void setShift(double shift);
    double getShift() {return shift;}

    bool selected;
    string getLabel(spAspDataInfo_t dataInfo);
    string getIndex();
    string getKeyInd();
    string toString();
    string toString(string root);
    void sumTrace(spAspTrace_t trace, int flag = ADD_TRACE);

    list<spAspTrace_t> *getSumTraceList();
    bool ismultiColor();

    void dodc();
    double dcMargin;

    double getInteg(double freq1, double freq2);
    void save(string path);
    void setBCModel(float *model, int n);
    void doBC();
    void undoBC();
    float *getBCModel() {return bcModel;}
    bool getDoneBC() {return doneBC;}

private:
    double mminX, mmaxX; // min, max PPM of entire spectral data 
    double minX, maxX; // min, max PPM of used spectral data 
    int npts, first, last;
    float *sumData;
    float *bcModel;
    bool doneBC;
    // backup in order to restore session (minX,maxX,scale may be changed by sumTrace) 
    double input_minX, input_maxX, input_scale; 
    
    list<spAspTrace_t> *sumTraceList;
    void makeTrace(char words[MAXWORDNUM][2*MAXSTR], int nw);

    double shift; // trace can have a horizontal shift.
		// this is for display only, i.e, trace is shifted realtive to FOV
		// i.e., add shift to minX, maxX when display the trace.
		// In AspTrace class, shift is applyed when adding trace in sumTrace.
		// The shift is also added to peak frequencies.
};

#endif /* ASPTRACE_H */
