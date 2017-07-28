/*
 * Copyright (C) 2017  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
//============================================================================
// Name        : ProcessProcpar.cpp
// Author      : Varian, Inc.
// Version     : 1.0
// Description : This module contains code that will process the custom tag
//               file that contains any custom DICOM tags that are to be added
//               to the final DICOM image file.
//============================================================================

#include <fstream>
#include <iostream>
using namespace std;

#include <unistd.h>
#include <errno.h>

#include "attrtype.h"
#include "attrothr.h"
#include "attrmxls.h"
#include "transynu.h"
#include "elmconst.h"
#include "mesgtext.h"
#include "ioopt.h"
#include "dcopt.h"
#include "rawsrc.h"

#include "FdfHeader.h"

