/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef AIPCFUNCS_H
#define AIPCFUNCS_H

#include "aipDataStruct.h"

#ifdef	__cplusplus
extern "C" {
#endif

/* --------- ANSI/C++ compliant function prototypes --------------- */

#if defined(__STDC__) || defined(__cplusplus)

void aipInsertData(dataStruct_t *data);
void *aipGetCommandTable();

#else

/* --------- NON-ANSI/C++ prototypes ------------  */

void aipInsertData();
void *aipGetCommandTable();

#endif
 
#ifdef	__cplusplus
}
#endif

#endif /* (not) AIPCFUNCS_H */
