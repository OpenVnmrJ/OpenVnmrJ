/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/

#ifndef ASPPOLYGON_H
#define ASPPOLYGON_H

#include "AspAnno.h"

class AspPolygon : public AspAnno
{ 

public:

    AspPolygon(spAspCell_t cell, int x, int y);
    AspPolygon(char words[MAXWORDNUM][MAXSTR], int nw);

    void display(spAspCell_t cell, spAspDataInfo_t dataInfo);

    void create(spAspCell_t cell, int x, int y);

private:

};

#endif /* ASPPOLYGON_H */
