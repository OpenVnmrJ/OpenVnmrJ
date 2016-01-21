/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/

#ifndef AIPRQSTUDY_H
#define AIPRQSTUDY_H

#include "aipRQnode.h"
#include "aipRQgroup.h"

class RQstudy : public RQnode 
{
public:

    RQstudy();
    RQstudy(string);

protected:

private:

    void initStudy(string str = "");

};

#endif /* AIPRQSTUDY_H */
