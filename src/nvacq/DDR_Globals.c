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

#define VERIFY_OS // verify os code transfer

#include "DDR_Globals.h"

SEM_ID   ddr_msg_sem=0;
SEM_ID   scan_sem=0;
MSG_Q_ID scanMsgQueue=0;
MSG_Q_ID msgsFromDDR=0;
MSG_Q_ID pTestDataRdyMsgQ=0; // for stand-alone tests
SEM_ID   pHostMsgMutex = 0;

int ddr_run_mode=0;
int ddr_debug=0;        // debug options
int total_ddr_msgs=0;   // total 405->C67 messages
int total_host_msgs=0;  // total C67->405 messages
int test_id=0;
int test_value1=0;
int test_value2=0;
int scan_count=0;
int fid_count=0;       // FID count (itr)
int fid_ct=0;	       // ct count (itr)
int ddr_il_incr = 0;   // interleave cycle count
