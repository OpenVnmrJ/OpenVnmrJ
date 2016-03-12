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
#include <vxWorks.h>
#include <rngLib.h>
#include <msgQLib.h>
#include "logMsgLib.h"

/* NDDS addition */
#include "ndds/ndds_c.h"
#include "NDDS_Obj.h"

#include "mBufferLib.h"
#include "nameClBufs.h"
#include "dataObj.h"


/*
   System Globals 
*/

/* NDDS */
extern NDDS_ID NDDS_Domain;


/* global hostname used for generating NDDS topics
  and other tests 
*/
#define HOSTNAME_SIZE 80
extern char hostName[HOSTNAME_SIZE];
 
/* main cluster buffer allocation for the 'name buffers' */
extern MBUFFER_ID mbufID ;

/* the 'name buffers' based on the cluster buffers */
extern NCLB_ID nClBufId;
 
extern int  BrdType;    /* Type of Board, RF, Master, PFG, DDR, Gradient, Etc. */
extern int  BrdNum;     /* The Board types Ordinal number, i.e. rf1 or rf2 */


/* Debuggging level, greater the number the greater the diagnostic output */
extern int DebugLevel;

extern MSG_Q_ID pMsgesToAParser;/* MsgQ used for Msges to Acode Parser */

extern FIFOBUF_ID pCntrlFifoBuf;   /* PS Timing Control FIFO Buffer Object */
extern int cntrlFifoDmaChan;       /* device paced DMA channel for PS control FIFO */

/*  FID data upload globals  */
extern MSG_Q_ID pDspDataRdyMsgQ;    /* DSP -> dspDataXfer task */
extern DATAOBJ_ID pTheDataObject;   /* FID statblock, etc. */

extern int SystemRevId;
extern int InterpRevId;

