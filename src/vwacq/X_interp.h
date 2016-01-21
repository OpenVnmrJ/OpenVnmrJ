/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef INCxinterph
#define INCxinterph


/* ------------- Make C header file C++ compliant ------------------- */
#ifdef __cplusplus
extern "C" {
#endif



/* --------- ANSI/C++ compliant function prototypes --------------- */

#if defined(__STDC__) || defined(__cplusplus)

extern establishShimType();
extern ladcHandler();
extern shimHandler();
extern spnrHandler();
extern syncHandler();
extern statHandler();
extern statTimer();
extern nvRamHandler();
extern tuneHandler();
extern autshmHandler();
extern rcvrgainHandler();
extern fixAcodeHandler();
extern setAttnHandler();
extern setvtHandler();
extern shmsetHandler();
extern lkfreqHandler();

extern anop( int val);

#else                                                   
/* --------- NON-ANSI/C++ prototypes ------------  */

extern establishShimType();
extern ladcHandler();
extern shimHandler();
extern spnrHandler();
extern syncHandler();
extern statHandler();
extern statTimer();
extern nvRamHandler();
extern tuneHandler();
extern autshmHandler();
extern rcvrgainHandler();
extern fixAcodeHandler();
extern setAttnHandler();
extern setvtHandler();
extern shmsetHandler();
extern lkfreqHandler();

extern anop();

#endif

#ifdef __cplusplus
}
#endif

#endif

