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

#ifndef INCffileh
#define INCffileh

#ifndef VNMRS_WIN32
#include <inttypes.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/mman.h>
#else  /* win32 includes */
#include <windows.h>
#include "inttypes.h"
#endif

/* ------------- Make C header file C++ compliant ------------------- */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
        long long  offsetAddr;    /* offset address into mmap file */
        char       *filePath;      /* File path that was mmapped */
        int        fd;             /* file descriptor ofopen file */
        int        fileAccess;     /* permit argument passed to mOpen */
} FILE_OBJ;

typedef FILE_OBJ *FILE_ID;

extern FILE_ID fFileOpen(char *filename, uint64_t size, int permit);
extern int fFileClose(FILE_ID pFfileId);
extern long long fFidSeek(FILE_ID pFfileId, int fidNumber, int headerSize, unsigned long bbytes);
extern int fFileWrite(FILE_ID md,char *data, int bytelen, long long offset);
extern int fFileRead(FILE_ID md,char *data, int bytelen, long long offset);
extern void fShow(FILE_ID pFfileId);

/* for routines that seek then use the internal mfile struct to obtain the offeset, etc 
 *  we need to lock this structure if multiple threads are accessing the same mfileObj
 *
 *         Author:  Greg Brissey    4/19/2005
 */

#ifdef __cplusplus
}
#endif

#endif
