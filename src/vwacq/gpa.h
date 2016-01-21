/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef GPA_H
#define GPA_H


/* ------------- Make C header file C++ compliant ------------------- */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    short pxTune;
    short ixTune;
    short pyTune;
    short iyTune;
    short pzTune;
    short izTune;
    int gpaStat;
} GpaHdrInfo;

/* This should be 256 bytes (0x100) */
typedef struct {
    GpaHdrInfo hdr;
    char errCodes[256 - sizeof(GpaHdrInfo)];
} GpaInfo;

#if ( CPU==CPU32 )

#if defined(__STDC__) || defined(__cplusplus)
/* --------- ANSI/C++ compliant function prototypes --------------- */
extern void determineGpaType(void);
extern int setGpaTuning(short cmd, short value);
#else
/* --------- NON-ANSI/C++ prototypes ------------  */
extern void determineGpaType();
extern int setGpaTuning();
#endif

#endif

#ifdef __cplusplus
}
#endif

#endif

