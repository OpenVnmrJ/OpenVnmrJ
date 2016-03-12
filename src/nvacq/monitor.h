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
#ifndef INCmonitorh
#define INCmonitorh

#define MAX_MONITOR_MSG_STR 256

/* ------------- Make C header file C++ compliant ------------------- */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct _monitor_update {
    long cmd;
    long arg1;
    long arg2;
    long arg3;
    long arg4;
    long arg5;
    unsigned long crc32chksum;
    struct {
    	unsigned int len;
    	char val[MAX_MONITOR_MSG_STR+2] ;
    } msgstr;
} MONITOR_MSG;

#ifdef __cplusplus
}
#endif

#endif
