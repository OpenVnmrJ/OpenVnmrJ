/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef SRAMDEF
#define SRAMDEF
#define  MAXCHANNELS		6
#define  DF_SIZE		1
#define  LF_SIZE		2

/*  The offset is spelled out in units of 16-bit words.
    Most values require 1 such word; thus the default.
    The lock frequency requires 2 16-bit words		*/

#define  PTS_OFFSET		0
#define  LF_OFFSET		(PTS_OFFSET+MAXCHANNELS)
#define  H1F_OFFSET		(LF_OFFSET+LF_SIZE)
#define  SYS_OFFSET             (H1F_OFFSET+DF_SIZE)

#define  CS_OFFSET		(SYS_OFFSET+DF_SIZE)
#define  SRAM_SIZE		(CS_OFFSET+DF_SIZE)
#endif SRAMDEF
