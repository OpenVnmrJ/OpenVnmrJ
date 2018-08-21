/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/

#ifndef ASPROI_H
#define ASPROI_H

#include "sharedPtr.h"
#include "AspCursor.h"
#include "AspCell.h"

class AspRoi
{
public:

    int id;
    spAspCursor_t cursor1;
    spAspCursor_t cursor2;
    string label;
    int color;
    int opaque;
    int height; // % of pixht for 1D roi.
    bool show;

    AspRoi(spAspCursor_t cursor1, spAspCursor_t cursor2, int ind);
    ~AspRoi();

    int getRank();

    void draw(spAspCell_t cell);
    bool getRoiBox(spAspCell_t cell,double &vx, double &vy, double &vw, double &vh);
    bool select(spAspCell_t cell, int x, int y, bool handle=false);
    void selectHandle(spAspCell_t cell, int x, int y, bool handle=false);
    void modify(spAspCell_t cell, int x, int y, int prevx, int prevy);

    bool selected; // not used
    int mouseOver;
    void setColor(char *name);
    int getColor();
    void modifyRoi(double vx, double vy, double vxPrev, double vyPrev);
    bool mouseOverChanged;

private:
};

typedef boost::shared_ptr<AspRoi> spAspRoi_t;
extern spAspRoi_t nullAspRoi;

#endif /* ASPROI_H */
