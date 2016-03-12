/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*
Id: dmaMsgQueue.c,v 1.6 2004/04/12 21:04:02 rthrift Exp rthrift
=========================================================================
FILE: dmaMsgQueue.c
=========================================================================
Purpose:
	Provide a collection of message queue handling functions for use by
	the DMA drivers.

Externally callable routines:
	dmaCreateSGMsgQueue		- Create & populate SG list node msg queue.
	dmaDeleteSGMsgQueue		- Delete SG list node msg queue and its msgs.
	dmaPutFreeSGNode		- Push a free SG list node onto the queue.
	dmaGetFreeSGNode		- Pop a free SG list node off the queue.
	dmaNumFreeSGListNodes	- Return no. free SG list nodes on the queue.

	dmaCreateTxDescMsgQueue	- Create & populate txfr descriptors queue.
	dmaDeleteTxDescMsgQueue	- Delete the txfr descriptor message queue.
	dmaPutFreeTxDesc		- Push a free txfr descriptor onto its queue.
	dmaGetFreeTxDesc		- Pop a free txfr descriptor off its queue.
	dmaNumFreeTxDescs		_ Return no. of free txfr descriptors available.

	dmaCreateErrMsgQueue	- Create message queue for DMA error messages,
	dmaDeleteErrMsgQueue	- Delete DMA error messages queue

	dmaCreateRunMsgQueue	- Create a message queue for runDMA task.
	dmaDeleteRunMsgQueue	- Delete runDMA tasks's message queue.

	dmaCreateUserMsgQueue	- Create message queue for user calls (sendDMA)
	dmaDeleteUserMsgQueue	- Delete message queue for user calls

	dmaCreateEndMsgQueue	- Create message queue for endDMA task.
	dmaDeleteEndMsgQueue	- Delete message queue for endDMA task.

Internal support routines:
	Name                  Supports:
    --------              -----------
	putQMsg				- dmaPutFreeSGNode, dmaPutFreeTxDesc
	getQMsg				- dmaGetFreeSGNode,  dmaGetFreeTxDesc
	dmaLogErrMsgs		- DMA Error Message Queue

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
#include <stdio.h>
#include <msgQLib.h>
#include <taskLib.h>
#include <string.h>
#include "dmaDrv.h"
#include "dmaMsgQueue.h"
#include "dmaDebugOptions.h"
#define  DMA_MSG_QUEUE_C

/* ------------ Handle for the SG list nodes message queue ------------ */
static MSG_Q_ID sgQueue = NULL;

/* ------------ Handle for txfr descriptors message queue ------------- */
static MSG_Q_ID txDescs = NULL;

/* ------------- Handle for the runDMA task message queue ------------- */
MSG_Q_ID runDMAQ = NULL;

/* ----------- Handle for DMA error message queue (Global) ------------ */
MSG_Q_ID dmaErrQ = NULL;

/* ------------ Handle for user DMA message queue (Global) ------------ */
MSG_Q_ID sendDMAQ = NULL;

/* -------- Handle for the endDMA task message queue (Global) --------- */
MSG_Q_ID endDMAQ = NULL;

/* ------------- Prototypes for internal support routines ------------- */
int numFreeSGListNodes(void);
int numFreeTxDescNodes(void);
static STATUS putQMsg(Q_t, void *);
static void *getQMsg(Q_t);

/* ------------------ Globals for error msg handling ------------------ */
char estr[MAX_ERR_MSG_LEN];		/* Err msg string						*/
int  lstr;						/* Length of Err msg string				*/

/*
=========================================================================
NAME: dmaPutFreeSGNode, dmaPutFreeTxDesc
=========================================================================
PURPOSE:
	Push a free list element onto its respective message queue.
	dmaPutFreeSGNode: push free element onto SG nodes message queue.
	dmaPutFreeTxDesc: push free element onto txfr descriptors message queue.

ARGUMENTS:
	nodeAddr	- Address of a free list element.

RETURN VALUE:
	OK			- Successful operation.
	ERROR		- An error occurred.

COMMENT:
	A message consists only of the address of a list element.

AUTHOR: 
	Robert L. Thrift, 2004
=========================================================================
*/
STATUS dmaPutFreeSGNode(sgnode_t *nodeAddr)
{
	STATUS s;
	void *node;

	node = (void *) nodeAddr;
	s = putQMsg(SG_NODE_TYPE, nodeAddr);
	if (s == ERROR)
		perror("dmaDrv > dmaPutFreeSGNode");
	return s;
}

