/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef INCmfileh
#define INCmfileh

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

#ifdef __INTERIX
typedef unsigned long long int uint64_t;
#endif

#define SHR_EXPQ_PATH        "/tmp/ExpQs"
#define SHR_ACTIVE_EXPQ_PATH "/tmp/ExpActiveQ"
#define SHR_EXP_STATUS_PATH  "/tmp/ExpStatus"
#define SHR_SEM_USAGE_PATH   "/tmp/IPC_V_SEM_DBM"
#define SHR_MSG_Q_DBM_PATH   "/tmp/msgQKeyDbm"
#define SHR_PROCQ_PATH       "/tmp/ProcQs"
#define SHR_ACTIVEQ_PATH     "/tmp/ActiveQ"

#define SHR_EXPQ_KEY         ((int) 101)
#define SHR_ACTIVE_EXPQ_KEY  ((int) 201)
#define SHR_EXP_STATUS_KEY   ((int) 301)
#define SHR_SEM_USAGE_KEY    ((int) 401)
#define SHR_MSG_Q_DBM_KEY    ((int) 501)
#define SHR_PROCQ_KEY        ((int) 601)
#define SHR_ACTIVEQ_KEY      ((int) 701)
#define SHR_EXP_INFO_KEY     ((int) 24484)
#define SHR_EXP_INFO_RW_KEY  ((int) 24485)


/* ------------- Make C header file C++ compliant ------------------- */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
        uint64_t   byteLen;        /* byte length of file mmapped */
        uint64_t   newByteLen;     /* length (bytes) written to file */
        uint64_t   firstByte;      /* offset to first byte of mapped section */
        char       *mapStrtAddr;   /* mmap file starting address */
        char       *offsetAddr;    /* offset address into mmap file */
        char       *filePath;      /* File path that was mmapped */
        unsigned int mapLen;         /* length (page mult) of file mmapped */
        int        fileAccess;     /* permit argument passed to mOpen */
        int        fd;             /* file descriptor of opened file */
        int        multiMap;       /* large files are mapped in sections */
        int        mmapProt;       /* access protection for mmap */
        int        pageSize;       /* page size needed by mmap */
#ifdef  VNMRS_WIN32
		HANDLE     fileHandle;     /* win32 file handle */
		HANDLE     mapHandle;      /* win32 file mapping handle */
		DWORD      mapLenHW;	       /* high word of size */
		DWORD      mapLenLW;        /* low word of size  */
#endif

#ifdef  THREADED
/* for routines that seek then use the internal mfile struct to obtain the offeset, etc 
 *  we need to lock this structure if multiple threads are accessing the same mfileObj
 *
 *         Author:  Greg Brissey    4/19/2005
 */
#ifndef  VNMRS_WIN32
	pthread_mutex_t   mutex;   /* access protection in a threaded environment */
#else
    CRITICAL_SECTION CriticalSection;  /* Windows access protection in a threaded environment */
#endif
#endif

} MFILE_OBJ;

typedef MFILE_OBJ *MFILE_ID;

#define MF_NORMAL  	MADV_NORMAL
#define MF_RANDOM  	MADV_RANDOM
#define MF_SEQUENTIAL  	MADV_SEQUENTIAL

extern MFILE_ID mOpen(char* filename, uint64_t size, int permit);
extern int mClose(MFILE_ID pMfileId);
extern int mAdvise(MFILE_ID md,int advice);
extern void mShow(MFILE_ID pMfileId);
extern int mFidSeek(MFILE_ID pMfileId, int fidNumber, int headerSize, unsigned long bbytes);

/* for routines that seek then use the internal mfile struct to obtain the offeset, etc 
 *  we need to lock this structure if multiple threads are accessing the same mfileObj
 *
 *         Author:  Greg Brissey    4/19/2005
 */
/* if not compiled with THREADED then these are just dummy routines */
extern int mMutexLock(MFILE_ID pMfileId);
extern int mMutexUnlock(MFILE_ID pMfileId);

#ifdef __cplusplus
}
#endif

#endif
