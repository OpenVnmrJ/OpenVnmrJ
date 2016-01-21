/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/

#ifndef AIPSPECVIEWLIST_H
#define AIPSPECVIEWLIST_H

#include <map>
#include <string>
#include <utility>
#include <list>
using std::list;
#include <set>
using std::set;

#include "aipSpecView.h"
#include "aipSpecStruct.h"

#define CSILAYOUT 0
#define HORIZLAYOUT 1
#define VERTLAYOUT 2
#define GRIDLAYOUT 3
#define STACKLAYOUT 4

typedef std::map<int, spSpecView_t> SpecViewMap;

class SpecViewList 
{
public:

    SpecViewList(int id);
    ~SpecViewList();
    void setSpecKeys(std::list<string> keys); 
    void setSpecViews(graphInfo_t *gInfo, int n); 
    void selectSpecView(std::list<int> selectedViews); 
    int selectSpecView(int x, int y);
    void displaySpec(int fx, int fy, int fw, int fh);
    bool showSpec();
    void clearList();
    graphInfo_t *getGraphInfo(int index);
    void setLayoutType(int layout) {layoutType=layout;}
    int getLayoutType() {return layoutType;}
    void setSelSpec(bool b) {selSpec=b;} 
    bool getSelSpec() {return selSpec;} 
    string getFirstKey();
    void getSpecKeys(std::list<string> *keys);
    int getSliceInd() { return sliceInd;}
    void setSliceInd(int ind) { sliceInd = ind;}
    int getRows() { return rows;}
    void setRows(int value) { rows = value;}
    int getCols() { return cols;}
    void setCols(int value) { cols = value;}
    bool hasSpec();
    void showSpecView(int ind);
    void setRoi(bool b);
    specStruct_t *get2DCSISpecStruct(string key);

protected:
    
private:

    SpecViewMap *specViewMap;
    std::list<string> dataKeys;
    bool showGrid;
    bool show3DGrid;
    bool showNumber;
    bool selSpec;
    int layoutType;
    int specColors[12];
    int gridColors[4];
    int sliceInd;
    int rows, cols; // used by aipShowSpec

    int frameID;
    void initColors();
    void drawSpec(string key, int fx, int fy, int fw, int fh, int color);
    void drawFid(int fx, int fy, int fw, int fh, int color);
    void drawPhasefile(string key, int fx, int fy, int fw, int fh, int color);
    void drawGrid(int fx, int fy, int fw, int fh);
    void drawSelectedGrid(int fx, int fy, int fw, int fh);
    void drawNumber(int fx, int fy, int fw, int fh);
};

#endif /* AIPSPECVIEWLIST_H */
