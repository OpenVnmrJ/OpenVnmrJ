/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef INCmsgqlibh
#define INCmsgqlibh

#ifndef VNMRS_WIN32   /* Windows native compilation define */

#include "ipcMsgQLib.h"
#include "shrMLib.h"

#endif  /* VNMRS_WIN32 */

/* ------------- Make C header file C++ compliant ------------------- */
#ifdef __cplusplus
extern "C" {
#endif


#ifndef VNMRS_WIN32

/* System V Async Message Queue structure */

typedef struct _msgQobj {
			char 		       *msgQprocName;
		 	IPC_MSG_Q_ID 		msgId;
  			SHR_MEM_ID		shrmMsgQInfo;
			key_t			ipcKey;
			pid_t			pid;
			int			maxSize;
			int 		AsyncFlag;
        	     } MSG_Q_OBJ;

typedef MSG_Q_OBJ *MSG_Q_ID;

typedef struct _msgQdbm { 
			char procName[16];
			key_t ipcKey;
			pid_t pid;
			int maxSize;
			int AsyncFlag;
	       } MSG_Q_DBM_ENTRY;



typedef MSG_Q_DBM_ENTRY  *MSG_Q_DBM_ID;

#define MSQ_Q_DBM_ESIZE (sizeof(MSG_Q_DBM_ENTRY))
#define MSQ_Q_DBM_SIZE (sizeof(MSG_Q_DBM_ENTRY) * PROCIPC_DB_SIZE)

#define MSGQ_DBM_PATH "/tmp/IPC_Proc_Info"
#define MSGQ_DBM_KEY  26330

#else  /* +++++++++++++++ VNMRS_WIN32 +++++++++++++++++++++++++++++ */

#include "rngBlkLib.h"

#define MSG_STR (WM_USER + 1)
#define WINDOWS_NAME_SUFIX "_Msg_Window"
typedef void (*MSGCALLBACK)(void *);


typedef struct _msgQobj {
			char 	   *msgQprocName;   /* proc name, e.g. "Expproc" */
			char       *msgQWindowName;  /* registered msgQ Window name, e.g. "Expproc_MSG_WINDOW" */
			HINSTANCE  hInstance;
	                ATOM       registeredWindowClass;  /* for deregister */
	                HWND       msgWindowHandle;
//                      MSGCALLBACK pMsgCallback;
			RINGBLK_ID  pMsgRingBuf;
			int			maxSize;
			int 		AsyncFlag;
        	     } MSG_Q_OBJ;

typedef MSG_Q_OBJ *MSG_Q_ID;


#endif /* VNMRS_WIN32 */
/* --------- ANSI/C++ compliant function prototypes --------------- */
 
#if defined(__STDC__) || defined(__cplusplus) || defined(VNMRS_WIN32)
extern MSG_Q_ID createMsgQ(char *myProcName,int keyindex, int msgSize);
extern int 	recvMsgQ(MSG_Q_ID pMsgQ, char *buffer, int len, int mode);
extern int 	msgesInMsgQ(MSG_Q_ID pMsgQ);
extern int 	deleteMsgQ(MSG_Q_ID pMsgQ);
extern MSG_Q_ID openMsgQ(char *msgQIdStr);
extern int 	sendMsgQ(MSG_Q_ID pMsgQ, char *message, int len, int priority, int waitflg);
extern int 	setMsgQAsync( MSG_Q_ID pMsgQ, void (*callback)() );
extern int	closeMsgQ(MSG_Q_ID pMsgQ);
 
#else
/* --------- NON-ANSI/C++ prototypes ------------  */

extern MSG_Q_ID createMsgQ();
extern int 	recvMsgQ();
extern int 	msgesInMsgQ();
extern int 	deleteMsgQ();
extern MSG_Q_ID openMsgQ();
extern int 	sendMsgQ();
extern int 	setMsgQAsync();
extern int	closeMsgQ();
 

#endif

 
#ifdef __cplusplus
}
#endif
 
#endif
