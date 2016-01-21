/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/

#ifndef ASPINTEG_H
#define ASPINTEG_H

#include "sharedPtr.h"
#include "AspUtil.h"
#include "AspCursor.h"
#include "AspCell.h"
#include "AspDataInfo.h"
#include "AspAnno.h"
#include "AspTrace.h"

class AspInteg : public AspAnno
{
public:

    AspInteg(int index, double freq1, double freq2, double amp);
    AspInteg(char words[MAXWORDNUM][MAXSTR], int nw);
    ~AspInteg();

    void init();
    void display(spAspCell_t cell, spAspTrace_t trace, spAspDataInfo_t dataInfo,int integFlag,
	double scale, double off);

    string toString();
    string getKey();

    int select(spAspCell_t cell, int x, int y);
    void modify(spAspCell_t cell, int x, int y, int prevX, int prevY);
    void modifyVert(spAspCell_t cell, int x, int y, int prevX, int prevY);
    void getLabel(spAspDataInfo_t dataInfo, string &lbl, int &cwd, int &cht);
    void getValue(spAspDataInfo_t dataInfo, string &lbl, int &cwd, int &cht);

    string dataID;
    float *m_data;
    int m_datapts;
    double m_scale, m_yoff;
    double absValue,normValue;

private:
};

typedef boost::shared_ptr<AspInteg> spAspInteg_t;
extern spAspInteg_t nullAspInteg;

#endif /* ASPINTEG_H */
