/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/

#ifndef ASPPEAK_H
#define ASPPEAK_H

#include "AspCursor.h"
#include "AspAnno.h"

class AspPeak
{
public:

    AspPeak(spAspCursor_t cursor);
    AspPeak(int ind, spAspCursor_t c);
    AspPeak(char words[MAXWORDNUM][MAXSTR], int nw);
    ~AspPeak();

    void display(spAspCell_t cell, int xoff, int yoff, spAspDataInfo_t dataInfo, int peakFlag);
    void display1d(spAspCell_t cell, int xoff, int yoff, spAspDataInfo_t dataInfo, int peakFlag);
    void display2d(spAspCell_t cell, int xoff, int yoff, spAspDataInfo_t dataInfo, int peakFlag);
    string toString();

    int getIndex() {return index;}
    void setIndex(int ind) {index=ind;}
    int getRank() {return rank;}
    spAspCursor_t getCursor() {return cursor;}
    string getLabel() {return label;}
    Dpoint_t *getLabelPos() {return &labelLoc;}
    Dpoint_t *getAutoPos() {return &autoLoc;}
    double getAmp() {return amplitude;}
    double getHeight() {return height;}
    double getIntegral() {return integral;}
    double getXval();
    void setXval(double x);
    double getYval();

    void setLabel(string str) {label=str;}
    void setLocation(Dpoint_t loc) {labelLoc=loc;}
    void setAutoLocation(Dpoint_t loc) {autoLoc=loc;}
    void setAmp(double d) {amplitude=d;}
    void setHeight(double d) {height=d;}
    void setIntegral(double d) {integral=d;}
    int select(int x, int y);
    void modify(spAspCell_t cell, int x, int y);
    void getLabel(spAspDataInfo_t dataInfo, string &lbl, int &cwd, int &cht);

    int selected;

    string dataID;

private:
    void initPeak();
    int index;
    int rank;
    spAspCursor_t cursor;
    double height;
    double integral;
    double amplitude;
    string label;
    Dpoint_t labelLoc;
    Dpoint_t autoLoc;
  
    string labelStr;
    int labelX,labelY,labelW,labelH;
    int markX,markY,markW,markH;
};

typedef boost::shared_ptr<AspPeak> spAspPeak_t;
extern spAspPeak_t nullAspPeak;

#endif /* ASPPEAK_H */
