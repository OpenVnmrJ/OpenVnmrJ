/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef INCtermhandlerh
#define INCtermhandlerh

#define ASM_SERIAL_PORT 0
#define SMS_SERIAL_PORT 1
#define SMS2_SERIAL_PORT 2
#define GILSON215_SERIAL_PORT 3
#define PATIENT_TABLE_SERIAL_PORT 4

/* ------------- Make C header file C++ compliant ------------------- */
#ifdef __cplusplus
extern "C" {
#endif


#if defined(__STDC__) || defined(__cplusplus)

extern int initPort(char *device, int robotype);
extern int restorePort(int sPort);
extern int flushPort(int sPort);
extern int drainPort(int sPort);

#else
/* --------- NON-ANSI/C++ prototypes ------------  */


extern int initPort();
extern int restorePort();
extern int flushPort();
extern int drainPort();

#endif

#ifdef __cplusplus
}
#endif

#endif

