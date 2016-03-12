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
#ifndef INCnexush
#define INCnexush

#define MAX_MONITOR_MSG_STR 256

/* ------------- Make C header file C++ compliant ------------------- */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct _cntlr_comm_msg {
    char cntlrId[16];
    int cmd;
    int errorcode;
    int warningcode;
    long arg1;
    long arg2;
    long arg3;
    unsigned long crc32chksum;
    char msgstr[COMM_MAX_STR_SIZE];
} CNTLR_COMM_MSG;


extern int send2Master(int cmd,int arg1,int arg2,int arg3,char *strmsg);
extern int send2AllCntlrs(int cmd, int arg1,int arg2,int arg3,char *strmsg);
extern void initialExceptionComm(void *callbackFunc);
extern int sendException(int cmd, int arg1,int arg2,int arg3,char *strmsg);
extern int rollcall();

#ifdef __cplusplus
}
#endif

#endif