STATUS dmaPutFreeTxDesc(txDesc_t *nodeAddr)
{
	STATUS s;
	void *node;

	node = (void *) nodeAddr;
	s = putQMsg(TX_DESC_TYPE, nodeAddr);
	if (s == ERROR)
		perror("dmaDrv > dmaPutFreeTxDesc");
	return s;
}

/*
=========================================================================
NAME: dmaGetFreeSGNode, dmaGetFreeTxDesc
=========================================================================
PURPOSE:
	Pop a free structure address off its message queue.
	dmaGetFreeSGNode: take free SG node struct from SG Nodes msg queue.
	dmaGetFreeTxDesc: take free txfr descriptor from txDesc msg queue.

ARGUMENTS:
	None.

RETURN VALUE:
	Address of a free list element.
	Returns NULL in case of error.

COMMENTS:
	A message consists only of the address of a scatter-gather list node,
	as defined in dmaDrv.h

	Task will suspend if there are no messages in the message queue.
	Presumably a DMA scatter-gather transfer now in progress will
	complete and push freed-up list elements back onto the message queue,
	which will allow our task to resume.

	These functions MUST NOT be called from an interrupt service routine,
	as they can suspend the ISR!

AUTHOR: 
	Robert L. Thrift, 2004
=========================================================================
*/
sgnode_t *dmaGetFreeSGNode(void)
{
	void *node;

	node = getQMsg(SG_NODE_TYPE);
	if (node == NULL)
		perror("dmaDrv > dmaGetFreeSGNode");
	memset(node, 0, sizeof(sgnode_t));
	return (sgnode_t *)node;
}

txDesc_t *dmaGetFreeTxDesc(void)
{
	void *node;

	node = getQMsg(TX_DESC_TYPE);
	if (node == NULL)
		perror("dmaDrv > dmaGetFreeTxDesc");
	memset(node, 0, sizeof(txDesc_t));
	return (txDesc_t *)node;
}

/* =========================================================================
   - Return no. free SG list nodes on the queue.
*/
int dmaNumFreeSGListNodes(void) 	
{
     return( msgQNumMsgs(sgQueue) );
}

/* =========================================================================
   - Return no. of free txfr descriptors available.
*/
int dmaNumFreeTxDescs(void)
{
     return( msgQNumMsgs(txDescs) );
}

/*
=========================================================================
NAME: dmaCreateSGMsgQueue
=========================================================================
PURPOSE:
	Create and initialize the message queue for holding the addresses of
	free scatter-gather list nodes.

ARGUMENTS:
	None.

RETURN VALUE:
	OK			- Successful operation.
	ERROR		- An error occurred. Any error at this point is fatal.

COMMENTS:
	A message in the queue consists only of the address of a scatter-
	gather list node, as defined in dmaDrv.h

	All scatter-gather list nodes are created at this time, to avoid
	memory fragmentation that could result from repeated malloc and
	free of SG list nodes while the program is running. Obtain the
	address of a free list node by calling dmaGetFreeSGNode(). Restore a used
	node to the free list by calling dmaPutFreeSGNode().

	If static variable sgQueue is non-NULL, and the queue is not empty,
	assume that the message queue has already been initialized and
	delete it before proceeding.

AUTHOR: 
	Robert L. Thrift, 2004
=========================================================================
*/
STATUS dmaCreateSGMsgQueue(int numSgNodes)
{
	int i;
	STATUS status;

	/* ---------------- Check: are we re-initializing? ---------------- */
	if (sgQueue)
		if (dmaDeleteSGMsgQueue() != OK)
			perror("dmaDrv > dmaCreateSGMsgQueue");

	/* ---------------- Create the basic message queue ---------------- */
	sgQueue = msgQCreate(numSgNodes, (int)sizeof(sgnode_t *),
							MSG_Q_FIFO);
	if (sgQueue == NULL)
		return ERROR;

	/* --------- Populate the SG msg queue with SG list nodes --------- */
	for (i = 0; i < numSgNodes; i++)
	{
		sgnode_t *new;
		/* new = (sgnode_t *)(malloc(sizeof(sgnode_t))); */
		new = (sgnode_t *)(memalign(_CACHE_ALIGN_SIZE,sizeof(sgnode_t)));
		if (new != NULL)
		{
			status = dmaPutFreeSGNode(new);
			if (status != OK)
			{
				/* -------- Err returned from dmaPutFreeSGNode -------- */
				perror("dmaDrv > dmaCreateSGMsgQueue > dmaPutFreeSGNode");
				return ERROR;
			}
		}
		else
		{
			/* -------------- Error returned from malloc -------------- */
			perror("dmaDrv > dmaCreateSGMsgQueue");
			return ERROR;
		}
	}
	return OK;
}

