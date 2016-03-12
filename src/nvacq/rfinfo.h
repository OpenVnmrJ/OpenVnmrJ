/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*
 */
#ifndef INCrfinfoh
#define INCrfinfoh

/* ------------- Make C header file C++ compliant ------------------- */
#ifdef __cplusplus
extern "C" {
#endif

struct _RFInfo {
   int ampHiLow;
   int xmtrHiLow;
   int tunePwr;
};

/* --------- ANSI/C++ compliant function prototypes --------------- */

#if defined(__STDC__) || defined(__cplusplus)



#else
/* --------- NON-ANSI/C++ prototypes ------------  */

#endif

#ifdef __cplusplus
}
#endif

#endif
