/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/

#ifndef ASPREGION_H
#define ASPREGION_H

#include "sharedPtr.h"
#include "AspUtil.h"
#include "AspCursor.h"
#include "AspCell.h"
#include "AspDataInfo.h"
#include "AspAnno.h"
#include "AspTrace.h"

class AspRegion : public AspAnno
{
public:

    AspRegion(int index, double xval, double yval);
    AspRegion(char words[MAXWORDNUM][MAXSTR], int nw);
    ~AspRegion();

    void init();
    void display(spAspCell_t cell, spAspTrace_t trace, spAspDataInfo_t dataInfo);

    string toString();

    int select(spAspCell_t cell, int x, int y);
    void modify(spAspCell_t cell, int x, int y, int prevX, int prevY);

    string dataID;

private:
};

typedef boost::shared_ptr<AspRegion> spAspRegion_t;
extern spAspRegion_t nullAspRegion;

#endif /* ASPREGION_H */
