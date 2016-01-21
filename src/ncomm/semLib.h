/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef INCsemlibh
#define INCsemlibh

#ifndef VNMRS_WIN32
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
typedef int SEM_HANDLE;
#else
#include <Windows.h>
typedef HANDLE SEM_HANDLE;
#endif

#define  SEM_RECORD_SIZE 50 

/* ------------- Make C header file C++ compliant ------------------- */
#ifdef __cplusplus
extern "C" {
#endif

/* --------- ANSI/C++ compliant function prototypes --------------- */
#if defined(__STDC__) || defined(__cplusplus)
extern SEM_HANDLE semCreate(key_t key, int initval);
extern SEM_HANDLE semOpen(key_t key, int initval);
extern void semDelete(SEM_HANDLE id);
extern void semClose(SEM_HANDLE id);
extern void semTake(SEM_HANDLE id);
extern void semGive(SEM_HANDLE id);


#else
/* --------- NON-ANSI/C++ prototypes ------------  */

extern int semCreate();
extern int semOpen();
extern void semDelete();
extern void semClose();
extern void semTake();
extern void semGive();

#endif

 
#ifdef __cplusplus
}
#endif
 
#endif
