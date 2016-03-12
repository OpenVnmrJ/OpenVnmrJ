/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* mboxObj.h Copyright (c) 1994-1996 Varian Assoc.,Inc. All Rights Reserved */
/*
 */

/* #define _POSIX_SOURCE /* defined when source is waited tobe POSIX-compliant */
/* #define _SYSV_SOURCE /* defined when source is System V */
/* #ifdef __STDC__ /* used to determine if using an ANSI compiler */


#ifndef INCmboxobjh
#define INCmboxobjh


/* ------------- Make C header file C++ compliant ------------------- */
#ifdef __cplusplus
extern "C" {
#endif

/* --------- ANSI/C++ compliant function prototypes --------------- */

#if defined(__STDC__) || defined(__cplusplus)

extern int  mboxInit();
extern int mboxDelete();
extern char autoStatReg();
extern void m32DevCntrlReg(char value);
extern int mboxGenGetMsg(char *msgbuffer, int size);
extern int mboxGenPutMsg(char *msgbuffer, int size);
extern void mboxGenMsgComplete(int stat);
extern int mboxSPinGetMsg(char *msgbuffer, int size);
extern void mboxSpinMsgComplete(int stat);
extern int mboxVTGetMsg(char *msgbuffer, int size,int timeout);
extern void mboxVTMsgComplete(int stat);
extern int mboxShimGetMsg(char *msgbuffer, int size);
extern void mboxShimMsgComplete(int stat);
extern int mboxShimApGetMsg(char *msgbuffer);
extern void mboxShow(int level);

#else
/* --------- NON-ANSI/C++ prototypes ------------  */

extern int  mboxInit();
extern int mboxDelete();
extern char autoStatReg();
extern void m32DevCntrlReg();
extern int mboxGenGetMsg();
extern int mboxGenPutMsg();
extern void mboxGenMsgComplete();
extern int mboxSpinGetMsg();
extern void mboxSpinMsgComplete();
extern int mboxVTGetMsg();
extern void mboxVTMsgComplete();
extern int mboxShimGetMsg();
extern void mboxShimMsgComplete();
extern int mboxShimApGetMsg();
extern void mboxShow();

#endif

#ifdef __cplusplus
}
#endif

#endif

