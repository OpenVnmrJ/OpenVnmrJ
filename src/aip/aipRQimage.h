/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/

#ifndef AIPRQIMAGE_H
#define AIPRQIMAGE_H

#include "aipRQnode.h"

class RQimage : public RQnode 
{
public:

    RQimage();
    RQimage(string);
    RQimage(RQnode *, string);

    void getIndex4Key(string key, int *rank, int *slice, int *array_index, int *echo);

    string getIndexStr();

    string getStudyPath();
    string getGroupPath();

protected:

private:

    void initImage(string str = "");
};

#endif /* AIPRQIMAGE_H */