/*
=========================================================================
NAME: dmaCreateTxDescMsgQueue
=========================================================================
PURPOSE:
	Create and initialize the message queue for holding the addresses of
	free transfer descriptors.

ARGUMENTS:
	None.

RETURN VALUE:
	OK			- Successful operation.
	ERROR		- An error occurred. Any error at this point is fatal.

COMMENTS:
	A message in the queue consists only of the address of a transfer
	descriptor, as defined in dmaDrv.h

	All transfer descriptors are created at this time, to avoid
	memory fragmentation that could result from repeated malloc and
	free of descriptors while the program is running. Obtain the
	address of a free descriptor by calling dmaGetFreeTxDesc(). Restore a used
	node to the free list by calling dmaPutFreeTxDesc().

	If static variable txDescs is non-NULL, and the queue is not empty,
	assume that the message queue has already been initialized and
	delete it before proceeding.

AUTHOR: 
	Robert L. Thrift, 2004
=========================================================================
*/
STATUS dmaCreateTxDescMsgQueue(int numTxDesc)
{
	int i;
	STATUS status;

	/* ---------------- Check: are we re-initializing? ---------------- */
	if (txDescs)
		if (dmaDeleteTxDescMsgQueue() != OK)
			perror("dmaDrv > dmaCreateTxDescMsgQueue");

	/* ---------------- Create the basic message queue ---------------- */
	txDescs = msgQCreate(numTxDesc, (int)sizeof(txDesc_t *),
							MSG_Q_FIFO);
	if (txDescs == NULL)
		return ERROR;

	/* - Populate txfr descriptors queue with pre-allocated elements -- */
	for (i = 0; i < numTxDesc; i++)
	{
		txDesc_t *new;
		/* new = (txDesc_t *)(malloc(sizeof(txDesc_t))); */
		new = (txDesc_t *)(memalign(_CACHE_ALIGN_SIZE,sizeof(txDesc_t)));
		if (new != NULL)
		{
			status = dmaPutFreeTxDesc(new);
			if (status != OK)
			{
				/* -------- Err returned from dmaPutFreeTxDesc -------- */
				perror("dmaDrv > dmaCreateTxDescMsgQueue");
				return ERROR;
			}
		}
		else
		{
			/* -------------- Error returned from malloc -------------- */
			perror("dmaDrv > dmaCreateTxDescMsgQueue");
			return ERROR;
		}
	}
	return OK;
}

/*
=========================================================================
NAME: dmaDeleteSGMsgQueue
=========================================================================
PURPOSE:
	Delete the SG list nodes message queue, and all free SG list nodes
	whose addresses currently exist in the queue.

ARGUMENTS:
	None.

RETURN VALUE:
	OK			- Successful operation.
	ERROR		- An error occurred.

COMMENT:
	Any SG list nodes that are currently a part of an active
	scatter-gather list will not be in the queue of free node addresses,
	thus will not be deleted.
	It is recommended to call dmaSGQueueFree() before calling this
	routine, as this will restore the absent list nodes back into the
	message queue, from which they will be deleted as expected.

AUTHOR: 
	Robert L. Thrift, 2004
=========================================================================
*/
STATUS dmaDeleteSGMsgQueue(void)
{
	STATUS rtnval;
	int num_nodes;
	sgnode_t *node;

	if (! sgQueue)
		/* ------- There is no message queue, so no work to do. ------- */
		return OK;

	/* ---- Retrieve each node address from the queue and free it ----- */
	num_nodes = numFreeSGListNodes();
	if (num_nodes == ERROR)
	{
		perror("dmaDrv > dmaDeleteSGMsgQueue > numFreeSGListNodes");
		return ERROR;
	}

	while (num_nodes--)
	{
		node = (sgnode_t *)dmaGetFreeSGNode();
		if (node)
			free ((void *) node);
	}

	/* ------------- Now delete the message queue itself. ------------- */
	rtnval = msgQDelete(sgQueue);
	if (rtnval == ERROR)
		perror("dmaDrv > dmaDeleteSGMsgQueue > msgQDelete");

	sgQueue = NULL;
	return rtnval;
}

