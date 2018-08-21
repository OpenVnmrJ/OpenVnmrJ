/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/

#ifndef ASPOVAL_H
#define ASPOVAL_H

#include "AspBox.h"

class AspOval : public AspBox
{ 

public:

    AspOval(spAspCell_t cell, int x, int y);
    AspOval(char words[MAXWORDNUM][MAXSTR], int nw);

private:

};

#endif /* ASPOVAL_H */
