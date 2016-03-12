/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* Id: dmaDrv.h,v 1.6 2004/04/30 15:05:12 rthrift Exp rthrift  */
/*
=========================================================================
FILE: dmaDrv.h
=========================================================================
PURPOSE:
	Provide definitions for DMA driver routines (dmaDrv.c).

COMMENTS:
	Specific to PowerPC 405GPr CPU.
	Standard ANSI C/C++ compilation is assumed.

AUTHOR:
	Robert L. Thrift
=========================================================================
     Copyright (c) 2004, Varian Associates, Inc. All Rights Reserved.
     This software contains proprietary and confidential information
            of Varian Associates, Inc. and its contributors.
  Use, disclosure and reproduction is prohibited without prior consent.
=========================================================================
*/

#ifndef __INCdmadrvh
#define __INCdmadrvh
#include <semLib.h>
#include <msgQLib.h>
#include "rngLLib.h"
#include "dmaReg.h"

#ifdef __cplusplus
extern "C" {
#endif		/* __cplusplus */

/* 
 * -----------------------------------------------------------------------
 * The number of DMA channels available on PPC 405GPr.
 * -----------------------------------------------------------------------
 */
#define MAX_DMA_CHANNELS 4

/* 
 * -----------------------------------------------------------------------
 * Base DMA interrupt number.
 * Channel 0: 5		Channel 1: 6
 * Channel 2: 7		Channel 3: 8
 * We define interrupt no. 5 as the base number for DMA channel 0, and
 * other interrupt nos. go up from there. The interrupt nos. for the DMA
 * channels are defined by the hardware and cannot be changed.
 * -----------------------------------------------------------------------
 */
#define DMA_CHAN_INT_BASE 5

/* 
 * -----------------------------------------------------------------------
 * enum dmaTransferStatus (dmaStatus_t)
 * -----------------------------------------------------------------------
 * Possible states for a single transfer descriptor on the queue.
 * Used by various DMA driver tasks to update the transfer's current
 * status as it proceeds.
 * -----------------------------------------------------------------------
 */
enum dmaTransferStatus {
	UNKNOWN_TXFR_STATUS = 0,	/* Unknown or uninitialized				*/
	NOT_READY,					/* Still assembling transfer data		*/
	READY,						/* Ready to fire						*/
	RUNNING,					/* Fired off and running				*/
	DONE,						/* Transfer completed					*/
	ERR_STATUS					/* Error status							*/
};
typedef enum dmaTransferStatus dmaStatus_t;

/* 
 * -----------------------------------------------------------------------
 * enum dmaTransferType (txfr_t)
 * -----------------------------------------------------------------------
 * Types of DMA transfers we can do. Used by sendDMA() to decide how to
 * set up the transfer.
 *
 * MEMORY_TO_MEMORY		Mem-to-mem transfer, both addresses incrementing.
 *						Used mainly for RAM-to-RAM buffer transfers.
 * MEMORY_TO_MEMORY_SRC_PACED	Mem-to-mem transfer, both addresses incrementing.
 *						with device paced, Used mainly for DDR DSP HPI-to-RAM buffer transfers.
 * MEMORY_TO_MEMORY_DST_PACED	Mem-to-mem transfer, both addresses incrementing.
 *						with device paced.
 * MEMORY_TO_PERIPHERAL	Write (mem-to-mem device-paced, source address
 *						incrementing, destination address fixed).
 *						Used for writing to an output FIFO.
 * PERIPHERAL_TO_MEMORY	Read (mem-to-mem device-paced, destination addr.
 *						incrementing, source address fixed).
 *						Used for reading from an input FIFO.
 * MEMORY_TO_FPGA		Mem-to-mem to fixed addr., non-device-paced,
 *						source addr. incrementing, dest. addr. fixed.
 *						Used for writing to external memory or external
 *						memory-like device through an FPGA register.
 * FPGA_TO_MEMORY		Mem-to-mem from fixed addr., non-device-paced,
 *						source addr. fixed, dest. addr. incrementing.
 *						Used for reading from external memory or external
 *						memory-like device through an FPGA register.
 * -----------------------------------------------------------------------
 */
enum dmaTransferType {
	UNINITIALIZED_TRANSFER_TYPE = 0,	/* Unknown						*/
	MEMORY_TO_MEMORY,
	MEMORY_TO_PERIPHERAL,
	PERIPHERAL_TO_MEMORY,
	MEMORY_TO_FPGA,
	FPGA_TO_MEMORY,
	MEMORY_TO_MEMORY_SRC_PACED,
	MEMORY_TO_MEMORY_DST_PACED
};
typedef enum dmaTransferType txfr_t;

/* 
 * -----------------------------------------------------------------------
 * enum dmaDevicePaced (dp_t)
 * -----------------------------------------------------------------------
 * Specifier for device-paced transfers.
 * DEVICE_PACED_NOT		Non-device-paced.
 * DEVICE_PACED			Device-paced.
 * -----------------------------------------------------------------------
 */
enum dmaDevicePaced {
	UNINITIALIZED_PACING = 0,	/* Unknown								*/
	DEVICE_PACED_NOT,
	DEVICE_PACED
};
typedef enum dmaDevicePaced dp_t;

/* 
 * -----------------------------------------------------------------------
 * enum dmaUseSGList (usesg_t)
 * -----------------------------------------------------------------------
 * Specifier for use of scatter-gather list, or not. Used by sendDMA()
 * to decide whether to service a scatter-gather list, or a single-
 * buffer transfer.
 * NO_SG_LIST			Single buffer transfer, no scatter-gather list.
 * SG_LIST				Using scatter-gather list.
 * -----------------------------------------------------------------------
 */
enum dmaUseSGList {
	UNINITIALIZED_SG_LIST = 0,	/* Unknown								*/
	NO_SG_LIST,
	SG_LIST
};
typedef enum dmaUseSGList usesg_t;

/* 
 * -----------------------------------------------------------------------
 * struct dmaSGListNode (sgnode_t)
 * -----------------------------------------------------------------------
 * A DMA transfer descriptor node in a scatter/gather list.
 * This is a forward-linked list only, no reverse links. The format of
 * this struct is dictated by the DMA hardware implementation, and
 * must not be changed!
 * A scatter-gather list is a linked chain of these nodes.
 * -----------------------------------------------------------------------
 */
struct dmaSGListNode {
	UINT32	dmaCCW;				/* DMA Channel Control Word				*/
	UINT32	dmaSrcAddr;			/* DMA source address					*/
	UINT32	dmaDstAddr;			/* DMA destination address				*/
	UINT32	dmaCtrlBits;		/* LK, TCI, ETI, ERI bits and Count		*/
	struct dmaSGListNode *next;	/* Ptr. to next entry in list			*/
};
typedef struct dmaSGListNode sgnode_t;

/* 
 * -----------------------------------------------------------------------
 * struct dmaTransferDesc (txDesc_t)
 * -----------------------------------------------------------------------
 * A DMA transfer descriptor in a queue of DMA transfer descriptors.
 * This struct is a member of a linked list  of descriptors which is
 * headed by a struct dmaSGListQueueHead (one per channel) which points
 * to the first and last of these DMA transfer descriptors. Each
 * transfer descriptor describes a single DMA transfer operation in
 * the queue.  A single DMA transfer operation may be comprised of a
 * single data buffer to be transmitted somewhere via DMA, or it may be
 * a scatter-gather list of a number of buffers chained together.
 *
 * This struct does double duty as (1) a pointer to a single buffer
 * awaiting transfer, and (2) the head of a scatter-gather list. The
 * 'txfrType' element determines which it is.
 *
 * The element 'txfrStatus' is designated as NOT_READY when the descrip-
 * tor is first attached to a queue, and is changed to READY when it is
 * all set up and ready to run. A list of pointers to free transfer
 * descriptors is maintained in a message queue. To use a descriptor,
 * remove it from the message queue and attach it to the DMA transfer
 * queue. After the transfer remove it from the transfer queue and push
 * it back onto the free descriptors queue.
 *
 * The element 'internalSGFlag' is set if the sender originally wanted
 * to send a single buffer, but it turned out to be too large for the
 * DMA hardware to do a single transfer, and the driver routines created
 * a scatter-gather list in order to handle it. It means that the
 * scatter-gather list must be cleaned up by the driver after the data
 * transfer is complete, because the user does not know that it exists.
 * -----------------------------------------------------------------------
 */
struct dmaTxfrDesc {
	int	channel;		/* Which channel to use			*/
	dmaStatus_t txfrStatus;		/* NOT_READY, READY, RUNNING, or DONE	*/
	usesg_t srcType;		/* Source type, SG_LIST or NO_SG_LIST	*/
	txfr_t	 transferType;		/* MEMORY_TO_MEMORY, etc.		*/
	int txfrControl;		/* Channel control bits to be used	*/

	/* --------- Linked list pointers for the transfer queue ---------- */
	struct dmaTxfrDesc	*prev;	/* Previous transfer desc. in queue	*/
	struct dmaTxfrDesc	*next;	/* Next transfer desc. in queue		*/

	/* --- Following elements used only for single-buffer transfers --- */
	UINT32 *srcAddr;			/* Source address		*/
	UINT32 *dstAddr;			/* Destination address		*/
	int txfrSize;				/* No. 32 bit words in DMA transfer */

	/* ----- Following elements used only if txfrType is SG_LIST ------ */
	sgnode_t *first;			/* First SG node in chained SG list.	*/
	sgnode_t *last;				/* Last SG node in chained SG list.	*/
	int count;				/* No. of nodes in SG list.		*/
	int internalSGFlag;			/* 1 if SG list created internally,	*/
						/* 0 if SG list created by caller.	*/
	MSG_Q_ID	srcMsgQ;		/* Message queue to which src. buffers	*/
						/* should be returned. (May be NULL.)	*/
	MSG_Q_ID	dstMsgQ;		/* Message queue to which dst. buffers	*/
						/* should be returned. (May be NULL.)	*/
};
typedef struct dmaTxfrDesc txDesc_t;

/* 
 * -----------------------------------------------------------------------
 * struct dmaSGListQueueHead (qhead_t)
 * -----------------------------------------------------------------------
 * The head of a transfer descriptor queue for one DMA channel.
 * There is only one of these per channel, created at initialization
 * time, and they persist for the life of the program. If the queue is
 * empty, first and last pointers are NULL and count is zero.
 * The queue is a linked list of transfer descriptor ptrs. (txDesc_t *)
 * Only one transfer in the list will be doing DMA at any one time, but
 * the user may be building another transfer in some other descriptor on
 * the queue.
 * -----------------------------------------------------------------------
 */
struct dmaSGListQueueHead {
	txDesc_t	*first;			/* First transfer descriptor, or NULL	*/
	txDesc_t	*last;			/* Last  transfer descriptor, or NULL	*/
	int			count;			/* No. of transfers in queue			*/
};
typedef struct dmaSGListQueueHead qhead_t;

/* 
 * -----------------------------------------------------------------------
 * struct dma_info (dmainfo_t)
 * -----------------------------------------------------------------------
 * Descriptor for current info about a DMA channel.
 * There is only one of these per channel, created at initialization
 * time, and they persist for the life of the program.
 * -----------------------------------------------------------------------
 */
struct dma_info {
	int	 channel;			/* My channel: 0,1,2,3			*/
	SEM_ID	 lock;				/* Taken if chan. currently assigned.	*/
	qhead_t  *queue;			/* Ptr. to head of channel's queue list	*/
	txDesc_t *current;			/* Ptr. to current txfer, if any	*/
	/* -------- We can add any other desired useful info here --------- */
	/* ------ added these to replace the link list queue, should be simplier, GMB 6/3/04 */
	RINGL_ID readyQueue;   /* ready so we can queue dma request and have endDMATask start them, gmb 6/4/04 */
	/* RINGL_ID doneQueue; future enhancement, gmb 6/4/04 */
        SEM_ID  pDmaMutex;			/* used to mutually excluded concurrent access */
};
typedef struct dma_info dmainfo_t;

/* 
 * -----------------------------------------------------------------------
 * struct dmaRequest
 * -----------------------------------------------------------------------
 * This is just a list of parameters which is identical to the argument
 * list for the sendDMA() function, placed into a struct so that the
 * whole block of parameters can be conveniently placed into a message
 * queue as a single message.
 * The message consists of the entire struct and will be picked up by
 * sendDMATask() which will set up and run the transfer.
 *
 * If useSG == SG_LIST, srcAddr is the address of the head of the user-
 * constructed scatter-gather list (i.e., address of a struct dmaTxfrDesc).
 *
 * if useSG == NO_SG_LIST, srcAddr is the memory address of the source
 * data buffer.
 * -----------------------------------------------------------------------
 */
struct dmaRequest {
	int		 channel;			/* Which channel in use					*/
	txfr_t	 transferType;		/* memory-to-memory, memory-to-FIFO etc	*/
	usesg_t	 useSG;				/* scatter-gather list or not			*/
	UINT32	 srcAddr;			/* Source address for transfer data		*/
	UINT32	 destAddr;			/* Destination addr. for transfer data	*/
	int		 txferSize;			/* No. of words to transfer				*/
	MSG_Q_ID srcMsgQueue;		/* Caller's src. buffer msg Q or NULL	*/
	MSG_Q_ID dstMsgQueue;		/* Caller's dst. buffer msg Q or NULL	*/
};

/* 
 * -----------------------------------------------------------------------
 * Interrupt vectors for DMA channels.
 * These are fixed in the 405Gpr chip and cannot be changed.
 * -----------------------------------------------------------------------
 */
#define DMA0INTNUM 5
#define DMA1INTNUM 6
#define DMA2INTNUM 7
#define DMA3INTNUM 8

/* 
 * -----------------------------------------------------------------------
 * Macro to shift a bit into the desired position in a 32-bit word.
 * According to IBM, leftmost bit (MSB) is bit 0, LSB is bit 31.
 * -----------------------------------------------------------------------
 */
#define BITSET(bitnum) ((unsigned long)0x80000000L >> bitnum)

/* 
 * -----------------------------------------------------------------------
 * Macro to create a 32-bit word with a given bit cleared, all other set.
 * -----------------------------------------------------------------------
 */
#define BITCLR(bitnum) (~ BITSET(bitnum))

/* 
 * -----------------------------------------------------------------------
 * Define all the individual bits in a 32-bit word, for various uses.
 * -----------------------------------------------------------------------
 */
#define BIT0    0x80000000L
#define BIT1    0x40000000L
#define BIT2    0x20000000L
#define BIT3    0x10000000L
#define BIT4    0x08000000L
#define BIT5    0x04000000L
#define BIT6    0x02000000L
#define BIT7    0x01000000L
#define BIT8    0x00800000L
#define BIT9    0x00400000L
#define BIT10   0x00200000L
#define BIT11   0x00100000L
#define BIT12   0x00080000L
#define BIT13   0x00040000L
#define BIT14   0x00020000L
#define BIT15   0x00010000L
#define BIT16   0x00008000L
#define BIT17   0x00004000L
#define BIT18   0x00002000L
#define BIT19   0x00001000L
#define BIT20   0x00000800L
#define BIT21   0x00000400L
#define BIT22   0x00000200L
#define BIT23   0x00000100L
#define BIT24   0x00000080L
#define BIT25   0x00000040L
#define BIT26   0x00000020L
#define BIT27   0x00000010L
#define BIT28   0x00000008L
#define BIT29   0x00000004L
#define BIT30   0x00000002L
#define BIT31   0x00000001L

/* 
 * -----------------------------------------------------------------------
 * Define some WindView events for testing DMA routines.
 * Add more as needed.
 * -----------------------------------------------------------------------
 */
#define DMA_EVENT_BASE	120
#define DMA_START		(DMA_EVENT_BASE + 0)
#define DMA_DONE		(DMA_EVENT_BASE + 1)
#define DMA_TIMEOUT		(DMA_EVENT_BASE + 2)
#define DMA_SG_START	(DMA_EVENT_BASE + 3)
#define DMA_SG_DONE		(DMA_EVENT_BASE + 4)
#define DMA_SG_TIMEOUT	(DMA_EVENT_BASE + 5)
#define DMA_ISR_START	(DMA_EVENT_BASE + 6)
#define DMA_ISR_END		(DMA_EVENT_BASE + 7)

/* 
 * -----------------------------------------------------------------------
 * Various DMA-related compiler constants.
 * DMA_TSTACK				generic task stack size for DMA tasks
 * MIN_DMA_PRIORITY			minimum practical priority for DMA driver
 * RUNDMA_PRIORITY			task priority for runDMA task
 * SENDDMA_PRIORITY			task priority for sendDMATask
 * LOGMSG_PRIORITY			task priority for dmaLogMsg task
 * SEM_TIMEOUT_SECS			semaphore timeout constant
 * MSG_TIMEOUT_SECS			message queue timeout constant
 * CHANNEL_CHECK_FREQUENCY	No. times to check channel busy while waiting
 * INCREMENT				Increment-address flag value (sendDMATask)
 * NO_INCREMENT				Increment-address flag value (sendDMATask)
 * FIFO_CHECK_FREQUENCY		No. times/sec to check for something in FIFO
 * FIFO_WAIT_TIMEOUT_SECS	Timeout while waiting for something in FIFO
 * MIN_DATA_FIFO_COUNT		How much is "something in FIFO"
 * -----------------------------------------------------------------------
 */
#define DMA_TSTACK		4096
#define MIN_DMA_PRIORITY	20
#define SEM_TIMEOUT_SECS	60
#define MSG_TIMEOUT_SECS	60
#define CHANNEL_CHECK_FREQUENCY	4
#define INCREMENT       1
#define NO_INCREMENT    0
#define FIFO_CHECK_FREQUENCY    4
#define FIFO_WAIT_TIMEOUT_SECS  10
#define MIN_DATA_FIFO_COUNT     20

/* -------------------- DMA Info array in dmaDrv.c -------------------- */
#ifndef DMA_DRV_C
extern dmainfo_t dmaInfo[MAX_DMA_CHANNELS];
#endif

/* 
 * -----------------------------------------------------------------------
 * Prototypes for dmaDrv.c routines
 * -----------------------------------------------------------------------
 */
void		dmaInit(int ququeSize, int sqListSize);
void		dmaCleanup(void);
int		dmaGetChannel(int);
dmainfo_t  *dmaGetChannelInfo(int);
void		dmaFreeChannel(int);
void		dmaStart(int, usesg_t, txDesc_t *);
STATUS		dmaFIFO_out(int, usesg_t, UINT32, int, int);

sgnode_t   *dmaSGNodeCreate(UINT32, UINT32, UINT32);
void		dmaSGNodeAppend(sgnode_t *, txDesc_t *);

txDesc_t   *dmaSGListCreate(int);
STATUS		dmaSGListRemove(int, txDesc_t *);

STATUS		dmaSGQueueAppend(int, txDesc_t *);
STATUS		dmaQueueFree(int);

STATUS		dmaTxfrSetup(int, UINT32, UINT32, int);

void		runDMA(int, int, int, int, int, int, int, int, int, int);
void		sendDMATask(int, int, int, int, int, int, int, int, int, int);
void		endDMATask(int, int, int, int, int, int, int, int, int, int);
STATUS		sendDMA(int, txfr_t, usesg_t, UINT32, UINT32, int, MSG_Q_ID, MSG_Q_ID);
STATUS		dmaXfer(int, txfr_t, usesg_t, UINT32, UINT32, int, MSG_Q_ID, MSG_Q_ID);
void            execNextDmaRequestInQueue(int channel);
STATUS		queueDmaTransfer(int, txfr_t, usesg_t, UINT32, UINT32, int, MSG_Q_ID, MSG_Q_ID);

void		format_long(char *, long);

#ifdef __cplusplus
}
#endif		/* __cplusplus */
#endif		/* __INCdmadrvh */
