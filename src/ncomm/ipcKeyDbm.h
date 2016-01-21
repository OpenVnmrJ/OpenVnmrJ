/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef INCipckeydbmh
#define INCipckeydbmh

#include "shrMLib.h"

#ifdef XXX
#define Expproc_Key 	((int) 1)
#define RecvProc_Key	((int) 2)
#define SendProc_Key 	((int) 3)
#define Procproc_Key 	((int) 4)
#define Roboproc_Key 	((int) 5)
#define Autoproc_Key 	((int) 6)
#define StartUser_Key 	((int) 100)
#endif

#define IPCKEY_MAXSTR_LEN 256

/* ------------- Make C header file C++ compliant ------------------- */
#ifdef __cplusplus
extern "C" {
#endif


/* System V IPC Key DataBase structure */

typedef struct _ipcKeyDbmData {
		char idstr[IPCKEY_MAXSTR_LEN];  /* Id String of process */
		pid_t pid;        /* Current PID of Process */
		key_t ipcKey;    /* Process System V IPC Key */
		int  pidActive;   /* TRUE is PID is Acitve */
		int  asyncFlag;   /* TRUE is MsgQ is setup for Async */
		int  maxLenMsg;   /* maximum len message on MsgQ */
		char path[IPCKEY_MAXSTR_LEN];	  /* File Path */
          } IPC_KEY_DBM_DATA;

typedef struct _ipcKeyDbm {
		char *filepath;	  /* File Path to DataBase */
		int  entries;	  /* Number of Key Entries in DBM */
                SHR_MEM_ID  dbmdata;  /* Share Memory Obj Id */
          } IPC_KEY_DBM_OBJ;
 
typedef IPC_KEY_DBM_OBJ *IPC_KEY_DBM_ID;
 
/* --------- ANSI/C++ compliant function prototypes --------------- */

#if defined(__STDC__) || defined(__cplusplus)                          

extern IPC_KEY_DBM_ID  ipcKeyDbmCreate(char* filename, int keyId, int dbmEntries );
extern void ipcKeyDbmClose(IPC_KEY_DBM_ID dbmId);
extern int ipcKeyGet(IPC_KEY_DBM_ID dbmId, char* idstr,IPC_KEY_DBM_DATA* entry);
extern int ipcKeySet(IPC_KEY_DBM_ID dbmId, char* idstr, int maxlen, int pid,int keyindex, char *filename);
extern int ipcKeySetAsync(IPC_KEY_DBM_ID dbmId, char* idstr);

#else                                  
/* --------- NON-ANSI/C++ prototypes ------------  */
 
extern IPC_KEY_DBM_ID  ipcKeyDbmCreate();
extern void ipcKeyDbmClose();
extern int ipcKeyGet();
extern int ipcKeySet();
extern int ipcKeySetAsync();
extern void  ipcKeyDbmShow();
 
#endif
 

#ifdef __cplusplus
}
#endif                   

#endif 