/*
=========================================================================
NAME: dmaDeleteTxDescMsgQueue
=========================================================================
PURPOSE:
	Delete the DMA transfer descriptors message queue, and all descriptor
	elements whose addresses currently exist in the queue.

ARGUMENTS:
	None.

RETURN VALUE:
	OK			- Successful operation.
	ERROR		- An error occurred.

COMMENT:
	Any DMA transfer descriptor structs that are currently a part of an
	active DMA transfer queue will not be in the queue of free descriptor
	addresses, thus will not be deleted.
	It is recommended to call dmaSGQueueFree() before calling this
	routine, as this will restore the absent addresses back into the
	message queue, from which they will be deleted as expected.

AUTHOR: 
	Robert L. Thrift, 2004
=========================================================================
*/
STATUS dmaDeleteTxDescMsgQueue(void)
{
	STATUS rtnval;
	int num_descs;
	txDesc_t *node;

	if (! txDescs)
		/* ------- There is no message queue, so no work to do. ------- */
		return OK;

	/*  Get each transfer descriptor address from the queue & free it	*/
	num_descs = numFreeTxDescNodes();
	if (num_descs == ERROR)
	{
		perror("dmaDrv > dmaDeleteTxDescMsgQueue > numFreeTxDescNodes");
		return ERROR;
	}

	while (num_descs--)
	{
		node = (txDesc_t *)dmaGetFreeTxDesc();
		if (node)
			free ((void *) node);
	}

	/* ------------- Now delete the message queue itself. ------------- */
	rtnval = msgQDelete(txDescs);
	if (rtnval == ERROR)
		perror("dmaDrv > dmaDeleteTxDescMsgQueue > msgQDelete");

	txDescs = NULL;
	return rtnval;
}

/*
=========================================================================
NAME: numFreeSGListNodes
=========================================================================
PURPOSE:
	Return the number of free SG list nodes on the message queue.

ARGUMENTS:
	None.

RETURN VALUE:
	Nonzero		- Number of message addresses in the queue.
	Zero		- An error occurred, or nothing in the queue.

COMMENT:

AUTHOR: 
	Robert L. Thrift, 2004
=========================================================================
*/
int numFreeSGListNodes(void)
{
	int num;

	num = msgQNumMsgs(sgQueue);
	if (num == ERROR)
	{
		perror("dmaDrv > numFreeSGListNodes > msgQNumMsgs");
		return 0;
	}
	return num;
}

/*
=========================================================================
NAME: numFreeTxDescNodes
=========================================================================
PURPOSE:
	Return the number of free txfr descriptors on the message queue.

ARGUMENTS:
	None.

RETURN VALUE:
	Nonzero		- Number of message addresses in the queue.
	Zero		- An error occurred, or nothing in the queue.

COMMENT:

AUTHOR: 
	Robert L. Thrift, 2004
=========================================================================
*/
int numFreeTxDescNodes(void)
{
	int num;

	num = msgQNumMsgs(txDescs);
	if (num == ERROR)
	{
		perror("dmaDrv > numFreeTxDescNodes > msgQNumMsgs");
		return 0;
	}
	return num;
}

/*
=========================================================================
NAME: putQMsg
=========================================================================
PURPOSE:
	Push a free list element onto one of the DMA driver message queues.

ARGUMENTS:
	queueType	- One of: SG_NODE_TYPE, TX_DESC_TYPE.
	nodeAddr	- Adddress of the element to be pushed onto its queue.

RETURN VALUE:
	OK			- Operation succeeded.
	ERROR		- Operation failed.

COMMENTS:
	Push the address of a free list element onto: (a) queue of
	scatter-gather list nodes; or (b) queue of transfer descriptors.

	A message consists only of the address of a pre-allocated struct,
	as defined in dmaDrv.h

	The NO_WAIT timeout argument to msgQSend() is required if this
	function is to be callable from an interrupt service routine.

AUTHOR: 
	Robert L. Thrift, 2004
=========================================================================
*/
STATUS putQMsg(Q_t queueType, void *nodeAddr)
{
	STATUS s;
	UINT32 node;

	node = (UINT32) nodeAddr;
	switch(queueType) {
		case SG_NODE_TYPE:
			/* --------- Push element onto SG list node queue --------- */
			s = msgQSend(sgQueue, (char *)&node, (UINT)(sizeof(UINT32)),
						NO_WAIT, MSG_PRI_NORMAL);
			break;
		case TX_DESC_TYPE:
			/* -------- Push element onto TX descriptors queue -------- */
			s = msgQSend(txDescs, (char *)&node, (UINT)(sizeof(UINT32)),
				NO_WAIT, MSG_PRI_NORMAL);
			break;
		default:
			return ERROR;
			break;
	}

	if (s == ERROR)
		perror("dmaDrv > putQmsg > msgQSend");

	return s;
}

