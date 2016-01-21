/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/

#ifndef ASPBAR_H
#define ASPBAR_H

#include "AspLine.h"

class AspBar : public AspLine
{ 

public:

    AspBar(spAspCell_t cell, int x, int y);
    AspBar(char words[MAXWORDNUM][MAXSTR], int nw);

    void display(spAspCell_t cell, spAspDataInfo_t dataInfo);

    void getLabel(spAspDataInfo_t dataInfo, string &lbl, int &cwd, int &cht);

private:

};

#endif /* ASPBAR_H */
