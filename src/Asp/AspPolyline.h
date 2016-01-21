/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/

#ifndef ASPPOLYLINE_H
#define ASPPOLYLINE_H

#include "AspPolygon.h"

class AspPolyline : public AspPolygon
{ 

public:

    AspPolyline(spAspCell_t cell, int x, int y);
    AspPolyline(char words[MAXWORDNUM][MAXSTR], int nw);

private:

};

#endif /* ASPPOLYLINE_H */
