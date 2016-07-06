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
#ifndef INCtuneh
#define INCtuneh

/* ------------- Make C header file C++ compliant ------------------- */
#ifdef __cplusplus
extern "C" {
#endif

#define  TUNE_TIMEOUT	5

#define MAX_TUNE_MSG_STR 256

typedef struct _tune_update_ {
    int cmd;
    long channel;
    long arg2;
    long arg3;
    unsigned long crc32chksum;
    char msgstr[MAX_TUNE_MSG_STR+2];
} TUNE_MSG;


/* --------- ANSI/C++ compliant function prototypes --------------- */

#if defined(__STDC__) || defined(__cplusplus)



#else
/* --------- NON-ANSI/C++ prototypes ------------  */

#endif

#ifdef __cplusplus
}
#endif

#endif
