/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
//=========================================================================
// DDR_Globals.h:  global variables
//=========================================================================
#ifndef DDR_GLOBALS_H
#define DDR_GLOBALS_H

#include <stdlib.h>
#include <intLib.h>
#include <logLib.h>
#include <msgQLib.h>
#include <semLib.h>

#define STAND_ALONE 0

extern SEM_ID   ddr_msg_sem;
extern SEM_ID   scan_sem;
extern MSG_Q_ID scanMsgQueue;
extern MSG_Q_ID msgsFromDDR;
extern MSG_Q_ID pTestDataRdyMsgQ;
extern MSG_Q_ID pDspDataRdyMsgQ;
extern SEM_ID   pHostMsgMutex;

extern int ddr_run_mode;    // stand-alone mode
extern int ddr_debug;        // debug options
extern int total_ddr_msgs,total_host_msgs;
extern int test_id,test_value1,test_value2;
extern int scan_count,fid_count,fid_ct;
#endif
