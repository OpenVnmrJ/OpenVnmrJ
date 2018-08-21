/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/
/* 
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "imagelist.h"

Create_ListClass(Imagelist);

ImagelistLink::~ImagelistLink() {}

ImagelistLink& ImagelistLink::Print() {
  printf("object[%d]\n", item);
  return *this;
}

Imagelist::Imagelist(char *newpath)
{
    filepath = strdup(newpath);
}

Imagelist::~Imagelist()
{
    if (filepath){
	free(filepath);
    }
}

char *
Imagelist::Filename()
{
    return filepath;
}