/*
=========================================================================
NAME: getQMsg
=========================================================================
PURPOSE:
	Pop a free list element off of one of the DMA driver message queues.

ARGUMENTS:
	queueType	- One of: SG_NODE_TYPE, TX_DESC_TYPE.

RETURN VALUE:
	Address of a free list element, or NULL if there was an error.

COMMENT:
	Pop the address of a free list element off of: (a) queue of
	scatter-gather list nodes; or (b) queue of DMA transfer descriptors.

	A message consists only of the address of a pre-allocated struct,
	as defined in dmaDrv.h

	This function MUST NOT be called from an interrupt service routine,
	as it can suspend the ISR!

AUTHOR: 
	Robert L. Thrift, 2004
=========================================================================
*/
void *getQMsg(Q_t queueType)
{
	void *node;
	int result;

	switch(queueType) {
		case SG_NODE_TYPE:
			result = msgQReceive(sgQueue, (char *)&node,
					(UINT)(sizeof(sgnode_t *)), WAIT_FOREVER);
			break;
		case TX_DESC_TYPE:
			result = msgQReceive(txDescs, (char *)&node,
					(UINT)(sizeof(txDesc_t *)), WAIT_FOREVER);
			break;
		default:
			return NULL;
			break;
	}

	if (result == ERROR)
	{
		perror("dmaDrv > getQMsg > msgQReceive");
		return NULL;
	}

	/* ------- Return addr. of pre-allocated free list element ------- */
	return node;
}

/*
=========================================================================
NAME: dmaCreateErrMsgQueue
=========================================================================
PURPOSE:
	Create and initialize a message queue for DMA error messages.

COMMENT:

AUTHOR: 
	Robert L. Thrift, 2004
=========================================================================
*/
STATUS dmaCreateErrMsgQueue(void)
{
	if (dmaErrQ)
		msgQDelete(dmaErrQ);

	dmaErrQ = msgQCreate(MAX_ERR_MSGS, MAX_ERR_MSG_LEN, MSG_Q_FIFO);

	if (dmaErrQ == NULL)
	{
		printf("dmaCreateErrMsgQueue: cannot create error msg queue.\n");
		return ERROR;
	}
	return OK;
}

/*
=========================================================================
NAME: dmaDeleteErrMsgQueue
=========================================================================
PURPOSE:
	Delete message queue created by dmaCreateErrMsgQueue().

COMMENT:
	Supports dmaCleanup() routine.

AUTHOR: 
	Robert L. Thrift, 2004
=========================================================================
*/
void dmaDeleteErrMsgQueue(void)
{
	msgQDelete(dmaErrQ);
	dmaErrQ = NULL;
}

/*
=========================================================================
NAME: dmaCreateEndMsgQueue
=========================================================================
PURPOSE:
	Create and initialize a message queue for DMA error messages.

COMMENT:

AUTHOR: 
	Robert L. Thrift, 2004
=========================================================================
*/
STATUS dmaCreateEndMsgQueue(void)
{
	if (endDMAQ)
		msgQDelete(endDMAQ);

	endDMAQ = msgQCreate(MAX_END_MSGS, sizeof(UINT32), MSG_Q_FIFO);

	if (endDMAQ == NULL)
	{
		printf("dmaCreateEndMsgQueue: cannot create error msg queue.\n");
		return ERROR;
	}
	return OK;
}

/*
=========================================================================
NAME: dmaDeleteEndMsgQueue
=========================================================================
PURPOSE:
	Delete message queue created by dmaCreateEndMsgQueue().

COMMENT:
	Supports dmaCleanup() routine.

AUTHOR: 
	Robert L. Thrift, 2004
=========================================================================
*/
void dmaDeleteEndMsgQueue(void)
{
	msgQDelete(endDMAQ);
	endDMAQ = NULL;
}

