/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/

#ifndef ASPPOINT_H
#define ASPPOINT_H

#include "AspAnno.h"

class AspPoint : public AspAnno
{ 

public:

    AspPoint(spAspCell_t cell, int x, int y, bool trCase=false);
    AspPoint(char words[MAXWORDNUM][MAXSTR], int nw);

    void create(spAspCell_t cell, int x, int y);

    void display(spAspCell_t cell, spAspDataInfo_t dataInfo);

private:

};

#endif /* ASPPOINT_H */
