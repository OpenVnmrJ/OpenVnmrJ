/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef INChostmsgchanh
#define INChostmsgchanh

#define MSG_Q_DBM_PATH "/tmp/msgQKeyDbm"
#define MSG_Q_DBM_SIZE 20

#define EXPPROC 0
#define RECVPROC 1
#define SENDPROC 2
#define PROCPROC 3
#define ROBOPROC 4
#define AUTOPROC 5
#define ATPROC   6
#define SPULPROC 7

#define EXP_MSG_SIZE  256
#define RECV_MSG_SIZE 256
#define SEND_MSG_SIZE 256
#define PROC_MSG_SIZE 256
#define ROBO_MSG_SIZE 256
#define AUTO_MSG_SIZE 256
#define AT_MSG_SIZE   256
#define SPUL_MSG_SIZE 256

/*  Used in sendIpcMsg.c  */
/* Same order as defined by EXPPROC, RECVPROC, SENDPROC, PROCPROC, ROBOPROC, etc... */
#define EXP_RECORD { NULL, "Expproc", EXP_MSG_SIZE }
#define RECV_RECORD { NULL, "Recvproc", RECV_MSG_SIZE }
#define SEND_RECORD { NULL, "Sendproc", SEND_MSG_SIZE }
#define PROC_RECORD { NULL, "Procproc", PROC_MSG_SIZE }
#define ROBO_RECORD { NULL, "Roboproc", ROBO_MSG_SIZE }
#define AUTO_RECORD { NULL, "Autoproc", AUTO_MSG_SIZE }
#define AT_RECORD   { NULL, "Atproc",   AT_MSG_SIZE }
#define SPUL_RECORD { NULL, "Spulproc", SPUL_MSG_SIZE }
#define NULL_RECORD { NULL, " ", 0 }

/* keys with ftok() */
#define EXPPROC_MSGQ_KEY  11
#define RECVPROC_MSGQ_KEY 22
#define SENDPROC_MSGQ_KEY 33
#define PROCPROC_MSGQ_KEY 44
#define ROBOPROC_MSGQ_KEY 55
#define AUTOPROC_MSGQ_KEY 66
#define ATPROC_MSGQ_KEY   77
#define SPULPROC_MSGQ_KEY 88

/* direct IPC Keys fro msgQs */
/* Last two digits used by ipcMsgQLib.c as index into proc name table
   so don't change em
*/
#define EXPPROC_MSGQ_IPCKEY  0x26330000
#define RECVPROC_MSGQ_IPCKEY 0x26330001
#define SENDPROC_MSGQ_IPCKEY 0x26330002
#define PROCPROC_MSGQ_IPCKEY 0x26330003
#define ROBOPROC_MSGQ_IPCKEY 0x26330004
#define AUTOPROC_MSGQ_IPCKEY 0x26330005
#define ATPROC_MSGQ_IPCKEY   0x26330006
#define SPULPROC_MSGQ_IPCKEY 0x26330007

#define EXPPROC_IPC_DB { "Expproc", EXPPROC_MSGQ_IPCKEY, -1, EXP_MSG_SIZE }
#define RECVPROC_IPC_DB { "Recvproc", RECVPROC_MSGQ_IPCKEY, -1, RECV_MSG_SIZE }
#define SENDPROC_IPC_DB { "Sendproc", SENDPROC_MSGQ_IPCKEY, -1, SEND_MSG_SIZE }
#define PROCPROC_IPC_DB { "Procproc", PROCPROC_MSGQ_IPCKEY, -1, PROC_MSG_SIZE }
#define ROBOPROC_IPC_DB { "Roboproc", ROBOPROC_MSGQ_IPCKEY, -1, ROBO_MSG_SIZE }
#define AUTOPROC_IPC_DB { "Autoproc", AUTOPROC_MSGQ_IPCKEY, -1, AUTO_MSG_SIZE }
#define ATPROC_IPC_DB   { "Atproc",   ATPROC_MSGQ_IPCKEY,   -1, AT_MSG_SIZE }
#define SPULPROC_IPC_DB { "Spulproc", SPULPROC_MSGQ_IPCKEY, -1, SPUL_MSG_SIZE }
#define PROC_NULL_RECORD { " ", -1, -1, 0 }

#define PROCIPC_DB_SIZE 9

#define EXPPROC_CHANNEL  1
#define RECVPROC_CHANNEL 2
#define SENDPROC_CHANNEL 3
#define UPDTPROC_CHANNEL 4

#endif 