/*
=========================================================================
TASK: dmaLogErrMsgs
=========================================================================
PURPOSE:
	Watch the dma err message queue and print or log an error message
	when one arrives in the queue.

COMMENTS:
	Task only wakes up when there is a message to be printed.

	None of the arguments (defined for taskSpawn) are used.

	Don't append newlines to the messages sent to the task, we will
	add a newline to each message here.

AUTHOR: 
	Robert L. Thrift, 2004
=========================================================================
*/
void dmaLogErrMsgs(int dummy0, int dummy1, int dummy2, int dummy3,
				   int dummy4, int dummy5, int dummy6, int dummy7,
				   int dummy8, int dummy9)
{
	char msgBuf[80];

	FOREVER
	{
		/* ------------- Normally we will pend right here ------------- */
		msgQReceive(dmaErrQ, msgBuf, 80, WAIT_FOREVER);

		/* ------ Got a message, print it and go back to waiting ------ */
		msgBuf[79] = '\000';	/* In case the queue truncated it		*/

		/* --- Substitute another logging mechanism here if desired --- */
		printf("%s\n", msgBuf);
	}
}

/*
=========================================================================
NAME: dmaLogMsg
=========================================================================
PURPOSE:
	Provide a simplified interface for logging DMA messages onto
	the DMA error message queue.

COMMENT:
	Messages are pushed onto the message queue using standardized
	message queue options. They are not necessarily error messages;
	they could be debugging messages, etc. as well.

AUTHOR: 
	Robert L. Thrift, 2004
=========================================================================
*/
void dmaLogMsg(char *msgBuf)
{
	int msgLength;
	STATUS status;

        /* use the standard messsage loggin routine */
        diagPrint(NULL,msgBuf);
#ifdef XXX_GMB
	msgLength = strlen(msgBuf) + 1;
	status = msgQSend(dmaErrQ, msgBuf, msgLength, NO_WAIT, MSG_PRI_NORMAL);
	if (status == ERROR)
	{
		printf("dmaLogMsg: cannot log: %s (%d chars)\n", msgBuf, msgLength);
		printf("           %d messages in queue.\n", msgQNumMsgs(dmaErrQ));
	}
#if 0
        taskDelay(calcSysClkTicks(17));  /* Give up the CPU, let msg task run on time	*/
#endif
#endif
}

/*
=========================================================================
NAME: dmaCreateRunMsgQueue
=========================================================================
PURPOSE:
	Create and initialize a message queue for the runDMA task.

COMMENT:

AUTHOR: 
	Robert L. Thrift, 2004
=========================================================================
*/
STATUS dmaCreateRunMsgQueue(void)
{
	if (runDMAQ)
		msgQDelete(runDMAQ);

	runDMAQ = msgQCreate(RUN_MSG_Q_MAX, RUN_MSG_LEN, MSG_Q_FIFO);

	if (runDMAQ == NULL)
	{
		printf("dmaCreateRunMsgQueue: cannot create runDMA msg queue.\n");
		return ERROR;
	}

	return OK;
}

/*
=========================================================================
NAME: dmaDeleteRunMsgQueue
=========================================================================
PURPOSE:
	Delete message queue created by dmaCreateRunMsgQueue().

COMMENT:
	Supports dmaCleanup() routine.

AUTHOR: 
	Robert L. Thrift, 2004
=========================================================================
*/
void dmaDeleteRunMsgQueue(void)
{
	msgQDelete(runDMAQ);
	runDMAQ = NULL;
}


/*
=========================================================================
NAME: dmaCreateUserMsgQueue
=========================================================================
PURPOSE:
	Create and initialize a message queue for user's DMA jobs.

COMMENT:
	Used for user messages to sendDMA routine.

AUTHOR: 
	Robert L. Thrift, 2004
=========================================================================
*/
STATUS dmaCreateUserMsgQueue(void)
{
	if (sendDMAQ)
		msgQDelete(sendDMAQ);

	sendDMAQ = msgQCreate(MAX_USER_MSGS, MAX_USER_MSG_LEN, MSG_Q_FIFO);

	if (sendDMAQ == NULL)
	{
		printf("dmaCreateUserMsgQueue: cannot create sendDMA msg queue.\n");
		return ERROR;
	}

	return OK;
}

/*
=========================================================================
NAME: dmaDeleteUserMsgQueue
=========================================================================
PURPOSE:
	Delete message queue created by dmaCreateUserMsgQueue().

COMMENT:
	Supports dmaCleanup() routine.

AUTHOR: 
	Robert L. Thrift, 2004
=========================================================================
*/
void dmaDeleteUserMsgQueue(void)
{
	msgQDelete(sendDMAQ);
	sendDMAQ = NULL;
}

