/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/

#ifndef ASPCURSOR_H
#define ASPCURSOR_H

#include <string>
#include "sharedPtr.h"
#include "AspUtil.h"

class AspCursor
{
public:

    int rank;
    aspResonance_t *resonances;

    AspCursor(double freq, string name);
    AspCursor(double freq1, double freq2, string name1, string name2);
    ~AspCursor();

    string toString();

private:
};

typedef boost::shared_ptr<AspCursor> spAspCursor_t;
extern spAspCursor_t nullAspCursor;

#endif /* ASPCURSOR_H */
