/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef SIBTASK_H
#define SIBTASK_H

#include "sibObj.h"

#ifdef __cplusplus
extern "C" {
#endif
    
#if defined(__STDC__) || defined(__cplusplus)
    /* --------- ANSI/C++ function prototypes --------------- */
    IMPORT int startSibTask(int, int, int);
    IMPORT VOID sibTrip(SIB_OBJ *);
    IMPORT int sibStatusCheck(SIB_OBJ *, int);
    IMPORT int sibPostErrors(SIB_OBJ *, int, int);
    IMPORT int sibPostBypassWarnings(SIB_OBJ *);
#else
    /* --------- K&R prototypes ------------  */
    IMPORT int startSibTask();
    IMPORT VOID sibTrip();
    IMPORT int sibStatusCheck();
    IMPORT int sibPostErrors();
    IMPORT int sibPostBypassWarnings();
#endif /* __STDC__ */
 
#ifdef __cplusplus
}
#endif

#endif /* SIBTASK_H */
