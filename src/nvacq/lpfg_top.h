/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#ifndef __LPFG_TOP_H__
#define __LPFG_TOP_H__
#define LPFG_DATA_PORT_WIDTH 96
#define LPFG_SAMPLE_DATA_DEPTH 512
#define LPFG_CHIP_ID 13
#define LPFG_CHIP_REVISION 0
#define LPFG_NUM_OPB_MASTERS 1
#define LPFG_NUM_OPB_SLAVES 3
#define LPFG_NUM_INTERRUPT_CONTROLLERS 2
#define LPFG_PFG_NUM_CLKS_IN_2MSEC 166667
#define LPFG_LOCK_CTC_PULSE_WIDTH 16
#define LPFG_SYNC_INT_SIZE 4
#define LPFG_SYNC_INT_POS 3
#define LPFG_SYNC_INT_BITS ((((1<<(4))-1))<<3)
#define LPFG_PFG_REGISTER_BASE (0x00000)
#define LPFG_LOCK_REGISTER_BASE (0x80000)
#define LPFG_LOCK_ADC_SAMPLE0_BASE (0x90000)
#define LPFG_LOCK_ADC_SAMPLE1_BASE (0xa0000)
#define LPFG_LOCK_DDS_BASE (0x100000)
#define LPFG_DATA_FIFO_WIDTH (26+3*16+12+1+3)
#define LPFG_LOCK_ADC_SAMPLE_PTR_SIZE log2(5120)
#define LPFG_LOCK_ADC_BUFFER_SIZE 5120
#define LPFG_FIFO_DURATION_WIDTH 26
#define LPFG_FIFO_DURATION_HOLDING_WIDTH 26
#define LPFG_PFG_AMP_WIDTH 16
#define LPFG_PFG_AMP_HOLDING_WIDTH 16
#define LPFG_PFG_GATES_WIDTH 12
#define LPFG_PFG_GATES_HOLDING_WIDTH 12
#endif
