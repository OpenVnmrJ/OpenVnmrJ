/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/

#ifndef ASPCELL_H
#define ASPCELL_H


#include <list>

#include "sharedPtr.h"
#include "AspUtil.h"
#include "AspDataInfo.h"
#include <map>

class AspCell
{
public:
    int id;			// Unique ID for this gframe

    AspCell(double px, double py, double pw, double ph, double vx, double vy, double vw, double vh);
    AspCell(double px, double py, double pw, double ph);
    ~AspCell();

    void setAxisNames(string xname, string yname);
    void setAxisLabels(string xlabel, string ylabel);

        double pix2val(int dim, double d, bool mm=false);
        double val2pix(int dim, double d, bool mm=false);
    double pix2mm(int dim, double pix);
    double mm2pix(int dim, double mm);

    bool select(int x, int y);

    void getPixCell(double &px, double &py, double &pw, double &ph);
    void getValCell(double &vx, double &vy, double &vw, double &vh);
    string getXname() {return xname;}
    string getYname() {return yname;}

    void drawPolyline(float *data, int npts, int step, double vx, double vy, double vw, double vh, int color, double vScale, double yoff);
    void showAxis(int axflag, int mode);
    void setDataInfo(spAspDataInfo_t info) {dataInfo=info;}
    spAspDataInfo_t getDataInfo() { return dataInfo;}
    double getCali(int dim);

private:
    double pstx, psty, pwd, pht;
    double vstx, vsty, vwd, vht;
    bool selected;
    string xname,yname;
    string xlabel,ylabel;
    spAspDataInfo_t dataInfo;
};

typedef boost::shared_ptr<AspCell> spAspCell_t;
extern spAspCell_t nullAspCell;

#endif /* ASPCELL_H */
