/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* Id: dmaMsgQueue.h,v 1.3 2004/04/04 17:14:09 rthrift Exp rthrift  */
/*
=========================================================================
FILE: dmaMsgQueue.h
=========================================================================
Purpose:
	Provided definitions needed by dmaMsgQueue.c and by functions that
	need to call routines in dmaMsgQueue.c.

Comments:

Author:
	Robert L. Thrift, 2004
=========================================================================
     Copyright (c) 2004, Varian Associates, Inc. All Rights Reserved.
     This software contains proprietary and confidential information
            of Varian Associates, Inc. and its contributors.
  Use, disclosure and reproduction is prohibited without prior consent.
=========================================================================
*/
#ifndef INCdmaMsgQueue_h
#define INCdmaMsgQueue_h
#include <vxWorks.h>		/* General VxWorks definitions				*/
#include <msgQLib.h>		/* Message queue library routines			*/
#include "dmaDrv.h"			/* Defines various structs					*/

/* -- No. of SG list nodes that the SG list node msg queue can hold. -- */
#define SG_MSG_Q_MAX	256

/* - No. of txfr descriptor addresses that txfr desc. queue can hold -- */
#define TX_DESC_MSG_Q_MAX	64

/* ------- No. of msgs that the endDMAQ message queue can hold -------- */
#define MAX_END_MSGS	64

/* ------------- Parameters for the runDMA message queue -------------- */
#define RUN_MSG_Q_MAX	64
#define RUN_MSG_LEN	(sizeof(txDesc_t *))

/* --- No. and size of error msgs that DMA error msg queue can hold --- */
#define MAX_ERR_MSGS    1024
#define MAX_ERR_MSG_LEN 256

/* ---- No. and size of msgs that sendDMA task msg queue can hold ----- */
#define MAX_USER_MSGS	128
#define MAX_USER_MSG_LEN (sizeof(struct dmaRequest))

/* ---------- Global message queue pointers in dmaMsgQueue.c ---------- */
#ifndef DMA_MSG_Q_C
extern  MSG_Q_ID runDMAQ;
extern	MSG_Q_ID dmaErrQ;
extern	MSG_Q_ID sendDMAQ;
#endif

/* ---------- Types of message queues internal to DMA driver ---------- */
enum queueType {
	UNKNOWN_QUEUE_TYPE = 0,
	SG_NODE_TYPE,
	TX_DESC_TYPE
};
typedef enum queueType Q_t;

/* ---------------------------- Prototypes ---------------------------- */
void		dmaLogMsg(char *);
void		dmaLogErrMsgs(int, int, int, int, int, int, int, int, int, int);

/* ---------------------- Create message queues ----------------------- */
STATUS		dmaCreateSGMsgQueue(int size);
STATUS		dmaCreateTxDescMsgQueue(int size);
STATUS		dmaCreateErrMsgQueue(void);
STATUS		dmaCreateRunMsgQueue(void);
STATUS		dmaCreateUserMsgQueue(void);
STATUS		dmaCreateEndMsgQueue(void);

/* ---------------------- Delete message queues ----------------------- */
STATUS		dmaDeleteSGMsgQueue(void);
STATUS		dmaDeleteTxDescMsgQueue(void);
void		dmaDeleteRunMsgQueue(void);
void		dmaDeleteUserMsgQueue(void);
void		dmaDeleteEndMsgQueue(void);
void		dmaDeleteErrMsgQueue(void);

/* -------------------- Push message onto a queue --------------------- */
STATUS		dmaPutFreeSGNode(sgnode_t *);
STATUS		dmaPutFreeTxDesc(txDesc_t *);

/* -------------------- Pop message off of a queue -------------------- */
sgnode_t *  dmaGetFreeSGNode(void);
txDesc_t *  dmaGetFreeTxDesc(void);

/* ------------------ Get no. of messages on a queue ------------------ */
int			dmaNumFreeSGListNodes(void);
int			dmaNumFreeTXDescNodes(void);

#endif	/* INCdmaMsgQueue_h */
