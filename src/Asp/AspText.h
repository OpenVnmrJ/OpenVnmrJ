/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/

#ifndef ASPTEXT_H
#define ASPTEXT_H

#include "AspPoint.h"

class AspText : public AspPoint
{ 

public:

    AspText(spAspCell_t cell, int x, int y);
    AspText(char words[MAXWORDNUM][MAXSTR], int nw);

private:

};

#endif /* ASPTEXT_H */
