/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/

#ifndef ASPLINE_H
#define ASPLINE_H

#include "AspAnno.h"

class AspLine : public AspAnno
{ 

public:

    AspLine(spAspCell_t cell, int x, int y);
    AspLine(char words[MAXWORDNUM][MAXSTR], int nw);

    void create(spAspCell_t cell, int x, int y);

    void display(spAspCell_t cell, spAspDataInfo_t dataInfo);

    int select(int x, int y);

private:

};

#endif /* ASPLINE_H */
