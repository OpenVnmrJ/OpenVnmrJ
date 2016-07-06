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
#ifndef INCflashUpdateh
#define INCflashUpdateh

/* ------------- Make C header file C++ compliant ------------------- */
#ifdef __cplusplus
extern "C" {
#endif

#ifdef XXX
typedef struct {
                long bufferAddr;
		char filename[FLASH_MAX_STR_SIZE];
		char backupname[FLASH_MAX_STR_SIZE];
		char md5sig[FLASH_MAX_STR_SIZE];
		char msgstr[FLASH_MAX_STR_SIZE];
                unsigned long totalBytes;
	        unsigned long crc32chksum;  /* CRC-32 checksum */
} FLASHFILE_UPDATEINFO;
 
typedef FLASHFILE_UPDATEINFO *FLASH_INFO_ID;
#endif
 
#define MAX_FLASH_UPDATE_MSG_STR 256

typedef struct _flash_update_ {
    int cmd;
    long filesize;
    long arg2;
    long arg3;
    unsigned long crc32chksum;
    char msgstr[MAX_FLASH_UPDATE_MSG_STR+2];
} FLASH_UPDATE_MSG;


/* --------- ANSI/C++ compliant function prototypes --------------- */

#if defined(__STDC__) || defined(__cplusplus)

extern MSG_Q_ID startFlashUpdate(int priority,int options,int stacksize);

#else
/* --------- NON-ANSI/C++ prototypes ------------  */

#endif

#ifdef __cplusplus
}
#endif

#endif
